#include <vector>
#include <map>
#include <functional>
#include <exception>
#include <type_traits>

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/archive/binary_oarchive.hpp"

#include "jansson.h"
#include "gtest/gtest.h"
#include "common/IDebugLog.h"

#include "jcontainers_constants.h"
#include "util/singleton.h"
#include "util/util.h"
#include "util/istring.h"
#include "iarchive_with_blob.h"

#include "object/object_context.h"
#include "domains/domain_master.h"


#include "domains/domain_master_serialization.h"

namespace domain_master {


    namespace {

        auto get_domains_from_fs() -> std::set<util::istring> {
            namespace fs = boost::filesystem;
            std::set<util::istring> domains;
            fs::path dir = util::relative_to_dll_path(JC_DATA_FILES "Domains/");
            for (fs::directory_iterator itr(dir), end; itr != end; ++itr) {
                auto f = itr->path ().filename ().generic_string ();
                if (f != ".force-install")
                    domains.insert(f.c_str());
            }
            return domains;
        }

        /*template<class List>
        auto construct_domains(List&& list) -> std::map<util::istring, context*> {
            std::map<util::istring, context*> contexts;

            for (auto& name : list) {
                contexts[name.c_str()] = new context();
            }
            return contexts;
        }*/

        template<class Func, class ...Args>
        auto invoke_for_all(master& ths, Func&& f, Args && ...args) -> void {
            f(ths.get_default_domain(), std::forward<Args>(args) ...);
            for (auto& pair : ths.active_domains_map()) {
                f(*pair.second, std::forward<Args>(args) ...);
            }
        }

        ////


        struct activity_stopper {
            master& _m;
            activity_stopper(master& m) : _m(m) {
                invoke_for_all(m, std::mem_fn(&collections::object_context::stop_activity));
            }
            ~activity_stopper() {
                invoke_for_all(_m, std::mem_fn(&collections::object_context::start_activity));
            }
        };


        auto u_clearState(master& ths) -> void {
            ths.get_form_observer().u_clearState();
            invoke_for_all(ths, std::mem_fn(&context::u_clearState));
        }

        auto u_print_stats(master& self) -> void {
            self.get_form_observer().u_print_status();

            //invoke_for_all(self, [](context& d) {
                JC_log("Default domain");
                self.get_default_domain().u_print_stats();
            //});

            for (auto& pair : self.active_domains_map()) {
                JC_log("Domain: %s", pair.first.c_str());
                pair.second->u_print_stats();
            }
        }

        auto u_delete_inactive_domains(master& self) -> void {
            util::tree_erase_if(self.active_domains_map(), [&](const master::DomainsMap::value_type& p) {
                return self.active_domain_names.find(p.first) == self.active_domain_names.end();
            });
        }

    }

    namespace {
        template<class T, class D>
        inline std::unique_ptr<T, D> make_unique_ptr(T* data, D destr) {
            return std::unique_ptr<T, D>(data, destr);
        }

        using serialization_version = collections::serialization_version;

        struct header {

            serialization_version commonVersion;

            static header imitate_old_header() {
                return{ serialization_version::no_header };
            }

            static header make() {
                return{ serialization_version::current };
            }

            static const char *common_version_key() { return "commonVersion"; }

            static header read_from_stream(std::istream & stream) {

                uint32_t hdrSize = 0;
                stream >> hdrSize;

                std::vector<char> buffer(hdrSize);
                stream.read(&buffer.front(), buffer.size());

                auto js = make_unique_ptr(json_loadb(&buffer.front(), buffer.size(), 0, nullptr), &json_decref);
                if (!js) { // parsing failed
                    return imitate_old_header();
                }

                return{ (serialization_version)json_integer_value(json_object_get(js.get(), common_version_key())) };
            }

            static auto write_to_json() -> decltype(make_unique_ptr((json_t *)nullptr, &json_decref)) {
                auto header = make_unique_ptr(json_object(), &json_decref);

                json_object_set(header.get(), common_version_key(), json_integer((json_int_t)serialization_version::current));

                return header;
            }

            static void write_to_stream(std::ostream & stream) {
                auto header = write_to_json();
                auto data = make_unique_ptr(json_dumps(header.get(), 0), free);

                uint32_t hdrSize = strlen(data.get());
                stream << (uint32_t)hdrSize;
                stream.write(data.get(), hdrSize);
            }
        };

        auto read_from_stream(master& self, std::istream& stream) -> void {
            //_context.read_from_stream(s);

            stream.flags(stream.flags() | std::ios::binary);

#       if 0
            std::ofstream file("dump", std::ios::binary | std::ios::out);
            std::copy(
                std::istreambuf_iterator<char>(stream),
                std::istreambuf_iterator<char>(),
                std::ostreambuf_iterator<char>(file)
                );
            file.close();
#       endif

            activity_stopper stopper{ self };
            {
                // i have assumed that Skyrim devs are not idiots to run scripts in process of save game loading
                //write_lock g(_mutex);

                u_clearState(self);

                if (stream.peek() != std::istream::traits_type::eof()) {

                    try {

                        auto hdr = header::read_from_stream(stream);
                        bool isNotSupported = serialization_version::current < hdr.commonVersion
                            || hdr.commonVersion <= serialization_version::no_header;

                        if (isNotSupported) {
                            std::ostringstream error;
                            error << "Unable to load serialized data of version " << (int)hdr.commonVersion
                                << ". Current serialization version is " << (int)serialization_version::current;
                            throw std::logic_error(error.str());
                        }

                        {
                            hack::iarchive_with_blob real_archive(stream, self.get_default_domain(), self.get_default_domain());
                            boost::archive::binary_iarchive& archive = real_archive;

                            // (stream) -> [(name,context)]

                            if (hdr.commonVersion <= serialization_version::pre_dyn_form_watcher) {
                                self.get_default_domain().load_data_in_old_way(archive);
                            }
                            else {
                                archive >> self;
                            }
                        }

                        u_delete_inactive_domains(self);

                        invoke_for_all(self, std::mem_fn(&context::u_postLoadInitializations));
                        invoke_for_all(self, std::mem_fn(&context::u_applyUpdates), hdr.commonVersion);
                        invoke_for_all(self, std::mem_fn(&context::u_postLoadMaintenance), hdr.commonVersion);
                    }
                    catch (const std::exception& exc) {
                        _FATALERROR("caught exception (%s) during archive load - '%s'",
                            typeid(exc).name(), exc.what());
                        u_clearState(self);

                        // force whole app to crash
                        // jc_assert(false);
                    }
                    catch (...) {
                        _FATALERROR("caught unknown (non std::*) exception");
                        u_clearState(self);

                        // force whole app to crash
                        //jc_assert(false);
                    }
                }

                u_print_stats(self);
            }

        }

        auto write_to_stream(master& self, std::ostream& stream) -> void {
            stream.flags(stream.flags() | std::ios::binary);

            activity_stopper s{ self };
            {
                // we can also cleanup objects here
                {
                    self.get_form_observer().u_remove_expired_forms();
                }

                header::write_to_stream(stream);
                boost::archive::binary_oarchive arch{ stream };

                // [(name, domain)] -> stream

                arch << self;

                u_print_stats(self);
            }
        }


    }

    context& master::get_or_create_domain_with_name(const util::istring& name)
    {
        auto itr = _domains.find(name);
        if (itr != _domains.cend()) {
            return *itr->second;
        }

        auto dom = std::make_shared<context>(this->get_form_observer());
        _VMESSAGE("Created domain %s %p", name.c_str(), dom.get());
        _domains.emplace(DomainsMap::value_type{ name, dom });
        return *dom;
    }

    context* master::get_domain_if_active(const util::istring& name)
    {
        return active_domain_names.find(name) != active_domain_names.end()
            ? &get_or_create_domain_with_name(name) : nullptr;
    }

    context& master::get_default_domain()
    {
        return _default_domain;
    }

    master::DomainsMap& master::active_domains_map()
    {
        return _domains;
    }

    form_observer& master::get_form_observer()
    {
        return _form_watcher;
    }

    namespace {
        util::singleton<master, false> g_domain_master_singleton{
            []() {
                auto m = new master();
                m->active_domain_names = get_domains_from_fs();
                return m;
            }
        };
    }

    master& master::instance() {
        return g_domain_master_singleton.get();
    }

    void master::clear_state() {
        activity_stopper s{ *this };
        u_clearState(*this);
        u_delete_inactive_domains(*this);
    }

    void master::read_from_stream(std::istream& s) {
        domain_master::read_from_stream(*this, s);
    }

    void master::write_to_stream(std::ostream& s) {
        domain_master::write_to_stream(*this, s);
    }

    namespace testing {

        TEST(master, get_or_create_domain_with_name)
        {
            ::domain_master::master m;
            EXPECT_TRUE(m.active_domains_map().empty());
            m.get_or_create_domain_with_name("help");
            EXPECT_FALSE(m.active_domains_map().empty());
        }

        TEST(master, u_delete_inactive_domains)
        {
            ::domain_master::master m;
            EXPECT_TRUE(m.active_domains_map().empty());
            m.get_or_create_domain_with_name("help");
            EXPECT_FALSE(m.active_domains_map().empty());

            u_delete_inactive_domains(m);
            EXPECT_TRUE(m.active_domains_map().empty());
        }

        /*
        TEST(master, backward_compatibility)
        {
            namespace fs = boost::filesystem;

            fs::path dir = util::relative_to_dll_path("test_data/backward_compatibility");
            bool atLeastOneTested = false;

            for (fs::directory_iterator itr(dir), end; itr != end; ++itr) {
                if (fs::is_regular_file(*itr)) {
                    atLeastOneTested = true;

                    std::ifstream file(itr->path().generic_string(), std::ios::in | std::ios::binary);

                    ::domain_master::master m;
                    m.read_from_stream(file);

                    EXPECT_TRUE(m.get_default_domain().object_count() > 100); // dumb assumption

                    std::stringstream stream;
                    m.write_to_stream(stream);

                    std::stringstream n{ stream.str() };
                    m.read_from_stream(n);

                    //abort()

                    EXPECT_TRUE(m.get_default_domain().object_count() > 100);
                }
            }

            EXPECT_FALSE(atLeastOneTested);
        }
        */
    }
}


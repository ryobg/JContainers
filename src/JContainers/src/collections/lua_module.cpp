
#include "lua_module.h"

#include <boost/noncopyable.hpp>
//#include <boost/thread/tss.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lockfree/queue.hpp>

#include <utility>
#include <string>
#include <sstream>
#include <mutex>
#include <algorithm>
#include <thread>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "meta.h"
#include "util/util.h"
#include "gtest.h"
#include "util/spinlock.h"
#include "reflection/reflection.h"
#include "jcontainers_constants.h"

#include "collections/collections.h"
#include "collections/context.h"
#include "collections/functions.h"
#include "collections/access.h"

// Module imports:

namespace lua {
    using tes_context = collections::tes_context;
    using tes_context_standalone = collections::tes_context_standalone;

    using object_base = collections::object_base;

    using item = collections::item;
    using item_type = collections::item_type;
    using FormId = collections::FormId;

    namespace cl = collections;
}

#include "lua_native_funcs.hpp"

namespace lua { namespace aux_wip {

    using namespace api;

    class context final : public boost::noncopyable {

        lua_State *_lua = nullptr;
        tes_context& _context;

    public:

        lua_State *state() const {
            return _lua;
        }

        explicit context(tes_context& context) : _context(context) {
            reopen_if_closed();
            JC_log("Lua context created");
        }

        ~context() {
            close();
            JC_log("Lua context destructed");
        }

        boost::optional<item> eval_lua_function(object_base *object, const char *lua_string) {

            assert(lua_string);

            lua_pushcfunction(_lua, LuaErrorHandler);
            int errorHandler = lua_gettop(_lua);

            lua_getglobal(_lua, "JC_compileAndRun");
            lua_pushstring(_lua, lua_string);
            lua_pushlightuserdata(_lua, object);
            enum { num_args = 2, returned = 1 };

            if (lua_pcall(_lua, num_args, returned, errorHandler) != LUA_OK) {
                JC_log ("Lua string: %s", lua_string);
                return boost::none;
            }
            else {
                item result;
                auto val = reinterpret_cast<const JCValue *>(lua_topointer(_lua, -1));
                JCValue_fillItem(_context, val, result);
                return result;
            }
        }

        void reopen_if_closed() {
            int seconds;
            for (seconds = -1; !_lua && seconds < 180; seconds++) {
                if ((_lua = luaL_newstate ())) {
                    if (seconds < 0)
                        JC_log ("reopen_if_closed(): initial Lua allocation failed; retrying up to 3 minutes (32-bit or low memory)");
                    std::this_thread::sleep_for (std::chrono::milliseconds (1000));
                }
            }

            if (seconds > 0)
                JC_log ("reopen_if_closed() delayed %d seconds while trying to allocate memory", seconds);
            if (!_lua)
                JC_log ("reopen_if_closed() timed out while trying to allocate LuaJIT memory; subsequent Lua operations may crash");

            luaL_openlibs (_lua);
            setupLuaContext (_lua, _context);
        }

        void close() {
            if (_lua) {
                auto lua = _lua;
                _lua = nullptr;
                lua_close(lua);
            }
        }

    private:

        static const char* top_string(lua_State *l) {
            if (lua_gettop(l) > 0 && lua_isstring(l, -1)) {
                return lua_tostring(l, -1);
            }
            return "<can't obtain string>";
        }

        static void print_top_string(lua_State *l, const char* preambula) {
            JC_log("Lua %s: %s", preambula, top_string(l));
        }

        static bool setupLuaContext(lua_State *l, tes_context& context) {
            typedef boost::filesystem::path path;

            auto initScriptPath = util::relative_to_dll_path(JC_DATA_FILES "InternalLuaScripts/init.lua");

            lua_pushcfunction(l, LuaErrorHandler);
            int errorHandler = lua_gettop(l);

            if (luaL_loadfile(l, initScriptPath.generic_string().c_str()) != LUA_OK) {
                print_top_string(l, "unable to load init.lua and etc");
                return false;
            }

            // push data path, dll path
            lua_pushstring(l, util::relative_to_dll_path(JC_DATA_FILES).generic_string().c_str());
            lua_pushstring(l, util::dll_path().generic_string().c_str());
            lua_pushlightuserdata(l, &context);
            enum { num_lua_args = 3 }; // DONT FORGET ME!!

            if (lua_pcall(l, num_lua_args, LUA_MULTRET, errorHandler) != LUA_OK) {
                print_top_string(l, "unable to execute init.lua and etc");
                return false;
            }

            return true;
        }

        static int LuaErrorHandler(lua_State *l) {

            int message = lua_gettop(l);//1
            print_top_string(l, "error");

            lua_getglobal(l, "debug");//2
            lua_pushstring(l, "traceback");
            lua_gettable(l, 2);
            //int function = lua_gettop(l);

            lua_call(l, 0, 1);

            print_top_string(l, "trace");

            lua_pushvalue(l, message);
            return 1;
        }
    };

    // just a pool, factory of contexts.
    // any thead can obtain free (or newly created), initialized lua-context
    // the tread have to return it back via @release
    class context_pool final : public collections::dependent_context, boost::noncopyable {
        static const uint32_t queue_capacity = 16;

        boost::lockfree::queue<context*> _queue { queue_capacity };
        tes_context& _tcontext;
        std::atomic_int32_t _aquired_count = 0;

    public:

        context& aquire() {
            ++_aquired_count;

            context *ctx = nullptr;

            if (!_queue.pop(ctx)) {
                ctx = new context(_tcontext);
            }

            assert(ctx);
            return *ctx;
        }

        void release(context& ctx) {
            --_aquired_count;

            _queue.push(&ctx);
        }

        explicit context_pool(tes_context& tc)
            : _tcontext(tc)
        {
            tc.add_dependent_context(*this);
        }

        ~context_pool() {
            _tcontext.remove_dependent_context(*this);
            clear();
        }

        void clear_state() override {
            clear();
        }

    private:

        void clear() {
            warn_if_aquired();
            _queue.consume_all([](context *ctx) {
                assert(ctx);
                delete ctx;
            });
        }

        void warn_if_aquired() {
            jc_assert_msg(_aquired_count == 0, "Lua: %u lua-contexts are still active and used", _aquired_count.load ());
        }
    };

    // aquires a context from context_pool and releases it when destroyed
    class autofreed_context final : boost::noncopyable {
        context_pool& _pool;
        context& _context;
    public:
        explicit autofreed_context(context_pool& pool)
            : _pool(pool)
            , _context(pool.aquire())
        {}

        ~autofreed_context() {
            _pool.release(_context);
        }

        context* operator -> () { return &_context; }
        context& context() { return _context; }
    };


    struct fixture : public ::testing::Test {
        tes_context_standalone tc;
        context_pool pool;
        fixture() : pool(tc) {}
    };

#if 1

    TEST_F(fixture, Lua_trtr)
    {
        std::atomic_int8_t stop = 0;

        auto task = [&]() {
            while (stop.load(std::memory_order_acquire) == 0) {
                EXPECT_TRUE(autofreed_context(pool)->eval_lua_function(nullptr, "return testing.perform()")->intValue() != 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        };

        std::thread t1(task), t2(task);
        std::this_thread::sleep_for(std::chrono::seconds(5));
        stop.store(1, std::memory_order_release);

        t1.join();
        t2.join();
    }

    TEST_F(fixture, Lua_evalLua)
    {
        autofreed_context lc(pool);

        auto testTransporting = [&](const char *str) { return lc->eval_lua_function(nullptr, str); };

        EXPECT_TRUE(*testTransporting("return 10") == 10.0f);
        EXPECT_TRUE(*testTransporting("return 'die'") == std::string("die"));
        EXPECT_TRUE(*testTransporting("return Form(20)") == cl::make_weak_form_id(FormId(20), tc));


        auto& db = tc.root();
        EXPECT_TRUE(*testTransporting("return JDB") == db.base());

        db.set("test", 10);
        EXPECT_TRUE(db["test"] == 10);

        auto result = lc->eval_lua_function(nullptr,
            R"===(for k,v in pairs(JDB) do print('JDB.test', k, v) end;
            assert(JDB.test == 10);
            return JDB.test)==="
            );
        EXPECT_TRUE(result.is_initialized());
        EXPECT_TRUE(*result == 10.f);

    }

    TEST_F(fixture, Lua_launch_all_lua_tests)
    {
        EXPECT_TRUE(autofreed_context(pool)->eval_lua_function(nullptr, "return testing.perform()")->intValue() != 0);
    }
#endif
}
}

namespace lua {

    boost::optional<item> eval_lua_function(tes_context& ctx, object_base *object, const char *lua_string) {
        auto pool = static_cast<aux_wip::context_pool*>(ctx.lua_context.get());
        return aux_wip::autofreed_context(*pool)->eval_lua_function(object, lua_string);
    }

    static tes_context::post_init g_extender([](tes_context& ctx){
        ctx.lua_context = std::make_shared<aux_wip::context_pool>(ctx);
    });
}

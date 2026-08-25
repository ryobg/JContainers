#pragma once

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "util/util.h"
#include "util/stl_ext.h"

namespace collections {

/// Redefine in each logging module
#undef  JC_LOG_API_SOURCE
#define JC_LOG_API_SOURCE "JString"

	extern boost::optional<std::vector<std::string>> wrap_string(const char *csource, int charsPerLine);

    class tes_string : public reflection::class_meta_mixin_t<tes_string> {
    public:

        tes_string() {
            metaInfo._className = "JString";
            metaInfo.comment = "various string utility methods";
        }

        static object_base * wrap(tes_context& ctx, const char* source, SInt32 charsPerLine)
        {
            JC_LOG_API ("..., %d", charsPerLine);

			auto strings = wrap_string(source, charsPerLine);

            if (!strings) {
                return nullptr;
            }

            return &array::objectWithInitializer([&](array &obj) {

                for (auto& str : *strings) {
                    obj.u_container().emplace_back(std::move(str));
                }
            },
                ctx);
        }
        REGISTERF2(wrap, "sourceText charactersPerLine=60",
"Breaks source text onto set of lines of almost equal size.\n\
Returns JArray object containing lines.\n\
Accepts ASCII and UTF-8 encoded strings only");

        static UInt32 stoul (const char* num_string, SInt32 base = 16)
        {
            JC_LOG_API ("%s", num_string);
            if (!num_string)
                return 0;
            try {
                return std::stoul (num_string, nullptr, base);
            }
            catch (const std::exception&) {
                return 0;
            }
        }
        REGISTERF2_STATELESS (stoul, "num_string base=16", "Converts a base string into unsigned integer");

        static UInt32 decodeFormStringToFormId(const char* form_string) {
            JC_LOG_API ("%s", form_string);
            return util::to_integral(decodeFormStringToForm(form_string));
        }
        static FormId decodeFormStringToForm (const char* form_string) {
            JC_LOG_API ("%s", form_string);
            return forms::string_to_form (form_string).value_or (FormId::Zero);
        }
        static skse::string_ref encodeFormToString (FormId id) {
            JC_LOG_API ("0x%x", id);
            return skse::string_ref { forms::form_to_string (id).value_or ("") };
        }
        static skse::string_ref encodeFormIdToString(UInt32 id) {
            JC_LOG_API ("0x%x", id);
            return encodeFormToString( util::to_enum<FormId>(id) );
        }

        REGISTERF2_STATELESS(decodeFormStringToFormId, "formString", "FormId|Form <-> \"__formData|<pluginName>|<lowFormId>\"-string converisons");
        REGISTERF2_STATELESS(decodeFormStringToForm, "formString", "");
        REGISTERF2_STATELESS(encodeFormToString, "value", "");
        REGISTERF2_STATELESS(encodeFormIdToString, "formId", "");

    private:
        static boost::uuids::random_generator generateUUID_gen;
        static util::spinlock generateUUID_lock;

    public:
        static std::string generateUUID ()
        {
            JC_LOG_API ("");
            auto uuid = [] {
                std::lock_guard<util::spinlock> guard (generateUUID_lock);
                return generateUUID_gen ();
            } ();
            return boost::uuids::to_string (uuid);
        }
        REGISTERF2_STATELESS(generateUUID, "", "Generates random uuid-string like 2e80251a-ab22-4ad8-928c-2d1c9561270e");
    };

    boost::uuids::random_generator  tes_string::generateUUID_gen;
    util::spinlock                  tes_string::generateUUID_lock;


    TES_META_INFO(tes_string);

    TEST(tes_string, wrap)
    {
        tes_context_standalone ctx;

        auto testData = json_deserializer::json_from_file(
            util::relative_to_dll_path("test_data/tes_string/string_wrap.json").generic_string().c_str() );
        EXPECT_TRUE( json_is_array(testData.get()) );

		auto testWrap = [&](const char *string, int linesCount, int charsPerLine) {
            auto obj = tes_string::wrap(ctx, string, charsPerLine);
            if (linesCount == -1) {
                EXPECT_NIL(obj);
            }
            else {
                EXPECT_NOT_NIL(obj);
                EXPECT_TRUE(obj->s_count() >= linesCount);
            }
		};

        size_t index = 0;
        json_t *value = nullptr;
        json_array_foreach(testData.get(), index, value) {
            int charsPerLine = -1;
            json_t *jtext = nullptr;
            int linesCountMinimum = -1;

            json_error_t error;
            int succeed = json_unpack_ex(value, &error, 0,
                "{s:i, s:o, s:i}", "charsPerLine", &charsPerLine, "text", &jtext, "linesCountMinimum", &linesCountMinimum);
            EXPECT_TRUE(succeed == 0);

            testWrap(json_string_value(jtext), linesCountMinimum, charsPerLine);
        }
    }

    TEST(tes_string, generateUUID)
    {
        auto uidString = tes_string::generateUUID();
        EXPECT_FALSE(uidString.empty());

        auto uidString2 = tes_string::generateUUID();
        EXPECT_NE(uidString, uidString2);
    }

    TEST (tes_string, stoul)
    {
        auto res = tes_string::stoul (nullptr);
        EXPECT_EQ (res, 0u);

        res = tes_string::stoul ("0");
        EXPECT_EQ (res, 0u);

        res = tes_string::stoul ("1");
        EXPECT_EQ (res, 1u);

        res = tes_string::stoul ("A");
        EXPECT_EQ (res, 10u);

        res = tes_string::stoul ("F");
        EXPECT_EQ (res, 15u);

        res = tes_string::stoul ("10");
        EXPECT_EQ (res, 16u);

        res = tes_string::stoul ("0xFFFF");
        EXPECT_EQ (res, 65535u);
    }
}

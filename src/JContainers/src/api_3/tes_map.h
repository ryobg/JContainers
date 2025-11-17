namespace tes_api_3 {

/// Redefine in each logging module
#undef  JC_LOG_API_SOURCE
#define JC_LOG_API_SOURCE "JMap"

    using namespace collections;


    template<class Key
        , class Cnt
        , class key_ref = Key &
        , class key_cref = const key_ref
    >
    class tes_map_t : public class_meta< tes_map_t<Key, Cnt, key_ref, key_cref> > {
    public:

        using map_functions = map_functions_templ < Cnt >;
        using map_type = Cnt;
        using tes_key = reflection::binding::convert_to_tes_type<typename map_type::key_type>;

        typedef typename Cnt* ref;

        tes_map_t() {
            metaInfo.comment = "Associative key-value container.\n"
                "Inherits JValue functionality";
        }

        REGISTERF(tes_object::object<Cnt>, "object", "", kCommentObject);

        template<class T>
        static T getItem(tes_context& ctx, ref obj, key_cref key, T def = default_value<T>()) {
            JC_LOG_API ("%p, ..., ...", (void*) obj);
            map_functions::doReadOp(obj, key, [&](item& itm) { def = itm.readAs<T>(); });
            return def;
        }
        REGISTERF(getItem<SInt32>, "getInt", "object key default=0", "Returns the value associated with the @key. If not, returns @default value");
        REGISTERF(getItem<Float32>, "getFlt", "object key default=0.0", "");
        REGISTERF(getItem<skse::string_ref>, "getStr", "object key default=\"\"", "");
        REGISTERF(getItem<object_base*>, "getObj", "object key default=0", "");
        REGISTERF(getItem<form_ref>, "getForm", "object key default=None", "");

        template<class T>
        static void setItem(tes_context& ctx, ref obj, key_cref key, T val) {
            JC_LOG_API ("%p, ..., ...", (void*) obj);
            map_functions::doWriteOp(obj, key, [&](item& itm) { itm = val; });
        }
        REGISTERF(setItem<SInt32>, "setInt", "* key value", "Inserts @key: @value pair. Replaces existing pair with the same @key");
        REGISTERF(setItem<Float32>, "setFlt", "* key value", "");
        REGISTERF(setItem<const char *>, "setStr", "* key value", "");
        REGISTERF(setItem<object_base*>, "setObj", "* key container", "");
        REGISTERF(setItem<form_ref>, "setForm", "* key value", "");

        template<class T>
        static T insertItem (tes_context& ctx, ref obj, key_cref key, T val)
        {
            JC_LOG_API ("%p, ..., ...", (void*) obj);
            if (obj && map_key_checker::check (key))
            {
                object_lock g (obj);
                if (item* i = obj->u_get (key))
                {
                    return i->readAs<T> ();
                }
                obj->u_get_or_create (key) = val;
                return val;
            }
            return default_value<T> ();
        }
        REGISTERF (insertItem<SInt32>, "insertInt", "* key value", "Inserts @key: @value pair. Does nothing if the @key already exists");
        REGISTERF (insertItem<Float32>, "insertFlt", "* key value", "");
        REGISTERF (insertItem<skse::string_ref>, "insertStr", "* key value", "");
        REGISTERF (insertItem<object_base*>, "insertObj", "* key container", "");
        REGISTERF (insertItem<form_ref>, "insertForm", "* key value", "");

        static bool hasKey(tes_context& ctx, ref obj, key_cref key) {
            JC_LOG_API ("%p, ...", (void*) obj);
            return valueType(ctx, obj, key) != 0;
        }
        REGISTERF2(hasKey, "* key", "Returns true, if the container has @key: value pair");

        static SInt32 valueType(tes_context& ctx, ref obj, key_cref key) {
            JC_LOG_API ("%p, ...", (void*) obj);
            auto type = item_type::no_item;
            map_functions::doReadOp(obj, key, [&](item& itm) { type = itm.type(); });
            return (SInt32)type;
        }
        REGISTERF2(valueType, "* key", "Returns type of the value associated with the @key.\n"VALUE_TYPE_COMMENT);

        static object_base* allKeys(tes_context& ctx, ref obj)
        {
            JC_LOG_API ("%p", (void*) obj);

            if (!obj) {
                return nullptr;
            }

            return &array::objectWithInitializer([&](array &arr) {
                object_lock g(obj);

                arr._array.reserve(obj->u_count());
                for each(auto& pair in obj->u_container()) {
                    arr.u_container().emplace_back(pair.first);
                }
            },
                ctx);
        }
        REGISTERF(allKeys, "allKeys", "*", "Returns a new array containing all keys");

        static VMResultArray<tes_key> allKeysPArray(tes_context& ctx, ref obj)
        {
            JC_LOG_API ("%p", (void*) obj);

            if (!obj) {
                return VMResultArray<tes_key>();
            }

            VMResultArray<tes_key> keys;
            object_lock l(obj);
            keys.reserve(obj->u_count());
            std::transform(obj->u_container().begin(), obj->u_container().end(),
                std::back_inserter(keys),
                [&ctx](const typename map_type::value_type& p) {
                    return reflection::binding::get_converter<typename map_type::key_type>::convert2Tes(p.first);
                }
            );

            return keys;
        }
        REGISTERF2(allKeysPArray, "*", "");

        static object_base* allValues(tes_context& ctx, ref obj)
        {
            JC_LOG_API ("%p", (void*) obj);

            if (!obj) {
                return nullptr;
            }

            return &array::objectWithInitializer([&](array &arr) {
                object_lock g(obj);

                arr._array.reserve(obj->u_count());
                for each(auto& pair in obj->u_container()) {
                    arr._array.push_back(pair.second);
                }
            },
                ctx);
        }
        REGISTERF(allValues, "allValues", "*", "Returns a new array containing all values");

        static bool removeKey(tes_context& ctx, ref obj, key_cref key)
        {
            JC_LOG_API ("%p, ...", (void*) obj);

            if (obj) {
                return obj->erase(key);
            }
            return false;
        }
        REGISTERF(removeKey, "removeKey", "* key", "Removes the pair from the container where the key equals to the @key");

        static SInt32 count(tes_context& ctx, ref obj)
        {
            JC_LOG_API ("%p", (void*) obj);

            if (!obj) {
                return 0;
            }

            return obj->s_count();
        }
        REGISTERF2(count, "*", "Returns count of pairs in the conainer");

        static void clear(tes_context& ctx, ref obj)
        {
            JC_LOG_API ("%p", (void*) obj);

            if (!obj) {
                return;
            }

            obj->s_clear();
        }
        REGISTERF2(clear, "*", "Removes all pairs from the container");

        static void addPairs(tes_context& ctx, ref obj, const ref source, bool overrideDuplicates)
        {
            JC_LOG_API ("%p, %p, %d", (void*) obj, (void*) source, int (overrideDuplicates));

            if (!obj || !source || source == obj) {
                return;
            }

            object_lock g(obj);
            object_lock c(source);

            if (overrideDuplicates) {
                for (const auto& pair : source->u_container()) {
                    obj->u_container()[pair.first] = pair.second;
                }
            }
            else {
                obj->u_container().insert(source->u_container().begin(), source->u_container().end());
            }
        }
        REGISTERF2(addPairs, "* source overrideDuplicates", "Inserts key-value pairs from the source container");

        void additionalSetup();

        //////////////////////////////////////////////////////////////////////////

        static Key nextKey(tes_context& ctx, ref obj, key_cref previousKey, key_ref endKey) {
            return map_functions::nextKey_forPapyrus(obj, previousKey, endKey);
        }

        static Key getNthKey(tes_context& ctx, ref obj, SInt32 keyIndex) {
            Key ith;
            map_functions::getNthKey(obj, keyIndex, [&](const typename Cnt::key_type& key) { ith = key; });
            return ith;
        }
    };

    typedef tes_map_t<const char*, map, const char*, const char*> tes_map;
    typedef tes_map_t<form_ref_lightweight, form_map, form_ref_lightweight, form_ref_lightweight> tes_form_map;
    typedef tes_map_t<int32_t, integer_map, int32_t, int32_t> tes_integer_map;

    void tes_map::additionalSetup() {
        metaInfo._className = "JMap";
    }

    void tes_form_map::additionalSetup() {
        metaInfo._className = "JFormMap";
    }

    void tes_integer_map::additionalSetup() {
        metaInfo._className = "JIntMap";
    }

    TES_META_INFO(tes_map);
    TES_META_INFO(tes_form_map);
    TES_META_INFO(tes_integer_map);

    //////////////////////////////////////////////////////////////////////////

    const char *tes_map_nextKey_comment =
R"===(Simplifies iteration over container's contents.
Accepts the @previousKey, returns the next key.
If @previousKey == @endKey the function returns the first key.
The function always returns so-called 'valid' keys (the ones != @endKey).
The function returns @endKey ('invalid' key) only once to signal that iteration has reached its end.
In most cases, if the map doesn't contain an invalid key ("" for JMap, None form-key for JFormMap)
it's ok to omit the @endKey.

Usage:

    string key = JMap.nextKey(map, previousKey="", endKey="")
    while key != ""
      <retrieve values here>
      key = JMap.nextKey(map, key, endKey="")
    endwhile
)===";

    struct tes_map_ext : class_meta < tes_map_ext > {
        REGISTER_TES_NAME("JMap");
        template<class Key>
        static Key nextKey(tes_context& ctx, map* obj, const char* previousKey = "", const char * endKey = "") {
            Key str(endKey);
            map_functions::nextKey(obj, previousKey, [&](const std::string& key) { str = key.c_str(); });
            return str;
        }
        REGISTERF(nextKey<skse::string_ref>, "nextKey", STR(* previousKey="" endKey=""), tes_map_nextKey_comment);

        static const char * getNthKey_comment() { return "Retrieves N-th key. " NEGATIVE_IDX_COMMENT "\nWorst complexity is O(n/2)"; }

        template<class Key>
        static Key getNthKey(tes_context& ctx, map* obj, SInt32 keyIndex) {
            Key ith;
            map_functions::getNthKey(obj, keyIndex, [&](const std::string& key) { ith = key.c_str(); });
            return ith;
        }
        REGISTERF(getNthKey<skse::string_ref>, "getNthKey", "* keyIndex", getNthKey_comment());
    };

    struct tes_form_map_ext : class_meta < tes_form_map_ext > {
        REGISTER_TES_NAME("JFormMap");
        REGISTERF(tes_form_map_ext::nextKey, "nextKey", STR(* previousKey=None endKey=None), tes_map_nextKey_comment);
        REGISTERF(tes_form_map::getNthKey, "getNthKey", "* keyIndex", tes_map_ext::getNthKey_comment());

        struct KeyCompareForNextKey {
            template<class K1, class K2>
            bool operator()(const K1& nKey, const K2& endKey) const {
                return skse::lookup_form(nKey.get()) == skse::lookup_form(endKey.get());
            }
        };

        static form_ref_lightweight nextKey(tes_context& ctx, form_map* obj
            , const form_ref_lightweight& previousKey
            , const form_ref_lightweight& endKey)
        {
            // @KeyCompare predicate customizes @nextKey_forPapyrus function
            // so that the function will not return unloaded (None) form keys at Papyrus level
            return map_functions_templ < form_map >::nextKey_forPapyrus(obj, previousKey, endKey, KeyCompareForNextKey{});
        }
    };

    struct tes_integer_map_ext : class_meta < tes_integer_map_ext > {
        REGISTER_TES_NAME("JIntMap");
        REGISTERF(tes_integer_map::nextKey, "nextKey", STR(* previousKey=0 endKey=0), tes_map_nextKey_comment);
        REGISTERF(tes_integer_map::getNthKey, "getNthKey", "* keyIndex", tes_map_ext::getNthKey_comment());
    };

    TES_META_INFO(tes_map_ext);
    TES_META_INFO(tes_form_map_ext);
    TES_META_INFO(tes_integer_map_ext);

    JC_TEST(tes_form_map, next_key_iteration)
    {
        using namespace collections;

        collections::form_map* fmap = tes_object::object<form_map>(context);
        fmap->u_container()[make_weak_form_id(util::to_enum<FormId>(0x14), context)] = item{ 10 };
        fmap->u_container()[make_weak_form_id(util::to_enum<FormId>(0x20), context)] = item{ 14 };

        auto countIterations = [&](collections::form_map* fmap) -> int {
            int cycle_counter = 0;
            const form_ref_lightweight endKey{};
            form_ref_lightweight key = tes_form_map_ext::nextKey(context, fmap, endKey, endKey);
            while (key) {
                ++cycle_counter;
                key = tes_form_map_ext::nextKey(context, fmap, key, endKey);
            }
            return cycle_counter;
        };

        EXPECT_EQ(fmap->s_count(), 2);
        EXPECT_EQ(countIterations(fmap), 2);

        fmap->u_container()[form_ref::make_expired(util::to_enum<FormId>(0x15))] = item{ "nill" };
        fmap->u_container()[form_ref{}] = item{ "nill" };

        EXPECT_EQ(fmap->s_count(), 4);
        EXPECT_EQ(countIterations(fmap), 2);
    }

}

#pragma once

namespace lua {
    using array = collections::array;
    using map = collections::map;
    using form_map = collections::form_map;

    using map_functions = collections::map_functions;
    using formmap_functions = collections::formmap_functions;
    using array_functions = collections::array_functions;
    using CollectionType = collections::CollectionType;

    using forms::form_ref;
    using forms::FormId;
    using forms::FormIdUnredlying;
    using collections::HACK_get_tcontext;
}

namespace lua { namespace api {

    //using namespace collections;

#   define cexport extern "C" __declspec(dllexport)

    namespace { // a trick to shut-up compiler
#   include "Data/SKSE/Plugins/JCData/InternalLuaScripts/api_for_lua.h"
    }

    cexport const char* JC_hello() {
        return "lua, are you there?";
    }

    cexport void* JC_get_c_function(const char *functionName, const char * className) {
        auto fi = reflection::find_function_of_class(functionName, className);
        return fi ? fi->c_func : nullptr;
    }

    JCToLuaValue JCToLuaValue_None() {
        return{ item_type::no_item, { 0 }, 0 };
    }

    cexport void JCToLuaValue_free(JCToLuaValue* v) {
        if (v && v->type == item_type::string) {
            free((void *)v->string);
        }
    }

    cexport void CString_free(CString *str) {
        if (str) {
            free((void *)str->str);
        }
    }

    CString CString_None() {
        return{ nullptr, 0 };
    }

    CString CString_copy(const char *origin, size_t length = -1) {
        if (!origin) {
            return CString_None();
        }

        length = length != -1 ? length : strlen(origin);
        char *string = (char *)calloc(length + 1, sizeof(char));
        strcpy(string, origin);
        return{ string, length };
    }

    CString CString_copy(const std::string& origin) {
        return CString_copy(origin.c_str(), origin.size());
    }

    JCToLuaValue JCToLuaValue_fromItem(const item& itm) {
        struct t : public boost::static_visitor < > {
            JCToLuaValue value;

            void operator ()(const std::string& str) {
                value.string = CString_copy(str.c_str(), str.size()).str;
                value.stringLength = str.size();
            }

            void operator ()(const item::Real& val) {
                value.real = val;
            }

            void operator ()(const SInt32& val) {
                value.integer = val;
            }

            void operator ()(const boost::blank&) {}

            void operator ()(const collections::internal_object_ref& val) {
                value.object = { val.get() };
            }

            void operator ()(const form_ref& val) {
                value.form = { (FormIdUnredlying)val.get() };
            }

        } converter;

        converter.value.type = itm.type();
        itm.var().apply_visitor(converter);
        return converter.value;
    }
    
    JCToLuaValue JCToLuaValue_fromItem(const item* itm) {
        return itm ? JCToLuaValue_fromItem(*itm) : JCToLuaValue_None();
    }

    void JCValue_fillItem(tes_context& context, const JCValue *v, item& itm) {
        switch (v ? v->type : item_type::no_item) {
        case item_type::form:
            itm = collections::make_weak_form_id((FormId)v->form.___id, context);
            break;
        // @see issue #40
        case item_type::integer:
            itm = v->integer;
            break;
        case item_type::real:
            itm = v->real;
            break;
        case item_type::object:
            itm = (object_base *)v->object;
            break;
        case item_type::string:
            itm = v->string;
            break;
        case item_type::no_item:
        case item_type::none:
            itm = boost::blank();
            break;
        default:
            assert(false);
            break;
        }
    }

    template<class JCV> std::string JCValue_toString(const JCV *v) {
        std::stringstream str;
        if (!v) {
            str << "nil";
        }
        else {
            switch (v->type)
            {
            case collections::no_item:
                str << "no_item";
                break;
            case collections::none:
                str << "none";
                break;
            case collections::integer:
                str << "integer " << v->integer;
                break;
            case collections::real:
                str << "real " << v->real;
                break;
            case collections::form:
                str << "form " << v->form;
                break;
            case collections::object:
                str << "object " << v->object;
                break;
            case collections::string:
                str << "string " << (const char*)v->string;
                break;
            default:
                assert(false);
                break;
            }
        }

        return str.str();
    }
    template<class JCV> std::string JCValue_toString(const JCV &v) { return JCValue_toString(&v); }

    cexport handle JValue_retain(object_base* obj) { return (obj ? obj->stack_retain(), obj : nullptr); }
    cexport handle JValue_release(object_base* obj) { return (obj ? obj->stack_release(), nullptr : nullptr); }
    cexport collections::CollectionType JValue_typeId(object_base* obj) { return (obj ? obj->_type : CollectionType::None); }

    cexport JCToLuaValue JValue_solvePath(tes_context *context, object_base *obj, cstring path) {
        namespace ca = collections::ca;
        assert(context && "context is null");
        auto value = JCToLuaValue_None();
        if (obj) {
            ca::visit_value(*obj, path, ca::constant, [&value](const item &itm) {
                value = JCToLuaValue_fromItem(&itm);
            });
        }
        return value;
    }

    cexport JCToLuaValue JArray_getValue(array* obj, index key) {
        JCToLuaValue v(JCToLuaValue_None());
        array_functions::doReadOp(obj, key, [=, &v](index idx) {
            v = JCToLuaValue_fromItem(obj->u_container()[idx]);
        });
        //std::cout << "value returned: " << JCValue_toString(v) << std::endl;
        return v;
    }

    cexport void JArray_setValue(array* obj, index key, const JCValue* val) {
        array_functions::doReadOp(obj, key, [=](index idx) {
            JCValue_fillItem(HACK_get_tcontext(*obj), val, obj->u_container()[idx]);
        });
        //std::cout << "value assigned: " << JCValue_toString(val) << std::endl;
    }

    cexport void JArray_insert(array* obj, const JCValue* val, index key) {
        array_functions::doWriteOp(obj, key, [=](index idx) {
            auto& cnt = obj->u_container();
            JCValue_fillItem(HACK_get_tcontext(*obj), val, *cnt.insert(cnt.begin() + idx, item()));
        });
        //std::cout << "value assigned: " << JCValue_toString(val) << std::endl;
    }

    //////////////////////////////////////////////////////////////////////////
    cexport CString JMap_nextKey(const map *obj, cstring lastKey) {
        CString next = CString_None();
        map_functions::nextKey(obj, lastKey, [&](const std::string& key) { next = CString_copy(key); });
        return next;
    }

    cexport void JMap_setValue(map *obj, cstring key, const JCValue* val) {
        map_functions::doWriteOp(obj, key, [obj, val](item& itm) { JCValue_fillItem(HACK_get_tcontext(*obj), val, itm); });
    }

    cexport JCToLuaValue JMap_getValue(map *obj, cstring key) {
        return map_functions::doReadOpR(obj, key, JCToLuaValue_None(), [](item& itm) { return JCToLuaValue_fromItem(itm); });
    }
    //////////////////////////////////////////////////////////////////////////

    static_assert(sizeof FormId == sizeof CForm, "");

    cexport FormId JFormMap_nextKey(const form_map *obj, FormId lastKey) {
        form_ref next;
        formmap_functions::nextKey(obj, make_weak_form_id(lastKey, HACK_get_tcontext(*obj)), [&](const form_ref& key) { next = key; });
        return next.get();
    }

    cexport void JFormMap_setValue(form_map *obj, FormId key, const JCValue* val) {
        formmap_functions::doWriteOp(obj, make_weak_form_id(key, HACK_get_tcontext(*obj)), [obj, val](item& itm) { JCValue_fillItem(HACK_get_tcontext(*obj), val, itm); });
    }

    cexport JCToLuaValue JFormMap_getValue(form_map *obj, FormId key) {
        return formmap_functions::doReadOpR(obj, make_weak_form_id(key, HACK_get_tcontext(*obj)), JCToLuaValue_None(), [](item& itm) { return JCToLuaValue_fromItem(itm); });
    }

    cexport void JFormMap_removeKey(form_map *obj, FormId key) {
        if (obj) {
            obj->erase(make_weak_form_id(key, HACK_get_tcontext(*obj)));
        }
    }

    ////////////////////////////

    cexport handle JDB_instance(tes_context *jc_context) {
        return &jc_context->root();
    }
}
}
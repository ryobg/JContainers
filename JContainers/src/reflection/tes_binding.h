#pragma once

#include "skse64/PapyrusNativeFunctions.h"
#include "skse/string.h"
#include "reflection/reflection.h"

class BGSListForm;

namespace reflection { namespace binding {

    // traits placeholders
    template<class JType>
    struct IdentityConverter {
        typedef JType tes_type;

        template<class State>
        static const tes_type& convert2J(const tes_type& val, const State&) {
            return val;
        }

        static const tes_type& convert2Tes(const tes_type& val) {
            return val;
        }
    };

    template<class JType, class TesType>
    struct StaticCastValueConverter {
        typedef TesType tes_type;

        template<class State>
        static JType convert2J(const TesType& val, const State&) {
            return static_cast<JType>(val);
        }

        static TesType convert2Tes(const JType& val) {
            return static_cast<TesType>(val);
        }
    };

    struct StringConverter {
        using tes_type = skse::string_ref;

        template<class State>
        static const char* convert2J(const skse::string_ref& str, const State&) {
            return str.c_str();
        }

        template<class AnyString>
        static skse::string_ref convert2Tes(const AnyString& str) {
            return skse::string_ref(str);
        }
    };

    template<class JType> struct GetConv : IdentityConverter < JType > {
        //typedef ValueConverter<TesType> Conv;
    };

    template<> struct GetConv<void> {
        using tes_type = void;
    };

    template<> struct GetConv<const char*> : StringConverter{};
    template<> struct GetConv<std::string> : StringConverter{};

    template<> struct GetConv<int32_t> : StaticCastValueConverter<int32_t, SInt32>{};
    template<> struct GetConv<uint32_t> : StaticCastValueConverter<uint32_t, UInt32>{};

    //////////////////////////////////////////////////////////////////////////

    template<class T>
    function_parameter type_info();

    template<class T> struct j2Str {
        static function_parameter typeInfo() {
            return reflection::binding::type_info<T>();
        }
    };

    template<class T> struct j2Str < VMArray<T> > {
        static function_parameter typeInfo() {
            std::string str(j2Str<T>::typeInfo().tes_type_name);
            str += "[]";
            function_parameter info = { str, "values" };
            return info;
        }
    };

    template<class T> struct j2Str < VMResultArray<T> > : j2Str < VMArray<T> > {};

    //////////////////////////////////////////////////////////////////////////


    template<size_t ParamCnt>
    struct native_function_selector;

#define MAKE_ME_HAPPY(N)\
    template<> struct native_function_selector<N> {\
        template<class... Params> using function = ::NativeFunction ## N <::StaticFunctionTag, Params...>;\
    };

    MAKE_ME_HAPPY(0);
    MAKE_ME_HAPPY(1);
    MAKE_ME_HAPPY(2);
    MAKE_ME_HAPPY(3);
    MAKE_ME_HAPPY(4);
    MAKE_ME_HAPPY(5);
    MAKE_ME_HAPPY(6);

    template<size_t ParamCnt>
    struct state_native_function_selector;

#undef MAKE_ME_HAPPY
#define MAKE_ME_HAPPY(N)\
    template<> struct state_native_function_selector<N> {\
        template<class State, class... Params> using function = ::NativeFunctionWithState ## N <State, Params...>; \
    };

    MAKE_ME_HAPPY(0);
    MAKE_ME_HAPPY(1);
    MAKE_ME_HAPPY(2);
    MAKE_ME_HAPPY(3);
    MAKE_ME_HAPPY(4);
    MAKE_ME_HAPPY(5);
    MAKE_ME_HAPPY(6);

#undef  MAKE_ME_HAPPY

    template<class T>
    using remove_cref = typename std::remove_const<typename std::remove_reference<T>::type>::type;

    template<class T>
    using get_converter = GetConv< remove_cref<T> >;

    template<class T>
    using convert_to_tes_type = typename get_converter<T>::tes_type;

    // Template monster, proxy class that:
    // - adapts my internal types to native Papyrus types and vica versa
    // - generates native Papyrus function
    // - holds function meta-info, like @parameter_info
    template <typename T> struct proxy;
    template <typename T> struct state_proxy;

    template <class R, class... Params>
    struct proxy<R(*)(Params ...)>
    {
        static std::vector<type_info_func> parameter_info() {
            return {
                &j2Str< convert_to_tes_type<R> >::typeInfo,
                &j2Str< convert_to_tes_type<Params> >::typeInfo ...
            };
        }

        using return_type = R;

        static const bool is_stateless = true;

        // subtype @magick to workaround some msvc2013 bug
        template< R(*func)(Params ...) >
        struct magick {

            using base = proxy;

            static auto func_ptr() -> decltype(func) {
                return func;
            }

            struct non_void_ret {
                static convert_to_tes_type<R> tes_func(
                    StaticFunctionTag* tag,
                    convert_to_tes_type<Params> ... params)
                {
                    return GetConv<R>::convert2Tes(
                        func(
                            get_converter<Params>::convert2J(params, tag) ...
                        )
                    );
                }
            };

            struct void_ret {
                static void tes_func(
                    StaticFunctionTag* tag,
                    convert_to_tes_type<Params> ... params)
                {
                    func(get_converter<Params>::convert2J(params, tag) ...);
                }
            };

            using tes_func_holder = typename std::conditional<
                std::is_void<R>::value,
                void_ret,
                non_void_ret>::type;

            static void bind(const bind_args& args) {
                args.registry.RegisterFunction
                (
                    new typename native_function_selector<sizeof...(Params)>::function<
                        convert_to_tes_type<R>, convert_to_tes_type<Params> ...>
                        (
                            args.functionName.c_str(),
                            args.className.c_str(),
                            &tes_func_holder::tes_func,
                            &args.registry
                        )
                );
            }
        };


    };

    template <class R, class State, class... Params>
    struct state_proxy<R(*)(State&, Params ...)>
    {
        static std::vector<type_info_func> parameter_info() {
            return {
                &j2Str< convert_to_tes_type<R> >::typeInfo,
                &j2Str< convert_to_tes_type<Params> >::typeInfo ...
            };
        }

        using return_type = R;

        static const bool is_stateless = false;

        // subtype @magick to workaround some msvc2013 bug
        template< R(*func)(State&, Params ...) >
        struct magick {

            using base = state_proxy;

            static auto func_ptr() -> decltype(func) {
                return func;
            }

            struct non_void_ret {
                static convert_to_tes_type<R> tes_func(
                    State& state,
                    convert_to_tes_type<Params> ... params)
                {
                    return GetConv<R>::convert2Tes(
                        func(
                            state,
                            get_converter<Params>::convert2J(params, state) ...
                        )
                    );
                }
            };

            struct void_ret {
                static void tes_func(
                    State& state,
                    convert_to_tes_type<Params> ... params)
                {
                    func(state, get_converter<Params>::convert2J(params, state) ...);
                }
            };

            using tes_func_holder = typename std::conditional<
                std::is_void<R>::value,
                void_ret,
                non_void_ret>::type;

            static void bind(const bind_args& args) {
                args.registry.RegisterFunction
                (
                    new typename state_native_function_selector<sizeof...(Params)>::function<
                        State, convert_to_tes_type<R>, convert_to_tes_type<Params> ...>
                        (
                            args.functionName.c_str(),
                            args.className.c_str(),
                            &tes_func_holder::tes_func,
                            &args.registry,
                            *reinterpret_cast<State*>(args.shared_state)
                        )
                );
            }
        };


    };


#define CONCAT(x, y) CONCAT1 (x, y)
#define CONCAT1(x, y) x##y

    // MSVC2012 bug workaround
    template <typename T> T msvc_identity(T);

    struct name_setter {
        explicit name_setter(class_info& info, const char* className) {
            info._className = className;
        }
    };

#define REGISTER_TES_NAME(ScriptTesName)  \
    ::reflection::binding::name_setter _name_setter{ metaInfo, ScriptTesName };

    struct function_registree {

        template<class Binder, class String2>
        inline function_registree(class_info& info,
            Binder,
            const char* funcname, const char* argument_names, const String2& comment)
        {
            using namespace ::reflection;

            static_assert( false == std::is_same<Binder::base::return_type, const char *>::value,
                "a trap for 'const char *' return types" );

            function_info metaF;
            metaF.registrator = &Binder::bind;
            metaF.param_list_func = &Binder::base::parameter_info;
             
            metaF.argument_names = (argument_names) ? (argument_names) : "";
            metaF.setComment(comment);
            metaF.name = funcname;
            metaF.tes_func = &Binder::tes_func_holder::tes_func;
            metaF.c_func = static_cast<c_function>(Binder::func_ptr());
            metaF._stateless = Binder::base::is_stateless;

            info.addFunction(metaF);
        }
    };

#define REGISTERF REGISTERF_STATE
#define REGISTERF_STATELESS(func, _funcname, _args, _comment)\
    ::reflection::binding::function_registree CONCAT(_func_registree_, __LINE__){ metaInfo,\
        ::reflection::binding::proxy<decltype(::reflection::binding::msvc_identity(&func))>::magick<&func>(), \
        _funcname, _args, _comment };

#define REGISTERF2(func, args, comment)     REGISTERF(func, #func, args, comment)
#define REGISTERF2_STATELESS(func, args, comment)     REGISTERF_STATELESS(func, #func, args, comment)

#define REGISTERF_STATE(func, _funcname, _args, _comment)\
    ::reflection::binding::function_registree CONCAT(_func_registree_, __LINE__){ \
        metaInfo, \
        ::reflection::binding::state_proxy<decltype(::reflection::binding::msvc_identity(&func))>::magick<&func>(), \
        _funcname, _args, _comment \
    };

    struct papyrus_textblock_setter {
        explicit papyrus_textblock_setter(class_info& info, const papyrus_text_block& text) {
            info.add_text_block(text);
        }
    };

#define REGISTER_TEXT(text) \
    ::reflection::binding::papyrus_textblock_setter CONCAT(_textblock_setter_, __LINE__){ metaInfo, ::reflection::papyrus_text_block(text) };


}
}

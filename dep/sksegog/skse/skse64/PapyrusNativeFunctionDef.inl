// Native

#define CLASS_NAME __MACRO_JOIN__(NativeFunction, NUM_PARAMS)
#define LATENT_SPEC 0
#define ACCEPTS_STATE 0

#define VOID_SPEC 0
#include "skse64/PapyrusNativeFunctionDef_Base.inl"

#define VOID_SPEC 1
#include "skse64/PapyrusNativeFunctionDef_Base.inl"

#undef LATENT_SPEC
#undef CLASS_NAME
#undef ACCEPTS_STATE

// Latent native

#define CLASS_NAME __MACRO_JOIN__(LatentNativeFunction, NUM_PARAMS)
#define LATENT_SPEC 1
#define ACCEPTS_STATE 0

#define VOID_SPEC 0
#include "skse64/PapyrusNativeFunctionDef_Base.inl"

#define VOID_SPEC 1
#include "skse64/PapyrusNativeFunctionDef_Base.inl"

#undef LATENT_SPEC
#undef CLASS_NAME
#undef ACCEPTS_STATE

// Extra functionality for JContainer

#define CLASS_NAME __MACRO_JOIN__(NativeFunctionWithState, NUM_PARAMS)
#define LATENT_SPEC 0
#define ACCEPTS_STATE 1

#define VOID_SPEC 0
#include "PapyrusNativeFunctionDef_Base.inl"
#define VOID_SPEC 1
#include "PapyrusNativeFunctionDef_Base.inl"

#undef LATENT_SPEC
#undef CLASS_NAME
#undef ACCEPTS_STATE
#undef NUM_PARAMS

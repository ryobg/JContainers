#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "skse/skse.h"

namespace forms 
{

inline bool is_form_handle(FormHandle handle) {
    return ((uint64_t)handle >> 32) == 0x0000FFFF;
}

inline FormId form_handle_to_id(FormHandle handle) {
    return static_cast<FormId>(handle);
}

inline FormHandle form_id_to_handle(FormId id) {
    return (FormHandle)(0x0000ffff00000000 | (uint64_t)id);
}

/// Whether absolute form id is considered static (and not global/dynamic - bound to a plugin)
inline bool is_static (FormId n) 
{
    auto u32 = static_cast<std::uint32_t> (n);
    return (u32 & 0xff00'0000u) != 0xff00'0000u;
}

/// See if that absolute form id looks like originating from a light weight mod (*.esl)
inline bool is_light (FormId n)
{
    auto u32 = static_cast<std::uint32_t> (n);
    return (u32 & 0xff00'0000u) == 0xfe00'0000u;
}

/// Gets the relative, to its mod, index of a form id
inline std::uint32_t local_id (FormId n) 
{
    auto u32 = static_cast<std::uint32_t> (n);
    return is_light (n) ? u32 & 0x0000'0fffu : u32 & 0x00ff'ffffu;
}

//--------------------------------------------------------------------------------------------------

/**
 * Convert the incoming absolute form id to canonical string representation
 *
 * No checks are made whether that form really exist (maybe see skse#resolve_handle())
 *
 * @param n is the form id
 * @return the string or std::nullopt if a mod for the incoming static form was not found
 */

inline std::optional<std::string> form_to_string (FormId n) 
{
    using namespace std;

    string s ("__formData|"); //likely on stack (aka SSO)
    auto u32 = static_cast<uint32_t> (n);

    if (is_static (n))
    {
        optional<string_view> mod;

        if (is_light (n))
        {
            mod = skse::loaded_light_mod_name (uint16_t ((u32 >> 12) & 0x0fffu));
            u32 &= 0x0000'0fffu;
        }
        else
        {
            mod = skse::loaded_mod_name (uint8_t (u32 >> 24));
            u32 &= 0x00ff'ffffu;
        }

        if (!mod)
            return nullopt;

        s += mod->data ();
    }

    //TODO: replace with std::to_chars when MSVC wake to implement it
    char form[12];
    snprintf (form, sizeof form, "|0x%x", u32);

    return s + form;
}

//--------------------------------------------------------------------------------------------------

inline bool is_form_string (const char* p) 
{
    constexpr char prefix[] = "__formData|";
    return p && std::strncmp (p, prefix, sizeof prefix - 1) == 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * Obtains form id from considering the file (if any) from which it originates.
 *
 * This method tries to implement the conventional Papyrus `GetFormFromFile` function. An 
 * enhancement is made to return dynamic/global form id, if the incoming file name is empty.
 *
 * @note SKSE code was reinterpreted to find the actual 12-bit light mod index.
 *
 * @param file name of the mod (e.g. "Skyrim.esm", "The war of green turtles.esl" and etc.)
 * @param form identifier (bits occupying mod ones will be ignored)
 * @return the absolute form id, if any
 */

inline std::optional<FormId> form_from_file (std::string_view const& file, std::uint32_t form)
{
    using namespace std;

    if (file.empty ())
        return FormId (0xff000000u | form);

    if (optional<uint32_t> ndx = skse::form_from_file (file, form))
    {
        return FormId (*ndx);
    }

    return nullopt;
}

//--------------------------------------------------------------------------------------------------

/**
 * Deduce a form identifier out of proper string.
 *
 * Several options are available:
 *
 * * `__formData|<mod name>|<relative id>`
 *   Search for mod name form prefix and append the given relative, 24-bit for esp/esm and
 *   12-bit for esl, identifier. Any incoming id bits in the place of the mod bits will be
 *   ignored (e.g. `__formData|test.esl|0x006780004` is about form #4).
 *
 * * `__formData||<dynamic form id>`
 *   Same as above, but as the mod name is missing, assume the form identifier is about dynamic
 *   forms. In practice the most significant bits will be set always.
 *
 * @param pstr - can be nullptr or empty too. Assumed it is alive during the duration of this call.
 * @return optional absolute form identifier, converted from the passed string.
 */

inline std::optional<FormId> string_to_form (const char* pstr)
{
    using namespace std;

    if (!pstr)
        return nullopt;

    string_view str (pstr);

    constexpr char prefix[] = "__formData|";
    if (str.find (prefix) != 0)
        return nullopt;
    str.remove_prefix (sizeof prefix - 1);

    auto mpos = str.find ('|');
    if (mpos == string_view::npos)
        return nullopt;

    string_view const mod = str.substr (0, mpos);
    string_view const fid = str.substr (mpos + 1);

    uint32_t form;
    try {
        form = stoul (string (fid), nullptr, 0); // likely std::string on stack (aka SSO)
    }
    catch (...) {
        return nullopt;
    }

    return form_from_file (mod, form);
}

//--------------------------------------------------------------------------------------------------

}

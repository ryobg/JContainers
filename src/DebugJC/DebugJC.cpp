#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/// Note that there will be "path" and "path3" empty folders as test leftovers
int main (int argc, const char* argv[])
{
    if (argc != 2)
        return 1;

    auto h = LoadLibraryA (argv[1]);
    if (!h)
        return 2;

    int rc;
    try 
    {
        typedef bool (__cdecl *entry_t) (int, const char**);
        if (auto entry = (entry_t) GetProcAddress (h, "JC_runTests"))
            rc = entry (argc, argv);
    }
    catch (...)
    {
        rc = 3;
    }

    FreeLibrary (h);
    return rc;
}


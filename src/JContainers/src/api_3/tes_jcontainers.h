namespace tes_api_3 {

/// Redefine in each logging module
#undef  JC_LOG_API_SOURCE
#define JC_LOG_API_SOURCE "JContainers"

    using namespace collections;

    class tes_jcontainers : public class_meta<tes_jcontainers> {
    public:

        REGISTER_TES_NAME("JContainers");

        void additionalSetup () override 
        {
            metaInfo.comment = "Utility and Versioning functionality\n"
                "\n"
                "JContainers version: " JC_VERSION_STR;
        }

        static UInt32 APIVersion () {
            JC_LOG_API ("");
            return static_cast<UInt32> (consts::api_version);
        }
        REGISTERF2_STATELESS (APIVersion, nullptr, "JContainers uses API.Feature.Minor.Patch versioning.");

        static UInt32 featureVersion () {
            JC_LOG_API ("");
            return static_cast<UInt32> (consts::feature_version);
        }
        REGISTERF2_STATELESS (featureVersion, nullptr, nullptr);

        static UInt32 minorVersion () {
            JC_LOG_API ("");
            return static_cast<UInt32> (consts::minor_version);
        }
        REGISTERF2_STATELESS (minorVersion, nullptr, nullptr);

        static UInt32 patchVersion () {
            JC_LOG_API ("");
            return static_cast<UInt32> (consts::patch_version);
        }
        REGISTERF2_STATELESS (patchVersion, nullptr, nullptr);

        static UInt32 versionInt () 
        {
            JC_LOG_API("");
            return 
                  static_cast<UInt32> (consts::api_version) * 1000000u
                + static_cast<UInt32> (consts::feature_version) * 10000u
                + static_cast<UInt32> (consts::minor_version) * 100u
                + static_cast<UInt32> (consts::patch_version);
        }
        REGISTERF2_STATELESS (versionInt, nullptr, []() {
            std::stringstream ss; ss
                << "Returns the full JContainers version as a sortable integer using AABBCCDD format.\n"
                << "\n"
                << "Formula:\n"
                << "    api * 1000000 + feature * 10000 + minor * 100 + patch\n"
                << "\n"
                << "Example:\n"
                << "    4.2.13.1 => 4021301\n"
                << "\n"
                << "Current version int is " << versionInt ();
            return ss.str ();
        });

        static std::string versionString () 
        {
            JC_LOG_API("");
            return JC_VERSION_STR;
        }
        REGISTERF2_STATELESS (versionString, nullptr, []() {
            std::stringstream ss; ss
                << "Returns the full JContainers version string in api.feature.minor.patch format.\n"
                << "\n"
                << "Current version string is " JC_VERSION_STR;
            return ss.str();
        });

        static bool versionAtLeast (UInt32 api, UInt32 feature, UInt32 minor = 0, UInt32 patch = 0) 
        {
            JC_LOG_API("");
            return versionInt () >= api*1000000u + feature*10000u + minor*100u + patch;
        }
        REGISTERF2_STATELESS(versionAtLeast, "api feature minor=0 patch=0", []() {
            std::stringstream ss; ss
                << "Returns true if the installed JContainers version is at least the requested version.\n"
                << "\n"
                << "Recommended compatibility check:\n"
                << "    bool valid = JContainers.versionAtLeast (4, 2, 13, 1)\n"
                << "\n"
                << "This should be preferred over APIVersion() and featureVersion().";
            return ss.str ();
        });

        static bool fileExistsAtPath(const char *filename)
        {
            JC_LOG_API ("%s", filename ? filename : "");

            if (!filename) {
                return false;
            }

            struct _stat buf;
            int result = _stat(filename, &buf);
            return result == 0;
        }
        REGISTERF2_STATELESS(fileExistsAtPath, "path", "Returns true if the file at a specified @path exists");

        template<class StringList>
        static StringList contentsOfDirectoryAtPath(
            const char *directoryPath
            ,const char *nameEndsWith = "")
        {
            JC_LOG_API ("%s, %s", directoryPath ? directoryPath : "", nameEndsWith ? nameEndsWith : "");

            if (!directoryPath) {
                return StringList{};
            }

            if (!nameEndsWith) {
                nameEndsWith = "";
            }

            StringList result{};
            namespace fs = boost::filesystem;

            try {
                fs::path root(directoryPath);
                for (fs::directory_iterator itr(root), end_itr; itr != end_itr; ++itr) {
                    const fs::path& path = itr->path();
                    if (!*nameEndsWith ||
                        path.extension().generic_string().compare(nameEndsWith) == 0)
                    {
                        result.emplace_back(itr->path().generic_string());
                    }
                }
            }
            catch (const boost::filesystem::filesystem_error& exc) {
                JC_LOG_TES_API_ERROR(JContainsers, contentsOfDirectoryAtPath, "throws '%s'", exc.what());
            }

            return result;
        }
        REGISTERF_STATELESS(
            contentsOfDirectoryAtPath<VMResultArray<skse::string_ref>>, "contentsOfDirectoryAtPath",
            "directoryPath extension=\"\"", nullptr);

        static void removeFileAtPath(const char *filename)
        {
            JC_LOG_API ("%s", filename ? filename : "");

            if (filename) {
                boost::filesystem::remove_all(filename);
            }
        }
        REGISTERF2_STATELESS(removeFileAtPath, "path", "Deletes the file or directory identified by the @path");

        static std::string userDirectory()
        {
            JC_LOG_API ("");

            char path[MAX_PATH];
            if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, path))) {
                return std::string();
            }

            strcat_s(path, sizeof(path), "/" JC_USER_FILES);

            // race condition possible. hope it's not critical
            if (!boost::filesystem::exists(path) && (boost::filesystem::create_directories(path), !boost::filesystem::exists(path))) {
                return std::string();
            }

            return path;
        }

        static skse::string_ref _userDirectory() {
            return userDirectory().c_str();
        }
        REGISTERF_STATELESS(_userDirectory, "userDirectory", "", "A path to user-specific directory - " JC_USER_FILES);

        static bool __isInstalled() {
            return true;
        }
        REGISTERF2_STATELESS(__isInstalled, nullptr, "For internal purposes, do not use it.");

        REGISTER_TEXT([]() {
            const char fmt[] = R"===(
; Returns true if JContainers plugin installed properly
bool function isInstalled() global
    return __isInstalled() && %u == APIVersion() && %u == featureVersion()
endfunction
)===";
            char buff[sizeof(fmt) * 3 / 2] = { '\0' };
            assert(-1 != sprintf_s(buff, fmt, consts::api_version, consts::feature_version));
            return std::string(buff);
        });
    };

    TES_META_INFO(tes_jcontainers);

    TEST(tes_jcontainers, userDirectory)
    {
        tes_context_standalone ctx;

        auto write_file = [&](const boost::filesystem::path& path) {
            boost::filesystem::remove_all(path);

            EXPECT_FALSE(boost::filesystem::is_regular(path));

            object_stack_ref obj = tes_object::object<map>(ctx);
            tes_object::writeToFile(ctx, obj.get(), path.string().c_str());

            EXPECT_TRUE(boost::filesystem::is_regular(path));

            boost::filesystem::remove_all(path);
        };

        auto path = tes_jcontainers::userDirectory();
        EXPECT_TRUE(!path.empty());
        EXPECT_TRUE(boost::filesystem::is_directory(path));

        write_file(tes_jcontainers::userDirectory() + "/MyMod/123/settings.json");
        write_file(tes_jcontainers::userDirectory() + "/settings.json");
        write_file(tes_jcontainers::userDirectory() + "settings2.json");
        write_file("obj3");
        write_file("path/obj3");
        write_file("/path2/obj3");
        write_file("path3\\obj3");
        write_file("\\path4\\obj3");
    }

    TEST(tes_jcontainers, contentsOfDirectoryAtPath)
    {
        std::vector<std::string> vec;
        EXPECT_NO_THROW(vec = tes_jcontainers::contentsOfDirectoryAtPath<decltype(vec)>(":invaliddir"));
        EXPECT_TRUE(vec.empty());
    }
}

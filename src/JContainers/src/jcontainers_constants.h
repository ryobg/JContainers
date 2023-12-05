#pragma once

#include <cstdint>

namespace collections {

#   define JC_API_VERSION           4
#   define JC_FEATURE_VERSION       2
#   define JC_PATCH_VERSION         7

#   define JC_FILE_VERSION          JC_API_VERSION, JC_FEATURE_VERSION, JC_PATCH_VERSION, 0

#   define JC_VERSION_STR           STR(JC_API_VERSION)           \
                                    "." STR(JC_FEATURE_VERSION)   \
                                    "." STR(JC_PATCH_VERSION)

#   define JC_DATA_FILES            "JCData/"

#ifdef JC_SKSE_VR

#   define JC_PLUGIN_NAME           "JContainersVR"
#   define JC_SKSE_LOGS             "\\My Games\\Skyrim VR\\SKSE\\"
#   define JC_USER_FILES            "My Games/Skyrim VR/JCUser/"

#elif JC_SKSE_GOG

#   define JC_PLUGIN_NAME           "JContainersGOG"
#   define JC_SKSE_LOGS             "\\My Games\\Skyrim Special Edition GOG\\SKSE\\"
#   define JC_USER_FILES            "My Games/Skyrim Special Edition GOG/JCUser/"

#else

#   define JC_PLUGIN_NAME           "JContainers64"
#   define JC_SKSE_LOGS             "\\My Games\\Skyrim Special Edition\\SKSE\\"
#   define JC_USER_FILES            "My Games/Skyrim Special Edition/JCUser/"

#endif

#   define JC_PLUGIN_FILENAME       JC_PLUGIN_NAME ".dll"

    enum class consts : std::uint32_t {
        storage_chunk = 'JSTR',

        api_version = JC_API_VERSION,
        feature_version = JC_FEATURE_VERSION,
        patch_version = JC_PATCH_VERSION,
    };

}

#pragma once

#include <stdint.h>
#include "typedefs.h"

namespace collections {

#   define JC_PLUGIN_NAME           "JContainers64"
#   define JC_API_VERSION           4
#   define JC_FEATURE_VERSION       0
#   define JC_PATCH_VERSION         0

#   define VER_FILE_VERSION         JC_API_VERSION, JC_FEATURE_VERSION, JC_PATCH_VERSION, 0

#   define JC_VERSION_STR           STR(JC_API_VERSION)           \
                                    "." STR(JC_FEATURE_VERSION)   \
                                    "." STR(JC_PATCH_VERSION)

#   define JC_USER_FILES            "My Games/Skyrim Special Edition/JCUser/"
#   define JC_DATA_FILES            "JCData/"

    enum class consts : uint32_t {
        storage_chunk = 'JSTR',

        api_version = JC_API_VERSION,
        feature_version = JC_FEATURE_VERSION,
        patch_version = JC_PATCH_VERSION,
    };

}

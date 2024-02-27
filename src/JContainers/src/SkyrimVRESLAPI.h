#pragma once
#include "skse64/GameReferences.h"
#include "skse64/PluginAPI.h"
#include <skse64/GameData.h>
// Interface code based on https://github.com/adamhynek/higgs

#ifdef JC_SKSE_VR
namespace SkyrimVRESLPluginAPI
{
	constexpr const auto SkyrimVRESLPluginName = "SkyrimVRESL";
	// A message used to fetch SkyrimVRESL's interface
	struct SkyrimVRESLMessage
	{
		enum : uint32_t
		{
			kMessage_GetInterface = 0xeacb2bef
		};  // Randomly generated
		void* (*GetApiFunction)(unsigned int revisionNumber) = nullptr;
	};

	struct TESFileCollection
	{
	public:
		// members
		tArray<ModInfo*> files;       // 00
		tArray<ModInfo*> smallFiles;  // 18
	};
	STATIC_ASSERT(sizeof(TESFileCollection) == 0x30);

	// Returns an ISkyrimVRESLInterface001 object compatible with the API shown below
	// This should only be called after SKSE sends kMessage_PostLoad to your plugin
	struct ISkyrimVRESLInterface001;
	ISkyrimVRESLInterface001* GetSkyrimVRESLInterface001(const PluginHandle& pluginHandle, SKSEMessagingInterface* messagingInterface);

	// This object provides access to SkyrimVRESL's mod support API
	struct ISkyrimVRESLInterface001
	{
		// Gets the SkyrimVRESL build number
		virtual unsigned int GetBuildNumber() = 0;

		/// @brief Get the SSE compatible TESFileCollection for SkyrimVR.
		/// This should be called after kDataLoaded to ensure it's been populated.
		/// @return Pointer to TESFileCollection CompiledFileCollection.
		const virtual TESFileCollection* GetCompiledFileCollection() = 0;
	};

// Converts the lower bits of a FormID to a full FormID depending on plugin type
static inline UInt32 GetFullFormID(const ModInfo* modInfo, UInt32 formLower)
{
	// Use modIndex of 0xFE as check for light plugin to determine proper form ID composition
	return (modInfo->modIndex != 0xFE) ? UInt32(modInfo->modIndex) << 24 | (formLower & 0xFFFFFF) : 0xFE000000 | (UInt32(modInfo->lightIndex) << 12) | (formLower & 0xFFF);
}

const ModInfo* LookupAllLoadedModByName(const char* modName);
const ModInfo* LookupLoadedLightModByName(const char* modName);
}  // namespace SkyrimVRESLPluginAPI
extern SkyrimVRESLPluginAPI::ISkyrimVRESLInterface001* g_SkyrimVRESLInterface;
#endif
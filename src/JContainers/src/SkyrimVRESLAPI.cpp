#include "SkyrimVRESLAPI.h"
// Interface code based on https://github.com/adamhynek/higgs

#ifdef JC_SKSE_VR
// Stores the API after it has already been fetched
SkyrimVRESLPluginAPI::ISkyrimVRESLInterface001* g_SkyrimVRESLInterface = nullptr;

// Fetches the interface to use from SkyrimVRESL
SkyrimVRESLPluginAPI::ISkyrimVRESLInterface001* SkyrimVRESLPluginAPI::GetSkyrimVRESLInterface001(const PluginHandle& pluginHandle, SKSEMessagingInterface* messagingInterface)
{
	// If the interface has already been fetched, rturn the same object
	if (g_SkyrimVRESLInterface) {
		return g_SkyrimVRESLInterface;
	}

	// Dispatch a message to get the plugin interface from SkyrimVRESL
	SkyrimVRESLMessage message;
	messagingInterface->Dispatch(pluginHandle, SkyrimVRESLMessage::kMessage_GetInterface, (void*)&message, sizeof(SkyrimVRESLMessage*), SkyrimVRESLPluginName);
	if (!message.GetApiFunction) {
		return nullptr;
	}

	// Fetch the API for this version of the SkyrimVRESL interface
	g_SkyrimVRESLInterface = static_cast<ISkyrimVRESLInterface001*>(message.GetApiFunction(1));
	return g_SkyrimVRESLInterface;
}

const ModInfo* SkyrimVRESLPluginAPI::LookupAllLoadedModByName(const char* modName)
{
	DataHandler* dataHandler = DataHandler::GetSingleton();
	if (dataHandler)
	{
		if (!g_SkyrimVRESLInterface)
		{
			return dataHandler->LookupLoadedModByName(modName);
		}
		else
		{
			const ModInfo* modInfo = dataHandler->LookupLoadedModByName(modName);
			if (modInfo == nullptr)
			{
				modInfo = SkyrimVRESLPluginAPI::LookupLoadedLightModByName(modName);
			}
			return modInfo;
		}
	}
	return nullptr;
}

const ModInfo* SkyrimVRESLPluginAPI::LookupLoadedLightModByName(const char* modName)
{
	if (!g_SkyrimVRESLInterface)
	{
		DataHandler* dataHandler = DataHandler::GetSingleton();
		if (dataHandler)
		{
			return dataHandler->LookupLoadedModByName(modName);
		}
		else
		{
			return nullptr;
		}
	}
	else
	{
		const SkyrimVRESLPluginAPI::TESFileCollection* fileCollection = g_SkyrimVRESLInterface->GetCompiledFileCollection();
		if (fileCollection != nullptr)
		{
			for (int i = 0; i < fileCollection->smallFiles.count; i++)
			{
				ModInfo* smallFile = nullptr;
				fileCollection->smallFiles.GetNthItem(i, smallFile);
				if (smallFile != nullptr)
				{
					int modNameLength = strlen(modName);
					if (modNameLength == strlen(smallFile->name) && _strnicmp(smallFile->name, modName, modNameLength) == 0)
					{
						return smallFile;
					}
				}
			}
		}
		return nullptr;
	}
}
#endif
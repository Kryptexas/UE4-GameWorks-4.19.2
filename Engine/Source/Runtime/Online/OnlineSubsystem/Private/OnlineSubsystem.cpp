// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPrivatePCH.h"
#include "OnlineSessionInterface.h"
#include "ModuleManager.h"
#include "OnlineKeyValuePair.h"
#include "NboSerializer.h"


IMPLEMENT_MODULE( FOnlineSubsystemModule, OnlineSubsystem );

DEFINE_LOG_CATEGORY(LogOnline);
DEFINE_LOG_CATEGORY(LogOnlineGame);

#if STATS
ONLINESUBSYSTEM_API DEFINE_STAT(STAT_Online_Async);
ONLINESUBSYSTEM_API DEFINE_STAT(STAT_Online_AsyncTasks);
ONLINESUBSYSTEM_API DEFINE_STAT(STAT_Session_Interface);
ONLINESUBSYSTEM_API DEFINE_STAT(STAT_Voice_Interface);
#endif

uint32 GetBuildUniqueId()
{
	static bool bStaticCheck = false;
	static bool bUseBuildIdOverride = false;
	static int32 BuildIdOverride = 0;
	if (!bStaticCheck)
	{
#if !UE_BUILD_SHIPPING
		if (FParse::Value(FCommandLine::Get(), TEXT("BuildIdOverride="), BuildIdOverride) && BuildIdOverride != 0)
		{
			bUseBuildIdOverride = true;
		}
		else
#endif // !UE_BUILD_SHIPPING
		{
			if (!GConfig->GetBool(TEXT("OnlineSubsystem"), TEXT("bUseBuildIdOverride"), bUseBuildIdOverride, GEngineIni))
			{
				UE_LOG_ONLINE(Warning, TEXT("Missing bUseBuildIdOverride= in [OnlineSubsystem] of DefaultEngine.ini"));
			}

			if (!GConfig->GetInt(TEXT("OnlineSubsystem"), TEXT("BuildIdOverride"), BuildIdOverride, GEngineIni))
			{
				UE_LOG_ONLINE(Warning, TEXT("Missing BuildIdOverride= in [OnlineSubsystem] of DefaultEngine.ini"));
			}
		}

		bStaticCheck = true;
	}

	uint32 Crc = 0;
	if (bUseBuildIdOverride == false)
	{
		/** Engine package CRC doesn't change, can't be used as the version - BZ */
		FNboSerializeToBuffer Buffer(64);
		// Serialize to a NBO buffer for consistent CRCs across platforms
		Buffer << GEngineNetVersion;
		// Now calculate the CRC
		Crc = FCrc::MemCrc_DEPRECATED((uint8*)Buffer, Buffer.GetByteCount());
	}
	else
	{
		Crc = (uint32)BuildIdOverride;
	}

	UE_LOG_ONLINE(Verbose, TEXT("GetBuildUniqueId: GEngineNetVersion %d bUseBuildIdOverride %d BuildIdOverride %d Crc %d"),
		GEngineNetVersion,
		bUseBuildIdOverride,
		BuildIdOverride,
		Crc);

	return Crc;
}

bool IsPlayerInSessionImpl(IOnlineSession* SessionInt, FName SessionName, const FUniqueNetId& UniqueId)
{
	bool bFound = false;
	FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName);
	if (Session != NULL)
	{
		const bool bIsSessionOwner = *Session->OwningUserId == UniqueId;

		FUniqueNetIdMatcher PlayerMatch(UniqueId);
		if (bIsSessionOwner || 
			Session->RegisteredPlayers.FindMatch(PlayerMatch) != INDEX_NONE)
		{
			bFound = true;
		}
	}
	return bFound;
}

/** Helper function to turn the friendly subsystem name into the module name */
static inline FName GetOnlineModuleName(const FString& SubsystemName)
{
	FString ModuleBase(TEXT("OnlineSubsystem"));

	FName ModuleName;
	if (!SubsystemName.StartsWith(ModuleBase, ESearchCase::CaseSensitive))
	{
		ModuleName = FName(*(ModuleBase + SubsystemName));
	}
	else
	{
		ModuleName = FName(*SubsystemName);
	}

	return ModuleName;
}

/**
 * Helper function that loads a given platform service module if it isn't already loaded
 *
 * @param SubsystemName Name of the requested platform service to load
 * @return The module interface of the requested platform service, NULL if the service doesn't exist
 */
static TSharedPtr<IModuleInterface> LoadSubsystemModule(const FString& SubsystemName)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_SHIPPING_WITH_EDITOR
	// Early out if we are overriding the module load
	bool bAttemptLoadModule = !FParse::Param(FCommandLine::Get(), *FString::Printf(TEXT("no%s"), *SubsystemName));
	if (bAttemptLoadModule)
#endif
	{
		FName ModuleName;
		FModuleManager& ModuleManager = FModuleManager::Get();

		ModuleName = GetOnlineModuleName(SubsystemName);
		if (!ModuleManager.IsModuleLoaded(ModuleName))
		{
			// Attempt to load the module
			ModuleManager.LoadModule(ModuleName);
		}

		if (ModuleManager.IsModuleLoaded(ModuleName))
		{
			// Now return the module itself
			return ModuleManager.GetModuleInterfaceRef(ModuleName);
		}
	}

	return NULL;
}

/**
 * Called right after the module DLL has been loaded and the module object has been created
 * Overloaded to allow the default subsystem a chance to load
 */
void FOnlineSubsystemModule::StartupModule()
{
	FString InterfaceString;

	// Load the platform defined "default" online services module
	if (GConfig->GetString(TEXT("OnlineSubsystem"), TEXT("DefaultPlatformService"), InterfaceString, GEngineIni) &&
		InterfaceString.Len() > 0)
	{
		FName InterfaceName = FName(*InterfaceString);
		if (LoadSubsystemModule(InterfaceString).IsValid() && PlatformServices.Contains(InterfaceName))
		{
			DefaultPlatformService = InterfaceName;
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("Unable to load default OnlineSubsystem module %s, using NULL interface"), *InterfaceString);
			InterfaceString = TEXT("Null");
			InterfaceName = FName(*InterfaceString);
			if (LoadSubsystemModule(InterfaceString).IsValid() && PlatformServices.Contains(InterfaceName))
			{
				DefaultPlatformService = InterfaceName;
			}
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No default platform service specified for OnlineSubsystem"));
	}
}

/**
 * Called before the module is unloaded, right before the module object is destroyed.
 * Overloaded to shut down all loaded online subsystems
 */
void FOnlineSubsystemModule::ShutdownModule()
{
	ShutdownOnlineSubsystem();
}

/**
 * Unload and cleanup all registered online subsystems
 */
void FOnlineSubsystemModule::ShutdownOnlineSubsystem()
{
	FModuleManager& ModuleManager = FModuleManager::Get();
	// Unload all the supporting factories
	for (TMap<FName, IOnlineSubsystem*>::TIterator It(PlatformServices); It; ++It)
	{
		UE_LOG(LogOnline, Display, TEXT("Unloading online subsystem: %s"), *It.Key().ToString());

		// Unloading the module will do proper cleanup
		FName ModuleName = GetOnlineModuleName(It.Key().ToString());

		const bool bIsShutdown = true;
		ModuleManager.UnloadModule(ModuleName, bIsShutdown);
	} 
	//ensure(PlatformServices.Num() == 0);
}

/** 
 * Register a new online subsystem interface with the base level factory provider
 * @param FactoryName - name of subsystem as referenced by consumers
 * @param Factory - instantiation of the online subsystem interface, this will take ownership
 */
void FOnlineSubsystemModule::RegisterPlatformService(const FName FactoryName, IOnlineSubsystem* Factory)
{
	if (!PlatformServices.Contains(FactoryName))
	{
		PlatformServices.Add(FactoryName, Factory);
	}
}

/** 
 * Unregister an existing online subsystem interface from the base level factory provider
 * @param FactoryName - name of subsystem as referenced by consumers
 */
void FOnlineSubsystemModule::UnregisterPlatformService(const FName FactoryName)
{
	if (PlatformServices.Contains(FactoryName))
	{
		PlatformServices.Remove(FactoryName);
	}
}

/** 
 * Main entry point for accessing an online subsystem by name
 * Will load the appropriate module if the subsystem isn't currently loaded
 * It's possible that the subsystem doesn't exist and therefore can return NULL
 *
 * @param SubsystemName - name of subsystem as referenced by consumers
 * @return Requested online subsystem, or NULL if that subsystem was unable to load or doesn't exist
 */
IOnlineSubsystem* FOnlineSubsystemModule::GetOnlineSubsystem(const FName InSubsystemName)
{
	FName SubsystemName = InSubsystemName;
	if (SubsystemName == NAME_None)
	{
		SubsystemName = DefaultPlatformService;
	}

	IOnlineSubsystem** OSSFactory = NULL;
	if (SubsystemName != NAME_None)
	{
		OSSFactory = PlatformServices.Find(SubsystemName);
		if (OSSFactory == NULL)
		{
			// Attempt to load the requested factory
			TSharedPtr<IModuleInterface> NewModule = LoadSubsystemModule(SubsystemName.ToString());
			if( NewModule.IsValid() )
			{
				// If the module loaded successfully this should be non-NULL;
				OSSFactory = PlatformServices.Find(SubsystemName);
			}
			if (OSSFactory == NULL)
			{
				UE_LOG(LogOnline, Warning, TEXT("Unable to load OnlineSubsystem module %s"), *InSubsystemName.ToString());
			}
		}
	}

	return (OSSFactory == NULL) ? NULL : *OSSFactory;
}

/** 
 * Determine if a subsystem is loaded by the OSS module
 *
 * @param SubsystemName - name of subsystem as referenced by consumers
 * @return true if module for the subsystem is loaded
 */
bool FOnlineSubsystemModule::IsOnlineSubsystemLoaded(const FName InSubsystemName) const
{
	bool bIsLoaded = false;

	FName SubsystemName = InSubsystemName;
	if (SubsystemName == NAME_None)
	{
		SubsystemName = DefaultPlatformService;
	}
	if (SubsystemName != NAME_None)
	{
		if (FModuleManager::Get().IsModuleLoaded(GetOnlineModuleName(SubsystemName.ToString())))
		{
			bIsLoaded = true;
		}
	}
	return bIsLoaded;
}


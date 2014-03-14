// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TargetPlatformManagerModule.cpp: Implements the FTargetPlatformManagerModule class.
=============================================================================*/

#include "TargetPlatformPrivatePCH.h"


DEFINE_LOG_CATEGORY_STATIC(LogTargetPlatformManager, Log, All);


/**
 * Module for the target platform manager
 */
class FTargetPlatformManagerModule
	: public ITargetPlatformManagerModule
{
public:

	/**
	 * Default constructor.
	 */
	FTargetPlatformManagerModule()
		: bRestrictFormatsToRuntimeOnly(false), bForceCacheUpdate(true)
	{
		GetTargetPlatforms();
		GetActiveTargetPlatforms();
		GetAudioFormats();
		GetTextureFormats();
		GetShaderFormats();

		bForceCacheUpdate = false;

		FModuleManager::Get().OnModulesChanged().AddRaw(this, &FTargetPlatformManagerModule::ModulesChangesCallback);
	}

	virtual ~FTargetPlatformManagerModule()
	{
		FModuleManager::Get().OnModulesChanged().RemoveAll(this);
	}


public:

	virtual void Invalidate()
	{
		bForceCacheUpdate = true;

		GetTargetPlatforms();
		GetActiveTargetPlatforms();
		GetAudioFormats();
		GetTextureFormats();
		GetShaderFormats();

		bForceCacheUpdate = false;
	}

	virtual const TArray<ITargetPlatform*>& GetTargetPlatforms() OVERRIDE
	{
		if (Platforms.Num() == 0 || bForceCacheUpdate)
		{
			DiscoverAvailablePlatforms();
		}

		return Platforms;
	}

	virtual ITargetDevicePtr FindTargetDevice(const FTargetDeviceId& DeviceId) OVERRIDE
	{
		ITargetPlatform* Platform = FindTargetPlatform(DeviceId.GetPlatformName());

		if (Platform != NULL)
		{
			return Platform->GetDevice(DeviceId);
		}

		return NULL;
	}

	virtual ITargetPlatform* FindTargetPlatform(FString Name) OVERRIDE
	{
		const TArray<ITargetPlatform*>& TargetPlatforms = GetTargetPlatforms();	
		
		for (int32 Index = 0; Index < TargetPlatforms.Num(); Index++)
		{
			if (TargetPlatforms[Index]->PlatformName() == Name)
			{
				return TargetPlatforms[Index];
			}
		}

		return NULL;
	}

	virtual const TArray<ITargetPlatform*>& GetActiveTargetPlatforms() OVERRIDE
	{
		static bool bInitialized = false;
		static TArray<ITargetPlatform*> Results;

		if (!bInitialized || bForceCacheUpdate)
		{
			bInitialized = true;

			Results.Empty(Results.Num());

			const TArray<ITargetPlatform*>& TargetPlatforms = GetTargetPlatforms();	

			FString PlatformStr;

			if (FParse::Value(FCommandLine::Get(), TEXT("TARGETPLATFORM="), PlatformStr))
			{
				if (PlatformStr == TEXT("None"))
				{
				}
				else if (PlatformStr == TEXT("All"))
				{
					Results = TargetPlatforms;
				}
				else
				{
					TArray<FString> PlatformNames;

					PlatformStr.ParseIntoArray(&PlatformNames, TEXT("+"), true);

					for (int32 Index = 0; Index < TargetPlatforms.Num(); Index++)
					{
						if (PlatformNames.Contains(TargetPlatforms[Index]->PlatformName()))
						{
							Results.Add(TargetPlatforms[Index]);
						}
					}

					if (Results.Num() == 0)
					{
						// An invalid platform was specified...
						// Inform the user and exit.
						UE_LOG(LogTargetPlatformManager, Fatal, TEXT("Invalid target platform specified (%s)."), *PlatformStr);
					}
				}
			}
			else
			{
				// if there is no argument, use the current platform and only build formats that are actually needed to run.
				bRestrictFormatsToRuntimeOnly = true;

				for (int32 Index = 0; Index < TargetPlatforms.Num(); Index++)
				{
					if (TargetPlatforms[Index]->IsRunningPlatform())
					{
						Results.Add(TargetPlatforms[Index]);
					}
				}
			}

			if (!Results.Num())
			{
				UE_LOG(LogTargetPlatformManager, Display, TEXT("Not building assets for any platform."));
			}
			else
			{
				for (int32 Index = 0; Index < Results.Num(); Index++)
				{
					UE_LOG(LogTargetPlatformManager, Display, TEXT("Building Assets For %s"), *Results[Index]->PlatformName());
				}
			}
		}

		return Results;
	}

	virtual bool RestrictFormatsToRuntimeOnly() OVERRIDE
	{
		GetActiveTargetPlatforms(); // make sure this is initialized

		return bRestrictFormatsToRuntimeOnly;
	}

	virtual ITargetPlatform* GetRunningTargetPlatform() OVERRIDE
	{
		static bool bInitialized = false;
		static ITargetPlatform* Result = NULL;

		if (!bInitialized || bForceCacheUpdate)
		{
			bInitialized = true;
			Result = NULL;

			const TArray<ITargetPlatform*>& TargetPlatforms = GetTargetPlatforms();	

			for (int32 Index = 0; Index < TargetPlatforms.Num(); Index++)
			{
				if (TargetPlatforms[Index]->IsRunningPlatform())
				{
					 // we should not have two running platforms
					checkf((Result == NULL),
						TEXT("Found multiple running platforms.\n\t%s\nand\n\t%s"),
						*Result->PlatformName(),
						*TargetPlatforms[Index]->PlatformName()
						);
					Result = TargetPlatforms[Index];
				}
			}
		}

		return Result;
	}

	virtual const TArray<const IAudioFormat*>& GetAudioFormats() OVERRIDE
	{
		static bool bInitialized = false;
		static TArray<const IAudioFormat*> Results;

		if (!bInitialized || bForceCacheUpdate)
		{
			bInitialized = true;
			Results.Empty(Results.Num());

			TArray<FName> Modules;

			FModuleManager::Get().FindModules(TEXT("*AudioFormat*"), Modules);

			if (!Modules.Num())
			{
				UE_LOG(LogTargetPlatformManager, Error, TEXT("No target audio formats found!"));
			}

			for (int32 Index = 0; Index < Modules.Num(); Index++)
			{
				IAudioFormatModule* Module = FModuleManager::LoadModulePtr<IAudioFormatModule>(Modules[Index]);
				if (Module)
				{
					IAudioFormat* Format = Module->GetAudioFormat();
					if (Format != NULL)
					{
						Results.Add(Format);
					}
				}
			}
		}

		return Results;
	}

	virtual const IAudioFormat* FindAudioFormat(FName Name) OVERRIDE
	{
		const TArray<const IAudioFormat*>& AudioFormats = GetAudioFormats();

		for (int32 Index = 0; Index < AudioFormats.Num(); Index++)
		{
			TArray<FName> Formats;

			AudioFormats[Index]->GetSupportedFormats(Formats);

			for (int32 FormatIndex = 0; FormatIndex < Formats.Num(); FormatIndex++)
			{
				if (Formats[FormatIndex] == Name)
				{
					return AudioFormats[Index];
				}
			}
		}

		return NULL;
	}

	virtual const TArray<const ITextureFormat*>& GetTextureFormats() OVERRIDE
	{
		static bool bInitialized = false;
		static TArray<const ITextureFormat*> Results;

		if (!bInitialized || bForceCacheUpdate)
		{
			bInitialized = true;
			Results.Empty(Results.Num());

			TArray<FName> Modules;

			FModuleManager::Get().FindModules(TEXT("*TextureFormat*"), Modules);

			if (!Modules.Num())
			{
				UE_LOG(LogTargetPlatformManager, Error, TEXT("No target texture formats found!"));
			}

			for (int32 Index = 0; Index < Modules.Num(); Index++)
			{
				ITextureFormatModule* Module = FModuleManager::LoadModulePtr<ITextureFormatModule>(Modules[Index]);
				if (Module)
				{
					ITextureFormat* Format = Module->GetTextureFormat();
					if (Format != NULL)
					{
						Results.Add(Format);
					}
				}
			}
		}

		return Results;
	}

	virtual const ITextureFormat* FindTextureFormat(FName Name) OVERRIDE
	{
		const TArray<const ITextureFormat*>& TextureFormats = GetTextureFormats();

		for (int32 Index = 0; Index < TextureFormats.Num(); Index++)
		{
			TArray<FName> Formats;

			TextureFormats[Index]->GetSupportedFormats(Formats);

			for (int32 FormatIndex = 0; FormatIndex < Formats.Num(); FormatIndex++)
			{
				if (Formats[FormatIndex] == Name)
				{
					return TextureFormats[Index];
				}
			}
		}

		return NULL;
	}

	virtual const TArray<const IShaderFormat*>& GetShaderFormats() OVERRIDE
	{
		static bool bInitialized = false;
		static TArray<const IShaderFormat*> Results;

		if (!bInitialized || bForceCacheUpdate)
		{
			bInitialized = true;
			Results.Empty(Results.Num());

			TArray<FName> Modules;

			FModuleManager::Get().FindModules(SHADERFORMAT_MODULE_WILDCARD, Modules);

			if (!Modules.Num())
			{
				UE_LOG(LogTargetPlatformManager, Error, TEXT("No target shader formats found!"));
			}

			for (int32 Index = 0; Index < Modules.Num(); Index++)
			{
				IShaderFormatModule* Module = FModuleManager::LoadModulePtr<IShaderFormatModule>(Modules[Index]);
				if (Module)
				{
					IShaderFormat* Format = Module->GetShaderFormat();
					if (Format != NULL)
					{
						Results.Add(Format);
					}
				}
			}
		}
		return Results;
	}

	virtual const IShaderFormat* FindShaderFormat(FName Name) OVERRIDE
	{
		const TArray<const IShaderFormat*>& ShaderFormats = GetShaderFormats();	

		for (int32 Index = 0; Index < ShaderFormats.Num(); Index++)
		{
			TArray<FName> Formats;
			
			ShaderFormats[Index]->GetSupportedFormats(Formats);
		
			for (int32 FormatIndex = 0; FormatIndex < Formats.Num(); FormatIndex++)
			{
				if (Formats[FormatIndex] == Name)
				{
					return ShaderFormats[Index];
				}
			}
		}

		return NULL;
	}

	virtual uint16 ShaderFormatVersion(FName Name) OVERRIDE
	{
		static TMap<FName, uint16> AlreadyFound;
		uint16* Result = AlreadyFound.Find(Name);

		if (!Result)
		{
			const IShaderFormat* SF = FindShaderFormat(Name);

			if (SF)
			{
				Result = &AlreadyFound.Add(Name, SF->GetVersion(Name));
			}
		}

		check(Result);

		return *Result;
	}

	virtual const TArray<const IPhysXFormat*>& GetPhysXFormats() OVERRIDE
	{
		static bool bInitialized = false;
		static TArray<const IPhysXFormat*> Results;

		if (!bInitialized || bForceCacheUpdate)
		{
			bInitialized = true;
			Results.Empty(Results.Num());
			
			TArray<FName> Modules;
			FModuleManager::Get().FindModules(TEXT("PhysXFormat*"), Modules);
			
			if (!Modules.Num())
			{
				UE_LOG(LogTargetPlatformManager, Error, TEXT("No target PhysX formats found!"));
			}

			for (int32 Index = 0; Index < Modules.Num(); Index++)
			{
				IPhysXFormatModule* Module = FModuleManager::LoadModulePtr<IPhysXFormatModule>(Modules[Index]);
				if (Module)
				{
					IPhysXFormat* Format = Module->GetPhysXFormat();
					if (Format != NULL)
					{
						Results.Add(Format);
					}
				}
			}
		}

		return Results;
	}

	virtual const IPhysXFormat* FindPhysXFormat(FName Name) OVERRIDE
	{
		const TArray<const IPhysXFormat*>& PhysXFormats = GetPhysXFormats();

		for (int32 Index = 0; Index < PhysXFormats.Num(); Index++)
		{
			TArray<FName> Formats;

			PhysXFormats[Index]->GetSupportedFormats(Formats);
		
			for (int32 FormatIndex = 0; FormatIndex < Formats.Num(); FormatIndex++)
			{
				if (Formats[FormatIndex] == Name)
				{
					return PhysXFormats[Index];
				}
			}
		}

		return NULL;
	}


protected:

	/**
	 * Discovers the available target platforms.
	 */
	void DiscoverAvailablePlatforms( )
	{
		Platforms.Empty(Platforms.Num());

		TArray<FName> Modules;

		FModuleManager::Get().FindModules(TEXT("*TargetPlatform"), Modules);

		// remove this module from the list
		Modules.Remove(FName(TEXT("TargetPlatform")));

		if (!Modules.Num())
		{
			UE_LOG(LogTargetPlatformManager, Error, TEXT("No target platforms found!"));
		}

		for (int32 Index = 0; Index < Modules.Num(); Index++)
		{
			ITargetPlatformModule* Module = FModuleManager::LoadModulePtr<ITargetPlatformModule>(Modules[Index]);
			if (Module)
			{
				ITargetPlatform* Platform = Module->GetTargetPlatform();
				if (Platform != NULL)
				{
					Platforms.Add(Platform);
				}
			}
		}
	}


private:

	void ModulesChangesCallback(FName ModuleName, EModuleChangeReason::Type ReasonForChange)
	{
		if (ModuleName.ToString().Contains(TEXT("TargetPlatform")) )
		{
			Invalidate();
		}
	}
	
	// If true we should build formats that are actually required for use by the runtime. 
	// This happens for an ordinary editor run and more specifically whenever there is no
	// TargetPlatform= on the command line.
	bool bRestrictFormatsToRuntimeOnly;

	// Flag to force reinitialization of all cached data. This is needed to have up-to-date caches
	// in case of a module reload of a TargetPlatform-Module.
	bool bForceCacheUpdate;

	// Holds the list of discovered platforms.
	TArray<ITargetPlatform*> Platforms;
};


IMPLEMENT_MODULE(FTargetPlatformManagerModule, TargetPlatform);

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetsPrivatePCH.h"
#include "ModuleManager.h"


/**
 * Implements the MediaAssets module.
 */
class FMediaAssetsModule
	: public FSelfRegisteringExec
	, public IModuleInterface
{
public:

	// FSelfRegisteringExec interface

	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) override
	{
		if (FParse::Command(&Cmd, TEXT("MOVIE")))
		{
			FString MovieCmd = FParse::Token(Cmd, 0);

			if (MovieCmd.Contains(TEXT("PLAY")))
			{
				for (TObjectIterator<UMediaAsset> It; It; ++It)
				{
					UMediaAsset* MediaAsset = *It;
					MediaAsset->Play();
				}
			}
			else if (MovieCmd.Contains(TEXT("PAUSE")))
			{
				for (TObjectIterator<UMediaAsset> It; It; ++It)
				{
					UMediaAsset* MediaAsset = *It;
					MediaAsset->Pause();
				}
			}
			else if (MovieCmd.Contains(TEXT("STOP")))
			{
				for (TObjectIterator<UMediaAsset> It; It; ++It)
				{
					UMediaAsset* MediaAsset = *It;
					MediaAsset->Stop();
				}
			}

			return true;
		}

		return false;
	}

public:

	// IModuleInterface interface

	virtual void StartupModule( ) override { }

	virtual void ShutdownModule( ) override { }

	virtual bool SupportsDynamicReloading( ) override
	{
		return false;
	}
};


IMPLEMENT_MODULE(FMediaAssetsModule, MediaAssets);

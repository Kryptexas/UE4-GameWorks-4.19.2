// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MfMediaPrivate.h"
#include "IMfMediaModule.h"

#if MFMEDIA_SUPPORTED_PLATFORM
	#include "MfMediaPlayer.h"
#endif


DEFINE_LOG_CATEGORY(LogMfMedia);

#define LOCTEXT_NAMESPACE "FMfMediaModule"


/**
 * Implements the MfMedia module.
 */
class FMfMediaModule
	: public IMfMediaModule
{
public:

	/** Default constructor. */
	FMfMediaModule()
		: Initialized(false)
	{ }

public:

	//~ IMfMediaModule interface

	virtual TSharedPtr<IMediaPlayer> CreatePlayer() override
	{
#if MFMEDIA_SUPPORTED_PLATFORM
		if (Initialized)
		{
			return MakeShareable(new FMfMediaPlayer());
		}
#endif
		return nullptr;
	}

public:

	//~ IModuleInterface interface

	virtual void StartupModule() override
	{
#if MFMEDIA_SUPPORTED_PLATFORM
		// initialize Windows Media Foundation
		HRESULT Result = MFStartup(MF_VERSION);

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Log, TEXT("Failed to initialize Xbox One Media Foundation, Error %i"), Result);

			return;
		}

		Initialized = true;
#endif
	}

	virtual void ShutdownModule() override
	{
#if MFMEDIA_SUPPORTED_PLATFORM
		Initialized = false;

		if (Initialized)
		{
			// shutdown Windows Media Foundation
			MFShutdown();
		}
#endif
	}

private:

	/** Whether the module has been initialized. */
	bool Initialized;
};


IMPLEMENT_MODULE(FMfMediaModule, MfMedia);


#undef LOCTEXT_NAMESPACE

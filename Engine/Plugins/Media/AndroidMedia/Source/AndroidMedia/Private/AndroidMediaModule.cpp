// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AndroidMediaPCH.h"
#include "AndroidMediaPlayer.h"
#include "IMediaModule.h"
#include "IMediaPlayerFactory.h"


#define LOCTEXT_NAMESPACE "FAndroidMediaModule"


class FAndroidMediaModule
	: public IModuleInterface
	, public IMediaPlayerFactory
{
public:

	// IModuleInterface interface

	virtual void StartupModule() override
	{
		if (IsSupported())
		{
			IMediaModule* MediaModule = FModuleManager::LoadModulePtr<IMediaModule>("Media");
			if (nullptr != MediaModule)
			{
				SupportedFileTypes.Add(TEXT("3gpp"), LOCTEXT("Format3gpp", "3GPP Multimedia File"));
				SupportedFileTypes.Add(TEXT("aac"), LOCTEXT("FormatAac", "MPEG-2 Advanced Audio Coding File"));
				SupportedFileTypes.Add(TEXT("mp4"), LOCTEXT("FormatMp4", "MPEG-4 Movie"));

				// initialize supported URI schemes
				SupportedUriSchemes.Add(TEXT("http://"));
				SupportedUriSchemes.Add(TEXT("httpd://"));
				SupportedUriSchemes.Add(TEXT("https://"));
				SupportedUriSchemes.Add(TEXT("mms://"));
				SupportedUriSchemes.Add(TEXT("rtsp://"));
				SupportedUriSchemes.Add(TEXT("rtspt://"));
				SupportedUriSchemes.Add(TEXT("rtspu://"));

				MediaModule->RegisterPlayerFactory(*this);
			}
		}
	}

	virtual void ShutdownModule() override
	{
		if (IsSupported())
		{
			IMediaModule* MediaModule = FModuleManager::GetModulePtr<IMediaModule>("Media");
			if (nullptr != MediaModule)
			{
				MediaModule->UnregisterPlayerFactory(*this);
			}
		}
	}

	// IMediaPlayerFactory interface

	virtual TSharedPtr<IMediaPlayer> CreatePlayer() override
	{
		if (IsSupported())
		{
			return MakeShareable(new FAndroidMediaPlayer());
		}
		else
		{
			return nullptr;
		}
	}

	virtual const FMediaFileTypes& GetSupportedFileTypes() const override
	{
		return SupportedFileTypes;
	}

	virtual bool SupportsUrl(const FString& Url) const override
	{
		const FString Extension = FPaths::GetExtension(Url);

		if (!Extension.IsEmpty() && SupportedFileTypes.Contains(Extension))
		{
			return true;
		}

		for (const FString& Scheme : SupportedUriSchemes)
		{
			if (Url.StartsWith(Scheme))
			{
				return true;
			}
		}

		return false;
	}

private:

	// The collection of supported media file types.
	FMediaFileTypes SupportedFileTypes;

	/** The collection of supported URI schemes. */
	TArray<FString> SupportedUriSchemes;

	bool IsSupported()
	{
		return FAndroidMisc::GetAndroidBuildVersion() >= 14;
	}
};

IMPLEMENT_MODULE(FAndroidMediaModule, AndroidMedia)

#undef LOCTEXT_NAMESPACE

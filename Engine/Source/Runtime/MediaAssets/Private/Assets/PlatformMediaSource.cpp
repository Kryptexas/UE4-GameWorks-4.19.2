// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "PlatformMediaSource.h"
#include "UObject/SequencerObjectVersion.h"
#include "MediaAssetsPrivate.h"

#if WITH_EDITOR
	#include "Interfaces/ITargetPlatform.h"
#endif


/* UMediaSource interface
 *****************************************************************************/

FString UPlatformMediaSource::GetUrl() const
{
	// Guard against reentrant calls.
	static bool bIsGettingUrl = false;
	if (bIsGettingUrl)
	{
		UE_LOG(LogMediaAssets, Warning, TEXT("UPlatformMediaSource::GetUrl - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return FString();
	}
	TGuardValue<bool> GettingUrlGuard(bIsGettingUrl, true);

	UMediaSource* PlatformMediaSource = GetMediaSource();
	return (PlatformMediaSource != nullptr) ? PlatformMediaSource->GetUrl() : FString();
}


void UPlatformMediaSource::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FSequencerObjectVersion::GUID);

#if WITH_EDITORONLY_DATA
	if (Ar.IsCooking())
	{
		UMediaSource** PlatformMediaSource = PlatformMediaSources.Find(Ar.CookingTarget()->PlatformName());
		MediaSource = (PlatformMediaSource != nullptr) ? *PlatformMediaSource : nullptr;
	}
#endif
	
	if (Ar.IsLoading() && (Ar.CustomVer(FSequencerObjectVersion::GUID) < FSequencerObjectVersion::RenameMediaSourcePlatformPlayers))
	{
		FString DummyDefaultSource;
		Ar << DummyDefaultSource;

#if WITH_EDITORONLY_DATA
		Ar << PlatformMediaSources;
#endif
	}
	else
	{
#if WITH_EDITORONLY_DATA
		Ar << PlatformMediaSources;
#else
		Ar << MediaSource;
#endif
	}
}


bool UPlatformMediaSource::Validate() const
{
	// Guard against reentrant calls.
	static bool bIsValidating = false;
	if (bIsValidating)
	{
		UE_LOG(LogMediaAssets, Warning, TEXT("UPlatformMediaSource::Validate - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return false;
	}
	TGuardValue<bool> ValidatingGuard(bIsValidating, true);

#if WITH_EDITORONLY_DATA
	for (auto PlatformNameMediaSourcePair : PlatformMediaSources)
	{
		UMediaSource* PlatformMediaSource = PlatformNameMediaSourcePair.Value;
		if (PlatformMediaSource != nullptr && PlatformMediaSource->Validate() == false)
		{
			// If any platform is specified but doesn't validate, the entire platform media source is not valid.
			return false;
		}
	}
	// If there are any platform media sources specified they will have been validated above.
	return PlatformMediaSources.Num() > 0;
#else
	return (MediaSource != nullptr) ? MediaSource->Validate() : false;
#endif
}


/* UPlatformMediaSource implementation
 *****************************************************************************/

UMediaSource* UPlatformMediaSource::GetMediaSource() const
{
#if WITH_EDITORONLY_DATA
	const FString RunningPlatformName(FPlatformProperties::IniPlatformName());
	UMediaSource*const* PlatformMediaSource = PlatformMediaSources.Find(RunningPlatformName);

	if (PlatformMediaSource == nullptr)
	{
		return nullptr;
	}

	return *PlatformMediaSource;
#else
	return MediaSource;
#endif
}


/* IMediaOptions interface
 *****************************************************************************/

bool UPlatformMediaSource::GetMediaOption(const FName& Key, bool DefaultValue) const
{
	// Guard against reentrant calls.
	static bool bIsGettingOption = false;
	if (bIsGettingOption)
	{
		UE_LOG(LogMediaAssets, Warning, TEXT("UPlatformMediaSource::GetMediaOption - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return DefaultValue;
	}
	TGuardValue<bool> GettingOptionGuard(bIsGettingOption, true);

	UMediaSource* PlatformMediaSource = GetMediaSource();
	
	if (PlatformMediaSource != nullptr)
	{
		return PlatformMediaSource->GetMediaOption(Key, DefaultValue);
	}
	
	return Super::GetMediaOption(Key, DefaultValue);
}


double UPlatformMediaSource::GetMediaOption(const FName& Key, double DefaultValue) const
{
	// Guard against reentrant calls.
	static bool bIsGettingOption = false;
	if (bIsGettingOption)
	{
		UE_LOG(LogMediaAssets, Warning, TEXT("UPlatformMediaSource::GetMediaOption - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return DefaultValue;
	}
	TGuardValue<bool> GettingOptionGuard(bIsGettingOption, true);

	UMediaSource* PlatformMediaSource = GetMediaSource();
	
	if (PlatformMediaSource != nullptr)
	{
		return PlatformMediaSource->GetMediaOption(Key, DefaultValue);
	}

	return Super::GetMediaOption(Key, DefaultValue);
}


int64 UPlatformMediaSource::GetMediaOption(const FName& Key, int64 DefaultValue) const
{
	// Guard against reentrant calls.
	static bool bIsGettingOption = false;
	if (bIsGettingOption)
	{
		UE_LOG(LogMediaAssets, Warning, TEXT("UPlatformMediaSource::GetMediaOption - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return DefaultValue;
	}
	TGuardValue<bool> GettingOptionGuard(bIsGettingOption, true);

	UMediaSource* PlatformMediaSource = GetMediaSource();
	
	if (PlatformMediaSource != nullptr)
	{
		return PlatformMediaSource->GetMediaOption(Key, DefaultValue);
	}

	return Super::GetMediaOption(Key, DefaultValue);
}


FString UPlatformMediaSource::GetMediaOption(const FName& Key, const FString& DefaultValue) const
{
	// Guard against reentrant calls.
	static bool bIsGettingOption = false;
	if (bIsGettingOption)
	{
		UE_LOG(LogMediaAssets, Warning, TEXT("UPlatformMediaSource::GetMediaOption - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return DefaultValue;
	}
	TGuardValue<bool> GettingOptionGuard(bIsGettingOption, true);

	UMediaSource* PlatformMediaSource = GetMediaSource();
	
	if (PlatformMediaSource != nullptr)
	{
		return PlatformMediaSource->GetMediaOption(Key, DefaultValue);
	}

	return Super::GetMediaOption(Key, DefaultValue);
}


FText UPlatformMediaSource::GetMediaOption(const FName& Key, const FText& DefaultValue) const
{
	// Guard against reentrant calls.
	static bool bIsGettingOption = false;
	if (bIsGettingOption)
	{
		UE_LOG(LogMediaAssets, Warning, TEXT("UPlatformMediaSource::GetMediaOption - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return DefaultValue;
	}
	TGuardValue<bool> GettingOptionGuard(bIsGettingOption, true);

	UMediaSource* PlatformMediaSource = GetMediaSource();
	
	if (PlatformMediaSource != nullptr)
	{
		return PlatformMediaSource->GetMediaOption(Key, DefaultValue);
	}

	return Super::GetMediaOption(Key, DefaultValue);
}


bool UPlatformMediaSource::HasMediaOption(const FName& Key) const
{
	// Guard against reentrant calls.
	static bool bIsHasOption = false;
	if (bIsHasOption)
	{
		UE_LOG(LogMediaAssets, Warning, TEXT("UPlatformMediaSource::HasMediaOption - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return false;
	}
	TGuardValue<bool> HasOptionGuard(bIsHasOption, true);

	UMediaSource* PlatformMediaSource = GetMediaSource();
	
	if (PlatformMediaSource != nullptr)
	{
		return PlatformMediaSource->HasMediaOption(Key);
	}

	return Super::HasMediaOption(Key);
}

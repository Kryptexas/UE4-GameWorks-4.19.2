// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetEditorPrivatePCH.h"


/* UMediaAssetFactory structors
 *****************************************************************************/

UMediaAssetFactory::UMediaAssetFactory( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	SupportedClass = UMediaAsset::StaticClass();

	bCreateNew = false;
	bEditorImport = true;

	// register media framework callbacks
	auto MediaModule = FModuleManager::GetModulePtr<IMediaModule>("Media");

	if (MediaModule != nullptr)
	{
		MediaModule->OnFactoryAdded().AddUObject(this, &UMediaAssetFactory::HandleMediaPlayerFactoryAdded);
		MediaModule->OnFactoryRemoved().AddUObject(this, &UMediaAssetFactory::HandleMediaPlayerFactoryRemoved);
	}

	ReloadMediaFormats();
}


/* UFactory overrides
 *****************************************************************************/

UObject* UMediaAssetFactory::FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn )
{
	UMediaAsset* MediaAsset = CastChecked<UMediaAsset>(StaticConstructObject(Class, InParent, Name, Flags));
	MediaAsset->OpenUrl(CurrentFilename);

	return MediaAsset;
}


/* UMediaAssetFactory implementation
 *****************************************************************************/

void UMediaAssetFactory::ReloadMediaFormats( )
{
	Formats.Reset();

	auto MediaModule = FModuleManager::GetModulePtr<IMediaModule>("Media");

	if (MediaModule != nullptr)
	{
		FMediaFormats SupportedFormats;
		MediaModule->GetSupportedFormats(SupportedFormats);

		for (auto& Format : SupportedFormats)
		{
			Formats.Add(Format.Key + TEXT(";") + Format.Value.ToString());
		}
	}
}


/* UMediaAssetFactory callbacks
 *****************************************************************************/

void UMediaAssetFactory::HandleMediaPlayerFactoryAdded( )
{
	ReloadMediaFormats();
}

void UMediaAssetFactory::HandleMediaPlayerFactoryRemoved( )
{
	ReloadMediaFormats();
}

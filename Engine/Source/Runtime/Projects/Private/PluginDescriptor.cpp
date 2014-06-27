// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ProjectsPrivatePCH.h"
#include "PluginDescriptor.h"

#define LOCTEXT_NAMESPACE "PluginDescriptor"

FPluginDescriptor::FPluginDescriptor()
{
	FileVersion = EPluginDescriptorVersion::Latest;
	Version = 0;
	bCanContainContent = false;
	bIsBetaVersion = false;
}

bool FPluginDescriptor::Load(const FString& FileName, FText& OutFailReason)
{
	// Read the file to a string
	FString FileContents;
	if (!FFileHelper::LoadFileToString(FileContents, *FileName))
	{
		OutFailReason = FText::Format( LOCTEXT("FailedToLoadDescriptorFile", "Failed to open descriptor file '{0}'"), FText::FromString( FileName ) );
		return false;
	}

	// Deserialize a JSON object from the string
	TSharedPtr< FJsonObject > Object;
	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(FileContents);
	if ( !FJsonSerializer::Deserialize(Reader, Object) || !Object.IsValid() )
	{
		OutFailReason = FText::Format( LOCTEXT("FailedToReadDescriptorFile", "Failed to read file. {0}"), FText::FromString(Reader->GetErrorMessage()) );
		return false;
	}

	// Parse it as a plugin descriptor
	return Read(*Object.Get(), OutFailReason);
}

bool FPluginDescriptor::Read(const FJsonObject& Object, FText& OutFailReason)
{
	// Read the file version
	int32 FileVersionInt32;
	if(!Object.TryGetNumberField(TEXT("FileVersion"), FileVersionInt32))
	{
		if(!Object.TryGetNumberField(TEXT("PluginFileVersion"), FileVersionInt32))
		{
			OutFailReason = LOCTEXT("InvalidProjectFileVersion", "File does not have a valid 'FileVersion' number.");
			return false;
		}
	}

	// Check that it's within range
	EPluginDescriptorVersion::Type FileVersion = (EPluginDescriptorVersion::Type)FileVersionInt32;
	if ( FileVersion <= EPluginDescriptorVersion::Invalid || FileVersion > EPluginDescriptorVersion::Latest )
	{
		FText ReadVersionText = FText::FromString( FString::Printf( TEXT( "%d" ), (int32)FileVersion ) );
		FText LatestVersionText = FText::FromString( FString::Printf( TEXT( "%d" ), (int32)EPluginDescriptorVersion::Latest ) );
		OutFailReason = FText::Format( LOCTEXT("ProjectFileVersionTooLarge", "File appears to be in a newer version ({0}) of the file format that we can load (max version: {1})."), ReadVersionText, LatestVersionText);
		return false;
	}

	// Read the other fields
	Object.TryGetNumberField(TEXT("Version"), Version);
	Object.TryGetStringField(TEXT("VersionName"), VersionName);
	Object.TryGetStringField(TEXT("FriendlyName"), FriendlyName);
	Object.TryGetStringField(TEXT("Description"), Description);

	if ( !Object.TryGetStringField(TEXT("Category"), Category) )
	{
		// Category used to be called CategoryPath in .uplugin files
		Object.TryGetStringField(TEXT("CategoryPath"), Category);
	}
        
	// Due to a difference in command line parsing between Windows and Mac, we shipped a few Mac samples containing
	// a category name with escaped quotes. Remove them here to make sure we can list them in the right category.
	if(Category.Len() >= 2 && Category.StartsWith(TEXT("\"")) && Category.EndsWith(TEXT("\"")))
	{
		Category = Category.Mid(1, Category.Len() - 2);
	}

	Object.TryGetStringField(TEXT("CreatedBy"), CreatedBy);
	Object.TryGetStringField(TEXT("CreatedByURL"), CreatedByURL);

	if(!FModuleDescriptor::ReadArray(Object, TEXT("Modules"), Modules, OutFailReason))
	{
		return false;
	}

	Object.TryGetBoolField(TEXT("CanContainContent"), bCanContainContent);
	Object.TryGetBoolField(TEXT("IsBetaVersion"), bIsBetaVersion);
	return true;
}

#undef LOCTEXT_NAMESPACE

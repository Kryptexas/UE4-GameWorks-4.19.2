// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AddContentDialogPCH.h"
#include "FeaturePackContentSource.h"

#include "AssetToolsModule.h"
#include "IPlatformFilePak.h"

FFeaturePackContentSource::FFeaturePackContentSource(FString InFeaturePackPath)
{
	FeaturePackPath = InFeaturePackPath;

	// Create a pak platform file and mount the feature pack file.
	FPakPlatformFile PakPlatformFile;
	FString CommandLine;
	PakPlatformFile.Initialize(&FPlatformFileManager::Get().GetPlatformFile(), TEXT(""));
	FString MountPoint = "root:/";
	PakPlatformFile.Mount(*InFeaturePackPath, 0, *MountPoint);

	// Gets the manifest file as a JSon string
	TArray<uint8> ManifestBuffer;
	LoadPakFileToBuffer(PakPlatformFile, FPaths::Combine(*MountPoint, TEXT("manifest.json")), ManifestBuffer);
	FString ManifestString;
	FFileHelper::BufferToString(ManifestString, ManifestBuffer.GetData(), ManifestBuffer.Num());

	// Populate text fields from the manifest.
	TSharedPtr<FJsonObject> ManifestObject;
	TSharedRef<TJsonReader<>> ManifestReader = TJsonReaderFactory<>::Create(ManifestString);
	FJsonSerializer::Deserialize(ManifestReader, ManifestObject);
	for (TSharedPtr<FJsonValue> NameValue : ManifestObject->GetArrayField("Name"))
	{
		TSharedPtr<FJsonObject> LocalizedNameObject = NameValue->AsObject();
		LocalizedNames.Add(FLocalizedText(
			LocalizedNameObject->GetStringField("Language"),
			FText::FromString(LocalizedNameObject->GetStringField("Text"))));
	}

	for (TSharedPtr<FJsonValue> DescriptionValue : ManifestObject->GetArrayField("Description"))
	{
		TSharedPtr<FJsonObject> LocalizedDescriptionObject = DescriptionValue->AsObject();
		LocalizedDescriptions.Add(FLocalizedText(
			LocalizedDescriptionObject->GetStringField("Language"),
			FText::FromString(LocalizedDescriptionObject->GetStringField("Text"))));
	}

	FString CategoryString = ManifestObject->GetStringField("Category");
	if (CategoryString == "CodeFeature")
	{
		Category = EContentSourceCategory::CodeFeature;
	}
	else if (CategoryString == "BlueprintFeature")
	{
		Category = EContentSourceCategory::BlueprintFeature;
	}
	else
	{
		Category = EContentSourceCategory::Unknown;
	}

	// Load image data
	FString IconFilename = ManifestObject->GetStringField("Thumbnail");
	TSharedPtr<TArray<uint8>> IconImageData = MakeShareable(new TArray<uint8>());
	LoadPakFileToBuffer(PakPlatformFile, FPaths::Combine(*MountPoint, TEXT("Media"), *IconFilename), *IconImageData);
	IconData = MakeShareable(new FImageData(IconFilename, IconImageData));

	const TArray<TSharedPtr<FJsonValue>> ScreenshotFilenameArray = ManifestObject->GetArrayField("Screenshots");
	for (const TSharedPtr<FJsonValue> ScreenshotFilename : ScreenshotFilenameArray)
	{
		TSharedPtr<TArray<uint8>> SingleScreenshotData = MakeShareable(new TArray<uint8>);
		LoadPakFileToBuffer(PakPlatformFile, FPaths::Combine(*MountPoint, TEXT("Media"), *ScreenshotFilename->AsString()), *SingleScreenshotData);
		ScreenshotData.Add(MakeShareable(new FImageData(ScreenshotFilename->AsString(), SingleScreenshotData)));
	}
}

void FFeaturePackContentSource::LoadPakFileToBuffer(FPakPlatformFile& PakPlatformFile, FString Path, TArray<uint8>& Buffer)
{
	TSharedPtr<IFileHandle> FileHandle(PakPlatformFile.OpenRead(*Path));
	Buffer.AddUninitialized(FileHandle->Size());
	FileHandle->Read(Buffer.GetData(), FileHandle->Size());
}

TArray<FLocalizedText> FFeaturePackContentSource::GetLocalizedNames()
{
	return LocalizedNames;
}

TArray<FLocalizedText> FFeaturePackContentSource::GetLocalizedDescriptions()
{
	return LocalizedDescriptions;
}

EContentSourceCategory FFeaturePackContentSource::GetCategory()
{
	return Category;
}

TSharedPtr<FImageData> FFeaturePackContentSource::GetIconData()
{
	return IconData;
}

TArray<TSharedPtr<FImageData>> FFeaturePackContentSource::GetScreenshotData()
{
	return ScreenshotData;
}

void FFeaturePackContentSource::InstallToProject(FString InstallPath)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	TArray<FString> AssetPaths;
	AssetPaths.Add(FeaturePackPath);
	AssetToolsModule.Get().ImportAssets(AssetPaths, InstallPath);
}

FFeaturePackContentSource::~FFeaturePackContentSource()
{
}
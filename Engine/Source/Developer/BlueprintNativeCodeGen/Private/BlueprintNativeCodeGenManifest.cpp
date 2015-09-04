// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintNativeCodeGenPCH.h"
#include "BlueprintNativeCodeGenManifest.h"
#include "NativeCodeGenCommandlineParams.h"
#include "App.h" // for GetGameName()
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonWriter.h"


/*******************************************************************************
 * BlueprintNativeCodeGenManifestImpl
 ******************************************************************************/

namespace BlueprintNativeCodeGenManifestImpl
{
	static const int64 CPF_NoFlags = 0x00;
	static const FString ManifestFileExt = TEXT(".bpgen.manifest");
	static const FString CppFileExt	     = TEXT(".cpp");
	static const FString HeaderFileExt   = TEXT(".h");
	static const FString HeaderSubDir    = TEXT("Public");
	static const FString CppSubDir       = TEXT("Private");

	/** */
	static FString GetManifestFilePath(const FString& ModulePath);

	/**  */
	static bool LoadManifest(FBlueprintNativeCodeGenManifest* Manifest);

	/**  */
	static FString GenerateHeaderSavePath(const FString& ModulePath, const FAssetData& Asset);
	static FString GenerateCppSavePath(const FString& ModulePath, const FAssetData& Asset);
}

//------------------------------------------------------------------------------
static FString BlueprintNativeCodeGenManifestImpl::GetManifestFilePath(const FString& ModulePath)
{
	return FPaths::Combine(*ModulePath, FApp::GetGameName()) + ManifestFileExt;
}

//------------------------------------------------------------------------------
static bool BlueprintNativeCodeGenManifestImpl::LoadManifest(FBlueprintNativeCodeGenManifest* Manifest)
{
	const FString ManifestFilePath = BlueprintNativeCodeGenManifestImpl::GetManifestFilePath(Manifest->GetTargetPath());

	FString ManifestStr;
	if (FFileHelper::LoadFileToString(ManifestStr, *ManifestFilePath))
	{
		TSharedRef< TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(ManifestStr);

		TSharedPtr<FJsonObject> JsonObject;
		if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
		{
			return FJsonObjectConverter::JsonObjectToUStruct<FBlueprintNativeCodeGenManifest>(JsonObject.ToSharedRef(), Manifest,
				/*CheckFlags =*/CPF_NoFlags, /*SkipFlags =*/CPF_NoFlags);
		}
	}
	return false;
}

//------------------------------------------------------------------------------
static FString BlueprintNativeCodeGenManifestImpl::GenerateHeaderSavePath(const FString& ModulePath, const FAssetData& Asset)
{
	// @TODO: Coordinate this with the CppBackend module
	return FPaths::Combine(*FPaths::Combine(*ModulePath, *HeaderSubDir), *Asset.AssetName.ToString()) + HeaderFileExt;
}

//------------------------------------------------------------------------------
static FString BlueprintNativeCodeGenManifestImpl::GenerateCppSavePath(const FString& ModulePath, const FAssetData& Asset)
{
	return FPaths::Combine(*FPaths::Combine(*ModulePath, *CppSubDir), *Asset.AssetName.ToString()) + CppFileExt;
}

/*******************************************************************************
 * FBlueprintNativeCodeGenManifest
 ******************************************************************************/

FBlueprintNativeCodeGenManifest::FBlueprintNativeCodeGenManifest(const FString TargetPath)
	: ModulePath(TargetPath)
{
	if (ModulePath.IsEmpty())
	{
		ModulePath = FPaths::GameIntermediateDir();
	}
	// do NOT load from an existing interface, as this is the default 
	// constructor used by the USTRUCT() system
}

//------------------------------------------------------------------------------
FBlueprintNativeCodeGenManifest::FBlueprintNativeCodeGenManifest(const FNativeCodeGenCommandlineParams& CommandlineParams)
{
	ModulePath = CommandlineParams.ModuleOutputDir;
	// incorporate an existing manifest (in case we're only re-converting a 
	// handful of assets and adding them into an existing module)
	if (BlueprintNativeCodeGenManifestImpl::LoadManifest(this))
	{
		// reset ModulePath afterwards (in case what we've loaded has been moved
		ModulePath = CommandlineParams.ModuleOutputDir;
	}
}

//------------------------------------------------------------------------------
FConvertedAssetRecord& FBlueprintNativeCodeGenManifest::CreateConversionRecord(const FAssetData& AssetInfo)
{
	FConvertedAssetRecord NewConversionRecord;
	NewConversionRecord.AssetType = AssetInfo.GetClass();
	NewConversionRecord.AssetPath = AssetInfo.PackagePath.ToString();
	NewConversionRecord.GeneratedHeaderPath = BlueprintNativeCodeGenManifestImpl::GenerateHeaderSavePath(ModulePath, AssetInfo);
	NewConversionRecord.GeneratedCppPath = BlueprintNativeCodeGenManifestImpl::GenerateCppSavePath(ModulePath, AssetInfo);

	return ConvertedAssets[ ConvertedAssets.Add(NewConversionRecord) ];
}

//------------------------------------------------------------------------------
bool FBlueprintNativeCodeGenManifest::Save() const
{
	TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

	if (FJsonObjectConverter::UStructToJsonObject(FBlueprintNativeCodeGenManifest::StaticStruct(), this, JsonObject,
		/*CheckFlags =*/BlueprintNativeCodeGenManifestImpl::CPF_NoFlags, /*SkipFlags =*/BlueprintNativeCodeGenManifestImpl::CPF_NoFlags))
	{
		FString FileContents;
		TSharedRef< TJsonWriter<> > JsonWriter = TJsonWriterFactory<>::Create(&FileContents);

		if (FJsonSerializer::Serialize(JsonObject, JsonWriter))
		{
			JsonWriter->Close();

			const FString ManifestFilePath = BlueprintNativeCodeGenManifestImpl::GetManifestFilePath(GetTargetPath());
			return FFileHelper::SaveStringToFile(FileContents, *ManifestFilePath);
		}
	}
	return false;
}


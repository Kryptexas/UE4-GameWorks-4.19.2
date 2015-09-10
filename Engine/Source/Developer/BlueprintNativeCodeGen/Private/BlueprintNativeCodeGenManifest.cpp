// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintNativeCodeGenPCH.h"
#include "BlueprintNativeCodeGenManifest.h"
#include "NativeCodeGenCommandlineParams.h"
#include "App.h" // for GetGameName()
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogNativeCodeGenManifest, Log, All);

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

	/**  */
	static bool LoadManifest(const FString& FilePath, FBlueprintNativeCodeGenManifest* Manifest);

	static FString GetBaseFilename(const FAssetData& Asset);

	/**  */
	static FString GenerateHeaderSavePath(const FString& ModulePath, const FAssetData& Asset);
	static FString GenerateCppSavePath(const FString& ModulePath, const FAssetData& Asset);

	static FString GetComparibleDirPath(const FString& DirectoryPath);
}

//------------------------------------------------------------------------------
static bool BlueprintNativeCodeGenManifestImpl::LoadManifest(const FString& FilePath, FBlueprintNativeCodeGenManifest* Manifest)
{
	FString ManifestStr;
	if (FFileHelper::LoadFileToString(ManifestStr, *FilePath))
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
static FString BlueprintNativeCodeGenManifestImpl::GetBaseFilename(const FAssetData& Asset)
{
	// @TODO: Coordinate this with the CppBackend module (so it matches includes)
	FString Filename = Asset.AssetName.ToString();
	if ( Asset.GetClass()->IsChildOf(UBlueprint::StaticClass()) )
	{
		static const FString BlueprintClassPostfix(TEXT("_C"));
		Filename += BlueprintClassPostfix;
	}
	return Filename;
}

//------------------------------------------------------------------------------
static FString BlueprintNativeCodeGenManifestImpl::GenerateHeaderSavePath(const FString& ModulePath, const FAssetData& Asset)
{
	return FPaths::Combine(*FPaths::Combine(*ModulePath, *HeaderSubDir), *GetBaseFilename(Asset)) + HeaderFileExt;
}

//------------------------------------------------------------------------------
static FString BlueprintNativeCodeGenManifestImpl::GenerateCppSavePath(const FString& ModulePath, const FAssetData& Asset)
{
	return FPaths::Combine(*FPaths::Combine(*ModulePath, *CppSubDir), *GetBaseFilename(Asset)) + CppFileExt;
}

//------------------------------------------------------------------------------
static FString BlueprintNativeCodeGenManifestImpl::GetComparibleDirPath(const FString& DirectoryPath)
{
	FString NormalizedPath = DirectoryPath;

	const FString PathDelim = TEXT("/");
	if (!NormalizedPath.EndsWith(PathDelim))
	{
		// to account for the case where the relative path would resolve to X: 
		// (when we want "X:/")... ConvertRelativePathToFull() leaves the 
		// trailing slash, and NormalizeDirectoryName() will remove it (if it is
		// not a drive letter)
		NormalizedPath += PathDelim;
	}

	if (FPaths::IsRelative(NormalizedPath))
	{
		NormalizedPath = FPaths::ConvertRelativePathToFull(NormalizedPath);
	}
	FPaths::NormalizeDirectoryName(NormalizedPath);
	return NormalizedPath;
}

/*******************************************************************************
 * FBlueprintNativeCodeGenManifest
 ******************************************************************************/

//------------------------------------------------------------------------------
FString FBlueprintNativeCodeGenManifest::GetDefaultFilename()
{
	return FApp::GetGameName() + BlueprintNativeCodeGenManifestImpl::ManifestFileExt;
}

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
	using namespace BlueprintNativeCodeGenManifestImpl;
	bool const bLoadExistingManifest = !CommandlineParams.bWipeRequested || CommandlineParams.ModuleOutputDir.IsEmpty();
	
	ModulePath = CommandlineParams.ModuleOutputDir;
	if (FPaths::IsRelative(ModulePath))
	{
		FPaths::MakePathRelativeTo(ModulePath, *FPaths::GameDir());
	}

	if (!CommandlineParams.ManifestFilePath.IsEmpty())
	{
		ManifestPath = CommandlineParams.ManifestFilePath;
	}
	else
	{
		if (!ensure(!ModulePath.IsEmpty()))
		{
			ModulePath = FPaths::Combine(*FPaths::GameIntermediateDir(), TEXT("BpCodeGen"));
		}
		ManifestPath = FPaths::Combine(*ModulePath, *GetDefaultFilename());
	}
	
	// incorporate an existing manifest (in case we're only re-converting a 
	// handful of assets and adding them into an existing module)
	if (bLoadExistingManifest && LoadManifest(ManifestPath, this))
	{
		// if they specified a separate module path, lets make sure we use that
		// over what was found in the existing manifest
		if (!CommandlineParams.ModuleOutputDir.IsEmpty())
		{
			if (CommandlineParams.bWipeRequested)
			{
				ModulePath = CommandlineParams.ModuleOutputDir;
			}
			else
			{
				const FString ExpectedPath = GetComparibleDirPath(CommandlineParams.ModuleOutputDir);
				const FString TargetPath   = GetComparibleDirPath(GetTargetPath());

				if ( !FPaths::IsSamePath(ExpectedPath, TargetPath) )
				{
					UE_LOG(LogNativeCodeGenManifest, Error, 
						TEXT("The existing manifest's module path does not match what was specified via the commandline."));
				}
			}
		}

		// if we were only interested in obtaining the module path
		if (CommandlineParams.bWipeRequested)
		{
			Clear();
		}
	}

	if (CommandlineParams.bWipeRequested)
	{
		const FString TargetPath = GetTargetPath();

		IFileManager& FileManager = IFileManager::Get();
		FileManager.DeleteDirectory(*FPaths::Combine(*TargetPath, *CppSubDir));
		FileManager.DeleteDirectory(*FPaths::Combine(*TargetPath, *HeaderSubDir));
	}
}

//------------------------------------------------------------------------------
FString FBlueprintNativeCodeGenManifest::GetTargetPath() const
{
	FString TargetPath = ModulePath;
	if (FPaths::IsRelative(ModulePath))
	{
		TargetPath = FPaths::ConvertRelativePathToFull(FPaths::GameDir(), ModulePath);
		TargetPath = FPaths::ConvertRelativePathToFull(TargetPath);
	}
	return TargetPath;
}

//------------------------------------------------------------------------------
FConvertedAssetRecord& FBlueprintNativeCodeGenManifest::CreateConversionRecord(const FAssetData& AssetInfo)
{
	FConvertedAssetRecord NewConversionRecord;
	NewConversionRecord.AssetType = AssetInfo.GetClass();
	NewConversionRecord.AssetPath = AssetInfo.PackagePath.ToString();
	NewConversionRecord.GeneratedHeaderPath = BlueprintNativeCodeGenManifestImpl::GenerateHeaderSavePath(GetTargetPath(), AssetInfo);
	NewConversionRecord.GeneratedCppPath    = BlueprintNativeCodeGenManifestImpl::GenerateCppSavePath(GetTargetPath(), AssetInfo);

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
			return FFileHelper::SaveStringToFile(FileContents, *ManifestPath);
		}
	}
	return false;
}

//------------------------------------------------------------------------------
void FBlueprintNativeCodeGenManifest::Clear()
{
	ConvertedAssets.Empty();
	ModuleDependencies.Empty();
}


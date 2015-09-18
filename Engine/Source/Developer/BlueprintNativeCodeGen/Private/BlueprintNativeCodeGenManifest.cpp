// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintNativeCodeGenPCH.h"
#include "BlueprintNativeCodeGenManifest.h"
#include "NativeCodeGenCommandlineParams.h"
#include "App.h" // for GetGameName()
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonWriter.h"
#include "IBlueprintCompilerCppBackendModule.h" // for GetBaseFilename()

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

	static FString GetHeaderSaveDir(const FString& ModulePath);
	static FString GetCppSaveDir(const FString& ModulePath);
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
static FString BlueprintNativeCodeGenManifestImpl::GetHeaderSaveDir(const FString& ModulePath)
{
	return FPaths::Combine(*ModulePath, *HeaderSubDir);
}

//------------------------------------------------------------------------------
static FString BlueprintNativeCodeGenManifestImpl::GetCppSaveDir(const FString& ModulePath)
{
	return FPaths::Combine(*ModulePath, *CppSubDir);
}

//------------------------------------------------------------------------------
static FString BlueprintNativeCodeGenManifestImpl::GetBaseFilename(const FAssetData& Asset)
{
	return IBlueprintCompilerCppBackendModule::GetBaseFilename(Asset.GetAsset());
}

//------------------------------------------------------------------------------
static FString BlueprintNativeCodeGenManifestImpl::GenerateHeaderSavePath(const FString& ModulePath, const FAssetData& Asset)
{
	return FPaths::Combine(*GetHeaderSaveDir(ModulePath), *GetBaseFilename(Asset)) + HeaderFileExt;
}

//------------------------------------------------------------------------------
static FString BlueprintNativeCodeGenManifestImpl::GenerateCppSavePath(const FString& ModulePath, const FAssetData& Asset)
{
	return FPaths::Combine(*GetCppSaveDir(ModulePath), *GetBaseFilename(Asset)) + CppFileExt;
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
		ManifestPath = FPaths::Combine(*GetModuleDir(), *GetDefaultFilename());
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

				FString TargetPath = GetModuleDir();
				TargetPath = GetComparibleDirPath(TargetPath);

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
		else
		{
			MapConvertedAssets();
		}
	}
}

//------------------------------------------------------------------------------
FConvertedAssetRecord& FBlueprintNativeCodeGenManifest::CreateConversionRecord(const FAssetData& AssetInfo)
{
	const FString AssetPath = AssetInfo.PackageName.ToString();
	if (FConvertedAssetRecord* ExistingConversionRecord = FindConversionRecord(AssetPath))
	{
		return *ExistingConversionRecord;
	}

	const FString TargetModulePath = GetModuleDir();

	FConvertedAssetRecord NewConversionRecord;
	NewConversionRecord.AssetPtr  = AssetInfo.GetAsset();
	NewConversionRecord.AssetType = AssetInfo.GetClass();
	NewConversionRecord.AssetPath = AssetPath;
	NewConversionRecord.GeneratedHeaderPath = BlueprintNativeCodeGenManifestImpl::GenerateHeaderSavePath(TargetModulePath, AssetInfo);
	NewConversionRecord.GeneratedCppPath    = BlueprintNativeCodeGenManifestImpl::GenerateCppSavePath(TargetModulePath, AssetInfo);

	RecordMap.Add(AssetPath, ConvertedAssets.Num());
	return ConvertedAssets[ ConvertedAssets.Add(NewConversionRecord) ];
}

//------------------------------------------------------------------------------
FConvertedAssetRecord* FBlueprintNativeCodeGenManifest::FindConversionRecord(const FString& AssetPath, bool bSlowSearch)
{
	FConvertedAssetRecord* FoundRecord = nullptr;
	if (int32* IndexPtr = RecordMap.Find(AssetPath))
	{
		check(*IndexPtr < ConvertedAssets.Num());
		FoundRecord = &ConvertedAssets[*IndexPtr];
		ensure(FoundRecord->AssetPath == AssetPath);
	}
	else if (bSlowSearch)
	{
		for (FConvertedAssetRecord& ConversionRecord : ConvertedAssets)
		{
			if (ConversionRecord.AssetPath == AssetPath)
			{
				FoundRecord = &ConversionRecord;
			}
		}
	}
	return FoundRecord;
}

//------------------------------------------------------------------------------
TArray<FString> FBlueprintNativeCodeGenManifest::GetTargetPaths() const 
{
	const FString TargetModulePath = GetModuleDir();

	TArray<FString> DestPaths;
	DestPaths.Add(ManifestPath);
	DestPaths.Add(BlueprintNativeCodeGenManifestImpl::GetHeaderSaveDir(TargetModulePath));
	DestPaths.Add(BlueprintNativeCodeGenManifestImpl::GetCppSaveDir(TargetModulePath));
	// @TODO: Add *Build.cs file

	return DestPaths;
}

//------------------------------------------------------------------------------
bool FBlueprintNativeCodeGenManifest::Save(bool bSort) const
{
	if (bSort)
	{
		ConvertedAssets.Sort([](const FConvertedAssetRecord& Lhs, const FConvertedAssetRecord& Rhs)->bool
		{ 
			if (Lhs.AssetType == Rhs.AssetType)
			{
				return Lhs.AssetPath < Rhs.AssetPath;
			}
			return Lhs.AssetType->GetName() < Rhs.AssetType->GetName();
		});
	}

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
	RecordMap.Empty();
}

//------------------------------------------------------------------------------
FString FBlueprintNativeCodeGenManifest::GetModuleDir() const
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
void FBlueprintNativeCodeGenManifest::MapConvertedAssets()
{
	RecordMap.Empty(ConvertedAssets.Num());

	for (int32 RecordIndex = 0; RecordIndex < ConvertedAssets.Num(); ++RecordIndex)
	{
		const FConvertedAssetRecord& AssetRecord = ConvertedAssets[RecordIndex];
		RecordMap.Add(AssetRecord.AssetPath, RecordIndex);
		
// 		if (UObject* AssetObj = AssetRecord.AssetPtr.Get())
// 		{
//			RecordMap.Add(AssetRecord.AssetPath, RecordIndex);
// 		}
// 		else
// 		{
// 			UE_LOG(LogBlueprintCodeGen, Warning, TEXT("Invalid conversion record found for: %s"), *AssetRecord.AssetPath);
// 		}
	}
}


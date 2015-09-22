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
	static const FString ManifestFileExt    = TEXT(".bpgen.manifest");
	static const FString CppFileExt	        = TEXT(".cpp");
	static const FString HeaderFileExt      = TEXT(".h");
	static const FString HeaderSubDir       = TEXT("Public");
	static const FString CppSubDir          = TEXT("Private");
	static const FString ModuleBuildFileExt = TEXT(".Build.cs");
	static const FString FallbackModuleName = TEXT("BpCodeGen");

	/**  */
	static bool LoadManifest(const FString& FilePath, FBlueprintNativeCodeGenManifest* Manifest);

	static FString GetHeaderSaveDir(const FString& ModulePath);
	static FString GetCppSaveDir(const FString& ModulePath);
	static FString GetBaseFilename(const FAssetData& Asset);

	/**  */
	static FString GenerateHeaderSavePath(const FString& ModulePath, const FAssetData& Asset);
	static FString GenerateCppSavePath(const FString& ModulePath, const FAssetData& Asset);

	static FString GetComparibleDirPath(const FString& DirectoryPath);

	static bool GatherModuleDependencies(const UObject* AssetObj, TArray<UPackage*>& DependenciesOut);
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

//------------------------------------------------------------------------------
static bool BlueprintNativeCodeGenManifestImpl::GatherModuleDependencies(const UObject* AssetObj, TArray<UPackage*>& DependenciesOut)
{
	UPackage* AssetPackage = AssetObj->GetOutermost();

	const FLinkerLoad* PkgLinker = FLinkerLoad::FindExistingLinkerForPackage(AssetPackage);
	const bool bFoundLinker = (PkgLinker != nullptr);

	if (ensure(bFoundLinker))
	{
		for (const FObjectImport& PkgImport : PkgLinker->ImportMap)
		{
			if (PkgImport.ClassName != NAME_Package)
			{
				continue;
			}

			UPackage* DependentPackage = FindObject<UPackage>(/*Outer =*/nullptr, *PkgImport.ObjectName.ToString(), /*ExactClass =*/true);
			if (DependentPackage == nullptr)
			{
				continue;
			}

			// we want only native packages, ones that are not editor-only
			if ((DependentPackage->PackageFlags & (PKG_CompiledIn | PKG_EditorOnly)) == PKG_CompiledIn)
			{
				DependenciesOut.AddUnique(DependentPackage);// PkgImport.ObjectName.ToString());
			}
		}
	}

	return bFoundLinker;
}

/*******************************************************************************
 * FConvertedAssetRecord
 ******************************************************************************/
 
//------------------------------------------------------------------------------
FConvertedAssetRecord::FConvertedAssetRecord(const FAssetData& AssetInfo, const FString& TargetModulePath)
	: AssetPtr(AssetInfo.GetAsset())
	, AssetType(AssetInfo.GetClass())
	, AssetPath(AssetInfo.PackageName.ToString())
	, GeneratedHeaderPath(BlueprintNativeCodeGenManifestImpl::GenerateHeaderSavePath(TargetModulePath, AssetInfo))
	, GeneratedCppPath(BlueprintNativeCodeGenManifestImpl::GenerateCppSavePath(TargetModulePath, AssetInfo))
{
}

//------------------------------------------------------------------------------
bool FConvertedAssetRecord::IsValid()
{
	// every conversion will have a header file (interfaces don't have implementation files)
	return AssetPtr.IsValid() && !GeneratedHeaderPath.IsEmpty() && (AssetType != nullptr) && !AssetPath.IsEmpty();
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
	
	ModuleName = CommandlineParams.ModuleName;
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
		if (!ensure(!ModuleName.IsEmpty()))
		{
			ModuleName = FallbackModuleName;
		}
		if (!ensure(!ModulePath.IsEmpty()))
		{
			ModulePath = FPaths::Combine(*FPaths::GameIntermediateDir(), *ModuleName);
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

		const bool bNewModuleNameRequested = !CommandlineParams.ModuleName.IsEmpty() && (CommandlineParams.ModuleName != ModuleName);
		if (bNewModuleNameRequested && !CommandlineParams.bPreviewRequested) 
		{
			// delete the old module file (if one exists)
			IFileManager::Get().Delete(*GetBuildFilePath());
		}

		// if we were only interested in obtaining the module path
		if (CommandlineParams.bWipeRequested)
		{
			ModuleName = CommandlineParams.ModuleName;
			Clear();
		}
		else
		{
			if (bNewModuleNameRequested)
			{
				UE_LOG(LogNativeCodeGenManifest, Warning, TEXT("The specified module name (%s) doesn't match the existing one (%s). Overridding with the new name."),
					*CommandlineParams.ModuleName, *ModuleName);
				ModuleName = CommandlineParams.ModuleName;
			}
			MapConvertedAssets();
		}
	}
}

//------------------------------------------------------------------------------
FConvertedAssetRecord& FBlueprintNativeCodeGenManifest::CreateConversionRecord(const FAssetData& AssetInfo)
{
	const FString AssetPath = AssetInfo.PackageName.ToString();
	const FString TargetModulePath = GetModuleDir();
	// load the asset (if it isn't already)
	const UObject* AssetObj = AssetInfo.GetAsset();

	FConvertedAssetRecord* ConversionRecordPtr = FindConversionRecord(AssetPath);
	if (ConversionRecordPtr == nullptr)
	{
		ConversionRecordPtr = &ConvertedAssets[ ConvertedAssets.Add(FConvertedAssetRecord(AssetInfo, TargetModulePath)) ];
	}
	else if ( !ensure((ConversionRecordPtr->AssetPath == AssetPath) && (ConversionRecordPtr->AssetType == AssetInfo.GetClass())) )
	{
		UE_LOG(LogNativeCodeGenManifest, Warning, 
			TEXT("The existing manifest entery for '%s' doesn't match what was expected (asset path and/or type). Updating it to match the asset."),
			*AssetPath);

		ConversionRecordPtr->AssetPath = AssetPath;
		ConversionRecordPtr->AssetType = AssetInfo.GetClass();
	}
	
	FConvertedAssetRecord& ConversionRecord = *ConversionRecordPtr;
	if (!ConversionRecord.IsValid())
	{
		// if this was a manifest entry that was loaded from an existing file, 
		// then it wouldn't have the asset pointer (which is transient data)
		if (!ConversionRecord.AssetPtr.IsValid())
		{
			ConversionRecord.AssetPtr = AssetObj;
		}
		if (ConversionRecord.GeneratedHeaderPath.IsEmpty())
		{
			ConversionRecord.GeneratedHeaderPath = BlueprintNativeCodeGenManifestImpl::GenerateHeaderSavePath(TargetModulePath, AssetInfo);
			ConversionRecord.GeneratedCppPath    = BlueprintNativeCodeGenManifestImpl::GenerateCppSavePath(TargetModulePath, AssetInfo);
		}
		ensure(ConversionRecord.IsValid());
	}

	return ConversionRecord;
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
bool FBlueprintNativeCodeGenManifest::LogDependencies(const FAssetData& AssetInfo)
{
	// load the asset (if it isn't already)
	const UObject* AssetObj = AssetInfo.GetAsset();

	return BlueprintNativeCodeGenManifestImpl::GatherModuleDependencies(AssetObj, ModuleDependencies);
}

//------------------------------------------------------------------------------
TArray<FString> FBlueprintNativeCodeGenManifest::GetTargetPaths() const 
{
	const FString TargetModulePath = GetModuleDir();

	TArray<FString> DestPaths;
	DestPaths.Add(ManifestPath);
	DestPaths.Add(BlueprintNativeCodeGenManifestImpl::GetHeaderSaveDir(TargetModulePath));
	DestPaths.Add(BlueprintNativeCodeGenManifestImpl::GetCppSaveDir(TargetModulePath));
	DestPaths.Add(GetBuildFilePath());

	return DestPaths;
}

//------------------------------------------------------------------------------
FString FBlueprintNativeCodeGenManifest::GetBuildFilePath() const
{
	return FPaths::Combine(*GetModuleDir(), *ModuleName) + BlueprintNativeCodeGenManifestImpl::ModuleBuildFileExt;
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
	}
}


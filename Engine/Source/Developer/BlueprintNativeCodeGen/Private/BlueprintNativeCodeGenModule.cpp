// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintNativeCodeGenPCH.h"

#include "AssetRegistryModule.h"
#include "BlueprintNativeCodeGenManifest.h"
#include "BlueprintNativeCodeGenModule.h"
#include "BlueprintNativeCodeGenUtils.h"
#include "IBlueprintCompilerCppBackendModule.h"
#include "Engine/Blueprint.h" // for EBlueprintType

/*******************************************************************************
* NativizationCookControllerImpl
******************************************************************************/

namespace NativizationCookControllerImpl
{
	// If you change this plugin name you must update logic in CookCommand.Automation.cs
	static const TCHAR* DefaultPluginName = TEXT("NativizedAssets");
}

/*******************************************************************************
 * FBlueprintNativeCodeGenModule
 ******************************************************************************/
 
class FBlueprintNativeCodeGenModule : public IBlueprintNativeCodeGenModule
									, public IBlueprintNativeCodeGenCore
{
public:
	FBlueprintNativeCodeGenModule()
	{
	}

	//~ Begin IBlueprintNativeCodeGenModule interface
	virtual void Convert(UPackage* Package, ESavePackageResult ReplacementType, const TCHAR* PlatformName) override;
	virtual void SaveManifest(int32 Id = -1) override;
	virtual void MergeManifest(int32 ManifestIdentifier) override;
	virtual void FinalizeManifest() override;
protected:
	virtual void Initialize(const FNativeCodeGenInitData& InitData) override;
	virtual void InitializeForRerunDebugOnly(const TArray< TPair< FString, FString > >& CodegenTargets) override;
	//~ End IBlueprintNativeCodeGenModule interface

	//~ Begin FScriptCookReplacmentCoordinator interface
	virtual EReplacementResult IsTargetedForReplacement(const UPackage* Package) const override;
	virtual EReplacementResult IsTargetedForReplacement(const UObject* Object) const override;
	virtual UClass* FindReplacedClass(const UClass* Class) const override;
	//~ End FScriptCookReplacmentCoordinator interface
private:
	FBlueprintNativeCodeGenManifest& GetManifest(const TCHAR* PlatformName);

	TMap< FString, TUniquePtr<FBlueprintNativeCodeGenManifest> > Manifests;

	TArray<FString> ExcludedAssetTypes;
	TArray<FString> ExcludedBlueprintTypes;
	TArray<FString> TargetPlatformNames;
};

void FBlueprintNativeCodeGenModule::Initialize(const FNativeCodeGenInitData& InitData)
{
	GConfig->GetArray(TEXT("BlueprintNativizationSettings"), TEXT("ExcludedAssetTypes"), ExcludedAssetTypes, GEditorIni);
	GConfig->GetArray(TEXT("BlueprintNativizationSettings"), TEXT("ExcludedBlueprintTypes"), ExcludedBlueprintTypes, GEditorIni);

	IBlueprintNativeCodeGenCore::Register(this);

	// Each platform will need a manifest, because each platform could cook different assets:
	for (auto& Platform : InitData.CodegenTargets)
	{
		const TCHAR* TargetDirectory = *Platform.Value;
		FString OutputPath = FPaths::Combine(TargetDirectory, NativizationCookControllerImpl::DefaultPluginName);

		Manifests.Add(FString(*Platform.Key), TUniquePtr<FBlueprintNativeCodeGenManifest>(new FBlueprintNativeCodeGenManifest(NativizationCookControllerImpl::DefaultPluginName, OutputPath)));

		TargetPlatformNames.Add(Platform.Key);
	}

	IBlueprintCompilerCppBackendModule& BackEndModule = (IBlueprintCompilerCppBackendModule&)IBlueprintCompilerCppBackendModule::Get();
	auto& ConversionQueryDelegate = BackEndModule.OnIsTargetedForConversionQuery();
	auto ShouldConvert = [](const UObject* AssetObj)
	{
		if (IBlueprintNativeCodeGenCore::Get())
		{
			EReplacementResult ReplacmentResult = IBlueprintNativeCodeGenCore::Get()->IsTargetedForReplacement(AssetObj);
			return ReplacmentResult == EReplacementResult::ReplaceCompletely;
		}
		return false;
	};
	ConversionQueryDelegate.BindStatic(ShouldConvert);
}

void FBlueprintNativeCodeGenModule::InitializeForRerunDebugOnly(const TArray< TPair< FString, FString > >& CodegenTargets)
{
	for (const auto& Platform : CodegenTargets)
	{
		// load the old manifest:
		const TCHAR* TargetDirectory = *Platform.Value;
		FString OutputPath = FPaths::Combine(TargetDirectory, NativizationCookControllerImpl::DefaultPluginName, *FBlueprintNativeCodeGenPaths::GetDefaultManifestPath());
		Manifests.Add(FString(*Platform.Key), TUniquePtr<FBlueprintNativeCodeGenManifest>(new FBlueprintNativeCodeGenManifest(FPaths::ConvertRelativePathToFull(OutputPath))));
		//FBlueprintNativeCodeGenManifest OldManifest(FPaths::ConvertRelativePathToFull(OutputPath));
		// reconvert every assets listed in the manifest:
		for (const auto& ConversionTarget : GetManifest(*Platform.Key).GetConversionRecord())
		{
			// load the package:
			UPackage* Package = LoadPackage(nullptr, *ConversionTarget.Value.TargetObjPath, LOAD_None);

			// reconvert it:
			Convert(Package, ESavePackageResult::ReplaceCompletely, *Platform.Key);
		}

		// reconvert every unconverted dependency listed in the manifest:
		for (const auto& ConversionTarget : GetManifest(*Platform.Key).GetUnconvertedDependencies())
		{
			// load the package:
			UPackage* Package = LoadPackage(nullptr, *ConversionTarget.Key.GetPlainNameString(), LOAD_None);

			// reconvert it:
			Convert(Package, ESavePackageResult::GenerateStub, *Platform.Key);
		}
	}

}

FBlueprintNativeCodeGenManifest& FBlueprintNativeCodeGenModule::GetManifest(const TCHAR* PlatformName)
{
	FString PlatformNameStr(PlatformName);
	TUniquePtr<FBlueprintNativeCodeGenManifest>* Result = Manifests.Find(PlatformNameStr);
	check(Result->IsValid());
	return **Result;
}

void FBlueprintNativeCodeGenModule::Convert(UPackage* Package, ESavePackageResult CookResult, const TCHAR* PlatformName)
{
	if (CookResult != ESavePackageResult::ReplaceCompletely && CookResult != ESavePackageResult::GenerateStub)
	{
		// nothing to convert
		return;
	}

	// Find the struct/enum to convert:
	UStruct* Struct = nullptr;
	UEnum* Enum = nullptr;
	TArray<UObject*> Objects;
	GetObjectsWithOuter(Package, Objects, false);
	for (auto Entry : Objects)
	{
		if (Entry->HasAnyFlags(RF_Transient))
		{
			continue;
		}

		Struct = Cast<UStruct>(Entry);
		if (Struct)
		{
			break;
		}

		Enum = Cast<UEnum>(Entry);
		if (Enum)
		{
			break;
		}
	}

	if (Struct == nullptr && Enum == nullptr)
	{
		check(false);
		return;
	}

	IBlueprintCompilerCppBackendModule& BackEndModule = (IBlueprintCompilerCppBackendModule&)IBlueprintCompilerCppBackendModule::Get();
	auto& BackendPCHQuery = BackEndModule.OnPCHFilenameQuery();

	FBlueprintNativeCodeGenPaths TargetPaths = GetManifest(PlatformName).GetTargetPaths();
	BackendPCHQuery.BindLambda([TargetPaths]()->FString
	{
		return TargetPaths.RuntimePCHFilename();
	});

	const IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	if (CookResult == ESavePackageResult::GenerateStub)
	{
		FAssetData AssetInfo = Registry.GetAssetByObjectPath(*Struct->GetPathName());
		FString FileContents;
		TUniquePtr<IBlueprintCompilerCppBackend> Backend_CPP(IBlueprintCompilerCppBackendModuleInterface::Get().Create());
		// Apparently we can only generate wrappers for classes, so any logic that results in non classes requesting
		// wrappers will fail here:

		FileContents = Backend_CPP->GenerateWrapperForClass(CastChecked<UClass>(Struct));
		if (!FileContents.IsEmpty())
		{
			FFileHelper::SaveStringToFile(FileContents, *(GetManifest(PlatformName).CreateUnconvertedDependencyRecord(AssetInfo.PackageName, AssetInfo).GeneratedWrapperPath));
		}
		// The stub we generate still may have dependencies on other modules, so make sure the module dependencies are 
		// still recorded so that the .build.cs is generated correctly. Without this you'll get include related errors 
		// (or possibly linker errors) in stub headers:
		GetManifest(PlatformName).GatherModuleDependencies(Package);
	}
	else
	{
		check(CookResult == ESavePackageResult::ReplaceCompletely);
		// convert:
		UField* ForConversion = Enum;
		if (ForConversion == nullptr)
		{
			ForConversion = Struct;
		}

		FAssetData AssetInfo = Registry.GetAssetByObjectPath(*ForConversion->GetPathName());
		FConvertedAssetRecord& ConversionRecord = GetManifest(PlatformName).CreateConversionRecord(*ForConversion->GetPathName(), AssetInfo);
		TSharedPtr<FString> HeaderSource(new FString());
		TSharedPtr<FString> CppSource(new FString());

		FBlueprintNativeCodeGenUtils::GenerateCppCode(ForConversion, HeaderSource, CppSource);
		bool bSuccess = !HeaderSource->IsEmpty() || !CppSource->IsEmpty();
		// Run the cpp first, because we cue off of the presence of a header for a valid conversion record (see
		// FConvertedAssetRecord::IsValid)
		if (!CppSource->IsEmpty())
		{
			if (!FFileHelper::SaveStringToFile(*CppSource, *ConversionRecord.GeneratedCppPath))
			{
				bSuccess &= false;
				ConversionRecord.GeneratedCppPath.Empty();
			}
			CppSource->Empty(CppSource->Len());
		}
		else
		{
			ConversionRecord.GeneratedCppPath.Empty();
		}

		if (bSuccess && !HeaderSource->IsEmpty())
		{
			if (!FFileHelper::SaveStringToFile(*HeaderSource, *ConversionRecord.GeneratedHeaderPath))
			{
				bSuccess &= false;
				ConversionRecord.GeneratedHeaderPath.Empty();
			}
			HeaderSource->Empty(HeaderSource->Len());
		}
		else
		{
			ConversionRecord.GeneratedHeaderPath.Empty();
		}

		check(bSuccess);
		if (bSuccess)
		{
			GetManifest(PlatformName).GatherModuleDependencies(Package);
		}
	}

	BackendPCHQuery.Unbind();
}

void FBlueprintNativeCodeGenModule::SaveManifest(int32 Id )
{
	for (auto& PlatformName : TargetPlatformNames)
	{
		GetManifest(*PlatformName).Save(Id);
	}
}

void FBlueprintNativeCodeGenModule::MergeManifest(int32 ManifestIdentifier)
{
	for (auto& PlatformName : TargetPlatformNames)
	{
		FBlueprintNativeCodeGenManifest& CurrentManifest = GetManifest(*PlatformName);
		FBlueprintNativeCodeGenManifest OtherManifest = FBlueprintNativeCodeGenManifest(CurrentManifest.GetTargetPaths().ManifestFilePath() + FString::FromInt(ManifestIdentifier));
		CurrentManifest.Merge(OtherManifest);
	}
} 

void FBlueprintNativeCodeGenModule::FinalizeManifest()
{
	for(auto& PlatformName : TargetPlatformNames)
	{
		GetManifest(*PlatformName).Save(-1);
		check(FBlueprintNativeCodeGenUtils::FinalizePlugin(GetManifest(*PlatformName)));
	}
}

UClass* FBlueprintNativeCodeGenModule::FindReplacedClass(const UClass* Class) const
{
	// we're only looking to replace class types:
	while (Class)
	{
		if (Class == UUserDefinedEnum::StaticClass())
		{
			return UEnum::StaticClass();
		}
		if (Class == UUserDefinedStruct::StaticClass())
		{
			return UScriptStruct::StaticClass();
		}
		if (Class == UBlueprintGeneratedClass::StaticClass())
		{
			return UDynamicClass::StaticClass();
		}

		Class = Class->GetSuperClass();
	}

	return nullptr;
}

EReplacementResult FBlueprintNativeCodeGenModule::IsTargetedForReplacement(const UPackage* Package) const
{
	// non-native packages with enums and structs should be converted, unless they are blacklisted:
	UStruct* Struct = nullptr;
	UEnum* Enum = nullptr;
	TArray<UObject*> Objects;
	GetObjectsWithOuter(Package, Objects, false);
	for (auto Entry : Objects)
	{
		Struct = Cast<UStruct>(Entry);
		Enum = Cast<UEnum>(Entry);
		if (Struct || Enum)
		{
			break;
		}
	}

	UObject* Target = Struct;
	if (Target == nullptr)
	{
		Target = Enum;
	}
	return IsTargetedForReplacement(Target);
}

EReplacementResult FBlueprintNativeCodeGenModule::IsTargetedForReplacement(const UObject* Object) const
{
	const UStruct* Struct = Cast<UStruct>(Object);
	const UEnum* Enum = Cast<UEnum>(Object);

	if (Struct == nullptr && Enum == nullptr)
	{
		return EReplacementResult::DontReplace;
	}

	if (const UClass* BlueprintClass = Cast<UClass>(Struct))
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy))
		{
			const EBlueprintType UnconvertableBlueprintTypes[] = {
				//BPTYPE_Const,		// WTF is a "const" Blueprint?
				BPTYPE_MacroLibrary,
				BPTYPE_LevelScript,
			};

			EBlueprintType BlueprintType = Blueprint->BlueprintType;
			for (int32 TypeIndex = 0; TypeIndex < ARRAY_COUNT(UnconvertableBlueprintTypes); ++TypeIndex)
			{
				if (BlueprintType == UnconvertableBlueprintTypes[TypeIndex])
				{
					return EReplacementResult::DontReplace;
				}
			}
		}
	}

	auto IsObjectFromDeveloperPackage = [](const UObject* Obj) -> bool
	{
		return Obj && Obj->GetOutermost()->HasAllPackagesFlags(PKG_Developer);
	};

	auto IsDeveloperObject = [&](const UObject* Obj) -> bool
	{
		if (Obj)
		{
			if (IsObjectFromDeveloperPackage(Obj))
			{
				return true;
			}
			const UStruct* StructToTest = Obj->IsA<UStruct>() ? CastChecked<const UStruct>(Obj) : Obj->GetClass();
			for (; StructToTest; StructToTest = StructToTest->GetSuperStruct())
			{
				if (IsObjectFromDeveloperPackage(StructToTest))
				{
					return true;
				}
			}
		}
		return false;
	};

	if (Object && (IsEditorOnlyObject(Object) || IsDeveloperObject(Object)))
	{
		UE_LOG(LogBlueprintCodeGen, Warning, TEXT("Object %s depends on Editor or Development stuff. Is shouldn't be cooked."), *GetPathNameSafe(Object));
		return EReplacementResult::DontReplace;
	}

	// check blacklists:
	// we can't use FindObject, because we may be converting a type while saving
	if ((Struct && ExcludedAssetTypes.Find(Struct->GetPathName()) != INDEX_NONE) ||
		(Enum && ExcludedAssetTypes.Find(Enum->GetPathName()) != INDEX_NONE))
	{
		return EReplacementResult::GenerateStub;
	}

	EReplacementResult Result = EReplacementResult::ReplaceCompletely;
	while (Struct)
	{
		// This happens because the cooker incorrectly cooks editor only packages. Specifically happens for the blackjack sample
		// project due to a FStringAssetReference in BaseEditor.ini:
		if (Struct->RootPackageHasAnyFlags(PKG_EditorOnly))
		{
			return EReplacementResult::DontReplace;
		}

		if (ExcludedBlueprintTypes.Find(Struct->GetPathName()) != INDEX_NONE)
		{
			Result = EReplacementResult::GenerateStub;
		}
		Struct = Struct->GetSuperStruct();
	}

	return Result;
}

IMPLEMENT_MODULE(FBlueprintNativeCodeGenModule, BlueprintNativeCodeGen);
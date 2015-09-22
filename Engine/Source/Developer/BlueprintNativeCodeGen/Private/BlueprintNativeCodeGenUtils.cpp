// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintNativeCodeGenPCH.h"
#include "BlueprintNativeCodeGenUtils.h"
#include "NativeCodeGenCommandlineParams.h"
#include "BlueprintNativeCodeGenCoordinator.h"
#include "Kismet2/KismetReinstanceUtilities.h"	// for FBlueprintCompileReinstancer
#include "Kismet2/CompilerResultsLog.h" 
#include "KismetCompilerModule.h"
#include "Engine/Blueprint.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "Kismet2/KismetEditorUtilities.h"		// for CompileBlueprint()
#include "OutputDevice.h"						// for GWarn
#include "GameProjectUtils.h"					// for GenerateGameModuleBuildFile
#include "Editor/GameProjectGeneration/Public/GameProjectUtils.h" // for GenerateGameModuleBuildFile()
#include "App.h"								// for GetGameName()

DEFINE_LOG_CATEGORY(LogBlueprintCodeGen)

/*******************************************************************************
 * BlueprintNativeCodeGenUtilsImpl
 ******************************************************************************/

namespace BlueprintNativeCodeGenUtilsImpl
{
	static bool WipeTargetPaths(const TArray<FString>& TargetPaths);
	static bool GenerateModuleBuildFile(const FString& ModuleName, const FBlueprintNativeCodeGenManifest& Manifest);
}

//------------------------------------------------------------------------------
static bool BlueprintNativeCodeGenUtilsImpl::WipeTargetPaths(const TArray<FString>& TargetPaths)
{
	IFileManager& FileManager = IFileManager::Get();

	bool bSuccess = true;
	for (const FString& Path : TargetPaths)
	{
		if (FileManager.FileExists(*Path))
		{
			bSuccess &= FileManager.Delete(*Path);
		}
		else if (FileManager.DirectoryExists(*Path))
		{
			bSuccess &= FileManager.DeleteDirectory(*Path, /*RequireExists =*/false, /*Tree =*/true);
		}
	}

	return bSuccess;
}

//------------------------------------------------------------------------------
static bool BlueprintNativeCodeGenUtilsImpl::GenerateModuleBuildFile(const FString& ModuleName, const FBlueprintNativeCodeGenManifest& Manifest)
{
	FModuleManager& ModuleManager = FModuleManager::Get();
	
	TArray<FString> PublicDependencies;
	if (GameProjectUtils::ProjectHasCodeFiles()) 
	{
		const FString GameModuleName = FApp::GetGameName();
		if (ModuleManager.ModuleExists(*GameModuleName))
		{
			PublicDependencies.Add(GameModuleName);
		}
	}

	TArray<FString> PrivateDependencies;

	const TArray<UPackage*>& ModulePackages = Manifest.GetModuleDependencies();
	PrivateDependencies.Reserve(ModulePackages.Num());

	for (UPackage* ModulePkg : ModulePackages)
	{
		const FString ModuleName = FPackageName::GetLongPackageAssetName(ModulePkg->GetName());
		if (ModuleManager.ModuleExists(*ModuleName))
		{
			PrivateDependencies.Add(ModuleName);
		}
		else
		{
			UE_LOG(LogBlueprintCodeGen, Warning, TEXT("Failed to find module for package: %s"), *ModuleName);
		}
	}

	const FString BuildFilePath = Manifest.GetBuildFilePath();
	FText ErrorMessage;
	GameProjectUtils::GenerateGameModuleBuildFile(BuildFilePath, ModuleName, PublicDependencies, PrivateDependencies, ErrorMessage);

	if (!ErrorMessage.IsEmpty())
	{
		UE_LOG(LogBlueprintCodeGen, Error, TEXT("Failed to generate module build file: %s"), *ErrorMessage.ToString());
	}
	return !ErrorMessage.IsEmpty();
}

/*******************************************************************************
 * FBlueprintNativeCodeGenUtils
 ******************************************************************************/

//------------------------------------------------------------------------------
bool FBlueprintNativeCodeGenUtils::GenerateCodeModule(const FNativeCodeGenCommandlineParams& CommandParams)
{
	FScopedFeedbackContext ScopedErrorTracker;
	FBlueprintNativeCodeGenCoordinator Coordinator(CommandParams);

	if (ScopedErrorTracker.HasErrors())
	{
		// creating the coordinator/manifest produced an error... do not carry on!
		return false;
	}
	const FBlueprintNativeCodeGenManifest& Manifest = Coordinator.GetManifest();

	if (CommandParams.bWipeRequested && !CommandParams.bPreviewRequested)
	{
		TArray<FString> TargetPaths = Manifest.GetTargetPaths();
		if (!BlueprintNativeCodeGenUtilsImpl::WipeTargetPaths(TargetPaths))
		{
			UE_LOG(LogBlueprintCodeGen, Warning, TEXT("Failed to wipe target files/directories."));
		}
	}

	TSharedPtr<FString> HeaderSource(new FString());
	TSharedPtr<FString> CppSource(new FString());

	auto ConvertSingleAsset = [&Coordinator, &HeaderSource, &CppSource](FConvertedAssetRecord& ConversionRecord)->bool
	{
		FScopedFeedbackContext NestedErrorTracker;

		UObject* AssetObj = ConversionRecord.AssetPtr.Get();
		if (AssetObj != nullptr)
		{
			FBlueprintNativeCodeGenUtils::GenerateCppCode(AssetObj, HeaderSource, CppSource);
		}

		bool bSuccess = !HeaderSource->IsEmpty() || !CppSource->IsEmpty();
		// run the cpp first, because we cue off of the presence of a header for 
		// a valid conversion record (see FConvertedAssetRecord::IsValid)
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
		return bSuccess && !NestedErrorTracker.HasErrors();
	};

	FBlueprintNativeCodeGenCoordinator::FConversionDelegate ConversionDelegate = FBlueprintNativeCodeGenCoordinator::FConversionDelegate::CreateLambda(ConvertSingleAsset);
	if (CommandParams.bPreviewRequested)
	{
		ConversionDelegate.BindLambda([](FConvertedAssetRecord& ConversionRecord){ return true; });
	}

	const bool bSuccess = Coordinator.ProcessConversionQueue(ConversionDelegate);
	if (bSuccess && !CommandParams.bPreviewRequested)
	{
		BlueprintNativeCodeGenUtilsImpl::GenerateModuleBuildFile(CommandParams.ModuleName, Manifest);
	}

	// save the manifest regardless of success (so we can get an idea of how successful it was)
	Manifest.Save();
	return bSuccess & !ScopedErrorTracker.HasErrors();
}

//------------------------------------------------------------------------------
void FBlueprintNativeCodeGenUtils::GenerateCppCode(UObject* Obj, TSharedPtr<FString> OutHeaderSource, TSharedPtr<FString> OutCppSource)
{
	auto UDEnum = Cast<UUserDefinedEnum>(Obj);
	auto UDStruct = Cast<UUserDefinedStruct>(Obj);
	auto BPGC = Cast<UClass>(Obj);
	auto InBlueprintObj = BPGC ? Cast<UBlueprint>(BPGC->ClassGeneratedBy) : Cast<UBlueprint>(Obj);

	OutHeaderSource->Empty();
	OutCppSource->Empty();

	if (InBlueprintObj)
	{
		check(InBlueprintObj->GetOutermost() != GetTransientPackage());
		checkf(InBlueprintObj->GeneratedClass, TEXT("Invalid generated class for %s"), *InBlueprintObj->GetName());
		check(OutHeaderSource.IsValid());
		check(OutCppSource.IsValid());

		auto BlueprintObj = InBlueprintObj;
		{
			TSharedPtr<FBlueprintCompileReinstancer> Reinstancer = FBlueprintCompileReinstancer::Create(BlueprintObj->GeneratedClass);

			IKismetCompilerInterface& Compiler = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(KISMET_COMPILER_MODULENAME);
			
			TGuardValue<bool> GuardTemplateNameFlag(GCompilingBlueprint, true);
			FCompilerResultsLog Results;			

			FKismetCompilerOptions CompileOptions;
			CompileOptions.CompileType = EKismetCompileType::Cpp;
			CompileOptions.OutCppSourceCode = OutCppSource;
			CompileOptions.OutHeaderSourceCode = OutHeaderSource;
			Compiler.CompileBlueprint(BlueprintObj, CompileOptions, Results);

			if (EBlueprintType::BPTYPE_Interface == BlueprintObj->BlueprintType && OutCppSource.IsValid())
			{
				OutCppSource->Empty(); // ugly temp hack
			}
		}
		FKismetEditorUtilities::CompileBlueprint(BlueprintObj);
	}
	else if ((UDEnum || UDStruct) && OutHeaderSource.IsValid())
	{
		IKismetCompilerInterface& Compiler = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(KISMET_COMPILER_MODULENAME);
		if (UDEnum)
		{
			*OutHeaderSource = Compiler.GenerateCppCodeForEnum(UDEnum);
		}
		else if (UDStruct)
		{
			*OutHeaderSource = Compiler.GenerateCppCodeForStruct(UDStruct);
		}
	}
	else
	{
		ensure(false);
	}
}

/*******************************************************************************
 * FScopedFeedbackContext
 ******************************************************************************/

//------------------------------------------------------------------------------
FBlueprintNativeCodeGenUtils::FScopedFeedbackContext::FScopedFeedbackContext()
	: OldContext(GWarn)
	, ErrorCount(0)
	, WarningCount(0)
{
	TreatWarningsAsErrors = GWarn->TreatWarningsAsErrors;
	GWarn = this;
}

//------------------------------------------------------------------------------
FBlueprintNativeCodeGenUtils::FScopedFeedbackContext::~FScopedFeedbackContext()
{
	GWarn = OldContext;
}

//------------------------------------------------------------------------------
bool FBlueprintNativeCodeGenUtils::FScopedFeedbackContext::HasErrors()
{
	return (ErrorCount > 0) || (TreatWarningsAsErrors && (WarningCount > 0));
}

//------------------------------------------------------------------------------
void FBlueprintNativeCodeGenUtils::FScopedFeedbackContext::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category)
{
	switch (Verbosity)
	{
	case ELogVerbosity::Warning:
		{
			++WarningCount;
		} 
		break;

	case ELogVerbosity::Error:
	case ELogVerbosity::Fatal:
		{
			++ErrorCount;
		} 
		break;

	default:
		break;
	}

	OldContext->Serialize(V, Verbosity, Category);
}

//------------------------------------------------------------------------------
void FBlueprintNativeCodeGenUtils::FScopedFeedbackContext::Flush()
{
	WarningCount = ErrorCount = 0;
	OldContext->Flush();
}

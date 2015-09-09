// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintNativeCodeGenPCH.h"
#include "BlueprintNativeCodeGenUtils.h"
#include "BlueprintNativeCodeGenCoordinator.h"
#include "Kismet2/KismetReinstanceUtilities.h"	// for FBlueprintCompileReinstancer
#include "Kismet2/CompilerResultsLog.h" 
#include "KismetCompilerModule.h"
#include "Engine/Blueprint.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "Kismet2/KismetEditorUtilities.h"		// for CompileBlueprint()
#include "OutputDevice.h"						// for GWarn

/*******************************************************************************
 * BlueprintNativeCodeGenUtilsImpl
 ******************************************************************************/

namespace BlueprintNativeCodeGenUtilsImpl
{
	
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

/*******************************************************************************
 * FBlueprintNativeCodeGenUtils
 ******************************************************************************/

//------------------------------------------------------------------------------
bool FBlueprintNativeCodeGenUtils::GenerateCodeModule(FBlueprintNativeCodeGenCoordinator& Coordinator)
{
	FBlueprintNativeCodeGenManifest& Manifest = Coordinator.Manifest;

	TSharedPtr<FString> HeaderSource(new FString());
	TSharedPtr<FString> CppSource(   new FString());

	bool bSuccess = (Coordinator.ConversionQueue.Num() > 0);
	for (const FAssetData& Asset : Coordinator.ConversionQueue)
	{
		FScopedFeedbackContext ScopedErrorTracker;
		FConvertedAssetRecord& ConversionRecord = Manifest.CreateConversionRecord(Asset);

		// loads the object if it is not already loaded
		UObject* AssetObj = Asset.GetAsset();
		FBlueprintNativeCodeGenUtils::GenerateCppCode(AssetObj, HeaderSource, CppSource);

		bSuccess &= !HeaderSource->IsEmpty() || !CppSource->IsEmpty();
		if (!HeaderSource->IsEmpty())
		{
			bSuccess &= FFileHelper::SaveStringToFile(*HeaderSource, *ConversionRecord.GeneratedHeaderPath);
		}
		else
		{
			ConversionRecord.GeneratedHeaderPath.Empty();
		}

		if (!CppSource->IsEmpty())
		{
			bSuccess &= FFileHelper::SaveStringToFile(*CppSource, *ConversionRecord.GeneratedCppPath);
		}
		else
		{
			ConversionRecord.GeneratedCppPath.Empty();
		}

		if (ScopedErrorTracker.HasErrors())
		{
			bSuccess = false;
			break;
		}
	}
	Coordinator.Manifest.Save();

	return bSuccess;
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

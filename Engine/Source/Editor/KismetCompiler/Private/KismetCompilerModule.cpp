// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	KismetCompilerModule.cpp
=============================================================================*/

#include "KismetCompilerPrivatePCH.h"

#include "KismetCompiler.h"
#include "AnimBlueprintCompiler.h"

#include "Editor/UnrealEd/Public/Kismet2/KismetDebugUtilities.h"
#include "Editor/UnrealEd/Public/Kismet2/KismetReinstanceUtilities.h"

#include "UserDefinedStructureCompilerUtils.h"

DEFINE_LOG_CATEGORY(LogK2Compiler);

IMPLEMENT_MODULE( FKismet2CompilerModule, KismetCompiler );


#define LOCTEXT_NAMESPACE "KismetCompiler"

//////////////////////////////////////////////////////////////////////////
// FKismet2CompilerModule

struct FBlueprintIsBeingCompiledHelper
{
private:
	UBlueprint* Blueprint;
public:
	FBlueprintIsBeingCompiledHelper(UBlueprint* InBlueprint) : Blueprint(InBlueprint)
	{
		check(NULL != Blueprint);
		check(!Blueprint->bBeingCompiled);
		Blueprint->bBeingCompiled = true;
	}

	~FBlueprintIsBeingCompiledHelper()
	{
		Blueprint->bBeingCompiled = false;
	}
};

// Compiles a blueprint.
void FKismet2CompilerModule::CompileBlueprintInner(class UBlueprint* Blueprint, bool bPrintResultSuccess, const FKismetCompilerOptions& CompileOptions, FCompilerResultsLog& Results)
{
	FBlueprintIsBeingCompiledHelper BeingCompiled(Blueprint);

	Blueprint->CurrentMessageLog = &Results;

	double CompileStartTime = FPlatformTime::Seconds();

	// Early out if blueprint parent is missing
	if (Blueprint->ParentClass == NULL)
	{
		Results.Error(*LOCTEXT("KismetCompileError_MalformedParentClasss", "Blueprint @@ has missing or NULL parent class.").ToString(), Blueprint);
	}
	else if (UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(Blueprint))
	{
		FAnimBlueprintCompiler Compiler(AnimBlueprint, Results, CompileOptions);
		Compiler.Compile();
		check(Compiler.NewClass);
	}
	else
	{
		FKismetCompilerContext Compiler(Blueprint, Results, CompileOptions);
		Compiler.Compile();
		check(Compiler.NewClass);
	}

	double FinishTime = FPlatformTime::Seconds();

	FNumberFormattingOptions TimeFormat;
	TimeFormat.MaximumFractionalDigits = 2;
	TimeFormat.MinimumFractionalDigits = 2;
	TimeFormat.MaximumIntegralDigits = 4;
	TimeFormat.MinimumIntegralDigits = 4;
	TimeFormat.UseGrouping = false;

	FFormatNamedArguments Args;
	Args.Add(TEXT("Time"), FText::AsNumber(FinishTime - GStartTime, &TimeFormat));
	Args.Add(TEXT("BlueprintName"), FText::FromString(Blueprint->GetName()));
	Args.Add(TEXT("NumWarnings"), Results.NumWarnings);
	Args.Add(TEXT("NumErrors"), Results.NumErrors);
	Args.Add(TEXT("TotalMilliseconds"), (int)((FinishTime - CompileStartTime) * 1000));

	if (Results.NumErrors > 0)
	{
		FString FailMsg = FText::Format(LOCTEXT("CompileFailed", "[{Time}] Compile of {BlueprintName} failed. {NumErrors} Fatal Issue(s) {NumWarnings} Warning(s) [in {TotalMilliseconds} ms]"), Args).ToString();
		Results.Warning(*FailMsg);
	}
	else if(Results.NumWarnings > 0)
	{
		FString WarningMsg = FText::Format(LOCTEXT("CompileWarning", "[{Time}] Compile of {BlueprintName} successful, but with {NumWarnings} Warning(s) [in {TotalMilliseconds} ms]"), Args).ToString();
		Results.Warning(*WarningMsg);
	}
	else if(bPrintResultSuccess)
	{
		FString SuccessMsg = FText::Format(LOCTEXT("CompileSuccess", "[{Time}] Compile of {BlueprintName} successful! [in {TotalMilliseconds} ms]"), Args).ToString();
		Results.Note(*SuccessMsg);
	}

	Blueprint->CurrentMessageLog = NULL;
}

// Compiles a blueprint.
void FKismet2CompilerModule::CompileBlueprint(class UBlueprint* Blueprint, const FKismetCompilerOptions& CompileOptions, FCompilerResultsLog& Results, FBlueprintCompileReinstancer* ParentReinstancer)
{
	SCOPE_SECONDS_COUNTER(GBlueprintCompileTime);

	const bool bIsBrandNewBP = (Blueprint->SkeletonGeneratedClass == NULL) && (Blueprint->GeneratedClass == NULL) && (Blueprint->ParentClass != NULL) && !CompileOptions.bIsDuplicationInstigated;

	if ((CompileOptions.CompileType == EKismetCompileType::StructuresOnly )
		|| (CompileOptions.CompileType == EKismetCompileType::Full))
	{
		FUserDefinedStructureCompilerUtils::CompileStructs(Blueprint, Results, false);
	}

	if((CompileOptions.CompileType != EKismetCompileType::BytecodeOnly) && (CompileOptions.CompileType != EKismetCompileType::StructuresOnly))
	{
		FBlueprintCompileReinstancer SkeletonReinstancer(Blueprint->SkeletonGeneratedClass);

		FCompilerResultsLog SkeletonResults;
		SkeletonResults.bSilentMode = true;
		FKismetCompilerOptions SkeletonCompileOptions;
		SkeletonCompileOptions.CompileType = EKismetCompileType::SkeletonOnly;
		CompileBlueprintInner(Blueprint, /*bPrintResultSuccess=*/ false, SkeletonCompileOptions, SkeletonResults);
	}

	// If this was a full compile, take appropriate actions depending on the success of failure of the compile
	if( CompileOptions.IsGeneratedClassCompileType() )
	{
 		// Perform the full compile
		CompileBlueprintInner(Blueprint, /*bPrintResultSuccess=*/ true, CompileOptions, Results);

		if (Results.NumErrors == 0)
		{
			// Blueprint is error free.  Go ahead and fix up debug info
			Blueprint->Status = (0 == Results.NumWarnings) ? BS_UpToDate : BS_UpToDateWithWarnings;
			
			Blueprint->BlueprintSystemVersion = UBlueprint::GetCurrentBlueprintSystemVersion();

			// Reapply breakpoints to the bytecode of the new class
			for (int32 Index = 0; Index < Blueprint->Breakpoints.Num(); ++Index)
			{
				UBreakpoint* Breakpoint = Blueprint->Breakpoints[Index];
				FKismetDebugUtilities::ReapplyBreakpoint(Breakpoint);
			}
		}
		else
		{
			// Should never get errors from a brand new blueprint!
			ensure(!bIsBrandNewBP || (Results.NumErrors == 0));

			// There were errors.  Compile the generated class to have function stubs
			Blueprint->Status = BS_Error;

			// Reinstance objects here, so we can preserve their memory layouts to reinstance them again
			if( ParentReinstancer != NULL )
			{
				ParentReinstancer->UpdateBytecodeReferences();

				if(!Blueprint->bIsRegeneratingOnLoad)
				{
					ParentReinstancer->ReinstanceObjects();
				}
			}

			FBlueprintCompileReinstancer StubReinstancer(Blueprint->GeneratedClass);

			// Toss the half-baked class and generate a stubbed out skeleton class that can be used
			FCompilerResultsLog StubResults;
			StubResults.bSilentMode = true;
			FKismetCompilerOptions StubCompileOptions(CompileOptions);
			StubCompileOptions.CompileType = EKismetCompileType::StubAfterFailure;

			CompileBlueprintInner(Blueprint, /*bPrintResultSuccess=*/ false, StubCompileOptions, StubResults);

			StubReinstancer.UpdateBytecodeReferences();
			if( !Blueprint->bIsRegeneratingOnLoad )
			{
				StubReinstancer.ReinstanceObjects();
			}
		}
	}

	UPackage* Package = Blueprint->GetOutermost();
	if( Package )
	{
		UMetaData* MetaData =  Package->GetMetaData();
		MetaData->RemoveMetaDataOutsidePackage();
	}
}

// Recover a corrupted blueprint
void FKismet2CompilerModule::RecoverCorruptedBlueprint(class UBlueprint* Blueprint)
{
	UPackage* Package = Blueprint->GetOutermost();

	// Get rid of any stale classes
	for (FObjectIterator ObjIt; ObjIt; ++ObjIt)
	{
		UObject* TestObject = *ObjIt;
		if (TestObject->GetOuter() == Package)
		{
			// This object is in the blueprint package; is it expected?
			if (UClass* TestClass = Cast<UClass>(TestObject))
			{
				if ((TestClass != Blueprint->SkeletonGeneratedClass) && (TestClass != Blueprint->GeneratedClass))
				{
					// Unexpected UClass
					FKismetCompilerUtilities::ConsignToOblivion(TestClass, false);
				}
			}
		}
	}

	CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );
}

void FKismet2CompilerModule::RemoveBlueprintGeneratedClasses(class UBlueprint* Blueprint)
{
	if (Blueprint != NULL)
	{
		if (Blueprint->GeneratedClass != NULL)
		{
			FKismetCompilerUtilities::ConsignToOblivion(Blueprint->GeneratedClass, Blueprint->bIsRegeneratingOnLoad);
			Blueprint->GeneratedClass = NULL;
		}

		if (Blueprint->SkeletonGeneratedClass != NULL)
		{
			FKismetCompilerUtilities::ConsignToOblivion(Blueprint->SkeletonGeneratedClass, Blueprint->bIsRegeneratingOnLoad);
			Blueprint->SkeletonGeneratedClass = NULL;
		}
	}
}

#undef LOCTEXT_NAMESPACE

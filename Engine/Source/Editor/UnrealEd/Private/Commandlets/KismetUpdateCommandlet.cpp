// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// This commandlet refreshes and recompiles Kismet blueprints.

#include "UnrealEd.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintGraphDefinitions.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintUpdateCommandlet, Log, All);

//////////////////////////////////////////////////////////////////////////
// UKismetUpdateCommandlet

UKismetUpdateCommandlet::UKismetUpdateCommandlet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

int32 UKismetUpdateCommandlet::InitializeResaveParameters(const TArray<FString>& Tokens, const TArray<FString>& Switches, TArray<FString>& MapPathNames)
{
	// Do the inherited setup
	int32 Result = Super::InitializeResaveParameters(Tokens, Switches, MapPathNames);

	// Limit resaving to packages that contain a blueprint
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* TestClass = *ClassIt;
		if (TestClass->IsChildOf(UBlueprint::StaticClass()))
		{
			ResaveClasses.AddUnique(TestClass->GetFName());
		}
	}

	// Filter out a lot of un-needed debugging information
	Verbosity = INFORMATIVE;

	//@TODO: Should not be required!
	AddToRoot();

	return Result;
}

FString UKismetUpdateCommandlet::GetChangelistDescription() const
{
	return TEXT("Update Blueprints");
}

void UKismetUpdateCommandlet::PerformAdditionalOperations(class UObject* Object, bool& bSavePackage)
{
	if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
	{
		// Refresh and compile the blueprint
		CompileOneBlueprint(Blueprint);
		bSavePackage = true;

		// Log this blueprint as already compiled, so others that reference it don't try to recompile it
		FString PathName = Blueprint->GetPathName();
		AlreadyCompiledFullPaths.Add(PathName);
	}
}

void UKismetUpdateCommandlet::CompileOneBlueprint(UBlueprint* Blueprint)
{
	FString PathName = Blueprint->GetPathName();

	if (!AlreadyCompiledFullPaths.Contains(PathName))
	{
		// Make sure any referenced interfaces are already recompiled
		for (int32 i = 0; i < Blueprint->ImplementedInterfaces.Num(); ++i)
		{
			const FBPInterfaceDescription& InterfaceDesc = Blueprint->ImplementedInterfaces[i];
			if (UBlueprint* InterfaceBP = Cast<UBlueprint>(InterfaceDesc.Interface->ClassGeneratedBy))
			{
				CompileOneBlueprint(InterfaceBP);
			}
		}

		// Make sure any referenced macros are already recompiled
		{
			// Find all macro instances
			TArray<UK2Node_MacroInstance*> MacroNodes;
			FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_MacroInstance>(Blueprint, /*out*/ MacroNodes);

			// Find all unique macro blueprints that contributed to the used macro instances
			TSet<UBlueprint*> UniqueMacroBlueprints;
			for (int32 i = 0; i < MacroNodes.Num(); ++i)
			{
				UBlueprint* MacroBP = FBlueprintEditorUtils::FindBlueprintForGraphChecked(MacroNodes[i]->GetMacroGraph());
				UniqueMacroBlueprints.Add(MacroBP);
			}

			// Compile each of the unique macro libraries
			for (TSet<UBlueprint*>::TIterator It(UniqueMacroBlueprints); It; ++It)
			{
				UBlueprint* MacroBP = *It;
				CompileOneBlueprint(MacroBP);
			}
		}

		// Refresh and compile this blueprint
		FBlueprintEditorUtils::RefreshAllNodes(Blueprint);

		const bool bIsRegeneratingOnLoad = true;
		FKismetEditorUtilities::CompileBlueprint(Blueprint, bIsRegeneratingOnLoad);

		// Notify the user of errors
		if (Blueprint->Status == BS_Error)
		{
			UE_LOG(LogBlueprintUpdateCommandlet, Warning, TEXT("[REPORT] Error: Failed to compile blueprint '%s'"), *Blueprint->GetName());
		}
	}
}

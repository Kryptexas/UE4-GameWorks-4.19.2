// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "BlueprintUtilities.h"
#include "AnimGraphDefinitions.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/KismetReinstanceUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetDebugUtilities.h"
#include "Toolkits/AssetEditorManager.h"
#include "Editor/Kismet/Public/BlueprintEditorModule.h"
#include "Editor/Kismet/Public/FindInBlueprints.h"
#include "Editor/Kismet/Public/FindInBlueprintUtils.h"
#include "Toolkits/ToolkitManager.h"
#include "Editor/KismetCompiler/Public/KismetCompilerModule.h"
#include "Kismet2/CompilerResultsLog.h"
#include "AssetSelection.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "AssetRegistryModule.h"
#include "Kismet2NameValidators.h"
#include "Layers/Layers.h"
#include "ScopedTransaction.h"
#include "AssetToolsModule.h"
#include "EngineAnalytics.h"
#include "IAnalyticsProvider.h"
#include "MessageLog.h"
#include "StructureEditorUtils.h"
#include "ActorEditorUtils.h"
#include "ObjectEditorUtils.h"
#include "DlgPickAssetPath.h"
#include "ComponentAssetBroker.h"

#define LOCTEXT_NAMESPACE "UnrealEd.Editor"
//////////////////////////////////////////////////////////////////////////
// FArchiveInvalidateTransientRefs

/**
 * Archive built to go through and find any references to objects in the transient package, and then NULL those references
 */
class FArchiveInvalidateTransientRefs : public FArchiveUObject
{
public:
	FArchiveInvalidateTransientRefs()
	{
		ArIsObjectReferenceCollector = true;
		ArIsPersistent = false;
		ArIgnoreArchetypeRef = false;
	}
protected:
	/** 
	 * UObject serialize operator implementation
	 *
	 * @param Object	reference to Object reference
	 * @return reference to instance of this class
	 */
	FArchive& operator<<( UObject*& Object )
	{
		// Check if this is a reference to an object existing in the transient package, and if so, NULL it.
		if ((Object != NULL) && (Object->GetOutermost() == GetTransientPackage()) )
		{
			check( Object->IsValidLowLevel() );
			Object = NULL;
		}

		return *this;
	}
};


//////////////////////////////////////////////////////////////////////////
// FBlueprintObjectsBeingDebuggedIterator

FBlueprintObjectsBeingDebuggedIterator::FBlueprintObjectsBeingDebuggedIterator(UBlueprint* InBlueprint)
	: Blueprint(InBlueprint)
{
}

UObject* FBlueprintObjectsBeingDebuggedIterator::operator* () const
{
	return Blueprint->GetObjectBeingDebugged();
}

UObject* FBlueprintObjectsBeingDebuggedIterator::operator-> () const
{
	return Blueprint->GetObjectBeingDebugged();
}

FBlueprintObjectsBeingDebuggedIterator& FBlueprintObjectsBeingDebuggedIterator::operator++()
{
	Blueprint = NULL;
	return *this;
}

bool FBlueprintObjectsBeingDebuggedIterator::IsValid() const
{
	return Blueprint != NULL;
}



//////////////////////////////////////////////////////////////////////////
// FObjectsBeingDebuggedIterator

FObjectsBeingDebuggedIterator::FObjectsBeingDebuggedIterator()
	: SelectedActorsIter(*GEditor->GetSelectedActors())
	, LevelScriptActorIndex(INDEX_NONE)
{
	FindNextLevelScriptActor();
}

UWorld* FObjectsBeingDebuggedIterator::GetWorld() const
{
	return (GEditor->PlayWorld != NULL) ? GEditor->PlayWorld : GWorld;
}

UObject* FObjectsBeingDebuggedIterator::operator* () const
{
	return SelectedActorsIter ? *SelectedActorsIter : (UObject*)(GetWorld()->GetLevel(LevelScriptActorIndex)->GetLevelScriptActor());
}

UObject* FObjectsBeingDebuggedIterator::operator-> () const
{
	return SelectedActorsIter ? *SelectedActorsIter : (UObject*)(GetWorld()->GetLevel(LevelScriptActorIndex)->GetLevelScriptActor());
}

FObjectsBeingDebuggedIterator& FObjectsBeingDebuggedIterator::operator++()
{
	if (SelectedActorsIter)
	{
		++SelectedActorsIter;
	}
	else
	{
		FindNextLevelScriptActor();
	}

	return *this;
}

bool FObjectsBeingDebuggedIterator::IsValid() const
{
	return SelectedActorsIter || (LevelScriptActorIndex < GetWorld()->GetNumLevels());
}

void FObjectsBeingDebuggedIterator::FindNextLevelScriptActor()
{
	while (++LevelScriptActorIndex < GetWorld()->GetNumLevels())
	{
		ULevel* Level = GetWorld()->GetLevel(LevelScriptActorIndex);
		if ((Level != NULL) && (Level->GetLevelScriptActor() != NULL))
		{
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

// Static variable definition
TArray<FString> FKismetEditorUtilities::TrackedBlueprintParentList;

/** Create the correct event graphs for this blueprint */
void FKismetEditorUtilities::CreateDefaultEventGraphs(UBlueprint* Blueprint)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraph* Ubergraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, K2Schema->GN_EventGraph, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	Ubergraph->bAllowDeletion = false; //@TODO: Really, just want to make sure we never drop below 1, not that you cannot delete any particular one!
	FBlueprintEditorUtils::AddUbergraphPage(Blueprint, Ubergraph);

	Blueprint->LastEditedDocuments.AddUnique(Ubergraph);
}

/** Create a new Blueprint and initialize it to a valid state. */
UBlueprint* FKismetEditorUtilities::CreateBlueprint(UClass* ParentClass, UObject* Outer, const FName NewBPName, EBlueprintType BlueprintType, TSubclassOf<UBlueprint> BlueprintClassType, FName CallingContext)
{
	check(FindObject<UBlueprint>(Outer, *NewBPName.ToString()) == NULL); 

	// Not all types are legal for all parent classes, if the parent class is const then the blueprint cannot be an ubergraph-bearing one
	if ((BlueprintType == BPTYPE_Normal) && (ParentClass->HasAnyClassFlags(CLASS_Const)))
	{
		BlueprintType = BPTYPE_Const;
	}
	
	// Create new UBlueprint object
	UBlueprint* NewBP = ConstructObject<UBlueprint>(*BlueprintClassType, Outer, NewBPName, RF_Public|RF_Standalone|RF_Transactional);
	NewBP->Status = BS_BeingCreated;
	NewBP->BlueprintType = BlueprintType;
	NewBP->ParentClass = ParentClass;
	NewBP->BlueprintSystemVersion = UBlueprint::GetCurrentBlueprintSystemVersion();
	NewBP->bIsNewlyCreated = true;

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Create SimpleConstructionScript and UserConstructionScript
	if (FBlueprintEditorUtils::SupportsConstructionScript(NewBP))
	{ 
		// >>> Temporary workaround, before a BlueprintGeneratedClass is the main asset.
		FName NewSkelClassName, NewGenClassName;
		NewBP->GetBlueprintClassNames(NewGenClassName, NewSkelClassName);
		UBlueprintGeneratedClass* NewClass = ConstructObject<UBlueprintGeneratedClass>(
			UBlueprintGeneratedClass::StaticClass(), NewBP->GetOutermost(), NewGenClassName, RF_Public|RF_Transactional);
		NewBP->GeneratedClass = NewClass;
		NewClass->ClassGeneratedBy = NewBP;
		NewClass->SetSuperStruct(ParentClass);
		// <<< Temporary workaround

		NewBP->SimpleConstructionScript = NewObject<USimpleConstructionScript>(NewClass);
		NewBP->SimpleConstructionScript->SetFlags(RF_Transactional);
		NewBP->LastEditedDocuments.Add(NewBP->SimpleConstructionScript);

		UEdGraph* UCSGraph = FBlueprintEditorUtils::CreateNewGraph(NewBP, K2Schema->FN_UserConstructionScript, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
		FBlueprintEditorUtils::AddFunctionGraph(NewBP, UCSGraph, /*bIsUserCreated=*/ false, AActor::StaticClass());

		// If the blueprint is derived from another blueprint, add in a super-call automatically
		if( NewBP->ParentClass && NewBP->ParentClass->ClassGeneratedBy )
		{
			check( UCSGraph->Nodes.Num() > 0 );
			UK2Node_FunctionEntry* UCSEntry = CastChecked<UK2Node_FunctionEntry>(UCSGraph->Nodes[0]);
			UK2Node_CallParentFunction* ParentCallNodeTemplate = NewObject<UK2Node_CallParentFunction>();
			ParentCallNodeTemplate->FunctionReference.SetExternalMember(K2Schema->FN_UserConstructionScript, NewBP->ParentClass);
			UK2Node_CallParentFunction* ParentCallNode = FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node_CallParentFunction>(UCSGraph, ParentCallNodeTemplate, FVector2D(200, 0));

			// Wire up the new node
			UEdGraphPin* ExecPin = UCSEntry->FindPin(K2Schema->PN_Then);
			UEdGraphPin* SuperPin = ParentCallNode->FindPin(K2Schema->PN_Execute);
			ExecPin->MakeLinkTo(SuperPin);
		}

		NewBP->LastEditedDocuments.Add(UCSGraph);
		UCSGraph->bAllowDeletion = false;
	}

	// Create default event graph(s)
	if (FBlueprintEditorUtils::DoesSupportEventGraphs(NewBP))
	{
		check(NewBP->UbergraphPages.Num() == 0);
		CreateDefaultEventGraphs(NewBP);
	}

	// if this is an anim blueprint, add the root animation graph
	//@TODO: ANIMREFACTOR: This kind of code should be on a per-blueprint basis; not centralized here
	if (UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(NewBP))
	{
		if (UAnimBlueprint::FindRootAnimBlueprint(AnimBP) == NULL)
		{
			// Only allow an anim graph if there isn't one in a parent blueprint
			UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(AnimBP, K2Schema->GN_AnimGraph, UAnimationGraph::StaticClass(), UAnimationGraphSchema::StaticClass());
			FBlueprintEditorUtils::AddDomainSpecificGraph(NewBP, NewGraph);
			NewBP->LastEditedDocuments.Add(NewGraph);
			NewGraph->bAllowDeletion = false;
		}
	}

	// Create initial UClass
	IKismetCompilerInterface& Compiler = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(KISMET_COMPILER_MODULENAME);
	
	FCompilerResultsLog Results;
	const bool bReplaceExistingInstances = false;
	NewBP->Status = BS_Dirty;
	FKismetCompilerOptions CompileOptions;
	Compiler.CompileBlueprint(NewBP, CompileOptions, Results);

	// Report blueprint creation to analytics
	if (FEngineAnalytics::IsAvailable())
	{
		TArray<FAnalyticsEventAttribute> Attribs;

		// translate the CallingContext into a string for analytics
		if (CallingContext != NAME_None)
		{
			Attribs.Add(FAnalyticsEventAttribute(FString("Context"), CallingContext.ToString()));
		}
		
		Attribs.Add(FAnalyticsEventAttribute(FString("ParentType"), ParentClass->ClassGeneratedBy == NULL ? FString("Native") : FString("Blueprint")));

		if(IsTrackedBlueprintParent(ParentClass))
		{
			Attribs.Add(FAnalyticsEventAttribute(FString("ParentClass"), ParentClass->GetName()));
		}

		const UGeneralProjectSettings& ProjectSettings = *GetDefault<UGeneralProjectSettings>();
		Attribs.Add(FAnalyticsEventAttribute(FString("ProjectId"), ProjectSettings.ProjectID.ToString()));

		FEngineAnalytics::GetProvider().RecordEvent(FString("Editor.Usage.BlueprintCreated"), Attribs);
	}

	return NewBP;
}

void FKismetEditorUtilities::CompileBlueprint(UBlueprint* BlueprintObj, bool bIsRegeneratingOnLoad, bool bSkipGarbageCollection, bool bSaveIntermediateProducts, FCompilerResultsLog* pResults)
{
	// The old class is either the GeneratedClass if we had an old successful compile, or the SkeletonGeneratedClass stub if there were previously fatal errors
	UClass* OldClass = (BlueprintObj->GeneratedClass != NULL && (BlueprintObj->GeneratedClass != BlueprintObj->SkeletonGeneratedClass)) ? BlueprintObj->GeneratedClass : NULL;

	// Load the compiler
	IKismetCompilerInterface& Compiler = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(KISMET_COMPILER_MODULENAME);

	// Prepare old objects for reinstancing
	TGuardValue<bool> GuardTemplateNameFlag(GCompilingBlueprint, true);

	// Compile
	FCompilerResultsLog LocalResults;
	FCompilerResultsLog& Results = (pResults != NULL) ? *pResults : LocalResults;

	// STRUCTURES RECOMPILE
	if (FStructureEditorUtils::StructureEditingEnabled())
	{
		FKismetCompilerOptions StructCompileOptions;
		StructCompileOptions.CompileType = EKismetCompileType::StructuresOnly;
		Compiler.CompileBlueprint(BlueprintObj, StructCompileOptions, Results, NULL);
	}

	FBlueprintCompileReinstancer ReinstanceHelper(OldClass);

	// Suppress errors/warnings in the log if we're recompiling on load on a build machine
	Results.bLogInfoOnly = BlueprintObj->bIsRegeneratingOnLoad && GIsBuildMachine;

	FKismetCompilerOptions CompileOptions;
	CompileOptions.bSaveIntermediateProducts = bSaveIntermediateProducts;
	Compiler.CompileBlueprint(BlueprintObj, CompileOptions, Results, &ReinstanceHelper);

	FBlueprintEditorUtils::UpdateDelegatesInBlueprint(BlueprintObj);

	if (FBlueprintEditorUtils::IsLevelScriptBlueprint(BlueprintObj))
	{
		// When the Blueprint is recompiled, then update the bound events for level scripting
		ULevelScriptBlueprint* LevelScriptBP = CastChecked<ULevelScriptBlueprint>(BlueprintObj);

		if (ULevel* BPLevel = LevelScriptBP->GetLevel())
		{
			BPLevel->OnLevelScriptBlueprintChanged(LevelScriptBP);
		}
	}

	ReinstanceHelper.UpdateBytecodeReferences();

	if (!bIsRegeneratingOnLoad && (OldClass != NULL))
	{
		// Strip off any external components from the CDO, if needed because of reparenting, etc
		FKismetEditorUtilities::StripExternalComponents(BlueprintObj);

		// Ensure that external SCS node references match up with the generated class
		if(BlueprintObj->SimpleConstructionScript)
		{
			BlueprintObj->SimpleConstructionScript->FixupRootNodeParentReferences();
		}

		// Replace instances of this class
		ReinstanceHelper.ReinstanceObjects();

 		if (!bSkipGarbageCollection)
 		{
 			// Garbage collect to make sure the old class and actors are disposed of
 			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
 		}

		// If you need to verify that all old instances are taken care of, uncomment this!
		// ReinstanceHelper.VerifyReplacement();
	}

	// Blueprint has changed, broadcast notify
	BlueprintObj->BroadcastChanged();

	if(GEditor)
	{
		GEditor->BroadcastBlueprintCompiled();	
	}

	// Default Values are now set in CDO. And these copies could be soon obsolete, so better to reset them.
	for(int VarIndex = 0; VarIndex < BlueprintObj->NewVariables.Num(); ++VarIndex)
	{
		BlueprintObj->NewVariables[VarIndex].DefaultValue.Empty();
	}

	// for interface changes, auto-refresh nodes on any dependent blueprints
	if (FBlueprintEditorUtils::IsInterfaceBlueprint(BlueprintObj))
	{
		TArray<UBlueprint*> DependentBPs;
		FBlueprintEditorUtils::GetDependentBlueprints(BlueprintObj, DependentBPs);

		// refresh each dependent blueprint
		for ( auto ObjIt = DependentBPs.CreateConstIterator(); ObjIt; ++ObjIt )
		{
			FBlueprintEditorUtils::RefreshAllNodes(*ObjIt);
		}
	}

	if(!bIsRegeneratingOnLoad && BlueprintObj->GeneratedClass)
	{
		UBlueprint::ValidateGeneratedClass(BlueprintObj->GeneratedClass);
	}
}

/** Generates a blueprint skeleton only.  Minimal compile, no notifications will be sent, no GC, etc.  Only successful if there isn't already a skeleton generated */
void FKismetEditorUtilities::GenerateBlueprintSkeleton(UBlueprint* BlueprintObj, bool bForceRegeneration)
{
	check(BlueprintObj);

	if( BlueprintObj->SkeletonGeneratedClass == NULL || bForceRegeneration )
	{
		UPackage* Package = Cast<UPackage>(BlueprintObj->GetOutermost());
		bool bIsPackageDirty = Package ? Package->IsDirty() : false;
					
		IKismetCompilerInterface& Compiler = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(KISMET_COMPILER_MODULENAME);

		TGuardValue<bool> GuardTemplateNameFlag(GCompilingBlueprint, true);
		FCompilerResultsLog Results;

		FKismetCompilerOptions CompileOptions;
		CompileOptions.CompileType = EKismetCompileType::SkeletonOnly;
		Compiler.CompileBlueprint(BlueprintObj, CompileOptions, Results);

		// Restore the package dirty flag here
		if( Package != NULL )
		{
			Package->SetDirtyFlag(bIsPackageDirty);
		}
	}
}

/** Recompiles the bytecode of a blueprint only.  Should only be run for recompiling dependencies during compile on load */
void FKismetEditorUtilities::RecompileBlueprintBytecode(UBlueprint* BlueprintObj)
{
	check(BlueprintObj);
	check(BlueprintObj->GeneratedClass);

	IKismetCompilerInterface& Compiler = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(KISMET_COMPILER_MODULENAME);

	TGuardValue<bool> GuardTemplateNameFlag(GCompilingBlueprint, true);
	FCompilerResultsLog Results;

	FBlueprintCompileReinstancer ReinstanceHelper(BlueprintObj->GeneratedClass, true);

	FKismetCompilerOptions CompileOptions;
	CompileOptions.CompileType = EKismetCompileType::BytecodeOnly;
	Compiler.CompileBlueprint(BlueprintObj, CompileOptions, Results);

	ReinstanceHelper.UpdateBytecodeReferences();
}

/** Tries to make sure that a blueprint is conformed to its native parent, in case any native class flags have changed */
void FKismetEditorUtilities::ConformBlueprintFlagsAndComponents(UBlueprint* BlueprintObj)
{
	// Propagate native class flags to the children class.  This fixes up cases where native instanced components get added after BP creation, etc
	const UClass* ParentClass = BlueprintObj->ParentClass;

	if( UClass* SkelClass = BlueprintObj->SkeletonGeneratedClass )
	{
		SkelClass->ClassFlags |= (ParentClass->ClassFlags & CLASS_ScriptInherit);
		UObject* SkelCDO = SkelClass->GetDefaultObject();
		SkelCDO->InstanceSubobjectTemplates();
	}

	if( UClass* GenClass = BlueprintObj->GeneratedClass )
	{
		GenClass->ClassFlags |= (ParentClass->ClassFlags & CLASS_ScriptInherit);
		UObject* GenCDO = GenClass->GetDefaultObject();
		GenCDO->InstanceSubobjectTemplates();
	}
}

/** @return		true is it's possible to create a blueprint from the specified class */
bool FKismetEditorUtilities::CanCreateBlueprintOfClass(const UClass* Class)
{
	bool bAllowDerivedBlueprints = false;
	GConfig->GetBool(TEXT("Kismet"), TEXT("AllowDerivedBlueprints"), /*out*/ bAllowDerivedBlueprints, GEngineIni);

	const bool bCanCreateBlueprint =
		!Class->HasAnyClassFlags(CLASS_Deprecated)
		&& !Class->HasAnyClassFlags(CLASS_NewerVersionExists)
		&& (!Class->ClassGeneratedBy || bAllowDerivedBlueprints);

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	const bool bIsValidClass = Class->GetBoolMetaDataHierarchical(FBlueprintMetadata::MD_IsBlueprintBase) || (Class == UObject::StaticClass());

	return bCanCreateBlueprint && bIsValidClass;
}

UBlueprint* FKismetEditorUtilities::CreateBlueprintFromActor(const FString& Path, UObject* Object, bool bReplaceActor )
{
	UBlueprint* NewBlueprint = NULL;

	if (AActor* Actor = Cast<AActor>(Object))
	{
		// Create a blueprint
		FString PackageName = Path;
		FString AssetName = FPackageName::GetLongPackageAssetName(Path);


		// If no AssetName was found, generate a unique asset name.
		if(AssetName.Len() == 0)
		{
			PackageName = FPackageName::GetLongPackagePath(Path);
			FString BasePath = PackageName + TEXT("/") + LOCTEXT("BlueprintName_Default", "NewBlueprint").ToString();
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
			AssetToolsModule.Get().CreateUniqueAssetName(BasePath, TEXT(""), PackageName, AssetName);
		}

		UPackage* Package = CreatePackage(NULL, *PackageName);

		if(Package)
		{
			UActorFactory* FactoryToUse = GEditor->FindActorFactoryForActorClass( Actor->GetClass() );
			const FName BlueprintName = FName(*AssetName);

			if( FactoryToUse != NULL )
			{
				// Create the blueprint
				UObject* Asset = FactoryToUse->GetAssetFromActorInstance(Actor);
				if (Asset)
				{
					NewBlueprint = FactoryToUse->CreateBlueprint( Asset, Package, BlueprintName, FName("CreateFromActor") );
				}
			}
			else 
			{
				// We don't have a factory, but we can still try to create a blueprint for this actor class
				NewBlueprint = FKismetEditorUtilities::CreateBlueprint( Object->GetClass(), Package, BlueprintName, EBlueprintType::BPTYPE_Normal, UBlueprint::StaticClass(), FName("CreateFromActor") );
			}
		}

		if(NewBlueprint)
		{
			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(NewBlueprint);

			// Mark the package dirty
			Package->MarkPackageDirty();

			if(NewBlueprint->GeneratedClass)
			{
				UObject* CDO = NewBlueprint->GeneratedClass->GetDefaultObject();
				UEditorEngine::CopyPropertiesForUnrelatedObjects(Object, CDO);
				if(AActor* CDOAsActor = Cast<AActor>(CDO))
				{
					if(USceneComponent* Scene = CDOAsActor->GetRootComponent())
					{
						Scene->SetRelativeLocation(FVector::ZeroVector);

						// Clear out the attachment info after having copied the properties from the source actor
						Scene->AttachParent = NULL;
						Scene->AttachChildren.Empty();

						// Ensure the light mass information is cleaned up
						Scene->InvalidateLightingCache();
					}
				}
			}

			if(bReplaceActor)
			{
				TArray<AActor*> Actors;
				Actors.Add(Actor);

				FVector Location = Actor->GetActorLocation();
				FRotator Rotator = Actor->GetActorRotation();

				CreateBlueprintInstanceFromSelection(NewBlueprint, Actors, Location, Rotator);
			}
		}
	}

	if (NewBlueprint)
	{
		// Open the editor for the new blueprint
		FAssetEditorManager::Get().OpenEditorForAsset(NewBlueprint);
	}
	return NewBlueprint;
}

// This class cracks open the selected actors, harvests their components, and creates a new blueprint containing copies of them
class FCreateConstructionScriptFromSelectedActors
{
public:
	FCreateConstructionScriptFromSelectedActors()
		: Blueprint(NULL)
		, SCS(NULL)
	{
	}

	/** Util to get a unique name within the BP from the supplied Actor */
	FName GetUniqueNameFromActor(FKismetNameValidator& InValidator, const AActor* InActor)
	{
		FName NewName;
		NewName = FName(*InActor->GetActorLabel());

		//find valid name--
		for (int32 i = 1; EValidatorResult::Ok != InValidator.IsValid(NewName); ++i)
		{
			NewName = FName(*InActor->GetActorLabel(), i);
			check(i < 10000);
		}

		return NewName;
	}

	UBlueprint* Execute(FString Path, TArray<AActor*> SelectedActors, bool bReplaceInWorld)
	{
		if (SelectedActors.Num() > 0)
		{
			// See if we have a single actor with a single component selected
			// If so, we don't bother creating the root SceneComponent.
			USceneComponent* SingleSceneComp = NULL;
			if(SelectedActors.Num() == 1)
			{
				TArray<UActorComponent*> Components;
				SelectedActors[0]->GetComponents(Components);
				for(UActorComponent* Component : Components)
				{
					USceneComponent* SceneComp = Cast<USceneComponent>(Component);
					if( SceneComp != NULL && 
						!SceneComp->bHiddenInGame && 
						SceneComp->GetClass()->HasMetaData(FBlueprintMetadata::MD_BlueprintSpawnableComponent) )
					{
						// If we already found one, then there is more than one, so set to NULL and break and continue
						if(SingleSceneComp != NULL)
						{
							SingleSceneComp = NULL;
							break;
						}
						// This is the first valid scene component we have found, save it
						else
						{
							SingleSceneComp = SceneComp;
						}
					}
				}
			}

			// Determine the origin for all the actors, so it can be backed out when saving them in the blueprint
			FTransform NewActorTransform = FTransform::Identity;

			// Create a blueprint
			FString PackageName = Path;
			FString AssetName = FPackageName::GetLongPackageAssetName(Path);
			FString BasePath = PackageName + TEXT("/") + AssetName;

			// If no AssetName was found, generate a unique asset name.
			if(AssetName.Len() == 0)
			{
				BasePath = PackageName + TEXT("/") + LOCTEXT("BlueprintName_Default", "NewBlueprint").ToString();
				FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
				AssetToolsModule.Get().CreateUniqueAssetName(BasePath, TEXT(""), PackageName, AssetName);
			}

			UPackage* Package = CreatePackage(NULL, *PackageName);
			Blueprint = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), Package, *AssetName, BPTYPE_Normal, UBlueprint::StaticClass(), FName("HarvestFromActors"));

			check(Blueprint->SimpleConstructionScript != NULL);
			SCS = Blueprint->SimpleConstructionScript;

			FKismetNameValidator Validator(Blueprint);

			// If we have a single component selected, make a BP with a single component
			if(SingleSceneComp != NULL)
			{
				USCS_Node* Node = CreateUSCSNode(SingleSceneComp);
				Node->VariableName = GetUniqueNameFromActor(Validator, SingleSceneComp->GetOwner());
				SCS->AddNode(Node);

				NewActorTransform = SingleSceneComp->ComponentToWorld;
			}
			// Multiple actors/component selected, so we create a root scenecomponent and attach things to it
			else
			{
				// Find average location of all selected actors
				FVector AverageLocation = FVector::ZeroVector;
				for (auto It(SelectedActors.CreateConstIterator()); It; ++It)
				{
					if (const AActor* Actor = Cast<AActor>(*It))
					{
						if (USceneComponent* RootComponent = Actor->GetRootComponent())
						{
							AverageLocation += Actor->GetActorLocation();
						}
					}
				}
				AverageLocation /= (float)SelectedActors.Num();

				// Spawn the new BP at that location
				NewActorTransform.SetTranslation(AverageLocation);

				// Add a new scene component to serve as the root node
				USCS_Node* RootNode = SCS->CreateNode(USceneComponent::StaticClass());
				SCS->AddNode(RootNode);

				TMap<const AActor*, USCS_Node*> ActorToUSCSNodeMap;

				// Add each actor's root component to the mapping of Actors to USCS_Node map to be organized later
				for (auto ActorIt = SelectedActors.CreateConstIterator(); ActorIt; ++ActorIt)
				{
					const AActor* Actor = *ActorIt;
					USceneComponent* ActorRootComponent = Actor->GetRootComponent();
					// Check we have a component and it is valid to add to a BP
					if(ActorRootComponent != NULL && ActorRootComponent->GetClass()->HasMetaData(FBlueprintMetadata::MD_BlueprintSpawnableComponent))
					{
						USCS_Node* Node = CreateUSCSNode(ActorRootComponent);
						Node->VariableName = GetUniqueNameFromActor(Validator, Actor);

						ActorToUSCSNodeMap.Add(Actor, Node);
					}
				}

				// Attach all immediate children to their parent, or the root if their parent is not being added
				for (auto ActorIt = SelectedActors.CreateConstIterator(); ActorIt; ++ActorIt)
				{
					const AActor* Actor = *ActorIt;
					USceneComponent* ActorRootComponent = Actor->GetRootComponent();
					// Check we have a component and it is valid to add to a BP
					if(ActorRootComponent != NULL && ActorRootComponent->GetClass()->HasMetaData(FBlueprintMetadata::MD_BlueprintSpawnableComponent))
					{			
						USCS_Node** CurrentNode = ActorToUSCSNodeMap.Find(Actor);
						USCS_Node** ParentNode = ActorToUSCSNodeMap.Find(Actor->GetAttachParentActor());

						check(CurrentNode);

						FTransform ComponentToWorldSpace;
						if(ParentNode)
						{
							ComponentToWorldSpace = TransformChildComponent(Actor->GetRootComponent(), *CurrentNode, Actor->GetAttachParentActor()->GetRootComponent()->GetComponentToWorld());
							(*ParentNode)->AddChildNode(*CurrentNode);
						}
						else
						{
							ComponentToWorldSpace = TransformChildComponent(Actor->GetRootComponent(), *CurrentNode, NewActorTransform);
							RootNode->AddChildNode(*CurrentNode);
						}

						// Attach any children components as well, as long as their owner is this Actor
						AddActorComponents(Actor, Actor->GetRootComponent(), *CurrentNode, ComponentToWorldSpace);
					}
				}
			}

			// Regenerate skeleton class as components have been added since initial generation
			FKismetEditorUtilities::GenerateBlueprintSkeleton(Blueprint,true); 
			
			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(Blueprint);

			// Mark the package dirty
			Package->MarkPackageDirty();

			// Delete the old actors and create a new instance in the map
			if (bReplaceInWorld)
			{
				FVector Location = NewActorTransform.GetLocation();
				FRotator Rotator = NewActorTransform.Rotator();

				FKismetEditorUtilities::CreateBlueprintInstanceFromSelection(Blueprint, SelectedActors, Location, Rotator);
			}

			// Open the editor for the new blueprint
			FAssetEditorManager::Get().OpenEditorForAsset(Blueprint);

			return Blueprint;
		}
		return NULL;
	}

protected:

	void CopyComponentToTemplate(UActorComponent* Source, UActorComponent* Destination)
	{
		check(Source->GetClass() == Destination->GetClass());
		UClass* CommonBaseClass = Source->GetClass();
		UActorComponent* DefaultComponent = CastChecked<UActorComponent>(Source->GetArchetype());

		//@TODO: Copy of code from inside CopyActorComponentProperties

		// Iterate through the properties, only copying those which are non-native, non-transient, non-component, and not identical
		// to the values in the default component
		for (UProperty* Property = CommonBaseClass->PropertyLink; Property != NULL; Property = Property->PropertyLinkNext)
		{
			const bool bIsTransient = !!( Property->PropertyFlags & CPF_Transient );
			const bool bIsIdentical = Property->Identical_InContainer(Source, DefaultComponent);
			const bool bIsComponent = !!( Property->PropertyFlags & (CPF_InstancedReference | CPF_ContainsInstancedReference) );

			if (!bIsTransient && !bIsIdentical && !bIsComponent)
			{
				Property->CopyCompleteValue_InContainer(Destination, Source);
			}
		}
	}

	/** Creates a USCS Node for the passed in component */
	USCS_Node* CreateUSCSNode(USceneComponent* Component)
	{
		// Create a new SCS node for this component
		USCS_Node* NewNode = SCS->CreateNode(Component->GetClass());

		// Copy properties across to the new template
		CopyComponentToTemplate(Component, NewNode->ComponentTemplate);

		return NewNode;
	}

	/**
	 * Transforms a USCS_Node component based on the original Component and the passed in transform.
	 *
	 * @param InOriginalComponent		The original component the USCS_Node is based on
	 * @param InUSCSNode				The USCS_Node based on the original component
	 * @param InParentToWorldSpace		FTransform to place the USCS_Node in the correct spot.
	 *
	 * @return							The component to world transform of the USCS_Node component
	 */
	FTransform TransformChildComponent(USceneComponent* InOriginalComponent, USCS_Node* InUSCSNode, const FTransform& InParentToWorldSpace)
	{
		// Position this node relative to it's parent
		USceneComponent* NewTemplate = CastChecked<USceneComponent>(InUSCSNode->ComponentTemplate);

		FTransform ChildToWorld = InOriginalComponent->GetComponentToWorld();
		FTransform RelativeTransform = ChildToWorld.GetRelativeTransform(InParentToWorldSpace);

		NewTemplate->SetRelativeTransform(RelativeTransform);

		return ChildToWorld;
	}

	/**
	 * Adds all child components (but not child actors) to the RootNode as a USCS_Node
	 *
	 * @param InParentActor		The actor all components must be a child of
	 * @param InComponent		The component to pull child components from
	 * @param InRootNode		The root node to attach all new USCS_Nodes to
	 * @param InParentToWorldSpace		FTransform to place the USCS_Node in the correct spot.
	 */
	void AddActorComponents(const AActor* InParentActor, USceneComponent* InComponent, USCS_Node* InRootNode, const FTransform& InParentToWorldSpace)
	{
		if (InComponent)
		{
			for (auto ChildIt = InComponent->AttachChildren.CreateConstIterator(); ChildIt; ++ChildIt)
			{
				USceneComponent* ChildComponent = *ChildIt;

				if(ChildComponent->GetOwner() == InParentActor)
				{				
					// if component is 'editor only' we usually don't want to copy it
					// Also check its a valid class to add
					if(!ChildComponent->bHiddenInGame && ChildComponent->GetClass()->HasMetaData(FBlueprintMetadata::MD_BlueprintSpawnableComponent))
					{
						USCS_Node* NewNode = CreateUSCSNode(ChildComponent);
						FTransform ComponentToWorldSpace = TransformChildComponent(ChildComponent, NewNode, InParentToWorldSpace);
						InRootNode->AddChildNode(NewNode);

						// Add any child components that may be owned by this Actor
						AddActorComponents(InParentActor, ChildComponent, NewNode, ComponentToWorldSpace);
					}
				}

			}
		}
	}


protected:
	UBlueprint* Blueprint;
	USimpleConstructionScript* SCS;
};


UBlueprint* FKismetEditorUtilities::HarvestBlueprintFromActors(const FString& Path, const TArray<AActor*>& Actors, bool bReplaceInWorld)
{
	FCreateConstructionScriptFromSelectedActors Creator;
	return Creator.Execute(Path, Actors, bReplaceInWorld);
}

AActor* FKismetEditorUtilities::CreateBlueprintInstanceFromSelection(UBlueprint* Blueprint, TArray<AActor*>& SelectedActors, const FVector& Location, const FRotator& Rotator)
{
	check (SelectedActors.Num() > 0 );

	// Create transaction to cover conversion
	const FScopedTransaction Transaction( NSLOCTEXT("EditorEngine", "ConvertActorToBlueprint", "Replace Actor(s) with blueprint") );

	// Assume all selected actors are in the same world
	UWorld* World = SelectedActors[0]->GetWorld();

	for(auto It(SelectedActors.CreateIterator());It;++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			// Remove from active selection in editor
			GEditor->SelectActor(Actor, /*bSelected=*/ false, /*bNotify=*/ false);

			GEditor->Layers->DisassociateActorFromLayers(Actor);
			World->EditorDestroyActor(Actor, false);
		}
	}

	AActor* NewActor = World->SpawnActor(Blueprint->GeneratedClass, &Location, &Rotator);
	GEditor->Layers->InitializeNewActorLayers(NewActor);

	// Update selection to new actor
	GEditor->SelectActor( NewActor, /*bSelected=*/ true, /*bNotify=*/ true );

	return NewActor;
}

UBlueprint* FKismetEditorUtilities::CreateBlueprintFromClass(FText InWindowTitle, UClass* InParentClass, FString NewNameSuggestion)
{
	check(FKismetEditorUtilities::CanCreateBlueprintOfClass(InParentClass));

	// Pre-generate a unique asset name to fill out the path picker dialog with.
	if (NewNameSuggestion.Len() == 0)
	{
		NewNameSuggestion = TEXT("NewBlueprint");
	}

	FString PackageName = FString(TEXT("/Game/Blueprints/")) + NewNameSuggestion;
	FString Name;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);

	TSharedPtr<SDlgPickAssetPath> PickAssetPathWidget =
		SNew(SDlgPickAssetPath)
		.Title(InWindowTitle)
		.DefaultAssetPath(FText::FromString(PackageName));

	if (EAppReturnType::Ok == PickAssetPathWidget->ShowModal())
	{
		// Get the full name of where we want to create the physics asset.
		FString UserPackageName = PickAssetPathWidget->GetFullAssetPath().ToString();
		FName BPName(*FPackageName::GetLongPackageAssetName(UserPackageName));

		// Check if the user inputed a valid asset name, if they did not, give it the generated default name
		if (BPName == NAME_None)
		{
			// Use the defaults that were already generated.
			UserPackageName = PackageName;
			BPName = *Name;
		}

		// Then find/create it.
		UPackage* Package = CreatePackage(NULL, *UserPackageName);
		check(Package);

		// Create and init a new Blueprint
		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(InParentClass, Package, BPName, BPTYPE_Normal, UBlueprint::StaticClass(), FName("LevelEditorActions"));
		if (Blueprint)
		{
			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(Blueprint);

			// Mark the package dirty...
			Package->MarkPackageDirty();

			return Blueprint;
		}
	}
	return NULL;
}

UBlueprint* FKismetEditorUtilities::CreateBlueprintUsingAsset(UObject* Asset, bool bOpenInEditor)
{
	// Check we have an asset.
	if(Asset == NULL)
	{
		return NULL;
	}

	// Check we can create a component from this asset
	TSubclassOf<UActorComponent> ComponentClass = FComponentAssetBrokerage::GetPrimaryComponentForAsset(Asset->GetClass());
	if(ComponentClass != NULL)
	{
		// Create a new empty Actor BP
		UBlueprint* NewBP = CreateBlueprintFromClass(LOCTEXT("CreateBlueprint", "Create Blueprint"), AActor::StaticClass(), Asset->GetName());
		if(NewBP != NULL)
		{
			// Create a new SCS node
			check(NewBP->SimpleConstructionScript != NULL);
			USCS_Node* NewNode = NewBP->SimpleConstructionScript->CreateNode(ComponentClass);

			// Assign the asset to the template
			FComponentAssetBrokerage::AssignAssetToComponent(NewNode->ComponentTemplate, Asset);

			// Add node to the SCS
			NewBP->SimpleConstructionScript->AddNode(NewNode);

			// Recompile skeleton because of the new component we added
			FKismetEditorUtilities::GenerateBlueprintSkeleton(NewBP, true);

			// Open in BP editor if desired
			if(bOpenInEditor)
			{
				FAssetEditorManager::Get().OpenEditorForAsset(NewBP);
			}
		}

		return NewBP;
	}

	return NULL;
}


TSharedPtr<class IBlueprintEditor> FKismetEditorUtilities::GetIBlueprintEditorForObject( const UObject* ObjectToFocusOn, bool bOpenEditor )
{
	check(ObjectToFocusOn);

	// Find the associated blueprint
	UBlueprint* TargetBP = Cast<UBlueprint>(const_cast<UObject*>(ObjectToFocusOn));
	if (TargetBP == NULL)
	{
		for (UObject* TestOuter = ObjectToFocusOn->GetOuter(); TestOuter; TestOuter = TestOuter->GetOuter())
		{
			TargetBP = Cast<UBlueprint>(TestOuter);
			if (TargetBP != NULL)
			{
				break;
			}
		}
	}

	TSharedPtr<IBlueprintEditor> BlueprintEditor;
	if (TargetBP != NULL)
	{
		if (bOpenEditor)
		{
			// @todo toolkit major: Needs world-centric support
			FAssetEditorManager::Get().OpenEditorForAsset(TargetBP);
		}

		TSharedPtr< IToolkit > FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(TargetBP);
		// If we found a BlueprintEditor
		if (FoundAssetEditor.IsValid() && FoundAssetEditor->IsBlueprintEditor())
		{
			BlueprintEditor = StaticCastSharedPtr<IBlueprintEditor>(FoundAssetEditor);
		}
	}
	return BlueprintEditor;
}

void FKismetEditorUtilities::PasteNodesHere( class UEdGraph* Graph, const FVector2D& Location )
{
	TSharedPtr<class IBlueprintEditor> Kismet = GetIBlueprintEditorForObject(Graph,false);
	if(Kismet.IsValid())
	{
		Kismet->PasteNodesHere(Graph,Location);
	}
}

bool FKismetEditorUtilities::CanPasteNodes( const class UEdGraph* Graph )
{
	bool bCanPaste = false;
	TSharedPtr<class IBlueprintEditor> Kismet = GetIBlueprintEditorForObject(Graph,false);
	if (Kismet.IsValid())
	{
		bCanPaste = Kismet->CanPasteNodes();
	}
	return bCanPaste;
}

bool FKismetEditorUtilities::GetBoundsForSelectedNodes(const class UBlueprint* Blueprint,  class FSlateRect& Rect, float Padding)
{
	bool bCanPaste = false;
	TSharedPtr<class IBlueprintEditor> Kismet = GetIBlueprintEditorForObject(Blueprint, false);
	if (Kismet.IsValid())
	{
		bCanPaste = Kismet->GetBoundsForSelectedNodes(Rect, Padding);
	}
	return bCanPaste;
}

int32 FKismetEditorUtilities::GetNumberOfSelectedNodes(const class UBlueprint* Blueprint)
{
	int32 NumberNodesSelected = 0;
	TSharedPtr<class IBlueprintEditor> Kismet = GetIBlueprintEditorForObject(Blueprint, false);
	if (Kismet.IsValid())
	{
		NumberNodesSelected = Kismet->GetNumberOfSelectedNodes();
	}
	return NumberNodesSelected;
}

/** Open a Kismet window, focusing on the specified object (either a pin, a node, or a graph).  Prefers existing windows, but will open a new application if required. */
void FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(const UObject* ObjectToFocusOn, bool bRequestRename)
{
	TSharedPtr<IBlueprintEditor> BlueprintEditor = GetIBlueprintEditorForObject(ObjectToFocusOn, true);
	if (BlueprintEditor.IsValid())
	{
		BlueprintEditor->FocusWindow();
		BlueprintEditor->JumpToHyperlink(ObjectToFocusOn, bRequestRename);
	}
}

void FKismetEditorUtilities::ShowActorReferencesInLevelScript(const AActor* Actor)
{
	if (Actor != NULL)
	{
		ULevelScriptBlueprint* LSB = Actor->GetLevel()->GetLevelScriptBlueprint();
		if (LSB != NULL)
		{
			// @todo toolkit major: Needs world-centric support.  Other spots, too?
			FAssetEditorManager::Get().OpenEditorForAsset(LSB);
			TSharedPtr<IToolkit> FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(LSB);
			if (FoundAssetEditor.IsValid())
			{
				TSharedRef<IBlueprintEditor> BlueprintEditor = StaticCastSharedRef<IBlueprintEditor>(FoundAssetEditor.ToSharedRef());
				BlueprintEditor->FocusWindow();

				const bool bSetFindWithinBlueprint = true;
				const bool bSelectFirstResult = true;
				BlueprintEditor->SummonSearchUI(bSetFindWithinBlueprint, Actor->GetName(), bSelectFirstResult);
			}
		}

	}
}

// Upgrade any cosmetically stale information in a blueprint (done when edited instead of PostLoad to make certain operations easier)
void FKismetEditorUtilities::UpgradeCosmeticallyStaleBlueprint(UBlueprint* Blueprint)
{
	// Rename the ubergraph page 'StateGraph' to be named 'EventGraph' if possible
	if (FBlueprintEditorUtils::DoesSupportEventGraphs(Blueprint))
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		UEdGraph* OldStateGraph = FindObject<UEdGraph>(Blueprint, TEXT("StateGraph"));
		UObject* CollidingObject = FindObject<UObject>(Blueprint, *(K2Schema->GN_EventGraph.ToString()));

		if ((OldStateGraph != NULL) && (CollidingObject == NULL))
		{
			check(!OldStateGraph->HasAnyFlags(RF_Public));
			OldStateGraph->Rename(*(K2Schema->GN_EventGraph.ToString()), OldStateGraph->GetOuter(), REN_DoNotDirty | REN_ForceNoResetLoaders);
			Blueprint->Status = BS_Dirty;
		}
	}
}

void FKismetEditorUtilities::CreateNewBoundEventForActor(AActor* Actor, FName EventName)
{
	if(Actor != NULL && EventName != NAME_None)
	{
		// First, find the property we want to bind to
		UMulticastDelegateProperty* DelegateProperty = FindField<UMulticastDelegateProperty>(Actor->GetClass(), EventName);
		if(DelegateProperty != NULL)
		{
			// Get the correct level script blueprint
			ULevelScriptBlueprint* LSB = Actor->GetLevel()->GetLevelScriptBlueprint();
			UEdGraph* TargetGraph = NULL;
			if(LSB != NULL && LSB->UbergraphPages.Num() > 0)
			{
				TargetGraph = LSB->UbergraphPages[0]; // Just use the forst graph
			}

			if(TargetGraph != NULL)
			{
				// Figure out a decent place to stick the node
				const FVector2D NewNodePos = TargetGraph->GetGoodPlaceForNewNode();

				// Create a new event node
				UK2Node_ActorBoundEvent* EventNodeTemplate = NewObject<UK2Node_ActorBoundEvent>();
				EventNodeTemplate->InitializeActorBoundEventParams(Actor, DelegateProperty);

				UK2Node_ActorBoundEvent* EventNode = FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node_ActorBoundEvent>(TargetGraph, EventNodeTemplate, NewNodePos);

				// Finally, bring up kismet and jump to the new node
				if(EventNode != NULL)
				{
					BringKismetToFocusAttentionOnObject(EventNode);
				}
			}
		}
	}
}

void FKismetEditorUtilities::CreateNewBoundEventForComponent(UActorComponent* Component, FName EventName, UBlueprint* Blueprint, UObjectProperty* ComponentProperty)
{
	if ((Component != NULL) && (EventName != NAME_None) && (Blueprint != NULL) && (ComponentProperty != NULL))
	{
		// First, find the property we want to bind to
		UMulticastDelegateProperty* DelegateProperty = FindField<UMulticastDelegateProperty>(Component->GetClass(), EventName);
		if(DelegateProperty != NULL)
		{
			UEdGraph* TargetGraph = NULL;
			if(Blueprint->UbergraphPages.Num() > 0)
			{
				TargetGraph = Blueprint->UbergraphPages[0]; // Just use the first graph
			}

			if(TargetGraph != NULL)
			{
				// Figure out a decent place to stick the node
				const FVector2D NewNodePos = TargetGraph->GetGoodPlaceForNewNode();

				// Create a new event node
				UK2Node_ComponentBoundEvent* EventNodeTemplate = NewObject<UK2Node_ComponentBoundEvent>();
				EventNodeTemplate->InitializeComponentBoundEventParams(ComponentProperty, DelegateProperty);

				UK2Node_ComponentBoundEvent* EventNode = FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node_ComponentBoundEvent>(TargetGraph, EventNodeTemplate, NewNodePos);

				// Finally, bring up kismet and jump to the new node
				if (EventNode != NULL)
				{
					BringKismetToFocusAttentionOnObject(EventNode);
				}
			}
		}
	}
}

const UK2Node_ActorBoundEvent* FKismetEditorUtilities::FindBoundEventForActor(AActor* Actor, FName EventName)
{
	const UK2Node_ActorBoundEvent* Node = NULL;
	if(Actor != NULL && EventName != NAME_None)
	{
		ULevelScriptBlueprint* LSB = Actor->GetLevel()->GetLevelScriptBlueprint();
		if(LSB != NULL)
		{
			TArray<UK2Node_ActorBoundEvent*> EventNodes;
			FBlueprintEditorUtils::GetAllNodesOfClass(LSB, EventNodes);
			for(int32 i=0; i<EventNodes.Num(); i++)
			{
				UK2Node_ActorBoundEvent* BoundEvent = EventNodes[i];
				if(BoundEvent->EventOwner == Actor && BoundEvent->DelegatePropertyName == EventName)
				{
					Node = BoundEvent;
					break;
				}
			}
		}
	}
	return Node;
}

const UK2Node_ComponentBoundEvent* FKismetEditorUtilities::FindBoundEventForComponent(const UBlueprint* Blueprint, FName EventName, FName PropertyName)
{
	const UK2Node_ComponentBoundEvent* Node = NULL;
	if (Blueprint && EventName != NAME_None && PropertyName != NAME_None)
	{
		TArray<UK2Node_ComponentBoundEvent*> EventNodes;
		FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, EventNodes);
		for(auto NodeIter = EventNodes.CreateIterator(); NodeIter; ++NodeIter)
		{
			UK2Node_ComponentBoundEvent* BoundEvent = *NodeIter;
			if ((BoundEvent->ComponentPropertyName == PropertyName) && (BoundEvent->DelegatePropertyName == EventName))
			{
				Node = *NodeIter;
				break;
			}
		}
	}
	return Node;
}

bool FKismetEditorUtilities::IsClassABlueprintInterface(const UClass* Class)
{
	if (Class->HasAnyClassFlags(CLASS_Interface) && !Class->HasAnyClassFlags(CLASS_NewerVersionExists))
	{
		return true;
	}
	return false;
}



bool FKismetEditorUtilities::CanBlueprintImplementInterface(UBlueprint const* Blueprint, UClass const* Class)
{
	bool bCanImplementInterface = false;

	// if the class is an actual implementable interface
	if (IsClassABlueprintInterface(Class) && !Class->HasMetaData(FBlueprintMetadata::MD_CannotImplementInterfaceInBlueprint))
	{
		bCanImplementInterface = true;

		UClass const* const ParentClass = Blueprint->ParentClass;
		// see if the parent class has any prohibited interfaces
		if ((ParentClass != NULL) && ParentClass->HasMetaData(FBlueprintMetadata::MD_ProhibitedInterfaces))
		{
			FString const& ProhibitedList = Blueprint->ParentClass->GetMetaData(FBlueprintMetadata::MD_ProhibitedInterfaces);
			
			TArray<FString> ProhibitedInterfaceNames;
			ProhibitedList.ParseIntoArray(&ProhibitedInterfaceNames, TEXT(","), true);

			FString const& InterfaceName = Class->GetName();
			// loop over all the prohibited interfaces
			for (int32 ExclusionIndex = 0; ExclusionIndex < ProhibitedInterfaceNames.Num(); ++ExclusionIndex)
			{
				FString const& Exclusion = ProhibitedInterfaceNames[ExclusionIndex].Trim();
				// if this interface matches one of the prohibited ones
				if (InterfaceName == Exclusion) 
				{
					bCanImplementInterface = false;
					break;
				}
			}
		}
	}

	return bCanImplementInterface;
}

bool FKismetEditorUtilities::IsClassABlueprintSkeleton(const UClass* Class)
{
	// Find generating blueprint for a class
	UBlueprint* GeneratingBP = Cast<UBlueprint>(Class->ClassGeneratedBy);
	if( GeneratingBP )
	{
		return (Class == GeneratingBP->SkeletonGeneratedClass) && (GeneratingBP->SkeletonGeneratedClass != GeneratingBP->GeneratedClass);
	}
	return false;
}

/** Run over the components references, and then NULL any that fall outside this blueprint's scope (e.g. components brought over after reparenting from another class, which are now in the transient package) */
void FKismetEditorUtilities::StripExternalComponents(class UBlueprint* Blueprint)
{
	FArchiveInvalidateTransientRefs InvalidateRefsAr;
	
	UClass* SkeletonGeneratedClass = Blueprint->SkeletonGeneratedClass;
	UObject* SkeletonCDO = SkeletonGeneratedClass->GetDefaultObject();

	SkeletonCDO->Serialize(InvalidateRefsAr);

	UClass* GeneratedClass = Blueprint->GeneratedClass;
	UObject* GeneratedCDO = GeneratedClass->GetDefaultObject();

	GeneratedCDO->Serialize(InvalidateRefsAr);
}


/* Add search meta information for a given class type*/
template<class NodeType>
inline void AddSearchMetaInfo( TArray<UObject::FAssetRegistryTag> &OutTags, const FName& Category, const UBlueprint* Blueprint)
{
	TArray<NodeType*> Nodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Nodes );

	//To avoid storing duplicate entries in the asset registry, if duplicates are found we indicate how many by appending "::#"
	TMap<FString,int> AllPaths;
	for(auto It(Nodes.CreateConstIterator());It;++It)
	{
		NodeType* Node = *It;

		// Make sure we don't add MD for nodes that are going away soon
		if( Node->GetOuter()->IsPendingKill() )
		{
			continue;
		}

		// If the result is empty, the node has no data to offer.
		FString Result = FindInBlueprintsUtil::GetNodeTypePath(Node);
		if(Result.IsEmpty())
		{
			continue;
		}
		Result += FString("::") + Node->GetNodeTitle(ENodeTitleType::ListView);

		if(int* Count = AllPaths.Find(Result))
		{
			(*Count)++;
		}
		else
		{
			AllPaths.Add(Result,1);
		}
	}

	FString FinalString;
	for(auto It(AllPaths.CreateConstIterator());It;++It)
	{
		FString Result = *It.Key();
		if(It.Value()>1)
		{
			Result = FString::Printf(TEXT("%s::%i"), *It.Key(), int(It.Value()));
		}
		FinalString += Result + ",";

	}
	OutTags.Add( UObject::FAssetRegistryTag(Category, FinalString, UObject::FAssetRegistryTag::TT_Hidden) );
}



void FKismetEditorUtilities::GetAssetRegistryTagsForBlueprint(const UBlueprint* Blueprint, TArray<UObject::FAssetRegistryTag>& OutTags)
{
	//Add information about which functions are called
	AddSearchMetaInfo<UK2Node_CallFunction>(OutTags, TEXT("CallsFunctions"), Blueprint);

	//Add information about which macro instances are used
	AddSearchMetaInfo<UK2Node_MacroInstance>(OutTags, TEXT("MacroInstances"), Blueprint);

	//Add information about which events are used 
	AddSearchMetaInfo<UK2Node_Event>(OutTags, TEXT("ImplementsFunction"), Blueprint);

	//Add information about which variable gets are used
	AddSearchMetaInfo<UK2Node_VariableGet>(OutTags, TEXT("VariableGet"), Blueprint);

	//Add information about which variable sets are used
	AddSearchMetaInfo<UK2Node_VariableSet>(OutTags, TEXT("VariableSet"), Blueprint);

	//Add information about which delegates
	AddSearchMetaInfo<UK2Node_BaseMCDelegate>(OutTags, TEXT("MulticastDelegate"), Blueprint);
	AddSearchMetaInfo<UK2Node_DelegateSet>(OutTags, TEXT("DelegateSet"), Blueprint);

	//Add information for all node comments
	AddSearchMetaInfo<UEdGraphNode>(OutTags, TEXT("Comments"), Blueprint);
}

bool FKismetEditorUtilities::IsTrackedBlueprintParent(const UClass* ParentClass)
{
	if (ParentClass->ClassGeneratedBy == NULL)
	{
		// Always track native parent classes
		return true;
	}

	UBlueprint* ParentBlueprint = Cast<UBlueprint>(ParentClass->ClassGeneratedBy);

	// Cache the list of allowed blueprint names the first time it is requested
	if (TrackedBlueprintParentList.Num() == 0)
	{
		GConfig->GetArray(TEXT("Kismet"), TEXT("TrackedBlueprintParents"), /*out*/ TrackedBlueprintParentList, GEngineIni);
	}

	for (auto TrackedBlueprintIter = TrackedBlueprintParentList.CreateConstIterator(); TrackedBlueprintIter; ++TrackedBlueprintIter)
	{
		if (ParentBlueprint->GetName().EndsWith(*TrackedBlueprintIter))
		{
			return true;
		}
	}
	return false;
}

bool FKismetEditorUtilities::IsActorValidForLevelScript(const AActor* Actor)
{
	return Actor && !FActorEditorUtils::IsABuilderBrush(Actor);
}

bool FKismetEditorUtilities::AnyBoundLevelScriptEventForActor(AActor* Actor, bool bCouldAddAny)
{
	if (IsActorValidForLevelScript(Actor))
	{
		for (TFieldIterator<UMulticastDelegateProperty> PropertyIt(Actor->GetClass(), EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			UProperty* Property = *PropertyIt;
			// Check for multicast delegates that we can safely assign
			if (!Property->HasAnyPropertyFlags(CPF_Parm) && Property->HasAllPropertyFlags(CPF_BlueprintAssignable))
			{
				const FName EventName = Property->GetFName();
				const UK2Node_ActorBoundEvent* ExistingNode = FKismetEditorUtilities::FindBoundEventForActor(Actor, EventName);
				if ((NULL != ExistingNode) != bCouldAddAny)
				{
					return true;
				}
			}
		}
	}
	return false;
}

void FKismetEditorUtilities::AddLevelScriptEventOptionsForActor(class FMenuBuilder& MenuBuilder, TWeakObjectPtr<AActor> ActorPtr, bool bExistingEvents, bool bNewEvents, bool bOnlyEventName)
{
	struct FCreateEventForActorHelper
	{
		static void CreateEventForActor(TWeakObjectPtr<AActor> InActorPtr, FName EventName)
		{
			if (!GEditor->bIsSimulatingInEditor && GEditor->PlayWorld == NULL)
			{
				AActor* Actor = InActorPtr.Get();
				if (Actor != NULL && EventName != NAME_None)
				{
					const UK2Node_ActorBoundEvent* ExistingNode = FKismetEditorUtilities::FindBoundEventForActor(Actor, EventName);
					if (ExistingNode != NULL)
					{
						FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(ExistingNode);
					}
					else
					{
						FKismetEditorUtilities::CreateNewBoundEventForActor(Actor, EventName);
					}
				}
			}
		}
	};

	AActor* Actor = ActorPtr.Get();
	if (IsActorValidForLevelScript(Actor))
	{
		// Struct to store event properties by category
		struct FEventCategory
		{
			FString CategoryName;
			TArray<UProperty*> EventProperties;
		};
		// ARray of event properties by category
		TArray<FEventCategory> CategorizedEvents;

		// Find all events we can assign
		for (TFieldIterator<UMulticastDelegateProperty> PropertyIt(Actor->GetClass(), EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			UProperty* Property = *PropertyIt;
			// Check for multicast delegates that we can safely assign
			if (!Property->HasAnyPropertyFlags(CPF_Parm) && Property->HasAllPropertyFlags(CPF_BlueprintAssignable))
			{
				// Get category for this property
				FString PropertyCategory = FObjectEditorUtils::GetCategory(Property);
				// See if we already have a list for this
				bool bFound = false;
				for (FEventCategory& Category : CategorizedEvents)
				{
					if(Category.CategoryName == PropertyCategory)
					{
						Category.EventProperties.Add(Property);
						bFound = true;
					}
				}
				// If not, create one
				if(!bFound)
				{
					FEventCategory NewCategory;
					NewCategory.CategoryName = PropertyCategory;
					NewCategory.EventProperties.Add(Property);
					CategorizedEvents.Add(NewCategory);
				}
			}
		}

		// Now build the menu
		for(FEventCategory& Category : CategorizedEvents)
		{
			MenuBuilder.BeginSection(NAME_None, FText::FromString(Category.CategoryName));

			for(UProperty* Property : Category.EventProperties)
			{
				const FName EventName = Property->GetFName();
				const UK2Node_ActorBoundEvent* ExistingNode = FKismetEditorUtilities::FindBoundEventForActor(Actor, EventName);

				if ((!ExistingNode && !bNewEvents) || (ExistingNode && !bExistingEvents))
				{
					continue;
				}

				FText EntryText;
				if (bOnlyEventName)
				{
					EntryText = FText::FromName(EventName);
				}
				else
				{
					FFormatNamedArguments Args;
					Args.Add(TEXT("EventName"), FText::FromName(EventName));

					if (NULL == ExistingNode)
					{
						EntryText = FText::Format(LOCTEXT("AddEvent_ToolTip", "Add {EventName}"), Args);
					}
					else
					{
						EntryText = FText::Format(LOCTEXT("ViewEvent_ToolTip", "View {EventName}"), Args);
					}
				}

				// create menu entry
				MenuBuilder.AddMenuEntry(
					EntryText,
					Property->GetToolTipText(),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateStatic(&FCreateEventForActorHelper::CreateEventForActor, ActorPtr, EventName))
					);
			}

			MenuBuilder.EndSection();
		}
	}
}

void FKismetEditorUtilities::GetInformationOnMacro(UEdGraph* MacroGraph, /*out*/ UK2Node_Tunnel*& EntryNode, /*out*/ UK2Node_Tunnel*& ExitNode, bool& bIsMacroPure)
{
	check(MacroGraph);

	// Look at the graph for the entry & exit nodes
	TArray<UK2Node_Tunnel*> TunnelNodes;
	MacroGraph->GetNodesOfClass(TunnelNodes);

	for (int32 i = 0; i < TunnelNodes.Num(); i++)
	{
		UK2Node_Tunnel* Node = TunnelNodes[i];

		// Composite nodes should never be considered for function entry / exit, since we're searching for a graph's terminals
		if (Node->IsEditable() && !Node->IsA(UK2Node_Composite::StaticClass()))
		{
			if (Node->bCanHaveOutputs)
			{
				check(!EntryNode);
				EntryNode = Node;
			}
			else if (Node->bCanHaveInputs)
			{
				check(!ExitNode);
				ExitNode = Node;
			}
		}
	}

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Determine the macro's purity
	//@TODO: May want to check what is *inside* a macro too, to determine it's relative purity
	bIsMacroPure = true;

	if (EntryNode != NULL)
	{
		for (int32 PinIndex = 0; PinIndex < EntryNode->Pins.Num(); ++PinIndex)
		{
			if (K2Schema->IsExecPin(*(EntryNode->Pins[PinIndex])))
			{
				bIsMacroPure = false;
				break;
			}
		}
	}

	if (bIsMacroPure && (ExitNode != NULL))
	{
		for (int32 PinIndex = 0; PinIndex < ExitNode->Pins.Num(); ++PinIndex)
		{
			if (K2Schema->IsExecPin(*(ExitNode->Pins[PinIndex])))
			{
				bIsMacroPure = false;
				break;
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE 

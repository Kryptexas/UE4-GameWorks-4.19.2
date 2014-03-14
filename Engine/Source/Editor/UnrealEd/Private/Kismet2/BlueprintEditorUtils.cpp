// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "LatentActions.h"
#include "EngineLevelScriptClasses.h"
#include "Kismet2/CompilerResultsLog.h"

#include "Editor/KismetCompiler/Public/KismetCompilerModule.h"
#include "AnimGraphDefinitions.h"
#include "Editor/UnrealEd/Public/Kismet2/KismetEditorUtilities.h"
#include "Editor/UnrealEd/Public/Kismet2/KismetDebugUtilities.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"
#include "Editor/ClassViewer/Public/ClassViewerModule.h"
#include "Editor/ClassViewer/Public/ClassViewerFilter.h"

#include "BlueprintEditor.h"
#include "Editor/UnrealEd/Public/Kismet2/Kismet2NameValidators.h"

#include "DefaultValueHelper.h"
#include "ObjectEditorUtils.h"
#include "ToolkitManager.h"

#define LOCTEXT_NAMESPACE "Blueprint"

DEFINE_LOG_CATEGORY(LogBlueprintDebug);

FBlueprintEditorUtils::FFixLevelScriptActorBindingsEvent FBlueprintEditorUtils::FixLevelScriptActorBindingsEvent;

struct FCompareNodePriority
{
	FORCEINLINE bool operator()( const UK2Node& A, const UK2Node& B ) const
	{
		const bool NodeAChangesStructure = A.NodeCausesStructuralBlueprintChange();
		const bool NodeBChangesStructure = B.NodeCausesStructuralBlueprintChange();

		return ((NodeAChangesStructure > NodeBChangesStructure) ? true : false);
	}
};

/**
 * This helper does a depth first search, looking for the highest parent class that
 * implements the specified interface.
 * 
 * @param  Class		The class whose inheritance tree you want to check (NOTE: this class is checked itself as well).
 * @param  Interface	The interface that you're looking for.
 * @return NULL if the interface wasn't found, otherwise the highest parent class that implements the interface.
 */
static UClass* FindInheritedInterface(UClass* const Class, FBPInterfaceDescription const& Interface)
{
	UClass* ClassWithInterface = NULL;

	if (Class != NULL)
	{
		UClass* const ParentClass = Class->GetSuperClass();
		// search depth first so that we may find the highest parent in the chain that implements this interface
		ClassWithInterface = FindInheritedInterface(ParentClass, Interface);

		for (auto InterfaceIt(Class->Interfaces.CreateConstIterator()); InterfaceIt && (ClassWithInterface == NULL); ++InterfaceIt)
		{
			if (InterfaceIt->Class == Interface.Interface)
			{
				ClassWithInterface = Class;
				break;
			}
		}
	}

	return ClassWithInterface;
}

/**
 * This helper can be used to find a duplicate interface that is implemented higher
 * up the inheritance hierarchy (which can happen when you change parents or add an 
 * interface to a parent that's already implemented by a child).
 * 
 * @param  Interface	The interface you wish to find a duplicate of.
 * @param  Blueprint	The blueprint you wish to search.
 * @return True if one of the blueprint's super classes implements the specified interface, false if the child is free to implement it.
 */
static bool IsInterfaceImplementedByParent(FBPInterfaceDescription const& Interface, UBlueprint const* const Blueprint)
{
	check(Blueprint != NULL);
	return (FindInheritedInterface(Blueprint->ParentClass, Interface) != NULL);
}

/**
 * A helper function that takes two nodes belonging to the same graph and deletes 
 * one, replacing it with the other (moving over pin connections, etc.).
 * 
 * @param  OldNode	The node you want deleted.
 * @param  NewNode	The new replacement node that should take OldNode's place.
 */
static void ReplaceNode(UK2Node* OldNode, UK2Node* NewNode)
{
	check(OldNode->GetClass() == NewNode->GetClass());
	check(OldNode->GetOuter() == NewNode->GetOuter());

	UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
	K2Schema->BreakNodeLinks(*NewNode);

	for (UEdGraphPin* OldPin : OldNode->Pins)
	{
		UEdGraphPin* NewPin = NewNode->FindPinChecked(OldPin->PinName);
		NewPin->CopyPersistentDataFromOldPin(*OldPin);
	}

	K2Schema->BreakNodeLinks(*OldNode);

	NewNode->NodePosX = OldNode->NodePosX;
	NewNode->NodePosY = OldNode->NodePosY;

	FBlueprintEditorUtils::RemoveNode(OldNode->GetBlueprint(), OldNode, /* bDontRecompile =*/ true);
}

/**
 * Promotes any graphs that belong to the specified interface, and repurposes them
 * as parent overrides (function graphs that implement a parent's interface).
 * 
 * @param  Interface	The interface you're looking to promote.
 * @param  BlueprintObj	The blueprint that you want this applied to.
 */
static void PromoteInterfaceImplementationToOverride(FBPInterfaceDescription const& Interface, UBlueprint* const BlueprintObj)
{
	check(BlueprintObj != NULL);
	// find the parent whose interface we're overriding 
	UClass* ParentClass = FindInheritedInterface(BlueprintObj->ParentClass, Interface);

	if (ParentClass != NULL)
	{
		for (UEdGraph* InterfaceGraph : Interface.Graphs)
		{
			check(InterfaceGraph != NULL);

			TArray<UK2Node_FunctionEntry*> EntryNodes;
			InterfaceGraph->GetNodesOfClass(EntryNodes);
			check(EntryNodes.Num() == 1); 
			UK2Node_FunctionEntry* OldEntryNode = EntryNodes[0];

			TArray<UK2Node_FunctionResult*> ExitNodes;
			InterfaceGraph->GetNodesOfClass(ExitNodes);
			check(ExitNodes.Num() == 1);
			UK2Node_FunctionResult* OldExitNode = ExitNodes[0];

			// this will create its own entry and exit nodes
			FBlueprintEditorUtils::AddFunctionGraph(BlueprintObj, InterfaceGraph, /* bIsUserCreated =*/ false, ParentClass);

			InterfaceGraph->GetNodesOfClass(EntryNodes);
			for (UK2Node_FunctionEntry* EntryNode : EntryNodes)
			{
				if (EntryNode == OldEntryNode)
				{
					continue;
				}
				
				ReplaceNode(OldEntryNode, EntryNode);
				break;
			}

			InterfaceGraph->GetNodesOfClass(ExitNodes);
			for (UK2Node_FunctionResult* ExitNode : ExitNodes)
			{
				if (ExitNode == OldExitNode)
				{
					continue;
				}

				ReplaceNode(OldExitNode, ExitNode);
				break;
			}
		}

		// if any graphs were moved
		if (Interface.Graphs.Num() > 0)
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BlueprintObj);
		}
	}
}

void FBlueprintEditorUtils::RefreshAllNodes(UBlueprint* Blueprint)
{
	TArray<UK2Node*> AllNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, AllNodes);

	const bool bIsMacro = (Blueprint->BlueprintType == BPTYPE_MacroLibrary);
	if( AllNodes.Num() > 1 )
	{
		AllNodes.Sort(FCompareNodePriority());
	}

	bool bLastChangesStructure = (AllNodes.Num() > 0) ? AllNodes[0]->NodeCausesStructuralBlueprintChange() : true;
	for( TArray<UK2Node*>::TIterator NodeIt(AllNodes); NodeIt; ++NodeIt )
	{
		UK2Node* CurrentNode = *NodeIt;

		// See if we've finished the batch of nodes that affect structure, and recompile the skeleton if needed
		const bool bCurrentChangesStructure = CurrentNode->NodeCausesStructuralBlueprintChange();
		if( bLastChangesStructure != bCurrentChangesStructure )
		{
			// Make sure sorting was valid!
			check(bLastChangesStructure && !bCurrentChangesStructure);

			// Recompile the skeleton class, now that all changes to entry point structure has taken place
			// Ignore this for macros
			if (!bIsMacro)
			{
				FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			}
			bLastChangesStructure = bCurrentChangesStructure;
		}

		//@todo:  Do we really need per-schema refreshing?
		const UEdGraphSchema* Schema = CurrentNode->GetGraph()->GetSchema();
		Schema->ReconstructNode(*CurrentNode, true);
	}

	// If all nodes change structure, catch that case and recompile now
	if( bLastChangesStructure )
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
}

void FBlueprintEditorUtils::RefreshExternalBlueprintDependencyNodes(UBlueprint* Blueprint)
{
	TArray<UK2Node*> AllNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, AllNodes);


	for( auto NodeIt = AllNodes.CreateIterator(); NodeIt; ++NodeIt )
	{
		UK2Node* Node = *NodeIt;
		if( Node->HasExternalBlueprintDependencies() )
		{
			//@todo:  Do we really need per-schema refreshing?
			const UEdGraphSchema* Schema = Node->GetGraph()->GetSchema();
			Schema->ReconstructNode(*Node, true);
		}
	}
}

void FBlueprintEditorUtils::PreloadMembers(UObject* InObject)
{
	// Collect a list of all things this element owns
	TArray<UObject*> BPMemberReferences;
	FReferenceFinder ComponentCollector(BPMemberReferences, InObject, false, true, true, true);
	ComponentCollector.FindReferences(InObject);

	// Iterate over the list, and preload everything so it is valid for refreshing
	for( TArray<UObject*>::TIterator it(BPMemberReferences); it; ++it )
	{
		UObject* CurrentObject = *it;
		if( CurrentObject->HasAnyFlags(RF_NeedLoad) )
		{
			CurrentObject->GetLinker()->Preload(CurrentObject);
			PreloadMembers(CurrentObject);
		}
	}
}

void FBlueprintEditorUtils::PreloadConstructionScript(UBlueprint* Blueprint)
{
	ULinkerLoad* TargetLinker = (Blueprint && Blueprint->SimpleConstructionScript) ? Blueprint->SimpleConstructionScript->GetLinker() : NULL;
	if( TargetLinker )
	{
		TargetLinker->Preload(Blueprint->SimpleConstructionScript);

		if (USCS_Node* DefaultSceneRootNode = Blueprint->SimpleConstructionScript->GetDefaultSceneRootNode())
		{
			DefaultSceneRootNode->PreloadChain();
		}

		const TArray<USCS_Node*>& RootNodes = Blueprint->SimpleConstructionScript->GetRootNodes();
		for(int32 NodeIndex = 0; NodeIndex < RootNodes.Num(); ++NodeIndex)
		{
			RootNodes[NodeIndex]->PreloadChain();
		}
	}
}

/** Helper function to patch the new CDO into the linker where the old one existed */
void PatchNewCDOIntoLinker(UObject* CDO, ULinkerLoad* Linker, int32 ExportIndex, TArray<UObject*>& ObjLoaded)
{
	if( (CDO != NULL) && (Linker != NULL) && (ExportIndex != INDEX_NONE) )
	{
		// Get rid of the old thing that was in its place
		UObject* OldCDO = Linker->ExportMap[ExportIndex].Object;
		if( OldCDO != NULL )
		{
			EObjectFlags OldObjectFlags = OldCDO->GetFlags();
			OldCDO->ClearFlags(RF_NeedLoad|RF_NeedPostLoad);
			OldCDO->SetLinker(NULL, INDEX_NONE);
			// Copy flags from the old CDO.
			CDO->SetFlags(OldObjectFlags);
			// Make sure the new CDO gets PostLoad called on it so add it to ObjLoaded list.
			if (OldObjectFlags & RF_NeedPostLoad)
			{
				ObjLoaded.Add(CDO);
			}
		}

		// Patch the new CDO in, and update the Export.Object
		CDO->SetLinker(Linker, ExportIndex);
		Linker->ExportMap[ExportIndex].Object = CDO;
	}
}

UClass* FBlueprintEditorUtils::FindFirstNativeClass(UClass* Class)
{
	for(; Class; Class = Class->GetSuperClass() )
	{
		if( 0 != (Class->ClassFlags & CLASS_Native))
		{
			break;
		}
	}
	return Class;
}

void FBlueprintEditorUtils::GetAllGraphNames(const UBlueprint* Blueprint, TArray<FName>& GraphNames)
{
	TArray< UEdGraph* > GraphList;
	Blueprint->GetAllGraphs(GraphList);

	for(int32 GraphIdx = 0; GraphIdx < GraphList.Num(); ++GraphIdx)
	{
		GraphNames.Add(GraphList[GraphIdx]->GetFName());
	}

	// Include all functions from parents because they should never conflict
	TArray<UBlueprint*> ParentBPStack;
	UBlueprint::GetBlueprintHierarchyFromClass(Blueprint->SkeletonGeneratedClass, ParentBPStack);
	for (int32 StackIndex = ParentBPStack.Num() - 1; StackIndex >= 0; --StackIndex)
	{
		UBlueprint* ParentBP = ParentBPStack[StackIndex];
		check(ParentBP != NULL);

		for(int32 FunctionIndex = 0; FunctionIndex < ParentBP->FunctionGraphs.Num(); ++FunctionIndex)
		{
			GraphNames.AddUnique(ParentBP->FunctionGraphs[FunctionIndex]->GetFName());
		}
	}
}

/** 
 * Check FKismetCompilerContext::SetCanEverTickForActor
 */
struct FSaveActorFlagsHelper
{
	bool bOverride;
	bool bCanEverTick;
	bool bCanBeDamaged;
	UClass * Class;

	FSaveActorFlagsHelper(UClass * InClass) : Class(InClass)
	{
		bOverride = (AActor::StaticClass() == FBlueprintEditorUtils::FindFirstNativeClass(Class));
		if(Class && bOverride)
		{
			AActor * CDActor = Cast<AActor>(Class->GetDefaultObject());
			if(CDActor)
			{
				bCanEverTick = CDActor->PrimaryActorTick.bCanEverTick;
				bCanBeDamaged = CDActor->bCanBeDamaged;
			}
		}
	}

	~FSaveActorFlagsHelper()
	{
		if(Class && bOverride)
		{
			AActor * CDActor = Cast<AActor>(Class->GetDefaultObject());
			if(CDActor)
			{
				CDActor->PrimaryActorTick.bCanEverTick = bCanEverTick;
				CDActor->bCanBeDamaged = bCanBeDamaged;
			}
		}
	}
};

//////////////////////////////////////////////////////////////////////////

/**
 * Archive built to go through and find any references to objects in the transient package, and then NULL those references
 */
class FArchiveMoveSkeletalRefs : public FArchiveUObject
{
public:
	FArchiveMoveSkeletalRefs(UBlueprint* TargetBP)
		: TargetBlueprint(TargetBP)
	{
		ArIsObjectReferenceCollector = true;
		ArIsPersistent = false;
		ArIgnoreArchetypeRef = false;
	}

	void UpdateReferences()
	{
		if( TargetBlueprint != NULL && (TargetBlueprint->BlueprintType != BPTYPE_MacroLibrary) )
		{
			check(TargetBlueprint->SkeletonGeneratedClass);
			check(TargetBlueprint->GeneratedClass);
			TargetBlueprint->SkeletonGeneratedClass->GetDefaultObject()->Serialize(*this);
			TargetBlueprint->GeneratedClass->GetDefaultObject()->Serialize(*this);

			TArray<UObject*> SubObjs;
			GetObjectsWithOuter(TargetBlueprint, SubObjs, true);

			for( auto ObjIt = SubObjs.CreateIterator(); ObjIt; ++ObjIt )
			{
				(*ObjIt)->Serialize(*this);
			}

			TargetBlueprint->bLegacyNeedToPurgeSkelRefs = false;
		}
	}

protected:
	UBlueprint* TargetBlueprint;

	/** 
	 * UObject serialize operator implementation
	 *
	 * @param Object	reference to Object reference
	 * @return reference to instance of this class
	 */
	FArchive& operator<<( UObject*& Object )
	{
		// Check if this is a reference to an object existing in the transient package, and if so, NULL it.
		if (Object != NULL )
		{
			if( UClass* RefClass = Cast<UClass>(Object) )
			{
				UClass* AuthClass = RefClass->GetAuthoritativeClass();
				if( RefClass != AuthClass )
				{
					Object = AuthClass;
				}
			}
		}

		return *this;
	}

private:
	// Want to make them HAVE to use a blueprint, so we can control what we replace refs on
	FArchiveMoveSkeletalRefs()
	{
	}
};

//////////////////////////////////////////////////////////////////////////

UClass* FBlueprintEditorUtils::RegenerateBlueprintClass(UBlueprint* Blueprint, UClass* ClassToRegenerate, UObject* PreviousCDO, TArray<UObject*>& ObjLoaded)
{
	bool bRegenerated = false;

	// Cache off the dirty flag for the package, so we can restore it later
	UPackage* Package = Cast<UPackage>(Blueprint->GetOutermost());
	bool bIsPackageDirty = Package ? Package->IsDirty() : false;

	bool bNeedsSkelRefRemoval = false;

	if( ShouldRegenerateBlueprint(Blueprint) && !Blueprint->bHasBeenRegenerated )
	{
		Blueprint->bIsRegeneratingOnLoad = true;

		// Cache off the linker index, if needed
		FName GeneratedName, SkeletonName;
		Blueprint->GetBlueprintCDONames(GeneratedName, SkeletonName);
		int32 OldSkelLinkerIdx = INDEX_NONE;
		int32 OldGenLinkerIdx = INDEX_NONE;
		ULinkerLoad* OldLinker = Blueprint->GetLinker();
		for( int32 i = 0; i < OldLinker->ExportMap.Num(); i++ )
		{
			FObjectExport& ThisExport = OldLinker->ExportMap[i];
			if( ThisExport.ObjectName == SkeletonName )
			{
				OldSkelLinkerIdx = i;
			}
			else if( ThisExport.ObjectName == GeneratedName )
			{
				OldGenLinkerIdx = i;
			}

			if( OldSkelLinkerIdx != INDEX_NONE && OldGenLinkerIdx != INDEX_NONE )
			{
				break;
			}
		}

		// Ensure that the package metadata is preloaded and valid
		if( Package )
		{
			UMetaData* MetaData = Package->GetMetaData();
			if( MetaData->HasAnyFlags(RF_NeedLoad) )
			{
				MetaData->GetLinker()->Preload(MetaData);
			}
		}


		// Make sure the simple construction script is loaded, since the outer hierarchy isn't compatible with PreloadMembers past the root node
		FBlueprintEditorUtils::PreloadConstructionScript(Blueprint);

#if WITH_EDITORONLY_DATA
		// Make sure all interface dependencies are loaded at this point
		for( TArray<FBPInterfaceDescription>::TIterator InterfaceIt(Blueprint->ImplementedInterfaces); InterfaceIt; ++InterfaceIt )
		{
			UClass* InterfaceClass = (*InterfaceIt).Interface;
			if( InterfaceClass && InterfaceClass->HasAnyFlags(RF_NeedLoad) )
			{
				InterfaceClass->GetLinker()->Preload(InterfaceClass);
			}
		}
#endif // WITH_EDITORONLY_DATA

		// Preload the blueprint and all its parts before refreshing nodes. Otherwise, the nodes might not maintain their proper linkages
		if( PreviousCDO )
		{
			FBlueprintEditorUtils::PreloadMembers(PreviousCDO);
		}

		FBlueprintEditorUtils::PreloadMembers(Blueprint);

		// Purge any NULL graphs
		FBlueprintEditorUtils::PurgeNullGraphs(Blueprint);

		// Now that things have been preloaded, see what work needs to be done to refresh this blueprint
		const bool bIsMacro = (Blueprint->BlueprintType == BPTYPE_MacroLibrary);
		const bool bHasCode = !FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint) && !bIsMacro;

		if( bHasCode )
		{
			// Make sure parent function calls are up to date
			FBlueprintEditorUtils::ConformCallsToParentFunctions(Blueprint);

			// Make sure events are up to date
			FBlueprintEditorUtils::ConformImplementedEvents(Blueprint);

			// Make sure interfaces are up to date
			FBlueprintEditorUtils::ConformImplementedInterfaces(Blueprint);

			// Refresh all nodes to make sure function signatures are up to date, etc.
			FBlueprintEditorUtils::RefreshAllNodes(Blueprint);

			// Compile the actual blueprint
			FKismetEditorUtilities::CompileBlueprint(Blueprint, true);
		}
		else if( bIsMacro )
		{
			// Just refresh all nodes in macro blueprints, but don't recompil
			FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
		}
		else
		{
			if (Blueprint->IsGeneratedClassAuthoritative() && (Blueprint->GeneratedClass != NULL))
			{
				check(PreviousCDO != NULL);
				check(Blueprint->SkeletonGeneratedClass != NULL);

				// We now know we're a data-only blueprint on the outer pass (generate class is valid), where generated class is authoritative
				// If the PreviousCDO is to the skeleton, then it will corrupt data when copied over the AuthoriativeClass later on in this function
				if (PreviousCDO == Blueprint->SkeletonGeneratedClass->GetDefaultObject())
				{
					check(Blueprint->PRIVATE_InnermostPreviousCDO == NULL);
					Blueprint->PRIVATE_InnermostPreviousCDO = Blueprint->GeneratedClass->GetDefaultObject();
				}
			}

			// No actual compilation work to be done, but try to conform the class and fix up anything that might need to be updated if the native base class has changed in any way
			FKismetEditorUtilities::ConformBlueprintFlagsAndComponents(Blueprint);

			// Flag data only blueprints as being up-to-date
		}
		
		// Patch the new CDOs to the old indices in the linker
		if( Blueprint->SkeletonGeneratedClass )
		{
			PatchNewCDOIntoLinker(Blueprint->SkeletonGeneratedClass->GetDefaultObject(), OldLinker, OldSkelLinkerIdx, ObjLoaded);
		}
		if( Blueprint->GeneratedClass )
		{
			PatchNewCDOIntoLinker(Blueprint->GeneratedClass->GetDefaultObject(), OldLinker, OldGenLinkerIdx, ObjLoaded);
		}

		// Success or failure, there's no point in trying to recompile this class again when other objects reference it
		// redo data only blueprints later, when we actually have a generated class
		Blueprint->bHasBeenRegenerated = !FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint) || Blueprint->GeneratedClass != NULL; 

		Blueprint->bIsRegeneratingOnLoad = false;

		if( Package )
		{
			// Tell the linker to try to find exports in memory first, so that it gets the new, regenerated versions
			Package->FindExportsInMemoryFirst(true);
		}

		bRegenerated = bHasCode;
	}

	if ( PreviousCDO != NULL )
	{
		if( Blueprint->bRecompileOnLoad && !GForceDisableBlueprintCompileOnLoad)
		{
			if (Blueprint->BlueprintType != BPTYPE_MacroLibrary)
			{
				// Verify that we had a skeleton generated class if we had a previous CDO, to make sure we have something to copy into
				check(Blueprint->SkeletonGeneratedClass);
			}

			const bool bPreviousMatchesGenerated = (PreviousCDO == Blueprint->GeneratedClass->GetDefaultObject());

			if (Blueprint->BlueprintType != BPTYPE_MacroLibrary)
			{
				UObject* CDOThatKickedOffCOL = PreviousCDO;
				if (Blueprint->IsGeneratedClassAuthoritative() && !bPreviousMatchesGenerated && Blueprint->PRIVATE_InnermostPreviousCDO)
				{
					PreviousCDO = Blueprint->PRIVATE_InnermostPreviousCDO;
				}
			}

			// If this is the top of the compile-on-load stack for this object, copy the old CDO properties to the newly created one unless they are the same
			UClass* AuthoritativeClass = (Blueprint->IsGeneratedClassAuthoritative() ? Blueprint->GeneratedClass : Blueprint->SkeletonGeneratedClass);
			if (AuthoritativeClass != NULL && PreviousCDO != AuthoritativeClass->GetDefaultObject())
			{
				TGuardValue<bool> GuardTemplateNameFlag(GCompilingBlueprint, true);

				// Make sure the previous CDO has been fully loaded before we use it
				if( PreviousCDO )
				{
					FBlueprintEditorUtils::PreloadMembers(PreviousCDO);
				}

				// Copy over the properties from the old CDO to the new
				PropagateParentBlueprintDefaults(AuthoritativeClass);
				UObject* NewCDO = AuthoritativeClass->GetDefaultObject();
				{
					FSaveActorFlagsHelper SaveActorFlags(AuthoritativeClass);
					UEditorEngine::FCopyPropertiesForUnrelatedObjectsParams CopyDetails;
					CopyDetails.bAggressiveDefaultSubobjectReplacement = true;
					CopyDetails.bDoDelta = false;
					UEditorEngine::CopyPropertiesForUnrelatedObjects(PreviousCDO, NewCDO, CopyDetails);
				}

				if( bRegenerated )
				{
					// Collect the instanced components in both the old and new CDOs
					TArray<UObject*> OldComponents, NewComponents;
					PreviousCDO->CollectDefaultSubobjects(OldComponents,true);
					NewCDO->CollectDefaultSubobjects(NewComponents,true);

					// For all components common to both, patch the linker table with the new version of the component, so things that reference the default (e.g. InternalArchetypes) will have the updated version
					for( auto OldCompIt = OldComponents.CreateIterator(); OldCompIt; ++OldCompIt )
					{
						UObject* OldComponent = (*OldCompIt);
						const FName OldComponentName = OldComponent->GetFName();
						for( auto NewCompIt = NewComponents.CreateIterator(); NewCompIt; ++NewCompIt )
						{
							UObject* NewComponent = *NewCompIt;
							if( NewComponent->GetFName() == OldComponentName )
							{
								ULinkerLoad::PRIVATE_PatchNewObjectIntoExport(OldComponent, NewComponent);
								break;
							}
						}
					}
					NewCDO->CheckDefaultSubobjects();
					NewCDO->ConditionalPostLoad();
				}
			}

			Blueprint->PRIVATE_InnermostPreviousCDO = NULL;
		}
		else
		{
			// If we didn't recompile, we still need to propagate flags, and instance components
			FKismetEditorUtilities::ConformBlueprintFlagsAndComponents(Blueprint);
		}

		// If this is the top of the compile-on-load stack for this object, copy the old CDO properties to the newly created one
		if (!Blueprint->IsGeneratedClassAuthoritative() && Blueprint->GeneratedClass != NULL)
		{
			TGuardValue<bool> GuardTemplateNameFlag(GCompilingBlueprint, true);

			UObject* SkeletonCDO = Blueprint->SkeletonGeneratedClass->GetDefaultObject();
			UObject* GeneratedCDO = Blueprint->GeneratedClass->GetDefaultObject();

			UEditorEngine::FCopyPropertiesForUnrelatedObjectsParams CopyDetails;
			CopyDetails.bAggressiveDefaultSubobjectReplacement = false;
			CopyDetails.bDoDelta = false;
			UEditorEngine::CopyPropertiesForUnrelatedObjects(SkeletonCDO, GeneratedCDO, CopyDetails);
			
			Blueprint->SetLegacyGeneratedClassIsAuthoritative();
		}

		Blueprint->ClearFlags(RF_BeingRegenerated);
		bNeedsSkelRefRemoval = true;
	}

	if ( bRegenerated )
	{		
		// Fix any invalid metadata
		UPackage* GeneratedClassPackage = Blueprint->GeneratedClass->GetOuterUPackage();
		GeneratedClassPackage->GetMetaData()->RemoveMetaDataOutsidePackage();
	}

	if ( bNeedsSkelRefRemoval && Blueprint->bLegacyNeedToPurgeSkelRefs)
	{
		// Remove any references to the skeleton class, replacing them with refs to the generated class instead
		FArchiveMoveSkeletalRefs SkelRefArchiver(Blueprint);
		SkelRefArchiver.UpdateReferences();
	}

	// Restore the dirty flag
	if( Package )
	{
		Package->SetDirtyFlag(bIsPackageDirty);
	}

	return bRegenerated ? Blueprint->GeneratedClass : NULL;
}

void FBlueprintEditorUtils::PropagateParentBlueprintDefaults(UClass* ClassToPropagate)
{
	check(ClassToPropagate);

	UObject* NewCDO = ClassToPropagate->GetDefaultObject();
	
	check(NewCDO);

	// Get the blueprint's BP derived lineage
	TArray<UBlueprint*> ParentBP;
	UBlueprint::GetBlueprintHierarchyFromClass(ClassToPropagate, ParentBP);

	// Starting from the least derived BP class, copy the properties into the new CDO
	for(int32 i = ParentBP.Num() - 1; i > 0; i--)
	{
		checkf(ParentBP[i]->GeneratedClass != NULL, TEXT("Parent classes for class %s have not yet been generated.  Compile-on-load must be processed for the parent class first."), *ClassToPropagate->GetName());
		UObject* LayerCDO = ParentBP[i]->GeneratedClass->GetDefaultObject();

		UEditorEngine::FCopyPropertiesForUnrelatedObjectsParams CopyDetails;
		CopyDetails.bReplaceObjectClassReferences = false;
		UEditorEngine::CopyPropertiesForUnrelatedObjects(LayerCDO, NewCDO, CopyDetails);
	}
}

void FBlueprintEditorUtils::PostDuplicateBlueprint(UBlueprint* Blueprint)
{
	// Only recompile after duplication if this isn't PIE
	if (!GIsPlayInEditorWorld)
	{
		check(Blueprint->GeneratedClass != NULL);
		{
			// Grab the old CDO, which contains the blueprint defaults
			UClass* OldBPGCAsClass = Blueprint->GeneratedClass;
			UBlueprintGeneratedClass* OldBPGC = (UBlueprintGeneratedClass*)(OldBPGCAsClass);
			UObject* OldCDO = OldBPGC->GetDefaultObject();
			check(OldCDO != NULL);

			// Grab the old class templates, which needs to be moved to the new class
			USimpleConstructionScript* SCSRootNode = Blueprint->SimpleConstructionScript;
			Blueprint->SimpleConstructionScript = NULL;

			TArray<UActorComponent*> Templates = Blueprint->ComponentTemplates;
			Blueprint->ComponentTemplates.Empty();

			TArray<UTimelineTemplate*> Timelines = Blueprint->Timelines;
			Blueprint->Timelines.Empty();

			// Null out the existing class references, the compile will create new ones
			Blueprint->GeneratedClass = NULL;
			Blueprint->SkeletonGeneratedClass = NULL;

			// Make sure the new blueprint has a shiny new class
			IKismetCompilerInterface& Compiler = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(KISMET_COMPILER_MODULENAME);
			FCompilerResultsLog Results;
			FKismetCompilerOptions CompileOptions;
			CompileOptions.bIsDuplicationInstigated = true;

			//SCS can change structure of the class
			const bool bDontCompileClassBeforeScsIsSet = FBlueprintEditorUtils::SupportsConstructionScript(Blueprint) ||  FBlueprintEditorUtils::IsActorBased(Blueprint);
			if (bDontCompileClassBeforeScsIsSet)
			{ 
				FName NewSkelClassName, NewGenClassName;
				Blueprint->GetBlueprintClassNames(NewGenClassName, NewSkelClassName);
				UBlueprintGeneratedClass* NewClass = ConstructObject<UBlueprintGeneratedClass>(
					UBlueprintGeneratedClass::StaticClass(), Blueprint->GetOutermost(), NewGenClassName, RF_Public|RF_Transactional);
				Blueprint->GeneratedClass = NewClass;
				NewClass->ClassGeneratedBy = Blueprint;
				NewClass->SetSuperStruct(Blueprint->ParentClass);
			}
			else
			{
				Compiler.CompileBlueprint(Blueprint, CompileOptions, Results);
			}

			TMap<UObject*, UObject*> OldToNewMap;

			UClass* NewBPGCAsClass = Blueprint->GeneratedClass;
			UBlueprintGeneratedClass* NewBPGC = (UBlueprintGeneratedClass*)(NewBPGCAsClass);
			if( SCSRootNode )
			{
				NewBPGC->SimpleConstructionScript = Cast<USimpleConstructionScript>(StaticDuplicateObject(SCSRootNode, NewBPGC, *SCSRootNode->GetName()));
				TArray<USCS_Node*> AllNodes = NewBPGC->SimpleConstructionScript->GetAllNodes();

				// Duplicate all component templates
				for(auto NodeIt = AllNodes.CreateIterator(); NodeIt; ++NodeIt)
				{
					USCS_Node* CurrentNode = *NodeIt;
					if(CurrentNode->ComponentTemplate)
					{
						UActorComponent* DuplicatedComponent = CastChecked<UActorComponent>(StaticDuplicateObject(CurrentNode->ComponentTemplate, NewBPGC, *CurrentNode->ComponentTemplate->GetName()));
						OldToNewMap.Add(CurrentNode->ComponentTemplate, DuplicatedComponent);
						CurrentNode->ComponentTemplate = DuplicatedComponent;
					}
				}

				if (USCS_Node* DefaultSceneRootNode = NewBPGC->SimpleConstructionScript->GetDefaultSceneRootNode())
				{
					if (!AllNodes.Contains(DefaultSceneRootNode) && DefaultSceneRootNode->ComponentTemplate)
					{
						UActorComponent* DuplicatedComponent =  Cast<UActorComponent>(OldToNewMap.FindRef(DefaultSceneRootNode->ComponentTemplate));
						if (!DuplicatedComponent)
						{
							DuplicatedComponent = CastChecked<UActorComponent>(StaticDuplicateObject(DefaultSceneRootNode->ComponentTemplate, NewBPGC, *DefaultSceneRootNode->ComponentTemplate->GetName()));
							OldToNewMap.Add(DefaultSceneRootNode->ComponentTemplate, DuplicatedComponent);
						}
						DefaultSceneRootNode->ComponentTemplate = DuplicatedComponent;
					}
				}
			}

			for(auto CompIt = Templates.CreateIterator(); CompIt; ++CompIt)
			{
				UActorComponent* OldComponent = *CompIt;
				UActorComponent* NewComponent = CastChecked<UActorComponent>(StaticDuplicateObject(OldComponent, NewBPGC, *OldComponent->GetName()));

				NewBPGC->ComponentTemplates.Add(NewComponent);
				OldToNewMap.Add(OldComponent, NewComponent);
			}

			for(auto TimelineIt = Timelines.CreateIterator(); TimelineIt; ++TimelineIt)
			{
				UTimelineTemplate* OldTimeline = *TimelineIt;
				UTimelineTemplate* NewTimeline = CastChecked<UTimelineTemplate>(StaticDuplicateObject(OldTimeline, NewBPGC, *OldTimeline->GetName()));

				NewBPGC->Timelines.Add(NewTimeline);
				OldToNewMap.Add(OldTimeline, NewTimeline);
			}

			Blueprint->SimpleConstructionScript = NewBPGC->SimpleConstructionScript;
			Blueprint->ComponentTemplates = NewBPGC->ComponentTemplates;
			Blueprint->Timelines = NewBPGC->Timelines;

			if (bDontCompileClassBeforeScsIsSet)
			{ 
				Compiler.CompileBlueprint(Blueprint, CompileOptions, Results);
			}

			FArchiveReplaceObjectRef<UObject> ReplaceTemplateRefs(NewBPGC, OldToNewMap, /*bNullPrivateRefs=*/ false, /*bIgnoreOuterRef=*/ false, /*bIgnoreArchetypeRef=*/ false);

			// Now propagate the values from the old CDO to the new one
			check(Blueprint->SkeletonGeneratedClass != NULL);

			UObject* NewCDO = Blueprint->GeneratedClass->GetDefaultObject();
			check(NewCDO != NULL);
			UEditorEngine::CopyPropertiesForUnrelatedObjects(OldCDO, NewCDO);
		}

		Blueprint->UserDefinedStructures.Empty();

		// And compile again to make sure they go into the generated class, get cleaned up, etc...
		FKismetEditorUtilities::CompileBlueprint(Blueprint, false, true);

		// it can still keeps references to some external objects
		Blueprint->LastEditedDocuments.Empty();
	}

	// Should be no instances of this new blueprint, so no need to replace any
}

void FBlueprintEditorUtils::RemoveGeneratedClasses(UBlueprint* Blueprint)
{
	IKismetCompilerInterface& Compiler = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(KISMET_COMPILER_MODULENAME);
	Compiler.RemoveBlueprintGeneratedClasses(Blueprint);
}

void FBlueprintEditorUtils::UpdateDelegatesInBlueprint(UBlueprint* Blueprint)
{
	check(Blueprint);
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	for (auto GraphIt = Graphs.CreateIterator(); GraphIt; ++GraphIt)
	{
		UEdGraph* Graph = (*GraphIt);
		if(!IsGraphIntermediate(Graph))
		{
			TArray<UK2Node_CreateDelegate*> CreateDelegateNodes;
			Graph->GetNodesOfClass(CreateDelegateNodes);
			for (auto NodeIt = CreateDelegateNodes.CreateIterator(); NodeIt; ++NodeIt)
			{
				(*NodeIt)->HandleAnyChangeInner();
			}

			TArray<UK2Node_Event*> EventNodes;
			Graph->GetNodesOfClass(EventNodes);
			for (auto NodeIt = EventNodes.CreateIterator(); NodeIt; ++NodeIt)
			{
				(*NodeIt)->UpdateDelegatePin();
			}
		}
	}
}

// Blueprint has materially changed.  Recompile the skeleton, notify observers, and mark the package as dirty.
void FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(UBlueprint* Blueprint)
{
	if (Blueprint->Status != BS_BeingCreated && !Blueprint->bBeingCompiled)
	{
		{
			// Invoke the compiler to update the skeleton class definition
			IKismetCompilerInterface& Compiler = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(KISMET_COMPILER_MODULENAME);

			FCompilerResultsLog Results;
			Results.bLogInfoOnly = Blueprint->bIsRegeneratingOnLoad;

			FKismetCompilerOptions CompileOptions;
			CompileOptions.CompileType = EKismetCompileType::SkeletonOnly;
			Compiler.CompileBlueprint(Blueprint, CompileOptions, Results);
			Blueprint->Status = BS_Dirty;
		}
		UpdateDelegatesInBlueprint(Blueprint);

		// Notify any interested parties that the blueprint has changed
		Blueprint->BroadcastChanged();

		Blueprint->MarkPackageDirty();
	}
}

// Blueprint has changed in some manner that invalidates the compiled data (link made/broken, default value changed, etc...)
void FBlueprintEditorUtils::MarkBlueprintAsModified(UBlueprint* Blueprint)
{
	if (Blueprint->Status != BS_BeingCreated)
	{
		Blueprint->Status = BS_Dirty;
		Blueprint->MarkPackageDirty();
		Blueprint->PostEditChange();
	}
}

bool FBlueprintEditorUtils::ShouldRegenerateBlueprint(UBlueprint* Blueprint)
{
	return !GForceDisableBlueprintCompileOnLoad
		&& Blueprint->bRecompileOnLoad
		&& !Blueprint->bIsRegeneratingOnLoad;
}

// Helper function to get the blueprint that ultimately owns a node.
UBlueprint* FBlueprintEditorUtils::FindBlueprintForNode(const UEdGraphNode* Node)
{
	return FindBlueprintForGraph(Node->GetGraph());
}

// Helper function to get the blueprint that ultimately owns a node.  Cannot fail.
UBlueprint* FBlueprintEditorUtils::FindBlueprintForNodeChecked(const UEdGraphNode* Node)
{
	return FindBlueprintForGraphChecked(Node->GetGraph());
}


// Helper function to get the blueprint that ultimately owns a graph.
UBlueprint* FBlueprintEditorUtils::FindBlueprintForGraph(const UEdGraph* Graph)
{
	for (UObject* TestOuter = Graph->GetOuter(); TestOuter; TestOuter = TestOuter->GetOuter())
	{
		if (UBlueprint* Result = Cast<UBlueprint>(TestOuter))
		{
			return Result;
		}
	}

	return NULL;
}

// Helper function to get the blueprint that ultimately owns a graph.  Cannot fail.
UBlueprint* FBlueprintEditorUtils::FindBlueprintForGraphChecked(const UEdGraph* Graph)
{
	UBlueprint* Result = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	check(Result);
	return Result;
}

bool FBlueprintEditorUtils::IsGraphNameUnique(UBlueprint* Blueprint, const FName& InName)
{
	// Check for any object directly created in the blueprint
	if( !FindObject<UObject>(Blueprint, *InName.ToString()) )
	{
		// Next, check for functions with that name in the blueprint's class scope
		if( !FindField<UField>(Blueprint->SkeletonGeneratedClass, InName) )
		{
			// Finally, check function entry points
			TArray<UK2Node_Event*> AllEvents;
			FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Event>(Blueprint, AllEvents);

			for(int32 i=0; i < AllEvents.Num(); i++)
			{
				UK2Node_Event* EventNode = AllEvents[i];
				check(EventNode);

				if( EventNode->CustomFunctionName == InName
					|| EventNode->EventSignatureName == InName )
				{
					return false;
				}
			}

			// All good!
			return true;
		}
	}

	return false;
}

UEdGraph* FBlueprintEditorUtils::CreateNewGraph(UObject* ParentScope, const FName& GraphName, TSubclassOf<class UEdGraph> GraphClass, TSubclassOf<class UEdGraphSchema> SchemaClass)
{
	UEdGraph* NewGraph = NULL;
	bool bRename = false;

	// Ensure this name isn't already being used for a graph
	if (GraphName != NAME_None)
	{
		UEdGraph* ExistingGraph = FindObject<UEdGraph>(ParentScope, *(GraphName.ToString()));
		ensureMsgf(!ExistingGraph, TEXT("Graph %s already exists: %s"), *GraphName.ToString(), *ExistingGraph->GetFullName());

		// Rename the old graph out of the way; but we have already failed at this point
		if (ExistingGraph)
		{
			ExistingGraph->Rename(NULL, ExistingGraph->GetOuter(), REN_DoNotDirty | REN_ForceNoResetLoaders);
		}

		// Construct new graph with the supplied name
		NewGraph = ConstructObject<UEdGraph>(GraphClass, ParentScope, NAME_None, RF_Transactional);
		bRename = true;
	}
	else
	{
		// Construct a new graph with a default name
		NewGraph = ConstructObject<UEdGraph>(GraphClass, ParentScope, NAME_None, RF_Transactional);
	}

	NewGraph->Schema = SchemaClass;

	// Now move to where we want it to. Workaround to ensure transaction buffer is correctly utilized
	if (bRename)
	{
		NewGraph->Rename(*(GraphName.ToString()), ParentScope, REN_DoNotDirty | REN_ForceNoResetLoaders);
	}
	return NewGraph;
}


void FBlueprintEditorUtils::AddFunctionGraph(UBlueprint* Blueprint, class UEdGraph* Graph, bool bIsUserCreated, UClass* SignatureFromClass)
{
	// Give the schema a chance to fill out any required nodes (like the entry node or results node)
	const UEdGraphSchema* Schema = Graph->GetSchema();
	const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(Graph->GetSchema());

	Schema->CreateDefaultNodesForGraph(*Graph);

	if (K2Schema != NULL)
	{
		K2Schema->CreateFunctionGraphTerminators(*Graph, SignatureFromClass);

		if (bIsUserCreated)
		{
			// We need to flag the entry node to make sure that the compiled function is callable from Kismet2
			K2Schema->AddExtraFunctionFlags(Graph, (FUNC_BlueprintCallable|FUNC_BlueprintEvent|FUNC_Public));
			K2Schema->MarkFunctionEntryAsEditable(Graph, true);
		}
	}

	Blueprint->FunctionGraphs.Add(Graph);

	// Potentially adjust variable names for any child blueprints
	ValidateBlueprintChildVariables(Blueprint, Graph->GetFName());

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}

UFunction* FBlueprintEditorUtils::FindFunctionInImplementedInterfaces(const UBlueprint* Blueprint, const FName& FunctionName, bool * bOutInvalidInterface )
{
	if(Blueprint)
	{
		TArray<UClass*> InterfaceClasses;
		FindImplementedInterfaces(Blueprint, false, InterfaceClasses);

		if( bOutInvalidInterface )
		{
			*bOutInvalidInterface = false;
		}

		// Now loop through the interface classes and try and find the function
		for (auto It = InterfaceClasses.CreateConstIterator(); It; It++)
		{
			UClass* SearchClass = (*It);
			if( SearchClass )
			{
				if (UFunction* OverridenFunction = SearchClass->FindFunctionByName(FunctionName, EIncludeSuperFlag::ExcludeSuper))
				{
					return OverridenFunction;
				}
			}
			else if( bOutInvalidInterface )
			{
				*bOutInvalidInterface = true;
			}
		}
	}

	return NULL;
}

void FBlueprintEditorUtils::FindImplementedInterfaces(const UBlueprint* Blueprint, bool bGetAllInterfaces, TArray<UClass*>& ImplementedInterfaces)
{
	// First get the ones this blueprint implemented
	for (auto It = Blueprint->ImplementedInterfaces.CreateConstIterator(); It; It++)
	{
		ImplementedInterfaces.AddUnique((*It).Interface);
	}

	if (bGetAllInterfaces)
	{
		// Now get all the ones the blueprint's parents implemented
		UClass* BlueprintParent =  Blueprint->ParentClass;
		while (BlueprintParent)
		{
			for (auto It = BlueprintParent->Interfaces.CreateConstIterator(); It; It++)
			{
				ImplementedInterfaces.AddUnique((*It).Class);
			}
			BlueprintParent = BlueprintParent->GetSuperClass();
		}
	}
}

void FBlueprintEditorUtils::AddMacroGraph( UBlueprint* Blueprint, class UEdGraph* Graph, bool bIsUserCreated, UClass* SignatureFromClass )
{
	// Give the schema a chance to fill out any required nodes (like the entry node or results node)
	const UEdGraphSchema* Schema = Graph->GetSchema();
	const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(Graph->GetSchema());

	Schema->CreateDefaultNodesForGraph(*Graph);

	if (K2Schema != NULL)
	{
		K2Schema->CreateMacroGraphTerminators(*Graph, SignatureFromClass);

		if (bIsUserCreated)
		{
			// We need to flag the entry node to make sure that the compiled function is callable from Kismet2
			K2Schema->AddExtraFunctionFlags(Graph, (FUNC_BlueprintCallable|FUNC_BlueprintEvent));
			K2Schema->MarkFunctionEntryAsEditable(Graph, true);
		}
	}

	// Mark the graph as public if it's going to be referenced directly from other blueprints
	if (Blueprint->BlueprintType == BPTYPE_MacroLibrary)
	{
		Graph->SetFlags(RF_Public);
	}

	Blueprint->MacroGraphs.Add(Graph);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}

void FBlueprintEditorUtils::AddInterfaceGraph(UBlueprint* Blueprint, class UEdGraph* Graph, UClass* InterfaceClass)
{
	const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(Graph->GetSchema());
	if (K2Schema != NULL)
	{
		K2Schema->CreateFunctionGraphTerminators(*Graph, InterfaceClass);
	}
}

void FBlueprintEditorUtils::AddUbergraphPage(UBlueprint* Blueprint, class UEdGraph* Graph)
{
#if WITH_EDITORONLY_DATA
	Blueprint->UbergraphPages.Add(Graph);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
#endif	//#if WITH_EDITORONLY_DATA
}

void FBlueprintEditorUtils::AddDomainSpecificGraph(UBlueprint* Blueprint, class UEdGraph* Graph)
{
	// Give the schema a chance to fill out any required nodes (like the entry node or results node)
	const UEdGraphSchema* Schema = Graph->GetSchema();
	Schema->CreateDefaultNodesForGraph(*Graph);

	check(Blueprint->BlueprintType != BPTYPE_MacroLibrary);

#if WITH_EDITORONLY_DATA
	Blueprint->FunctionGraphs.Add(Graph);
#endif	//#if WITH_EDITORONLY_DATA
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}

// Remove the supplied set of graphs from the Blueprint.
void FBlueprintEditorUtils::RemoveGraphs( UBlueprint* Blueprint, const TArray<class UEdGraph*>& GraphsToRemove )
{
	for (int32 ItemIndex=0; ItemIndex < GraphsToRemove.Num(); ++ItemIndex)
	{
		UEdGraph* Graph = GraphsToRemove[ItemIndex];
		FBlueprintEditorUtils::RemoveGraph(Blueprint, Graph, EGraphRemoveFlags::MarkTransient);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}

// Removes the supplied graph from the Blueprint.
void FBlueprintEditorUtils::RemoveGraph(UBlueprint* Blueprint, class UEdGraph* GraphToRemove, EGraphRemoveFlags::Type Flags /*= Transient | Recompile */)
{
	struct Local
	{
		static bool IsASubGraph(UEdGraph* Graph)
		{
			UObject* Outer = Graph->GetOuter();
			return ( Outer && Outer->IsA( UK2Node_Composite::StaticClass() ) );
		}
	};

	GraphToRemove->Modify();

	for (UObject* TestOuter = GraphToRemove->GetOuter(); TestOuter; TestOuter = TestOuter->GetOuter())
	{
		if (TestOuter == Blueprint)
		{
			Blueprint->DelegateSignatureGraphs.Remove( GraphToRemove );
			Blueprint->FunctionGraphs.Remove( GraphToRemove );
			Blueprint->UbergraphPages.Remove( GraphToRemove );
			if(Blueprint->MacroGraphs.Remove( GraphToRemove ) > 0 ) 
			{
				//removes all macro nodes using this macro graph
				TArray<UK2Node_MacroInstance*> MacroNodes;
				FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, MacroNodes);
				for(auto It(MacroNodes.CreateIterator());It;++It)
				{
					UK2Node_MacroInstance* Node = *It;
					if(Node->GetMacroGraph() == GraphToRemove)
					{
						FBlueprintEditorUtils::RemoveNode(Blueprint, Node);
					}
				}
			}

			for( auto InterfaceIt = Blueprint->ImplementedInterfaces.CreateIterator(); InterfaceIt; ++InterfaceIt )
			{
				FBPInterfaceDescription& CurrInterface = *InterfaceIt;
				CurrInterface.Graphs.Remove( GraphToRemove );
			}
		}
		else if (UEdGraph* OuterGraph = Cast<UEdGraph>(TestOuter))
		{
			// remove ourselves
			OuterGraph->SubGraphs.Remove(GraphToRemove);
		}
		else if (! (Cast<UK2Node_Composite>(TestOuter)	|| 
					Cast<UAnimStateNodeBase>(TestOuter)	||
					Cast<UAnimStateTransitionNode>(TestOuter)	||
					Cast<UAnimGraphNode_StateMachineBase>(TestOuter)) )
		{
			break;
		}
	}

	// Remove timelines held in the graph
	TArray<UK2Node_Timeline*> AllTimelineNodes;
	GraphToRemove->GetNodesOfClass<UK2Node_Timeline>(AllTimelineNodes);
	for (auto TimelineIt = AllTimelineNodes.CreateIterator(); TimelineIt; ++TimelineIt)
	{
		UK2Node_Timeline* TimelineNode = *TimelineIt;
		TimelineNode->DestroyNode();
	}

	// Handle subgraphs held in graph
	TArray<UK2Node_Composite*> AllCompositeNodes;
	GraphToRemove->GetNodesOfClass<UK2Node_Composite>(AllCompositeNodes);

	const bool bDontRecompile = true;
	for (auto CompIt = AllCompositeNodes.CreateIterator(); CompIt; ++CompIt)
	{
		UK2Node_Composite* CompNode = *CompIt;
		if (CompNode->BoundGraph && Local::IsASubGraph(CompNode->BoundGraph))
		{
			FBlueprintEditorUtils::RemoveGraph(Blueprint, CompNode->BoundGraph, EGraphRemoveFlags::None);
		}
	}

	GraphToRemove->GetSchema()->HandleGraphBeingDeleted(*GraphToRemove);

	GraphToRemove->Rename(NULL, Blueprint->GetOuter(), REN_DoNotDirty | REN_DontCreateRedirectors);
	GraphToRemove->ClearFlags(RF_Standalone | RF_RootSet | RF_Public);

	if (Flags & EGraphRemoveFlags::MarkTransient)
	{
		GraphToRemove->SetFlags(RF_Transient);
	}

	GraphToRemove->MarkPendingKill();

	if (Flags & EGraphRemoveFlags::Recompile )
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
}

/** Rename a graph and mark objects for modified */
void FBlueprintEditorUtils::RenameGraph(UEdGraph* Graph, const FString& NewNameStr)
{
	const FName NewName(*NewNameStr);
	if (Graph->Rename(*NewNameStr, Graph->GetOuter(), REN_Test))
	{
		// Cache old name
		FName OldGraphName = Graph->GetFName();
		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(Graph);

		// Ensure we have undo records
		Graph->Modify();
		Graph->Rename(*NewNameStr, Graph->GetOuter(), (Blueprint->bIsRegeneratingOnLoad ? REN_ForceNoResetLoaders : 0) | REN_DontCreateRedirectors);

		// Clean function entry node if it exists
		for (auto NodeIt = Graph->Nodes.CreateIterator(); NodeIt; ++NodeIt)
		{
			UEdGraphNode* Node = *NodeIt;
			if (UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node))
			{
				if (EntryNode->SignatureName == OldGraphName)
				{
					EntryNode->Modify();
					EntryNode->SignatureName = NewName;
					break;
				}
				else if (EntryNode->CustomGeneratedFunctionName == OldGraphName)
				{
					EntryNode->Modify();
					EntryNode->CustomGeneratedFunctionName = NewName;
					break;
				}
			}
		}

		// Rename any function call points
		TArray<UK2Node_CallFunction*> AllFunctionCalls;
		FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_CallFunction>(Blueprint, AllFunctionCalls);
	
		for (auto FuncIt = AllFunctionCalls.CreateIterator(); FuncIt; ++FuncIt)
		{
			UK2Node_CallFunction* FunctionNode = *FuncIt;

			if (FunctionNode->FunctionReference.IsSelfContext() && (FunctionNode->FunctionReference.GetMemberName() == OldGraphName))
			{
				FunctionNode->Modify();
				FunctionNode->FunctionReference.SetSelfMember(NewName);
			}
		}

		// Potentially adjust variable names for any child blueprints
		ValidateBlueprintChildVariables(Blueprint, Graph->GetFName());

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
}

void FBlueprintEditorUtils::RenameGraphWithSuggestion(class UEdGraph* Graph, TSharedPtr<class INameValidatorInterface> NameValidator, const FString& DesiredName )
{
	FString NewName = DesiredName;
	NameValidator->FindValidString(NewName);
	UBlueprint* BP = FBlueprintEditorUtils::FindBlueprintForGraphChecked(Graph);
	Graph->Rename(*NewName, Graph->GetOuter(), (BP->bIsRegeneratingOnLoad ? REN_ForceNoResetLoaders : 0) | REN_DontCreateRedirectors);
}

/** 
 * Cleans up a Node in the blueprint
 */
void FBlueprintEditorUtils::RemoveNode(UBlueprint* Blueprint, UEdGraphNode* Node, bool bDontRecompile)
{
	check(Node);

	const UEdGraphSchema* Schema = NULL;

	// Ensure we mark parent graph modified
	if (UEdGraph* GraphObj = Node->GetGraph())
	{
		GraphObj->Modify();
		Schema = GraphObj->GetSchema();
	}

	if (Blueprint != NULL)
	{
		// Remove any breakpoints set on the node
		if (UBreakpoint* Breakpoint = FKismetDebugUtilities::FindBreakpointForNode(Blueprint, Node))
		{
			FKismetDebugUtilities::StartDeletingBreakpoint(Breakpoint, Blueprint);
		}

		// Remove any watches set on the node's pins
		for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
		{
			FKismetDebugUtilities::RemovePinWatch(Blueprint, Node->Pins[PinIndex]);
		}
	}

	Node->Modify();

	// Timelines will be removed from the blueprint if the node is a UK2Node_Timeline
	if (Schema)
	{
		Schema->BreakNodeLinks(*Node);
	}

	Node->DestroyNode(); 

	if (!bDontRecompile && (Blueprint != NULL))
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
}

UEdGraph* FBlueprintEditorUtils::GetTopLevelGraph(UEdGraph* InGraph)
{
	UEdGraph* GraphToTest = const_cast<UEdGraph*>(InGraph);

	for (UObject* TestOuter = GraphToTest; TestOuter; TestOuter = TestOuter->GetOuter())
	{
		// reached up to the blueprint for the graph
		if (UBlueprint* Blueprint = Cast<UBlueprint>(TestOuter))
		{
			break;
		}
		else
		{
			GraphToTest = Cast<UEdGraph>(TestOuter);
		}
	}
	return GraphToTest;
}

UK2Node_Event* FBlueprintEditorUtils::FindOverrideForFunction(const UBlueprint* Blueprint, const UClass* SignatureClass, FName SignatureName)
{
	TArray<UK2Node_Event*> AllEvents;
	FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Event>(Blueprint, AllEvents);

	for(int32 i=0; i<AllEvents.Num(); i++)
	{
		UK2Node_Event* EventNode = AllEvents[i];
		check(EventNode);
		if(	EventNode->bOverrideFunction == true &&
			EventNode->EventSignatureClass == SignatureClass &&
			EventNode->EventSignatureName == SignatureName )
		{
			return EventNode;
		}
	}

	return NULL;
}

bool FBlueprintEditorUtils::IsBlueprintDependentOn(UBlueprint const* Blueprint, UBlueprint const* TestBlueprint)
{
	if ( Blueprint && (Blueprint != TestBlueprint) )
	{
		// is TestBlueprint an interface we implement?
		for (int32 BPIdx=0; BPIdx<Blueprint->ImplementedInterfaces.Num(); BPIdx++)
		{
			FBPInterfaceDescription const& InterfaceDesc = Blueprint->ImplementedInterfaces[BPIdx];
			if ( (TestBlueprint->GeneratedClass == InterfaceDesc.Interface) && (TestBlueprint->GeneratedClass != NULL) )
			{
				return true;
			}
		}

		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		for (auto GraphIt = Graphs.CreateIterator(); GraphIt; ++GraphIt)
		{
			UEdGraph* const Graph = (*GraphIt);
			if (!IsGraphIntermediate(Graph))
			{
				// do we have any function calls that ref BP's class or subclasses?
				TArray<UK2Node_CallFunction*> CallFunctionNodes;
				Graph->GetNodesOfClass(CallFunctionNodes);
				for (auto NodeIt = CallFunctionNodes.CreateIterator(); NodeIt; ++NodeIt)
				{
					UK2Node_CallFunction* const Node = *NodeIt;
					if (Node)
					{
						UClass* const FuncClass = Node->FunctionReference.GetMemberParentClass(Node);
						if (FuncClass->IsChildOf(TestBlueprint->GeneratedClass))
						{
							return true;
						}
					}

					UK2Node_CallFunctionOnMember* const OnMemberNode = Cast<UK2Node_CallFunctionOnMember>(Node);
					if (OnMemberNode)
					{
						UClass* const MemberFuncClass = OnMemberNode->MemberVariableToCallOn.GetMemberParentClass(OnMemberNode);
						if (MemberFuncClass->IsChildOf(TestBlueprint->GeneratedClass))
						{
							return true;
						}
					}
				}

				// do we have any variable refs to objects of BP's class subclasses?
				TArray<UK2Node_Variable*> VarNodes;
				Graph->GetNodesOfClass(VarNodes);
				for (auto NodeIt = VarNodes.CreateIterator(); NodeIt; ++NodeIt)
				{
					UK2Node_Variable* const Node = *NodeIt;
					UClass* const VarClass = Node->VariableReference.GetMemberParentClass(Node);

					if (VarClass->IsChildOf(TestBlueprint->GeneratedClass))
					{
						return true;
					}
				}

				// do we have any delegates to objects of BP's class subclasses?
				TArray<UK2Node_BaseMCDelegate*> DelNodes;
				Graph->GetNodesOfClass(DelNodes);
				for (auto NodeIt = DelNodes.CreateIterator(); NodeIt; ++NodeIt)
				{
					UK2Node_BaseMCDelegate* const Node = *NodeIt;
					UClass* const DelClass = Node->DelegateReference.GetMemberParentClass(Node);

					if (DelClass->IsChildOf(TestBlueprint->GeneratedClass))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}


void FBlueprintEditorUtils::GetDependentBlueprints(UBlueprint* Blueprint, TArray<UBlueprint*>& DependentBlueprints)
{
	DependentBlueprints.Empty();

	//@TODO PRETEST MERGE:  Swap this when main is merged to pretest!
#if 0
 	TArray<UObject*> AllBlueprints;
 	bool const bIncludeDerivedClasses = true;
 	GetObjectsOfClass(UBlueprint::StaticClass(), AllBlueprints, bIncludeDerivedClasses );

	for ( auto ObjIt = AllBlueprints.CreateConstIterator(); ObjIt; ++ObjIt )
#else
	// Use old school slower iterator until fast object collection makes it to pretest
	for( TObjectIterator<UBlueprint> ObjIt; ObjIt; ++ObjIt )
#endif
	{
		// we know the class is correct so a fast cast is ok here
		UBlueprint* const TestBP = (UBlueprint*) *ObjIt;
		if ( FBlueprintEditorUtils::IsBlueprintDependentOn(TestBP, Blueprint))
		{
			DependentBlueprints.Add(TestBP);
		}
	}
}


bool FBlueprintEditorUtils::IsGraphIntermediate(const UEdGraph* Graph)
{
	if (Graph)
	{
		return Graph->HasAllFlags(RF_Transient);
	}
	return false;
}

bool FBlueprintEditorUtils::IsDataOnlyBlueprint(const UBlueprint* Blueprint)
{
	// Blueprint interfaces are always compiled
	if (Blueprint->BlueprintType == BPTYPE_Interface)
	{
		return false;
	}

	// No new variables defined
	if (Blueprint->NewVariables.Num() > 0)
	{
		return false;
	}
	
	// No extra functions, other than the user construction script
	if ((Blueprint->FunctionGraphs.Num() > 1) || (Blueprint->MacroGraphs.Num() > 0))
	{
		return false;
	}

	if (Blueprint->DelegateSignatureGraphs.Num())
	{
		return false;
	}

	if (Blueprint->ComponentTemplates.Num() || Blueprint->Timelines.Num())
	{
		return false;
	}

	if(USimpleConstructionScript* SimpleConstructionScript = Blueprint->SimpleConstructionScript)
	{
		auto Nodes = SimpleConstructionScript->GetAllNodes();
		if (Nodes.Num() > 1)
		{
			return false;
		}
		if ((1 == Nodes.Num()) && (Nodes[0] != SimpleConstructionScript->GetDefaultSceneRootNode()))
		{
			return false;
		}
	}

	// Make sure there's nothing in the user construction script, other than an entry node
	UEdGraph* UserConstructionScript = (Blueprint->FunctionGraphs.Num() == 1) ? Blueprint->FunctionGraphs[0] : NULL;
	if (UserConstructionScript)
	{
		//Call parent construction script may be added automatically
		if (UserConstructionScript->Nodes.Num() > 1)
		{
			return false;
		}
	}

	// No extra eventgraphs
	if( Blueprint->UbergraphPages.Num() > 1 )
	{
		return false;
	}

	// EventGraph is empty
	UEdGraph* EventGraph = (Blueprint->UbergraphPages.Num() == 1) ? Blueprint->UbergraphPages[0] : NULL;
	if( EventGraph && EventGraph->Nodes.Num() > 0 )
	{
		return false;
	}

	// No implemented interfaces
	if( Blueprint->ImplementedInterfaces.Num() > 0 )
	{
		return false;
	}

	return true;
}

bool FBlueprintEditorUtils::IsBlueprintConst(const UBlueprint* Blueprint)
{
	// Macros aren't marked as const because they can modify variables when instanced into a non const class
	// and will be caught at compile time if they're modifying variables on a const class.
	return Blueprint->BlueprintType == BPTYPE_Const;
}

bool FBlueprintEditorUtils::IsActorBased(const UBlueprint* Blueprint)
{
	return Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf(AActor::StaticClass());
}

bool FBlueprintEditorUtils::IsDelegateSignatureGraph(const UEdGraph* Graph)
{
	if(Graph)
	{
		if(const UBlueprint* Blueprint = FindBlueprintForGraph(Graph))
		{
			return (NULL != Blueprint->DelegateSignatureGraphs.FindByKey(Graph));
		}

	}
	return false;
}

bool FBlueprintEditorUtils::IsInterfaceBlueprint(const UBlueprint* Blueprint)
{
	return (Blueprint->BlueprintType == BPTYPE_Interface);
}

bool FBlueprintEditorUtils::IsLevelScriptBlueprint(const UBlueprint* Blueprint)
{
	return (Blueprint->BlueprintType == BPTYPE_LevelScript);
}

bool FBlueprintEditorUtils::SupportsConstructionScript(const UBlueprint* Blueprint)
{
	return(	!FBlueprintEditorUtils::IsInterfaceBlueprint(Blueprint) && 
			!FBlueprintEditorUtils::IsBlueprintConst(Blueprint) && 
			!FBlueprintEditorUtils::IsLevelScriptBlueprint(Blueprint) && 
			FBlueprintEditorUtils::IsActorBased(Blueprint)) &&
			!(Blueprint->BlueprintType == BPTYPE_MacroLibrary);
}

UEdGraph* FBlueprintEditorUtils::FindUserConstructionScript(const UBlueprint* Blueprint)
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	for( auto GraphIt = Blueprint->FunctionGraphs.CreateConstIterator(); GraphIt; ++GraphIt )
	{
		UEdGraph* CurrentGraph = *GraphIt;
		if( CurrentGraph->GetFName() == Schema->FN_UserConstructionScript )
		{
			return CurrentGraph;
		}
	}

	return NULL;
}

UEdGraph* FBlueprintEditorUtils::FindEventGraph(const UBlueprint* Blueprint)
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	for( auto GraphIt = Blueprint->UbergraphPages.CreateConstIterator(); GraphIt; ++GraphIt )
	{
		UEdGraph* CurrentGraph = *GraphIt;
		if( CurrentGraph->GetFName() == Schema->GN_EventGraph )
		{
			return CurrentGraph;
		}
	}

	return NULL;
}

bool FBlueprintEditorUtils::DoesBlueprintDeriveFrom(const UBlueprint* Blueprint, UClass* TestClass)
{
	check(Blueprint->SkeletonGeneratedClass != NULL);
	return	TestClass != NULL && 
		Blueprint->SkeletonGeneratedClass->IsChildOf(TestClass);
}

bool FBlueprintEditorUtils::DoesBlueprintContainField(const UBlueprint* Blueprint, UField* TestField)
{
	// Get the class of the field
	if(TestField)
	{
		UClass* TestClass = CastChecked<UClass>(TestField->GetOuter());
		return FBlueprintEditorUtils::DoesBlueprintDeriveFrom(Blueprint, TestClass);
	}
	return false;
}

bool FBlueprintEditorUtils::DoesSupportOverridingFunctions(const UBlueprint* Blueprint)
{
	return Blueprint->BlueprintType != BPTYPE_MacroLibrary 
		&& Blueprint->BlueprintType != BPTYPE_Interface;
}

bool FBlueprintEditorUtils::DoesSupportTimelines(const UBlueprint* Blueprint)
{
	// Right now, just assume actor based blueprints support timelines
	return FBlueprintEditorUtils::IsActorBased(Blueprint) && FBlueprintEditorUtils::DoesSupportEventGraphs(Blueprint);
}

bool FBlueprintEditorUtils::DoesSupportEventGraphs(const UBlueprint* Blueprint)
{
	return Blueprint->BlueprintType == BPTYPE_Normal 
		|| Blueprint->BlueprintType == BPTYPE_LevelScript;
}

/** Returns whether or not the blueprint supports implementing interfaces */
bool FBlueprintEditorUtils::DoesSupportImplementingInterfaces(const UBlueprint* Blueprint)
{
	return Blueprint->BlueprintType != BPTYPE_MacroLibrary 
		&& Blueprint->BlueprintType != BPTYPE_Interface 
		&& Blueprint->BlueprintType != BPTYPE_LevelScript;
}

// Returns a descriptive name of the type of blueprint passed in
FString FBlueprintEditorUtils::GetBlueprintTypeDescription(const UBlueprint* Blueprint)
{
	FString BlueprintTypeString;
	switch (Blueprint->BlueprintType)
	{
	case BPTYPE_LevelScript:
		BlueprintTypeString = LOCTEXT("BlueprintType_LevelScript", "Level Blueprint").ToString();
		break;
	case BPTYPE_MacroLibrary:
		BlueprintTypeString = LOCTEXT("BlueprintType_MacroLibrary", "Macro Library").ToString();
		break;
	case BPTYPE_Interface:
		BlueprintTypeString = LOCTEXT("BlueprintType_Interface", "Interface").ToString();
		break;
	case BPTYPE_Normal:
	case BPTYPE_Const:
		BlueprintTypeString = Blueprint->GetClass()->GetName();
		break;
	default:
		BlueprintTypeString = TEXT("Unknown blueprint type");
	}

	return BlueprintTypeString;
}

//////////////////////////////////////////////////////////////////////////
// Variables


// Find the index of a variable first declared in this blueprint. Returns INDEX_NONE if not found.
int32 FBlueprintEditorUtils::FindNewVariableIndex(const UBlueprint* Blueprint, const FName& InName) 
{
	if(InName != NAME_None)
	{
		for(int32 i=0; i<Blueprint->NewVariables.Num(); i++)
		{
			if(Blueprint->NewVariables[i].VarName == InName)
			{
				return i;
			}
		}
	}

	return INDEX_NONE;
}

bool FBlueprintEditorUtils::MoveVariableBeforeVariable(UBlueprint* Blueprint, FName VarNameToMove, FName TargetVarName, bool bDontRecompile)
{
	bool bMoved = false;
	int32 VarIndexToMove = FindNewVariableIndex(Blueprint, VarNameToMove);
	int32 TargetVarIndex = FindNewVariableIndex(Blueprint, TargetVarName);
	if(VarIndexToMove != INDEX_NONE && TargetVarIndex != INDEX_NONE)
	{
		// Copy var we want to move
		FBPVariableDescription MoveVar = Blueprint->NewVariables[VarIndexToMove];
		// When we remove item, will back all items after it. If your target is after it, need to adjust
		if(TargetVarIndex > VarIndexToMove)
		{
			TargetVarIndex--;
		}
		// Remove var we are moving
		Blueprint->NewVariables.RemoveAt(VarIndexToMove);
		// Add in before target variable
		Blueprint->NewVariables.Insert(MoveVar, TargetVarIndex);

		if(!bDontRecompile)
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		}
		bMoved = true;
	}
	return bMoved;
}

int32 FBlueprintEditorUtils::FindFirstNewVarOfCategory(const UBlueprint* Blueprint, FName Category)
{
	for(int32 VarIdx=0; VarIdx<Blueprint->NewVariables.Num(); VarIdx++)
	{
		if(Blueprint->NewVariables[VarIdx].Category == Category)
		{
			return VarIdx;
		}
	}
	return INDEX_NONE;
}


bool FBlueprintEditorUtils::MoveCategoryBeforeCategory(UBlueprint* Blueprint, FName CategoryToMove, FName TargetCategory)
{
	// Do nothing if moving to yourself, or no category
	if(CategoryToMove == TargetCategory || CategoryToMove == NAME_None)
	{
		return false;
	}

	// Before we do anything, check that there is a variable of the target category
	if(FindFirstNewVarOfCategory(Blueprint, TargetCategory) == INDEX_NONE)
	{
		return false;
	}

	// Move all vars of the category to move into temp array
	TArray<FBPVariableDescription> VarsToMove;
	for(int32 VarIdx = Blueprint->NewVariables.Num()-1; VarIdx >= 0; VarIdx--)
	{
		if(Blueprint->NewVariables[VarIdx].Category == CategoryToMove)
		{
			VarsToMove.Insert(Blueprint->NewVariables[VarIdx], 0);
			Blueprint->NewVariables.RemoveAt(VarIdx);
		}
	}

	// Bail if nothing to move
	if(VarsToMove.Num() == 0)
	{
		return false;
	}

	// Find insertion point
	int32 InsertionIndex = FindFirstNewVarOfCategory(Blueprint, TargetCategory);

	// Now insert the nodes we want to move at that point
	check(InsertionIndex != INDEX_NONE);
	for(int32 VarIdx=0; VarIdx<VarsToMove.Num(); VarIdx++)
	{
		Blueprint->NewVariables.Insert(VarsToMove[VarIdx], InsertionIndex + VarIdx);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	return true;
}

int32 FBlueprintEditorUtils::FindTimelineIndex(const UBlueprint* Blueprint, const FName& InName) 
{
	const FName TimelineTemplateName = *UTimelineTemplate::TimelineVariableNameToTemplateName(InName);
	for(int32 i=0; i<Blueprint->Timelines.Num(); i++)
	{
		if(Blueprint->Timelines[i]->GetFName() == TimelineTemplateName)
		{
			return i;
		}
	}

	return INDEX_NONE;
}

void FBlueprintEditorUtils::GetSCSVariableNameList(const UBlueprint* Blueprint, TArray<FName>& VariableNames)
{
	if(Blueprint != NULL && Blueprint->SimpleConstructionScript != NULL)
	{
		TArray<USCS_Node*> SCSNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
		for(int32 NodeIndex = 0; NodeIndex < SCSNodes.Num(); ++NodeIndex)
		{
			USCS_Node* SCS_Node = SCSNodes[NodeIndex];
			if(SCS_Node != NULL)
			{
				FName VariableName = SCS_Node->GetVariableName();
				if(VariableName != NAME_None)
				{
					VariableNames.AddUnique(VariableName);
				}
			}
		}
	}
}

int32 FBlueprintEditorUtils::FindSCS_Node(const UBlueprint* Blueprint, const FName& InName) 
{
	if (Blueprint->SimpleConstructionScript)
	{
		TArray<USCS_Node*> AllSCS_Nodes = Blueprint->SimpleConstructionScript->GetAllNodes();
	
		for(int32 i=0; i<AllSCS_Nodes.Num(); i++)
		{
			if(AllSCS_Nodes[i]->VariableName == InName)
			{
				return i;
			}
		}
	}

	return INDEX_NONE;
}

void FBlueprintEditorUtils::SetBlueprintOnlyEditableFlag(UBlueprint* Blueprint, const FName& VarName, const bool bNewBlueprintOnly)
{
	const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName);

	if (bNewBlueprintOnly)
	{
		FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(Blueprint, VarName, FEdMode::MD_MakeEditWidget);
	}

	if (VarIndex != INDEX_NONE)
	{
		if( bNewBlueprintOnly )
		{
			Blueprint->NewVariables[VarIndex].PropertyFlags |= CPF_DisableEditOnInstance;
		}
		else
		{
			Blueprint->NewVariables[VarIndex].PropertyFlags &= ~CPF_DisableEditOnInstance;
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}

void FBlueprintEditorUtils::SetInterpFlag(UBlueprint* Blueprint, const FName& VarName, const bool bInterp)
{
	const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName);
	if (VarIndex != INDEX_NONE)
	{
		if( bInterp )
		{
			Blueprint->NewVariables[VarIndex].PropertyFlags |= (CPF_Interp);
		}
		else
		{
			Blueprint->NewVariables[VarIndex].PropertyFlags &= ~(CPF_Interp);
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}

void FBlueprintEditorUtils::SetVariableTransientFlag(UBlueprint* InBlueprint, const FName& InVarName, const bool bInIsTransient)
{
	const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(InBlueprint, InVarName);

	if (VarIndex != INDEX_NONE)
	{
		if( bInIsTransient )
		{
			InBlueprint->NewVariables[VarIndex].PropertyFlags |= CPF_Transient;
		}
		else
		{
			InBlueprint->NewVariables[VarIndex].PropertyFlags &= ~CPF_Transient;
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(InBlueprint);
}

void FBlueprintEditorUtils::SetVariableSaveGameFlag(UBlueprint* InBlueprint, const FName& InVarName, const bool bInIsSaveGame)
{
	const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(InBlueprint, InVarName);

	if (VarIndex != INDEX_NONE)
	{
		if( bInIsSaveGame )
		{
			InBlueprint->NewVariables[VarIndex].PropertyFlags |= CPF_SaveGame;
		}
		else
		{
			InBlueprint->NewVariables[VarIndex].PropertyFlags &= ~CPF_SaveGame;
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(InBlueprint);
}

void FBlueprintEditorUtils::SetBlueprintVariableMetaData(UBlueprint* Blueprint, const FName& VarName, const FName& MetaDataKey, const FString& MetaDataValue)
{
	const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName);
	if (VarIndex == INDEX_NONE)
	{
		//Not a NewVariable is the VarName from a Timeline?
		const int32 TimelineIndex = FBlueprintEditorUtils::FindTimelineIndex(Blueprint,VarName);

		if (TimelineIndex == INDEX_NONE)
		{
			//Not a Timeline is this a SCS Node?
			const int32 SCS_NodeIndex = FBlueprintEditorUtils::FindSCS_Node(Blueprint,VarName);

			if (SCS_NodeIndex != INDEX_NONE)
			{
				Blueprint->SimpleConstructionScript->GetAllNodes()[SCS_NodeIndex]->SetMetaData(MetaDataKey, MetaDataValue);
			}

		}
		else
		{
			Blueprint->Timelines[TimelineIndex]->SetMetaData(MetaDataKey, MetaDataValue);
		}
	}
	else
	{
		Blueprint->NewVariables[VarIndex].SetMetaData(MetaDataKey, MetaDataValue);
		UProperty* Property = FindField<UProperty>(Blueprint->SkeletonGeneratedClass, VarName);
		Property->SetMetaData(MetaDataKey, *MetaDataValue);
	}
}

bool FBlueprintEditorUtils::GetBlueprintVariableMetaData(UBlueprint* Blueprint, const FName& VarName, const FName& MetaDataKey, FString& OutMetaDataValue)
{
	const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName);
	if (VarIndex == INDEX_NONE)
	{
		//Not a NewVariable is the VarName from a Timeline?
		const int32 TimelineIndex = FBlueprintEditorUtils::FindTimelineIndex(Blueprint,VarName);

		if (TimelineIndex == INDEX_NONE)
		{
			//Not a Timeline is this a SCS Node?
			const int32 SCS_NodeIndex = FBlueprintEditorUtils::FindSCS_Node(Blueprint,VarName);

			if (SCS_NodeIndex != INDEX_NONE)
			{
				USCS_Node& Desc = *Blueprint->SimpleConstructionScript->GetAllNodes()[SCS_NodeIndex];

				int32 EntryIndex = Desc.FindMetaDataEntryIndexForKey(MetaDataKey);
				if (EntryIndex != INDEX_NONE)
				{
					OutMetaDataValue = Desc.GetMetaData(MetaDataKey);
					return true;
				}
			}
		}
		else
		{
			UTimelineTemplate& Desc = *Blueprint->Timelines[TimelineIndex];

			int32 EntryIndex = Desc.FindMetaDataEntryIndexForKey(MetaDataKey);
			if (EntryIndex != INDEX_NONE)
			{
				OutMetaDataValue = Desc.GetMetaData(MetaDataKey);
				return true;
			}
		}
	}
	else
	{
		FBPVariableDescription& Desc = Blueprint->NewVariables[VarIndex];

		int32 EntryIndex = Desc.FindMetaDataEntryIndexForKey(MetaDataKey);
		if (EntryIndex != INDEX_NONE)
		{
			OutMetaDataValue = Desc.GetMetaData(MetaDataKey);
			return true;
		}
	}

	OutMetaDataValue.Empty();
	return false;
}

void FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(UBlueprint* Blueprint, const FName& VarName, const FName& MetaDataKey)
{
	const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName);
	if (VarIndex == INDEX_NONE)
	{
		//Not a NewVariable is the VarName from a Timeline?
		const int32 TimelineIndex = FBlueprintEditorUtils::FindTimelineIndex(Blueprint,VarName);

		if (TimelineIndex == INDEX_NONE)
		{
			//Not a Timeline is this a SCS Node?
			const int32 SCS_NodeIndex = FBlueprintEditorUtils::FindSCS_Node(Blueprint,VarName);

			if (SCS_NodeIndex != INDEX_NONE)
			{
				Blueprint->SimpleConstructionScript->GetAllNodes()[SCS_NodeIndex]->RemoveMetaData(MetaDataKey);
			}

		}
		else
		{
			Blueprint->Timelines[TimelineIndex]->RemoveMetaData(MetaDataKey);
		}
	}
	else
	{
		Blueprint->NewVariables[VarIndex].RemoveMetaData(MetaDataKey);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}

void FBlueprintEditorUtils::SetBlueprintVariableCategory(UBlueprint* Blueprint, const FName& VarName, const FName& NewCategory, bool bDontRecompile)
{
	// Ensure we always set a category
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	FName SetCategory = NewCategory;
	if (SetCategory == NAME_None)
	{
		SetCategory = K2Schema->VR_DefaultCategory; 
	}

	UClass* SkeletonGeneratedClass = Blueprint->SkeletonGeneratedClass;
	if (UProperty* TargetProperty = FindField<UProperty>(SkeletonGeneratedClass, VarName))
	{
		UClass* OuterClass = CastChecked<UClass>(TargetProperty->GetOuter());
		const bool bIsNativeVar = (OuterClass->ClassGeneratedBy == NULL);

		if (!bIsNativeVar)
		{
			TargetProperty->SetMetaData(TEXT("Category"), *SetCategory.ToString());
			const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName);
			if (VarIndex != INDEX_NONE)
			{
				Blueprint->NewVariables[VarIndex].Category = SetCategory;
			}
			else
			{
				const int32 SCS_NodeIndex = FBlueprintEditorUtils::FindSCS_Node(Blueprint, VarName);
				if (SCS_NodeIndex != INDEX_NONE)
				{
					Blueprint->SimpleConstructionScript->GetAllNodes()[SCS_NodeIndex]->CategoryName = SetCategory;
				}
			}

			if (bDontRecompile == false)
			{
				FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			}
		}
	}
}

FName FBlueprintEditorUtils::GetBlueprintVariableCategory(UBlueprint* Blueprint, const FName& VarName)
{
	FName CategoryName = NAME_None;
	UClass* SkeletonGeneratedClass = Blueprint->SkeletonGeneratedClass;
	UProperty* TargetProperty = FindField<UProperty>(SkeletonGeneratedClass, VarName);
	if(TargetProperty != NULL)
	{
		CategoryName = FObjectEditorUtils::GetCategoryFName(TargetProperty);
	}

	if(CategoryName == NAME_None && Blueprint->SimpleConstructionScript != NULL)
	{
		// Look for the variable in the SCS (in case the Blueprint has not been compiled yet)
		const int32 SCS_NodeIndex = FBlueprintEditorUtils::FindSCS_Node(Blueprint, VarName);
		if (SCS_NodeIndex != INDEX_NONE)
		{
			return Blueprint->SimpleConstructionScript->GetAllNodes()[SCS_NodeIndex]->CategoryName;
		}
	}

	return CategoryName;
}

uint64* FBlueprintEditorUtils::GetBlueprintVariablePropertyFlags(UBlueprint* Blueprint, const FName& VarName)
{
	const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName);
	if (VarIndex != INDEX_NONE)
	{
		return &Blueprint->NewVariables[VarIndex].PropertyFlags;
	}
	return NULL;
}

FName FBlueprintEditorUtils::GetBlueprintVariableRepNotifyFunc(UBlueprint* Blueprint, const FName& VarName)
{
	const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName);
	if (VarIndex != INDEX_NONE)
	{
		return Blueprint->NewVariables[VarIndex].RepNotifyFunc;
	}
	return NAME_None;
}

void FBlueprintEditorUtils::SetBlueprintVariableRepNotifyFunc(UBlueprint* Blueprint, const FName& VarName, const FName& RepNotifyFunc)
{
	const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName);
	if (VarIndex != INDEX_NONE)
	{
		Blueprint->NewVariables[VarIndex].RepNotifyFunc = RepNotifyFunc;
	}
}


// Gets a list of function names currently in use in the blueprint, based on the skeleton class
void FBlueprintEditorUtils::GetFunctionNameList(const UBlueprint* Blueprint, TArray<FName>& FunctionNames)
{
	if( UClass* SkeletonClass = Blueprint->SkeletonGeneratedClass )
	{
		for( TFieldIterator<UFunction> FuncIt(SkeletonClass, EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt )
		{
			FunctionNames.Add( (*FuncIt)->GetFName() );
		}
	}
}

void FBlueprintEditorUtils::GetDelegateNameList(const UBlueprint* Blueprint, TArray<FName>& FunctionNames)
{
	check(Blueprint);
	for (int32 It = 0; It < Blueprint->DelegateSignatureGraphs.Num(); It++)
	{
		if (UEdGraph* Graph = Blueprint->DelegateSignatureGraphs[It])
		{
			FunctionNames.Add(Graph->GetFName());
		}
	}
}

UEdGraph* FBlueprintEditorUtils::GetDelegateSignatureGraphByName(UBlueprint* Blueprint, FName FunctionName)
{
	if ((NULL != Blueprint) && (FunctionName != NAME_None))
	{
		for (int32 It = 0; It < Blueprint->DelegateSignatureGraphs.Num(); It++)
		{
			if (UEdGraph* Graph = Blueprint->DelegateSignatureGraphs[It])
			{
				if(FunctionName == Graph->GetFName())
				{
					return Graph;
				}
			}
		}
	}
	return NULL;
}

// Gets a list of pins that should hidden for a given function
void FBlueprintEditorUtils::GetHiddenPinsForFunction(UBlueprint const* CallingContext, UFunction const* Function, TSet<FString>& HiddenPins)
{
	check(Function != NULL);
	TMap<FName, FString>* MetaData = UMetaData::GetMapForObject(Function);	
	if (MetaData != NULL)
	{
		for (TMap<FName, FString>::TConstIterator It(*MetaData); It; ++It)
		{
			static const FName NAME_LatentInfo = TEXT("LatentInfo");
			static const FName NAME_HidePin = TEXT("HidePin");

			const FName& Key = It.Key();

			if (Key == NAME_LatentInfo)
			{
				HiddenPins.Add(It.Value());
			}
			else if (Key == NAME_HidePin)
			{
				HiddenPins.Add(It.Value());
			}
			else if(Key == FBlueprintMetadata::MD_ExpandEnumAsExecs)
			{
				HiddenPins.Add(It.Value());
			}
			else if (Key == FBlueprintMetadata::MD_WorldContext)
			{
				bool bHasIntrinsicWorldContext = false;

				if (GEngine && CallingContext && CallingContext->ParentClass)
				{
					bHasIntrinsicWorldContext = CallingContext->ParentClass->GetDefaultObject()->ImplementsGetWorld();
				}

				// if the blueprint has world context that we can lookup with "self", 
				// then we can hide this pin (and default it to self)
				if (bHasIntrinsicWorldContext)
				{
					HiddenPins.Add(It.Value());
				}
			}
		}
	}
}

// Gets the visible class variable list.  This includes both variables introduced here and in all superclasses.
void FBlueprintEditorUtils::GetClassVariableList(const UBlueprint* Blueprint, TArray<FName>& VisibleVariables, bool bIncludePrivateVars) 
{
	// Existing variables in the parent class and above
	check(Blueprint->SkeletonGeneratedClass != NULL);
	for (TFieldIterator<UProperty> PropertyIt(Blueprint->SkeletonGeneratedClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;

		if ((!Property->HasAnyPropertyFlags(CPF_Parm) && (bIncludePrivateVars || Property->HasAllPropertyFlags(CPF_BlueprintVisible))))
		{
			VisibleVariables.Add(Property->GetFName());
		}
	}

	if (bIncludePrivateVars)
	{
		// Include SCS node variable names, timelines, and other member variables that may be pending compilation. Consider them to be "private" as they're not technically accessible for editing just yet.
		TArray<UBlueprint*> ParentBPStack;
		UBlueprint::GetBlueprintHierarchyFromClass(Blueprint->SkeletonGeneratedClass, ParentBPStack);
		for (int32 StackIndex = ParentBPStack.Num() - 1; StackIndex >= 0; --StackIndex)
		{
			UBlueprint* ParentBP = ParentBPStack[StackIndex];
			check(ParentBP != NULL);

			GetSCSVariableNameList(ParentBP, VisibleVariables);

			for(int32 VariableIndex = 0; VariableIndex < ParentBP->NewVariables.Num(); ++VariableIndex)
			{
				VisibleVariables.AddUnique(ParentBP->NewVariables[VariableIndex].VarName);
			}

			for(int32 TimelineIndex = 0; TimelineIndex < ParentBP->Timelines.Num(); ++TimelineIndex)
			{
				VisibleVariables.AddUnique(ParentBP->Timelines[TimelineIndex]->GetFName());
			}
		}
	}

	// "self" is reserved for all classes
	VisibleVariables.Add(NAME_Self);
}

void FBlueprintEditorUtils::GetNewVariablesOfType( const UBlueprint* Blueprint, const FEdGraphPinType& Type, TArray<FName>& OutVars )
{
	for(int32 i=0; i<Blueprint->NewVariables.Num(); i++)
	{
		const FBPVariableDescription& Var = Blueprint->NewVariables[i];
		if(Type == Var.VarType)
		{
			OutVars.Add(Var.VarName);
		}
	}
}

// Adds a member variable to the blueprint.  It cannot mask a variable in any superclass.
bool FBlueprintEditorUtils::AddMemberVariable(UBlueprint* Blueprint, const FName& NewVarName, const FEdGraphPinType& NewVarType, const FString& DefaultValue/* = FString()*/)
{
	// Don't allow vars with empty names
	if(NewVarName == NAME_None)
	{
		return false;
	}

	// First we need to see if there is already a variable with that name, in this blueprint or parent class
	TArray<FName> CurrentVars;
	FBlueprintEditorUtils::GetClassVariableList(Blueprint, CurrentVars);
	if(CurrentVars.Contains(NewVarName))
	{
		return false; // fail
	}

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Now create new variable
	FBPVariableDescription NewVar;

	NewVar.VarName = NewVarName;
	NewVar.VarGuid = FGuid::NewGuid();
	NewVar.FriendlyName = EngineUtils::SanitizeDisplayName( NewVarName.ToString(), (NewVarType.PinCategory == K2Schema->PC_Boolean) ? true : false );
	NewVar.VarType = NewVarType;
	// default new vars to 'kismet read/write' and 'only editable on owning CDO' 
	NewVar.PropertyFlags |= (CPF_Edit | CPF_BlueprintVisible | CPF_DisableEditOnInstance);
	if(NewVarType.PinCategory == K2Schema->PC_MCDelegate)
	{
		NewVar.PropertyFlags |= CPF_BlueprintAssignable | CPF_BlueprintCallable;
	}
	else if (NewVarType.PinCategory == K2Schema->PC_Object)
	{
		// if it's a PC_Object, then it should have an associated UClass object
		check(NewVarType.PinSubCategoryObject.IsValid());
		const UClass* ClassObject = Cast<UClass>(NewVarType.PinSubCategoryObject.Get());
		check(ClassObject != NULL);

		if (ClassObject->IsChildOf(AActor::StaticClass()))
		{
			// prevent Actor variables from having default values (because Blueprint templates are library elements that can 
			// bridge multiple levels and different levels might not have the actor that the default is referencing).
			NewVar.PropertyFlags |= CPF_DisableEditOnTemplate;
		}
	}
	NewVar.Category = K2Schema->VR_DefaultCategory;
	NewVar.DefaultValue = DefaultValue;

	Blueprint->NewVariables.Add(NewVar);

	// Potentially adjust variable names for any child blueprints
	FBlueprintEditorUtils::ValidateBlueprintChildVariables(Blueprint, NewVarName);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	return true;
}

// Removes a member variable if it was declared in this blueprint and not in a base class.
void FBlueprintEditorUtils::RemoveMemberVariable(UBlueprint* Blueprint, const FName& VarName)
{
	const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName);
	if (VarIndex != INDEX_NONE)
	{
		Blueprint->NewVariables.RemoveAt(VarIndex);
		FBlueprintEditorUtils::RemoveVariableNodes(Blueprint, VarName);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
}

void FBlueprintEditorUtils::BulkRemoveMemberVariables(UBlueprint* Blueprint, const TArray<FName>& VarNames)
{
	bool bModified = false;
	for (int32 i = 0; i < VarNames.Num(); ++i)
	{
		const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarNames[i]);
		if (VarIndex != INDEX_NONE)
		{
			Blueprint->NewVariables.RemoveAt(VarIndex);
			FBlueprintEditorUtils::RemoveVariableNodes(Blueprint, VarNames[i]);
			bModified = true;
		}
	}

	if (bModified)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
}

void FBlueprintEditorUtils::RemoveVariableNodes(UBlueprint* Blueprint, const FName& VarName, bool const bForSelfOnly)
{
	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);

	for(TArray<UEdGraph*>::TConstIterator it(AllGraphs); it; ++it)
	{
		const UEdGraph* CurrentGraph = *it;

		TArray<UK2Node_Variable*> GraphNodes;
		CurrentGraph->GetNodesOfClass(GraphNodes);

		for( TArray<UK2Node_Variable*>::TConstIterator NodeIt(GraphNodes); NodeIt; ++NodeIt )
		{
			UK2Node_Variable* CurrentNode = *NodeIt;

			UClass* SelfClass = Blueprint->GeneratedClass;
			UClass* VariableParent = CurrentNode->VariableReference.GetMemberParentClass(SelfClass);

			if ((SelfClass == VariableParent) || !bForSelfOnly)
			{
				if(VarName == CurrentNode->GetVarName())
				{
					CurrentNode->DestroyNode();
				}
			}
		}
	}
}

void FBlueprintEditorUtils::RenameComponentMemberVariable(UBlueprint* Blueprint, USCS_Node* Node, const FName& NewName)
{
	// Should not allow renaming to "none" (UI should prevent this)
	check(NewName != NAME_None);

	if (Node->VariableName != NewName)
	{
		Blueprint->Modify();
		
		// Update the name
		const FName OldName = Node->VariableName;
		Node->Modify();
		Node->VariableName = NewName;

		Node->NameWasModified();

		// Update any existing references to the old name
		if (OldName != NAME_None)
		{
			FBlueprintEditorUtils::ReplaceVariableReferences(Blueprint, OldName, NewName);
		}

		// Validate child blueprints and adjust variable names to avoid a potential name collision
		FBlueprintEditorUtils::ValidateBlueprintChildVariables(Blueprint, NewName);

		// And recompile
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
}

void FBlueprintEditorUtils::RenameMemberVariable(UBlueprint* Blueprint, const FName& OldName, const FName& NewName)
{
	if ((OldName != NewName) && (NewName != NAME_None))
	{
		const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OldName);
		if (VarIndex != INDEX_NONE)
		{
			const FScopedTransaction Transaction( LOCTEXT("RenameVariable", "Rename Variable") );
			Blueprint->Modify();

			// Update the name
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			FBPVariableDescription& Variable = Blueprint->NewVariables[VarIndex];
			Variable.VarName = NewName;
			Variable.FriendlyName = EngineUtils::SanitizeDisplayName( NewName.ToString(), (Variable.VarType.PinCategory == K2Schema->PC_Boolean) ? true : false );

			// Update any existing references to the old name
			FBlueprintEditorUtils::ReplaceVariableReferences(Blueprint, OldName, NewName);

			void* OldPropertyAddr = NULL;
			void* NewPropertyAddr = NULL;

			//Grab property of blueprint's current CDO
			UClass* GeneratedClass = Blueprint->GeneratedClass;
			UObject* GeneratedCDO = GeneratedClass->GetDefaultObject();
			UProperty* TargetProperty = FindField<UProperty>(GeneratedClass, OldName);

			if( TargetProperty )
			{
				// Grab the address of where the property is actually stored (UObject* base, plus the offset defined in the property)
				OldPropertyAddr = TargetProperty->ContainerPtrToValuePtr<void>(GeneratedCDO);
				if(OldPropertyAddr)
				{
					// if there is a property for variable, it means the original default value was already copied, so it can be safely overridden
					Variable.DefaultValue.Empty();
					TargetProperty->ExportTextItem(Variable.DefaultValue, OldPropertyAddr, OldPropertyAddr, NULL, PPF_None);
				}
			}

			// Validate child blueprints and adjust variable names to avoid a potential name collision
			FBlueprintEditorUtils::ValidateBlueprintChildVariables(Blueprint, NewName);

			// And recompile
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

			// Grab the new regenerated property and CDO
			UClass* NewGeneratedClass = Blueprint->GeneratedClass;
			UObject* NewGeneratedCDO = NewGeneratedClass->GetDefaultObject();
			UProperty* NewTargetProperty = FindField<UProperty>(NewGeneratedClass, NewName);

			if( NewTargetProperty )
			{
				// Get the property address of the new CDO
				NewPropertyAddr = NewTargetProperty->ContainerPtrToValuePtr<void>(NewGeneratedCDO);

				if( OldPropertyAddr && NewPropertyAddr )
				{
					// Copy the properties from old to new address.
					NewTargetProperty->CopyCompleteValue(NewPropertyAddr, OldPropertyAddr);
				}
			}
		}
		else
		{
			// Wasn't in the introduced variable list; try to find the associated SCS node
			//@TODO: The SCS-generated variables should be in the variable list and have a link back;
			// As it stands, you cannot do any metadata operations on a SCS variable, and you have to do icky code like the following
			TArray<USCS_Node*> Nodes = Blueprint->SimpleConstructionScript->GetAllNodes();
			for (TArray<USCS_Node*>::TConstIterator NodeIt(Nodes); NodeIt; ++NodeIt)
			{
				USCS_Node* CurrentNode = *NodeIt;
				if (CurrentNode->VariableName == OldName)
				{
					RenameComponentMemberVariable(Blueprint, CurrentNode, NewName);
					break;
				}
			}
		}
	}
}

void FBlueprintEditorUtils::ChangeMemberVariableType(UBlueprint* Blueprint, const FName& VariableName, const FEdGraphPinType& NewPinType)
{
	if (VariableName != NAME_None)
	{
		const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VariableName);
		if (VarIndex != INDEX_NONE)
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			FBPVariableDescription& Variable = Blueprint->NewVariables[VarIndex];
			
			// Update the variable type only if it is different
			if (Variable.VarType != NewPinType)
			{
				const FScopedTransaction Transaction( LOCTEXT("ChangeVariableType", "Change Variable Type") );
				Blueprint->Modify();

				Variable.VarType = NewPinType;

				// Destroy all our nodes, because the pin types could be incorrect now (as will the links)
				RemoveVariableNodes(Blueprint, VariableName);

				if (NewPinType.PinCategory == K2Schema->PC_Object)
				{
					// if it's a PC_Object, then it should have an associated UClass object
					check(NewPinType.PinSubCategoryObject.IsValid());
					const UClass* ClassObject = Cast<UClass>(NewPinType.PinSubCategoryObject.Get());
					check(ClassObject != NULL);

					if (ClassObject->IsChildOf(AActor::StaticClass()))
					{
						// prevent Actor variables from having default values (because Blueprint templates are library elements that can 
						// bridge multiple levels and different levels might not have the actor that the default is referencing).
						Variable.PropertyFlags |= CPF_DisableEditOnTemplate;
					}
					else 
					{
						// clear the disable-default-value flag that might have been present (if this was an AActor variable before)
						Variable.PropertyFlags &= ~(CPF_DisableEditOnTemplate);
					}
				}
				else 
				{
					// clear the disable-default-value flag that might have been present (if this was an AActor variable before)
					Variable.PropertyFlags &= ~(CPF_DisableEditOnTemplate);
				}

				// And recompile
				FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			}
		}
	}
}

void FBlueprintEditorUtils::ReplaceVariableReferences(UBlueprint* Blueprint, const FName& OldName, const FName& NewName)
{
	check((OldName != NAME_None) && (NewName != NAME_None));

	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);

	// Update any graph nodes that reference the old variable name to instead reference the new name
	for (TArray<UEdGraph*>::TConstIterator GraphIt(AllGraphs); GraphIt; ++GraphIt)
	{
		const UEdGraph* CurrentGraph = *GraphIt;

		TArray<UK2Node_Variable*> GraphNodes;
		CurrentGraph->GetNodesOfClass(GraphNodes);

		for (TArray<UK2Node_Variable*>::TConstIterator NodeIt(GraphNodes); NodeIt; ++NodeIt)
		{
			UK2Node_Variable* CurrentNode = *NodeIt;
			if (CurrentNode->VariableReference.IsSelfContext() && OldName == CurrentNode->GetVarName())
			{
				CurrentNode->Modify();
				CurrentNode->VariableReference.SetSelfMember(NewName);

				if (UEdGraphPin* Pin = CurrentNode->FindPin(OldName.ToString()))
				{
					Pin->Modify();
					Pin->PinName = NewName.ToString();
				}
			}
		}
	}
}

void FBlueprintEditorUtils::ReplaceVariableReferences(UBlueprint* Blueprint, const UProperty* OldVariable, const UProperty* NewVariable)
{
	check((OldVariable != NULL) && (NewVariable != NULL));

	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);

	// Update any graph nodes that reference the old variable name to instead reference the new name
	for (TArray<UEdGraph*>::TConstIterator GraphIt(AllGraphs); GraphIt; ++GraphIt)
	{
		const UEdGraph* CurrentGraph = *GraphIt;

		TArray<UK2Node_Variable*> GraphNodes;
		CurrentGraph->GetNodesOfClass(GraphNodes);

		for (TArray<UK2Node_Variable*>::TConstIterator NodeIt(GraphNodes); NodeIt; ++NodeIt)
		{
			UK2Node_Variable* CurrentNode = *NodeIt;
			if (CurrentNode->VariableReference.IsSelfContext() && CurrentNode->GetVarName() == OldVariable->GetFName())
			{
				CurrentNode->Modify();
				CurrentNode->VariableReference.SetSelfMember(NewVariable->GetFName());

				if (UEdGraphPin* Pin = CurrentNode->FindPin(OldVariable->GetFName().ToString()))
				{
					Pin->Modify();
					Pin->PinName = NewVariable->GetFName().ToString();
				}
			}
		}
	}
}

bool FBlueprintEditorUtils::IsVariableComponent(const FBPVariableDescription& Variable)
{
	// Find the variable in the list
	if( Variable.VarType.PinCategory == FString(TEXT("object")) )
	{
		const UClass* VarClass = Cast<const UClass>(Variable.VarType.PinSubCategoryObject.Get());
		return (VarClass && VarClass->HasAnyClassFlags(CLASS_DefaultToInstanced));
	}

	return false;
}

bool FBlueprintEditorUtils::IsVariableUsed(const UBlueprint* Blueprint, const FName& Name )
{
	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);
	for(TArray<UEdGraph*>::TConstIterator it(AllGraphs); it; ++it)
	{
		const UEdGraph* CurrentGraph = *it;

		TArray<UK2Node_Variable*> GraphNodes;
		CurrentGraph->GetNodesOfClass(GraphNodes);

		for( TArray<UK2Node_Variable*>::TConstIterator NodeIt(GraphNodes); NodeIt; ++NodeIt )
		{
			UK2Node_Variable* CurrentNode = *NodeIt;
			if(Name == CurrentNode->GetVarName())
			{
				return true;
			}
		}
	}
	return false;
}

bool FBlueprintEditorUtils::ValidateAllMemberVariables(UBlueprint* InBlueprint, UBlueprint* InParentBlueprint, const FName& InVariableName)
{
	for(int32 VariableIdx = 0; VariableIdx < InBlueprint->NewVariables.Num(); ++VariableIdx)
	{
		if(InBlueprint->NewVariables[VariableIdx].VarName == InVariableName)
		{
			FName NewChildName = FBlueprintEditorUtils::FindUniqueKismetName(InBlueprint, InVariableName.ToString());

			UE_LOG(LogBlueprint, Warning, TEXT("Blueprint %s (child of/implements %s) has a member variable with a conflicting name (%s). Changing to %s."), *InBlueprint->GetName(), *InParentBlueprint->GetName(), *InVariableName.ToString(), *NewChildName.ToString());

			FBlueprintEditorUtils::RenameMemberVariable(InBlueprint, InBlueprint->NewVariables[VariableIdx].VarName, NewChildName);
			return true;
		}
	}

	return false;
}

bool FBlueprintEditorUtils::ValidateAllComponentMemberVariables(UBlueprint* InBlueprint, UBlueprint* InParentBlueprint, const FName& InVariableName)
{
	if(InBlueprint->SimpleConstructionScript != NULL)
	{
		TArray<USCS_Node*> ChildSCSNodes = InBlueprint->SimpleConstructionScript->GetAllNodes();
		for(int32 NodeIndex = 0; NodeIndex < ChildSCSNodes.Num(); ++NodeIndex)
		{
			USCS_Node* SCS_Node = ChildSCSNodes[NodeIndex];
			if(SCS_Node != NULL && SCS_Node->GetVariableName() == InVariableName)
			{
				FName NewChildName = FBlueprintEditorUtils::FindUniqueKismetName(InBlueprint, InVariableName.ToString());

				UE_LOG(LogBlueprint, Warning, TEXT("Blueprint %s (child of/implements %s) has a component variable with a conflicting name (%s). Changing to %s."), *InBlueprint->GetName(), *InParentBlueprint->GetName(), *InVariableName.ToString(), *NewChildName.ToString());

				FBlueprintEditorUtils::RenameComponentMemberVariable(InBlueprint, SCS_Node, NewChildName);
				return true;
			}
		}
	}
	return false;
}

bool FBlueprintEditorUtils::ValidateAllTimelines(UBlueprint* InBlueprint, UBlueprint* InParentBlueprint, const FName& InVariableName)
{
	for (int32 TimelineIndex=0; TimelineIndex < InBlueprint->Timelines.Num(); ++TimelineIndex)
	{
		UTimelineTemplate* TimelineTemplate = InBlueprint->Timelines[TimelineIndex];
		if( TimelineTemplate )
		{
			if( TimelineTemplate->GetFName() == InVariableName )
			{
				FName NewName = FBlueprintEditorUtils::FindUniqueKismetName(InBlueprint, TimelineTemplate->GetName());
				FBlueprintEditorUtils::RenameTimeline(InBlueprint, TimelineTemplate->GetFName(), NewName);

				UE_LOG(LogBlueprint, Warning, TEXT("Blueprint %s (child of/implements %s) has a timeline with a conflicting name (%s). Changing to %s."), *InBlueprint->GetName(), *InParentBlueprint->GetName(), *InVariableName.ToString(), *NewName.ToString());
				return true;
			}
		}
	}
	return false;
}

bool FBlueprintEditorUtils::ValidateAllFunctionGraphs(UBlueprint* InBlueprint, UBlueprint* InParentBlueprint, const FName& InVariableName)
{
	for (int32 FunctionIndex=0; FunctionIndex < InBlueprint->FunctionGraphs.Num(); ++FunctionIndex)
	{
		UEdGraph* FunctionGraph = InBlueprint->FunctionGraphs[FunctionIndex];

		if( FunctionGraph->GetFName() == InVariableName )
		{
			FName NewName = FBlueprintEditorUtils::FindUniqueKismetName(InBlueprint, FunctionGraph->GetName());
			FBlueprintEditorUtils::RenameGraph(FunctionGraph, NewName.ToString());

			UE_LOG(LogBlueprint, Warning, TEXT("Blueprint %s (child of/implements %s) has a function graph with a conflicting name (%s). Changing to %s."), *InBlueprint->GetName(), *InParentBlueprint->GetName(), *InVariableName.ToString(), *NewName.ToString());
			return true;
		}
	}
	return false;
}

void FBlueprintEditorUtils::ValidateBlueprintChildVariables(UBlueprint* InBlueprint, const FName& InVariableName)
{
	// Iterate over currently-loaded Blueprints and potentially adjust their variable names if they conflict with the parent
	for(TObjectIterator<UBlueprint> BlueprintIt; BlueprintIt; ++BlueprintIt)
	{
		UBlueprint* ChildBP = *BlueprintIt;
		if(ChildBP != NULL && ChildBP->ParentClass != NULL)
		{
			TArray<UBlueprint*> ParentBPArray;
			// Get the parent hierarchy
			UBlueprint::GetBlueprintHierarchyFromClass(ChildBP->ParentClass, ParentBPArray);

			// Also get any BP interfaces we use
			TArray<UClass*> ImplementedInterfaces;
			FindImplementedInterfaces(ChildBP, true, ImplementedInterfaces);
			for(auto InterfaceIt(ImplementedInterfaces.CreateConstIterator()); InterfaceIt; InterfaceIt++)
			{
				UBlueprint* BlueprintInterfaceClass = UBlueprint::GetBlueprintFromClass(*InterfaceIt);
				if(BlueprintInterfaceClass != NULL)
				{
					ParentBPArray.Add(BlueprintInterfaceClass);
				}
			}

			if(ParentBPArray.Contains(InBlueprint))
			{
				bool bValidatedVariable = false;

				bValidatedVariable = ValidateAllMemberVariables(ChildBP, InBlueprint, InVariableName);

				if(!bValidatedVariable)
				{
					bValidatedVariable = ValidateAllComponentMemberVariables(ChildBP, InBlueprint, InVariableName);
				}

				if(!bValidatedVariable)
				{
					bValidatedVariable = ValidateAllTimelines(ChildBP, InBlueprint, InVariableName);
				}

				if(!bValidatedVariable)
				{
					bValidatedVariable = ValidateAllFunctionGraphs(ChildBP, InBlueprint, InVariableName);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void FBlueprintEditorUtils::ImportKismetDefaultValueToProperty(UEdGraphPin* SourcePin, UProperty* DestinationProperty, uint8* DestinationAddress, UObject* OwnerObject)
{
	FString LiteralString = SourcePin->GetDefaultAsString();

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	if (UStructProperty* StructProperty = Cast<UStructProperty>(DestinationProperty))
	{
		static UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));
		static UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Rotator"));
		static UScriptStruct* TransformStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Transform"));

		if (StructProperty->Struct == VectorStruct)
		{
			FDefaultValueHelper::ParseVector(LiteralString, *((FVector*)DestinationAddress));
			return;
		}
		else if (StructProperty->Struct == RotatorStruct)
		{
			FDefaultValueHelper::ParseRotator(LiteralString, *((FRotator*)DestinationAddress));
			return;
		}
		else if (StructProperty->Struct == TransformStruct)
		{
			(*(FTransform*)DestinationAddress).InitFromString( LiteralString );
			return;
		}
	}
	else if (UObjectPropertyBase* ObjectProperty = Cast<UObjectPropertyBase>(DestinationProperty))
	{
		ObjectProperty->SetObjectPropertyValue(DestinationAddress, SourcePin->DefaultObject);
		return;
	}

	const int32 PortFlags = 0;
	DestinationProperty->ImportText(*LiteralString, DestinationAddress, PortFlags, OwnerObject);
}

void FBlueprintEditorUtils::ExportPropertyToKismetDefaultValue(UEdGraphPin* TargetPin, UProperty* SourceProperty, uint8* SourceAddress, UObject* OwnerObject)
{
	FString LiteralString;

	if (UStructProperty* StructProperty = Cast<UStructProperty>(SourceProperty))
	{
		static UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));
		static UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Rotator"));
		static UScriptStruct* TransformStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Transform"));

		if (StructProperty->Struct == VectorStruct)
		{
			FVector& SourceVector = *((FVector*)SourceAddress);
			TargetPin->DefaultValue = FString::Printf(TEXT("%f, %f, %f"), SourceVector.X, SourceVector.Y, SourceVector.Z);
			return;
		}
		else if (StructProperty->Struct == RotatorStruct)
		{
			FRotator& SourceRotator = *((FRotator*)SourceAddress);
			TargetPin->DefaultValue = FString::Printf(TEXT("%f, %f, %f"), SourceRotator.Pitch, SourceRotator.Yaw, SourceRotator.Roll);
			return;
		}
		else if (StructProperty->Struct == TransformStruct)
		{
			TargetPin->DefaultValue = (*(FTransform*)SourceAddress).ToString();
			return;
		}
	}
	else if (UObjectPropertyBase* ObjectProperty = Cast<UObjectPropertyBase>(SourceProperty))
	{
		TargetPin->DefaultObject = ObjectProperty->GetObjectPropertyValue(SourceAddress);
		return;
	}

	//@TODO: Add support for object literals

	const int32 PortFlags = 0;

	TargetPin->DefaultValue.Empty();
	SourceProperty->ExportTextItem(TargetPin->DefaultValue, SourceAddress, NULL, OwnerObject, PortFlags);
}

//////////////////////////////////////////////////////////////////////////
// Interfaces

// Add a new interface, and member function graphs to the blueprint
bool FBlueprintEditorUtils::ImplementNewInterface(UBlueprint* Blueprint, const FName& InterfaceClassName)
{
	check(InterfaceClassName != NAME_None);

	// Attempt to find the class we want to implement
	UClass* InterfaceClass = (UClass*)StaticFindObject(UClass::StaticClass(), ANY_PACKAGE, *InterfaceClassName.ToString());

	// Make sure the class is found, and isn't native (since Blueprints don't necessarily generate native classes.
	check(InterfaceClass);

	// Check to make sure we haven't already implemented it
	for( int32 i = 0; i < Blueprint->ImplementedInterfaces.Num(); i++ )
	{
		if( Blueprint->ImplementedInterfaces[i].Interface == InterfaceClass )
		{
			Blueprint->Message_Warn( FString::Printf (*LOCTEXT("InterfaceAlreadyImplemented", "ImplementNewInterface: Blueprint '%s' already implements the interface called '%s'").ToString(), 
				*Blueprint->GetPathName(), 
				*InterfaceClassName.ToString())
				);
			return false;
		}
	}

	// Make a new entry for this interface
	FBPInterfaceDescription NewInterface;
	NewInterface.Interface = InterfaceClass;

	bool bAllFunctionsAdded = true;

	// Add the graphs for the functions required by this interface
	for( TFieldIterator<UFunction> FunctionIter(InterfaceClass, EFieldIteratorFlags::IncludeSuper); FunctionIter; ++FunctionIter )
	{
		UFunction* Function = *FunctionIter;
		if( UEdGraphSchema_K2::CanKismetOverrideFunction(Function) && !UEdGraphSchema_K2::FunctionCanBePlacedAsEvent(Function) )
		{
			FName FunctionName = Function->GetFName();
			UEdGraph* FuncGraph = FindObject<UEdGraph>( Blueprint, *(FunctionName.ToString()) );
			if (FuncGraph != NULL)
			{
				bAllFunctionsAdded = false;

				Blueprint->Message_Error( FString::Printf (*LOCTEXT("InterfaceFunctionConflicts", "ImplementNewInterface: Blueprint '%s' has a function or graph which conflicts with the function %s in the interface called '%s'").ToString(), 
					*Blueprint->GetPathName(), 
					*FunctionName.ToString(), 
					*InterfaceClassName.ToString())
					);
				break;
			}

			UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FunctionName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
			NewGraph->bAllowDeletion = false;
			NewInterface.Graphs.Add(NewGraph);

			FBlueprintEditorUtils::AddInterfaceGraph(Blueprint, NewGraph, InterfaceClass);
		}
	}

	if (bAllFunctionsAdded)
	{
		Blueprint->ImplementedInterfaces.Add(NewInterface);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
	return bAllFunctionsAdded;
}

// Gets the graphs currently in the blueprint associated with the specified interface
void FBlueprintEditorUtils::GetInterfaceGraphs(UBlueprint* Blueprint, const FName& InterfaceClassName, TArray<UEdGraph*>& ChildGraphs)
{
	ChildGraphs.Empty();

	if( InterfaceClassName == NAME_None )
	{
		return;
	}

	// Find the implemented interface
	for( int32 i = 0; i < Blueprint->ImplementedInterfaces.Num(); i++ )
	{
		if( Blueprint->ImplementedInterfaces[i].Interface->GetFName() == InterfaceClassName )
		{
			ChildGraphs = Blueprint->ImplementedInterfaces[i].Graphs;
			return;			
		}
	}
}

// Remove an implemented interface, and its associated member function graphs
void FBlueprintEditorUtils::RemoveInterface(UBlueprint* Blueprint, const FName& InterfaceClassName, bool bPreserveFunctions /*= false*/)
{
	if( InterfaceClassName == NAME_None )
	{
		return;
	}

	// Find the implemented interface
	int32 Idx = INDEX_NONE;
	for( int32 i = 0; i < Blueprint->ImplementedInterfaces.Num(); i++ )
	{
		if( Blueprint->ImplementedInterfaces[i].Interface->GetFName() == InterfaceClassName )
		{
			Idx = i;
			break;
		}
	}

	if( Idx != INDEX_NONE )
	{
		FBPInterfaceDescription& CurrentInterface = Blueprint->ImplementedInterfaces[Idx];

		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		// Remove all the graphs that we implemented
		for(TArray<UEdGraph*>::TIterator it(CurrentInterface.Graphs); it; ++it)
		{
			UEdGraph* CurrentGraph = *it;
			if(bPreserveFunctions)
			{
				CurrentGraph->bAllowDeletion = true;
				CurrentGraph->bAllowRenaming = true;
				CurrentGraph->bEditable = true;

				// We need to flag the entry node to make sure that the compiled function is callable
				Schema->AddExtraFunctionFlags(CurrentGraph, (FUNC_BlueprintCallable|FUNC_BlueprintEvent|FUNC_Public));
				Schema->MarkFunctionEntryAsEditable(CurrentGraph, true);

				Blueprint->FunctionGraphs.Add(CurrentGraph);
			}
			else
			{
				FBlueprintEditorUtils::RemoveGraph(Blueprint, CurrentGraph, EGraphRemoveFlags::MarkTransient);	// Do not recompile, yet*
			}
		}

		// Find all events placed in the event graph, and remove them
		TArray<UK2Node_Event*> AllEvents;
		FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, AllEvents);
		const UClass* InterfaceClass = Blueprint->ImplementedInterfaces[Idx].Interface;
		for(TArray<UK2Node_Event*>::TIterator NodeIt(AllEvents); NodeIt; ++NodeIt)
		{
			UK2Node_Event* EventNode = *NodeIt;
			if( EventNode->EventSignatureClass == InterfaceClass )
			{
				if(bPreserveFunctions)
				{
					// Create a custom event with the same name and signature
					const FVector2D PreviousNodePos = FVector2D(EventNode->NodePosX, EventNode->NodePosY);
					const FString PreviousNodeName = EventNode->EventSignatureName.ToString();
					const UFunction* PreviousSignatureFunction = EventNode->EventSignatureClass->FindFunctionByName(EventNode->EventSignatureName);
					check(PreviousSignatureFunction);
					
					UK2Node_CustomEvent* NewEvent = UK2Node_CustomEvent::CreateFromFunction(PreviousNodePos, EventNode->GetGraph(), PreviousNodeName, PreviousSignatureFunction, false);

					// Move the pin links from the old pin to the new pin to preserve connections
					for(auto PinIt = EventNode->Pins.CreateIterator(); PinIt; ++PinIt)
					{
						UEdGraphPin* CurrentPin = *PinIt;
						UEdGraphPin* TargetPin = NewEvent->FindPinChecked(CurrentPin->PinName);
						Schema->MovePinLinks(*CurrentPin, *TargetPin);
					}
				}
			
				EventNode->GetGraph()->RemoveNode(EventNode);
			}
		}

		// Then remove the interface from the list
		Blueprint->ImplementedInterfaces.RemoveAt(Idx, 1);
	
		// *Now recompile the blueprint (this needs to be done outside of RemoveGraph, after it's been removed from ImplementedInterfaces - otherwise it'll re-add it)
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
}

void FBlueprintEditorUtils::CleanNullGraphReferencesRecursive(UEdGraph* Graph)
{
	for (int32 GraphIndex = 0; GraphIndex < Graph->SubGraphs.Num(); )
	{
		if (UEdGraph* ChildGraph = Graph->SubGraphs[GraphIndex])
		{
			CleanNullGraphReferencesRecursive(ChildGraph);
			++GraphIndex;
		}
		else
		{
			UE_LOG(LogBlueprint, Warning, TEXT("Found NULL graph reference in children of '%s', removing it!"), *Graph->GetPathName());
			Graph->SubGraphs.RemoveAt(GraphIndex);
		}
	}
}

void FBlueprintEditorUtils::CleanNullGraphReferencesInArray(UBlueprint* Blueprint, TArray<UEdGraph*>& GraphArray)
{
	for (int32 GraphIndex = 0; GraphIndex < GraphArray.Num(); )
	{
		if (UEdGraph* Graph = GraphArray[GraphIndex])
		{
			CleanNullGraphReferencesRecursive(Graph);
			++GraphIndex;
		}
		else
		{
			UE_LOG(LogBlueprint, Warning, TEXT("Found NULL graph reference in '%s', removing it!"), *Blueprint->GetPathName());
			GraphArray.RemoveAt(GraphIndex);
		}
	}
}

void FBlueprintEditorUtils::PurgeNullGraphs(UBlueprint* Blueprint)
{
	CleanNullGraphReferencesInArray(Blueprint, Blueprint->UbergraphPages);
	CleanNullGraphReferencesInArray(Blueprint, Blueprint->FunctionGraphs);
	CleanNullGraphReferencesInArray(Blueprint, Blueprint->DelegateSignatureGraphs);
	CleanNullGraphReferencesInArray(Blueprint, Blueprint->MacroGraphs);
}

// Makes sure that calls to parent functions are valid, and removes them if not
void FBlueprintEditorUtils::ConformCallsToParentFunctions(UBlueprint* Blueprint)
{
	check(NULL != Blueprint);

	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);
	for(int GraphIndex = 0; GraphIndex < AllGraphs.Num(); ++GraphIndex)
	{
		UEdGraph* CurrentGraph = AllGraphs[GraphIndex];
		check(NULL != CurrentGraph);

		// Make sure the graph is loaded
		if(!CurrentGraph->HasAnyFlags(RF_NeedLoad|RF_NeedPostLoad))
		{
			TArray<UK2Node_CallParentFunction*> CallFunctionNodes;
			CurrentGraph->GetNodesOfClass<UK2Node_CallParentFunction>(CallFunctionNodes);

			// For each parent function call node in the graph
			for(int CallFunctionNodeIndex = 0; CallFunctionNodeIndex < CallFunctionNodes.Num(); ++CallFunctionNodeIndex)
			{
				UK2Node_CallParentFunction* CallFunctionNode = CallFunctionNodes[CallFunctionNodeIndex];
				check(NULL != CallFunctionNode);

				// Make sure the node has already been loaded
				if(!CallFunctionNode->HasAnyFlags(RF_NeedLoad|RF_NeedPostLoad))
				{
					// Attempt to locate the function within the parent class
					const UFunction* TargetFunction = CallFunctionNode->GetTargetFunction();
					TargetFunction = TargetFunction && Blueprint->ParentClass ? Blueprint->ParentClass->FindFunctionByName(TargetFunction->GetFName()) : NULL;
					if(TargetFunction)
					{
						// If the function signature does not match the parent class
						if(TargetFunction->GetOwnerClass() != CallFunctionNode->FunctionReference.GetMemberParentClass(Blueprint->ParentClass))
						{
							// Emit something to the log to indicate that we're making a change
							Blueprint->Message_Note(FString::Printf(*LOCTEXT("CallParentFunctionSignatureFixed_Note", "%s (%s) had an invalid function signature - it has now been fixed.").ToString(),
								*CallFunctionNode->GetNodeTitle(ENodeTitleType::ListView), *CallFunctionNode->GetName()));

							// Redirect to the correct parent function
							CallFunctionNode->SetFromFunction(TargetFunction);
						}
					}
					else
					{
						// Cache a reference to the output exec pin
						UEdGraphPin* OutputPin = CallFunctionNode->GetThenPin();
						check(NULL != OutputPin);

						// We're going to destroy the existing parent function call node, but first we need to persist any existing connections
						for(int PinIndex = 0; PinIndex < CallFunctionNode->Pins.Num(); ++PinIndex)
						{
							UEdGraphPin* InputPin = CallFunctionNode->Pins[PinIndex];
							check(NULL != InputPin);

							// If this is an input exec pin
							const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
							if(K2Schema && K2Schema->IsExecPin(*InputPin) && InputPin->Direction == EGPD_Input)
							{
								// Redirect any existing connections to the input exec pin to whatever pin(s) the output exec pin is connected to
								for(int InputLinkedToPinIndex = 0; InputLinkedToPinIndex < InputPin->LinkedTo.Num(); ++InputLinkedToPinIndex)
								{
									UEdGraphPin* InputLinkedToPin = InputPin->LinkedTo[InputLinkedToPinIndex];
									check(NULL != InputLinkedToPin);

									// Break the existing link to the node we're about to remove
									InputLinkedToPin->BreakLinkTo(InputPin);

									// Redirect the input connection to the output connection(s)
									for(int OutputLinkedToPinIndex = 0; OutputLinkedToPinIndex < OutputPin->LinkedTo.Num(); ++OutputLinkedToPinIndex)
									{
										UEdGraphPin* OutputLinkedToPin = OutputPin->LinkedTo[OutputLinkedToPinIndex];
										check(NULL != OutputLinkedToPin);

										// Make sure the output connection isn't linked to the node we're about to remove
										if(OutputLinkedToPin->LinkedTo.Contains(OutputPin))
										{
											OutputLinkedToPin->BreakLinkTo(OutputPin);
										}
										
										// Fix up the connection
										InputLinkedToPin->MakeLinkTo(OutputLinkedToPin);
									}
								}
							}
						}

						// Emit something to the log to indicate that we're making a change
						Blueprint->Message_Note(FString::Printf(*LOCTEXT("CallParentNodeRemoved_Note", "%s (%s) was not valid for this Blueprint - it has been removed.").ToString(),
							*CallFunctionNode->GetNodeTitle(ENodeTitleType::ListView), *CallFunctionNode->GetName()));

						// Destroy the existing parent function call node (this will also break pin links and remove it from the graph)
						CallFunctionNode->DestroyNode();
					}
				}
			}
		}
	}
}

// Makes sure that all events we handle exist, and replace with custom events if not
void FBlueprintEditorUtils::ConformImplementedEvents(UBlueprint* Blueprint)
{
	check(NULL != Blueprint);

	// Collect all event graph names
	TArray<FName> EventGraphNames;
	for(int EventGraphIndex = 0; EventGraphIndex < Blueprint->EventGraphs.Num(); ++EventGraphIndex)
	{
		UEdGraph* EventGraph = Blueprint->EventGraphs[EventGraphIndex];
		if(EventGraph)
		{
			EventGraphNames.AddUnique(EventGraph->GetFName());
		}
	}

	// Collect all implemented interface classes
	TArray<UClass*> ImplementedInterfaceClasses;
	FindImplementedInterfaces(Blueprint, true, ImplementedInterfaceClasses);

	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);
	for(int GraphIndex = 0; GraphIndex < AllGraphs.Num(); ++GraphIndex)
	{
		UEdGraph* CurrentGraph = AllGraphs[GraphIndex];
		check(NULL != CurrentGraph);

		// Make sure the graph is loaded
		if(!CurrentGraph->HasAnyFlags(RF_NeedLoad|RF_NeedPostLoad))
		{
			TArray<UK2Node_Event*> EventNodes;
			CurrentGraph->GetNodesOfClass<UK2Node_Event>(EventNodes);

			// For each event node in the graph
			for(int EventNodeIndex = 0; EventNodeIndex < EventNodes.Num(); ++EventNodeIndex)
			{
				UK2Node_Event* EventNode = EventNodes[EventNodeIndex];
				check(NULL != EventNode);

				// If the event is loaded and is not a custom event
				if(!EventNode->HasAnyFlags(RF_NeedLoad|RF_NeedPostLoad) && EventNode->bOverrideFunction)
				{
					if(Blueprint->GeneratedClass)
					{
						// See if the generated class implements an event with the given function signature
						const UFunction* TargetFunction = Blueprint->GeneratedClass->FindFunctionByName(EventNode->EventSignatureName);
						if(TargetFunction || EventGraphNames.Contains(EventNode->EventSignatureName))
						{
							// The generated class implements the event but the function signature is not up-to-date
							if(!Blueprint->GeneratedClass->IsChildOf(EventNode->EventSignatureClass)
								&& !ImplementedInterfaceClasses.Contains(EventNode->EventSignatureClass))
							{
								// Emit something to the log to indicate that we've made a change
								Blueprint->Message_Note(FString::Printf(*LOCTEXT("EventSignatureFixed_Note", "%s (%s) had an invalid function signature - it has now been fixed.").ToString(),
									*EventNode->GetNodeTitle(ENodeTitleType::ListView), *EventNode->GetName()));

								// Fix up the event signature
								EventNode->EventSignatureClass = Blueprint->GeneratedClass;
							}
						}
						else
						{
							// The generated class does not implement this event, so replace it with a custom event node instead (this will persist any existing connections)
							UEdGraphNode* CustomEventNode = CurrentGraph->GetSchema()->CreateSubstituteNode(EventNode, CurrentGraph, NULL);
							if(CustomEventNode)
							{
								// Emit something to the log to indicate that we've made a change
								Blueprint->Message_Note(FString::Printf(*LOCTEXT("EventNodeReplaced_Note", "%s (%s) was not valid for this Blueprint - it has been converted to a custom event.").ToString(),
									*EventNode->GetNodeTitle(ENodeTitleType::ListView), *EventNode->GetName()));

								// Destroy the old event node (this will also break all pin links and remove it from the graph)
								EventNode->DestroyNode();

								// Add the new custom event node to the graph
								CurrentGraph->Nodes.Add(CustomEventNode);
							}
						}
					}
				}
			}
		}
	}
}

// Makes sure that all graphs for all interfaces we implement exist, and add if not
void FBlueprintEditorUtils::ConformImplementedInterfaces(UBlueprint* Blueprint)
{
	check(NULL != Blueprint);
	FString ErrorStr;

	// Collect all variables names in current blueprint 
	TArray<FName> VariableNamesUsedInBlueprint;
	for (TFieldIterator<UProperty> VariablesIter(Blueprint->GeneratedClass); VariablesIter; ++VariablesIter)
	{
		VariableNamesUsedInBlueprint.Add(VariablesIter->GetFName());
	}
	for (auto VariablesIter = Blueprint->NewVariables.CreateConstIterator(); VariablesIter; ++VariablesIter)
	{
		VariableNamesUsedInBlueprint.AddUnique(VariablesIter->VarName);
	}

	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);
	// collect all existing event nodes, so we can find interface events that 
	// need to be converted to function graphs
	TArray<UK2Node_Event*> ImplementedEvents;
	for (UEdGraph const* Graph : AllGraphs)
	{
		Graph->GetNodesOfClass<UK2Node_Event>(ImplementedEvents);
	}

	for (int32 InterfaceIndex = 0; InterfaceIndex < Blueprint->ImplementedInterfaces.Num(); )
	{
		FBPInterfaceDescription& CurrentInterface = Blueprint->ImplementedInterfaces[InterfaceIndex];
		if (!CurrentInterface.Interface)
		{
			Blueprint->Status = BS_Error;
			Blueprint->ImplementedInterfaces.RemoveAt(InterfaceIndex, 1);
			continue;
		}

		// a interface could have since been added by the parent (or this blueprint could have been re-parented)
		if (IsInterfaceImplementedByParent(CurrentInterface, Blueprint))
		{			
			// have to remove the interface before we promote it (in case this method is reentrant)
			FBPInterfaceDescription LocalInterfaceCopy = CurrentInterface;
			Blueprint->ImplementedInterfaces.RemoveAt(InterfaceIndex, 1);

			// in this case, the interface needs to belong to the parent and not this
			// blueprint (we would have been prevented from getting in this state if we
			// had started with a parent that implemented this interface initially)
			PromoteInterfaceImplementationToOverride(LocalInterfaceCopy, Blueprint);
			continue;
		}

		// check to make sure that there aren't any interface methods that we originally 
		// implemented as events, but have since switched to functions 
		for (UK2Node_Event* EventNode : ImplementedEvents)
		{
			// if this event belongs to something other than this interface
			if (EventNode->EventSignatureClass != CurrentInterface.Interface)
			{
				continue;
			}

			UFunction* InterfaceFunction = CurrentInterface.Interface->FindFunctionByName(EventNode->EventSignatureName);
			// if the function is still ok as an event, no need to try and fix it up
			if (UEdGraphSchema_K2::FunctionCanBePlacedAsEvent(InterfaceFunction))
			{
				continue;
			}

			UEdGraph* EventGraph = EventNode->GetGraph();
			// we've already implemented this interface function as an event (which we need to replace)
			UK2Node_CustomEvent* CustomEventNode = Cast<UK2Node_CustomEvent>(EventGraph->GetSchema()->CreateSubstituteNode(EventNode, EventGraph, NULL));
			check(CustomEventNode != NULL);			

			// grab the function's name before we delete the node
			FName const FunctionName = EventNode->EventSignatureName;
			// destroy the old event node (this will also break all pin links and remove it from the graph)
			EventNode->DestroyNode();

			// have to rename so it doesn't conflict with the graph we're about to add
			CustomEventNode->RenameCustomEventCloseToName();
			EventGraph->Nodes.Add(CustomEventNode);

			// warn the user that their old functionality won't work (it's now connected 
			// to a custom node that isn't triggered anywhere)
			FText WarningMessageText = LOCTEXT("InterfaceEventNodeReplaced_Warn", "'%s' was promoted from an event to a function - it has been replaced by a custom event, which won't trigger unless you call it manually.");
			Blueprint->Message_Warn(FString::Printf(*WarningMessageText.ToString(), *FunctionName.ToString()));
		}

		// Cache off the graph names for this interface, for easier searching
		TArray<FName> GraphNames;
		for (int32 j = 0; j < CurrentInterface.Graphs.Num(); j++)
		{
			UEdGraph* CurrentGraph = CurrentInterface.Graphs[j];
			if( CurrentGraph )
			{
				GraphNames.AddUnique(CurrentGraph->GetFName());
			}
		}

		// Iterate over all the functions in the interface, and create graphs that are in the interface, but missing in the blueprint
		for (TFieldIterator<UFunction> FunctionIter(CurrentInterface.Interface, EFieldIteratorFlags::IncludeSuper); FunctionIter; ++FunctionIter)
		{
			UFunction* Function = *FunctionIter;
			const FName FunctionName = Function->GetFName();
			if(!VariableNamesUsedInBlueprint.Contains(FunctionName))
			{
				if( UEdGraphSchema_K2::CanKismetOverrideFunction(Function) && !UEdGraphSchema_K2::FunctionCanBePlacedAsEvent(Function) && !GraphNames.Contains(FunctionName) )
				{
					// interface methods initially create EventGraph stubs, so we need
					// to make sure we remove that entry so the new graph doesn't conflict (don't
					// worry, these are regenerated towards the end of a compile)
					for (UEdGraph* GraphStub : Blueprint->EventGraphs)
					{
						if (GraphStub->GetFName() == FunctionName)
						{
							FBlueprintEditorUtils::RemoveGraph(Blueprint, GraphStub, EGraphRemoveFlags::MarkTransient);
						}
					}

					// Check to see if we already have implemented 
					UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FunctionName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
					NewGraph->bAllowDeletion = false;
					CurrentInterface.Graphs.Add(NewGraph);

					FBlueprintEditorUtils::AddInterfaceGraph(Blueprint, NewGraph, CurrentInterface.Interface);
				}
			}
			else
			{
				Blueprint->Status = BS_Error;
				const FString NewError = FString::Printf( *LOCTEXT("InterfaceNameCollision_Error", "Interface name collision in blueprint: %s, interface: %s, name: %s").ToString(), 
					*Blueprint->GetFullName(), *CurrentInterface.Interface->GetFullName(), *FunctionName.ToString());
				Blueprint->Message_Error(NewError);
			}
		}

		// Iterate over all the graphs in the blueprint interface, and remove ones that no longer have functions 
		for (int32 j = 0; j < CurrentInterface.Graphs.Num(); j++)
		{
			// If we can't find the function associated with the graph, delete it
			UEdGraph* CurrentGraph = CurrentInterface.Graphs[j];
			if (!CurrentGraph || !FindField<UFunction>(CurrentInterface.Interface, CurrentGraph->GetFName()))
			{
				CurrentInterface.Graphs.RemoveAt(j, 1);
				j--;
			}
		}

		// not going to remove this interface, so let's continue forward
		++InterfaceIndex;
	}
}

/** Handle old Anim Blueprints (state machines in the wrong position, transition graphs with the wrong schema, etc...) */
void FBlueprintEditorUtils::UpdateOutOfDateAnimBlueprints(UBlueprint* InBlueprint)
{
	if (UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(InBlueprint))
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		// Ensure all transition graphs have the correct schema
		TArray<UAnimStateTransitionNode*> TransitionNodes;
		GetAllNodesOfClass<UAnimStateTransitionNode>(AnimBlueprint, /*out*/ TransitionNodes);
		for (auto NodeIt = TransitionNodes.CreateConstIterator(); NodeIt; ++NodeIt)
		{
			UAnimStateTransitionNode* Node = *NodeIt;
			UEdGraph* TestGraph = Node->BoundGraph;
			if (TestGraph->Schema == UAnimationGraphSchema::StaticClass())
			{
				TestGraph->Schema = UAnimationTransitionSchema::StaticClass();
			}
		}

		// Handle a reparented anim blueprint that either needs or no longer needs an anim graph
		if (UAnimBlueprint::FindRootAnimBlueprint(AnimBlueprint) == NULL)
		{
			// Add an anim graph if not present
			if (FindObject<UEdGraph>(AnimBlueprint, *(K2Schema->GN_AnimGraph.ToString())) == NULL)
			{
				UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(AnimBlueprint, K2Schema->GN_AnimGraph, UAnimationGraph::StaticClass(), UAnimationGraphSchema::StaticClass());
				FBlueprintEditorUtils::AddDomainSpecificGraph(AnimBlueprint, NewGraph);
				AnimBlueprint->LastEditedDocuments.Add(NewGraph);
				NewGraph->bAllowDeletion = false;
			}
		}
		else
		{
			// Remove an anim graph if present
			for (int32 i = 0; i < AnimBlueprint->FunctionGraphs.Num(); ++i)
			{
				UEdGraph* FuncGraph = AnimBlueprint->FunctionGraphs[i];
				if ((FuncGraph != NULL) && (FuncGraph->GetFName() == K2Schema->GN_AnimGraph))
				{
					UE_LOG(LogBlueprint, Log, TEXT("!!! Removing AnimGraph from %s, because it has a parent anim blueprint that defines the AnimGraph"), *AnimBlueprint->GetPathName());
					AnimBlueprint->FunctionGraphs.RemoveAt(i);
					break;
				}
			}
		}
	}
}

void FBlueprintEditorUtils::UpdateOutOfDateCompositeNodes(UBlueprint* Blueprint)
{
	for( auto It = Blueprint->UbergraphPages.CreateIterator(); It; ++It )
	{
		UpdateOutOfDateCompositeWithOuter(Blueprint, *It);
	}
	for( auto It = Blueprint->FunctionGraphs.CreateIterator(); It; ++It )
	{
		UpdateOutOfDateCompositeWithOuter(Blueprint, *It);
	}
}

void FBlueprintEditorUtils::UpdateOutOfDateCompositeWithOuter(UBlueprint* Blueprint, UEdGraph* OuterGraph )
{
	check(OuterGraph != NULL);
	check(FindBlueprintForGraphChecked(OuterGraph) == Blueprint);

	for (auto It = OuterGraph->Nodes.CreateIterator();It;++It)
	{
		UEdGraphNode* Node = *It;
		
		//Is this node of a type that has a BoundGraph to update
		UEdGraph* BoundGraph = NULL;
		if (UK2Node_Composite* Composite = Cast<UK2Node_Composite>(Node))
		{
			BoundGraph = Composite->BoundGraph;
		}
		else if (UAnimStateNode* StateNode = Cast<UAnimStateNode>(Node))
		{
			BoundGraph = StateNode->BoundGraph;
		}
		else if (UAnimStateTransitionNode* TransitionNode = Cast<UAnimStateTransitionNode>(Node))
		{
			BoundGraph = TransitionNode->BoundGraph;
		}
		else if (UAnimGraphNode_StateMachineBase* StateMachineNode = Cast<UAnimGraphNode_StateMachineBase>(Node))
		{
			BoundGraph = StateMachineNode->EditorStateMachineGraph;
		}

		if (BoundGraph)
		{
			// Check for out of date BoundGraph where outer is not the composite node
			if (BoundGraph->GetOuter() != Node)
			{
				// change the outer of the BoundGraph to be the composite node instead of the OuterGraph
				if (false == BoundGraph->Rename(*BoundGraph->GetName(), Node, ((BoundGraph->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad) ? REN_ForceNoResetLoaders : 0) | REN_DontCreateRedirectors)))
				{
					UE_LOG(LogBlueprintDebug, Log, TEXT("CompositeNode: On Blueprint '%s' could not fix Outer() for BoundGraph of composite node '%s'"), *Blueprint->GetPathName(), *Node->GetName());
				}
			}
		}
	}

	for (auto It = OuterGraph->SubGraphs.CreateIterator();It;++It)
	{
		UpdateOutOfDateCompositeWithOuter(Blueprint,*It);
	}
}

/** Ensure all component templates are in use */
void FBlueprintEditorUtils::UpdateComponentTemplates(UBlueprint* Blueprint)
{
	TArray<UActorComponent*> ReferencedTemplates;

	TArray<UK2Node_AddComponent*> AllComponents;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, AllComponents);

	for( auto CompIt = AllComponents.CreateIterator(); CompIt; ++CompIt )
	{
		UK2Node_AddComponent* ComponentNode = (*CompIt);
		check(ComponentNode);

		UActorComponent* ActorComp = ComponentNode->GetTemplateFromNode();
		if (ActorComp)
		{
			ensure(Blueprint->ComponentTemplates.Contains(ActorComp));

			// fix up existing content to be sure these are flagged as archetypes
			ActorComp->SetFlags(RF_ArchetypeObject);	
			ReferencedTemplates.Add(ActorComp);
		}
	}
	Blueprint->ComponentTemplates.Empty();
	Blueprint->ComponentTemplates.Append(ReferencedTemplates);
}

/** Ensures that the CDO root component reference is valid for Actor-based Blueprints */
void FBlueprintEditorUtils::UpdateRootComponentReference(UBlueprint* Blueprint)
{
	// The CDO's root component reference should match that of its parent class
	if(Blueprint && Blueprint->ParentClass && Blueprint->GeneratedClass)
	{
		AActor* ParentActorCDO = Cast<AActor>(Blueprint->ParentClass->GetDefaultObject(false));
		AActor* BlueprintActorCDO = Cast<AActor>(Blueprint->GeneratedClass->GetDefaultObject(false));
		if(ParentActorCDO && BlueprintActorCDO)
		{
			// If both CDOs are valid, check for a valid scene root component
			USceneComponent* ParentSceneRootComponent = ParentActorCDO->GetRootComponent();
			USceneComponent* BlueprintSceneRootComponent = BlueprintActorCDO->GetRootComponent();
			if((ParentSceneRootComponent == NULL && BlueprintSceneRootComponent != NULL)
				|| (ParentSceneRootComponent != NULL && BlueprintSceneRootComponent == NULL)
				|| (ParentSceneRootComponent != NULL && BlueprintSceneRootComponent != NULL && ParentSceneRootComponent->GetFName() != BlueprintSceneRootComponent->GetFName()))
			{
				// If the parent CDO has a valid scene root component
				if(ParentSceneRootComponent != NULL)
				{
					// Search for a scene component with the same name in the Blueprint CDO's Components list
					TArray<USceneComponent*> SceneComponents;
					BlueprintActorCDO->GetComponents(SceneComponents);
					for(int i = 0; i < SceneComponents.Num(); ++i)
					{
						USceneComponent* SceneComp = SceneComponents[i];
						if(SceneComp && SceneComp->GetFName() == ParentSceneRootComponent->GetFName())
						{
							// We found a match, so make this the new scene root component
							BlueprintActorCDO->SetRootComponent(SceneComp);
							break;
						}
					}
				}
				else if(BlueprintSceneRootComponent != NULL)
				{
					// The parent CDO does not have a valid scene root, so NULL out the Blueprint CDO reference to match
					BlueprintActorCDO->SetRootComponent(NULL);
				}
			}
		}
	}
}

/** Temporary fix for cut-n-paste error that failed to carry transactional flags */
void FBlueprintEditorUtils::UpdateTransactionalFlags(UBlueprint* Blueprint)
{
	TArray<UK2Node*> AllNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, AllNodes);

	for( auto NodeIt = AllNodes.CreateIterator(); NodeIt; ++NodeIt )
	{
		UK2Node* K2Node = (*NodeIt);
		check(K2Node);

		if (!K2Node->HasAnyFlags(RF_Transactional))
		{
			K2Node->SetFlags(RF_Transactional);
			Blueprint->Status = BS_Dirty;
		}
	}
}

void FBlueprintEditorUtils::UpdateStalePinWatches( UBlueprint* Blueprint )
{
	TSet<UEdGraphPin*> AllPins;

	// Find all unique pins being watched
	for (auto PinIt = Blueprint->PinWatches.CreateIterator(); PinIt; ++PinIt)
	{
		UEdGraphPin* Pin (*PinIt);
		if (Pin == NULL)
		{
			continue;
		}

		UEdGraphNode* OwningNode = Cast<UEdGraphNode>(Pin->GetOuter());
		// during node reconstruction, dead pins get moved to the transient 
		// package (so just in case this blueprint got saved with dead pin watches)
		if (OwningNode == NULL)
		{
			continue;
		}
		
		AllPins.Add(Pin);
	}

	// Refresh watched pins with unique pins (throw away null or duplicate watches)
	if (Blueprint->PinWatches.Num() != AllPins.Num())
	{
		Blueprint->PinWatches.Empty();
		for (auto PinIt = AllPins.CreateIterator(); PinIt; ++PinIt)
		{
			Blueprint->PinWatches.Add(*PinIt);
		}
		Blueprint->Status = BS_Dirty;
	}
}

FName FBlueprintEditorUtils::FindUniqueKismetName(const UBlueprint* InBlueprint, const FString& InBaseName)
{
	int32 Count = 0;
	FString KismetName;

	TSharedPtr<INameValidatorInterface> NameValidator = MakeShareable(new FKismetNameValidator(InBlueprint));
	while(NameValidator->IsValid(KismetName) != EValidatorResult::Ok)
	{
		KismetName = FString::Printf(TEXT("%s_%d"), *InBaseName, Count);
		Count++;
	}

	return FName(*KismetName);
}

FName FBlueprintEditorUtils::FindUniqueCustomEventName(const UBlueprint* Blueprint)
{
	return FindUniqueKismetName(Blueprint, LOCTEXT("DefaultCustomEventName", "CustomEvent").ToString());
}

//////////////////////////////////////////////////////////////////////////
// Timeline

FName FBlueprintEditorUtils::FindUniqueTimelineName(const UBlueprint* Blueprint)
{
	return FindUniqueKismetName(Blueprint, LOCTEXT("DefaultTimelineName", "Timeline").ToString());
}

UTimelineTemplate* FBlueprintEditorUtils::AddNewTimeline(UBlueprint* Blueprint, const FName& TimelineVarName)
{
	// Early out if we don't support timelines in this class
	if( !FBlueprintEditorUtils::DoesSupportTimelines(Blueprint) )
	{
		return NULL;
	}

	// First look to see if we already have a timeline with that name
	UTimelineTemplate* Timeline = Blueprint->FindTimelineTemplateByVariableName(TimelineVarName);
	if (Timeline != NULL)
	{
		UE_LOG(LogBlueprint, Log, TEXT("AddNewTimeline: Blueprint '%s' already contains a timeline called '%s'"), *Blueprint->GetPathName(), *TimelineVarName.ToString());
		return NULL;
	}
	else
	{
		Blueprint->Modify();
		check(NULL != Blueprint->GeneratedClass);
		// Construct new graph with the supplied name
		const FName TimelineTemplateName = *UTimelineTemplate::TimelineVariableNameToTemplateName(TimelineVarName);
		Timeline = NewNamedObject<UTimelineTemplate>(Blueprint->GeneratedClass, TimelineTemplateName, RF_Transactional);
		Blueprint->Timelines.Add(Timeline);

		// Potentially adjust variable names for any child blueprints
		FBlueprintEditorUtils::ValidateBlueprintChildVariables(Blueprint, TimelineVarName);

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		return Timeline;
	}
}

void FBlueprintEditorUtils::RemoveTimeline(UBlueprint* Blueprint, UTimelineTemplate* Timeline, bool bDontRecompile)
{
	// Ensure objects are marked modified
	Timeline->Modify();
	Blueprint->Modify();

	Blueprint->Timelines.Remove(Timeline);
	Timeline->MarkPendingKill();

	if( !bDontRecompile )
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
}

UK2Node_Timeline* FBlueprintEditorUtils::FindNodeForTimeline(UBlueprint* Blueprint, UTimelineTemplate* Timeline)
{
	check(Timeline);
	const FName TimelineVarName = *UTimelineTemplate::TimelineTemplateNameToVariableName(Timeline->GetFName());

	TArray<UK2Node_Timeline*> TimelineNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Timeline>(Blueprint, TimelineNodes);

	for(int32 i=0; i<TimelineNodes.Num(); i++)
	{
		UK2Node_Timeline* TestNode = TimelineNodes[i];
		if(TestNode->TimelineName == TimelineVarName)
		{
			return TestNode;
		}
	}

	return NULL; // no node found
}

bool FBlueprintEditorUtils::RenameTimeline(UBlueprint* Blueprint, const FName& OldName, const FName& NewName)
{
	check(Blueprint);

	bool bRenamed = false;


	TSharedPtr<INameValidatorInterface> NameValidator = MakeShareable(new FKismetNameValidator(Blueprint));
	const FString NewTemplateName = UTimelineTemplate::TimelineVariableNameToTemplateName(NewName);
	// NewName should be already validated. But one must make sure that NewTemplateName is also unique.
	const bool bUniqueNameForTemplate = (EValidatorResult::Ok == NameValidator->IsValid(NewTemplateName));

	UTimelineTemplate* Template = Blueprint->FindTimelineTemplateByVariableName(NewName);
	if ((Template == NULL) && bUniqueNameForTemplate)
	{
		Template = Blueprint->FindTimelineTemplateByVariableName(OldName);
		if (Template)
		{
			Blueprint->Modify();
			Template->Modify();

			if (UK2Node_Timeline* TimelineNode = FindNodeForTimeline(Blueprint, Template))
			{
				TimelineNode->Modify();
				TimelineNode->TimelineName = NewName;
			}

			const FString NewNameStr = NewName.ToString();
			const FString OldNameStr = OldName.ToString();

			TArray<UK2Node_Variable*> TimelineVarNodes;
			FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Variable>(Blueprint, TimelineVarNodes);
			for(int32 It = 0; It < TimelineVarNodes.Num(); It++)
			{
				UK2Node_Variable* TestNode = TimelineVarNodes[It];
				if(TestNode && (OldName == TestNode->GetVarName()))
				{
					UEdGraphPin* TestPin = TestNode->FindPin(OldNameStr);
					if(TestPin && (UTimelineComponent::StaticClass() == TestPin->PinType.PinSubCategoryObject.Get()))
					{
						TestNode->Modify();
						if(TestNode->VariableReference.IsSelfContext())
						{
							TestNode->VariableReference.SetSelfMember(NewName);
						}
						else
						{
							//TODO:
							UClass* ParentClass = TestNode->VariableReference.GetMemberParentClass((UClass*)NULL);
							TestNode->VariableReference.SetExternalMember(NewName, ParentClass);
						}
						TestPin->Modify();
						TestPin->PinName = NewNameStr;
					}
				}
			}

			Blueprint->Timelines.Remove(Template);
			
			UObject* ExistingObject = StaticFindObject(NULL, Template->GetOuter(), *NewTemplateName, true);
			if (ExistingObject != Template && ExistingObject != NULL)
			{
				ExistingObject->Rename(*MakeUniqueObjectName(ExistingObject->GetOuter(), ExistingObject->GetClass(), ExistingObject->GetFName()).ToString());
			}
			Template->Rename(*NewTemplateName, Template->GetOuter(), (Blueprint->bIsRegeneratingOnLoad ? REN_ForceNoResetLoaders : REN_None));
			Blueprint->Timelines.Add(Template);

			// Validate child blueprints and adjust variable names to avoid a potential name collision
			FBlueprintEditorUtils::ValidateBlueprintChildVariables(Blueprint, NewName);

			// Refresh references and flush editors
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			bRenamed = true;
		}
	}
	return bRenamed;
}

//////////////////////////////////////////////////////////////////////////
// LevelScriptBlueprint

int32 FBlueprintEditorUtils::FindNumReferencesToActorFromLevelScript(ULevelScriptBlueprint* LevelScriptBlueprint, AActor* InActor)
{
	TArray<UEdGraph*> AllGraphs;
	LevelScriptBlueprint->GetAllGraphs(AllGraphs);

	int32 RefCount = 0;

	for(TArray<UEdGraph*>::TConstIterator it(AllGraphs); it; ++it)
	{
		const UEdGraph* CurrentGraph = *it;

		TArray<UK2Node*> GraphNodes;
		CurrentGraph->GetNodesOfClass(GraphNodes);

		for( TArray<UK2Node*>::TConstIterator NodeIt(GraphNodes); NodeIt; ++NodeIt )
		{
			const UK2Node* CurrentNode = *NodeIt;
			if( CurrentNode != NULL && CurrentNode->GetReferencedLevelActor() == InActor )
			{
				RefCount++;
			}
		}
	}

	return RefCount;
}


void  FBlueprintEditorUtils::ModifyActorReferencedGraphNodes(ULevelScriptBlueprint* LevelScriptBlueprint, const AActor* InActor)
{
	TArray<UEdGraph*> AllGraphs;
	LevelScriptBlueprint->GetAllGraphs(AllGraphs);

	for(TArray<UEdGraph*>::TConstIterator it(AllGraphs); it; ++it)
	{
		const UEdGraph* CurrentGraph = *it;

		TArray<UK2Node*> GraphNodes;
		CurrentGraph->GetNodesOfClass(GraphNodes);

		for( TArray<UK2Node*>::TIterator NodeIt(GraphNodes); NodeIt; ++NodeIt )
		{
			UK2Node* CurrentNode = *NodeIt;
			if( CurrentNode != NULL && CurrentNode->GetReferencedLevelActor() == InActor )
			{
				CurrentNode->Modify();
			}
		}
	}
}

void FBlueprintEditorUtils::FindActorsThatReferenceActor( AActor* InActor, TArray<UClass*>& InClassesToIgnore, TArray<AActor*>& OutReferencingActors )
{
	// Iterate all actors in the same world as InActor
	for ( FActorIterator ActorIt(InActor->GetWorld()); ActorIt; ++ActorIt )
	{
		AActor* CurrentActor = *ActorIt;
		if (( CurrentActor ) && ( CurrentActor != InActor ))
		{
			bool bShouldIgnore = false;
			// Ignore Actors that aren't in the same level as InActor - cross level references are not allowed, so it's safe to ignore.
			if ( !CurrentActor->IsInLevel(InActor->GetLevel() ) )
			{
				bShouldIgnore = true;
			}
			// Ignore Actors if they are of a type we were instructed to ignore.
			for ( int32 IgnoreIndex = 0; IgnoreIndex < InClassesToIgnore.Num() && !bShouldIgnore; IgnoreIndex++ )
			{
				if ( CurrentActor->IsA( InClassesToIgnore[IgnoreIndex] ) )
				{
					bShouldIgnore = true;
				}
			}

			if ( !bShouldIgnore )
			{
				// Get all references from CurrentActor and see if any are the Actor we're searching for
				TArray<UObject*> References;
				FReferenceFinder Finder( References );
				Finder.FindReferences( CurrentActor );

				if ( References.Contains( InActor ) )
				{
					OutReferencingActors.Add( CurrentActor );
				}
			}
		}
	}
}

FBlueprintEditorUtils::FFixLevelScriptActorBindingsEvent& FBlueprintEditorUtils::OnFixLevelScriptActorBindings()
{
	return FixLevelScriptActorBindingsEvent;
}


bool FBlueprintEditorUtils::FixLevelScriptActorBindings(ALevelScriptActor* LevelScriptActor, const ULevelScriptBlueprint* ScriptBlueprint)
{
	if( ScriptBlueprint->BlueprintType != BPTYPE_LevelScript )
	{
		return false;
	}

	bool bWasSuccessful = true;

	TArray<UEdGraph*> AllGraphs;
	ScriptBlueprint->GetAllGraphs(AllGraphs);

	// Iterate over all graphs, and find all bound event nodes
	for( TArray<UEdGraph*>::TConstIterator GraphIt(AllGraphs); GraphIt; ++GraphIt )
	{
		TArray<UK2Node_ActorBoundEvent*> BoundEvents;
		(*GraphIt)->GetNodesOfClass(BoundEvents);

		for( TArray<UK2Node_ActorBoundEvent*>::TConstIterator NodeIt(BoundEvents); NodeIt; ++NodeIt )
		{
			UK2Node_ActorBoundEvent* EventNode = *NodeIt;

			// For each bound event node, verify that we have an entry point in the LSA, and add a delegate to the target
			if( EventNode && EventNode->EventOwner )
			{
				const FName TargetFunction = EventNode->CustomFunctionName;

				// Check to make sure the level scripting actor actually has the function defined
				if( LevelScriptActor->FindFunction(TargetFunction) )
				{
					// Grab the MC delegate we need to add to
					FMulticastScriptDelegate* TargetDelegate = EventNode->GetTargetDelegate();
					check(TargetDelegate);

					// Create the delegate, and add it if it doesn't already exist
					FScriptDelegate Delegate;
					Delegate.SetFunctionName(TargetFunction);
					Delegate.SetObject(LevelScriptActor);
					TargetDelegate->AddUnique(Delegate);
				}
				else
				{
					// For some reason, we don't have a valid entry point for the event in the LSA...
					UE_LOG(LogBlueprint, Warning, TEXT("Unable to bind event for node %s!  Please recompile the level blueprint."), (EventNode ? *EventNode->GetName() : TEXT("unknown")));
					bWasSuccessful = false;
				}
			}
		}

		// Find matinee controller nodes and update node name
		TArray<UK2Node_MatineeController*> MatineeControllers;
		(*GraphIt)->GetNodesOfClass(MatineeControllers);

		for( TArray<UK2Node_MatineeController*>::TConstIterator NodeIt(MatineeControllers); NodeIt; ++NodeIt )
		{
			const UK2Node_MatineeController* MatController = *NodeIt;

			if(MatController->MatineeActor != NULL)
			{
				MatController->MatineeActor->MatineeControllerName = MatController->GetFName();
			}
		}
	}

	// Allow external sub-systems perform changes to the level script actor
	FixLevelScriptActorBindingsEvent.Broadcast( LevelScriptActor, ScriptBlueprint, bWasSuccessful );


	return bWasSuccessful;
}

void FBlueprintEditorUtils::ListPackageContents(UPackage* Package, FOutputDevice& Ar)
{
	Ar.Logf(TEXT("Package %s contains:"), *Package->GetName());
	for (FObjectIterator ObjIt; ObjIt; ++ObjIt)
	{
		if (ObjIt->GetOuter() == Package)
		{
			Ar.Logf(TEXT("  %s (flags 0x%X)"), *ObjIt->GetFullName(), (int32)ObjIt->GetFlags());
		}
	}
}

bool FBlueprintEditorUtils::KismetDiagnosticExec(const TCHAR* InStream, FOutputDevice& Ar)
{
	const TCHAR* Str = InStream;

	if (FParse::Command(&Str, TEXT("FindBadBlueprintReferences")))
	{
		// Collect garbage first to remove any false positives
		CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );

		UPackage* TransientPackage = GetTransientPackage();

		// Unique blueprints/classes that contain badness
		TSet<UObject*> ObjectsContainingBadness;
		TSet<UPackage*> BadPackages;

		// Run thru every object in the world
		for (FObjectIterator ObjectIt; ObjectIt; ++ObjectIt)
		{
			UObject* TestObj = *ObjectIt;

			// If the test object is itself transient, there is no concern
			if (TestObj->HasAnyFlags(RF_Transient))
			{
				continue;
			}


			// Look for a containing scope (either a blueprint or a class)
			UObject* OuterScope = NULL;
			for (UObject* TestOuter = TestObj; (TestOuter != NULL) && (OuterScope == NULL); TestOuter = TestOuter->GetOuter())
			{
				if (UClass* OuterClass = Cast<UClass>(TestOuter))
				{
					if (OuterClass->ClassGeneratedBy != NULL)
					{
						OuterScope = OuterClass;
					}
				}
				else if (UBlueprint* OuterBlueprint = Cast<UBlueprint>(TestOuter))
				{
					OuterScope = OuterBlueprint;
				}
			}

			if ((OuterScope != NULL) && !OuterScope->IsIn(TransientPackage))
			{
				// Find all references
				TArray<UObject*> ReferencedObjects;
				FReferenceFinder ObjectReferenceCollector(ReferencedObjects, NULL, false, false, false, false);
				ObjectReferenceCollector.FindReferences(TestObj);

				for (auto TouchedIt = ReferencedObjects.CreateConstIterator(); TouchedIt; ++TouchedIt)
				{
					UObject* ReferencedObj = *TouchedIt;

					// Ignore any references inside the outer blueprint or class; they're intrinsically safe
					if (!ReferencedObj->IsIn(OuterScope))
					{
						// If it's a public reference, that's fine
						if (!ReferencedObj->HasAnyFlags(RF_Public))
						{
							// It's a private reference outside of the parent object; not good!
							Ar.Logf(TEXT("%s has a reference to %s outside of it's container %s"),
								*TestObj->GetFullName(),
								*ReferencedObj->GetFullName(),
								*OuterScope->GetFullName()
								);
							ObjectsContainingBadness.Add(OuterScope);
							BadPackages.Add(OuterScope->GetOutermost());
						}
					}
				}
			}
		}

		// Report all the bad outers as text dumps so the exact property can be identified
		Ar.Logf(TEXT("Summary of assets containing objects that have bad references"));
		for (auto BadIt = ObjectsContainingBadness.CreateConstIterator(); BadIt; ++BadIt)
		{
			UObject* BadObj = *BadIt;
			Ar.Logf(TEXT("\n\nObject %s referenced private objects outside of it's container asset inappropriately"), *BadObj->GetFullName());

			UBlueprint* Blueprint = Cast<UBlueprint>(BadObj);
			if (Blueprint == NULL)
			{
				if (UClass* Class = Cast<UClass>(BadObj))
				{
					Blueprint = CastChecked<UBlueprint>(Class->ClassGeneratedBy);

					if (Blueprint->GeneratedClass == Class)
					{
						Ar.Logf(TEXT("  => GeneratedClass of %s"), *Blueprint->GetFullName());
					}
					else if (Blueprint->SkeletonGeneratedClass == Class)
					{
						Ar.Logf(TEXT("  => SkeletonGeneratedClass of %s"), *Blueprint->GetFullName());
					}
					else
					{
						Ar.Logf(TEXT("  => ***FALLEN BEHIND*** class generated by %s"), *Blueprint->GetFullName());
					}
					Ar.Logf(TEXT("  Has an associated CDO named %s"), *Class->GetDefaultObject()->GetFullName());
				}
			}

			// Export the asset to text
			if (true)
			{
				UnMarkAllObjects(EObjectMark(OBJECTMARK_TagExp | OBJECTMARK_TagImp));
				FStringOutputDevice Archive;
				const FExportObjectInnerContext Context;
				UExporter::ExportToOutputDevice(&Context, BadObj, NULL, Archive, TEXT("copy"), 0, /*PPF_IncludeTransient*/ /*PPF_ExportsNotFullyQualified|*/PPF_Copy, false, NULL);
				FString ExportedText = Archive;

				Ar.Logf(TEXT("%s"), *ExportedText);
			}
		}

		// Report the contents of the bad packages
		for (auto BadIt = BadPackages.CreateConstIterator(); BadIt; ++BadIt)
		{
			UPackage* BadPackage = *BadIt;
			
			Ar.Logf(TEXT("\nBad package %s contains:"), *BadPackage->GetName());
			for (FObjectIterator ObjIt; ObjIt; ++ObjIt)
			{
				if (ObjIt->GetOuter() == BadPackage)
				{
					Ar.Logf(TEXT("  %s"), *ObjIt->GetFullName());
				}
			}
		}

		Ar.Logf(TEXT("\nFinished listing illegal private references"));
	}
	else if (FParse::Command(&Str, TEXT("ListPackageContents")))
	{
		if (UPackage* Package = FindPackage(NULL, Str))
		{
			FBlueprintEditorUtils::ListPackageContents(Package, Ar);
		}
		else
		{
			Ar.Logf(TEXT("Failed to find package %s"), Str);
		}
	}
	else if (FParse::Command(&Str, TEXT("RepairBlueprint")))
	{
		if (UBlueprint* Blueprint = FindObject<UBlueprint>(ANY_PACKAGE, Str))
		{
			IKismetCompilerInterface& Compiler = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(KISMET_COMPILER_MODULENAME);
			Compiler.RecoverCorruptedBlueprint(Blueprint);
		}
		else
		{
			Ar.Logf(TEXT("Failed to find blueprint %s"), Str);
		}
	}
	else if (FParse::Command(&Str, TEXT("ListOrphanClasses")))
	{
		UE_LOG(LogBlueprintDebug, Log, TEXT("--- LISTING ORPHANED CLASSES ---"));
		for( TObjectIterator<UClass> it; it; ++it )
		{
			UClass* CurrClass = *it;
			if( CurrClass->ClassGeneratedBy != NULL && CurrClass->GetOutermost() != GetTransientPackage() )
			{
				if( UBlueprint* GeneratingBP = Cast<UBlueprint>(CurrClass->ClassGeneratedBy) )
				{
					if( CurrClass != GeneratingBP->GeneratedClass && CurrClass != GeneratingBP->SkeletonGeneratedClass )
					{
						UE_LOG(LogBlueprintDebug, Log, TEXT(" - %s"), *CurrClass->GetFullName());				
					}
				}	
			}
		}

		return true;
	}
	else if (FParse::Command(&Str, TEXT("ListRootSetObjects")))
	{
		UE_LOG(LogBlueprintDebug, Log, TEXT("--- LISTING ROOTSET OBJ ---"));
		for( FObjectIterator it; it; ++it )
		{
			UObject* CurrObj = *it;
			if( CurrObj->HasAnyFlags(RF_RootSet) )
			{
				UE_LOG(LogBlueprintDebug, Log, TEXT(" - %s"), *CurrObj->GetFullName());
			}
		}
	}
	else
	{
		return false;
	}

	return true;
}


void FBlueprintEditorUtils::OpenReparentBlueprintMenu( UBlueprint* Blueprint, const TSharedRef<SWidget>& ParentContent, const FOnClassPicked& OnPicked)
{
	TArray< UBlueprint* > Blueprints;
	Blueprints.Add( Blueprint );
	OpenReparentBlueprintMenu( Blueprints, ParentContent, OnPicked );
}

class FBlueprintReparentFilter : public IClassViewerFilter
{
public:
	/** All children of these classes will be included unless filtered out by another setting. */
	TSet< const UClass* > AllowedChildrenOfClasses;

	/** Classes to not allow any children of into the Class Viewer/Picker. */
	TSet< const UClass* > DisallowedChildrenOfClasses;

	/** Classes to never show in this class viewer. */
	TSet< const UClass* > DisallowedClasses;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs ) OVERRIDE
	{
		// If it appears on the allowed child-of classes list (or there is nothing on that list)
		//		AND it is NOT on the disallowed child-of classes list
		//		AND it is NOT on the disallowed classes list
		return InFilterFuncs->IfInChildOfClassesSet( AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed && 
			InFilterFuncs->IfInChildOfClassesSet(DisallowedChildrenOfClasses, InClass) != EFilterReturn::Passed && 
			InFilterFuncs->IfInClassesSet(DisallowedClasses, InClass) != EFilterReturn::Passed;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) OVERRIDE
	{
		// If it appears on the allowed child-of classes list (or there is nothing on that list)
		//		AND it is NOT on the disallowed child-of classes list
		//		AND it is NOT on the disallowed classes list
		return InFilterFuncs->IfInChildOfClassesSet( AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed && 
			InFilterFuncs->IfInChildOfClassesSet(DisallowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Passed && 
			InFilterFuncs->IfInClassesSet(DisallowedClasses, InUnloadedClassData) != EFilterReturn::Passed;
	}
};

TSharedRef<SWidget> FBlueprintEditorUtils::ConstructBlueprintParentClassPicker( const TArray< UBlueprint* >& Blueprints, const FOnClassPicked& OnPicked)
{
	bool bIsActor = false;
	bool bIsAnimBlueprint = false;
	bool bIsLevelScriptActor = false;
	TArray<UClass*> BlueprintClasses;
	for( auto BlueprintIter = Blueprints.CreateConstIterator(); (!bIsActor && !bIsAnimBlueprint) && BlueprintIter; ++BlueprintIter )
	{
		const auto Blueprint = *BlueprintIter;
		bIsActor |= Blueprint->ParentClass->IsChildOf( AActor::StaticClass() );
		bIsAnimBlueprint |= Blueprint->IsA(UAnimBlueprint::StaticClass());
		bIsLevelScriptActor |= Blueprint->ParentClass->IsChildOf( ALevelScriptActor::StaticClass() );
		BlueprintClasses.Add(Blueprint->GeneratedClass);
	}

	// Fill in options
	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;

	TSharedPtr<FBlueprintReparentFilter> Filter = MakeShareable(new FBlueprintReparentFilter);
	Options.ClassFilter = Filter;
	Options.ViewerTitleString = LOCTEXT("ReparentBblueprint", "Reparent blueprint").ToString();

	// Only allow parenting to base blueprints.
	Options.bIsBlueprintBaseOnly = true;

	// never allow parenting to Interface
	Filter->DisallowedChildrenOfClasses.Add( UInterface::StaticClass() );

	// never allow parenting to children of itself
	for( auto ClassIt = BlueprintClasses.CreateIterator(); ClassIt; ++ClassIt )
	{
		Filter->DisallowedChildrenOfClasses.Add(*ClassIt);
	}

	if(bIsActor)
	{
		if(bIsLevelScriptActor)
		{
			// Don't allow conversion outside of the LevelScriptActor hierarchy
			Filter->AllowedChildrenOfClasses.Add( ALevelScriptActor::StaticClass() );
		}
		else
		{
			// Don't allow conversion outside of the Actor hierarchy
			Filter->AllowedChildrenOfClasses.Add( AActor::StaticClass() );
		}
	}

	if(!bIsLevelScriptActor)
	{
		// Don't allow non-LevelScriptActor->LevelScriptActor conversion
		Filter->DisallowedChildrenOfClasses.Add( ALevelScriptActor::StaticClass() );
	}

	if (bIsAnimBlueprint)
	{
		// If it's an anim blueprint, do not allow conversion to non anim
		Filter->AllowedChildrenOfClasses.Add( UAnimInstance::StaticClass() );
	}

	for( auto BlueprintIter = Blueprints.CreateConstIterator(); BlueprintIter; ++BlueprintIter )
	{
		const auto Blueprint = *BlueprintIter;

		// don't allow making me my own parent!
		Filter->DisallowedClasses.Add(Blueprint->GeneratedClass);
	}

	return FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(Options, OnPicked);
}

void FBlueprintEditorUtils::OpenReparentBlueprintMenu( const TArray< UBlueprint* >& Blueprints, const TSharedRef<SWidget>& ParentContent, const FOnClassPicked& OnPicked)
{
	if ( Blueprints.Num() == 0 )
	{
		return;
	}

	TSharedRef<SWidget> ClassPicker = ConstructBlueprintParentClassPicker(Blueprints, OnPicked);

	TSharedRef<SBorder> ClassPickerBorder = 
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
		[
			ClassPicker
		];

	// Show dialog to choose new parent class
	FSlateApplication::Get().PushMenu(
		ParentContent,
		ClassPickerBorder,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu),
		true,
		false,
		FVector2D(280, 400)
		);
}

void FBlueprintEditorUtils::UpdateOldPureFunctions( UBlueprint* Blueprint )
{
	if(UBlueprintGeneratedClass* GeneratedClass = Cast<UBlueprintGeneratedClass>(Blueprint->SkeletonGeneratedClass))
	{
		for(auto Function : TFieldRange<UFunction>(GeneratedClass, EFieldIteratorFlags::ExcludeSuper))
		{
			if(Function && ((Function->FunctionFlags & FUNC_BlueprintPure) > 0))
			{
				for(auto FuncGraphsIt(Blueprint->FunctionGraphs.CreateIterator());FuncGraphsIt;++FuncGraphsIt)
				{
					UEdGraph* FunctionGraph = *FuncGraphsIt;
					if(FunctionGraph->GetName() == Function->GetName())
					{
						TArray<UK2Node_FunctionEntry*> EntryNodes;
						FunctionGraph->GetNodesOfClass(EntryNodes);

						if( EntryNodes.Num() > 0 )
						{
							UK2Node_FunctionEntry* Entry = EntryNodes[0];
							Entry->Modify();
							Entry->ExtraFlags |= FUNC_BlueprintPure;
						}
					}
				}
			}
		}
	}
}

/** Call PostEditChange() on any Actors that are based on this Blueprint */
void FBlueprintEditorUtils::PostEditChangeBlueprintActors(UBlueprint* Blueprint)
{
	for(TObjectIterator<AActor> ActorIt; ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		if(Actor->GetClass()->IsChildOf(Blueprint->GeneratedClass) && !Actor->IsPendingKill())
		{
			Actor->PostEditChange();

			// Let components that got re-regsitered by PostEditChange also get begun play
			if(Actor->bActorInitialized)
			{
				Actor->InitializeComponents();
			}
		}
	}

	// Let the blueprint thumbnail renderer know that a blueprint has been modified so it knows to reinstance components for visualization
	FThumbnailRenderingInfo* RenderInfo = GUnrealEd->GetThumbnailManager()->GetRenderingInfo( Blueprint );
	if ( RenderInfo != NULL )
	{
		UBlueprintThumbnailRenderer* BlueprintThumbnailRenderer = Cast<UBlueprintThumbnailRenderer>(RenderInfo->Renderer);
		if ( BlueprintThumbnailRenderer != NULL )
		{
			BlueprintThumbnailRenderer->BlueprintChanged(Blueprint);
		}
	}
}

bool FBlueprintEditorUtils::IsPropertyReadOnlyInCurrentBlueprint(const UBlueprint* Blueprint, const UProperty* Property)
{
	if (Property && Property->HasAnyPropertyFlags(CPF_BlueprintReadOnly))
	{
		return true;
	}
	if (Property && Property->GetBoolMetaData(FBlueprintMetadata::MD_Private))
	{
		const UClass* OwningClass = CastChecked<UClass>(Property->GetOuter());
		if(OwningClass->ClassGeneratedBy != Blueprint)
		{
			return true;
		}
	}
	return false;
}

void FBlueprintEditorUtils::FindAndSetDebuggableBlueprintInstances()
{
	TMap< UBlueprint*, TArray< AActor* > > BlueprintsNeedingInstancesToDebug;

	// Find open blueprint editors that have no debug instances
	FAssetEditorManager& AssetEditorManager = FAssetEditorManager::Get();	
	TArray<UObject*> EditedAssets = AssetEditorManager.GetAllEditedAssets();		
	for(int32 i=0; i<EditedAssets.Num(); i++)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>( EditedAssets[i] );
		if( Blueprint != NULL )
		{
			if( Blueprint->GetObjectBeingDebugged() == NULL )
			{
				BlueprintsNeedingInstancesToDebug.FindOrAdd( Blueprint );
			}			
		}
	}

	// If we have blueprints with no debug objects selected try to find a suitable on to debug
	if( BlueprintsNeedingInstancesToDebug.Num() != 0 )
	{	
		// Priority is in the following order.
		// 1. Selected objects with the exact same type as the blueprint being debugged
		// 2. UnSelected objects with the exact same type as the blueprint being debugged
		// 3. Selected objects based on the type of blueprint being debugged
		// 4. UnSelected objects based on the type of blueprint being debugged
		USelection* Selected = GEditor->GetSelectedActors();
		const bool bDisAllowDerivedTypes = false;
		TArray< UBlueprint* > BlueprintsToRefresh;
		for (TMap< UBlueprint*, TArray< AActor* > >::TIterator ObjIt(BlueprintsNeedingInstancesToDebug); ObjIt; ++ObjIt)
		{	
			UBlueprint* EachBlueprint = ObjIt.Key();
			bool bFoundItemToDebug = false;
			AActor* SimilarInstanceSelected = NULL;
			AActor* SimilarInstanceUnselected = NULL;

			// First check selected objects.
			if( Selected->Num() != 0 )
			{
				for (int32 iSelected = 0; iSelected < Selected->Num() ; iSelected++)
				{
					AActor* ObjectAsActor = Cast<AActor>( Selected->GetSelectedObject( iSelected ) );

					if ((ObjectAsActor != NULL) && (ObjectAsActor->GetWorld() != NULL) && (ObjectAsActor->GetWorld()->WorldType != EWorldType::Preview))
					{
						if( IsObjectADebugCandidate(ObjectAsActor, EachBlueprint, true/*bInDisallowDerivedBlueprints*/ ) == true )
						{
							EachBlueprint->SetObjectBeingDebugged( ObjectAsActor );
							bFoundItemToDebug = true;
							BlueprintsToRefresh.Add( EachBlueprint );
							break;
						}
						else if( SimilarInstanceSelected == NULL)
						{
							// If we haven't found a similar selected instance already check for one now
							if( IsObjectADebugCandidate(ObjectAsActor, EachBlueprint, false/*bInDisallowDerivedBlueprints*/ ) == true )
							{
								SimilarInstanceSelected = ObjectAsActor;
							}
						}
					}
				}
			}
			// Nothing of this type selected, just find any instance of one of these objects.
			if (!bFoundItemToDebug)
			{
				for (TObjectIterator<UObject> It; It; ++It)
				{
					AActor* ObjectAsActor = Cast<AActor>( *It );
					if( ObjectAsActor && ( ObjectAsActor->GetWorld() ) && ( ObjectAsActor->GetWorld()->WorldType != EWorldType::Preview) )
					{
						if( IsObjectADebugCandidate(ObjectAsActor, EachBlueprint, true/*bInDisallowDerivedBlueprints*/ ) == true )
						{
							EachBlueprint->SetObjectBeingDebugged( ObjectAsActor );
							bFoundItemToDebug = true;
							BlueprintsToRefresh.Add( EachBlueprint );
							break;
						}
						else if( SimilarInstanceUnselected == NULL)
						{
							// If we haven't found a similar unselected instance already check for one now
							if( IsObjectADebugCandidate(ObjectAsActor, EachBlueprint, false/*bInDisallowDerivedBlueprints*/ ) == true )
							{
								SimilarInstanceUnselected = ObjectAsActor;
							}						
						}
					}
				}
			}

			// If we didn't find and exact type match, but we did find a related type use that.
			if( bFoundItemToDebug == false )
			{
				if( ( SimilarInstanceSelected != NULL ) || ( SimilarInstanceUnselected != NULL ) )
				{
					EachBlueprint->SetObjectBeingDebugged( SimilarInstanceSelected != NULL ? SimilarInstanceSelected : SimilarInstanceUnselected );
					BlueprintsToRefresh.Add( EachBlueprint );
				}
			}
		}

		// Refresh all blueprint windows that we have made a change to the debugging selection of
		for (int32 iRefresh = 0; iRefresh < BlueprintsToRefresh.Num() ; iRefresh++)
		{
			// Ensure its a blueprint editor !
			TSharedPtr< IToolkit > FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(BlueprintsToRefresh[ iRefresh]);
			if (FoundAssetEditor.IsValid() && FoundAssetEditor->IsBlueprintEditor())
			{
				TSharedPtr<IBlueprintEditor> BlueprintEditor = StaticCastSharedPtr<IBlueprintEditor>(FoundAssetEditor);
				BlueprintEditor->RefreshEditors();
			}
		}
	}
}

void FBlueprintEditorUtils::AnalyticsTrackNewNode( UEdGraphNode* NewNode, FName NodeClass, FName NodeType )
{
	UBlueprint* Blueprint = FindBlueprintForNodeChecked(NewNode);
	TSharedPtr<IToolkit> FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(Blueprint); 
	if (FoundAssetEditor.IsValid() && FoundAssetEditor->IsBlueprintEditor()) 
	{ 
		TSharedPtr<IBlueprintEditor> BlueprintEditor = StaticCastSharedPtr<IBlueprintEditor>(FoundAssetEditor);
		BlueprintEditor->AnalyticsTrackNewNode(NodeClass, NodeType);
	}
}

bool FBlueprintEditorUtils::IsObjectADebugCandidate( AActor* InActorObject, UBlueprint* InBlueprint, bool bInDisallowDerivedBlueprints )
{
	const bool bPassesFlags = !InActorObject->HasAnyFlags(RF_PendingKill | RF_ClassDefaultObject);
	bool bCanDebugThisObject;
	if( bInDisallowDerivedBlueprints == true )
	{
		bCanDebugThisObject = InActorObject->GetClass()->ClassGeneratedBy == InBlueprint;
	}
	else
	{
		bCanDebugThisObject = InActorObject->IsA( InBlueprint->GeneratedClass );
	}
	
	return bPassesFlags && bCanDebugThisObject;
}

bool FBlueprintEditorUtils::PropertyValueFromString(const UProperty* Property, const FString& Value, uint8* DefaultObject)
{
	bool bParseSuccedded = true;
	if( !Property->IsA(UStructProperty::StaticClass()) )
	{
		if( Property->IsA(UIntProperty::StaticClass()) )
		{
			int32 IntValue = 0;
			bParseSuccedded = FDefaultValueHelper::ParseInt(Value, IntValue);
			CastChecked<UIntProperty>(Property)->SetPropertyValue_InContainer(DefaultObject, IntValue);
		}
		else if( Property->IsA(UFloatProperty::StaticClass()) )
		{
			float FloatValue = 0.0f;
			bParseSuccedded = FDefaultValueHelper::ParseFloat(Value, FloatValue);
			CastChecked<UFloatProperty>(Property)->SetPropertyValue_InContainer(DefaultObject, FloatValue);
		}
		else if (const UByteProperty* ByteProperty = Cast<const UByteProperty>(Property))
		{
			int32 IntValue = 0;
			if (const UEnum* Enum = ByteProperty->Enum)
			{
				IntValue = Enum->FindEnumIndex(FName(*Value));
				bParseSuccedded = (INDEX_NONE != IntValue);
			}
			else
			{
				bParseSuccedded = FDefaultValueHelper::ParseInt(Value, IntValue);
			}
			bParseSuccedded = bParseSuccedded && ( IntValue <= 255 ) && ( IntValue >= 0 );
			ByteProperty->SetPropertyValue_InContainer(DefaultObject, IntValue);
		}
		else if( Property->IsA(UStrProperty::StaticClass()) )
		{
			CastChecked<UStrProperty>(Property)->SetPropertyValue_InContainer(DefaultObject, Value);
		}
		else if( Property->IsA(UBoolProperty::StaticClass()) )
		{
			CastChecked<UBoolProperty>(Property)->SetPropertyValue_InContainer(DefaultObject, Value.ToBool());
		}
		else if( Property->IsA(UNameProperty::StaticClass()) )
		{
			CastChecked<UNameProperty>(Property)->SetPropertyValue_InContainer(DefaultObject, FName(*Value));
		}
		else if( Property->IsA(UTextProperty::StaticClass()) )
		{
			CastChecked<UTextProperty>(Property)->SetPropertyValue_InContainer(DefaultObject, FText::FromString(Value));
		}
		else if( Property->IsA(UClassProperty::StaticClass()) )
		{
			CastChecked<UClassProperty>(Property)->SetPropertyValue_InContainer(DefaultObject, FindObject<UClass>(ANY_PACKAGE, *Value, true));
		}
		else if( Property->IsA(UObjectPropertyBase::StaticClass()) )
		{
			CastChecked<UObjectPropertyBase>(Property)->SetObjectPropertyValue_InContainer(DefaultObject, FindObject<UObject>(ANY_PACKAGE, *Value));
		}
		else if( Property->IsA(UArrayProperty::StaticClass()) )
		{
			const UArrayProperty* ArrayProp = CastChecked<UArrayProperty>(Property);
			ArrayProp->ImportText(*Value, ArrayProp->ContainerPtrToValuePtr<uint8>(DefaultObject), 0, NULL);
		}
		else if( Property->IsA(UMulticastDelegateProperty::StaticClass()) )
		{
			const UMulticastDelegateProperty* MCDelegateProp = CastChecked<UMulticastDelegateProperty>(Property);
			MCDelegateProp->ImportText(*Value, MCDelegateProp->ContainerPtrToValuePtr<uint8>(DefaultObject), 0, NULL);
		}
		else
		{
			// HOOK UP NEW TYPES HERE
			check(0);
		}

	}
	else 
	{
		static UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));
		static UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Rotator"));
		static UScriptStruct* TransformStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Transform"));
		static UScriptStruct* LinearColorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("LinearColor"));

		const UStructProperty* StructProperty = CastChecked<UStructProperty>(Property);

		// Struct properties must be handled differently, unfortunately.  We only support FVector, FRotator, and FTransform
		if( StructProperty->Struct == VectorStruct )
		{
			FVector V = FVector::ZeroVector;
			bParseSuccedded = FDefaultValueHelper::ParseVector(Value, V);
			Property->CopyCompleteValue( Property->ContainerPtrToValuePtr<uint8>(DefaultObject), &V );
		}
		else if( StructProperty->Struct == RotatorStruct )
		{
			FRotator R = FRotator::ZeroRotator;
			bParseSuccedded = FDefaultValueHelper::ParseRotator(Value, R);
			Property->CopyCompleteValue( Property->ContainerPtrToValuePtr<uint8>(DefaultObject), &R );
		}
		else if( StructProperty->Struct == TransformStruct )
		{
			FTransform T = FTransform::Identity;
			bParseSuccedded = T.InitFromString( Value );
			Property->CopyCompleteValue( Property->ContainerPtrToValuePtr<uint8>(DefaultObject), &T );
		}
		else if( StructProperty->Struct == LinearColorStruct )
		{
			FLinearColor Color;
			// Color form: "(R=%f,G=%f,B=%f,A=%f)"
			bParseSuccedded = Color.InitFromString(Value);
			Property->CopyCompleteValue( Property->ContainerPtrToValuePtr<uint8>(DefaultObject), &Color );
		}
	}

	return bParseSuccedded;
}

FName FBlueprintEditorUtils::GenerateUniqueGraphName(UBlueprint* const BlueprintOuter, FString const& ProposedName)
{
	FName UniqueGraphName(*ProposedName);

	int32 CountPostfix = 1;
	while (!FBlueprintEditorUtils::IsGraphNameUnique(BlueprintOuter, UniqueGraphName))
	{
		UniqueGraphName = FName(*FString::Printf(TEXT("%s%i"), *ProposedName, CountPostfix));
		++CountPostfix;
	}

	return UniqueGraphName;
}

bool FBlueprintEditorUtils::CheckIfNodeConnectsToSelection(UEdGraphNode* InNode, const TSet<UEdGraphNode*>& InSelectionSet)
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	for (int32 PinIndex = 0; PinIndex < InNode->Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* Pin = InNode->Pins[PinIndex];
		if(Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != Schema->PC_Exec)
		{
			for (int32 LinkIndex = 0; LinkIndex < Pin->LinkedTo.Num(); ++LinkIndex)
			{
				UEdGraphPin* LinkedToPin = Pin->LinkedTo[LinkIndex];

				// The InNode, which is NOT in the new function, is checking if one of it's pins IS in the function, return true if it is. If not, check the node.
				if(InSelectionSet.Contains(LinkedToPin->GetOwningNode()))
				{
					return true;
				}

				// Check the node recursively to see if it is connected back with selection.
				if(CheckIfNodeConnectsToSelection(LinkedToPin->GetOwningNode(), InSelectionSet))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool FBlueprintEditorUtils::CheckIfSelectionIsCycling(const TSet<UEdGraphNode*>& InSelectionSet, FCompilerResultsLog& InMessageLog)
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	for (auto NodeIt = InSelectionSet.CreateConstIterator(); NodeIt; ++NodeIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
		{
			for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
			{
				UEdGraphPin* Pin = Node->Pins[PinIndex];
				if(Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != Schema->PC_Exec)
				{
					for (int32 LinkIndex = 0; LinkIndex < Pin->LinkedTo.Num(); ++LinkIndex)
					{
						UEdGraphPin* LinkedToPin = Pin->LinkedTo[LinkIndex];

						// Check to see if this node, which is IN the selection, has any connections OUTSIDE the selection
						// If it does, check to see if those nodes have any connections IN the selection
						if(!InSelectionSet.Contains(LinkedToPin->GetOwningNode()))
						{
							if(CheckIfNodeConnectsToSelection(LinkedToPin->GetOwningNode(), InSelectionSet))
							{
								InMessageLog.Error(*LOCTEXT("DependencyCyleDetected_Error", "Dependency cycle detected, preventing node @@ from being scheduled").ToString(), LinkedToPin->GetOwningNode());
								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}

bool FBlueprintEditorUtils::IsPaletteActionReadOnly(TSharedPtr<FEdGraphSchemaAction> ActionIn, TSharedPtr<FBlueprintEditor> const BlueprintEditorIn)
{
	check(BlueprintEditorIn.IsValid());
	UBlueprint const* const BlueprintObj = BlueprintEditorIn->GetBlueprintObj();

	bool bIsReadOnly = false;
	if(ActionIn->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Graph* GraphAction = (FEdGraphSchemaAction_K2Graph*)ActionIn.Get();
		if ( (GraphAction->EdGraph != NULL) && !((GraphAction->EdGraph->bAllowDeletion || GraphAction->EdGraph->bAllowRenaming)) )
		{
			bIsReadOnly = true;
		}
	}
	else if(ActionIn->GetTypeId() == FEdGraphSchemaAction_K2Var::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Var* VarAction = (FEdGraphSchemaAction_K2Var*)ActionIn.Get();

		bIsReadOnly = true;

		if( FBlueprintEditorUtils::FindNewVariableIndex(BlueprintObj, VarAction->GetVariableName()) != INDEX_NONE)
		{
			bIsReadOnly = false;
		}
		else if(BlueprintObj->FindTimelineTemplateByVariableName(VarAction->GetVariableName()))
		{
			bIsReadOnly = false;
		}
		else if(BlueprintEditorIn->CanAccessComponentsMode())
		{
			// Wasn't in the introduced variable list; try to find the associated SCS node
			//@TODO: The SCS-generated variables should be in the variable list and have a link back;
			// As it stands, you cannot do any metadata operations on a SCS variable, and you have to do icky code like the following
			TArray<USCS_Node*> Nodes = BlueprintObj->SimpleConstructionScript->GetAllNodes();
			for (TArray<USCS_Node*>::TConstIterator NodeIt(Nodes); NodeIt; ++NodeIt)
			{
				USCS_Node* CurrentNode = *NodeIt;
				if (CurrentNode->VariableName == VarAction->GetVariableName())
				{
					bIsReadOnly = false;
					break;
				}
			}
		}
	}
	else if(ActionIn->GetTypeId() == FEdGraphSchemaAction_K2Delegate::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Delegate* DelegateAction = (FEdGraphSchemaAction_K2Delegate*)ActionIn.Get();

		if( FBlueprintEditorUtils::FindNewVariableIndex(BlueprintObj, DelegateAction->GetDelegateName()) == INDEX_NONE)
		{
			bIsReadOnly = true;
		}
	}
	else if (ActionIn->GetTypeId() == FEdGraphSchemaAction_K2Event::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Event* EventAction = (FEdGraphSchemaAction_K2Event*)ActionIn.Get();
		UK2Node* AssociatedNode = EventAction->NodeTemplate;

		bIsReadOnly = (AssociatedNode == NULL) || (!AssociatedNode->bCanRenameNode);	
	}

	return bIsReadOnly;
}


#undef LOCTEXT_NAMESPACE


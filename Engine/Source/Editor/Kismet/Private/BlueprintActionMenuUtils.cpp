// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "BlueprintActionMenuUtils.h"
#include "BlueprintActionMenuBuilder.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "KismetEditorUtilities.h"	// for CanPasteNodes()
#include "K2Node_Variable.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ActorBoundEvent.h"
#include "K2Node_ComponentBoundEvent.h"
#include "EdGraphSchema_K2.h"		// for bUseLegacyActionMenus 
#include "BlueprintEditorUtils.h"	// for DoesSupportComponents()

#define LOCTEXT_NAMESPACE "BlueprintActionMenuUtils"

/*******************************************************************************
* Static FBlueprintActionMenuUtils Helpers
******************************************************************************/

namespace BlueprintActionMenuUtilsImpl
{
	/**
	 * 
	 * 
	 * @param  Filter	
	 * @param  BlueprintAction	
	 * @return 
	 */
	static bool IsUnBoundSpawner(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * 
	 * 
	 * @param  ClassSet	
	 * @return 
	 */
	static UClass* FindCommonBaseClass(TArray<UClass*> const& ClassSet);
}

//------------------------------------------------------------------------------
static bool BlueprintActionMenuUtilsImpl::IsUnBoundSpawner(FBlueprintActionFilter const& /*Filter*/, FBlueprintActionInfo& BlueprintAction)
{
	return (BlueprintAction.GetBindings().Num() <= 0);
}

//------------------------------------------------------------------------------
static UClass* BlueprintActionMenuUtilsImpl::FindCommonBaseClass(TArray<UClass*> const& ClassSet)
{
	UClass* CommonClass = UObject::StaticClass();
	if (ClassSet.Num() > 0)
	{
		CommonClass = ClassSet[0];
		for (UClass const* Class : ClassSet)
		{
			while (!Class->IsChildOf(CommonClass))
			{
				CommonClass = CommonClass->GetSuperClass();
			}
		}
	}	
	return CommonClass;
}

/*******************************************************************************
 * FBlueprintActionMenuUtils
 ******************************************************************************/

//------------------------------------------------------------------------------
void FBlueprintActionMenuUtils::MakePaletteMenu(FBlueprintActionContext const& Context, UClass* FilterClass, FBlueprintActionMenuBuilder& MenuOut)
{
	MenuOut.Empty();
	
	uint32 FilterFlags = 0x00;
	if (FilterClass != nullptr)
	{
		// make sure we exclude global and static library actions
		FilterFlags = FBlueprintActionFilter::BPFILTER_RejectPersistentNonTargetFields;
	}
	
	FBlueprintActionFilter MenuFilter(FilterFlags);
	MenuFilter.Context = Context;
	
	// self member variables can be accessed through the MyBluprint panel (even
	// inherited ones)... external variables can be accessed through the context
	// menu (don't want to clutter the palette, I guess?)
	MenuFilter.RejectedNodeTypes.Add(UK2Node_Variable::StaticClass());
	
	if (FilterClass != nullptr)
	{
		MenuFilter.TargetClasses.Add(FilterClass);
	}

	MenuOut.AddMenuSection(MenuFilter, LOCTEXT("PaletteRoot", "Library"), /*MenuOrder =*/0, FBlueprintActionMenuBuilder::ConsolidatePropertyActions);
	MenuOut.RebuildActionList();
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuUtils::MakeContextMenu(FBlueprintActionContext const& Context, bool bIsContextSensitive, FBlueprintActionMenuBuilder& MenuOut)
{
	using namespace BlueprintActionMenuUtilsImpl;

	//--------------------------------------
	// Composing Filters
	//--------------------------------------

	FBlueprintActionFilter MenuFilter;
	MenuFilter.Context = Context;
	MenuFilter.Context.SelectedObjects.Empty();

	FBlueprintActionFilter ComponentsFilter;
	ComponentsFilter.Context = Context;
	ComponentsFilter.PermittedNodeTypes.Add(UK2Node_CallFunction::StaticClass());
	ComponentsFilter.PermittedNodeTypes.Add(UK2Node_ComponentBoundEvent::StaticClass());
	// only want bound actions for this menu section
	ComponentsFilter.AddRejectionTest(FBlueprintActionFilter::FRejectionTestDelegate::CreateStatic(IsUnBoundSpawner));

	FBlueprintActionFilter LevelActorsFilter;
	LevelActorsFilter.Context = Context;
	LevelActorsFilter.PermittedNodeTypes.Add(UK2Node_CallFunction::StaticClass());
	LevelActorsFilter.PermittedNodeTypes.Add(UK2Node_ActorBoundEvent::StaticClass());
	LevelActorsFilter.PermittedNodeTypes.Add(UK2Node_Literal::StaticClass());
	// only want bound actions for this menu section
	LevelActorsFilter.AddRejectionTest(FBlueprintActionFilter::FRejectionTestDelegate::CreateStatic(IsUnBoundSpawner));

	// make sure the bound menu sections have the proper OwnerClasses specified
	for (UObject* Selection : Context.SelectedObjects)
	{
		if (UObjectProperty* ObjProperty = Cast<UObjectProperty>(Selection))
		{
			ComponentsFilter.TargetClasses.Add(ObjProperty->PropertyClass);
			LevelActorsFilter.Context.SelectedObjects.Remove(Selection);
		}
		else if (AActor* LevelActor = Cast<AActor>(Selection))
		{
			ComponentsFilter.Context.SelectedObjects.Remove(Selection);
			// the loop below (over the editor's selected actors) will add to 
			// LevelActorsFilter's OwnerClasses
		}
		else
		{
			ComponentsFilter.Context.SelectedObjects.Remove(Selection);
			LevelActorsFilter.Context.SelectedObjects.Remove(Selection);
		}
	}

	// make sure all selected level actors are accounted for (in case the caller
	// did not include them in the context)
	for (FSelectionIterator LvlActorIt(*GEditor->GetSelectedActors()); LvlActorIt; ++LvlActorIt)
	{
		AActor* LevelActor = Cast<AActor>(*LvlActorIt);
		LevelActorsFilter.Context.SelectedObjects.AddUnique(LevelActor);
		LevelActorsFilter.TargetClasses.Add(LevelActor->GetClass());
	}

	bool const bOnlyBlueprintMembers = bIsContextSensitive && (MenuFilter.Context.Pins.Num() == 0);
	bool bCanOperateOnLevelActors = bIsContextSensitive;
	bool bCanHaveActorComponents  = bIsContextSensitive;
	// determine if we can operate on certain object selections (level actors, 
	// components, etc.)
	for (UBlueprint* Blueprint : Context.Blueprints)
	{
		UClass* BlueprintClass = Blueprint->SkeletonGeneratedClass;
		if (BlueprintClass != nullptr)
		{
			bCanOperateOnLevelActors &= BlueprintClass->IsChildOf<ALevelScriptActor>();
			if (bOnlyBlueprintMembers)
			{
				MenuFilter.TargetClasses.Add(BlueprintClass);
			}
		}
		bCanHaveActorComponents &= FBlueprintEditorUtils::DoesSupportComponents(Blueprint);
	}

	//--------------------------------------
	// Defining Menu Sections
	//--------------------------------------

	static int32 const MainMenuSectionGroup   = 000;
	static int32 const ComponentsSectionGroup = 100;
	static int32 const LevelActorSectionGroup = 101;

	MenuOut.Empty();

	bool const bAddComponentsSection = bIsContextSensitive && bCanHaveActorComponents && (ComponentsFilter.Context.SelectedObjects.Num() > 0);
	// add the components section to the menu (if we don't have any components
	// selected, then inform the user through a dummy menu entry)
	if (bAddComponentsSection)
	{
		MenuOut.AddMenuSection(ComponentsFilter, FText::GetEmpty(), ComponentsSectionGroup);
	}

	bool const bAddLevelActorsSection = bIsContextSensitive && bCanOperateOnLevelActors && (LevelActorsFilter.Context.SelectedObjects.Num() > 0);
	// add the level actor section to the menu
	if (bAddLevelActorsSection)
	{
		// since we're consolidating all the bound actions, then we have to pick 
		// one common base class to filter by
		UClass* CommonClass = FindCommonBaseClass(LevelActorsFilter.TargetClasses);
		LevelActorsFilter.TargetClasses.Empty();
		LevelActorsFilter.TargetClasses.Add(CommonClass);

		MenuOut.AddMenuSection(LevelActorsFilter, FText::GetEmpty(), LevelActorSectionGroup);
	}

	if (!bIsContextSensitive)
	{
		MenuFilter.Context.Pins.Empty();
	}
	MenuOut.AddMenuSection(MenuFilter);

	//--------------------------------------
	// Building the Menu
	//--------------------------------------

	MenuOut.RebuildActionList();

	UEditorExperimentalSettings const* ExperimentalSettings =  GetDefault<UEditorExperimentalSettings>();
	if (ExperimentalSettings->bUseRefactoredBlueprintMenuingSystem)
	{
		for (UEdGraph const* Graph : Context.Graphs)
		{
			if (FKismetEditorUtilities::CanPasteNodes(Graph))
			{
				// @TODO: Grey out menu option with tooltip if one of the nodes cannot paste into this graph
				TSharedPtr<FEdGraphSchemaAction> PasteHereAction(new FEdGraphSchemaAction_K2PasteHere(TEXT(""), LOCTEXT("PasteHereMenuName", "Paste here"), TEXT(""), MainMenuSectionGroup));
				MenuOut.AddAction(PasteHereAction);
				break;
			}
		}

		if (bIsContextSensitive && bCanHaveActorComponents && !bAddComponentsSection)
		{
			FText SelectComponentMsg = LOCTEXT("SelectComponentForEvents", "Select a Component to see available Events & Functions");
			FText SelectComponentToolTip = LOCTEXT("SelectComponentForEventsTooltip", "Select a Component in the MyBlueprint tab to see available Events and Functions in this menu.");
			TSharedPtr<FEdGraphSchemaAction> MsgAction = TSharedPtr<FEdGraphSchemaAction>(new FEdGraphSchemaAction_Dummy(TEXT(""), SelectComponentMsg, SelectComponentToolTip.ToString(), ComponentsSectionGroup));
			MenuOut.AddAction(MsgAction);
		}

		if (bIsContextSensitive && bCanOperateOnLevelActors && !bAddLevelActorsSection)
		{
			FText SelectActorsMsg = LOCTEXT("SelectActorForEvents", "Select Actor(s) to see available Events & Functions");
			FText SelectActorsToolTip = LOCTEXT("SelectActorForEventsTooltip", "Select Actor(s) in the level to see available Events and Functions in this menu.");
			TSharedPtr<FEdGraphSchemaAction> MsgAction = TSharedPtr<FEdGraphSchemaAction>(new FEdGraphSchemaAction_Dummy(TEXT(""), SelectActorsMsg, SelectActorsToolTip.ToString(), LevelActorSectionGroup));
			MenuOut.AddAction(MsgAction);
		}
	}
}

#undef LOCTEXT_NAMESPACE

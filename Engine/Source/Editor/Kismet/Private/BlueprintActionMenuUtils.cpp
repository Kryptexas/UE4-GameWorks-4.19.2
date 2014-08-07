// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "BlueprintActionMenuUtils.h"
#include "BlueprintActionMenuBuilder.h"
#include "BlueprintActionFilter.h"
#include "BlueprintBoundNodeSpawner.h"
#include "KismetEditorUtilities.h" // for CanPasteNodes()
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "EdGraphSchema_K2.h"

#define LOCTEXT_NAMESPACE "BlueprintActionMenuUtils"

/*******************************************************************************
* Static FBlueprintActionMenuUtils Helpers
******************************************************************************/

namespace BlueprintActionMenuUtilsImpl
{
	static bool IsUnBoundSpawner(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);
}

//------------------------------------------------------------------------------
static bool BlueprintActionMenuUtilsImpl::IsUnBoundSpawner(FBlueprintActionFilter const& /*Filter*/, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = true;
	if (UBlueprintBoundNodeSpawner const* BoundSpawner = Cast<UBlueprintBoundNodeSpawner>(BlueprintAction))
	{
		bIsFilteredOut = (BoundSpawner->GetBoundObject() == nullptr);
	}
	return bIsFilteredOut;
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
		FilterFlags = FBlueprintActionFilter::BPFILTER_ExcludeGlobalFields;
	}
	
	FBlueprintActionFilter MenuFilter(FilterFlags);
	MenuFilter.Context = Context;
	
	if (FilterClass != nullptr)
	{
		MenuFilter.OwnerClasses.Add(FilterClass);
	}

	MenuOut.AddMenuSection(MenuFilter, LOCTEXT("PaletteRoot", "Library"), /*MenuOrder =*/0, FBlueprintActionMenuBuilder::ConsolidatePropertyActions);
	MenuOut.RebuildActionList();
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuUtils::MakeContextMenu(FBlueprintActionContext const& Context, bool bIsContextSensitive, FBlueprintActionMenuBuilder& MenuOut)
{
	using namespace BlueprintActionMenuUtilsImpl;
	
	static int32 const MainMenuSectionGroup   = 000;
	static int32 const ComponentsSectionGroup = 100;

	FBlueprintActionFilter ComponentsFilter;
	ComponentsFilter.Context = Context;
	ComponentsFilter.NodeTypes.Add(UK2Node_ComponentBoundEvent::StaticClass());
	ComponentsFilter.NodeTypes.Add(UK2Node_CallFunction::StaticClass());
	ComponentsFilter.AddIsFilteredTest(FBlueprintActionFilter::FIsFilteredDelegate::CreateStatic(IsUnBoundSpawner));
	
	bool bHasComponentsSection = false;
	for (UObject* Selection : Context.SelectedObjects)
	{
		if (UObjectProperty* ObjProperty = Cast<UObjectProperty>(Selection))
		{
			ComponentsFilter.OwnerClasses.Add(ObjProperty->PropertyClass);
			bHasComponentsSection = true;
		}
	}

	MenuOut.Empty();

	FBlueprintActionFilter MenuFilter;
	MenuFilter.Context = Context;
	MenuFilter.Context.SelectedObjects.Empty();

	if (bIsContextSensitive)
	{
		if (bHasComponentsSection)
		{
			MenuOut.AddMenuSection(ComponentsFilter, FText::GetEmpty(), ComponentsSectionGroup);
		}
		
		for (UBlueprint* Blueprint : Context.Blueprints)
		{
			MenuFilter.OwnerClasses.Add(Blueprint->GeneratedClass);
		}
	}

	MenuOut.AddMenuSection(MenuFilter);
	MenuOut.RebuildActionList();

	if (!GetDefault<UEdGraphSchema_K2>()->bUseLegacyActionMenus)
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

		if (bIsContextSensitive && (Context.SelectedObjects.Num() == 0))
		{
			FText SelectComponentMsg = LOCTEXT("SelectComponentForEvents", "Select a Component to see available Events & Functions");
			FText SelectComponentToolTip = LOCTEXT("SelectComponentForEventsTooltip", "Select a Component in the MyBlueprint tab to see available Events and Functions in this menu.");
			TSharedPtr<FEdGraphSchemaAction> MsgAction = TSharedPtr<FEdGraphSchemaAction>(new FEdGraphSchemaAction_Dummy(TEXT(""), SelectComponentMsg, SelectComponentToolTip.ToString(), ComponentsSectionGroup));
			MenuOut.AddAction(MsgAction);
		}
	}	
}

#undef LOCTEXT_NAMESPACE

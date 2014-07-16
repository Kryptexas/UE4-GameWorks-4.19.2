// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "BlueprintActionMenuUtils.h"
#include "BlueprintActionMenuBuilder.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"

#define LOCTEXT_NAMESPACE "BlueprintActionMenuUtils"

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
void FBlueprintActionMenuUtils::MakeContextMenu(FBlueprintActionContext const& Context, TArray<UProperty*> const& SelectedProperties, FBlueprintActionMenuBuilder& MenuOut)
{
	MenuOut.Empty();
	
	FBlueprintActionFilter MenuFilter;
	MenuFilter.Context = Context;
	
	for (UProperty* SelectedProperty : SelectedProperties)
	{
		static int32 const ComponentsSectionGroup = 100;
		
		if (UObjectProperty* ObjProperty = Cast<UObjectProperty>(SelectedProperty))
		{
			MenuFilter.OwnerClasses.Add(ObjProperty->PropertyClass);
			FText const PropertyName = FText::FromName(ObjProperty->GetFName());
			
			MenuFilter.NodeTypes.Add(UK2Node_CallFunction::StaticClass());
			FText const FuncSectionName = FText::Format(LOCTEXT("ComponentFuncSection", "Call Function on {0}"), PropertyName);
			MenuOut.AddMenuSection(MenuFilter, FuncSectionName, ComponentsSectionGroup);
			
			MenuFilter.NodeTypes[0] = UK2Node_Event::StaticClass();
			FText const EventSectionName = FText::Format(LOCTEXT("ComponentEventSection", "Add Event for {0}"), PropertyName);
			MenuOut.AddMenuSection(MenuFilter, EventSectionName, ComponentsSectionGroup);
			
			MenuFilter.NodeTypes.Empty();
		}
	}
	
	for (UBlueprint* Blueprint : Context.Blueprints)
	{
		MenuFilter.OwnerClasses.Empty();
		MenuFilter.OwnerClasses.Add(Blueprint->GeneratedClass);
		
		MenuOut.AddMenuSection(MenuFilter);
	}
	
	MenuOut.RebuildActionList();
}

#undef LOCTEXT_NAMESPACE

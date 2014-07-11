// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintActionMenuBuilder.h"
#include "BlueprintActionMenuItem.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "K2ActionMenuBuilder.h"			// for FBlueprintPaletteListBuilder/FBlueprintGraphActionListBuilder
#include "EdGraphSchema_K2.h"				// for StaticClass(), bUseLegacyActionMenus, etc.

#define LOCTEXT_NAMESPACE "BlueprintActionMenuBuilder"

/*******************************************************************************
 * FBlueprintActionMenuItemFactory
 ******************************************************************************/

class FBlueprintActionMenuItemFactory
{
public:
	/** 
	 * Menu item factory constructor. Sets up the blueprint context, which
	 * is utilized when configuring blueprint menu items' names/tooltips/etc.
	 *
	 * @param  Context	The blueprint context for the menu being built.
	 */
	FBlueprintActionMenuItemFactory(FBlueprintActionContext const& Context);

	/** A root category to perpend every menu item with */
	FText RootCategory;
	/** The menu sort order to set every menu item with */
	int32 MenuGrouping;
	
	/**
	 * Spawns a new FBlueprintActionMenuItem with the node-spawner. Constructs
	 * the menu item's category, name, tooltip, etc.
	 * 
	 * @param  ItemOwner	The menu item owner that this will be assigned to.
	 * @param  Action		The node-spawner that the new menu item should wrap.
	 * @return A newly allocated FBlueprintActionMenuItem (which wraps the supplied action).
	 */
	TSharedPtr<FEdGraphSchemaAction> MakeMenuItem(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintNodeSpawner* Action);

private:
	/**
	 * Attempts to pull a menu name from the supplied spawner. If one isn't 
	 * provided, then it spawns a temporary node and pulls one from that node's 
	 * title.
	 * 
	 * @param  ItemOwner	The menu builder that this name will be listed from.
	 * @param  Action		The action you want to suss name information from.
	 * @return A name for the menu item wrapping this action.
	 */
	FText GetMenuNameForAction(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintNodeSpawner* Action);

	/**
	 * Attempts to pull a menu category from the supplied spawner. If one isn't 
	 * provided, then it spawns a temporary node and pulls one from that node.
	 * 
	 * @param  ItemOwner	The menu builder that the item will be listed from.
	 * @param  Action		The action you want to suss category information from.
	 * @return A category for the menu item wrapping this action.
	 */
	FText GetCategoryForAction(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintNodeSpawner* Action);

	/**
	 * Attempts to pull a menu tooltip from the supplied spawner. If one isn't 
	 * provided, then it spawns a temporary node and pulls one from that node.
	 * 
	 * @param  ItemOwner	The menu builder that the item will be listed from.
	 * @param  Action		The action you want to suss tooltip information from.
	 * @return A tooltip for the menu item wrapping this action.
	 */
	FText GetTooltipForAction(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintNodeSpawner* Action);

	/**
	 * Attempts to pull a keywords from the supplied spawner. If one isn't 
	 * provided, then it spawns a temporary node and pulls them from that.
	 * 
	 * @param  ItemOwner	The menu builder that the item will be listed from.
	 * @param  Action		The action you want to suss keyword information from.
	 * @return A keyword text string for the menu item wrapping this action.
	 */
	FText GetSearchKeywordsForAction(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintNodeSpawner* Action);

	/**
	 * Utility getter function that retrieves the blueprint context for the menu
	 * items being made.
	 * 
	 * @return The first blueprint out of the cached FBlueprintActionContext.
	 */
	UBlueprint* GetBlueprint() const;

	/**
	 * Utility function that will pull the "OwnerOfTemporaries" field from 
	 * ItemOwner. If the object is null, then this will allocate one on behalf
	 * of ItemOwner.
	 * 
	 * @param  The FBlueprintActionMenuBuilder you want an "OwnerOfTemporaries" from. 
	 * @return The FBlueprintActionMenuBuilder's OwnerOfTemporaries.
	 */
	UEdGraph* GetOwnerOfTemporaries(FBlueprintActionMenuBuilder& ItemOwner) const;

	/** Cached context for the blueprint menu being built */
	FBlueprintActionContext const& Context;
};

//------------------------------------------------------------------------------
FBlueprintActionMenuItemFactory::FBlueprintActionMenuItemFactory(FBlueprintActionContext const& ContextIn)
	: Context(ContextIn)
	, RootCategory(FText::GetEmpty())
	, MenuGrouping(0)
{
	
}

//------------------------------------------------------------------------------
TSharedPtr<FEdGraphSchemaAction> FBlueprintActionMenuItemFactory::MakeMenuItem(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintNodeSpawner* Action)
{
	FBlueprintActionMenuItem* NewMenuItem = new FBlueprintActionMenuItem(Action, MenuGrouping);
	
	NewMenuItem->MenuDescription    = GetMenuNameForAction(ItemOwner, Action);
	NewMenuItem->TooltipDescription = GetTooltipForAction(ItemOwner, Action).ToString();
	NewMenuItem->Category           = GetCategoryForAction(ItemOwner, Action).ToString();
	NewMenuItem->Keywords           = GetSearchKeywordsForAction(ItemOwner, Action).ToString();

	if (NewMenuItem->Category.IsEmpty())
	{
		NewMenuItem->Category = RootCategory.ToString();
	}
	else if (!RootCategory.IsEmpty())
	{
		NewMenuItem->Category = FString::Printf(TEXT("%s|%s"), *RootCategory.ToString(), *NewMenuItem->Category);
	}
	
	return MakeShareable(NewMenuItem);
}

//------------------------------------------------------------------------------
FText FBlueprintActionMenuItemFactory::GetMenuNameForAction(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintNodeSpawner* Action)
{
	check(Action != nullptr);
	// give the action the chance to save on performance (to keep from having to spawn a template node)
	FText MenuName = Action->GetDefaultMenuName();
	
	if (MenuName.IsEmpty())
	{
		if (UEdGraphNode* NodeTemplate = Action->MakeTemplateNode(GetOwnerOfTemporaries(ItemOwner)))
		{
			MenuName = NodeTemplate->GetNodeTitle(ENodeTitleType::ListView);
		}
		else
		{
			// need to give it some name, this is as good as any I guess
			MenuName = FText::FromName(Action->GetFName());
		}
	}
	
	return MenuName;
}

//------------------------------------------------------------------------------
FText FBlueprintActionMenuItemFactory::GetCategoryForAction(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintNodeSpawner* Action)
{
	check(Action != nullptr);
	// give the action the chance to save on performance (to keep from having to spawn a template node)
	FText MenuCategory = Action->GetDefaultMenuCategory();
	
	if (MenuCategory.IsEmpty())
	{
		UEdGraph* OwnerOfTemporaries = GetOwnerOfTemporaries(ItemOwner);
		// @TODO: consider moving GetMenuCategory() up into UEdGraphNode
		if (UK2Node* NodeTemplate = Cast<UK2Node>(Action->MakeTemplateNode(OwnerOfTemporaries)))
		{
			MenuCategory = NodeTemplate->GetMenuCategory();
		}
		
		// if the category is still empty, then try and construct the category
		// from the spawner's type
		if (MenuCategory.IsEmpty())
		{
			// put uncategorized function calls in a member function category
			// (sorted by their respective classes)
			if (UBlueprintFunctionNodeSpawner* FuncSpawner = Cast<UBlueprintFunctionNodeSpawner>(Action))
			{
				UFunction const* Function = FuncSpawner->GetFunction();
				check(Function != nullptr);
				UClass* FuncOwner = Function->GetOuterUClass();
				
				UBlueprint* Blueprint = GetBlueprint();
				check(Blueprint != nullptr);
				
				// if this is NOT a self function call (self function calls
				// don't get nested any deeper)
				if (!Blueprint->GeneratedClass->IsChildOf(FuncOwner))
				{
					MenuCategory = FText::FromName(FuncOwner->GetFName());
				}
				MenuCategory = FText::Format(LOCTEXT("MemberFunctionsCategory", "Call Funtions|{0}"), MenuCategory);
			}
		}
	}
	
	return MenuCategory;
}

//------------------------------------------------------------------------------
FText FBlueprintActionMenuItemFactory::GetTooltipForAction(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintNodeSpawner* Action)
{
	check(Action != nullptr);
	// give the action the chance to save on performance (to keep from having to spawn a template node)
	FText Tooltip = Action->GetDefaultMenuTooltip();
	
	if (Tooltip.IsEmpty())
	{
		if (UEdGraphNode* NodeTemplate = Action->MakeTemplateNode(GetOwnerOfTemporaries(ItemOwner)))
		{
			Tooltip = FText::FromString(NodeTemplate->GetTooltip());
		}
	}
	
	return Tooltip;
}

//------------------------------------------------------------------------------
FText FBlueprintActionMenuItemFactory::GetSearchKeywordsForAction(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintNodeSpawner* Action)
{
	check(Action != nullptr);
	// give the action the chance to save on performance (to keep from having to spawn a template node)
	FText SearchKeywords = Action->GetDefaultSearchKeywords();
	
	if (SearchKeywords.IsEmpty())
	{
		if (UEdGraphNode* NodeTemplate = Action->MakeTemplateNode(GetOwnerOfTemporaries(ItemOwner)))
		{
			SearchKeywords = FText::FromString(NodeTemplate->GetKeywords());
		}
	}
	
	return SearchKeywords;
}

//------------------------------------------------------------------------------
UBlueprint* FBlueprintActionMenuItemFactory::GetBlueprint() const
{
	UBlueprint* Blueprint = nullptr;
	if (Context.Blueprints.Num() > 0)
	{
		Blueprint = Context.Blueprints[0];
	}
	return Blueprint;
}

//------------------------------------------------------------------------------
UEdGraph* FBlueprintActionMenuItemFactory::GetOwnerOfTemporaries(FBlueprintActionMenuBuilder& ItemOwner) const
{
	UBlueprint* Blueprint = GetBlueprint();
	check(Blueprint != nullptr);
	
	UEdGraph*& OwnerOfTemporaries = ItemOwner.OwnerOfTemporaries;
	if (OwnerOfTemporaries == nullptr)
	{
		// unfortunately the OwnerOfTemporaries cannont be directly assigned to
		// the transient package because various nodes relay on the blueprint
		// outer (just like they rely on this graph outer); for example, the
		// call-function node uses the blueprint to see if the function has
		// self-context
		OwnerOfTemporaries = NewObject<UEdGraph>(Blueprint);
		OwnerOfTemporaries->SetFlags(RF_Transient);
		
		OwnerOfTemporaries->Schema = UEdGraphSchema_K2::StaticClass();
		if (Context.Graphs.Num() > 0)
		{
			OwnerOfTemporaries->Schema = Context.Graphs[0]->Schema;
		}
		
	}
	return OwnerOfTemporaries;
}

/*******************************************************************************
 * Static FBlueprintActionMenuBuilder Helpers
 ******************************************************************************/

namespace FBlueprintActionMenuBuilderImpl
{
	/** 
	 * Defines a sub-section of the overall blueprint menu (filter, heading, etc.)
	 */
	struct FMenuSectionDefinition
	{
		FMenuSectionDefinition(FBlueprintActionFilter const& SectionFilter);

		/** A filter for this section of the menu */
		FBlueprintActionFilter Filter;
		/** In charge of spawning menu items for this section (holds category/ordering information)*/
		FBlueprintActionMenuItemFactory ItemFactory;
	};

	/**
	 * To offer a fallback in case the action menu refactor is unstable, this
	 * method implements the old way we used collect blueprint menu actions (for
	 * both the palette and context menu).
	 * 
	 * @param  MenuSections	All the sections that were defined for FBlueprintActionMenuBuilder (we attempt to combine them)
	 * @param  MenuBuilder	The menu builder we want all the legacy actions appended to.
	 * @return 
	 */
	static void AppendLegacyActions(TArray< TSharedRef<FMenuSectionDefinition> > const& MenuSections, FBlueprintActionMenuBuilder& MenuBuilder);
}

//------------------------------------------------------------------------------
FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::FMenuSectionDefinition(FBlueprintActionFilter const& SectionFilterIn)
	: ItemFactory(SectionFilterIn.Context)
	, Filter(SectionFilterIn)
{
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilderImpl::AppendLegacyActions(TArray< TSharedRef<FMenuSectionDefinition> > const& MenuSections, FBlueprintActionMenuBuilder& MenuBuilder)
{
	check(MenuSections.Num() > 0);
	FBlueprintActionContext const& Context = MenuSections[0]->Filter.Context;

	if (Context.Graphs.Num() > 0)
	{
		FBlueprintGraphActionListBuilder ContextMenuBuilder(Context.Graphs[0]);
		if (Context.Pins.Num() > 0)
		{
			ContextMenuBuilder.FromPin = Context.Pins[0];
		}
		
		// @TODO: ContextMenuBuilder.SelectedObjects
// 		for (TSharedRef<FMenuSectionDefinition> MenuSection : MenuSections)
// 		{
// 			FBlueprintActionFilter const& SectionFilter = MenuSection->Filter;
// 		}

		UEdGraphSchema const* GraphSchema = GetDefault<UEdGraphSchema>(ContextMenuBuilder.CurrentGraph->Schema);
		GraphSchema->GetGraphContextActions(ContextMenuBuilder);

		MenuBuilder.Append(ContextMenuBuilder);
	}
	else if (Context.Blueprints.Num() > 0)
	{
		FBlueprintPaletteListBuilder PaletteBuilder(Context.Blueprints[0], MenuSections[0]->ItemFactory.RootCategory.ToString());

		UClass* FilterClass = nullptr;

		FBlueprintActionFilter const& SectionFilter = MenuSections[0]->Filter;
		if (SectionFilter.OwnerClasses.Num() > 0)
		{
			FilterClass = SectionFilter.OwnerClasses[0];
		}

		UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
		FK2ActionMenuBuilder(K2Schema).GetPaletteActions(PaletteBuilder, FilterClass); 

		MenuBuilder.Append(PaletteBuilder);
	}
}

/*******************************************************************************
 * FBlueprintActionMenuBuilder
 ******************************************************************************/

//------------------------------------------------------------------------------
FBlueprintActionMenuBuilder::FBlueprintActionMenuBuilder(FBlueprintActionFilter const& Filter, bool bAutoBuild/*= true*/)
{
	AddMenuSection(Filter);
	if (bAutoBuild)
	{
		RebuildActionList();
	}
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilder::AddMenuSection(FBlueprintActionFilter const& Filter, FText const& Heading/* = FText::GetEmpty()*/, int32 MenuOrder/* = 0*/)
{
	using namespace FBlueprintActionMenuBuilderImpl;
	
	TSharedRef<FMenuSectionDefinition> SectionDescRef = MakeShareable(new FMenuSectionDefinition(Filter));
	SectionDescRef->ItemFactory.RootCategory = Heading;
	SectionDescRef->ItemFactory.MenuGrouping = MenuOrder;

	MenuSections.Add(SectionDescRef);
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilder::RebuildActionList()
{
	using namespace FBlueprintActionMenuBuilderImpl;

	Empty();
	if (true)//!GetDefault<UEdGraphSchema_K2>()->bUseLegacyActionMenus)
	{
		GenerateMenuSections();
	}
	else if (MenuSections.Num() > 0)
	{
		// use the context from the first section (the legacy menu building did
		// it all in one sweep and knew how to mark off sections)
		AppendLegacyActions(MenuSections, *this);
	}

	// @TODO: account for all K2ActionMenuBuilder action types...
	// - FEdGraphSchemaAction_K2AddTimeline
	// - FEdGraphSchemaAction_K2ViewNode
	// - FEdGraphSchemaAction_K2AddCustomEvent
	//   FEdGraphSchemaAction_EventFromFunction
	//   FEdGraphSchemaAction_K2Var
	//   FEdGraphSchemaAction_K2Delegate
	//   FEdGraphSchemaAction_K2AssignDelegate
	// - FEdGraphSchemaAction_K2AddComment
	//   FEdGraphSchemaAction_K2PasteHere
	// - FEdGraphSchemaAction_K2NewNode
	//   FEdGraphSchemaAction_Dummy
	//   FEdGraphSchemaAction_K2AddCallOnActor
	//   FEdGraphSchemaAction_K2AddCallOnVariable
	// - FEdGraphSchemaAction_K2AddComponent
	//   FEdGraphSchemaAction_K2AddComment
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilder::GenerateMenuSections()
{
	using namespace FBlueprintActionMenuBuilderImpl;

	FBlueprintActionDatabase::FClassActionMap const& ClassActions = FBlueprintActionDatabase::Get().GetAllActions();
	for (auto ActionEntry : ClassActions)
	{
		UClass* Class = ActionEntry.Key;
		for (UBlueprintNodeSpawner* NodeSpawner : ActionEntry.Value)
		{
			for (TSharedRef<FMenuSectionDefinition> MenuSection : MenuSections)
			{
				if (MenuSection->Filter.IsFiltered(NodeSpawner))
				{
					// this node spawner doesn't belong in this section of the menu
					continue;
				}

				TSharedPtr<FEdGraphSchemaAction> NewActionPtr = MenuSection->ItemFactory.MakeMenuItem(*this, NodeSpawner);
				check(NewActionPtr.IsValid());

				AddAction(NewActionPtr);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "BlueprintActionMenuBuilder.h"
#include "BlueprintActionMenuItem.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintPropertyNodeSpawner.h"
#include "K2ActionMenuBuilder.h"		// for FBlueprintPaletteListBuilder/FBlueprintGraphActionListBuilder
#include "EdGraphSchema_K2.h"			// for StaticClass(), bUseLegacyActionMenus, etc.
#include "BlueprintEditor.h"			// for GetMyBlueprintWidget(), and GetIsContextSensitive()
#include "K2ActionMenuBuilder.h"
#include "SMyBlueprint.h"				// for SelectionAsVar()
#include "EdGraphSchema_K2_Actions.h"	// for FEdGraphSchemaAction_K2Var

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

	/**
	 * Clones the property spawner and clears its NodeClass field (used to 
	 * represent the property as a whole, encompasing all the node types that
	 * can be associated with it, like: getters, setters, etc.
	 *
	 * @param  ItemOwner	The menu item owner that this will be assigned to.
	 * @param  NodeSpawner	The node-spawner that we want to clone (and create a proxy for).
	 * @return A cloned UBlueprintPropertyNodeSpawner (representing the property).
	 */
	UBlueprintPropertyNodeSpawner* MakePropertyActionProxy(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintPropertyNodeSpawner* NodeSpawner);
	
	/**
	 * Checks to see if a proxy has already been made for the specified property
	 * (checks to see if someone has already called MakePropertyActionProxy()
	 * for a spawner associated with this property).
	 *
	 * @param  Property		The property you want to find a proxy for.
	 * @return True if there's a proxy already cached for the property, otherwise false.
	 */
	bool HasProxy(UProperty const* Property) const;
	
	/**
	 * Checks to see if the supplied property node spawner is a proxy itself 
	 * (was it spawned from MakePropertyActionProxy()? does it have an empty 
	 * NodeClass field?)
	 *
	 * @param  PropertyNodeSpawner	The spawner you want to check.
	 * @return True if the supplied node-spawner is a proxy, false otherwise.
	 */
	bool IsProxy(UBlueprintPropertyNodeSpawner* PropertyNodeSpawner) const;
	
protected:
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
	
	/** Tracks intermediate property proxies (if any were created) */
	TMap<UProperty const*, UBlueprintPropertyNodeSpawner*> PropertyProxyMap;
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
UBlueprintPropertyNodeSpawner* FBlueprintActionMenuItemFactory::MakePropertyActionProxy(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintPropertyNodeSpawner* PropertyNodeSpawner)
{
	UBlueprintPropertyNodeSpawner* ActionProxy = nullptr;
	
	UProperty const* Property = PropertyNodeSpawner->GetProperty();
	if (UBlueprintPropertyNodeSpawner** FoundProxy = PropertyProxyMap.Find(Property))
	{
		ActionProxy = *FoundProxy;
	}
	else
	{
		ActionProxy = DuplicateObject(PropertyNodeSpawner, GetOwnerOfTemporaries(ItemOwner));
		// we use a null node class to flag the proxy, FBlueprintActionMenuItem
		// queues off of this and will provide a drag-drop menu to the user
		ActionProxy->NodeClass = nullptr;
		PropertyProxyMap.Add(Property, ActionProxy);
	}
	
	return ActionProxy;
}

//------------------------------------------------------------------------------
bool FBlueprintActionMenuItemFactory::HasProxy(UProperty const* Property) const
{
	return PropertyProxyMap.Contains(Property);
}

//------------------------------------------------------------------------------
bool FBlueprintActionMenuItemFactory::IsProxy(UBlueprintPropertyNodeSpawner* NodeSpawner) const
{
	bool bIsProxy = false;
	
	UProperty const* Property = NodeSpawner->GetProperty();
	if (UBlueprintPropertyNodeSpawner* const* FoundProxy = PropertyProxyMap.Find(Property))
	{
		bIsProxy = (*FoundProxy == NodeSpawner);
	}
	check(!bIsProxy || (NodeSpawner->NodeClass == nullptr));
	
	return bIsProxy;
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
				MenuCategory = FText::Format(LOCTEXT("MemberFunctionsCategory", "Call Function|{0}"), MenuCategory);
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
		FMenuSectionDefinition(FBlueprintActionFilter const& SectionFilter, uint32 const Flags);
		
		/** Sets the root category for menu items in this section. */
		void SetSectionHeading(FText const& RootCategory);
		/** Gets the root category for menu items in this section. */
		FText const& GetSectionHeading() const;

		/** Sets the grouping for menu items belonging to this section. */
		void SetSectionSortOrder(int32 const MenuGrouping);
		
		/**
		 * Filters the supplied action and if it passes, spawns a new 
		 * FBlueprintActionMenuItem for the specified menu (does not add the 
		 * item to the menu-builder itself).
		 *
		 * @param  ItemOwner	The menu item owner that this will be assigned to.
		 * @param  Action		The node-spawner that the new menu item should wrap.
		 * @return An empty TSharedPtr if the action was filtered out, otherwise a newly allocated FBlueprintActionMenuItem.
		 */
		TSharedPtr<FEdGraphSchemaAction> MakeMenuItem(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintNodeSpawner* Action);
		
		
		/** Series of ESectionFlags, aimed at customizing how we construct this menu section */
		uint32 Flags;
		/** A filter for this section of the menu */
		FBlueprintActionFilter Filter;
	private:
		/** In charge of spawning menu items for this section (holds category/ordering information)*/
		FBlueprintActionMenuItemFactory ItemFactory;
	};
	
	/**
	 * To offer a fallback in case the action menu refactor is unstable, this
	 * method implements the old way we used collect blueprint menu actions (for
	 * both the palette and context menu).
	 *
	 * @param  MenuSection	The primary section for the FBlueprintActionMenuBuilder.
	 * @param  MenuOut  	The menu builder we want all the legacy actions appended to.
	 */
	static void AppendLegacyItems(FMenuSectionDefinition const& MenuDef, FBlueprintActionMenuBuilder& MenuOut);
}

//------------------------------------------------------------------------------
FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::FMenuSectionDefinition(FBlueprintActionFilter const& SectionFilterIn, uint32 const FlagsIn)
	: Flags(FlagsIn)
	, Filter(SectionFilterIn)
	, ItemFactory(Filter.Context)
{
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::SetSectionHeading(FText const& RootCategory)
{
	ItemFactory.RootCategory = RootCategory;
}

//------------------------------------------------------------------------------
FText const& FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::GetSectionHeading() const
{
	return ItemFactory.RootCategory;
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::SetSectionSortOrder(int32 const MenuGrouping)
{
	ItemFactory.MenuGrouping = MenuGrouping;
}

//------------------------------------------------------------------------------
TSharedPtr<FEdGraphSchemaAction> FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::MakeMenuItem(FBlueprintActionMenuBuilder& ItemOwner, UBlueprintNodeSpawner* Action)
{
	if (Flags & FBlueprintActionMenuBuilder::ConsolidatePropertyActions)
	{
		if (UBlueprintPropertyNodeSpawner* PropertySpawner = Cast<UBlueprintPropertyNodeSpawner>(Action))
		{
			UProperty const* Property = PropertySpawner->GetProperty();
			if (ItemFactory.HasProxy(Property))
			{
				// we've already filtered a proxy for this property, we don't
				// want to spawn another
				Action = nullptr;
			}
			else
			{
				// make a proxy action (that won't spawn anything)...
				// FBlueprintActionMenuItem should interpret this and offer the
				// user a drag-drop menu (with a list of multiple property nodes)
				Action = ItemFactory.MakePropertyActionProxy(ItemOwner, PropertySpawner);
			}
		}
	}
	
	TSharedPtr<FEdGraphSchemaAction> MenuEntry = nullptr;
	if ((Action != nullptr) && !Filter.IsFiltered(Action))
	{
		MenuEntry = ItemFactory.MakeMenuItem(ItemOwner, Action);
	}
	return MenuEntry;
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilderImpl::AppendLegacyItems(FMenuSectionDefinition const& MenuDef, FBlueprintActionMenuBuilder& MenuOut)
{
	FBlueprintActionFilter const&  MenuFilter  = MenuDef.Filter;
	FBlueprintActionContext const& MenuContext = MenuFilter.Context;
	
	// if this is for the context menu
	if (MenuContext.Graphs.Num() > 0)
	{
		UEdGraph* Graph = MenuContext.Graphs[0];
		UEdGraphSchema const* GraphSchema = GetDefault<UEdGraphSchema>(Graph->Schema);
		
		FBlueprintGraphActionListBuilder LegacyBuilder(Graph);
		if (MenuContext.Pins.Num() > 0)
		{
			LegacyBuilder.FromPin = MenuContext.Pins[0];
		}
		
		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(Graph);
		// not the best way to poll for Selected properties, but since this
		// function is only temporary until we switch over to the new system...
		FBlueprintEditor* EditorInst = (FBlueprintEditor*)FAssetEditorManager::Get().FindEditorForAsset(Blueprint, /*bFocusIfOpen =*/false);
		
		bool bIsContextSensitive = true;
		if (EditorInst != nullptr)
		{
			bIsContextSensitive = EditorInst->GetIsContextSensitive();
			
			FEdGraphSchemaAction_K2Var* SelectedVar = EditorInst->GetMyBlueprintWidget()->SelectionAsVar();
			if ((SelectedVar != nullptr) && (SelectedVar->GetProperty() != nullptr))
			{
				LegacyBuilder.SelectedObjects.Add(SelectedVar->GetProperty());
			}
		}
		
		if (bIsContextSensitive)
		{
			GraphSchema->GetGraphContextActions(LegacyBuilder);
			MenuOut.Append(LegacyBuilder);
		}
		else
		{
			FBlueprintPaletteListBuilder ContextlessLegacyBuilder(Blueprint);
			UEdGraphSchema_K2::GetAllActions(ContextlessLegacyBuilder);
			MenuOut.Append(ContextlessLegacyBuilder);
		}
	}
	else if (MenuContext.Blueprints.Num() > 0)
	{
		UBlueprint* Blueprint = MenuContext.Blueprints[0];
		FBlueprintPaletteListBuilder LegacyBuilder(Blueprint, MenuDef.GetSectionHeading().ToString());
		
		UClass* ClassFilter = nullptr;
		if (MenuFilter.OwnerClasses.Num() > 0)
		{
			ClassFilter = MenuFilter.OwnerClasses[0];
		}
		
		UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
		FK2ActionMenuBuilder(K2Schema).GetPaletteActions(LegacyBuilder, ClassFilter);
		
		MenuOut.Append(LegacyBuilder);
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
void FBlueprintActionMenuBuilder::Empty()
{
	FGraphActionListBuilderBase::Empty();
	MenuSections.Empty();
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilder::AddMenuSection(FBlueprintActionFilter const& Filter, FText const& Heading/* = FText::GetEmpty()*/, int32 MenuOrder/* = 0*/, uint32 const Flags/* = 0*/)
{
	using namespace FBlueprintActionMenuBuilderImpl;
	
	TSharedRef<FMenuSectionDefinition> SectionDescRef = MakeShareable(new FMenuSectionDefinition(Filter, Flags));
	SectionDescRef->SetSectionHeading(Heading);
	SectionDescRef->SetSectionSortOrder(MenuOrder);

	MenuSections.Add(SectionDescRef);
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilder::RebuildActionList()
{
	using namespace FBlueprintActionMenuBuilderImpl;

	FGraphActionListBuilderBase::Empty();
	
	if (!GetDefault<UEdGraphSchema_K2>()->bUseLegacyActionMenus)
	{
		FBlueprintActionDatabase::FClassActionMap const& ClassActions = FBlueprintActionDatabase::Get().GetAllActions();
		for (auto ActionEntry : ClassActions)
		{
			UClass* Class = ActionEntry.Key;
			for (UBlueprintNodeSpawner* NodeSpawner : ActionEntry.Value)
			{
				for (TSharedRef<FMenuSectionDefinition> MenuSection : MenuSections)
				{
					TSharedPtr<FEdGraphSchemaAction> MenuEntry = MenuSection->MakeMenuItem(*this, NodeSpawner);
					if (MenuEntry.IsValid())
					{
						AddAction(MenuEntry);
					}
				}
			}
		}
	}
	else if (MenuSections.Num() > 0)
	{
		AppendLegacyItems(*MenuSections[0], *this);
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

#undef LOCTEXT_NAMESPACE

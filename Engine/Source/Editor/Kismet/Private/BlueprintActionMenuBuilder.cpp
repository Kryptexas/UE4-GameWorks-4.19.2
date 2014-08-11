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
#include "BlueprintEditorUtils.h"		// for FindBlueprintForGraphChecked()
#include "BlueprintEditor.h"			// for GetFocusedGraph()

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
	 * @param  EditorContext
	 * @param  Action		The node-spawner that the new menu item should wrap.
	 * @return A newly allocated FBlueprintActionMenuItem (which wraps the supplied action).
	 */
	TSharedPtr<FEdGraphSchemaAction> MakeMenuItem(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action);

	/**
	 * Clones the property spawner and clears its NodeClass field (used to 
	 * represent the property as a whole, encompassing all the node types that
	 * can be associated with it, like: getters, setters, etc.
	 *
	 * @param  NodeSpawner	The node-spawner that we want to clone (and create a proxy for).
	 * @return A cloned UBlueprintPropertyNodeSpawner (representing the property).
	 */
	UBlueprintPropertyNodeSpawner* MakePropertyActionProxy( UBlueprintPropertyNodeSpawner* NodeSpawner);
	
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

	/**
	 * Clears out all the proxy items that this spawned (and tracked).
	 */
	void Empty();
	
protected:
	/**
	 * Attempts to pull a menu name from the supplied spawner. If one isn't 
	 * provided, then it spawns a temporary node and pulls one from that node's 
	 * title.
	 * 
	 * @param  EditorContext
	 * @param  Action		The action you want to suss name information from.
	 * @return A name for the menu item wrapping this action.
	 */
	FText GetMenuNameForAction(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action);

	/**
	 * Attempts to pull a menu category from the supplied spawner. If one isn't 
	 * provided, then it spawns a temporary node and pulls one from that node.
	 * 
	 * @param  EditorContext
	 * @param  Action		The action you want to suss category information from.
	 * @return A category for the menu item wrapping this action.
	 */
	FText GetCategoryForAction(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action);

	/**
	 * Attempts to pull a menu tooltip from the supplied spawner. If one isn't 
	 * provided, then it spawns a temporary node and pulls one from that node.
	 * 
	 * @param  EditorContext
	 * @param  Action		The action you want to suss tooltip information from.
	 * @return A tooltip for the menu item wrapping this action.
	 */
	FText GetTooltipForAction(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action);

	/**
	 * Attempts to pull a keywords from the supplied spawner. If one isn't 
	 * provided, then it spawns a temporary node and pulls them from that.
	 * 
	 * @param  EditorContext
	 * @param  Action		The action you want to suss keyword information from.
	 * @return A keyword text string for the menu item wrapping this action.
	 */
	FText GetSearchKeywordsForAction(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action);

	/**
	 * Attempts to pull a menu icon information from the supplied spawner. If
	 * info isn't provided, then it spawns a temporary node and pulls data from 
	 * that node.
	 * 
	 * @param  EditorContext
	 * @param  Action		The action you want to suss icon information from.
	 * @param  ColorOut		The color to tint the icon with.
	 * @return Name of the brush to use (use FEditorStyle::GetBrush() to resolve).
	 */
	FName GetMenuIconForAction(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action, FLinearColor& ColorOut);

	/**
	 * Utility getter function that retrieves the blueprint context for the menu
	 * items being made.
	 * 
	 * @return The first blueprint out of the cached FBlueprintActionContext.
	 */
	UBlueprint* GetTargetBlueprint() const;

	/**
	 * 
	 * 
	 * @param  Action	
	 * @param  EditorContext	
	 * @return 
	 */
	UEdGraphNode* GetTemplateNode(UBlueprintNodeSpawner* Action, TWeakPtr<FBlueprintEditor> EditorContext) const;

	/** Cached context for the blueprint menu being built */
	FBlueprintActionContext const& Context;
	
	/** Tracks intermediate property proxies (if any were created) */
	TMap<UProperty const*, UBlueprintPropertyNodeSpawner*> PropertyProxyMap;
};

//------------------------------------------------------------------------------
FBlueprintActionMenuItemFactory::FBlueprintActionMenuItemFactory(FBlueprintActionContext const& ContextIn)
	: RootCategory(FText::GetEmpty())
	, MenuGrouping(0)
	, Context(ContextIn)
{
}

//------------------------------------------------------------------------------
TSharedPtr<FEdGraphSchemaAction> FBlueprintActionMenuItemFactory::MakeMenuItem(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action)
{
	FLinearColor IconTint = FLinearColor::White;
	FName IconBrushName = GetMenuIconForAction(EditorContext, Action, IconTint);

	FBlueprintActionMenuItem* NewMenuItem = new FBlueprintActionMenuItem(Action, FEditorStyle::GetBrush(IconBrushName), IconTint, MenuGrouping);
	
	NewMenuItem->MenuDescription    = GetMenuNameForAction(EditorContext, Action);
	NewMenuItem->TooltipDescription = GetTooltipForAction(EditorContext, Action).ToString();
	NewMenuItem->Category           = GetCategoryForAction(EditorContext, Action).ToString();
	NewMenuItem->Keywords           = GetSearchKeywordsForAction(EditorContext, Action).ToString();

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
UBlueprintPropertyNodeSpawner* FBlueprintActionMenuItemFactory::MakePropertyActionProxy(UBlueprintPropertyNodeSpawner* PropertyNodeSpawner)
{
	UBlueprintPropertyNodeSpawner* ActionProxy = nullptr;
	
	UProperty const* Property = PropertyNodeSpawner->GetProperty();
	if (UBlueprintPropertyNodeSpawner** FoundProxy = PropertyProxyMap.Find(Property))
	{
		ActionProxy = *FoundProxy;
	}
	else
	{
		ActionProxy = DuplicateObject(PropertyNodeSpawner, GetTransientPackage());
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
void FBlueprintActionMenuItemFactory::Empty()
{
	PropertyProxyMap.Empty();
}

//------------------------------------------------------------------------------
FText FBlueprintActionMenuItemFactory::GetMenuNameForAction(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action)
{
	check(Action != nullptr);
	// give the action the chance to save on performance (to keep from having to spawn a template node)
	FText MenuName = Action->GetDefaultMenuName();
	
	if (MenuName.IsEmpty())
	{
		if (UEdGraphNode* NodeTemplate = GetTemplateNode(Action, EditorContext))
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
FText FBlueprintActionMenuItemFactory::GetCategoryForAction(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action)
{
	check(Action != nullptr);
	// give the action the chance to save on performance (to keep from having to spawn a template node)
	FText MenuCategory = Action->GetDefaultMenuCategory();
	
	if (MenuCategory.IsEmpty())
	{
		// @TODO: consider moving GetMenuCategory() up into UEdGraphNode
		if (UK2Node* NodeTemplate = Cast<UK2Node>(GetTemplateNode(Action, EditorContext)))
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
				
				UBlueprint* Blueprint = GetTargetBlueprint();
				check(Blueprint != nullptr);
				UClass* BlueprintClass = (Blueprint->SkeletonGeneratedClass != nullptr) ? Blueprint->SkeletonGeneratedClass : Blueprint->ParentClass;
				
				// if this is NOT a self function call (self function calls
				// don't get nested any deeper)
				if (!BlueprintClass->IsChildOf(FuncOwner))
				{
					MenuCategory = FuncOwner->GetDisplayNameText();
				}
				MenuCategory = FText::Format(LOCTEXT("MemberFunctionsCategory", "Call Function|{0}"), MenuCategory);
			}
		}
	}
	
	return MenuCategory;
}

//------------------------------------------------------------------------------
FText FBlueprintActionMenuItemFactory::GetTooltipForAction(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action)
{
	check(Action != nullptr);
	// give the action the chance to save on performance (to keep from having to spawn a template node)
	FText Tooltip = Action->GetDefaultMenuTooltip();
	
	if (Tooltip.IsEmpty())
	{
		if (UEdGraphNode* NodeTemplate = GetTemplateNode(Action, EditorContext))
		{
			Tooltip = FText::FromString(NodeTemplate->GetTooltip());
		}
	}
	
	return Tooltip;
}

//------------------------------------------------------------------------------
FText FBlueprintActionMenuItemFactory::GetSearchKeywordsForAction(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action)
{
	check(Action != nullptr);
	// give the action the chance to save on performance (to keep from having to spawn a template node)
	FText SearchKeywords = Action->GetDefaultSearchKeywords();
	
	if (SearchKeywords.IsEmpty())
	{
		if (UEdGraphNode* NodeTemplate = GetTemplateNode(Action, EditorContext))
		{
			SearchKeywords = FText::FromString(NodeTemplate->GetKeywords());
		}
	}
	
	return SearchKeywords;
}

//------------------------------------------------------------------------------
FName FBlueprintActionMenuItemFactory::GetMenuIconForAction(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action, FLinearColor& ColorOut)
{
	FName BrushName = Action->GetDefaultMenuIcon(ColorOut);
	if (BrushName.IsNone())
	{
		if (UEdGraphNode* NodeTemplate = GetTemplateNode(Action, EditorContext))
		{
			BrushName = NodeTemplate->GetPaletteIcon(ColorOut);
		}
	}
	return BrushName;
}

//------------------------------------------------------------------------------
UBlueprint* FBlueprintActionMenuItemFactory::GetTargetBlueprint() const
{
	UBlueprint* TargetBlueprint = nullptr;
	if (Context.Blueprints.Num() > 0)
	{
		TargetBlueprint = Context.Blueprints[0];
	}
	return TargetBlueprint;
}

//------------------------------------------------------------------------------
UEdGraphNode* FBlueprintActionMenuItemFactory::GetTemplateNode(UBlueprintNodeSpawner* Action, TWeakPtr<FBlueprintEditor> EditorContext) const
{
	UEdGraph* TargetGraph = nullptr;
	if (Context.Graphs.Num() > 0)
	{
		TargetGraph = Context.Graphs[0];
	}
	else
	{
		UBlueprint* Blueprint = GetTargetBlueprint();
		check(Blueprint != nullptr);
		
		if (Blueprint->UbergraphPages.Num() > 0)
		{
			TargetGraph = Blueprint->UbergraphPages[0];
		}
		else if (EditorContext.IsValid())
		{
			TargetGraph = EditorContext.Pin()->GetFocusedGraph();
		}
	}

	check(Action != nullptr);
	return Action->GetTemplateNode(TargetGraph);
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
		 * @param  EditorContext
		 * @param  Action		The node-spawner that the new menu item should wrap.
		 * @return An empty TSharedPtr if the action was filtered out, otherwise a newly allocated FBlueprintActionMenuItem.
		 */
		TSharedPtr<FEdGraphSchemaAction> MakeMenuItem(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action);
		
		/**
		 * Clears out any intermediate UBlueprintNodeSpawner actions that this
		 * instantiated.
		 */
		void Empty();
		
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
	 * @param  MenuSection		The primary section for the FBlueprintActionMenuBuilder.
	 * @param  BlueprintEditor	
	 * @param  MenuOut  		The menu builder we want all the legacy actions appended to.
	 */
	static void AppendLegacyItems(FMenuSectionDefinition const& MenuDef, TWeakPtr<FBlueprintEditor> BlueprintEditor, FBlueprintActionMenuBuilder& MenuOut);
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
TSharedPtr<FEdGraphSchemaAction> FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::MakeMenuItem(TWeakPtr<FBlueprintEditor> EditorContext, UBlueprintNodeSpawner* Action)
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
				Action = ItemFactory.MakePropertyActionProxy(PropertySpawner);
			}
		}
	}

	TSharedPtr<FEdGraphSchemaAction> MenuEntry = nullptr;
	if ((Action != nullptr) && !Filter.IsFiltered(Action))
	{
		MenuEntry = ItemFactory.MakeMenuItem(EditorContext, Action);
	}
	return MenuEntry;
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::Empty()
{
	ItemFactory.Empty();
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilderImpl::AppendLegacyItems(FMenuSectionDefinition const& MenuDef, TWeakPtr<FBlueprintEditor> BlueprintEditor, FBlueprintActionMenuBuilder& MenuOut)
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
		LegacyBuilder.SelectedObjects.Append(MenuContext.SelectedObjects);
		
		
		
		bool bIsContextSensitive = true;
		if (BlueprintEditor.IsValid())
		{
			bIsContextSensitive = BlueprintEditor.Pin()->GetIsContextSensitive();
		}
		
		if (bIsContextSensitive)
		{
			GraphSchema->GetGraphContextActions(LegacyBuilder);
			MenuOut.Append(LegacyBuilder);
		}
		else
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(Graph);
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
FBlueprintActionMenuBuilder::FBlueprintActionMenuBuilder(TWeakPtr<FBlueprintEditor> InBlueprintEditorPtr)
	: BlueprintEditorPtr(InBlueprintEditorPtr)
{
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
	for (TSharedRef<FMenuSectionDefinition> MenuSection : MenuSections)
	{
		// clear out intermediate actions that may have been spawned (like 
		// consolidated property actions).
		MenuSection->Empty();
	}
	
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
					TSharedPtr<FEdGraphSchemaAction> MenuEntry = MenuSection->MakeMenuItem(BlueprintEditorPtr, NodeSpawner);
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
		AppendLegacyItems(*MenuSections[0], BlueprintEditorPtr, *this);
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

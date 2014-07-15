// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EdGraph/EdGraphSchema.h" // for FGraphActionListBuilderBase

// Forward declarations
namespace FBlueprintActionMenuBuilderImpl
{
	// internal type, forward declared to hide implementation details
	struct FMenuSectionDefinition; 
};
class  FBlueprintActionFilter;
struct FBlueprintActionContext;

/**
 * Responsible for constructing a list of viable blueprint actions. Runs the 
 * blueprint actions database through a filter and spawns a series of 
 * FBlueprintActionMenuItems for actions that pass. Takes care of generating the 
 * each menu item's category/name/etc.
 */
struct BLUEPRINTGRAPH_API FBlueprintActionMenuBuilder : public FGraphActionListBuilderBase
{
public:
	/** Default constructor */
	FBlueprintActionMenuBuilder() {}
	
	/** Blueprint centric menu categories (not generic enough to make it into FCommonEditorCategory) */
	static FText const VariablesCategory;

	/**
	 * Good for constructing a menu built from a single filter. If the 
	 * bAutoBuild param is set to false, then know you have to manually call
	 * RebuildActionList() to generate the menu.
	 *
	 * @param  Filter		The filter you want applied for this main section of the menu.
	 * @param  bAutoBuild	If set, will automatically call RebuildActionList() here in the constructor.
	 */
	FBlueprintActionMenuBuilder(FBlueprintActionFilter const& Filter, bool bAutoBuild = true);

	/**
	 * Some action menus require multiple sections. One option is to create 
	 * multiple FBlueprintActionMenuBuilders and append them together, but that
	 * can be unperformant (each builder will run through the entire database
	 * separately)... This method provides an alternative, where you can specify
	 * a separate filter/heading/ordering for a sub-section of the menu.
	 *
	 * @param  Filter	 The filter you want applied to this section of the menu.
	 * @param  Heading	 The root category for this section of the menu (can be left blank).
	 * @param  MenuOrder The sort order to assign this section of the menu (higher numbers come first).
	 */
	void AddMenuSection(FBlueprintActionFilter const& Filter, FText const& Heading = FText::GetEmpty(), int32 MenuOrder = 0);
	
	/**
	 * Regenerates the entire menu list from the cached menu sections. Filters 
	 * and adds action items from the blueprint action database (as defined by 
	 * the MenuSections list).
	 */
	void RebuildActionList();

private:
	/** 
	 * Defines all the separate sections of the menu (filter, sort order, etc.).
	 * Defined as a TSharedRef<> so as to hide the implementation details (keep 
	 * this API clean).
	 */
	TArray< TSharedRef<FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition> > MenuSections;
};
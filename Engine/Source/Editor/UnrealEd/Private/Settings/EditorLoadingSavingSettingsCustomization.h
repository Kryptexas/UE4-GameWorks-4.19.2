// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EditorLoadingSavingSettingsCustomization.h: Declares the FEditorLoadingSavingSettingsCustomization class.
=============================================================================*/

#pragma once


#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"


#define LOCTEXT_NAMESPACE "FEditorLoadingSavingSettingsCustomization"


/**
 * Implements a details view customization for UGameMapsSettingsCustomization objects.
 */
class FEditorLoadingSavingSettingsCustomization
	: public IDetailCustomization
{
public:

	// Begin IDetailCustomization interface

	virtual void CustomizeDetails( IDetailLayoutBuilder& LayoutBuilder ) OVERRIDE
	{
		CustomizeStartupCategory(LayoutBuilder);
	}

	// End IDetailCustomization interface

public:

	/**
	 * Creates a new instance.
	 *
	 * @return A new struct customization for game maps settings.
	 */
	static TSharedRef<IDetailCustomization> MakeInstance( )
	{
		return MakeShareable(new FEditorLoadingSavingSettingsCustomization());
	}

protected:

	/**
	 * Customizes the Startup property category.
	 *
	 * This customization pulls in a setting from the game agnostic Editor settings, which
	 * are stored in a different UObject, but which we would like to show in this section.
	 *
	 * @param LayoutBuilder - The layout builder.
	 */
	void CustomizeStartupCategory( IDetailLayoutBuilder& LayoutBuilder )
	{
		IDetailCategoryBuilder& StartupCategory = LayoutBuilder.EditCategory("Startup");
		{
			TArray<UObject*> ObjectList;
			ObjectList.Add(&GEditor->AccessGameAgnosticSettings());
			StartupCategory.AddExternalProperty(ObjectList, "bLoadTheMostRecentlyLoadedProjectAtStartup");
		}
	}
};

#undef LOCTEXT_NAMESPACE
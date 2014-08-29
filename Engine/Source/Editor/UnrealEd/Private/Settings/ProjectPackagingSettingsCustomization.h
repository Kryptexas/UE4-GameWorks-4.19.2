// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyRestriction.h"


#define LOCTEXT_NAMESPACE "FProjectPackagingSettingsCustomization"


/**
 * Implements a details view customization for UProjectPackagingSettingsCustomization objects.
 */
class FProjectPackagingSettingsCustomization
	: public IDetailCustomization
{
public:

	// IDetailCustomization interface

	virtual void CustomizeDetails( IDetailLayoutBuilder& LayoutBuilder ) override
	{
		CustomizeProjectCategory(LayoutBuilder);
	}

public:

	/**
	 * Creates a new instance.
	 *
	 * @return A new struct customization for play-in settings.
	 */
	static TSharedRef<IDetailCustomization> MakeInstance( )
	{
		return MakeShareable(new FProjectPackagingSettingsCustomization());
	}

protected:

	/**
	 * Customizes the Project property category.
	 *
	 * @param LayoutBuilder The layout builder.
	 */
	void CustomizeProjectCategory( IDetailLayoutBuilder& LayoutBuilder )
	{
		// hide the DebugGame configuration for content-only games
		TArray<FString> TargetFileNames;
		IFileManager::Get().FindFiles(TargetFileNames, *(FPaths::GameSourceDir() / TEXT("*.target.cs")), true, false);

		if (TargetFileNames.Num() == 0)
		{
			IDetailCategoryBuilder& ProjectCategory = LayoutBuilder.EditCategory("Project");
			{
				TSharedRef<FPropertyRestriction> BuildConfigurationRestriction = MakeShareable(new FPropertyRestriction(LOCTEXT("DebugGameRestrictionReason", "The DebugGame build configuration is not available in content-only projects.")));
				BuildConfigurationRestriction->AddValue("DebugGame");

				TSharedRef<IPropertyHandle> BuildConfigurationHandle = LayoutBuilder.GetProperty("BuildConfiguration");
				BuildConfigurationHandle->AddRestriction(BuildConfigurationRestriction);
			}
		}
	}
};


#undef LOCTEXT_NAMESPACE

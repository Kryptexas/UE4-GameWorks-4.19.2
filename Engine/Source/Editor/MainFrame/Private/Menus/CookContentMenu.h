// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PackageProjectMenu.h: Declares the FPackageProjectMenu class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "FCookContentMenu"


/**
 * Static helper class for populating the "Cook Content" menu.
 */
class FCookContentMenu
{
public:

	/**
	 * Creates the menu.
	 *
	 * @param MenuBuilder - The builder for the menu that owns this menu.
	 */
	static void MakeMenu( FMenuBuilder& MenuBuilder )
	{
		TArray<ITargetPlatform*> Platforms = GetTargetPlatformManager()->GetTargetPlatforms();

		if (Platforms.Num() == 0)
		{
			return;
		}

		for (int32 PlatformIndex = 0; PlatformIndex < Platforms.Num(); ++PlatformIndex)
		{
			const ITargetPlatform* Platform = Platforms[PlatformIndex];

			// for the Editor we are only interested in cooking standalone games
			if (Platform->IsClientOnly() || Platform->IsServerOnly() || Platform->HasEditorOnlyData())
			{
				continue;
			}

			FUIAction Action(
				FExecuteAction::CreateStatic(&FMainFrameActionCallbacks::CookContent, Platform->PlatformName(), Platform->DisplayName()),
				FCanExecuteAction::CreateStatic(&FMainFrameActionCallbacks::CookContentCanExecute, Platform->PlatformName())
			);

			MenuBuilder.AddMenuEntry(
				Platform->DisplayName(),
				FText::Format(LOCTEXT("CookContentForPlatformTooltip", "Cook your game content for the {0} platform"), Platform->DisplayName()),
				FSlateIcon(FEditorStyle::GetStyleSetName(), *FString::Printf(TEXT("Launcher.Platform_%s"), *Platform->PlatformName())),
				Action
			);
		}
	}
};


#undef LOCTEXT_NAMESPACE

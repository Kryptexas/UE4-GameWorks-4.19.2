// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "LayersModule.h"

#define LOCTEXT_NAMESPACE "LayersCommands"

/**
 * The widget that represents a row in the LayersView's list view control.  Generates widgets for each column on demand.
 */
class SLayersCommandsMenu : public SCompoundWidget
{

public:

	SLATE_BEGIN_ARGS( SLayersCommandsMenu )
		:_CloseWindowAfterMenuSelection( true ) {}

		SLATE_ARGUMENT( bool, CloseWindowAfterMenuSelection )
	SLATE_END_ARGS()

	/**
	 * Construct this widget.  Called by the SNew() Slate macro.
	 *
	 * @param	InArgs				Declaration used by the SNew() macro to construct this widget
	 * @param	InImplementation	The UI logic not specific to slate
	 */
	void Construct( const FArguments& InArgs, const TSharedRef< FLayerCollectionViewModel > InViewModel/*, TSharedPtr< SLayersViewRow > InSingleSelectedRow */)
	{
		ViewModel = InViewModel;
		const FLayersViewCommands& Commands = FLayersViewCommands::Get();

		// Get all menu extenders for this context menu from the layers module
		FLayersModule& LayersModule = FModuleManager::GetModuleChecked<FLayersModule>( TEXT("Layers") );
		TArray<FLayersModule::FLayersMenuExtender> MenuExtenderDelegates = LayersModule.GetAllLayersMenuExtenders();

		TArray<TSharedPtr<FExtender>> Extenders;
		for (int32 i = 0; i < MenuExtenderDelegates.Num(); ++i)
		{
			if (MenuExtenderDelegates[i].IsBound())
			{
				Extenders.Add(MenuExtenderDelegates[i].Execute(ViewModel->GetCommandList()));
			}
		}
		TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

		// Build up the menu
		FMenuBuilder MenuBuilder( InArgs._CloseWindowAfterMenuSelection, ViewModel->GetCommandList(), MenuExtender );
		{ 
			MenuBuilder.BeginSection("LayersCreate", LOCTEXT( "MenuHeader", "Layers" ) );
			{
				MenuBuilder.AddMenuEntry( Commands.CreateEmptyLayer );
				MenuBuilder.AddMenuEntry( Commands.AddSelectedActorsToNewLayer );
				MenuBuilder.AddMenuEntry( Commands.AddSelectedActorsToSelectedLayer );
			}
			MenuBuilder.EndSection();

			MenuBuilder.BeginSection("LayersRemoveActors");
			{
				MenuBuilder.AddMenuEntry( Commands.RemoveSelectedActorsFromSelectedLayer );
			}
			MenuBuilder.EndSection();

			MenuBuilder.AddMenuEntry( FGenericCommands::Get().Delete, "DeleteLayer", LOCTEXT( "DeleteLayer", "Delete Layer" ),  LOCTEXT( "DeleteLayerToolTip", "Removes all actors from the selected layers and then deletes the layers" ) );
			MenuBuilder.AddMenuEntry( Commands.RequestRenameLayer );

			MenuBuilder.BeginSection("LayersSelection", LOCTEXT( "SelectionMenuHeader", "Selection" ) );
			{
				MenuBuilder.AddMenuEntry( Commands.SelectActors );
				MenuBuilder.AddMenuEntry( Commands.AppendActorsToSelection );
				MenuBuilder.AddMenuEntry( Commands.DeselectActors );
			}
			MenuBuilder.EndSection();

			MenuBuilder.BeginSection("LayersVisibility", LOCTEXT( "VisibilityMenuHeader", "Visibility" ) );
			{
				MenuBuilder.AddMenuEntry( Commands.MakeAllLayersVisible );
			}
			MenuBuilder.EndSection();
		}

		ChildSlot
		[
			MenuBuilder.MakeWidget()
		];
	}

private:
	/** The UI logic of the LayersView that is not Slate specific */
	TSharedPtr< FLayerCollectionViewModel > ViewModel;
};


#undef LOCTEXT_NAMESPACE

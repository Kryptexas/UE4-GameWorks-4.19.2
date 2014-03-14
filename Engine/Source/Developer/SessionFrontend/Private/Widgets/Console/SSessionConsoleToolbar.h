// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionConsoleToolbar.h: Declares the SSessionConsoleToolbar class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionConsoleToolbar"


/**
 * Implements the device toolbar widget.
 */
class SSessionConsoleToolbar
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionConsoleToolbar) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The construction arguments.
	 * @param CommandList - The command list to use.
	 */
	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct( const FArguments& InArgs, const TSharedRef<FUICommandList>& CommandList )
	{
		FSessionConsoleCommands::Register();

		// create the toolbar
		FToolBarBuilder Toolbar(CommandList, FMultiBoxCustomization::None);
		{
			Toolbar.AddToolBarButton(FSessionConsoleCommands::Get().Copy);
			Toolbar.AddSeparator();

			Toolbar.AddToolBarButton(FSessionConsoleCommands::Get().Clear);
			Toolbar.AddToolBarButton(FSessionConsoleCommands::Get().Save);
		}

		ChildSlot
		[
			SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(0.0f)
				[
					Toolbar.MakeWidget()
				]
		];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION
};


#undef LOCTEXT_NAMESPACE

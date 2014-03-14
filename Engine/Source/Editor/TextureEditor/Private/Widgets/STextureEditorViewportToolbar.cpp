// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	STextureEditorViewportToolbar.cpp: Implements the STextureEditorViewportToolbar class.
=============================================================================*/

#include "TextureEditorPrivatePCH.h"

#define LOCTEXT_NAMESPACE "STextureEditorViewportToolbar"


/* STextureEditorViewportToolbar interface
 *****************************************************************************/

void STextureEditorViewportToolbar::Construct( const FArguments& InArgs, const TSharedRef<FUICommandList>& InToolkitCommands )
{
	ToolkitCommands = InToolkitCommands;

	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(ViewOptionsMenuAnchor, SMenuAnchor)
					.Placement(MenuPlacement_ComboBox)
					[
						SNew(SButton)
							.ClickMethod(EButtonClickMethod::MouseDown)
							.ContentPadding(FMargin(5.0f, 2.0f))
							.VAlign(VAlign_Center)
							.ButtonStyle(FEditorStyle::Get(), "EditorViewportToolBar.MenuButton")
							.OnClicked(this, &STextureEditorViewportToolbar::HandleViewOptionsMenuButtonClicked)
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(STextBlock)
											.Text(LOCTEXT("ViewButtonText", "View"))
									]

								+ SHorizontalBox::Slot()
									.AutoWidth()
									.Padding(4.0f, 0.0f, 0.0f, 0.0f)
									[
										SNew(SImage)
											.Image(FEditorStyle::GetBrush("EditorViewportToolBar.MenuDropdown"))
									]
							]
					]
					.OnGetMenuContent(this, &STextureEditorViewportToolbar::GenerateViewOptionsMenu)
			]
	];
}


/* STextureEditorViewportToolbar implementation
 *****************************************************************************/

TSharedRef<SWidget> STextureEditorViewportToolbar::GenerateViewOptionsMenu( ) const
{
	FMenuBuilder MenuBuilder(true, ToolkitCommands);
	FTextureEditorViewOptionsMenu::MakeMenu(MenuBuilder);

	return MenuBuilder.MakeWidget();
}


FReply STextureEditorViewportToolbar::HandleViewOptionsMenuButtonClicked( )
{
	// If the menu button is clicked toggle the state of the menu anchor which will open or close the menu
	if (ViewOptionsMenuAnchor->ShouldOpenDueToClick())
	{
		ViewOptionsMenuAnchor->SetIsOpen( true );
	}
	else
	{
		ViewOptionsMenuAnchor->SetIsOpen( false );
	}

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE

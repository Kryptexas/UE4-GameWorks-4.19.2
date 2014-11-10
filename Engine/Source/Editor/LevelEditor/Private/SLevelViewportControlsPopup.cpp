// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "LevelEditor.h"
#include "SLevelViewportControlsPopup.h"
#include "SWebBrowser.h"

void SLevelViewportControlsPopup::Construct(const FArguments& InArgs)
{
	FVector2D PopupSize;
	FString PopupPath;

#if PLATFORM_WINDOWS
	PopupSize.Set(252, 558);
	PopupPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(*FPaths::EngineDir(), TEXT("Documentation/Source/Shared/Editor/Overlays/ViewportControlsWindows.html")));
#endif

	if (!PopupPath.IsEmpty())
	{
		// The size of the web browser is a hack to ensure that the browser shrinks,
		// otherwise it seems to show a scroll bar if the size never changes.
		ControlsWidget =
			SNew(SBox)
			.WidthOverride(PopupSize.X)
			.HeightOverride(PopupSize.Y)
			[
				SNew(SWebBrowser)
				.InitialURL(PopupPath)
				.ShowControls(false)
				.SupportsTransparency(true)
				.ViewportSize(PopupSize+1)
			];

		TAttribute<FText> ToolTipText = NSLOCTEXT("LevelViewportControlsPopup", "ViewportControlsToolTip", "Click to show Viewport Controls");
		Default = FEditorStyle::GetBrush("HelpIcon");
		Hovered = FEditorStyle::GetBrush("HelpIcon.Hovered");
		Pressed = FEditorStyle::GetBrush("HelpIcon.Pressed");

		ChildSlot
		[
			SAssignNew(MenuAnchor, SMenuAnchor)
			.Method(SMenuAnchor::UseCurrentWindow)
			.Placement(MenuPlacement_AboveAnchor)
			.OnGetMenuContent(this, &SLevelViewportControlsPopup::OnGetMenuContent)
			[
				SAssignNew(Button, SButton)
				.ContentPadding(5)
				.ButtonStyle(FEditorStyle::Get(), "HelpButton")
				.OnClicked(this, &SLevelViewportControlsPopup::OnClicked)
				.ClickMethod(EButtonClickMethod::MouseDown)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.ToolTipText(ToolTipText)
				[
					SAssignNew(ButtonImage, SImage)
					.Image(this, &SLevelViewportControlsPopup::GetButtonImage)
				]
			]
		];
	}
}

const FSlateBrush* SLevelViewportControlsPopup::GetButtonImage() const
{
	if (Button->IsPressed())
	{
		return Pressed;
	}

	if (ButtonImage->IsHovered())
	{
		return Hovered;
	}

	return Default;
}

FReply SLevelViewportControlsPopup::OnClicked() const
{
	// If the menu button is clicked toggle the state of the menu anchor which will open or close the menu
	if (MenuAnchor->ShouldOpenDueToClick())
	{
		MenuAnchor->SetIsOpen(true);
	}
	else
	{
		MenuAnchor->SetIsOpen(false);
	}

	return FReply::Handled();
}

TSharedRef<SWidget> SLevelViewportControlsPopup::OnGetMenuContent()
{
	return ControlsWidget.ToSharedRef();
}

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherCookedPlatforms.h: Declares the SSessionLauncherCookedPlatforms class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionLauncherCookedPlatforms"


/**
 * Implements the cooked platforms panel.
 */
class SSessionLauncherCookedPlatforms
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherCookedPlatforms) { }
	SLATE_END_ARGS()


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct(	const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
	{
		Model = InModel;

		ChildSlot
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					// platform menu
					MakePlatformMenu()
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 6.0f, 0.0f, 4.0f)
				[
					SNew(SSeparator)
						.Orientation(Orient_Horizontal)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.HAlign(HAlign_Right)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("SelectLabel", "Select:"))
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(8.0f, 0.0f)
						[
							// all platforms hyper-link
							SNew(SHyperlink)
								.OnNavigate(this, &SSessionLauncherCookedPlatforms::HandleAllPlatformsHyperlinkNavigate, true)
								.Text(LOCTEXT("AllPlatformsHyperlinkLabel", "All").ToString())
								.ToolTipText(LOCTEXT("AllPlatformsButtonTooltip", "Select all available platforms.").ToString())
								.Visibility(this, &SSessionLauncherCookedPlatforms::HandleAllPlatformsHyperlinkVisibility)									
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							// no platforms hyper-link
							SNew(SHyperlink)
								.OnNavigate(this, &SSessionLauncherCookedPlatforms::HandleAllPlatformsHyperlinkNavigate, false)
								.Text(LOCTEXT("NoPlatformsHyperlinkLabel", "None").ToString())
								.ToolTipText(LOCTEXT("NoPlatformsHyperlinkTooltip", "Deselect all platforms.").ToString())
								.Visibility(this, &SSessionLauncherCookedPlatforms::HandleAllPlatformsHyperlinkVisibility)									
						]
				]
		];
	}


protected:

	/**
	 * Builds the platform menu.
	 *
	 * @return Platform menu widget.
	 */
	TSharedRef<SWidget> MakePlatformMenu( ) const
	{
		TArray<ITargetPlatform*> Platforms = GetTargetPlatformManager()->GetTargetPlatforms();

		if (Platforms.Num() > 0)
		{
			TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);

			for (int32 PlatformIndex = 0; PlatformIndex < Platforms.Num(); ++PlatformIndex)
			{
				FString PlatformName = Platforms[PlatformIndex]->PlatformName();

				VerticalBox->AddSlot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						SNew(SCheckBox)
							.IsChecked(this, &SSessionLauncherCookedPlatforms::HandlePlatformMenuEntryIsChecked, PlatformName)
							.OnCheckStateChanged(this, &SSessionLauncherCookedPlatforms::HandlePlatformMenuEntryCheckedStateChanged, PlatformName)
							.Padding(FMargin(4.0f, 0.0f))
							.Content()
							[
								SNew(STextBlock)
									.ColorAndOpacity(this, &SSessionLauncherCookedPlatforms::HandlePlatformMenuEntryColorAndOpacity, PlatformName)
									.Text(PlatformName)
							]
					];
			}

			return VerticalBox;
		}

		return SNew(STextBlock)
			.ColorAndOpacity(FLinearColor::Red)
			.Text(LOCTEXT("NoPlatformsFoundErrorText", "Error: No platforms found.").ToString());
	}


private:

	// Callback for clicking the 'Select All Platforms' button.
	void HandleAllPlatformsHyperlinkNavigate( bool AllPlatforms )
	{
		ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

		if (SelectedProfile.IsValid())
		{
			if (AllPlatforms)
			{
				TArray<ITargetPlatform*> Platforms = GetTargetPlatformManager()->GetTargetPlatforms();

				for (int32 PlatformIndex = 0; PlatformIndex < Platforms.Num(); ++PlatformIndex)
				{
					SelectedProfile->AddCookedPlatform(Platforms[PlatformIndex]->PlatformName());
				}
			}
			else
			{
				SelectedProfile->ClearCookedPlatforms();
			}
		}
	}

	// Callback for determining the visibility of the 'Select All Platforms' button.
	EVisibility HandleAllPlatformsHyperlinkVisibility( ) const
	{
		if (GetTargetPlatformManager()->GetTargetPlatforms().Num() > 1)
		{
			return EVisibility::Visible;
		}

		return EVisibility::Collapsed;
	}

	// Callback for changing the checked state of a platform menu check box. 
	void HandlePlatformMenuEntryCheckedStateChanged( ESlateCheckBoxState::Type CheckState, FString PlatformName )
	{
		ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

		if (SelectedProfile.IsValid())
		{
			if (CheckState == ESlateCheckBoxState::Checked)
			{
				SelectedProfile->AddCookedPlatform(PlatformName);
			}
			else
			{
				SelectedProfile->RemoveCookedPlatform(PlatformName);
			}
		}
	}

	// Callback for getting the color of a platform menu check box.
	FSlateColor HandlePlatformMenuEntryColorAndOpacity( FString PlatformName ) const
	{
		ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

		if (SelectedProfile.IsValid())
		{
			ITargetPlatform* TargetPlatform = GetTargetPlatformManager()->FindTargetPlatform(PlatformName);

			if (TargetPlatform != NULL)
			{
//				if (TargetPlatform->HasValidBuild(SelectedProfile->GetProjectPath(), SelectedProfile->GetBuildConfiguration()))
				{
					return FEditorStyle::GetColor("Foreground");
				}
			}
		}

		return FLinearColor::Yellow;
	}

	// Callback for determining whether a platform menu entry is checked.
	ESlateCheckBoxState::Type HandlePlatformMenuEntryIsChecked( FString PlatformName ) const
	{
		ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

		if (SelectedProfile.IsValid())
		{
			if (SelectedProfile->GetCookedPlatforms().Contains(PlatformName))
			{
				return ESlateCheckBoxState::Checked;
			}
		}

		return ESlateCheckBoxState::Unchecked;
	}


private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};


#undef LOCTEXT_NAMESPACE

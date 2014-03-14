// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherProfileSelector.h: Declares the SSessionLauncherProfileSelector class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionLauncherProfileSelector"


/**
 * Implements a widget for launcher profile selection.
 */
class SSessionLauncherProfileSelector
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherProfileSelector) { }
	SLATE_END_ARGS()


public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherProfileSelector( )
	{
		if (Model.IsValid())
		{
			Model->OnProfileListChanged().RemoveAll(this);
			Model->OnProfileSelected().RemoveAll(this);
		}
	}


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
	{
		Model = InModel;

		ChildSlot
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("ProfileComboBoxLabel", "Profile:").ToString())
				]

			+ SHorizontalBox::Slot()
				.FillWidth(1.0)
				.Padding(8.0, 0.0, 0.0, 0.0)
				[
					// profile combo box
					SAssignNew(ProfileComboBox, SEditableComboBox<ILauncherProfilePtr>)
						.InitiallySelectedItem(Model->GetSelectedProfile())
						.OptionsSource(&Model->GetProfileManager()->GetAllProfiles())
						.AddButtonToolTip(LOCTEXT("AddProfileButtonToolTip", "Add a new profile").ToString())
						.RemoveButtonToolTip(LOCTEXT("DeleteProfileButtonToolTip", "Delete the selected profile").ToString())
						.OnAddClicked(this, &SSessionLauncherProfileSelector::HandleProfileComboBoxAddClicked)
						.OnGenerateWidget(this, &SSessionLauncherProfileSelector::HandleProfileComboBoxGenerateWidget)
						.OnGetEditableText(this, &SSessionLauncherProfileSelector::HandleProfileComboBoxGetEditableText)
						.OnRemoveClicked(this, &SSessionLauncherProfileSelector::HandleProfileComboBoxRemoveClicked)
						.OnSelectionChanged(this, &SSessionLauncherProfileSelector::HandleProfileComboBoxSelectionChanged)
						.IsRenameVisible(EVisibility::Hidden)
						.Content()
						[
							SNew(SEditableText)
								.Text(this, &SSessionLauncherProfileSelector::HandleProfileComboBoxContent)
								.OnTextCommitted(this, &SSessionLauncherProfileSelector::HandleProfileComboBoxSelectionRenamed)
						]
				]
		];

		Model->OnProfileSelected().AddSP(this, &SSessionLauncherProfileSelector::HandleProfileManagerProfileSelected);
		Model->OnProfileListChanged().AddSP(this, &SSessionLauncherProfileSelector::HandleProfileManagerProfileListChanged);
	}


private:

	// Callback for clicking the 'Add' button on the profile combo box.
	FReply HandleProfileComboBoxAddClicked( )
	{
		Model->GetProfileManager()->AddNewProfile();

		ProfileComboBox->RefreshOptions();

		return FReply::Handled();
	}

	// Callback for getting the content of the profile combo box.
	FText HandleProfileComboBoxContent( ) const
	{
		ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

		if (SelectedProfile.IsValid())
		{
			return FText::FromString(SelectedProfile->GetName());
		}

		return LOCTEXT("CreateProfileText", "Create a profile...");
	}

	// Callback for generating a row for the profile combo box.
	TSharedRef<SWidget> HandleProfileComboBoxGenerateWidget( ILauncherProfilePtr InItem )
	{
		return SNew(STextBlock)
			.Text(this, &SSessionLauncherProfileSelector::HandleProfileComboBoxWidgetText, InItem);
	}

	// Callback for getting the editable text of the currently selected profile item.
	FString HandleProfileComboBoxGetEditableText( )
	{
		ILauncherProfilePtr SelectedProfile = ProfileComboBox->GetSelectedItem();

		if (SelectedProfile.IsValid())
		{
			return SelectedProfile->GetName();
		}

		return FString();
	}

	// Callback for clicking the 'Remove' button on the profile combo box.
	FReply HandleProfileComboBoxRemoveClicked( )
	{
		ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

		if (SelectedProfile.IsValid())
		{
			Model->GetProfileManager()->RemoveProfile(SelectedProfile.ToSharedRef());

			ProfileComboBox->RefreshOptions();
		}

		return FReply::Handled();
	}

	// Callback for clicking the 'Rename device group' button.
	FReply HandleProfileComboBoxRenameClicked( )
	{
		return FReply::Handled();
	}

	// Callback for changing the selected profile in the profile combo box.
	void HandleProfileComboBoxSelectionChanged( ILauncherProfilePtr Selection, ESelectInfo::Type SelectInfo )
	{
		Model->SelectProfile(Selection);
	}

	// Callback for when the selected item in the profile combo box has been renamed.
	void HandleProfileComboBoxSelectionRenamed( const FText& CommittedText, ETextCommit::Type )
	{
		if (ProfileComboBox->GetSelectedItem().IsValid())
		{
			ProfileComboBox->GetSelectedItem()->SetName(CommittedText.ToString());
		}
	}

	// Callback for getting the text of a profile combo box widget.
	FString HandleProfileComboBoxWidgetText( ILauncherProfilePtr Profile ) const
	{
		if (Profile.IsValid())
		{
			return Profile->GetName();
		}
	
		return FString();
	}

	// Callback for changing the list of profiles in the profile manager.
	void HandleProfileManagerProfileListChanged( )
	{
		ProfileComboBox->RefreshOptions();
	}

	// Callback for changing the selected profile in the profile manager.
	void HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile )
	{
		ProfileComboBox->SetSelectedItem(SelectedProfile);
	}


private:

	// Holds the profile combo box.
	TSharedPtr<SEditableComboBox<ILauncherProfilePtr> > ProfileComboBox;

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};


#undef LOCTEXT_NAMESPACE

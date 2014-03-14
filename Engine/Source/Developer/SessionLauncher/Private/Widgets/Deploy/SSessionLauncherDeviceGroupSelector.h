// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherDeviceGroupSelector.h: Declares the SSessionLauncherDeviceGroupSelector class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionLauncherDeviceGroupSelector"


/**
 * Delegate type for device group selection changes.
 *
 * The first parameter is the selected device group (or NULL if the previously selected group was unselected).
 */
DECLARE_DELEGATE_OneParam(FOnSessionLauncherDeviceGroupSelected, const ILauncherDeviceGroupPtr&)


/**
 * Implements a widget for device group selection.
 */
class SSessionLauncherDeviceGroupSelector
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherDeviceGroupSelector) { }

		/**
		 * Exposes the initially selected device group.
		 */
		SLATE_ARGUMENT(ILauncherDeviceGroupPtr, InitiallySelectedGroup)
		
		/**
		 * Exposes a delegate to be invoked when a different device group has been selected.
		 */
		SLATE_EVENT(FOnSessionLauncherDeviceGroupSelected, OnGroupSelected)

	SLATE_END_ARGS()


public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherDeviceGroupSelector( )
	{
		if (ProfileManager.IsValid())
		{
			ProfileManager->OnDeviceGroupAdded().RemoveAll(this);
			ProfileManager->OnDeviceGroupRemoved().RemoveAll(this);
		}
	}


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InProfileManager - The profile manager to use.
	 */
	void Construct( const FArguments& InArgs, const ILauncherProfileManagerRef& InProfileManager )
	{
		OnGroupSelected = InArgs._OnGroupSelected;

		ProfileManager = InProfileManager;

		ChildSlot
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSessionLauncherFormLabel)
						.LabelText(LOCTEXT("DeviceGroupComboBoxLabel", "Device group to deploy to:"))
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0, 4.0, 0.0, 0.0)
				[
					SAssignNew(DeviceGroupComboBox, SEditableComboBox<ILauncherDeviceGroupPtr>)
						.InitiallySelectedItem(InArgs._InitiallySelectedGroup)
						.OptionsSource(&ProfileManager->GetAllDeviceGroups())
						.AddButtonToolTip(LOCTEXT("AddProfileButtonToolTip", "Add a new device group"))
						.RemoveButtonToolTip(LOCTEXT("DeleteProfileButtonToolTip", "Delete the selected device group"))
						.RenameButtonToolTip(LOCTEXT("RenameProfileButtonToolTip", "Rename the selected device group"))
						.OnAddClicked(this, &SSessionLauncherDeviceGroupSelector::HandleDeviceGroupComboBoxAddClicked)
						.OnGenerateWidget(this, &SSessionLauncherDeviceGroupSelector::HandleDeviceGroupComboBoxGenerateWidget)
						.OnGetEditableText(this, &SSessionLauncherDeviceGroupSelector::HandleDeviceGroupComboBoxGetEditableText)
						.OnRemoveClicked(this, &SSessionLauncherDeviceGroupSelector::HandleDeviceGroupComboBoxRemoveClicked)
						.OnSelectionChanged(this, &SSessionLauncherDeviceGroupSelector::HandleDeviceGroupComboBoxSelectionChanged)
						.OnSelectionRenamed(this, &SSessionLauncherDeviceGroupSelector::HandleDeviceGroupComboBoxSelectionRenamed)
						.Content()
						[
							SNew(STextBlock)
								.Text(this, &SSessionLauncherDeviceGroupSelector::HandleDeviceGroupComboBoxContent)
						]
				]
		];

		ProfileManager->OnDeviceGroupAdded().AddSP(this, &SSessionLauncherDeviceGroupSelector::HandleProfileManagerDeviceGroupsChanged);
		ProfileManager->OnDeviceGroupRemoved().AddSP(this, &SSessionLauncherDeviceGroupSelector::HandleProfileManagerDeviceGroupsChanged);
	}


	/**
	 * Gets the currently selected device group.
	 *
	 * @return The selected group, or NULL if no group is selected.
	 *
	 * @see SetSelectedGroup
	 */
	ILauncherDeviceGroupPtr GetSelectedGroup( ) const
	{
		return DeviceGroupComboBox->GetSelectedItem();
	}

	/**
	 * Sets the selected device group.
	 *
	 * @param DeviceGroup - The device group to select.
	 *
	 * @see GetSelectedGroup
	 */
	void SetSelectedGroup( const ILauncherDeviceGroupPtr& DeviceGroup )
	{
		if (!DeviceGroup.IsValid() || ProfileManager->GetAllDeviceGroups().Contains(DeviceGroup))
		{
			DeviceGroupComboBox->SetSelectedItem(DeviceGroup);
		}		
	}


private:

	// Callback for clicking the 'Add device group' button
	FReply HandleDeviceGroupComboBoxAddClicked( )
	{
		ILauncherDeviceGroupPtr NewGroup = ProfileManager->AddNewDeviceGroup();

		DeviceGroupComboBox->SetSelectedItem(NewGroup);

		return FReply::Handled();
	}

	// Callback for getting the device group combo box content.
	FString HandleDeviceGroupComboBoxContent() const
	{
		ILauncherDeviceGroupPtr SelectedGroup = DeviceGroupComboBox->GetSelectedItem();

		if (SelectedGroup.IsValid())
		{
			return SelectedGroup->GetName();
		}

		return LOCTEXT("CreateOrSelectGroupText", "Create or select a device group...").ToString();
	}

	// Callback for generating a row for the device group combo box.
	TSharedRef<SWidget> HandleDeviceGroupComboBoxGenerateWidget( ILauncherDeviceGroupPtr InItem )
	{
		return SNew(STextBlock)
			.Text(this, &SSessionLauncherDeviceGroupSelector::HandleDeviceGroupComboBoxWidgetText, InItem);
	}

	// Callback for getting the editable text of the currently selected device group.
	FString HandleDeviceGroupComboBoxGetEditableText( )
	{
		ILauncherDeviceGroupPtr SelectedGroup = DeviceGroupComboBox->GetSelectedItem();

		if (SelectedGroup.IsValid())
		{
			return SelectedGroup->GetName();
		}

		return FString();
	}

	// Callback for clicking the 'Delete device group' button
	FReply HandleDeviceGroupComboBoxRemoveClicked( )
	{
		ILauncherDeviceGroupPtr SelectedGroup = DeviceGroupComboBox->GetSelectedItem();

		if (SelectedGroup.IsValid())
		{
			ProfileManager->RemoveDeviceGroup(SelectedGroup.ToSharedRef());
		}

		if (ProfileManager->GetAllDeviceGroups().Num() > 0)
		{
			DeviceGroupComboBox->SetSelectedItem(ProfileManager->GetAllDeviceGroups()[0]);
		}
		else
		{
			DeviceGroupComboBox->SetSelectedItem(NULL);
		}

		return FReply::Handled();
	}

	// Callback for changing the selected device group in the device group combo box.
	void HandleDeviceGroupComboBoxSelectionChanged( ILauncherDeviceGroupPtr Selection, ESelectInfo::Type SelectInfo )
	{
		OnGroupSelected.ExecuteIfBound(Selection);
	}

	// Callback for when the selected item in the device group combo box has been renamed.
	void HandleDeviceGroupComboBoxSelectionRenamed( const FText& CommittedText, ETextCommit::Type )
	{
		DeviceGroupComboBox->GetSelectedItem()->SetName(CommittedText.ToString());
	}

	// Callback for getting the text of a device group combo box widget.
	FString HandleDeviceGroupComboBoxWidgetText( ILauncherDeviceGroupPtr Group ) const
	{
		if (Group.IsValid())
		{
			return Group->GetName();
		}

		return FString();
	}

	// Callback for changing the list of groups in the profile manager.
	void HandleProfileManagerDeviceGroupsChanged( const ILauncherDeviceGroupRef& /*ChangedProfile*/ )
	{
		DeviceGroupComboBox->RefreshOptions();
	}


private:

	// Holds the device group combo box.
	TSharedPtr<SEditableComboBox<ILauncherDeviceGroupPtr> > DeviceGroupComboBox;

	// Holds the profile manager.
	ILauncherProfileManagerPtr ProfileManager;


private:

	// Holds a delegate to be invoked when a different device group has been selected.
	FOnSessionLauncherDeviceGroupSelected OnGroupSelected;
};


#undef LOCTEXT_NAMESPACE

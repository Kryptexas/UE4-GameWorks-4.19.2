// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherLaunchRoleEditor.h: Declares the SSessionLauncherLaunchRoleEditor class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionLauncherLaunchRoleEditor"


/**
 * Implements the settings panel for a single launch role.
 */
class SSessionLauncherLaunchRoleEditor
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherLaunchRoleEditor)
		: _AvailableCultures()
		, _AvailableMaps()
	{ }
		
		/**
		 * The role to be edited initially.
		 */
		SLATE_ARGUMENT(ILauncherProfileLaunchRolePtr, InitialRole)

		/**
		 * The list of available cultures.
		 */
		SLATE_ARGUMENT(const TArray<FString>*, AvailableCultures)

		/**
		 * The list of available maps.
		 */
		SLATE_ARGUMENT(const TArray<FString>*, AvailableMaps)

	SLATE_END_ARGS()


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InRole - The launch role to edit.
	 */
	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct(	const FArguments& InArgs )
	{
		AvailableCultures = InArgs._AvailableCultures;
		AvailableMaps = InArgs._AvailableMaps;
		Role = InArgs._InitialRole;

		// create instance types menu
		FMenuBuilder InstanceTypeMenuBuilder(true, NULL);
		{
			FUIAction StandaloneClientAction(FExecuteAction::CreateSP(this, &SSessionLauncherLaunchRoleEditor::HandleInstanceTypeMenuEntryClicked, ELauncherProfileRoleInstanceTypes::StandaloneClient));
			InstanceTypeMenuBuilder.AddMenuEntry(LOCTEXT("StandaloneClient", "Standalone Client"), LOCTEXT("StandaloneClientActionHint", "Launch this instance as a standalone game client."), FSlateIcon(), StandaloneClientAction);

			FUIAction ListenServerAction(FExecuteAction::CreateSP(this, &SSessionLauncherLaunchRoleEditor::HandleInstanceTypeMenuEntryClicked, ELauncherProfileRoleInstanceTypes::ListenServer));
			InstanceTypeMenuBuilder.AddMenuEntry(LOCTEXT("ListenServer", "Listen Server"), LOCTEXT("ListenServerActionHint", "Launch this instance as a game client that can accept connections from other clients."), FSlateIcon(), ListenServerAction);

			FUIAction DedicatedServerAction(FExecuteAction::CreateSP(this, &SSessionLauncherLaunchRoleEditor::HandleInstanceTypeMenuEntryClicked, ELauncherProfileRoleInstanceTypes::DedicatedServer));
			InstanceTypeMenuBuilder.AddMenuEntry(LOCTEXT("DedicatedServer", "Dedicated Server"), LOCTEXT("DedicatedServerActionHint", "Launch this instance as a dedicated game server."), FSlateIcon(), DedicatedServerAction);

			FUIAction UnrealEditorAction(FExecuteAction::CreateSP(this, &SSessionLauncherLaunchRoleEditor::HandleInstanceTypeMenuEntryClicked, ELauncherProfileRoleInstanceTypes::UnrealEditor));
			InstanceTypeMenuBuilder.AddMenuEntry(LOCTEXT("UnrealEditor", "Unreal Editor"), LOCTEXT("UnrealEditorActionHint", "Launch this instance as an Unreal Editor."), FSlateIcon(), UnrealEditorAction);
		}

		ChildSlot
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSessionLauncherFormLabel)
						.LabelText(LOCTEXT("InstanceTypeComboBoxLabel", "Launch As:"))
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					// instance type menu
					SNew(SComboButton)
						.ButtonContent()
						[
							SNew(STextBlock)
								.Text(this, &SSessionLauncherLaunchRoleEditor::HandleInstanceTypeComboButtonContentText)
						]
						.ContentPadding(FMargin(6.0f, 2.0f))
						.MenuContent()
						[
							InstanceTypeMenuBuilder.MakeWidget()
						]
				]
									
			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SSessionLauncherFormLabel)
						.ErrorToolTipText(LOCTEXT("CultureNotAvailableError", "The selected culture is not being cooked or is not available."))
						.ErrorVisibility(this, &SSessionLauncherLaunchRoleEditor::HandleCultureValidationErrorIconVisibility)
						.LabelText(LOCTEXT("InitialCultureTextBoxLabel", "Initial Culture:"))
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					// initial culture combo box
					SAssignNew(CultureComboBox, STextComboBox)
						.ColorAndOpacity(this, &SSessionLauncherLaunchRoleEditor::HandleCultureComboBoxColorAndOpacity)
						.OptionsSource(&CultureList)
						.OnSelectionChanged(this, &SSessionLauncherLaunchRoleEditor::HandleCultureComboBoxSelectionChanged)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SSessionLauncherFormLabel)
						.ErrorToolTipText(LOCTEXT("MapNotAvailableError", "The selected map is not being cooked or is not available."))
						.ErrorVisibility(this, &SSessionLauncherLaunchRoleEditor::HandleMapValidationErrorIconVisibility)
						.LabelText(LOCTEXT("InitialMapTextBoxLabel", "Initial Map:"))
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					// initial map combo box
					SAssignNew(MapComboBox, STextComboBox)
						.ColorAndOpacity(this, &SSessionLauncherLaunchRoleEditor::HandleMapComboBoxColorAndOpacity)
						.OptionsSource(&MapList)
						.OnSelectionChanged(this, &SSessionLauncherLaunchRoleEditor::HandleMapComboBoxSelectionChanged)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SSessionLauncherFormLabel)
						.LabelText(LOCTEXT("CommandLineTextBoxLabel", "Additional Command Line Parameters:"))
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					// command line text box
					SAssignNew(CommandLineTextBox, SEditableTextBox)
						.OnTextChanged(this, &SSessionLauncherLaunchRoleEditor::HandleCommandLineTextBoxTextChanged)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 12.0f, 0.0f, 0.0f)
				[
					// v-sync check box
					SNew(SCheckBox)
						.IsChecked(this, &SSessionLauncherLaunchRoleEditor::HandleVsyncCheckBoxIsChecked)
						.OnCheckStateChanged(this, &SSessionLauncherLaunchRoleEditor::HandleVsyncCheckBoxCheckStateChanged)
						.Padding(FMargin(4.0f, 0.0f))
						.Content()
						[
							SNew(STextBlock)
								.Text(LOCTEXT("VsyncCheckBoxText", "Synchronize Screen Refresh Rate (VSync)"))
						]
				]
		];

		Refresh(InArgs._InitialRole);
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	/**
	 * Refreshes the widget.
	 *
	 * @param InRole - The role to edit, or NULL if no role is being edited.
	 */
	void Refresh( const ILauncherProfileLaunchRolePtr& InRole )
	{
		Role = InRole;

		ILauncherProfileLaunchRolePtr RolePtr = Role.Pin();

		CultureList.Reset();
		CultureList.Add(MakeShareable(new FString(LOCTEXT("DefaultCultureText", "<default>").ToString())));

		MapList.Reset();
		MapList.Add(MakeShareable(new FString(LOCTEXT("DefaultMapText", "<default>").ToString())));

		if (RolePtr.IsValid())
		{
			// refresh widgets
			CommandLineTextBox->SetText(FText::FromString(*RolePtr->GetCommandLine()));

			// rebuild culture list
			if (AvailableCultures != NULL)
			{
				for (int32 AvailableCultureIndex = 0; AvailableCultureIndex < AvailableCultures->Num(); ++AvailableCultureIndex)
				{
					TSharedPtr<FString> Culture = MakeShareable(new FString((*AvailableCultures)[AvailableCultureIndex]));

					CultureList.Add(Culture);

					if (*Culture == RolePtr->GetInitialCulture())
					{
						CultureComboBox->SetSelectedItem(Culture);
					}
				}
			}

			if (RolePtr->GetInitialCulture().IsEmpty() || (CultureList.Num() == 0))
			{
				CultureComboBox->SetSelectedItem(CultureList[0]);
			}

			// rebuild map list
			if (AvailableMaps != NULL)
			{
				for (int32 AvailableMapIndex = 0; AvailableMapIndex < AvailableMaps->Num(); ++AvailableMapIndex)
				{
					TSharedPtr<FString> Map = MakeShareable(new FString((*AvailableMaps)[AvailableMapIndex]));

					MapList.Add(Map);

					if (*Map == RolePtr->GetInitialMap())
					{
						MapComboBox->SetSelectedItem(Map);
					}
				}
			}

			if (RolePtr->GetInitialMap().IsEmpty() || (MapList.Num() == 0))
			{
				MapComboBox->SetSelectedItem(MapList[0]);
			}
		}
		else
		{
			CommandLineTextBox->SetText(FText::GetEmpty());
			CultureComboBox->ClearSelection();
			MapComboBox->ClearSelection();
		}

		CultureComboBox->RefreshOptions();
		MapComboBox->RefreshOptions();
	}


private:

	// Callback for when the text of the command line text box has changed.
	void HandleCommandLineTextBoxTextChanged( const FText& InText )
	{
		ILauncherProfileLaunchRolePtr RolePtr = Role.Pin();

		if (RolePtr.IsValid())
		{
			RolePtr->SetCommandLine(InText.ToString());
		}
	}

	// Callback for getting the name of the selected instance type.
	FString HandleInstanceTypeComboButtonContentText( ) const
	{
		ILauncherProfileLaunchRolePtr RolePtr = Role.Pin();

		if (RolePtr.IsValid())
		{
			return ELauncherProfileRoleInstanceTypes::ToString(RolePtr->GetInstanceType());
		}

		return FString();
	}

	// Callback for clicking an item in the 'Instance type' menu.
	void HandleInstanceTypeMenuEntryClicked( ELauncherProfileRoleInstanceTypes::Type InstanceType )
	{
		ILauncherProfileLaunchRolePtr RolePtr = Role.Pin();

		if (RolePtr.IsValid())
		{
			RolePtr->SetInstanceType(InstanceType);
		}
	}

	// Callback for getting the foreground text color of the culture combo box.
	FSlateColor HandleCultureComboBoxColorAndOpacity( ) const
	{
		ILauncherProfileLaunchRolePtr RolePtr = Role.Pin();

		if (RolePtr.IsValid())
		{
			const FString& InitialCulture = RolePtr->GetInitialCulture();

			if (InitialCulture.IsEmpty() || (AvailableCultures->Contains(InitialCulture)))
			{
				return FSlateColor::UseForeground();
			}
		}

		return FLinearColor::Red;
	}

	// Callback for changing the initial culture selection.
	void HandleCultureComboBoxSelectionChanged( TSharedPtr<FString> Selection, ESelectInfo::Type SelectInfo )
	{
		ILauncherProfileLaunchRolePtr RolePtr = Role.Pin();

		if (RolePtr.IsValid())
		{
			if (Selection.IsValid() && (Selection != CultureList[0]))
			{
				RolePtr->SetInitialCulture(*Selection);
			}
			else
			{
				RolePtr->SetInitialCulture(FString());
			}
		}
	}

	// Callback for getting the visibility of the validation error icon of the 'Initial Culture' combo box.
	EVisibility HandleCultureValidationErrorIconVisibility( ) const
	{
		ILauncherProfileLaunchRolePtr RolePtr = Role.Pin();

		if (RolePtr.IsValid())
		{
			const FString& InitialCulture = RolePtr->GetInitialCulture();

			if (InitialCulture.IsEmpty() || (AvailableCultures->Contains(InitialCulture)))
			{
				return EVisibility::Hidden;
			}
		}

		return EVisibility::Visible;
	}

	// Callback for getting the visibility of the validation error icon of the 'Launch As' combo box.
	EVisibility HandleLaunchAsValidationErrorIconVisibility( ) const
	{
		return EVisibility::Hidden;
	}

	// Callback for getting the foreground text color of the map combo box.
	FSlateColor HandleMapComboBoxColorAndOpacity( ) const
	{
		ILauncherProfileLaunchRolePtr RolePtr = Role.Pin();

		if (RolePtr.IsValid())
		{
			const FString& InitialMap = RolePtr->GetInitialMap();

			if (InitialMap.IsEmpty() || (AvailableMaps->Contains(InitialMap)))
			{
				return FSlateColor::UseForeground();
			}
		}

		return FLinearColor::Red;
	}

	// Callback for changing the initial map selection.
	void HandleMapComboBoxSelectionChanged( TSharedPtr<FString> Selection, ESelectInfo::Type SelectInfo )
	{
		ILauncherProfileLaunchRolePtr RolePtr = Role.Pin();

		if (RolePtr.IsValid())
		{
			if (Selection.IsValid() && (Selection != MapList[0]))
			{
				RolePtr->SetInitialMap(*Selection);
			}
			else
			{
				RolePtr->SetInitialMap(FString());
			}
		}
	}

	// Callback for getting the visibility of the validation error icon of the 'Initial Map' combo box.
	EVisibility HandleMapValidationErrorIconVisibility( ) const
	{
		ILauncherProfileLaunchRolePtr RolePtr = Role.Pin();

		if (RolePtr.IsValid())
		{
			const FString& InitialMap = RolePtr->GetInitialMap();

			if (InitialMap.IsEmpty() || (AvailableMaps->Contains(InitialMap)))
			{
				return EVisibility::Hidden;
			}
		}

		return EVisibility::Visible;
	}

	// Callback for checking the specified 'No VSync' check box.
	void HandleVsyncCheckBoxCheckStateChanged( ESlateCheckBoxState::Type NewState )
	{
		ILauncherProfileLaunchRolePtr RolePtr = Role.Pin();

		if (RolePtr.IsValid())
		{
			RolePtr->SetVsyncEnabled(NewState == ESlateCheckBoxState::Checked);
		}
	}

	// Callback for determining the checked state of the specified 'No VSync' check box.
	ESlateCheckBoxState::Type HandleVsyncCheckBoxIsChecked( ) const
	{
		ILauncherProfileLaunchRolePtr RolePtr = Role.Pin();

		if (RolePtr.IsValid())
		{
			if (RolePtr->IsVsyncEnabled())
			{
				return ESlateCheckBoxState::Checked;
			}
		}

		return ESlateCheckBoxState::Unchecked;
	}


private:

	// Holds a pointer to the list of available cultures.
	const TArray<FString>* AvailableCultures;

	// Holds a pointer to the list of available maps.
	const TArray<FString>* AvailableMaps;

	// Holds the command line text box.
	TSharedPtr<SEditableTextBox> CommandLineTextBox;

	// Holds the instance type combo box.
	TSharedPtr<SComboBox<TSharedPtr<ELauncherProfileRoleInstanceTypes::Type> > > InstanceTypeComboBox;

	// Holds the list of instance types.
	TArray<TSharedPtr<ELauncherProfileRoleInstanceTypes::Type> > InstanceTypeList;

	// Holds the culture text box.
	TSharedPtr<STextComboBox> CultureComboBox;

	// Holds the list of cultures that are available for the selected game.
	TArray<TSharedPtr<FString> > CultureList;

	// Holds the map text box.
	TSharedPtr<STextComboBox> MapComboBox;

	// Holds the list of maps that are available for the selected game.
	TArray<TSharedPtr<FString> > MapList;

	// Holds the profile that owns the role being edited.
	TWeakPtr<ILauncherProfile> Profile;

	// Holds a pointer to the role that is being edited in this widget.
	TWeakPtr<ILauncherProfileLaunchRole> Role;
};


#undef LOCTEXT_NAMESPACE

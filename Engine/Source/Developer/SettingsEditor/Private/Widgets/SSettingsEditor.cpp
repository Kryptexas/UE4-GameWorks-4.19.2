// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// Implements the SSettingsEditor class.

#include "SettingsEditorPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSettingsEditor"


class SSettingsFileCheckoutNotice : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSettingsFileCheckoutNotice) { }
		/** Called to determine if the associated file is unlocked */
		SLATE_ATTRIBUTE(bool, Unlocked)
	
		/** Slot for this button's content (optional) */
		SLATE_NAMED_SLOT(FArguments, LockedContent)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TAttribute<bool> bIsUnlocked;

private:

	// Callback for determining the visibility of the check-out button.
	EVisibility HandleCheckOutButtonVisibility() const;

	// Callback for getting the visibility of the 'Source control unavailable' text block.
	FText HandleSccUnavailableTextBlockText() const;

	// Callback for getting the widget index for the notice switcher.
	int32 HandleNoticeSwitcherWidgetIndex() const
	{
		return bIsUnlocked.Get() ? 1 : 0;
	}
};

EVisibility SSettingsFileCheckoutNotice::HandleCheckOutButtonVisibility() const
{
	if (ISourceControlModule::Get().IsEnabled() && ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

FText SSettingsFileCheckoutNotice::HandleSccUnavailableTextBlockText() const
{
	if (!ISourceControlModule::Get().IsEnabled())
	{
		return LOCTEXT("SccDisabledNotice", "[Source control is disabled]");
	}

	if (!ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		return LOCTEXT("SccUnavailableNotice", "[Source control server unavailable]");
	}

	return FText::GetEmpty();
}


void SSettingsFileCheckoutNotice::Construct(const FArguments& InArgs)
{
	bIsUnlocked = InArgs._Unlocked;

	// default configuration notice
	ChildSlot
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor::Yellow)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.0f)
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex(this, &SSettingsFileCheckoutNotice::HandleNoticeSwitcherWidgetIndex)

			// Locked slot
			+ SWidgetSwitcher::Slot()
			[
				SNew(SHorizontalBox)

				// Status icon
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("GenericLock"))
				]

				// Notice
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(16.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DefaultSettingsNotice_Source", "These settings are always saved in the default configuration file, which is currently under source control."))
				]

				// Call to action
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					InArgs._LockedContent.Widget
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(this, &SSettingsFileCheckoutNotice::HandleSccUnavailableTextBlockText)
				]
			]

			// Unlocked slot
			+ SWidgetSwitcher::Slot()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("GenericUnlock"))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(16.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DefaultSettingsNotice_Writable", "These settings are always saved in the default configuration file, which is currently writable."))
				]
			]
		]
	];
}




/* SSettingsEditor structors
 *****************************************************************************/

SSettingsEditor::~SSettingsEditor( )
{
	Model->OnSelectionChanged().RemoveAll(this);
	SettingsContainer->OnCategoryModified().RemoveAll(this);
}


/* SSettingsEditor interface
 *****************************************************************************/

void SSettingsEditor::Construct( const FArguments& InArgs, const ISettingsEditorModelRef& InModel )
{
	Model = InModel;
	DefaultConfigCheckOutTimer = 0.0f;
	DefaultConfigCheckOutNeeded = false;
	SettingsContainer = InModel->GetSettingsContainer();

	// initialize settings view
	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = true;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bObjectsUseNameArea = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.NotifyHook = this;
		DetailsViewArgs.bShowOptions = true;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;
	}

	SettingsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	SettingsView->SetEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SSettingsEditor::HandleSettingsViewEnabled)));
	SettingsView->SetVisibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &SSettingsEditor::HandleSettingsViewVisibility)));

	TSharedRef<SWidget> ConfigNoticeWidget =
		SNew(SSettingsFileCheckoutNotice)
		.Visibility(this, &SSettingsEditor::HandleDefaultConfigNoticeVisibility)
		.Unlocked(this, &SSettingsEditor::IsDefaultConfigEditable)
		.LockedContent()
		[
			SNew(SButton)
			.OnClicked(this, &SSettingsEditor::HandleCheckOutButtonClicked)
			.Text(LOCTEXT("CheckOutButtonText", "Check Out File"))
			.ToolTipText(LOCTEXT("CheckOutButtonTooltip", "Check out the default configuration file that holds these settings."))
			//@TODO: .Visibility(this, &SSettingsFileCheckoutNotice::HandleCheckOutButtonVisibility)
		];

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						// categories menu
						SNew(SScrollBox)

						+ SScrollBox::Slot()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SAssignNew(CategoriesBox, SVerticalBox)
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SSpacer)
								.Size(FVector2D(24.0f, 0.0f))
							]
						]
					]

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(24.0f, 0.0f, 24.0f, 0.0f))
					[
						SNew(SSeparator)
						.Orientation(Orient_Vertical)
					]

				+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SVerticalBox)
							.Visibility(this, &SSettingsEditor::HandleSettingsBoxVisibility)

						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 16.0f)
							[
								// title and button bar
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									[
										SNew(SVerticalBox)

										+ SVerticalBox::Slot()
											.AutoHeight()
											[
												// category title
												SNew(STextBlock)
													.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 18))
													.Text(this, &SSettingsEditor::HandleSettingsBoxTitleText)													
											]

										+ SVerticalBox::Slot()
											.AutoHeight()
											.Padding(0.0f, 8.0f, 0.0f, 0.0f)
											[
												// category description
												SNew(STextBlock)
													.ColorAndOpacity(FSlateColor::UseSubduedForeground())
													.Text(this, &SSettingsEditor::HandleSettingsBoxDescriptionText)
											]
									]

								+ SHorizontalBox::Slot()
									.AutoWidth()
									.HAlign(HAlign_Right)
									.VAlign(VAlign_Bottom)
									.Padding(16.0f, 0.0f, 0.0f, 0.0f)
									[
										// save as defaults button
										SNew(SButton)
											.IsEnabled(this, &SSettingsEditor::HandleSaveDefaultsButtonEnabled)
											.OnClicked(this, &SSettingsEditor::HandleSaveDefaultsButtonClicked)
											.Text(LOCTEXT("SaveDefaultsButtonText", "Set as Default"))
											.ToolTipText(LOCTEXT("SaveDefaultsButtonTooltip", "Save the values below as the new default settings"))
									]

								+ SHorizontalBox::Slot()
									.AutoWidth()
									.HAlign(HAlign_Right)
									.VAlign(VAlign_Bottom)
									.Padding(8.0f, 0.0f, 0.0f, 0.0f)
									[
										// export button
										SNew(SButton)
											.IsEnabled(this, &SSettingsEditor::HandleExportButtonEnabled)
											.OnClicked(this, &SSettingsEditor::HandleExportButtonClicked)
											.Text(LOCTEXT("ExportButtonText", "Export..."))
											.ToolTipText(LOCTEXT("ExportButtonTooltip", "Export these settings to a file on your computer"))
									]

								+ SHorizontalBox::Slot()
									.AutoWidth()
									.HAlign(HAlign_Right)
									.VAlign(VAlign_Bottom)
									.Padding(8.0f, 0.0f, 0.0f, 0.0f)
									[
										// import button
										SNew(SButton)
											.IsEnabled(this, &SSettingsEditor::HandleImportButtonEnabled)
											.OnClicked(this, &SSettingsEditor::HandleImportButtonClicked)
											.Text(LOCTEXT("ImportButtonText", "Import..."))
											.ToolTipText(LOCTEXT("ImportButtonTooltip", "Import these settings from a file on your computer"))
									]

								+ SHorizontalBox::Slot()
									.AutoWidth()
									.HAlign(HAlign_Right)
									.VAlign(VAlign_Bottom)
									.Padding(8.0f, 0.0f, 0.0f, 0.0f)
									[
										// reset defaults button
										SNew(SButton)
											.IsEnabled(this, &SSettingsEditor::HandleResetToDefaultsButtonEnabled)
											.OnClicked(this, &SSettingsEditor::HandleResetDefaultsButtonClicked)
											.Text(LOCTEXT("ResetDefaultsButtonText", "Reset to Defaults"))
											.ToolTipText(LOCTEXT("ResetDefaultsButtonTooltip", "Reset the settings below to their default values"))
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 0.0f, 0.0f, 16.0f)
							[
								ConfigNoticeWidget
							]

						+ SVerticalBox::Slot()
							.FillHeight(1.0f)
							[
								// settings area
								SNew(SOverlay)

								+ SOverlay::Slot()
									[
										SettingsView.ToSharedRef()
									]

								+ SOverlay::Slot()
									.Expose(CustomWidgetSlot)
							]
					]
			]
	];

	Model->OnSelectionChanged().AddSP(this, &SSettingsEditor::HandleModelSelectionChanged);
	SettingsContainer->OnCategoryModified().AddSP(this, &SSettingsEditor::HandleSettingsContainerCategoryModified);

	ReloadCategories();
}


/* SCompoundWidget interface
 *****************************************************************************/

void SSettingsEditor::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// cache selected settings object's configuration file state
	DefaultConfigCheckOutTimer += InDeltaTime;

	if (DefaultConfigCheckOutTimer >= 1.0f)
	{
		TWeakObjectPtr<UObject> SettingsObject = GetSelectedSettingsObject();

		if (SettingsObject.IsValid() && SettingsObject->GetClass()->HasAnyClassFlags(CLASS_Config | CLASS_DefaultConfig))
		{
			FString ConfigFileName = FString::Printf(TEXT("%sDefault%s.ini"), *FPaths::SourceConfigDir(), *SettingsObject->GetClass()->ClassConfigName.ToString());
			bool NewCheckOutNeeded = (FPaths::FileExists(ConfigFileName) && IFileManager::Get().IsReadOnly(*ConfigFileName));

			// file has been checked in or reverted
			if ((NewCheckOutNeeded == true) && (DefaultConfigCheckOutNeeded == false))
			{
				SettingsObject->ReloadConfig();
			}

			DefaultConfigCheckOutNeeded = NewCheckOutNeeded;
		}
		else
		{
			DefaultConfigCheckOutNeeded = false;
		}

		DefaultConfigCheckOutTimer = 0.0f;
	}
}


/* FNotifyHook interface
 *****************************************************************************/

void SSettingsEditor::NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged )
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		SelectedSection->Save();
	}
}


/* SSettingsEditor implementation
 *****************************************************************************/

bool SSettingsEditor::CheckOutDefaultConfigFile( )
{
	FText ErrorMessage;

	// check out configuration file
	TWeakObjectPtr<UObject> SettingsObject = GetSelectedSettingsObject();

	if (SettingsObject.IsValid())
	{
		FString RelativeConfigFilePath = FString::Printf(TEXT("%sDefault%s.ini"), *FPaths::SourceConfigDir(), *SettingsObject->GetClass()->ClassConfigName.ToString());
		FString AbsoluteConfigFilePath = FPaths::ConvertRelativePathToFull(RelativeConfigFilePath);

		TArray<FString> FilesToBeCheckedOut;
		FilesToBeCheckedOut.Add(AbsoluteConfigFilePath);

		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(AbsoluteConfigFilePath, EStateCacheUsage::ForceUpdate);

		if(SourceControlState.IsValid())
		{
			if (SourceControlState->IsDeleted())
			{
				ErrorMessage = LOCTEXT("ConfigFileMarkedForDeleteError", "Error: The configuration file is marked for deletion.");
			}
			else if (!SourceControlState->IsCurrent())
			{
				if (false)
				{
					if (SourceControlProvider.Execute(ISourceControlOperation::Create<FSync>(), FilesToBeCheckedOut))
					{
						SettingsObject->ReloadConfig();
					}
					else
					{
						ErrorMessage = LOCTEXT("FailedToSyncConfigFileError", "Error: Failed to sync the configuration file to head revision.");
					}
				}
			}
			else if (SourceControlState->CanCheckout() || SourceControlState->IsCheckedOutOther())
			{
				if (!SourceControlProvider.Execute(ISourceControlOperation::Create<FCheckOut>(), FilesToBeCheckedOut))
				{
					ErrorMessage = LOCTEXT("FailedToCheckOutConfigFileError", "Error: Failed to check out the configuration file.");
				}
			}
		}
	}

	// show errors, if any
	if (!ErrorMessage.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, ErrorMessage);

		return false;
	}

	return true;
}


TWeakObjectPtr<UObject> SSettingsEditor::GetSelectedSettingsObject( ) const
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		return SelectedSection->GetSettingsObject();
	}

	return nullptr;
}


TSharedRef<SWidget> SSettingsEditor::MakeCategoryWidget( const ISettingsCategoryRef& Category )
{
	const float VerticalSlotPadding = 10.0f;

	// create section widgets
	TSharedRef<SVerticalBox> SectionsBox = SNew(SVerticalBox);
	TArray<ISettingsSectionPtr> Sections;
	
	if (Category->GetSections(Sections) == 0)
	{
		return SNullWidget::NullWidget;
	}

	// list the sections
	for (TArray<ISettingsSectionPtr>::TConstIterator It(Sections); It; ++It)
	{
		const ISettingsSectionPtr& Section = *It;

		SectionsBox->AddSlot()
			.HAlign(HAlign_Left)
			.Padding(0.0f, VerticalSlotPadding, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0f, 0.0f, 4.0f, 0.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("TreeArrow_Collapsed_Hovered"))
							.Visibility(this, &SSettingsEditor::HandleSectionLinkImageVisibility, Section)
					]

				+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SHyperlink)
							.OnNavigate(this, &SSettingsEditor::HandleSectionLinkNavigate, Section)
							.Text(Section->GetDisplayName())
							.ToolTipText(Section->GetDescription())
					]
			];
	}

	const FSlateBrush* CategoryIcon = FEditorStyle::GetBrush(Category->GetIconName());

	// create category widget
	return SNew(SHorizontalBox)

	+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(8.0f, VerticalSlotPadding, 12.0f, VerticalSlotPadding)
		[
			// category icon
			SNew(SImage)
				.Image((CategoryIcon == FEditorStyle::GetDefaultBrush()) ? FEditorStyle::GetBrush("SettingsEditor.Category_Default") : CategoryIcon)
		]

	+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, VerticalSlotPadding, 0.0f, 0.0f)
				[
					// category title
					SNew(STextBlock)
						.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 18))
						.Text(Category->GetDisplayName())
				]

			+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					// sections list
					SectionsBox
				]
		];
}


void SSettingsEditor::ReloadCategories(  )
{
	CategoriesBox->ClearChildren();

	TArray<ISettingsCategoryPtr> Categories;
	SettingsContainer->GetCategories(Categories);

	for (TArray<ISettingsCategoryPtr>::TConstIterator It(Categories); It; ++It)
	{
		CategoriesBox->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				MakeCategoryWidget(It->ToSharedRef())
			];
	}
}


/* SSettingsEditor callbacks
 *****************************************************************************/

FReply SSettingsEditor::HandleCheckOutButtonClicked( )
{
	CheckOutDefaultConfigFile();

	return FReply::Handled();
}


EVisibility SSettingsEditor::HandleDefaultConfigNoticeVisibility( ) const
{
	TWeakObjectPtr<UObject> SettingsObject = GetSelectedSettingsObject();

	if (SettingsObject.IsValid() && SettingsObject->GetClass()->HasAnyClassFlags(CLASS_DefaultConfig))
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;	
}


FReply SSettingsEditor::HandleExportButtonClicked( )
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		if (LastExportDir.IsEmpty())
		{
			LastExportDir = FPaths::GetPath(GEditorUserSettingsIni);
		}

		FString DefaultFileName = FString::Printf(TEXT("%s Backup %s.ini"), *SelectedSection->GetDisplayName().ToString(), *FDateTime::Now().ToString(TEXT("%Y-%m-%d %H%M%S")));
		TArray<FString> OutFiles;

		if (FDesktopPlatformModule::Get()->SaveFileDialog(nullptr, LOCTEXT("ExportSettingsDialogTitle", "Export settings...").ToString(), LastExportDir, DefaultFileName, TEXT("Config files (*.ini)|*.ini"), EFileDialogFlags::None, OutFiles))
		{
			if (SelectedSection->Export(OutFiles[0]))
			{
				LastExportDir = FPaths::GetPath(OutFiles[0]);
			}			
		}
	}

	return FReply::Handled();
}


bool SSettingsEditor::HandleExportButtonEnabled( ) const
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		return SelectedSection->CanExport();
	}

	return false;
}


FReply SSettingsEditor::HandleImportButtonClicked( )
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		TArray<FString> OutFiles;

		if (FDesktopPlatformModule::Get()->OpenFileDialog(nullptr, LOCTEXT("ImportSettingsDialogTitle", "Import settings...").ToString(), FPaths::GetPath(GEditorUserSettingsIni), TEXT(""), TEXT("Config files (*.ini)|*.ini"), EFileDialogFlags::None, OutFiles))
		{
			SelectedSection->Import(OutFiles[0]);

			// Our section has changed but we will not receive a notify from notify hook 
			// as it was done here and not through user action in the editor.
			SelectedSection->Save();
		}
	}

	return FReply::Handled();
}


bool SSettingsEditor::HandleImportButtonEnabled( ) const
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		return (SelectedSection->CanEdit() && SelectedSection->CanImport() && !DefaultConfigCheckOutNeeded);
	}

	return false;
}


void SSettingsEditor::HandleModelSelectionChanged( )
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		TSharedPtr<SWidget> CustomWidget = SelectedSection->GetCustomWidget().Pin();

		// show settings widget
		if (CustomWidget.IsValid())
		{
			CustomWidgetSlot->Widget = CustomWidget.ToSharedRef();
		}
		else
		{
			CustomWidgetSlot->Widget = SNullWidget::NullWidget;
		}

		SettingsView->SetObject(SelectedSection->GetSettingsObject().Get());

		// focus settings widget
		TSharedPtr<SWidget> FocusWidget;

		if (CustomWidget.IsValid())
		{
			FocusWidget = CustomWidget;
		}
		else
		{
			FocusWidget = SettingsView;
		}

		FWidgetPath FocusWidgetPath;

		if (FSlateApplication::Get().GeneratePathToWidgetUnchecked(FocusWidget.ToSharedRef(), FocusWidgetPath))
		{
			FSlateApplication::Get().SetKeyboardFocus(FocusWidgetPath, EKeyboardFocusCause::SetDirectly);
		}
	}
	else
	{
		CustomWidgetSlot->Widget = SNullWidget::NullWidget;
		SettingsView->SetObject(nullptr);
	}

	DefaultConfigCheckOutTimer = 1.0f;
}


bool SSettingsEditor::IsDefaultConfigEditable( ) const
{
	return !DefaultConfigCheckOutNeeded;
}


FReply SSettingsEditor::HandleResetDefaultsButtonClicked( )
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		SelectedSection->ResetDefaults();
	}

	return FReply::Handled();
}


bool SSettingsEditor::HandleResetToDefaultsButtonEnabled( ) const
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		return (SelectedSection->CanEdit() && SelectedSection->CanResetDefaults());
	}

	return false;
}


FReply SSettingsEditor::HandleSaveDefaultsButtonClicked( )
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		if (DefaultConfigCheckOutNeeded)
		{
			if (ISourceControlModule::Get().IsEnabled())
			{
				if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("SaveAsDefaultNeedsCheckoutMessage", "The default configuration file for these settings is currently not writeable. Would you like to check it out from source control?")) != EAppReturnType::Yes)
				{
					return FReply::Handled();
				}

				CheckOutDefaultConfigFile();
			}
			else
			{
				FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SaveAsDefaultsIsReadOnlyMessage", "The default configuration file for these settings is currently not writeable, and source control is disabled."));
			}
		}

		SelectedSection->SaveDefaults();

		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SaveAsDefaultsSucceededMessage", "The default configuration file for these settings was updated successfully."));
	}

	return FReply::Handled();
}


bool SSettingsEditor::HandleSaveDefaultsButtonEnabled( ) const
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		return SelectedSection->CanSaveDefaults();
	}

	return false;
}


void SSettingsEditor::HandleSectionLinkNavigate( ISettingsSectionPtr Section )
{
	Model->SelectSection(Section);
}


EVisibility SSettingsEditor::HandleSectionLinkImageVisibility( ISettingsSectionPtr Section ) const
{
	if (Model->GetSelectedSection() == Section)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Hidden;
}


FText SSettingsEditor::HandleSettingsBoxDescriptionText( ) const
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		return SelectedSection->GetDescription();
	}

	return FText::GetEmpty();
}


FString SSettingsEditor::HandleSettingsBoxTitleText( ) const
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		return SelectedSection->GetCategory().Pin()->GetDisplayName().ToString() + TEXT(" - ") + SelectedSection->GetDisplayName().ToString();
	}

	return FString();
}


EVisibility SSettingsEditor::HandleSettingsBoxVisibility( ) const
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Hidden;
}


void SSettingsEditor::HandleSettingsContainerCategoryModified( const FName& CategoryName )
{
	ReloadCategories();
}


bool SSettingsEditor::HandleSettingsViewEnabled( ) const
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	return (SelectedSection.IsValid() && SelectedSection->CanEdit() && (!SelectedSection->HasDefaultSettingsObject() || !DefaultConfigCheckOutNeeded));
}


EVisibility SSettingsEditor::HandleSettingsViewVisibility( ) const
{
	ISettingsSectionPtr SelectedSection = Model->GetSelectedSection();

	if (SelectedSection.IsValid() && SelectedSection->GetSettingsObject().IsValid())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Hidden;
}


#undef LOCTEXT_NAMESPACE

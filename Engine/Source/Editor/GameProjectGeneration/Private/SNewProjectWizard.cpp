// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GameProjectGenerationPrivatePCH.h"
#include "MainFrame.h"
#include "DesktopPlatformModule.h"
#include "SourceCodeNavigation.h"
#include "SScrollBorder.h"

#define LOCTEXT_NAMESPACE "NewProjectWizard"

FName SNewProjectWizard::TemplatePageName = TEXT("Template");
FName SNewProjectWizard::NameAndLocationPageName = TEXT("NameAndLocation");

struct FTemplateItem
{
	FText Name;
	FText Description;
	FString ProjectFile;
	TSharedPtr<FSlateBrush> TemplateThumbnail;
	bool bGenerateCode;

	FTemplateItem(const FText& InName, const FText& InDescription, const TSharedPtr<FSlateBrush>& InTemplateThumbnail, bool InGenerateCode, const FString& InProjectFile)
		: Name(InName)
		, Description(InDescription)
		, ProjectFile(InProjectFile)
		, TemplateThumbnail(InTemplateThumbnail)
		, bGenerateCode(InGenerateCode)
	{}
};

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SNewProjectWizard::Construct( const FArguments& InArgs )
{
	LastValidityCheckTime = 0;
	ValidityCheckFrequency = 4;
	bLastGlobalValidityCheckSuccessful = true;
	bLastNameAndLocationValidityCheckSuccessful = true;
	bPreventPeriodicValidityChecksUntilNextChange = false;
	bItemsAreExpanded = false;
	bCopyStarterContent = GEditor->GetGameAgnosticSettings().bCopyStarterContentPreference;
	bStarterContentIsAvailable = GameProjectUtils::IsStarterContentAvailableForNewProjects();

	// If the items should be initially visible, show them now
	ExpanderRolloutCurve = FCurveSequence(0.0f, 0.1f, ECurveEaseFunction::QuadOut);
	if(bItemsAreExpanded)
	{
		ExpanderRolloutCurve.JumpToEnd();
	}

	// Find all template projects
	FindTemplateProjects();
	SetDefaultProjectLocation();

	// Width of all labels; increase this if adding a longer label to this page
	const float LabelWidth = 46;
	const float EditableTextHeight = 26;

	SAssignNew(TemplateListView, SListView< TSharedPtr<FTemplateItem> >)
	.ListItemsSource(&TemplateList)
	.SelectionMode(ESelectionMode::Single)
	.ClearSelectionOnClick(false)
	.OnGenerateRow(this, &SNewProjectWizard::HandleTemplateListViewGenerateRow)
	.OnMouseButtonDoubleClick(this, &SNewProjectWizard::HandleTemplateListViewDoubleClick)
	.OnSelectionChanged(this, &SNewProjectWizard::HandleTemplateListViewSelectionChanged);

	ChildSlot
	[
		SNew(SOverlay)

		// Wizard
		+SOverlay::Slot()
		[
			SAssignNew(MainWizard, SWizard)
			.ShowPageList(false)
			.ShowCancelButton(false)
			.CanFinish(this, &SNewProjectWizard::HandleCreateProjectWizardCanFinish)
			.FinishButtonText(LOCTEXT("FinishButtonText", "Create Project"))
			.FinishButtonToolTip(LOCTEXT("FinishButtonToolTip", "Creates your new project in the specified location with the specified template and name.") )
			.OnFinished(this, &SNewProjectWizard::HandleCreateProjectWizardFinished)
			.OnFirstPageBackClicked(InArgs._OnBackRequested)

			// Choose Template
			+SWizard::Page()
			.OnEnter(this, &SNewProjectWizard::OnPageVisited, TemplatePageName)
			[
				SNew(SVerticalBox)

				// Page description
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("ChooseTemplateDescription", "Choose a template for your project. If you choose a C++ template, you must have {0} installed to compile.\nBlueprint templates use only visual scripting, and do not require additional software."), FSourceCodeNavigation::GetSuggestedSourceCodeIDE()))
				]

				// Templates list
				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(0.0f, 10.0f)
				[
					SNew(SScrollBorder, TemplateListView.ToSharedRef())
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							TemplateListView.ToSharedRef()
						]
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(SCheckBox)
					.ToolTipText( LOCTEXT("CopyStarterContent_ToolTip", "Adds example content to your new project, including simple placeable meshes with basic materials and textures.  You can turn this off to start with a project that is only has the bare essentials for the selected project template."))
					.OnCheckStateChanged(this, &SNewProjectWizard::OnCopyStarterContentChanged)
					.IsChecked(this, &SNewProjectWizard::IsCopyStarterContentChecked)
					.Visibility(this, &SNewProjectWizard::GetCopyStarterContentCheckboxVisibility)
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CopyStarterContent", "Include starter content" ))
 					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					// Constant height, whether the label is visible or not
					SNew(SBox)
					.HeightOverride(20)
					[
						SNew(SBorder)
						.Visibility(this, &SNewProjectWizard::GetNameAndLocationErrorLabelVisibility)
						.BorderImage(FEditorStyle::GetBrush("GameProjectDialog.ErrorLabelBorder"))
						.Content()
						[
							SNew(STextBlock)
							.Text(this, &SNewProjectWizard::GetNameAndLocationErrorLabelText)
							.TextStyle(FEditorStyle::Get(), "GameProjectDialog.ErrorLabelFont")
						]
					]
				]

				// Properties
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("DetailsView.CategoryTop"))
					.BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f ))
					.Padding(FMargin(6.0f, 4.0f, 7.0f, 4.0f))
					[
						SNew(SVerticalBox)

						// Name
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)

							// Name label
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 4.0f, 6.0f, 4.0f)
							[
								SNew(SBox)
								.HAlign(HAlign_Right)
								.WidthOverride(LabelWidth)
								[
									SNew(STextBlock)
									.TextStyle(FEditorStyle::Get(), "GameProjectDialog.ProjectNamePathLabels")
									.Text(LOCTEXT("ProjectNameLabel", "Name"))
								]
							]

							// Name editable text
							+SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							.AutoWidth()
							.Padding(6.0f, 4.0f, 0.0f, 4.0f)
							[
								SNew(SBox)
								.HeightOverride(EditableTextHeight)
								.WidthOverride( 200.0f )	// We don't need a giant text field for the project name, so set a max width
								[
									SNew(SEditableTextBox)
									.Text(this, &SNewProjectWizard::GetCurrentProjectFileName)
									.OnTextChanged(this, &SNewProjectWizard::OnCurrentProjectFileNameChanged)
								]
							]

							+SHorizontalBox::Slot()	// Spacer

							// Expander button
							+SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(6.0f, 4.0f, 0.0f, 4.0f)
							[
								SNew(SButton)
								.OnClicked(this, &SNewProjectWizard::OnExpanderClicked)
								.ToolTipText(LOCTEXT("ExpanderButtonTooltip", "Click to toggle display of the full path and file creation preview"))
								[
									SNew(SBox)
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									[
										SNew(SImage)
										.Image(this, &SNewProjectWizard::GetExpanderIcon)
										.ColorAndOpacity(FLinearColor::Black)
									]
								]
							]
						]

						// Expander content
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.Padding(0.0f)
							.BorderImage(FStyleDefaults::GetNoBrush())
							.Visibility(this, &SNewProjectWizard::GetExpandedItemsVisibility)
							.DesiredSizeScale(this, &SNewProjectWizard::GetExpandedItemsScale)
							[
								SNew(SVerticalBox)

								// Path
								+SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SHorizontalBox)

									// Path label
									+SHorizontalBox::Slot()
									.AutoWidth()
									.Padding(0.0f, 4.0f, 6.0f, 4.0f)
									.VAlign(VAlign_Center)
									[
										SNew(SBox)
										.HAlign(HAlign_Right)
										.WidthOverride(LabelWidth)
										[
											SNew(STextBlock)
											.TextStyle(FEditorStyle::Get(), "GameProjectDialog.ProjectNamePathLabels")
											.Text(LOCTEXT("ProjectPathLabel", "Folder"))
										]
									]

									// Path editable text
									+SHorizontalBox::Slot()
									.Padding(6.0f, 4.0f, 0.0f, 4.0f)
									.VAlign(VAlign_Center)
									[
										SNew(SBox)
										.HeightOverride(EditableTextHeight)
										[
											SNew(SHorizontalBox)

											+SHorizontalBox::Slot()
											.FillWidth(1.0f)
											[
												SNew(SEditableTextBox)
												.Text(this, &SNewProjectWizard::GetCurrentProjectFilePath)
												.OnTextChanged(this, &SNewProjectWizard::OnCurrentProjectFilePathChanged)
											]

											+SHorizontalBox::Slot()
											.AutoWidth()
											.Padding(6.0f, 1.0f, 0.0f, 0.0f)
											[
												SNew(SButton)
												.VAlign(VAlign_Center)
												.OnClicked(this, &SNewProjectWizard::HandleBrowseButtonClicked)
												.Text(LOCTEXT( "BrowseButtonText", "Choose Folder"))
											]
										]
									]
								]

								+SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SHorizontalBox)

									+SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SVerticalBox)

										// Proj file preview
										+SVerticalBox::Slot()
										.AutoHeight()
										[
											SNew(SHorizontalBox)

											// Proj file preview label
											+SHorizontalBox::Slot()
											.AutoWidth()
											.VAlign(VAlign_Center)
											.Padding(0.0f, 4.0f, 6.0f, 4.0f)
											[
												SNew(SBox)
												.HAlign(HAlign_Right)
												.WidthOverride(LabelWidth)
												[
													SNew(STextBlock)
													.TextStyle(FEditorStyle::Get(), "GameProjectDialog.ProjectNamePathLabels")
													.Text(LOCTEXT("ProjFilePreviewLabel", "File"))
												]
											]

											// Proj file preview label
											+SHorizontalBox::Slot()
											.VAlign(VAlign_Center)
											.Padding(6.0f, 4.0f, 0.0f, 4.0f)
											[
												SNew(STextBlock)
												.Text(this, &SNewProjectWizard::GetProjectFilenameWithPathLabelText)
											]
										]

										// Layout explanation
										+SVerticalBox::Slot()
										.AutoHeight()
										.VAlign(VAlign_Center)
										.Padding(LabelWidth + 12.0f, 4.0f, 6.0f, 4.0f)
										[
											SNew(STextBlock)
											.Text(FText::Format(LOCTEXT("ProjectNameAndLocationDetails", "A folder containing a project file (.{0}) and a few other folders will be created for you at the specified path."), FText::FromString(IProjectManager::GetProjectFileExtension())))
										]
									]

									// Layout diagram
									+SHorizontalBox::Slot()
									.Padding(6.0f, 4.0f, 0.0f, 0.0f)
									[
										SNew(SBox)
										[
											ConstructDiagram()
										]
									]
								]
							]
						]
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f)
				[
					SNew(SBorder)
					.Padding(FMargin(0.0f, 3.0f, 0.0f, 0.0f))
					.BorderImage(FEditorStyle::GetBrush("DetailsView.CategoryBottom"))
					.BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f ))
				]
			]
		]

		// Global Error label
		+SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding( FMargin( 0.0f, 5.0f, 0.0f, 0.0f ) )
		[
			// Constant height, whether the label is visible or not
			SNew(SBox)
			.HeightOverride(20.0f)
			[
				SNew(SBorder)
				.Visibility(this, &SNewProjectWizard::GetGlobalErrorLabelVisibility)
				.BorderImage(FEditorStyle::GetBrush("GameProjectDialog.ErrorLabelBorder"))
				.Content()
				[
					SNew(SHorizontalBox)
										
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.FillWidth(1.0f)
					[
						SNew(STextBlock)
						.Text(this, &SNewProjectWizard::GetGlobalErrorLabelText)
						.TextStyle(FEditorStyle::Get(), TEXT("GameProjectDialog.ErrorLabelFont"))
					]

					// A link to a platform-specific IDE, only shown when a compiler is not available
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SHyperlink)
						.Text(FText::Format(LOCTEXT("IDEDownloadLinkText", "Download {0}"), FSourceCodeNavigation::GetSuggestedSourceCodeIDE()))
						.OnNavigate(this, &SNewProjectWizard::OnDownloadIDEClicked, FSourceCodeNavigation::GetSuggestedSourceCodeIDEDownloadURL())
						.Visibility(this, &SNewProjectWizard::GetGlobalErrorLabelIDELinkVisibility)
					]
									
					// A button to close the persistent global error text
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.ButtonStyle(FEditorStyle::Get(), "NoBorder")
						.ContentPadding(0.0f) 
						.OnClicked(this, &SNewProjectWizard::OnCloseGlobalErrorLabelClicked)
						.Visibility(this, &SNewProjectWizard::GetGlobalErrorLabelCloseButtonVisibility)
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("GameProjectDialog.ErrorLabelCloseButton"))
						]
					]
				]
			]
		]
	];

	// Initialize the current page name. Assuming the template page.
	CurrentPageName = TemplatePageName;

	// Select the first item
	if (TemplateList.Num() > 0)
	{
		TemplateListView->SetSelection(TemplateList[0], ESelectInfo::Direct);
	}

	UpdateProjectFileValidity();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SNewProjectWizard::OnCopyStarterContentChanged(ESlateCheckBoxState::Type InNewState)
{
	bCopyStarterContent = InNewState == ESlateCheckBoxState::Checked;
}
ESlateCheckBoxState::Type SNewProjectWizard::IsCopyStarterContentChecked() const
{
	return bCopyStarterContent ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

EVisibility SNewProjectWizard::GetCopyStarterContentCheckboxVisibility() const
{
	return bStarterContentIsAvailable ? EVisibility::Visible : EVisibility::Collapsed;
}

void SNewProjectWizard::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Every few seconds, the project file path is checked for validity in case the disk contents changed and the location is now valid or invalid.
	// After project creation, periodic checks are disabled to prevent a brief message indicating that the project you created already exists.
	// This feature is re-enabled if the user did not restart and began editing parameters again.
	if ( !bPreventPeriodicValidityChecksUntilNextChange && (InCurrentTime > LastValidityCheckTime + ValidityCheckFrequency) )
	{
		UpdateProjectFileValidity();
	}
}


TSharedRef<SWidget> SNewProjectWizard::ConstructDiagram()
{
	const FMargin FolderPadding(0.0f, 0.0f, 2.0f, 0.0f);

	return SNew(SVerticalBox)
	.ToolTipText(LOCTEXT("FileDiagramTooltip", "List of files and folders that will be created"))
	// Parent folder
	+SVerticalBox::Slot()
	.AutoHeight()
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.Padding(FolderPadding)
		.AutoWidth()
		[
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("GameProjectDialog.FolderIconOpen"))
		]

		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(this, &SNewProjectWizard::GetCurrentProjectFileParentFolder)
		]
	]

	// Project folder
	+SVerticalBox::Slot()
	.Padding(10.0f, 0.0f)
	.AutoHeight()
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.Padding(FolderPadding)
		.AutoWidth()
		[
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("GameProjectDialog.FolderIconOpen"))
		]

		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(this, &SNewProjectWizard::GetCurrentProjectFileNameString)
		]
	]

	// Config folder
	+SVerticalBox::Slot()
	.Padding(20.0f, 0.0f)
	.AutoHeight()
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.Padding(FolderPadding)
		.AutoWidth()
		[
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("GameProjectDialog.FolderIconClosed"))
		]

		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Config"))) // Not localized on purpose. This folder does not change with different languages!
		]
	]

	// Content folder
	+SVerticalBox::Slot()
	.Padding(20.0f, 0.0f)
	.AutoHeight()
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.Padding(FolderPadding)
		.AutoWidth()
		[
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("GameProjectDialog.FolderIconClosed"))
		]

		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Content"))) // Not localized on purpose. This folder does not change with different languages!
		]
	]

	// Project file
	+SVerticalBox::Slot()
	.Padding(20.0f, 0.0f)
	.AutoHeight()
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.Padding(FolderPadding)
		.AutoWidth()
		[
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("GameProjectDialog.ProjectFileIcon"))
		]

		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(this, &SNewProjectWizard::GetCurrentProjectFileNameStringWithExtension)
		]
	];
}


EVisibility SNewProjectWizard::GetExpandedItemsVisibility() const
{
	// The section is visible if its scale in Y is greater than 0
	const float Scale = GetExpandedItemsScale().Y;
	return (Scale > 0) ? EVisibility::Visible : EVisibility::Collapsed;
}


FVector2D SNewProjectWizard::GetExpandedItemsScale() const
{
	const float Scale = ExpanderRolloutCurve.GetLerp();
	return FVector2D(1.0f, Scale);
}


const FSlateBrush* SNewProjectWizard::GetExpanderIcon() const
{
	return (bItemsAreExpanded) ? FEditorStyle::GetBrush("DetailsView.PulldownArrow.Up") : FEditorStyle::GetBrush("DetailsView.PulldownArrow.Down");
}


FReply SNewProjectWizard::OnExpanderClicked()
{
	bItemsAreExpanded = !bItemsAreExpanded;

	if(bItemsAreExpanded)
	{
		ExpanderRolloutCurve = FCurveSequence(0.0f, 0.1f, ECurveEaseFunction::CubicOut);
		ExpanderRolloutCurve.Play();
	}
	else
	{
		ExpanderRolloutCurve = FCurveSequence(0.0f, 0.1f, ECurveEaseFunction::CubicIn);
		ExpanderRolloutCurve.PlayReverse();
	}

	return FReply::Handled();
}


const FSlateBrush* SNewProjectWizard::GetTemplateItemImage(TWeakPtr<FTemplateItem> TemplateItem) const
{
	if ( TemplateItem.IsValid() )
	{
		TSharedPtr<FTemplateItem> Item = TemplateItem.Pin();
		if ( Item->TemplateThumbnail.IsValid() )
		{
			return Item->TemplateThumbnail.Get();
		}
	}
	
	return FEditorStyle::GetBrush("GameProjectDialog.DefaultGameThumbnail");
}


void SNewProjectWizard::HandleTemplateListViewSelectionChanged(TSharedPtr<FTemplateItem> TemplateItem, ESelectInfo::Type SelectInfo)
{
	UpdateProjectFileValidity();
}


TSharedPtr<FTemplateItem> SNewProjectWizard::GetSelectedTemplateItem() const
{
	TArray< TSharedPtr<FTemplateItem> > SelectedItems = TemplateListView->GetSelectedItems();
	if ( SelectedItems.Num() > 0 )
	{
		return SelectedItems[0];
	}
	
	return NULL;
}


FText SNewProjectWizard::GetSelectedTemplateName() const
{
	TSharedPtr<FTemplateItem> SelectedItem = GetSelectedTemplateItem();
	if ( SelectedItem.IsValid() )
	{
		return SelectedItem->Name;
	}

	return FText::GetEmpty();
}


FText SNewProjectWizard::GetCurrentProjectFileName() const
{
	return FText::FromString( GetCurrentProjectFileNameString() );
}


FString SNewProjectWizard::GetCurrentProjectFileNameString() const
{
	return CurrentProjectFileName;
}


FString SNewProjectWizard::GetCurrentProjectFileNameStringWithExtension() const
{
	return CurrentProjectFileName + TEXT(".") + IProjectManager::GetProjectFileExtension();
}


void SNewProjectWizard::OnCurrentProjectFileNameChanged(const FText& InValue)
{
	CurrentProjectFileName = InValue.ToString();
	UpdateProjectFileValidity();
}


FText SNewProjectWizard::GetCurrentProjectFilePath() const
{
	return FText::FromString(CurrentProjectFilePath);
}


FString SNewProjectWizard::GetCurrentProjectFileParentFolder() const
{
	if ( CurrentProjectFilePath.EndsWith(TEXT("/")) || CurrentProjectFilePath.EndsWith("\\") )
	{
		return FPaths::GetCleanFilename( CurrentProjectFilePath.LeftChop(1) );
	}
	else
	{
		return FPaths::GetCleanFilename( CurrentProjectFilePath );
	}
}


void SNewProjectWizard::OnCurrentProjectFilePathChanged(const FText& InValue)
{
	CurrentProjectFilePath = InValue.ToString();
	UpdateProjectFileValidity();
}


FString SNewProjectWizard::GetProjectFilenameWithPathLabelText() const
{
	return GetProjectFilenameWithPath();
}


FString SNewProjectWizard::GetProjectFilenameWithPath() const
{
	if ( CurrentProjectFilePath.IsEmpty() )
	{
		// Don't even try to assemble the path or else it may be relative to the binaries folder!
		return TEXT("");
	}
	else
	{
		const FString ProjectName = CurrentProjectFileName;
		const FString ProjectPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*CurrentProjectFilePath);
		const FString Filename = ProjectName + TEXT(".") + IProjectManager::GetProjectFileExtension();
		const FString ProjectFilename = FPaths::Combine( *ProjectPath, *ProjectName, *Filename );

		return ProjectFilename;
	}
}


FReply SNewProjectWizard::HandleBrowseButtonClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if ( DesktopPlatform )
	{
		void* ParentWindowWindowHandle = NULL;

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		FString FolderName;
		const FString Title = LOCTEXT("NewProjectBrowseTitle", "Choose a project location").ToString();
		const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			ParentWindowWindowHandle,
			Title,
			LastBrowsePath,
			FolderName
			);

		if ( bFolderSelected )
		{
			if ( !FolderName.EndsWith(TEXT("/")) )
			{
				FolderName += TEXT("/");
			}

			LastBrowsePath = FolderName;
			CurrentProjectFilePath = FolderName;
		}
	}

	return FReply::Handled();
}


void SNewProjectWizard::OnDownloadIDEClicked(FString URL)
{
	FPlatformProcess::LaunchURL( *URL, NULL, NULL );
}


void SNewProjectWizard::HandleTemplateListViewDoubleClick( TSharedPtr<FTemplateItem> TemplateItem )
{
	// Advance to the name/location page
	const int32 NamePageIdx = 1;
	if ( MainWizard->CanShowPage(NamePageIdx) )
	{
		MainWizard->ShowPage(NamePageIdx);
	}
}


bool SNewProjectWizard::IsCreateProjectEnabled() const
{
	if ( CurrentPageName == NAME_None )//|| CurrentPageName == TemplatePageName )
	{
		return false;
	}

	return bLastGlobalValidityCheckSuccessful && bLastNameAndLocationValidityCheckSuccessful;
}


bool SNewProjectWizard::HandlePageCanShow( FName PageName ) const
{
	if (PageName == NameAndLocationPageName)
	{
		return bLastGlobalValidityCheckSuccessful;
	}

	return true;
}


void SNewProjectWizard::OnPageVisited(FName NewPageName)
{
	CurrentPageName = NewPageName;
}


EVisibility SNewProjectWizard::GetGlobalErrorLabelVisibility() const
{
	return GetGlobalErrorLabelText().IsEmpty() ? EVisibility::Hidden : EVisibility::Visible;
}


EVisibility SNewProjectWizard::GetGlobalErrorLabelCloseButtonVisibility() const
{
	return PersistentGlobalErrorLabelText.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}


EVisibility SNewProjectWizard::GetGlobalErrorLabelIDELinkVisibility() const
{
	return (IsCompilerRequired() && !FSourceCodeNavigation::IsCompilerAvailable()) ? EVisibility::Visible : EVisibility::Collapsed;
}


FText SNewProjectWizard::GetGlobalErrorLabelText() const
{
	if ( !PersistentGlobalErrorLabelText.IsEmpty() )
	{
		return PersistentGlobalErrorLabelText;
	}

	if ( !bLastGlobalValidityCheckSuccessful )
	{
		return LastGlobalValidityErrorText;
	}

	return FText::GetEmpty();
}


FReply SNewProjectWizard::OnCloseGlobalErrorLabelClicked()
{
	PersistentGlobalErrorLabelText = FText();

	return FReply::Handled();
}


EVisibility SNewProjectWizard::GetNameAndLocationErrorLabelVisibility() const
{
	return GetNameAndLocationErrorLabelText().IsEmpty() ? EVisibility::Hidden : EVisibility::Visible;
}


FText SNewProjectWizard::GetNameAndLocationErrorLabelText() const
{
	if ( !bLastNameAndLocationValidityCheckSuccessful )
	{
		return LastNameAndLocationValidityErrorText;
	}

	return FText::GetEmpty();
}

void SNewProjectWizard::FindTemplateProjects()
{
	// Add some default non-data driven templates
	TemplateList.Add( MakeShareable(new FTemplateItem(
		LOCTEXT("BlankProjectName", "Blank"),
		LOCTEXT("BlankProjectDescription", "A clean empty project with no code."),
		MakeShareable( new FSlateBrush( *FEditorStyle::GetBrush("GameProjectDialog.BlankProjectThumbnail") ) ),
		false,				// bGenerateCode
		TEXT("")			// No filename, this is a generation template
		)) );

	TemplateList.Add( MakeShareable(new FTemplateItem(
		LOCTEXT("BasicCodeProjectName", "Basic Code"),
		LOCTEXT("BasicCodeProjectDescription", "An empty project with some basic game framework code classes created."),
		MakeShareable( new FSlateBrush( *FEditorStyle::GetBrush("GameProjectDialog.BasicCodeThumbnail") ) ),
		true,				// bGenerateCode
		TEXT("")			// No filename, this is a generation template
		)) );

	// Now discover and all data driven templates
	TArray<FString> TemplateRootFolders;

	// @todo rocket make template folder locations extensible.
	TemplateRootFolders.Add( FPaths::RootDir() + TEXT("Templates") );

	// Form a list of all folders that could contain template projects
	TArray<FString> AllTemplateFolders;
	for ( auto TemplateRootFolderIt = TemplateRootFolders.CreateConstIterator(); TemplateRootFolderIt; ++TemplateRootFolderIt )
	{
		const FString Root = *TemplateRootFolderIt;
		const FString SearchString = Root / TEXT("*");
		TArray<FString> TemplateFolders;
		IFileManager::Get().FindFiles(TemplateFolders, *SearchString, /*Files=*/false, /*Directories=*/true);
		for ( auto TemplateFolderIt = TemplateFolders.CreateConstIterator(); TemplateFolderIt; ++TemplateFolderIt )
		{
			AllTemplateFolders.Add( Root / (*TemplateFolderIt) );
		}
	}

	// Add a template item for every discovered project
	for ( auto TemplateFolderIt = AllTemplateFolders.CreateConstIterator(); TemplateFolderIt; ++TemplateFolderIt )
	{
		const FString SearchString = (*TemplateFolderIt) / TEXT("*.") + IProjectManager::GetProjectFileExtension();
		TArray<FString> FoundProjectFiles;
		IFileManager::Get().FindFiles(FoundProjectFiles, *SearchString, /*Files=*/true, /*Directories=*/false);
		if ( FoundProjectFiles.Num() > 0 )
		{
			if ( ensure(FoundProjectFiles.Num() == 1) )
			{
				// Make sure a TemplateDefs ini file exists
				const FString Root = *TemplateFolderIt;
				UTemplateProjectDefs* TemplateDefs = GameProjectUtils::LoadTemplateDefs(Root);
				if ( TemplateDefs )
				{
					// Found a template. Add it to the template items list.
					const FString ProjectFilename = Root / FoundProjectFiles[0];
					FText TemplateName = TemplateDefs->GetDisplayNameText();
					FText TemplateDescription = TemplateDefs->GetLocalizedDescription();

					// If no template name was specified for the current culture, just use the project name
					if ( TemplateName.IsEmpty() )
					{
						TemplateName = FText::FromString(FPaths::GetBaseFilename(ProjectFilename));
					}

					// Only generate code if the template has a source folder
					const FString SourceFolder = (Root / TEXT("Source"));
					const bool bGenerateCode = IFileManager::Get().DirectoryExists(*SourceFolder);

					TSharedPtr<FSlateDynamicImageBrush> DynamicBrush;
					const FString ThumbnailPNGFile = FPaths::GetBaseFilename(ProjectFilename, false) + TEXT(".png");
					if ( FPlatformFileManager::Get().GetPlatformFile().FileExists(*ThumbnailPNGFile) )
					{
						const FName BrushName = FName(*ThumbnailPNGFile);
						DynamicBrush = MakeShareable( new FSlateDynamicImageBrush(BrushName , FVector2D(128,128) ) );
					}

					if (FPaths::GetCleanFilename(ProjectFilename) == GameProjectUtils::GetDefaultProjectTemplateFilename())
					{
						TemplateList.Insert(MakeShareable(new FTemplateItem(TemplateName, TemplateDescription, DynamicBrush, bGenerateCode, ProjectFilename)), 0);
					}
					else
					{
						TemplateList.Add(MakeShareable(new FTemplateItem(TemplateName, TemplateDescription, DynamicBrush, bGenerateCode, ProjectFilename)));
					}
				}
			}
			else
			{
				// More than one project file in this template? This is not legal, skip it.
				continue;
			}
		}
	}
}


void SNewProjectWizard::SetDefaultProjectLocation( )
{
	FString DefaultProjectFilePath;
	if ( GEditor->GetGameAgnosticSettings().CreatedProjectPaths.Num() <= 0 )
	{
		// No previously used path, decide a default path.
		DefaultProjectFilePath = GameProjectUtils::GetDefaultProjectCreationPath();
		IFileManager::Get().MakeDirectory(*DefaultProjectFilePath, true);
	}
	else
	{
		DefaultProjectFilePath = GEditor->GetGameAgnosticSettings().CreatedProjectPaths[0];
	}

	if ( !DefaultProjectFilePath.IsEmpty() && DefaultProjectFilePath.Right(1) == TEXT("/") )
	{
		DefaultProjectFilePath.LeftChop(1);
	}

	FPaths::NormalizeFilename(DefaultProjectFilePath);
	const FString GenericProjectName = LOCTEXT("DefaultProjectName", "MyProject").ToString();
	FString ProjectName = GenericProjectName;

	// Check to make sure the project file doesn't already exist
	FText FailReason;
	if ( !GameProjectUtils::IsValidProjectFileForCreation(DefaultProjectFilePath / ProjectName / ProjectName + TEXT(".") + IProjectManager::GetProjectFileExtension(), FailReason) )
	{
		// If it exists, find an appropriate numerical suffix
		const int MaxSuffix = 1000;
		int32 Suffix;
		for ( Suffix = 2; Suffix < MaxSuffix; ++Suffix )
		{
			ProjectName = GenericProjectName + FString::Printf(TEXT("%d"), Suffix);
			if ( GameProjectUtils::IsValidProjectFileForCreation(DefaultProjectFilePath / ProjectName / ProjectName + TEXT(".") + IProjectManager::GetProjectFileExtension(), FailReason) )
			{
				// Found a name that is not taken. Break out.
				break;
			}
		}

		if (Suffix >= MaxSuffix)
		{
			UE_LOG(LogGameProjectGeneration, Warning, TEXT("Failed to find a suffix for the default project name"));
			ProjectName = TEXT("");
		}
	}

	if ( !DefaultProjectFilePath.IsEmpty() )
	{
		CurrentProjectFileName = ProjectName;
		CurrentProjectFilePath = DefaultProjectFilePath;
		LastBrowsePath = CurrentProjectFilePath;
	}
}


void SNewProjectWizard::UpdateProjectFileValidity( )
{
	// Global validity
	{
		bLastGlobalValidityCheckSuccessful = true;

		TSharedPtr<FTemplateItem> SelectedTemplate = GetSelectedTemplateItem();
		if ( !SelectedTemplate.IsValid() )
		{
			bLastGlobalValidityCheckSuccessful = false;
			LastGlobalValidityErrorText = LOCTEXT("NoTemplateSelected", "No Template Selected");
		}
		else if ( IsCompilerRequired() )
		{
			if ( !FSourceCodeNavigation::IsCompilerAvailable() )
			{
				bLastGlobalValidityCheckSuccessful = false;
				LastGlobalValidityErrorText = FText::Format( LOCTEXT("NoCompilerFound", "No compiler was found. In order to use a C++ template, you must first install {0}."), FSourceCodeNavigation::GetSuggestedSourceCodeIDE() );
			}
			else if ( !FModuleManager::Get().IsUnrealBuildToolAvailable() )
			{
				bLastGlobalValidityCheckSuccessful = false;
				LastGlobalValidityErrorText = LOCTEXT("UBTNotFound", "Engine source code was not found. In order to use a C++ template, you must have engine source code in Engine/Source.");
			}
		}
	}

	// Name and Location Validity
	{
		if ( CurrentProjectFileName.Contains(TEXT("/")) || CurrentProjectFileName.Contains(TEXT("\\")) )
		{
			bLastNameAndLocationValidityCheckSuccessful = false;
			LastNameAndLocationValidityErrorText = LOCTEXT( "SlashOrBackslashInProjectName", "The project name may not contain a slash or backslash" );
		}
		else
		{
			FText FailReason;
			bLastNameAndLocationValidityCheckSuccessful = GameProjectUtils::IsValidProjectFileForCreation( GetProjectFilenameWithPath(), FailReason );
			if ( !bLastNameAndLocationValidityCheckSuccessful )
			{
				LastNameAndLocationValidityErrorText = FailReason;
			}
		}

		if ( !FPlatformMisc::IsValidAbsolutePathFormat(CurrentProjectFilePath) )
		{
			bLastNameAndLocationValidityCheckSuccessful = false;
			LastNameAndLocationValidityErrorText = LOCTEXT( "InvalidFolderPath", "The folder path is invalid" );
		}
		else
		{
			FText FailReason;
			bLastNameAndLocationValidityCheckSuccessful = GameProjectUtils::IsValidProjectFileForCreation( GetProjectFilenameWithPath(), FailReason );
			if ( !bLastNameAndLocationValidityCheckSuccessful )
			{
				LastNameAndLocationValidityErrorText = FailReason;
			}
		}
	}

	LastValidityCheckTime = FSlateApplication::Get().GetCurrentTime();

	// Since this function was invoked, periodic validity checks should be re-enabled if they were disabled.
	bPreventPeriodicValidityChecksUntilNextChange = false;
}


bool SNewProjectWizard::IsCompilerRequired( ) const
{
	TSharedPtr<FTemplateItem> SelectedTemplate = GetSelectedTemplateItem();
	return SelectedTemplate.IsValid() && SelectedTemplate->bGenerateCode;
}


bool SNewProjectWizard::CreateProject( const FString& ProjectFile )
{
	// Get the selected template
	TSharedPtr<FTemplateItem> SelectedTemplate = GetSelectedTemplateItem();

	if (!ensure(SelectedTemplate.IsValid()))
	{
		// A template must be selected.
		return false;
	}

	const bool bAddCode = SelectedTemplate->bGenerateCode;
	FText FailReason;

	if (!GameProjectUtils::CreateProject( ProjectFile, SelectedTemplate->ProjectFile, bAddCode, bCopyStarterContent, FailReason))
	{
		DisplayError(FailReason);
		return false;
	}

	// Successfully created the project. Update the last created location string.
	FString CreatedProjectPath = FPaths::GetPath(FPaths::GetPath(ProjectFile)); 

	// if the original path was the drives root (ie: C:/) the double path call strips the last /
	if (CreatedProjectPath.EndsWith(":"))
	{
		CreatedProjectPath.AppendChar('/');
	}

	GEditor->AccessGameAgnosticSettings().CreatedProjectPaths.Remove(CreatedProjectPath);
	GEditor->AccessGameAgnosticSettings().CreatedProjectPaths.Insert(CreatedProjectPath, 0);
	GEditor->AccessGameAgnosticSettings().bCopyStarterContentPreference = bCopyStarterContent;
	GEditor->AccessGameAgnosticSettings().PostEditChange();

	return true;
}


void SNewProjectWizard::CreateAndOpenProject( )
{
	if( IsCreateProjectEnabled() )
	{
		FString ProjectFile = GetProjectFilenameWithPath();
		if ( CreateProject(ProjectFile) )
		{
			// Prevent periodic validity checks. This is to prevent a brief error message about the project already existing while you are exiting.
			bPreventPeriodicValidityChecksUntilNextChange = true;

			/** Only prompt for project switching if we are already in a project */
			const bool bPromptForConfirmation = FApp::HasGameName();

			const bool bCodeAdded = GetSelectedTemplateItem()->bGenerateCode;
			if ( bCodeAdded )
			{
				// In non-rocket, the engine executable may need to be built in order to build the game binaries,
				// just open the code editing ide now instead of automatically building for them since it is not safe to do so.
				OpenCodeIDE( ProjectFile, bPromptForConfirmation );
			}
			else
			{
				// Successfully created a content only project. Now open it.
				OpenProject( ProjectFile, bPromptForConfirmation );
			}
		}
	}
}


bool SNewProjectWizard::OpenProject( const FString& ProjectFile, bool bPromptForConfirmation )
{
	if ( bPromptForConfirmation )
	{
		// Notify the user of the success, and ask to switch projects.
		FText SuccessMessage = FText::Format( LOCTEXT("NewProjectSuccessful", "Project '{0}' was successfully created. Would you like to open it now?"), FText::FromString(FPaths::GetBaseFilename(ProjectFile)) );
		if ( FMessageDialog::Open( EAppMsgType::YesNo, SuccessMessage ) == EAppReturnType::No )
		{
			// The user opted out of opening the new project. Just close the window.
			CloseWindowIfAppropriate();
			return false;
		}
	}

	FText FailReason;
	if ( GameProjectUtils::OpenProject( ProjectFile, FailReason ) )
	{
		// Successfully opened the project, the editor is closing.
		// Close this window in case something prevents the editor from closing (save dialog, quit confirmation, etc)
		CloseWindowIfAppropriate();
		return true;
	}

	DisplayError( FailReason );
	return false;
}


bool SNewProjectWizard::OpenCodeIDE( const FString& ProjectFile, bool bPromptForConfirmation )
{
	if ( bPromptForConfirmation )
	{
		// Notify the user of the success, and ask to switch projects.
		FText SuccessMessage = FText::Format( LOCTEXT("NewProjectSuccessfulIDE", "Project '{0}' was successfully created. Would you like to open it in {1}?"), FText::FromString(FPaths::GetBaseFilename(ProjectFile)), FSourceCodeNavigation::GetSuggestedSourceCodeIDE() );
		if ( FMessageDialog::Open( EAppMsgType::YesNo, SuccessMessage ) == EAppReturnType::No )
		{
			// The user opted out of opening the new project. Just close the window.
			CloseWindowIfAppropriate(true);
			return false;
		}
	}

	FText FailReason;

	if ( GameProjectUtils::OpenCodeIDE( ProjectFile, FailReason ) )
	{
		// Successfully opened code editing IDE, the editor is closing
		// Close this window in case something prevents the editor from closing (save dialog, quit confirmation, etc)
		CloseWindowIfAppropriate(true);
		return true;
	}

	DisplayError( FailReason );
	return false;
}


void SNewProjectWizard::CloseWindowIfAppropriate( bool ForceClose )
{
	if ( ForceClose || FApp::HasGameName() )
	{
		FWidgetPath WidgetPath;
		TSharedPtr<SWindow> ContainingWindow = FSlateApplication::Get().FindWidgetWindow( AsShared(), WidgetPath);

		if ( ContainingWindow.IsValid() )
		{
			ContainingWindow->RequestDestroyWindow();
		}
	}
}


void SNewProjectWizard::DisplayError( const FText& ErrorText )
{
	UE_LOG(LogGameProjectGeneration, Log, TEXT("%s"), *ErrorText.ToString());
	PersistentGlobalErrorLabelText = ErrorText;
}


/* SNewProjectWizard event handlers
 *****************************************************************************/

bool SNewProjectWizard::HandleCreateProjectWizardCanFinish( ) const
{
	return IsCreateProjectEnabled();
}


void SNewProjectWizard::HandleCreateProjectWizardFinished( )
{
	CreateAndOpenProject();
}


TSharedRef<ITableRow> SNewProjectWizard::HandleTemplateListViewGenerateRow( TSharedPtr<FTemplateItem> TemplateItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	if (!ensure(TemplateItem.IsValid()))
	{
		return SNew(STableRow<TSharedPtr<FTemplateItem>>, OwnerTable);
	}

	const FString CodeTemplateToolTip = FText::Format( LOCTEXT("CodeTemplateToolTip", "This template contains C++ source code files. To use this template you must have {0} installed."), FSourceCodeNavigation::GetSuggestedSourceCodeIDE() ).ToString();
	const int32 ThumbnailBorderPadding = 5;
	const int32 ThumbnailSize = 128;

	return SNew( STableRow<TSharedPtr<FTemplateItem>>, OwnerTable )
		.Style(FEditorStyle::Get(), "GameProjectDialog.TemplateListView.TableRow")
		[
			SNew(SBox)
				.HeightOverride(ThumbnailSize+ThumbnailBorderPadding+5)
				[
					SNew(SHorizontalBox)

					// Thumbnail
					+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SBox)
								.Padding(ThumbnailBorderPadding)
								.WidthOverride(ThumbnailSize+ThumbnailBorderPadding * 2)
								.HeightOverride(ThumbnailSize+ThumbnailBorderPadding * 2)
								[
									SNew(SImage)
										.Image(this, &SNewProjectWizard::GetTemplateItemImage, TWeakPtr<FTemplateItem>(TemplateItem))
								]
						]

					// Name and description
					+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(10.0f)
						.VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
								.Padding(0.0f, 0.0f, 0.0f, 8.0f)
								.AutoHeight()
								[
									SNew(SHorizontalBox)

									+ SHorizontalBox::Slot()
										.VAlign(VAlign_Center)
										[
											SNew(STextBlock)
												.TextStyle(FEditorStyle::Get(), "GameProjectDialog.TemplateItemTitle")
												.Text(TemplateItem->Name.ToString())
										]
						
									+ SHorizontalBox::Slot()
										.AutoWidth()
										.VAlign(VAlign_Center)
										[
											SNew(SImage)
												.Visibility(TemplateItem->bGenerateCode ? EVisibility::Visible : EVisibility::Collapsed)
												.ToolTipText(CodeTemplateToolTip)
												.Image(FEditorStyle::GetBrush("GameProjectDialog.CodeImage"))
										]
								]

								+ SVerticalBox::Slot()
									.FillHeight(1.0f)
									[
										SNew(STextBlock)
											.WrapTextAt(596)
											.Text(TemplateItem->Description.ToString())
									]
						]
				]
		];
}


#undef LOCTEXT_NAMESPACE

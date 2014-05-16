// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GameProjectGenerationPrivatePCH.h"
#include "MainFrame.h"
#include "DesktopPlatformModule.h"
#include "SVerbChoiceDialog.h"
#include "UProjectInfo.h"
#include "SourceCodeNavigation.h"

#define LOCTEXT_NAMESPACE "ProjectBrowser"


/**
 * Structure for project items.
 */
struct FProjectItem
{
	FText Name;
	FText Description;
	FString EngineIdentifier;
	bool bUpToDate;
	FString ProjectFile;
	TSharedPtr<FSlateBrush> ProjectThumbnail;
	bool bIsNewProjectItem;

	FProjectItem(const FText& InName, const FText& InDescription, const FString& InEngineIdentifier, bool InUpToDate, const TSharedPtr<FSlateBrush>& InProjectThumbnail, const FString& InProjectFile, bool InIsNewProjectItem)
		: Name(InName)
		, Description(InDescription)
		, EngineIdentifier(InEngineIdentifier)
		, bUpToDate(InUpToDate)
		, ProjectFile(InProjectFile)
		, ProjectThumbnail(InProjectThumbnail)
		, bIsNewProjectItem(InIsNewProjectItem)
	{ }

	/** Check if this project is up to date */
	bool IsUpToDate() const
	{
		return bUpToDate;
	}

	/** Gets the engine label for this project */
	FString GetEngineLabel() const
	{
		if(bUpToDate)
		{
			return FString();
		}
		else if(FDesktopPlatformModule::Get()->IsStockEngineRelease(EngineIdentifier))
		{
			return EngineIdentifier;
		}
		else
		{
			return FString(TEXT("?"));
		}
	}
};


/**
 * Structure for project categories.
 */
struct FProjectCategory
{
	FText CategoryName;
	TSharedPtr< STileView< TSharedPtr<FProjectItem> > > ProjectTileView;
	TArray< TSharedPtr<FProjectItem> > ProjectItemsSource;
	TArray< TSharedPtr<FProjectItem> > FilteredProjectItemsSource;
};


void ProjectItemToString(const TSharedPtr<FProjectItem> InItem, TArray<FString>& OutFilterStrings)
{
	OutFilterStrings.Add(InItem->Name.ToString());
}


/* SCompoundWidget interface
 *****************************************************************************/

SProjectBrowser::SProjectBrowser()
	: ProjectItemFilter( ProjectItemTextFilter::FItemToStringArray::CreateStatic( ProjectItemToString ))
	, NumFilteredProjects(0)
{

}


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SProjectBrowser::Construct( const FArguments& InArgs )
{
	NewProjectScreenRequestedDelegate = InArgs._OnNewProjectScreenRequested;
	bPreventSelectionChangeEvent = false;
	ThumbnailBorderPadding = 5;
	ThumbnailSize = 128.0f;
	bAllowProjectCreate = InArgs._AllowProjectCreate;

	// Prepare the categories box
	CategoriesBox = SNew(SVerticalBox);

	// Find all projects
	FindProjects();

	CategoriesBox->AddSlot()
	.HAlign(HAlign_Center)
	.Padding(FMargin(0.f, 25.f))
	[
		SNew(STextBlock)
		.Visibility(this, &SProjectBrowser::GetNoProjectsErrorVisibility)
		.Text(LOCTEXT("NoProjects", "You don't have any projects yet :("))
	];

	CategoriesBox->AddSlot()
	.HAlign(HAlign_Center)
	.Padding(FMargin(0.f, 25.f))
	[
		SNew(STextBlock)
		.Visibility(this, &SProjectBrowser::GetNoProjectsAfterFilterErrorVisibility)
		.Text(LOCTEXT("NoProjectsAfterFilter", "There are no projects that match the specified filter"))
	];

	ChildSlot
	[
		SNew(SVerticalBox)

		// Page description
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SelectProjectDescription", "Select an existing project from the list below."))
		]

		// Categories
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
			.Padding(4)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Top)
				.Padding(FMargin(5.f, 10.f))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					[
						SNew(SOverlay)

						+SOverlay::Slot()
						[
							SNew(SSearchBox)
							.HintText(LOCTEXT("FilterHint", "Filter Projects..."))
							.OnTextChanged(this, &SProjectBrowser::OnFilterTextChanged)
						]

						+SOverlay::Slot()
						[
							SNew(SBorder)
							.Visibility(this, &SProjectBrowser::GetFilterActiveOverlayVisibility)
							.BorderImage(FEditorStyle::Get().GetBrush("SearchBox.ActiveBorder"))
						]
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ButtonStyle(FCoreStyle::Get(), "NoBorder")
						.OnClicked(this, &SProjectBrowser::FindProjects)
						.ToolTipText(LOCTEXT("RefreshProjectList", "Refresh the project list"))
						[
							SNew(SImage).Image(FEditorStyle::Get().GetBrush("Icons.Refresh"))
						]
					]
				]

				+ SVerticalBox::Slot()
				[
					SNew(SScrollBox)

					+ SScrollBox::Slot()
					[
						CategoriesBox.ToSharedRef()
					]
				]
			]
		]

		+ SVerticalBox::Slot()
		.Padding( 0, 40, 0, 0 )	// Lots of vertical padding before the dialog buttons at the bottom
		.AutoHeight()
		[
			SNew( SHorizontalBox )

			// Auto-load project
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(4, 0)
			[
				SNew(SCheckBox)			
				.IsChecked(GEditor->GetGameAgnosticSettings().bLoadTheMostRecentlyLoadedProjectAtStartup)
				.OnCheckStateChanged(this, &SProjectBrowser::OnAutoloadLastProjectChanged)
				.Content()
				[
					SNew(STextBlock).Text(LOCTEXT("AutoloadOnStartupCheckbox", "Always load last project on startup"))
				]
			]

			// Browse Button
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("BrowseProjectButton", "Browse..."))
				.OnClicked(this, &SProjectBrowser::OnBrowseToProjectClicked)
				.ContentPadding( FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding") )
			]

			// Open Button
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("OpenProjectButton", "Open"))
				.OnClicked(this, &SProjectBrowser::HandleOpenProjectButtonClicked)
				.IsEnabled(this, &SProjectBrowser::HandleOpenProjectButtonIsEnabled)
				.ContentPadding( FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding") )
			]
		]
	];

	// Select the first item in the first category
	if (ProjectCategories.Num() > 0)
	{
		const TSharedRef<FProjectCategory> Category = ProjectCategories[0];
		if ( ensure(Category->ProjectItemsSource.Num() > 0) && ensure(Category->ProjectTileView.IsValid()))
		{
			Category->ProjectTileView->SetSelection(Category->ProjectItemsSource[0], ESelectInfo::Direct);
		}
	}

	bHasProjectFiles = false;
	for (auto CategoryIt = ProjectCategories.CreateConstIterator(); CategoryIt; ++CategoryIt)
	{
		const TSharedRef<FProjectCategory>& Category = *CategoryIt;

		if (Category->ProjectItemsSource.Num() > 0)
		{
			bHasProjectFiles = true;
			break;
		}
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SProjectBrowser::ConstructCategory( const TSharedRef<SVerticalBox>& InCategoriesBox, const TSharedRef<FProjectCategory>& Category ) const
{
	// Title
	InCategoriesBox->AddSlot()
	.Padding(FMargin(5.f, 0.f))
	.AutoHeight()
	[
		SNew(STextBlock)
		.Visibility(this, &SProjectBrowser::GetProjectCategoryVisibility, Category)
		.TextStyle(FEditorStyle::Get(), "GameProjectDialog.ProjectNamePathLabels")
		.Text(Category->CategoryName)
	];

	// Separator
	InCategoriesBox->AddSlot()
	.AutoHeight()
	.Padding(5.0f, 2.0f, 5.0f, 8.0f)
	[
		SNew(SSeparator)
		.Visibility(this, &SProjectBrowser::GetProjectCategoryVisibility, Category)
	];

	// Project tile view
	InCategoriesBox->AddSlot()
	.AutoHeight()
	.Padding(5.0f, 0.0f, 5.0f, 40.0f)
	[
		SAssignNew(Category->ProjectTileView, STileView<TSharedPtr<FProjectItem> >)
		.Visibility(this, &SProjectBrowser::GetProjectCategoryVisibility, Category)
		.ListItemsSource(&Category->FilteredProjectItemsSource)
		.SelectionMode(ESelectionMode::Single)
		.ClearSelectionOnClick(false)
		.OnGenerateTile(this, &SProjectBrowser::MakeProjectViewWidget)
		.OnContextMenuOpening(this, &SProjectBrowser::OnGetContextMenuContent)
		.OnMouseButtonDoubleClick(this, &SProjectBrowser::HandleProjectItemDoubleClick)
		.OnSelectionChanged(TSlateDelegates<TSharedPtr<FProjectItem>>::FOnSelectionChanged::CreateSP(this, &SProjectBrowser::HandleProjectViewSelectionChanged, Category->CategoryName))
		.ItemHeight(ThumbnailSize + ThumbnailBorderPadding + 32)
		.ItemWidth(ThumbnailSize + ThumbnailBorderPadding)
	];
}

bool SProjectBrowser::HasProjects() const
{
	return bHasProjectFiles;
}


/* SProjectBrowser implementation
 *****************************************************************************/

TSharedRef<ITableRow> SProjectBrowser::MakeProjectViewWidget(TSharedPtr<FProjectItem> ProjectItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	if ( !ensure(ProjectItem.IsValid()) )
	{
		return SNew( STableRow<TSharedPtr<FProjectItem>>, OwnerTable );
	}

	TSharedPtr<SWidget> Thumbnail;
	if ( ProjectItem->bIsNewProjectItem )
	{
		Thumbnail = SNew(SBox)
			.Padding(ThumbnailBorderPadding)
			[
				SNew(SBorder)
				.Padding(0)
				.BorderImage(FEditorStyle::GetBrush("MarqueeSelection"))
				[
					SNew(SBox)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.TextStyle( FEditorStyle::Get(), "GameProjectDialog.NewProjectTitle" )
						.Text( LOCTEXT("NewProjectThumbnailText", "NEW") )
					]
				]
			];
	}
	else
	{
		const FLinearColor Tint = ProjectItem->IsUpToDate() ? FLinearColor::White : FLinearColor::White.CopyWithNewOpacity(0.5);

		// Drop shadow border
		Thumbnail =	SNew(SBorder)
			.Padding(ThumbnailBorderPadding)
			.BorderImage( FEditorStyle::GetBrush("ContentBrowser.ThumbnailShadow") )
			.ColorAndOpacity(Tint)
			.BorderBackgroundColor(Tint)
			[
				SNew(SImage).Image(this, &SProjectBrowser::GetProjectItemImage, TWeakPtr<FProjectItem>(ProjectItem))
			];
	}

	TSharedRef<ITableRow> TableRow = SNew( STableRow<TSharedPtr<FProjectItem>>, OwnerTable )
	.Style(FEditorStyle::Get(), "GameProjectDialog.TemplateListView.TableRow")
	[
		SNew(SBox)
		.HeightOverride(ThumbnailSize+ThumbnailBorderPadding+5)
		[
			SNew(SVerticalBox)

			// Thumbnail
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.WidthOverride(ThumbnailSize + ThumbnailBorderPadding * 2)
				.HeightOverride(ThumbnailSize + ThumbnailBorderPadding * 2)
				[
					SNew(SOverlay)

					+ SOverlay::Slot()
					[
						Thumbnail.ToSharedRef()
					]

					// Show the out of date engine version for this project file
					+ SOverlay::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Bottom)
					.Padding(10)
					[
						SNew(STextBlock)
						.Text(ProjectItem->GetEngineLabel())
						.TextStyle(FEditorStyle::Get(), "ProjectBrowser.VersionOverlayText")
						.ColorAndOpacity(FLinearColor::White.CopyWithNewOpacity(0.5f))
						.Visibility(ProjectItem->IsUpToDate() ? EVisibility::Collapsed : EVisibility::Visible)
					]
				]
			]

			// Name
			+SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Top)
			[
				SNew(STextBlock)
				.HighlightText(this, &SProjectBrowser::GetItemHighlightText)
				.Text(ProjectItem->Name)
			]
		]
	];

	TableRow->AsWidget()->SetToolTip(MakeProjectToolTip(ProjectItem));

	return TableRow;
}


TSharedRef<SToolTip> SProjectBrowser::MakeProjectToolTip( TSharedPtr<FProjectItem> ProjectItem ) const
{
	// Create a box to hold every line of info in the body of the tooltip
	TSharedRef<SVerticalBox> InfoBox = SNew(SVerticalBox);

	if(!ProjectItem->Description.IsEmpty())
	{
		AddToToolTipInfoBox( InfoBox, LOCTEXT("ProjectTileTooltipDescription", "Description"), ProjectItem->Description );
	}

	{
		const FString ProjectPath = FPaths::GetPath(ProjectItem->ProjectFile);
		AddToToolTipInfoBox( InfoBox, LOCTEXT("ProjectTileTooltipPath", "Path"), FText::FromString(ProjectPath) );
	}

	if (!ProjectItem->IsUpToDate())
	{
		FText Description;
		if(FDesktopPlatformModule::Get()->IsStockEngineRelease(ProjectItem->EngineIdentifier))
		{
			Description = FText::FromString(ProjectItem->EngineIdentifier);
		}
		else
		{
			FString RootDir;
			if(FDesktopPlatformModule::Get()->GetEngineRootDirFromIdentifier(ProjectItem->EngineIdentifier, RootDir))
			{
				FString PlatformRootDir = RootDir;
				FPaths::MakePlatformFilename(PlatformRootDir);
				Description = FText::FromString(PlatformRootDir);
			}
			else
			{
				Description = LOCTEXT("UnknownEngineVersion", "Unknown engine version");
			}
		}
		AddToToolTipInfoBox(InfoBox, LOCTEXT("EngineVersion", "Engine"), Description);
	}

	TSharedRef<SToolTip> Tooltip = SNew(SToolTip)
	.TextMargin(1)
	.BorderImage( FEditorStyle::GetBrush("ProjectBrowser.TileViewTooltip.ToolTipBorder") )
	[
		SNew(SBorder)
		.Padding(6)
		.BorderImage( FEditorStyle::GetBrush("ProjectBrowser.TileViewTooltip.NonContentBorder") )
		[
			SNew(SVerticalBox)

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(SBorder)
				.Padding(6)
				.BorderImage( FEditorStyle::GetBrush("ProjectBrowser.TileViewTooltip.ContentBorder") )
				[
					SNew(SVerticalBox)

					+SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text( ProjectItem->Name )
						.Font( FEditorStyle::GetFontStyle("ProjectBrowser.TileViewTooltip.NameFont") )
					]
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.Padding(6)
				.BorderImage( FEditorStyle::GetBrush("ProjectBrowser.TileViewTooltip.ContentBorder") )
				[
					InfoBox
				]
			]
		]
	];

	return Tooltip;
}


void SProjectBrowser::AddToToolTipInfoBox(const TSharedRef<SVerticalBox>& InfoBox, const FText& Key, const FText& Value) const
{
	InfoBox->AddSlot()
	.AutoHeight()
	.Padding(0, 1)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0, 0, 4, 0)
		[
			SNew(STextBlock) .Text( FText::Format(LOCTEXT("ProjectBrowserTooltipFormat", "{0}:"), Key ) )
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock) .Text( Value )
			.ColorAndOpacity(FSlateColor::UseForeground())
		]
	];
}


TSharedPtr<SWidget> SProjectBrowser::OnGetContextMenuContent() const
{
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, nullptr);

	TSharedPtr<FProjectItem> SelectedProjectItem = GetSelectedProjectItem();
	const FText ProjectContextActionsText = (SelectedProjectItem.IsValid()) ? SelectedProjectItem->Name : LOCTEXT("ProjectActionsMenuHeading", "Project Actions");
	MenuBuilder.BeginSection("ProjectContextActions", ProjectContextActionsText);

	FFormatNamedArguments Args;
	Args.Add(TEXT("FileManagerName"), FPlatformMisc::GetFileManagerName());
	const FText ExploreToText = FText::Format(NSLOCTEXT("GenericPlatform", "ShowInFileManager", "Show In {FileManagerName}"), Args);

	MenuBuilder.AddMenuEntry(
		ExploreToText,
		LOCTEXT("FindInExplorerTooltip", "Finds this project on disk"),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &SProjectBrowser::ExecuteFindInExplorer ),
		FCanExecuteAction::CreateSP( this, &SProjectBrowser::CanExecuteFindInExplorer )
		)
		);

	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}


void SProjectBrowser::ExecuteFindInExplorer() const
{
	TSharedPtr<FProjectItem> SelectedProjectItem = GetSelectedProjectItem();
	check(SelectedProjectItem.IsValid());
	FPlatformProcess::ExploreFolder(*SelectedProjectItem->ProjectFile);
}


bool SProjectBrowser::CanExecuteFindInExplorer() const
{
	TSharedPtr<FProjectItem> SelectedProjectItem = GetSelectedProjectItem();
	return SelectedProjectItem.IsValid();
}


const FSlateBrush* SProjectBrowser::GetProjectItemImage(TWeakPtr<FProjectItem> ProjectItem) const
{
	if ( ProjectItem.IsValid() )
	{
		TSharedPtr<FProjectItem> Item = ProjectItem.Pin();
		if ( Item->ProjectThumbnail.IsValid() )
		{
			return Item->ProjectThumbnail.Get();
		}
	}
	
	return FEditorStyle::GetBrush("GameProjectDialog.DefaultGameThumbnail");
}


TSharedPtr<FProjectItem> SProjectBrowser::GetSelectedProjectItem() const
{
	for ( auto CategoryIt = ProjectCategories.CreateConstIterator(); CategoryIt; ++CategoryIt )
	{
		const TSharedRef<FProjectCategory>& Category = *CategoryIt;
		TArray< TSharedPtr<FProjectItem> > SelectedItems = Category->ProjectTileView->GetSelectedItems();
		if ( SelectedItems.Num() > 0 )
		{
			return SelectedItems[0];
		}
	}
	
	return NULL;
}


FText SProjectBrowser::GetSelectedProjectName() const
{
	TSharedPtr<FProjectItem> SelectedItem = GetSelectedProjectItem();
	if ( SelectedItem.IsValid() )
	{
		return SelectedItem->Name;
	}

	return FText::GetEmpty();
}

FReply SProjectBrowser::FindProjects()
{
	ProjectCategories.Empty();
	CategoriesBox->ClearChildren();

	const FText MyProjectsCategoryName = LOCTEXT("MyProjectsCategoryName", "My Projects");
	const FText SamplesCategoryName = LOCTEXT("SamplesCategoryName", "Samples");

	// Create a map of parent project folders to their category
	TMap<FString, FText> ParentProjectFoldersToCategory;

	// Add the default creation path, in case you've never created a project before or want to include this path anyway
	ParentProjectFoldersToCategory.Add(GameProjectUtils::GetDefaultProjectCreationPath(), MyProjectsCategoryName);

	// Add in every path that the user has ever created a project file. This is to catch new projects showing up in the user's project folders
	const TArray<FString> &CreatedProjectPaths = GEditor->GetGameAgnosticSettings().CreatedProjectPaths;
	for(int32 Idx = 0; Idx < CreatedProjectPaths.Num(); Idx++)
	{
		FString CreatedProjectPath = CreatedProjectPaths[Idx];
		FPaths::NormalizeDirectoryName(CreatedProjectPath);
		ParentProjectFoldersToCategory.Add(CreatedProjectPath, MyProjectsCategoryName);
	}

	// Create a map of project folders to their category
	TMap<FString, FText> ProjectFoldersToCategory;

	// Find all the subdirectories of all the parent folders
	for(TMap<FString, FText>::TConstIterator Iter(ParentProjectFoldersToCategory); Iter; ++Iter)
	{
		const FString &ParentProjectFolder = Iter.Key();

		TArray<FString> ProjectFolders;
		IFileManager::Get().FindFiles(ProjectFolders, *(ParentProjectFolder / TEXT("*")), false, true);

		for(int32 Idx = 0; Idx < ProjectFolders.Num(); Idx++)
		{
			ProjectFoldersToCategory.Add(ParentProjectFolder / ProjectFolders[Idx], Iter.Value());
		}
	}

	// Add all the launcher installed sample folders
	TArray<FString> LauncherSampleDirectories;
	FDesktopPlatformModule::Get()->EnumerateLauncherSampleInstallations(LauncherSampleDirectories);
	for(int32 Idx = 0; Idx < LauncherSampleDirectories.Num(); Idx++)
	{
		ProjectFoldersToCategory.Add(LauncherSampleDirectories[Idx], SamplesCategoryName);
	}

	// Create a map of all the project files to their category
	TMap<FString, FText> ProjectFilesToCategory;

	// Add all the folders that the user has opened recently
	const TArray<FString> &RecentlyOpenedProjectFiles = GEditor->GetGameAgnosticSettings().RecentlyOpenedProjectFiles;
	for (int32 Idx = 0; Idx < RecentlyOpenedProjectFiles.Num(); Idx++)
	{
		FString RecentlyOpenedProjectFile = RecentlyOpenedProjectFiles[Idx];
		FPaths::NormalizeFilename(RecentlyOpenedProjectFile);
		ProjectFilesToCategory.Add(RecentlyOpenedProjectFile, MyProjectsCategoryName);
	}

	// Scan the project folders for project files
	FString ProjectWildcard = FString::Printf(TEXT("*.%s"), *IProjectManager::GetProjectFileExtension());
	for(TMap<FString, FText>::TConstIterator Iter(ProjectFoldersToCategory); Iter; ++Iter)
	{
		const FString &ProjectFolder = Iter.Key();

		TArray<FString> ProjectFiles;
		IFileManager::Get().FindFiles(ProjectFiles, *(ProjectFolder / ProjectWildcard), true, false);

		for(int32 Idx = 0; Idx < ProjectFiles.Num(); Idx++)
		{
			ProjectFilesToCategory.Add(ProjectFolder / ProjectFiles[Idx], Iter.Value());
		}
	}

	// Add all the discovered non-foreign project files
	const TArray<FString> &NonForeignProjectFiles = FUProjectDictionary::GetDefault().GetProjectPaths();
	for(int32 Idx = 0; Idx < NonForeignProjectFiles.Num(); Idx++)
	{
		if(!NonForeignProjectFiles[Idx].Contains(TEXT("/Templates/")))
		{
			const FText &CategoryName = NonForeignProjectFiles[Idx].Contains(TEXT("/Samples/"))? SamplesCategoryName : MyProjectsCategoryName;
			ProjectFilesToCategory.Add(NonForeignProjectFiles[Idx], CategoryName);
		}
	}

	// Normalize all the filenames and make sure there are no duplicates
	TMap<FString, FText> AbsoluteProjectFilesToCategory;
	for(TMap<FString, FText>::TConstIterator Iter(ProjectFilesToCategory); Iter; ++Iter)
	{
		FString AbsoluteFile = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*Iter.Key());
		AbsoluteProjectFilesToCategory.Add(AbsoluteFile, Iter.Value());
	}

	// Add all the discovered projects to the list
	const FString EngineIdentifier = FDesktopPlatformModule::Get()->GetCurrentEngineIdentifier();
	for(TMap<FString, FText>::TConstIterator Iter(AbsoluteProjectFilesToCategory); Iter; ++Iter)
	{
		const FString& ProjectFilename = *Iter.Key();
		const FText& DetectedCategory = Iter.Value();
		if ( FPaths::FileExists(ProjectFilename) )
		{
			FProjectStatus ProjectStatus;
			if (IProjectManager::Get().QueryStatusForProject(ProjectFilename, ProjectStatus))
			{
				// @todo localized project name
				const FText ProjectName = FText::FromString(ProjectStatus.Name);
				const FText ProjectDescription = FText::FromString(ProjectStatus.Description);

				TSharedPtr<FSlateDynamicImageBrush> DynamicBrush;
				const FString ThumbnailPNGFile = FPaths::GetBaseFilename(ProjectFilename, false) + TEXT(".png");
				const FString AutoScreenShotPNGFile = FPaths::Combine(*FPaths::GetPath(ProjectFilename), TEXT("Saved"), TEXT("AutoScreenshot.png"));
				FString PNGFileToUse;
				if ( FPaths::FileExists(*ThumbnailPNGFile) )
				{
					PNGFileToUse = ThumbnailPNGFile;
				}
				else if ( FPaths::FileExists(*AutoScreenShotPNGFile) )
				{
					PNGFileToUse = AutoScreenShotPNGFile;
				}

				if ( !PNGFileToUse.IsEmpty() )
				{
					const FName BrushName = FName(*PNGFileToUse);
					DynamicBrush = MakeShareable( new FSlateDynamicImageBrush(BrushName, FVector2D(128,128) ) );
				}

				FText ProjectCategory;
				if ( ProjectStatus.bSignedSampleProject )
				{
					ProjectCategory = SamplesCategoryName;
				}
				else
				{
					ProjectCategory = DetectedCategory;
				}

				FString ProjectEngineIdentifier;
				bool bIsUpToDate = FDesktopPlatformModule::Get()->GetEngineIdentifierForProject(ProjectFilename, ProjectEngineIdentifier) && ProjectEngineIdentifier == EngineIdentifier;

				const bool bIsNewProjectItem = false;
				TSharedRef<FProjectItem> NewProjectItem = MakeShareable( new FProjectItem(ProjectName, ProjectDescription, ProjectEngineIdentifier, bIsUpToDate, DynamicBrush, ProjectFilename, bIsNewProjectItem ) );
				AddProjectToCategory(NewProjectItem, ProjectCategory);
			}
		}
	}

	// Make sure the category order is "My Projects", "Samples", then all remaining categories in alphabetical order
	TSharedPtr<FProjectCategory> MyProjectsCategory;
	TSharedPtr<FProjectCategory> SamplesCategory;

	for ( int32 CategoryIdx = ProjectCategories.Num() - 1; CategoryIdx >= 0; --CategoryIdx )
	{
		TSharedRef<FProjectCategory> Category = ProjectCategories[CategoryIdx];
		if ( Category->CategoryName.EqualTo(MyProjectsCategoryName) )
		{
			MyProjectsCategory = Category;
			ProjectCategories.RemoveAt(CategoryIdx);
		}
		else if ( Category->CategoryName.EqualTo(SamplesCategoryName) )
		{
			SamplesCategory = Category;
			ProjectCategories.RemoveAt(CategoryIdx);
		}
	}

	// Sort categories
	struct FCompareCategories
	{
		FORCEINLINE bool operator()( const TSharedRef<FProjectCategory>& A, const TSharedRef<FProjectCategory>& B ) const
		{
			return A->CategoryName.CompareToCaseIgnored(B->CategoryName) < 0;
		}
	};
	ProjectCategories.Sort( FCompareCategories() );

	// Now read the built-in categories (last added is first in the list)
	if ( SamplesCategory.IsValid() )
	{
		ProjectCategories.Insert(SamplesCategory.ToSharedRef(), 0);
	}
	if ( MyProjectsCategory.IsValid() )
	{
		ProjectCategories.Insert(MyProjectsCategory.ToSharedRef(), 0);
	}

	// Sort each individual category
	struct FCompareProjectItems
	{
		FORCEINLINE bool operator()( const TSharedPtr<FProjectItem>& A, const TSharedPtr<FProjectItem>& B ) const
		{
			return A->Name.CompareToCaseIgnored(B->Name) < 0;
		}
	};
	for ( int32 CategoryIdx = 0; CategoryIdx < ProjectCategories.Num(); ++CategoryIdx )
	{
		TSharedRef<FProjectCategory> Category = ProjectCategories[CategoryIdx];
		Category->ProjectItemsSource.Sort( FCompareProjectItems() );
	}

	PopulateFilteredProjectCategories();

	for (auto CategoryIt = ProjectCategories.CreateConstIterator(); CategoryIt; ++CategoryIt)
	{
		const TSharedRef<FProjectCategory>& Category = *CategoryIt;
		ConstructCategory(CategoriesBox.ToSharedRef(), Category);
	}

	return FReply::Handled();
}


void SProjectBrowser::AddProjectToCategory(const TSharedRef<FProjectItem>& ProjectItem, const FText& ProjectCategory)
{
	for ( auto CategoryIt = ProjectCategories.CreateConstIterator(); CategoryIt; ++CategoryIt )
	{
		const TSharedRef<FProjectCategory>& Category = *CategoryIt;
		if ( Category->CategoryName.EqualToCaseIgnored(ProjectCategory) )
		{
			Category->ProjectItemsSource.Add(ProjectItem);
			return;
		}
	}

	TSharedRef<FProjectCategory> NewCategory = MakeShareable( new FProjectCategory );
	NewCategory->CategoryName = ProjectCategory;
	NewCategory->ProjectItemsSource.Add(ProjectItem);
	ProjectCategories.Add(NewCategory);
}


void SProjectBrowser::PopulateFilteredProjectCategories()
{
	NumFilteredProjects = 0;
	for (auto& Category : ProjectCategories)
	{
		Category->FilteredProjectItemsSource.Empty();

		for (auto& ProjectItem : Category->ProjectItemsSource)
		{
			if (ProjectItemFilter.PassesFilter(ProjectItem))
			{
				Category->FilteredProjectItemsSource.Add(ProjectItem);
				++NumFilteredProjects;
			}
		}

		if (Category->ProjectTileView.IsValid())
		{
			Category->ProjectTileView->RequestListRefresh();
		}
	}
}

FReply SProjectBrowser::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	if (InKeyboardEvent.GetKey() == EKeys::F5)
	{
		return FindProjects();
	}

	return FReply::Unhandled();
}

bool SProjectBrowser::OpenProject( const FString& InProjectFile )
{
	FText FailReason;
	FString ProjectFile = InProjectFile;

	// Get the identifier for the project
	FString ProjectIdentifier;
	FDesktopPlatformModule::Get()->GetEngineIdentifierForProject(ProjectFile, ProjectIdentifier);
	
	// Get the identifier for the current engine
	FString CurrentIdentifier = FDesktopPlatformModule::Get()->GetCurrentEngineIdentifier();
	if(ProjectIdentifier != CurrentIdentifier)
	{
		// Get the current project status
		FProjectStatus ProjectStatus;
		if(!IProjectManager::Get().QueryStatusForProject(ProjectFile, ProjectStatus))
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CouldNotReadProjectStatus", "Unable to read project status."));
			return false;
		}

		// Button labels for the upgrade dialog
		TArray<FText> Buttons;
		int32 OpenCopyButton = Buttons.Add(LOCTEXT("ProjectConvert_OpenCopy", "Open a copy"));
		int32 OpenExistingButton = Buttons.Add(LOCTEXT("ProjectConvert_ConvertInPlace", "Convert in-place"));
		int32 SkipConversionButton = Buttons.Add(LOCTEXT("ProjectConvert_SkipConversion", "Skip conversion"));
		int32 CancelButton = Buttons.Add(LOCTEXT("ProjectConvert_Cancel", "Cancel"));

		// Prompt for upgrading. Different message for code and content projects, since the process is a bit trickier for code.
		int32 Selection;
		if(ProjectStatus.bCodeBasedProject)
		{
			Selection = SVerbChoiceDialog::ShowModal(LOCTEXT("ProjectConversionTitle", "Convert Project"), LOCTEXT("ConvertCodeProjectPrompt", "This project was made with a different version of the Unreal Engine. Converting to this version will rebuild your code projects.\n\nNew features and improvements sometimes cause API changes, which may require you to modify your code before it compiles. Content saved with newer versions of the editor will not open in older versions.\n\nWe recommend you open a copy of your project to avoid damaging the original."), Buttons);
		}
		else
		{
			Selection = SVerbChoiceDialog::ShowModal(LOCTEXT("ProjectConversionTitle", "Convert Project"), LOCTEXT("ConvertContentProjectPrompt", "This project was made with a different version of the Unreal Engine.\n\nOpening it with this version of the editor may prevent it opening with the original editor, and may lose data. We recommend you open a copy to avoid damaging the original."), Buttons);
		}

		// Handle the selection
		if(Selection == CancelButton)
		{
			return false;
		}
		if(Selection == OpenCopyButton)
		{
			FString NewProjectFile;
			if(!GameProjectUtils::DuplicateProjectForUpgrade(ProjectFile, NewProjectFile))
			{
				FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("ConvertProjectCopyFailed", "Couldn't copy project. Check you have sufficient hard drive space and write access to the project folder.") );
				return false;
			}
			ProjectFile = NewProjectFile;
		}
		if(Selection != SkipConversionButton)
		{
			// If it's a code-based project, generate project files and open visual studio after an upgrade
			if(ProjectStatus.bCodeBasedProject)
			{
				// Try to generate project files
				FStringOutputDevice OutputLog;
				OutputLog.SetAutoEmitLineTerminator(true);
				GLog->AddOutputDevice(&OutputLog);
				bool bHaveProjectFiles = FDesktopPlatformModule::Get()->GenerateProjectFiles(FPaths::RootDir(), ProjectFile, GWarn);
				GLog->RemoveOutputDevice(&OutputLog);

				// Display any errors
				if(!bHaveProjectFiles)
				{
					FFormatNamedArguments Args;
					Args.Add( TEXT("LogOutput"), FText::FromString(OutputLog) );
					FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("CouldNotGenerateProjectFiles", "Project files could not be generated. Log output:\n\n{LogOutput}"), Args));
					return false;
				}

				// Try to compile the project
				OutputLog.Empty();
				GLog->AddOutputDevice(&OutputLog);
				bool bCompileSucceeded = FDesktopPlatformModule::Get()->CompileGameProject(FPaths::RootDir(), ProjectFile, GWarn);
				GLog->RemoveOutputDevice(&OutputLog);

				// Try to compile the modules
				if(!bCompileSucceeded)
				{
					FText DevEnvName = FSourceCodeNavigation::GetSuggestedSourceCodeIDE( true );

					TArray<FText> CompileFailedButtons;
					int32 OpenIDEButton = CompileFailedButtons.Add(FText::Format(LOCTEXT("CompileFailedOpenIDE", "Open with {0}"), DevEnvName));
					int32 ViewLogButton = CompileFailedButtons.Add(LOCTEXT("CompileFailedViewLog", "View build log"));
					CompileFailedButtons.Add(LOCTEXT("CompileFailedCancel", "Cancel"));

					int32 CompileFailedChoice = SVerbChoiceDialog::ShowModal(LOCTEXT("ProjectUpgradeTitle", "Project Conversion Failed"), FText::Format(LOCTEXT("ProjectUpgradeCompileFailed", "The project failed to compile with this version of the engine. Would you like to open the project in {0}?"), DevEnvName), CompileFailedButtons);
					if(CompileFailedChoice == ViewLogButton)
					{
						CompileFailedButtons.RemoveAt(ViewLogButton);
						CompileFailedChoice = SVerbChoiceDialog::ShowModal(LOCTEXT("ProjectUpgradeTitle", "Project Conversion Failed"), FText::Format(LOCTEXT("ProjectUpgradeCompileFailed", "The project failed to compile with this version of the engine. Build output is as follows:\n\n{0}"), FText::FromString(OutputLog)), CompileFailedButtons);
					}

					if(CompileFailedChoice == OpenIDEButton && !GameProjectUtils::OpenCodeIDE(ProjectFile, FailReason))
					{
						FMessageDialog::Open(EAppMsgType::Ok, FailReason);
					}

					return false;
				}
			}

			// Update the game project to the latest version. This will prompt to check-out as necessary. We don't need to write the engine identifier directly, because it won't use the right .uprojectdirs logic.
			if(!GameProjectUtils::UpdateGameProject(TEXT("")) || !FDesktopPlatformModule::Get()->SetEngineIdentifierForProject(ProjectFile, CurrentIdentifier))
			{
				if(FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("ProjectUpgradeFailure", "Project file could not be updated to latest version. Attempt to open anyway?")) != EAppReturnType::Yes)
				{
					return false;
				}
			}
		}
	}

	// Open the project
	if (!GameProjectUtils::OpenProject(ProjectFile, FailReason))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FailReason);
		return false;
	}

	return true;
}


void SProjectBrowser::OpenSelectedProject( )
{
	if ( CurrentSelectedProjectPath.IsEmpty() )
	{
		return;
	}

	OpenProject( CurrentSelectedProjectPath.ToString() );
}

/* SProjectBrowser event handlers
 *****************************************************************************/

FReply SProjectBrowser::HandleOpenProjectButtonClicked( )
{
	OpenSelectedProject();

	return FReply::Handled();
}


bool SProjectBrowser::HandleOpenProjectButtonIsEnabled( ) const
{
	return !CurrentSelectedProjectPath.IsEmpty();
}


void SProjectBrowser::HandleProjectItemDoubleClick( TSharedPtr<FProjectItem> TemplateItem )
{
	OpenSelectedProject();
}

FReply SProjectBrowser::OnBrowseToProjectClicked()
{
	const FString ProjectFileDescription = LOCTEXT( "FileTypeDescription", "Unreal Project File" ).ToString();
	const FString ProjectFileExtension = FString::Printf(TEXT("*.%s"), *IProjectManager::GetProjectFileExtension());
	const FString FileTypes = FString::Printf( TEXT("%s (%s)|%s"), *ProjectFileDescription, *ProjectFileExtension, *ProjectFileExtension );

	// Find the first valid project file to select by default
	FString DefaultFolder = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::PROJECT);
	for ( auto ProjectIt = GEditor->GetGameAgnosticSettings().RecentlyOpenedProjectFiles.CreateConstIterator(); ProjectIt; ++ProjectIt )
	{
		if ( IFileManager::Get().FileSize(**ProjectIt) > 0 )
		{
			// This is the first uproject file in the recents list that actually exists
			DefaultFolder = FPaths::GetPath(*ProjectIt);
			break;
		}
	}

	// Prompt the user for the filenames
	TArray<FString> OpenFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bOpened = false;
	if ( DesktopPlatform )
	{
		void* ParentWindowWindowHandle = NULL;

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		bOpened = DesktopPlatform->OpenFileDialog(
			ParentWindowWindowHandle,
			LOCTEXT("OpenProjectBrowseTitle", "Open Project").ToString(),
			DefaultFolder,
			TEXT(""),
			FileTypes,
			EFileDialogFlags::None,
			OpenFilenames
		);
	}

	if ( bOpened && OpenFilenames.Num() > 0 )
	{
		HandleProjectViewSelectionChanged( NULL, ESelectInfo::Direct, FText() );

		FString Path = OpenFilenames[0];
		if ( FPaths::IsRelative( Path ) )
		{
			Path = FPaths::ConvertRelativePathToFull( Path );
		}

		CurrentSelectedProjectPath = FText::FromString( Path );

		OpenSelectedProject();
	}

	return FReply::Handled();
}


void SProjectBrowser::HandleProjectViewSelectionChanged(TSharedPtr<FProjectItem> ProjectItem, ESelectInfo::Type SelectInfo, FText CategoryName)
{
	if ( !bPreventSelectionChangeEvent )
	{
		TGuardValue<bool> SelectionEventGuard(bPreventSelectionChangeEvent, true);

		for ( auto CategoryIt = ProjectCategories.CreateConstIterator(); CategoryIt; ++CategoryIt )
		{
			const TSharedRef<FProjectCategory>& Category = *CategoryIt;
			if ( Category->ProjectTileView.IsValid() && !Category->CategoryName.EqualToCaseIgnored(CategoryName) )
			{
				Category->ProjectTileView->ClearSelection();
			}
		}

		const TSharedPtr<FProjectItem> SelectedItem = GetSelectedProjectItem();
		if ( SelectedItem.IsValid() && SelectedItem != CurrentlySelectedItem )
		{
			CurrentSelectedProjectPath = FText::FromString( SelectedItem->ProjectFile );
		}
	}
}

void SProjectBrowser::OnFilterTextChanged(const FText& InText)
{
	ProjectItemFilter.SetRawFilterText(InText);
	PopulateFilteredProjectCategories();
}

void SProjectBrowser::OnAutoloadLastProjectChanged(ESlateCheckBoxState::Type NewState)
{
	UEditorGameAgnosticSettings &Settings = GEditor->AccessGameAgnosticSettings();
	Settings.bLoadTheMostRecentlyLoadedProjectAtStartup = (NewState == ESlateCheckBoxState::Checked);

	UProperty* AutoloadProjectProperty = FindField<UProperty>(Settings.GetClass(), "bLoadTheMostRecentlyLoadedProjectAtStartup");
	if (AutoloadProjectProperty != NULL)
	{
		FPropertyChangedEvent PropertyUpdateStruct(AutoloadProjectProperty);
		Settings.PostEditChangeProperty(PropertyUpdateStruct);
	}
}

EVisibility SProjectBrowser::GetProjectCategoryVisibility(const TSharedRef<FProjectCategory> InCategory) const
{
	if (NumFilteredProjects == 0)
	{
		return EVisibility::Collapsed;
	}
	return InCategory->FilteredProjectItemsSource.Num() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SProjectBrowser::GetNoProjectsErrorVisibility() const
{
	return bHasProjectFiles ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SProjectBrowser::GetNoProjectsAfterFilterErrorVisibility() const
{
	return (bHasProjectFiles && NumFilteredProjects == 0) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SProjectBrowser::GetFilterActiveOverlayVisibility() const
{
	return ProjectItemFilter.GetRawFilterText().IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
}

FText SProjectBrowser:: GetItemHighlightText() const
{
	return ProjectItemFilter.GetRawFilterText();
}

#undef LOCTEXT_NAMESPACE

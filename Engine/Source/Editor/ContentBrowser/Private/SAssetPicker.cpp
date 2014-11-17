// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ContentBrowserPCH.h"

#define LOCTEXT_NAMESPACE "ContentBrowser"

void SAssetPicker::Construct( const FArguments& InArgs )
{
	TSharedPtr<AssetFilterCollectionType> FrontendFilters = MakeShareable(new AssetFilterCollectionType());

	BindCommands();

	OnAssetsActivated = InArgs._AssetPickerConfig.OnAssetsActivated;
	OnAssetSelected = InArgs._AssetPickerConfig.OnAssetSelected;
	OnAssetDoubleClicked = InArgs._AssetPickerConfig.OnAssetDoubleClicked;
	OnAssetEnterPressed = InArgs._AssetPickerConfig.OnAssetEnterPressed;
	bPendingFocusNextFrame = InArgs._AssetPickerConfig.bFocusSearchBoxWhenOpened;
	DefaultFilterMenuExpansion = InArgs._AssetPickerConfig.DefaultFilterMenuExpansion;

	for (auto DelegateIt = InArgs._AssetPickerConfig.GetCurrentSelectionDelegates.CreateConstIterator(); DelegateIt; ++DelegateIt)
	{
		if ((*DelegateIt) != NULL)
		{
			(**DelegateIt) = FGetCurrentSelectionDelegate::CreateSP(this, &SAssetPicker::GetCurrentSelection);
		}
	}

	TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);

	ChildSlot
	[
		VerticalBox
	];

	TAttribute< FText > HighlightText;
	EThumbnailLabel::Type ThumbnailLabel = InArgs._AssetPickerConfig.ThumbnailLabel;

	// Search box
	if (!InArgs._AssetPickerConfig.bAutohideSearchBar)
	{
		TextFilter = MakeShareable( new FFrontendFilter_Text() );
		FrontendFilters->Add( TextFilter );
		HighlightText = TAttribute< FText >( this, &SAssetPicker::GetHighlightedText );

		OtherDevelopersFilter = MakeShareable( new FFrontendFilter_ShowOtherDevelopers(nullptr) );
		FrontendFilters->Add( OtherDevelopersFilter );

		TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);

		if(InArgs._AssetPickerConfig.bAddFilterUI)
		{
			// Filter
			HorizontalBox->AddSlot()
			.AutoWidth()
			[
				SNew( STutorialWrapper, TEXT("ContentBrowserFiltersCombo") )
				[
					SNew( SComboButton )
					.ComboButtonStyle( FEditorStyle::Get(), "ContentBrowser.Filters.Style" )
					.ToolTipText( LOCTEXT( "AddFilterToolTip", "Add an asset filter." ) )
					.OnGetMenuContent( this, &SAssetPicker::MakeAddFilterMenu )
					.HasDownArrow( true )
					.ContentPadding( FMargin( 1, 0 ) )
					.ButtonContent()
					[
						SNew( STextBlock )
						.TextStyle( FEditorStyle::Get(), "ContentBrowser.Filters.Text" )
						.Text( LOCTEXT( "Filters", "Filters" ) )
					]
				]
			];
		}

		HorizontalBox->AddSlot()
		.FillWidth(1.0f)
		[
			SAssignNew( SearchBoxPtr, SAssetSearchBox )
			.HintText(NSLOCTEXT( "ContentBrowser", "SearchBoxHint", "Search Assets" ))
			.OnTextChanged( this, &SAssetPicker::OnSearchBoxChanged )
			.OnTextCommitted( this, &SAssetPicker::OnSearchBoxCommitted )
			.DelayChangeNotificationsWhileTyping( true )
		];

		HorizontalBox->AddSlot()
		.AutoWidth()
		[
			SNew( SCheckBox )
			.Style( FEditorStyle::Get(), "ToggleButtonCheckbox" )
			.ToolTipText( this, &SAssetPicker::GetShowOtherDevelopersToolTip )
			.OnCheckStateChanged( this, &SAssetPicker::HandleShowOtherDevelopersCheckStateChanged )
			.IsChecked( this, &SAssetPicker::GetShowOtherDevelopersCheckState )
			[
				SNew( SImage )
				.Image( FEditorStyle::GetBrush("ContentBrowser.ColumnViewDeveloperFolderIcon") )
			]
		];

		VerticalBox->AddSlot()
		.AutoHeight()
		.Padding( 0, 0, 0, 1 )
		[
			HorizontalBox
		];
	}

	// "None" button
	if (InArgs._AssetPickerConfig.bAllowNullSelection)
	{
		VerticalBox->AddSlot()
		.AutoHeight()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SButton)
						.ButtonStyle( FEditorStyle::Get(), "ContentBrowser.NoneButton" )
						.TextStyle( FEditorStyle::Get(), "ContentBrowser.NoneButtonText" )
						.Text( LOCTEXT("NoneButtonText", "( None )") )
						.ToolTipText( LOCTEXT("NoneButtonTooltip", "Clears the asset selection.") )
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked(this, &SAssetPicker::OnNoneButtonClicked)
				]

			// Trailing separator
			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 4)
				[
					SNew(SSeparator)
						.Orientation(Orient_Horizontal)
				]
		];
	}

	// Asset view
	
	// Break up the incoming filter into a sources data and backend filter.
	FSourcesData InitialSourcesData;
	InitialSourcesData.PackagePaths = InArgs._AssetPickerConfig.Filter.PackagePaths;
	InitialSourcesData.Collections = InArgs._AssetPickerConfig.Collections;
	InitialBackendFilter = InArgs._AssetPickerConfig.Filter;
	InitialBackendFilter.PackagePaths.Empty();

	if(InArgs._AssetPickerConfig.bAddFilterUI)
	{
		// Filters
		TArray<UClass*> FilterClassList;
		for(auto Iter = InitialBackendFilter.ClassNames.CreateIterator(); Iter; ++Iter)
		{
			FName ClassName = (*Iter);
			UClass* FilterClass = FindObject<UClass>(ANY_PACKAGE, *ClassName.ToString());
			if(FilterClass)
			{
				FilterClassList.AddUnique(FilterClass);
			}
		}

		VerticalBox->AddSlot()
		.AutoHeight()
		[
			SAssignNew(FilterListPtr, SFilterList)
			.OnFilterChanged(this, &SAssetPicker::OnFilterChanged)
			.FrontendFilters(FrontendFilters)
			.InitialClassFilters(FilterClassList)
			.ExtraFrontendFilters(InArgs._AssetPickerConfig.ExtraFrontendFilters)
		];
	}

	VerticalBox->AddSlot()
	.FillHeight(1.f)
	[
		SAssignNew(AssetViewPtr, SAssetView)
		.SelectionMode( InArgs._AssetPickerConfig.SelectionMode )
		.OnShouldFilterAsset(InArgs._AssetPickerConfig.OnShouldFilterAsset)
		.OnAssetSelected(InArgs._AssetPickerConfig.OnAssetSelected)
		.OnAssetsActivated(this, &SAssetPicker::HandleAssetsActivated)
		.OnGetAssetContextMenu(InArgs._AssetPickerConfig.OnGetAssetContextMenu)
		.AreRealTimeThumbnailsAllowed(this, &SAssetPicker::IsHovered)
		.FrontendFilters(FrontendFilters)
		.InitialSourcesData(InitialSourcesData)
		.InitialBackendFilter(InitialBackendFilter)
		.InitialViewType(InArgs._AssetPickerConfig.InitialAssetViewType)
		.InitialAssetSelection(InArgs._AssetPickerConfig.InitialAssetSelection)
		.ThumbnailScale(InArgs._AssetPickerConfig.ThumbnailScale)
		.ShowBottomToolbar(InArgs._AssetPickerConfig.bShowBottomToolbar)
		.OnAssetTagWantsToBeDisplayed(InArgs._AssetPickerConfig.OnAssetTagWantsToBeDisplayed)
		.AllowDragging( InArgs._AssetPickerConfig.bAllowDragging )
		.CanShowClasses( InArgs._AssetPickerConfig.bCanShowClasses )
		.CanShowFolders( false )
		.CanShowOnlyAssetsInSelectedFolders( InArgs._AssetPickerConfig.bCanShowOnlyAssetsInSelectedFolders )
		.CanShowRealTimeThumbnails( InArgs._AssetPickerConfig.bCanShowRealTimeThumbnails )
		.CanShowDevelopersFolder( InArgs._AssetPickerConfig.bCanShowDevelopersFolder )
		.PreloadAssetsForContextMenu( InArgs._AssetPickerConfig.bPreloadAssetsForContextMenu )
		.HighlightedText( HighlightText )
		.ThumbnailLabel( ThumbnailLabel )
		.AssetShowWarningText( InArgs._AssetPickerConfig.AssetShowWarningText)
		.AllowFocusOnSync(false)	// Stop the asset view from stealing focus (we're in control of that)
	];

	AssetViewPtr->RequestSlowFullListRefresh();
}

void SAssetPicker::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if ( bPendingFocusNextFrame && SearchBoxPtr.IsValid() )
	{
		FWidgetPath WidgetToFocusPath;
		FSlateApplication::Get().GeneratePathToWidgetUnchecked( SearchBoxPtr.ToSharedRef(), WidgetToFocusPath );
		FSlateApplication::Get().SetKeyboardFocus( WidgetToFocusPath, EKeyboardFocusCause::SetDirectly );
		bPendingFocusNextFrame = false;
	}

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

FReply SAssetPicker::OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	// Up and down move thru the filtered list
	int32 SelectionDelta = 0;

	if (InKeyboardEvent.GetKey() == EKeys::Up)
	{
		SelectionDelta = -1;
	}
	else if (InKeyboardEvent.GetKey() == EKeys::Down)
	{
		SelectionDelta = +1;
	}
	else if (InKeyboardEvent.GetKey() == EKeys::Enter)
	{
		TArray<FAssetData> SelectionSet = AssetViewPtr->GetSelectedAssets();
		HandleAssetsActivated(SelectionSet, EAssetTypeActivationMethod::Opened);

		return FReply::Handled();
	}

	if (SelectionDelta != 0)
	{
		AssetViewPtr->AdjustActiveSelection(SelectionDelta);

		return FReply::Handled();
	}

	if (Commands->ProcessCommandBindings(InKeyboardEvent))
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FText SAssetPicker::GetHighlightedText() const
{
	return TextFilter->GetRawFilterText();
}

void SAssetPicker::OnSearchBoxChanged(const FText& InSearchText)
{
	TextFilter->SetRawFilterText( InSearchText );
}

void SAssetPicker::OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo)
{
	TextFilter->SetRawFilterText( InSearchText );

	if (CommitInfo == ETextCommit::OnEnter)
	{
		TArray<FAssetData> SelectionSet = AssetViewPtr->GetSelectedAssets();
		if ( SelectionSet.Num() == 0 )
		{
			AssetViewPtr->AdjustActiveSelection(1);
			SelectionSet = AssetViewPtr->GetSelectedAssets();
		}
		HandleAssetsActivated(SelectionSet, EAssetTypeActivationMethod::Opened);
	}
}

TSharedRef<SWidget> SAssetPicker::MakeAddFilterMenu()
{
	return FilterListPtr->ExternalMakeAddFilterMenu(DefaultFilterMenuExpansion);
}

void SAssetPicker::OnFilterChanged()
{
	FARFilter Filter = FilterListPtr->GetCombinedBackendFilter();
	Filter.Append(InitialBackendFilter);
	AssetViewPtr->SetBackendFilter( Filter );
}

FReply SAssetPicker::OnNoneButtonClicked()
{
	OnAssetSelected.ExecuteIfBound(FAssetData());
	return FReply::Handled();
}

void SAssetPicker::HandleAssetsActivated(const TArray<FAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationMethod)
{
	if (ActivationMethod == EAssetTypeActivationMethod::DoubleClicked)
	{
		if (ActivatedAssets.Num() == 1)
		{
			OnAssetDoubleClicked.ExecuteIfBound(ActivatedAssets[0]);
		}
	}
	else if (ActivationMethod == EAssetTypeActivationMethod::Opened)
	{
		OnAssetEnterPressed.ExecuteIfBound(ActivatedAssets);
	}

	OnAssetsActivated.ExecuteIfBound( ActivatedAssets, ActivationMethod );
}

TArray< FAssetData > SAssetPicker::GetCurrentSelection()
{
	return AssetViewPtr->GetSelectedAssets();
}

FText SAssetPicker::GetShowOtherDevelopersToolTip() const
{
	if (OtherDevelopersFilter->GetShowOtherDeveloperAssets())
	{
		return LOCTEXT( "ShowOtherDevelopersFilterTooltipText", "Show Other Developers Assets");
	}
	else
	{
		return LOCTEXT( "HideOtherDevelopersFilterTooltipText", "Hide Other Developers Assets");
	}
}

void SAssetPicker::HandleShowOtherDevelopersCheckStateChanged( ESlateCheckBoxState::Type InCheckboxState )
{
	OtherDevelopersFilter->SetShowOtherDeveloperAssets( InCheckboxState == ESlateCheckBoxState::Checked );
}

ESlateCheckBoxState::Type SAssetPicker::GetShowOtherDevelopersCheckState() const
{
	return OtherDevelopersFilter->GetShowOtherDeveloperAssets() ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SAssetPicker::OnRenameRequested() const
{
	TArray< FAssetData > AssetViewSelectedAssets = AssetViewPtr->GetSelectedAssets();
	TArray< FString > SelectedFolders = AssetViewPtr->GetSelectedFolders();

	if ( AssetViewSelectedAssets.Num() == 1 && SelectedFolders.Num() == 0 )
	{
		// Don't operate on Redirectors
		if ( AssetViewSelectedAssets[0].AssetClass != UObjectRedirector::StaticClass()->GetFName() )
		{
			AssetViewPtr->RenameAsset(AssetViewSelectedAssets[0]);
		}
	}
	else if ( AssetViewSelectedAssets.Num() == 0 && SelectedFolders.Num() == 1 )
	{
		AssetViewPtr->RenameFolder(SelectedFolders[0]);
	}
}

bool SAssetPicker::CanExecuteRenameRequested()
{
	TArray< FAssetData > AssetViewSelectedAssets = AssetViewPtr->GetSelectedAssets();
	TArray< FString > SelectedFolders = AssetViewPtr->GetSelectedFolders();

	const bool bCanRenameFolder = ( AssetViewSelectedAssets.Num() == 0 && SelectedFolders.Num() == 1 );
	const bool bCanRenameAsset = ( AssetViewSelectedAssets.Num() == 1 && SelectedFolders.Num() == 0 ) && ( AssetViewSelectedAssets[0].AssetClass != UObjectRedirector::StaticClass()->GetFName() ); 

	return	bCanRenameFolder || bCanRenameAsset;
}

void SAssetPicker::BindCommands()
{
	Commands = MakeShareable(new FUICommandList);
	// bind commands
	Commands->MapAction( FGenericCommands::Get().Rename, FUIAction(
		FExecuteAction::CreateSP( this, &SAssetPicker::OnRenameRequested ),
		FCanExecuteAction::CreateSP( this, &SAssetPicker::CanExecuteRenameRequested )
		));
}

#undef LOCTEXT_NAMESPACE
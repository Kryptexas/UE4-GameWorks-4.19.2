// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ContentBrowserPCH.h"

#define LOCTEXT_NAMESPACE "ContentBrowser"

void AssetToString( AssetFilterType Asset, OUT TArray< FString >& Array )
{
	Array.Add( Asset.GetExportTextName() );
}

void SAssetPicker::Construct( const FArguments& InArgs )
{
	FilterCollection = MakeShareable( new AssetFilterCollectionType() );
	
	for (int Index = 0; Index < InArgs._AssetPickerConfig.ExtraFilters->Num(); Index++)
	{
		FilterCollection->Add( InArgs._AssetPickerConfig.ExtraFilters->GetFilterAtIndex( Index ) );
	}

	OnAssetsActivated = InArgs._AssetPickerConfig.OnAssetsActivated;
	OnAssetSelected = InArgs._AssetPickerConfig.OnAssetSelected;
	OnAssetClicked = InArgs._AssetPickerConfig.OnAssetClicked;
	OnAssetDoubleClicked = InArgs._AssetPickerConfig.OnAssetDoubleClicked;
	OnAssetEnterPressed = InArgs._AssetPickerConfig.OnAssetEnterPressed;
	bPendingFocusNextFrame = InArgs._AssetPickerConfig.bFocusSearchBoxWhenOpened;

	if ( InArgs._AssetPickerConfig.ClearSelectionDelegate != NULL )
	{
		(*InArgs._AssetPickerConfig.ClearSelectionDelegate) = FClearSelectionDelegate::CreateSP( this, &SAssetPicker::ClearSelection ) ;
	}

	for ( auto AssetIt = InArgs._AssetPickerConfig.GetCurrentSelectionDelegates.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		if ( (*AssetIt) != NULL )
		{
			(**AssetIt) = FGetCurrentSelectionDelegate::CreateSP( this, &SAssetPicker::GetCurrentSelection );
		}
	}

	if ( InArgs._AssetPickerConfig.AdjustSelectionDelegate != NULL )
	{
		(*InArgs._AssetPickerConfig.AdjustSelectionDelegate) = FAdjustSelectionDelegate::CreateSP( this, &SAssetPicker::AdjustSelection ) ;
	}

	TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);

	ChildSlot
	[
		VerticalBox
	];

	TAttribute< FText > HighlightText = InArgs._AssetPickerConfig.HighlightedText;
	TAttribute< EVisibility > LabelVisibility = InArgs._AssetPickerConfig.LabelVisibility;
	EThumbnailLabel::Type ThumbnailLabel = InArgs._AssetPickerConfig.ThumbnailLabel;

	// Search box
	if (!InArgs._AssetPickerConfig.bAutohideSearchBar)
	{
		TextFilter = MakeShareable( new TTextFilter< AssetFilterType >( TTextFilter< AssetFilterType >::FItemToStringArray::CreateStatic( &AssetToString ) ) );
		FilterCollection->Add( TextFilter );
		HighlightText = TAttribute< FText >( this, &SAssetPicker::GetHighlightedText );

		OtherDevelopersFilter = MakeShareable(new FOtherDevelopersAssetFilter());
		FilterCollection->Add( OtherDevelopersFilter );

		VerticalBox->AddSlot()
		.AutoHeight()
		.Padding( 0, 0, 0, 1 )
		[
			SNew( SHorizontalBox )

			+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew( SearchBoxPtr, SAssetSearchBox )
						.HintText(NSLOCTEXT( "ContentBrowser", "SearchBoxHint", "Search Assets" ))
						.OnTextChanged( this, &SAssetPicker::OnSearchBoxChanged )
						.OnTextCommitted( this, &SAssetPicker::OnSearchBoxCommitted )
						.DelayChangeNotificationsWhileTyping( true )
				]

			+ SHorizontalBox::Slot()
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
				]
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
	FARFilter InitialBackendFilter = InArgs._AssetPickerConfig.Filter;
	InitialBackendFilter.PackagePaths.Empty();

	VerticalBox->AddSlot()
	.FillHeight(1.f)
	[
		SAssignNew(AssetViewPtr, SAssetView)
		.SelectionMode( InArgs._AssetPickerConfig.SelectionMode )
		.OnShouldFilterAsset(InArgs._AssetPickerConfig.OnShouldFilterAsset)
		.OnAssetClicked(InArgs._AssetPickerConfig.OnAssetClicked)
		.OnAssetSelected(InArgs._AssetPickerConfig.OnAssetSelected)
		.OnAssetsActivated(this, &SAssetPicker::HandleAssetsActivated)
		.OnGetAssetContextMenu(InArgs._AssetPickerConfig.OnGetAssetContextMenu)
		.AreRealTimeThumbnailsAllowed(this, &SAssetPicker::IsHovered)
		.InitialSourcesData(InitialSourcesData)
		.InitialBackendFilter(InitialBackendFilter)
		.InitialViewType(InArgs._AssetPickerConfig.InitialAssetViewType)
		.InitialAssetSelection(InArgs._AssetPickerConfig.InitialAssetSelection)
		.ThumbnailScale(InArgs._AssetPickerConfig.ThumbnailScale)
		.OnThumbnailScaleChanged(InArgs._AssetPickerConfig.ThumbnailScaleChanged)
		.ShowBottomToolbar(InArgs._AssetPickerConfig.bShowBottomToolbar)
		.OnAssetTagWantsToBeDisplayed(InArgs._AssetPickerConfig.OnAssetTagWantsToBeDisplayed)
		.OnAssetDragged( InArgs._AssetPickerConfig.OnAssetDragged )
		.AllowDragging( InArgs._AssetPickerConfig.bAllowDragging )
		.CanShowClasses( InArgs._AssetPickerConfig.bCanShowClasses )
		.CanShowFolders( InArgs._AssetPickerConfig.bCanShowFolders )
		.CanShowOnlyAssetsInSelectedFolders( InArgs._AssetPickerConfig.bCanShowOnlyAssetsInSelectedFolders )
		.CanShowRealTimeThumbnails( InArgs._AssetPickerConfig.bCanShowRealTimeThumbnails )
		.CanShowDevelopersFolder( InArgs._AssetPickerConfig.bCanShowDevelopersFolder )
		.DynamicFilters( FilterCollection )
		.HighlightedText( HighlightText )
		.LabelVisibility( LabelVisibility )
		.ThumbnailLabel( ThumbnailLabel )
		.ConstructToolTipForAsset( InArgs._AssetPickerConfig.ConstructToolTipForAsset )
		.AssetShowWarningText( InArgs._AssetPickerConfig.AssetShowWarningText)
		.AllowFocusOnSync(false)	// Stop the asset view from stealing focus (we're in control of that)
	];

	AssetViewPtr->RequestListRefresh();
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
		HandleAssetsActivated(SelectionSet, EAssetTypeActivationMethod::EnterPressed);

		return FReply::Handled();
	}

	if (SelectionDelta != 0)
	{
		AssetViewPtr->AdjustActiveSelection(SelectionDelta);

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
		if ( SelectionSet.Num()==0 )
		{
			AssetViewPtr->AdjustActiveSelection(1);
			SelectionSet = AssetViewPtr->GetSelectedAssets();
		}
		HandleAssetsActivated(SelectionSet, EAssetTypeActivationMethod::EnterPressed);
	}
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
	else if (ActivationMethod == EAssetTypeActivationMethod::EnterPressed)
	{
		OnAssetEnterPressed.ExecuteIfBound(ActivatedAssets);
	}

	OnAssetsActivated.ExecuteIfBound( ActivatedAssets, ActivationMethod );
}

void SAssetPicker::ClearSelection()
{
	AssetViewPtr->ClearSelection();
}

TArray< FAssetData > SAssetPicker::GetCurrentSelection()
{
	return AssetViewPtr->GetSelectedAssets();
}

void SAssetPicker::AdjustSelection(const int32 direction)
{
	AssetViewPtr->AdjustActiveSelection(direction);
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

#undef LOCTEXT_NAMESPACE
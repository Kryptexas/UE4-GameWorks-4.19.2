// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ContentBrowserPCH.h"

#include "CollectionAssetManagement.h"
#include "CollectionViewTypes.h"
#include "CollectionContextMenu.h"
#include "ObjectTools.h"
#include "SourcesViewWidgets.h"
#include "ContentBrowserModule.h"
#include "SExpandableArea.h"
#include "SSearchBox.h"

#define LOCTEXT_NAMESPACE "ContentBrowser"

void SCollectionView::Construct( const FArguments& InArgs )
{
	OnCollectionSelected = InArgs._OnCollectionSelected;
	bAllowCollectionButtons = InArgs._AllowCollectionButtons;
	bAllowRightClickMenu = InArgs._AllowRightClickMenu;

	FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
	CollectionManagerModule.Get().OnCollectionCreated().AddSP( this, &SCollectionView::HandleCollectionCreated );
	CollectionManagerModule.Get().OnCollectionRenamed().AddSP( this, &SCollectionView::HandleCollectionRenamed );
	CollectionManagerModule.Get().OnCollectionDestroyed().AddSP( this, &SCollectionView::HandleCollectionDestroyed );

	Commands = TSharedPtr< FUICommandList >(new FUICommandList);
	CollectionContextMenu = MakeShareable(new FCollectionContextMenu( SharedThis(this) ));
	CollectionContextMenu->BindCommands(Commands);

	if ( InArgs._AllowQuickAssetManagement )
	{
		QuickAssetManagement = MakeShareable(new FCollectionAssetManagement());
	}

	FOnContextMenuOpening CollectionListContextMenuOpening;
	if ( InArgs._AllowContextMenu )
	{
		CollectionListContextMenuOpening = FOnContextMenuOpening::CreateSP( this, &SCollectionView::MakeCollectionListContextMenu );
	}

	PreventSelectionChangedDelegateCount = 0;

	TSharedRef< SWidget > HeaderContent = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Font( FEditorStyle::GetFontStyle("ContentBrowser.SourceTitleFont") )
					.Text( LOCTEXT("CollectionsListTitle", "Collections") )
					.Visibility( this, &SCollectionView::GetCollectionsTitleTextVisibility )
				]

				+ SHorizontalBox::Slot()
				[
					SAssignNew(SearchBoxPtr, SSearchBox)
					.HintText( LOCTEXT( "CollectionsViewSearchBoxHint", "Search Collections" ) )
					.OnTextChanged( this, &SCollectionView::ApplyCollectionsSearchFilter )
					.Visibility( this, &SCollectionView::GetCollectionsSearchBoxVisibility )
				]
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.ButtonStyle(FEditorStyle::Get(), "FlatButton")
				.ToolTipText(LOCTEXT("AddCollectionButtonTooltip", "Add a collection."))
				.OnClicked(this, &SCollectionView::MakeAddCollectionMenu)
				.ContentPadding( FMargin(2, 2) )
				.Visibility(this, &SCollectionView::GetAddCollectionButtonVisibility)
				[
					SNew(SImage) .Image( FEditorStyle::GetBrush("ContentBrowser.AddCollectionButtonIcon") )
				]
			];

	TSharedRef< SWidget > BodyContent = SNew(SVerticalBox)
			// Separator
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSeparator)
			]

			// Collections list
			+SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SAssignNew(CollectionListPtr, SListView< TSharedPtr<FCollectionItem> >)
				.ListItemsSource(&FilteredCollectionItems)
				.OnGenerateRow( this, &SCollectionView::GenerateCollectionRow )
				.ItemHeight(18)
				.SelectionMode(ESelectionMode::Multi)
				.OnSelectionChanged(this, &SCollectionView::CollectionSelectionChanged)
				.OnContextMenuOpening( CollectionListContextMenuOpening )
				.OnItemScrolledIntoView(this, &SCollectionView::CollectionItemScrolledIntoView)
				.ClearSelectionOnClick(false)
				.Visibility(this, &SCollectionView::GetCollectionListVisibility)
			];

	TSharedPtr< SWidget > Content;
	if ( InArgs._AllowCollapsing )
	{
		Content = SAssignNew(CollectionsExpandableAreaPtr, SExpandableArea)
			.MaxHeight(200)
			.BorderImage( FEditorStyle::GetBrush("NoBorder") )
			.HeaderPadding( FMargin(4.0f, 0.0f, 0.0f, 0.0f) )
			.HeaderContent()
			[
				SNew(SBox)
				.Padding(FMargin(6.0f, 0.0f, 0.0f, 0.0f))
				[
					HeaderContent
				]
			]
			.BodyContent()
			[
				BodyContent
			];
	}
	else
	{
		Content = SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			HeaderContent
		]

		+SVerticalBox::Slot()
		[
			BodyContent
		];
	}

	ChildSlot
	[
		Content.ToSharedRef()
	];

	UpdateCollectionItems();
}

void SCollectionView::HandleCollectionCreated( const FCollectionNameType& Collection )
{
	UpdateCollectionItems();
}

void SCollectionView::HandleCollectionRenamed( const FCollectionNameType& OriginalCollection, const FCollectionNameType& NewCollection )
{
	UpdateCollectionItems();
}

void SCollectionView::HandleCollectionDestroyed( const FCollectionNameType& Collection )
{
	UpdateCollectionItems();
}

void SCollectionView::UpdateCollectionItems()
{
	CollectionItems.Empty();

	// Load the collection manager module
	FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

	// Get collections of all types
	for (int32 TypeIdx = 0; TypeIdx < ECollectionShareType::CST_All; ++TypeIdx)
	{
		ECollectionShareType::Type CollectionType = ECollectionShareType::Type(TypeIdx);

		//Never display system collections
		if ( CollectionType == ECollectionShareType::CST_System )
		{
			continue;
		}

		TArray<FName> CollectionNames;
		CollectionManagerModule.Get().GetCollectionNames(CollectionType, CollectionNames);
		
		for (int32 CollectionIdx = 0; CollectionIdx < CollectionNames.Num(); ++CollectionIdx)
		{
			const FName& CollectionName = CollectionNames[CollectionIdx];
			CollectionItems.Add( MakeShareable(new FCollectionItem(CollectionName, CollectionType)) );
		}
	}

	CollectionItems.Sort( FCollectionItem::FCompareFCollectionItemByName() );

	ApplyCollectionsSearchFilter();
}

void SCollectionView::ApplyCollectionsSearchFilter()
{
	ApplyCollectionsSearchFilter( SearchBoxPtr->GetText() );
}

void SCollectionView::ApplyCollectionsSearchFilter( const FText& InSearchText )
{
	if ( InSearchText.IsEmptyOrWhitespace() )
	{
		FilteredCollectionItems = CollectionItems;
	}
	else
	{
		const FString& SearchTextString = InSearchText.ToString();

		FilteredCollectionItems.Reset();
		for ( const auto& CollectionItem : CollectionItems )
		{
			if ( CollectionItem->CollectionName.ToString().Contains(SearchTextString) )
			{
				FilteredCollectionItems.Add(CollectionItem);
			}
		}
	}

	CollectionListPtr->RequestListRefresh();
}

FText SCollectionView::GetFilterText() const
{
	return SearchBoxPtr->GetText();
}

void SCollectionView::SetSelectedCollections(const TArray<FCollectionNameType>& CollectionsToSelect)
{
	// Prevent the selection changed delegate since the invoking code requested it
	FScopedPreventSelectionChangedDelegate DelegatePrevention( SharedThis(this) );

	// Expand the collections area if we are indeed selecting at least one collection
	if ( CollectionsToSelect.Num() > 0 && CollectionsExpandableAreaPtr.IsValid() )
	{
		CollectionsExpandableAreaPtr->SetExpanded(true);
	}

	// Clear the selection to start, then add the selected paths as they are found
	CollectionListPtr->ClearSelection();

	for ( auto CollectionIt = CollectionItems.CreateConstIterator(); CollectionIt; ++CollectionIt )
	{
		const TSharedPtr<FCollectionItem>& Item = *CollectionIt;
		for ( auto SelectionIt = CollectionsToSelect.CreateConstIterator(); SelectionIt; ++SelectionIt )
		{
			const FCollectionNameType& Selection = *SelectionIt;
			if ( Item->CollectionName == Selection.Name && Item->CollectionType == Selection.Type )
			{
				CollectionListPtr->SetItemSelection(Item, true);
				CollectionListPtr->RequestScrollIntoView(Item);
			}
		}
	}
}

void SCollectionView::ClearSelection()
{
	// Prevent the selection changed delegate since the invoking code requested it
	FScopedPreventSelectionChangedDelegate DelegatePrevention( SharedThis(this) );

	// Clear the selection to start, then add the selected paths as they are found
	CollectionListPtr->ClearSelection();
}

TArray<FCollectionNameType> SCollectionView::GetSelectedCollections() const
{
	TArray<FCollectionNameType> RetArray;

	TArray<TSharedPtr<FCollectionItem>> Items = CollectionListPtr->GetSelectedItems();
	for ( int32 ItemIdx = 0; ItemIdx < Items.Num(); ++ItemIdx )
	{
		const TSharedPtr<FCollectionItem>& Item = Items[ItemIdx];
		RetArray.Add(FCollectionNameType(Item->CollectionName, Item->CollectionType));
	}

	return RetArray;
}

void SCollectionView::SetSelectedAssets(const TArray<FAssetData>& SelectedAssets)
{
	if ( QuickAssetManagement.IsValid() )
	{
		QuickAssetManagement->SetCurrentAssets(SelectedAssets);
	}
}

void SCollectionView::ApplyHistoryData ( const FHistoryData& History )
{	
	// Prevent the selection changed delegate because it would add more history when we are just setting a state
	FScopedPreventSelectionChangedDelegate DelegatePrevention( SharedThis(this) );

	CollectionListPtr->ClearSelection();
	for ( auto HistoryIt = History.SourcesData.Collections.CreateConstIterator(); HistoryIt; ++HistoryIt)
	{
		const FName Name = (*HistoryIt).Name;
		ECollectionShareType::Type Type = (*HistoryIt).Type;
		for ( auto CollectionIt = CollectionItems.CreateConstIterator(); CollectionIt; ++CollectionIt)
		{
			if ( (*CollectionIt)->CollectionName == Name && (*CollectionIt)->CollectionType == Type )
			{
				CollectionListPtr->SetItemSelection(*CollectionIt, true);
				break;
			}
		}
	}
}

void SCollectionView::SaveSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString) const
{
	FString SelectedCollectionsString;
	TArray< TSharedPtr<FCollectionItem> > CollectionSelectedItems = CollectionListPtr->GetSelectedItems();
	for ( auto CollectionIt = CollectionSelectedItems.CreateConstIterator(); CollectionIt; ++CollectionIt )
	{
		if ( SelectedCollectionsString.Len() > 0 )
		{
			SelectedCollectionsString += TEXT(",");
		}

		const TSharedPtr<FCollectionItem>& Collection = *CollectionIt;
		SelectedCollectionsString += Collection->CollectionName.ToString() + TEXT("?") + FString::FromInt(Collection->CollectionType);
	}
	const bool IsCollectionsExpanded = CollectionsExpandableAreaPtr.IsValid() ? CollectionsExpandableAreaPtr->IsExpanded() : true;
	GConfig->SetBool(*IniSection, *(SettingsString + TEXT(".CollectionsExpanded")), IsCollectionsExpanded, IniFilename);
	GConfig->SetString(*IniSection, *(SettingsString + TEXT(".SelectedCollections")), *SelectedCollectionsString, IniFilename);
}

void SCollectionView::LoadSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString)
{
	// Collection expansion state
	bool bCollectionsExpanded = false;
	if ( CollectionsExpandableAreaPtr.IsValid() && GConfig->GetBool(*IniSection, *(SettingsString + TEXT(".CollectionsExpanded")), bCollectionsExpanded, IniFilename) )
	{
		CollectionsExpandableAreaPtr->SetExpanded(bCollectionsExpanded);
	}

	// Selected Collections
	FString SelectedCollectionsString;
	TArray<FCollectionNameType> NewSelectedCollections;
	if ( GConfig->GetString(*IniSection, *(SettingsString + TEXT(".SelectedCollections")), SelectedCollectionsString, IniFilename) )
	{
		TArray<FString> NewSelectedCollectionStrings;
		SelectedCollectionsString.ParseIntoArray(NewSelectedCollectionStrings, TEXT(","), /*bCullEmpty*/true);

		for ( auto CollectionIt = NewSelectedCollectionStrings.CreateConstIterator(); CollectionIt; ++CollectionIt )
		{
			FString CollectionName;
			FString CollectionTypeString;
			if ( (*CollectionIt).Split(TEXT("?"), &CollectionName, &CollectionTypeString) )
			{
				int32 CollectionType = FCString::Atoi(*CollectionTypeString);

				if ( CollectionType >= 0 && CollectionType < ECollectionShareType::CST_All )
				{
					new(NewSelectedCollections) FCollectionNameType(FName(*CollectionName), ECollectionShareType::Type(CollectionType) );
				}
			}
		}
	}

	if ( NewSelectedCollections.Num() > 0 )
	{
		// Select the collections
		SetSelectedCollections(NewSelectedCollections);
		CollectionSelectionChanged( TSharedPtr<FCollectionItem>(), ESelectInfo::Direct );
	}
}

FReply SCollectionView::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
{
	if( Commands->ProcessCommandBindings( InKeyEvent ) )
	{
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

bool SCollectionView::ShouldAllowSelectionChangedDelegate() const
{
	return PreventSelectionChangedDelegateCount == 0;
}

FReply SCollectionView::MakeAddCollectionMenu()
{
	// Get all menu extenders for this context menu from the content browser module
	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>( TEXT("ContentBrowser") );
	TArray<FContentBrowserMenuExtender> MenuExtenderDelegates = ContentBrowserModule.GetAllCollectionViewContextMenuExtenders();

	TArray<TSharedPtr<FExtender>> Extenders;
	for (int32 i = 0; i < MenuExtenderDelegates.Num(); ++i)
	{
		if (MenuExtenderDelegates[i].IsBound())
		{
			Extenders.Add(MenuExtenderDelegates[i].Execute());
		}
	}
	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, NULL, MenuExtender, true);

	CollectionContextMenu->UpdateProjectSourceControl();

	CollectionContextMenu->MakeNewCollectionSubMenu(MenuBuilder);

	FSlateApplication::Get().PushMenu(
		AsShared(),
		FWidgetPath(),
		MenuBuilder.MakeWidget(),
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::TopMenu )
		);

	return FReply::Handled();
}

EVisibility SCollectionView::GetCollectionsTitleTextVisibility() const
{
	// Only show the title text if we have an expansion area, but are collapsed
	return (CollectionsExpandableAreaPtr.IsValid() && !CollectionsExpandableAreaPtr->IsExpanded()) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SCollectionView::GetCollectionsSearchBoxVisibility() const
{
	// Only show the search box if we have an expanded expansion area, or aren't currently using an expansion area
	return (!CollectionsExpandableAreaPtr.IsValid() || CollectionsExpandableAreaPtr->IsExpanded()) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SCollectionView::GetAddCollectionButtonVisibility() const
{
	return (bAllowCollectionButtons && ( !CollectionsExpandableAreaPtr.IsValid() || CollectionsExpandableAreaPtr->IsExpanded() ) ) ? EVisibility::Visible : EVisibility::Collapsed;
}

void SCollectionView::CreateCollectionItem( ECollectionShareType::Type CollectionType )
{
	if ( ensure(CollectionType != ECollectionShareType::CST_All) )
	{
		FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

		const FName BaseCollectionName = *LOCTEXT("NewCollectionName", "NewCollection").ToString();
		FName CollectionName;
		CollectionManagerModule.Get().CreateUniqueCollectionName(BaseCollectionName, CollectionType, CollectionName);
		TSharedPtr<FCollectionItem> NewItem = MakeShareable(new FCollectionItem(CollectionName, CollectionType));

		// Adding a new collection now, so clear any filter we may have applied
		SearchBoxPtr->SetText(FText::GetEmpty());

		// Mark the new collection for rename and that it is new so it will be created upon successful rename
		NewItem->bRenaming = true;
		NewItem->bNewCollection = true;

		CollectionItems.Add( NewItem );
		CollectionItems.Sort( FCollectionItem::FCompareFCollectionItemByName() );
		ApplyCollectionsSearchFilter();
		CollectionListPtr->RequestScrollIntoView(NewItem);
		CollectionListPtr->SetSelection( NewItem );
	}
}

void SCollectionView::RenameCollectionItem( const TSharedPtr<FCollectionItem>& ItemToRename )
{
	if ( ensure(ItemToRename.IsValid()) )
	{
		ItemToRename->bRenaming = true;
		CollectionListPtr->RequestScrollIntoView(ItemToRename);
	}
}

void SCollectionView::RemoveCollectionItems( const TArray<TSharedPtr<FCollectionItem>>& ItemsToRemove )
{
	TArray<TSharedPtr<FCollectionItem>> SelectedItems = CollectionListPtr->GetSelectedItems();

	// Remove all the items, while keeping track of the number of selected items that were removed.
	int32 LastSelectedItemIdx = INDEX_NONE;
	int32 NumSelectedItemsRemoved = 0;
	for (int32 RemoveIdx = 0; RemoveIdx < ItemsToRemove.Num(); ++RemoveIdx)
	{
		const TSharedPtr<FCollectionItem>& ItemToRemove = ItemsToRemove[RemoveIdx];

		int32 ItemIdx = INDEX_NONE;
		if ( CollectionItems.Find(ItemToRemove, ItemIdx) )
		{
			if ( SelectedItems.Contains(ItemToRemove) )
			{
				NumSelectedItemsRemoved++;
				LastSelectedItemIdx = ItemIdx;
			}

			CollectionItems.RemoveAt(ItemIdx);
		}
	}

	FScopedPreventSelectionChangedDelegate DelegatePrevention(SharedThis(this));
	CollectionListPtr->ClearSelection();

	// If we removed all the selected items and there is at least one other item, select it
	if ( LastSelectedItemIdx > INDEX_NONE && NumSelectedItemsRemoved > 0 && NumSelectedItemsRemoved >= SelectedItems.Num() && CollectionItems.Num() > 1 )
	{
		// The last selected item idx will refer to the next item in the list now that it has been removed.
		// If the removed item was the last item in the list, select the new last item
		const int32 NewSelectedItemIdx = FMath::Min(LastSelectedItemIdx, CollectionItems.Num() - 1);
		
		CollectionListPtr->SetSelection(CollectionItems[NewSelectedItemIdx]);
	}

	// Refresh the list
	ApplyCollectionsSearchFilter();
}

EVisibility SCollectionView::GetCollectionListVisibility() const
{
	return CollectionItems.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedRef<ITableRow> SCollectionView::GenerateCollectionRow( TSharedPtr<FCollectionItem> CollectionItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	check(CollectionItem.IsValid());

	// Only bind the check box callbacks if we're allowed to show check boxes
	TAttribute<bool> IsCollectionCheckBoxEnabledAttribute;
	TAttribute<ECheckBoxState> IsCollectionCheckedAttribute;
	FOnCheckStateChanged OnCollectionCheckStateChangedDelegate;
	if ( QuickAssetManagement.IsValid() )
	{
		IsCollectionCheckBoxEnabledAttribute.Bind(TAttribute<bool>::FGetter::CreateSP(this, &SCollectionView::IsCollectionCheckBoxEnabled, CollectionItem));
		IsCollectionCheckedAttribute.Bind(TAttribute<ECheckBoxState>::FGetter::CreateSP(this, &SCollectionView::IsCollectionChecked, CollectionItem));
		OnCollectionCheckStateChangedDelegate.BindSP(this, &SCollectionView::OnCollectionCheckStateChanged, CollectionItem);
	}

	TSharedPtr< STableRow< TSharedPtr<FTreeItem> > > TableRow = SNew( STableRow< TSharedPtr<FTreeItem> >, OwnerTable );
	TableRow->SetContent
		(
			SNew(SCollectionListItem)
			.ParentWidget(SharedThis(this))
			.CollectionItem(CollectionItem)
			.OnNameChangeCommit(this, &SCollectionView::CollectionNameChangeCommit)
			.OnVerifyRenameCommit(this, &SCollectionView::CollectionVerifyRenameCommit)
			.OnAssetsDragDropped(this, &SCollectionView::CollectionAssetsDropped)
			.IsSelected( TableRow.Get(), &STableRow< TSharedPtr<FTreeItem> >::IsSelectedExclusively )
			.IsReadOnly(this, &SCollectionView::IsCollectionNotRenamable)
			.HighlightText( this, &SCollectionView::GetFilterText)
			.IsCheckBoxEnabled( IsCollectionCheckBoxEnabledAttribute )
			.IsCollectionChecked( IsCollectionCheckedAttribute )
			.OnCollectionCheckStateChanged( OnCollectionCheckStateChangedDelegate )
		);

	return TableRow.ToSharedRef();
}

TSharedPtr<SWidget> SCollectionView::MakeCollectionListContextMenu()
{
	if ( !bAllowRightClickMenu )
	{
		return NULL;
	}

	return CollectionContextMenu->MakeCollectionListContextMenu(Commands);
}

bool SCollectionView::IsCollectionCheckBoxEnabled( TSharedPtr<FCollectionItem> CollectionItem ) const
{
	return QuickAssetManagement.IsValid() && QuickAssetManagement->IsCollectionEnabled(FCollectionNameType(CollectionItem->CollectionName, CollectionItem->CollectionType));
}

ECheckBoxState SCollectionView::IsCollectionChecked( TSharedPtr<FCollectionItem> CollectionItem ) const
{
	if ( QuickAssetManagement.IsValid() )
	{
		return QuickAssetManagement->GetCollectionCheckState(FCollectionNameType(CollectionItem->CollectionName, CollectionItem->CollectionType));
	}
	return ECheckBoxState::Unchecked;
}

void SCollectionView::OnCollectionCheckStateChanged( ECheckBoxState NewState, TSharedPtr<FCollectionItem> CollectionItem )
{
	if ( QuickAssetManagement.IsValid() )
	{
		switch(NewState)
		{
		case ECheckBoxState::Checked:
			QuickAssetManagement->AddCurrentAssetsToCollection(FCollectionNameType(CollectionItem->CollectionName, CollectionItem->CollectionType));
			break;

		case ECheckBoxState::Unchecked:
			QuickAssetManagement->RemoveCurrentAssetsFromCollection(FCollectionNameType(CollectionItem->CollectionName, CollectionItem->CollectionType));
			break;

		default:
			break;
		}
	}
}

void SCollectionView::CollectionSelectionChanged( TSharedPtr< FCollectionItem > CollectionItem, ESelectInfo::Type /*SelectInfo*/ )
{
	if ( ShouldAllowSelectionChangedDelegate() && OnCollectionSelected.IsBound() )
	{
		if ( CollectionItem.IsValid() )
		{
			OnCollectionSelected.Execute(FCollectionNameType(CollectionItem->CollectionName, CollectionItem->CollectionType));
		}
		else
		{
			OnCollectionSelected.Execute(FCollectionNameType(NAME_None, ECollectionShareType::CST_All));
		}
	}
}

void SCollectionView::CollectionAssetsDropped(const TArray<FAssetData>& AssetList, const TSharedPtr<FCollectionItem>& CollectionItem, FText& OutMessage)
{
	TArray<FName> ObjectPaths;

	for ( auto AssetIt = AssetList.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		ObjectPaths.Add((*AssetIt).ObjectPath);
	}

	FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
	int32 NumAdded = 0;
	if ( CollectionManagerModule.Get().AddToCollection(CollectionItem->CollectionName, CollectionItem->CollectionType, ObjectPaths, &NumAdded) )
	{
		if ( AssetList.Num() == 1 )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("AssetName"), FText::FromName(AssetList[0].AssetName) );
			Args.Add( TEXT("CollectionName"), FText::FromName(CollectionItem->CollectionName) );
			OutMessage = FText::Format( LOCTEXT("CollectionAssetsAdded", "Added {AssetName} to {CollectionName}"), Args );
		}
		else
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("Number"), NumAdded );
			Args.Add( TEXT("CollectionName"), FText::FromName(CollectionItem->CollectionName) );
			OutMessage = FText::Format( LOCTEXT("CollectionAssetsAdded", "Added {Number} asset(s) to {CollectionName}"), Args );
		}
	}
	else
	{
		OutMessage = CollectionManagerModule.Get().GetLastError();
	}
}

void SCollectionView::CollectionItemScrolledIntoView( TSharedPtr<FCollectionItem> CollectionItem, const TSharedPtr<ITableRow>& Widget )
{
	if ( CollectionItem->bRenaming && Widget.IsValid() && Widget->GetContent().IsValid() )
	{
		CollectionItem->OnRenamedRequestEvent.Broadcast();
	}
}

bool SCollectionView::IsCollectionNotRenamable() const
{
	CollectionContextMenu->UpdateProjectSourceControl();
	return !CollectionContextMenu->CanRenameSelectedCollections();
}

bool SCollectionView::CollectionNameChangeCommit( const TSharedPtr< FCollectionItem >& CollectionItem, const FString& NewName, bool bChangeConfirmed, FText& OutWarningMessage )
{
	// There should only ever be one item selected when renaming
	check(CollectionListPtr->GetNumItemsSelected() == 1);

	FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
	ECollectionShareType::Type CollectionType = CollectionItem->CollectionType;

	// If new name is empty, set it back to the original name
	const FName NewNameFinal( NewName.IsEmpty() ? CollectionItem->CollectionName : FName(*NewName) );

	if ( CollectionItem->bNewCollection )
	{
		CollectionItem->bNewCollection = false;

		// If we can canceled the name change when creating a new asset, we want to silently remove it
		if ( !bChangeConfirmed )
		{
			CollectionItems.Remove(CollectionItem);
			ApplyCollectionsSearchFilter();
			return false;
		}

		if ( !CollectionManagerModule.Get().CreateCollection(NewNameFinal, CollectionType) )
		{
			// Failed to add the collection, remove it from the list
			CollectionItems.Remove(CollectionItem);
			ApplyCollectionsSearchFilter();

			OutWarningMessage = FText::Format( LOCTEXT("CreateCollectionFailed", "Failed to create the collection. {0}"), CollectionManagerModule.Get().GetLastError());
			return false;
		}
	}
	else
	{
		// If the old name is the same as the new name, just early exit here.
		if ( CollectionItem->CollectionName == NewNameFinal )
		{
			return true;
		}

		// If the new name doesn't pass our current filter, we need to clear it
		if ( !NewNameFinal.ToString().Contains(SearchBoxPtr->GetText().ToString()) )
		{
			SearchBoxPtr->SetText(FText::GetEmpty());
		}

		// Otherwise perform the rename
		if ( !CollectionManagerModule.Get().RenameCollection(CollectionItem->CollectionName, CollectionType, NewNameFinal, CollectionType) )
		{
			// Failed to rename the collection
			OutWarningMessage = FText::Format( LOCTEXT("RenameCollectionFailed", "Failed to rename the collection. {0}"), CollectionManagerModule.Get().GetLastError());
			return false;
		}
	}

	// Make sure we are sorted
	CollectionItems.Sort( FCollectionItem::FCompareFCollectionItemByName() );

	// At this point CollectionItem is no longer a member of the CollectionItems list (as the list is repopulated by
	// UpdateCollectionItems, which is called by a broadcast from CollectionManagerModule::RenameCollection, above).
	// So search again for the item by name and type.

	auto FindCollectionItemPredicate = [CollectionType, &NewNameFinal] (const TSharedPtr<FCollectionItem>& Item)
	{
		return ( Item->CollectionType == CollectionType &&
				 Item->CollectionName == NewNameFinal );
	};

	auto NewCollectionItemPtr = CollectionItems.FindByPredicate( FindCollectionItemPredicate );

	// Reselect the path to notify that the selection has changed
	{
		FScopedPreventSelectionChangedDelegate DelegatePrevention( SharedThis(this) );
		CollectionListPtr->ClearSelection();
	}

	// Set the selection
	if (NewCollectionItemPtr)
	{
		const auto& NewCollectionItem = *NewCollectionItemPtr;
		CollectionListPtr->RequestScrollIntoView(NewCollectionItem);
		CollectionListPtr->SetItemSelection(NewCollectionItem, true);
	}

	return true;
}

bool SCollectionView::CollectionVerifyRenameCommit(const TSharedPtr< FCollectionItem >& CollectionItem, const FString& NewName, const FSlateRect& MessageAnchor, FText& OutErrorMessage)
{
	const FName NewNameFinal = *NewName;

	// If the new name is the same as the old name, consider this to be unchanged, and accept it.
	if (CollectionItem->CollectionName == NewNameFinal)
	{
		return true;
	}

	FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

	if (CollectionManagerModule.Get().CollectionExists(NewNameFinal, ECollectionShareType::CST_All))
	{
		// This collection already exists, inform the user and continue
		OutErrorMessage = FText::Format(LOCTEXT("RenameCollectionAlreadyExists", "A collection already exists with the name '{0}'."), FText::FromName(NewNameFinal));

		// Return false to indicate that the user should enter a new name
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

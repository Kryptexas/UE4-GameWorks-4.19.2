// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

DECLARE_DELEGATE_ThreeParams( FOnRenameBegin, const TSharedPtr<FAssetViewItem>& /*AssetItem*/, const FString& /*OriginalName*/, const FSlateRect& /*MessageAnchor*/)
DECLARE_DELEGATE_FourParams( FOnRenameCommit, const TSharedPtr<FAssetViewItem>& /*AssetItem*/, const FString& /*NewName*/, const FSlateRect& /*MessageAnchor*/, ETextCommit::Type /*CommitType*/ )
DECLARE_DELEGATE_RetVal_FourParams( bool, FOnVerifyRenameCommit, const TSharedPtr<FAssetViewItem>& /*AssetItem*/, const FText& /*NewName*/, const FSlateRect& /*MessageAnchor*/, FText& /*OutErrorMessage*/)
DECLARE_DELEGATE_OneParam( FOnItemDestroyed, const TSharedPtr<FAssetViewItem>& /*AssetItem*/);

namespace FAssetViewModeUtils 
{
	FReply OnViewModeKeyDown( const TSet< TSharedPtr<FAssetViewItem> >& SelectedItems, const FKeyboardEvent& InKeyboardEvent );
}

/** The tile view mode of the asset view */
class SAssetTileView : public STileView<TSharedPtr<FAssetViewItem>>
{
public:
	virtual bool SupportsKeyboardFocus() const OVERRIDE { return true; }
	virtual FReply OnKeyDown( const FGeometry& InGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
};

/** The list view mode of the asset view */
class SAssetListView : public SListView<TSharedPtr<FAssetViewItem>>
{
public:
	virtual bool SupportsKeyboardFocus() const OVERRIDE { return true; }
	virtual FReply OnKeyDown( const FGeometry& InGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
};

/** The columns view mode of the asset view */
class SAssetColumnView : public SListView<TSharedPtr<FAssetViewItem>>
{
public:
	virtual bool SupportsKeyboardFocus() const OVERRIDE { return true; }
	virtual FReply OnKeyDown( const FGeometry& InGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
};

/** A base class for all asset view items */
class SAssetViewItem : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_TwoParams( FOnAssetsDragDropped, const TArray<FAssetData>& /*AssetList*/, const FString& /*DestinationPath*/);
	DECLARE_DELEGATE_TwoParams( FOnPathsDragDropped, const TArray<FString>& /*PathNames*/, const FString& /*DestinationPath*/);
	DECLARE_DELEGATE_TwoParams( FOnFilesDragDropped, const TArray<FString>& /*FileNames*/, const FString& /*DestinationPath*/);

	SLATE_BEGIN_ARGS( SAssetViewItem )
		: _ShouldAllowToolTip(true)
		, _ThumbnailEditMode(false)
		, _LabelVisibility(EVisibility::Visible)
		, _ConstructToolTip()
		{}

		/** Data for the asset this item represents */
		SLATE_ARGUMENT( TSharedPtr<FAssetViewItem>, AssetItem )

		/** Delegate for when an asset name has entered a rename state */
		SLATE_EVENT( FOnRenameBegin, OnRenameBegin )

		/** Delegate for when an asset name has been entered for an item that is in a rename state */
		SLATE_EVENT( FOnRenameCommit, OnRenameCommit )

		/** Delegate for when an asset name has been entered for an item to verify the name before commit */
		SLATE_EVENT( FOnVerifyRenameCommit, OnVerifyRenameCommit )

		/** Called when any asset item is destroyed. Used in thumbnail management */
		SLATE_EVENT( FOnItemDestroyed, OnItemDestroyed )

		/** If false, the tooltip will not be displayed */
		SLATE_ATTRIBUTE( bool, ShouldAllowToolTip )

		/** If true, display the thumbnail edit mode UI */
		SLATE_ATTRIBUTE( bool, ThumbnailEditMode )

		/** The visibility state of the labels used in asset view items */
		SLATE_ATTRIBUTE( EVisibility, LabelVisibility )

		/** The tooltip to display when the cursor hovers over this widget */
		SLATE_EVENT( FConstructToolTipForAsset, ConstructToolTip )

		/** Delegate for when assets are dropped on this item, if it is a folder */
		SLATE_EVENT( FOnAssetsDragDropped, OnAssetsDragDropped )

		/** Delegate for when asset paths are dropped on this folder, if it is a folder  */
		SLATE_EVENT( FOnPathsDragDropped, OnPathsDragDropped )

		/** Delegate for when a list of files is dropped on this folder (if it is a folder) from an external source */
		SLATE_EVENT( FOnFilesDragDropped, OnFilesDragDropped )

	SLATE_END_ARGS()

	/** Virtual destructor */
	virtual ~SAssetViewItem();

	/** Performs common initialization logic for all asset view items */
	void Construct( const FArguments& InArgs );

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	virtual TSharedPtr<SToolTip> GetToolTip() OVERRIDE;
	virtual void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;

	/** Returns the color this item should be tinted with */
	virtual FSlateColor GetAssetColor() const;

	/** Get the border image to display */
	const FSlateBrush* GetBorderImage() const;

	/** Get the name text to be displayed for this item */
	FText GetNameText() const;

protected:
	/** Handles starting a name change */
	virtual void HandleBeginNameChange( const FText& OriginalText );

	/** Handles committing a name change */
	virtual void HandleNameCommitted( const FText& NewText, ETextCommit::Type CommitInfo );

	/** Handles verifying a name change */
	virtual bool HandleVerifyNameChanged( const FText& NewText, FText& OutErrorMessage );

	/** Handles committing a name change */
	virtual void OnAssetDataChanged();

	/** Notification for when the dirty flag changes */
	virtual void DirtyStateChanged();

	/** Gets the brush for the source control indicator image */
	const FSlateBrush* GetSCCStateImage() const;

	/** Gets the brush for the dirty indicator image */
	const FSlateBrush* GetDirtyImage() const;

	/** Gets the visibility for the thumbnail edit mode UI */
	EVisibility GetThumbnailEditModeUIVisibility() const;

	/** Creates a tooltip widget for this item */
	TSharedRef<SToolTip> CreateToolTipWidget() const;

	/** Gets the visibility of the checked out by other text block in the tooltip */
	EVisibility GetCheckedOutByOtherTextVisibility() const;

	/** Gets the text for the checked out by other text block in the tooltip */
	FString GetCheckedOutByOtherText() const;

	/** Helper function for CreateToolTipWidget. Gets the user description for the asset, if it exists. */
	FText GetAssetUserDescription() const;

	/** Helper function for CreateToolTipWidget. Adds a key value pair to the info box of the tooltip */
	void AddToToolTipInfoBox(const TSharedRef<SVerticalBox>& InfoBox, const FText& Key, const FText& Value, bool bImportant) const;

	/** Updates the bPackageDirty flag */
	void UpdatePackageDirtyState();

	/** Returns true if the package that contains the asset that this item represents is dirty. */
	bool IsDirty() const;

	/** Update the source control state of this item if required */
	void UpdateSourceControlState(float InDeltaTime);

	/** Cache the package name from the asset we are representing */
	void CachePackageName();

	/** Whether this item is a folder */
	bool IsFolder() const;

protected:

	TSharedPtr< SInlineEditableTextBlock > InlineRenameWidget;

	/** The data for this item */
	TSharedPtr<FAssetViewItem> AssetItem;

	/** The package containing the asset that this item represents */
	TWeakObjectPtr<UPackage> AssetPackage;

	/** The cached name of the package containing the asset that this item represents */
	FString CachedPackageName;

	/** The cached filename of the package containing the asset that this item represents */
	FString CachedPackageFileName;

	/** Delegate for when an asset name has entered a rename state */
	FOnRenameBegin OnRenameBegin;

	/** Delegate for when an asset name has been entered for an item that is in a rename state */
	FOnRenameCommit OnRenameCommit;

	/** Delegate for when an asset name has been entered for an item to verify the name before commit */
	FOnVerifyRenameCommit OnVerifyRenameCommit;

	/** Called when any asset item is destroyed. Used in thumbnail management */
	FOnItemDestroyed OnItemDestroyed;

	/** The geometry last frame. Used when telling popup messages where to appear. */
	FGeometry LastGeometry;

	/** If false, the tooltip will not be displayed */
	TAttribute<bool> ShouldAllowToolTip;

	/** If true, display the thumbnail edit mode UI */
	TAttribute<bool> ThumbnailEditMode;

	/** The visibility state of the labels used in asset view items */
	TAttribute<EVisibility> LabelVisibility;

	/**
	 * A map of class names to their important asset registry tags and values.
	 * Important tags will be featured in the tooltip if the value matches.
	 * An empty string is interpreted as indicating that any value is always important.
	 */
	TMap< FName, TMap<FName, FString> > ImportantTagMap;

	/** Cached brushes for the dirty state */
	const FSlateBrush* AssetDirtyBrush;

	/** Cached flag describing if the package is dirty */
	bool bPackageDirty;

	/** The callback for external code to construct the tooltip used for this asset */
	FConstructToolTipForAsset ConstructToolTipForAsset;

	/** Flag indicating whether we have requested initial source control state */
	bool bSourceControlStateRequested;

	/** Delay timer before we request a source control state update, to prevent spam */
	float SourceControlStateDelay;

	/** Delegate for when a list of assets is dropped on this item, if it is a folder */
	FOnAssetsDragDropped OnAssetsDragDropped;

	/** Delegate for when a list of folder paths is dropped on this item, if it is a folder */
	FOnPathsDragDropped OnPathsDragDropped;

	/** Delegate for when a list of files is dropped on this item (if it is a folder) from an external source */
	FOnFilesDragDropped OnFilesDragDropped;

	/** Whether an item is dragged over us or not */
	bool bDraggedOver;
};


/** An item in the asset list view */
class SAssetListItem : public SAssetViewItem
{
public:
	SLATE_BEGIN_ARGS( SAssetListItem )
		: _ThumbnailPadding(0)
		, _ThumbnailLabel( EThumbnailLabel::ClassName )
		, _ThumbnailHintColorAndOpacity( FLinearColor( 0.0f, 0.0f, 0.0f, 0.0f ) )
		, _ItemHeight(16)
		, _ShouldAllowToolTip(true)
		, _ThumbnailEditMode(false)
		, _AllowThumbnailHintLabel(false)
		{}

		/** The handle to the thumbnail this item should render */
		SLATE_ARGUMENT( TSharedPtr<FAssetThumbnail>, AssetThumbnail)

		/** Data for the asset this item represents */
		SLATE_ARGUMENT( TSharedPtr<FAssetViewItem>, AssetItem )

		/** How much padding to allow around the thumbnail */
		SLATE_ARGUMENT( float, ThumbnailPadding )

		/** The contents of the label displayed on the thumbnail */
		SLATE_ARGUMENT( EThumbnailLabel::Type, ThumbnailLabel )

		/**  */
		SLATE_ATTRIBUTE( FLinearColor, ThumbnailHintColorAndOpacity )

		/** The height of the list item */
		SLATE_ATTRIBUTE( float, ItemHeight )

		/** Delegate for when an asset name has entered a rename state */
		SLATE_EVENT( FOnRenameBegin, OnRenameBegin )

		/** Delegate for when an asset name has been entered for an item that is in a rename state */
		SLATE_EVENT( FOnRenameCommit, OnRenameCommit )

		/** Delegate for when an asset name has been entered for an item to verify the name before commit */
		SLATE_EVENT( FOnVerifyRenameCommit, OnVerifyRenameCommit )

		/** Called when any asset item is destroyed. Used in thumbnail management */
		SLATE_EVENT( FOnItemDestroyed, OnItemDestroyed )

		/** The tooltip to display when the cursor hovers over this widget */
		SLATE_EVENT( FConstructToolTipForAsset, ConstructToolTip )

		/** If false, the tooltip will not be displayed */
		SLATE_ATTRIBUTE( bool, ShouldAllowToolTip )

		/** The string in the title to highlight (used when searching by string) */
		SLATE_ATTRIBUTE( FText, HighlightText )

		/** If true, the thumbnail in this item can be edited */
		SLATE_ATTRIBUTE( bool, ThumbnailEditMode )

		/** Whether the thumbnail should ever show it's hint label */
		SLATE_ARGUMENT( bool, AllowThumbnailHintLabel )

		/** Whether the item is selected in the view */
		SLATE_ARGUMENT( FIsSelected, IsSelected )

		/** Delegate for when assets are dropped on this item, if it is a folder */
		SLATE_EVENT( FOnAssetsDragDropped, OnAssetsDragDropped )

		/** Delegate for when asset paths are dropped on this folder, if it is a folder  */
		SLATE_EVENT( FOnPathsDragDropped, OnPathsDragDropped )

		/** Delegate for when a list of files is dropped on this folder (if it is a folder) from an external source */
		SLATE_EVENT( FOnFilesDragDropped, OnFilesDragDropped )

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs );

	/** Handles committing a name change */
	virtual void OnAssetDataChanged() OVERRIDE;

private:
	/** Returns the size of the thumbnail widget */
	FOptionalSize GetThumbnailBoxSize() const;

	/** Returns the size of the source control state box widget */
	FOptionalSize GetSCCImageSize() const;

private:
	/** The handle to the thumbnail that this item is rendering */
	TSharedPtr<FAssetThumbnail> AssetThumbnail;

	/** The height allowed for this item */
	TAttribute<float> ItemHeight;
};

/** An item in the asset tile view */
class SAssetTileItem : public SAssetViewItem
{
public:
	SLATE_BEGIN_ARGS( SAssetTileItem )
		: _ThumbnailPadding(0)
		, _ItemWidth(16)
		, _ShouldAllowToolTip(true)
		, _ThumbnailEditMode(false)
		, _LabelVisibility(EVisibility::Visible)
		, _ThumbnailLabel( EThumbnailLabel::ClassName )
		, _ThumbnailHintColorAndOpacity( FLinearColor( 0.0f, 0.0f, 0.0f, 0.0f ) )
		, _AllowThumbnailHintLabel(true)
	{}

		/** The handle to the thumbnail this item should render */
		SLATE_ARGUMENT( TSharedPtr<FAssetThumbnail>, AssetThumbnail)

		/** Data for the asset this item represents */
		SLATE_ARGUMENT( TSharedPtr<FAssetViewItem>, AssetItem )

		/** How much padding to allow around the thumbnail */
		SLATE_ARGUMENT( float, ThumbnailPadding )

		/** The contents of the label displayed on the thumbnail */
		SLATE_ARGUMENT( EThumbnailLabel::Type, ThumbnailLabel )

		/**  */
		SLATE_ATTRIBUTE( FLinearColor, ThumbnailHintColorAndOpacity )

		/** Whether the thumbnail should ever show it's hint label */
		SLATE_ARGUMENT( bool, AllowThumbnailHintLabel )

		/** The width of the item */
		SLATE_ATTRIBUTE( float, ItemWidth )

		/** Delegate for when an asset name has entered a rename state */
		SLATE_EVENT( FOnRenameBegin, OnRenameBegin )

		/** Delegate for when an asset name has been entered for an item that is in a rename state */
		SLATE_EVENT( FOnRenameCommit, OnRenameCommit )

		/** Delegate for when an asset name has been entered for an item to verify the name before commit */
		SLATE_EVENT( FOnVerifyRenameCommit, OnVerifyRenameCommit )

		/** Called when any asset item is destroyed. Used in thumbnail management */
		SLATE_EVENT( FOnItemDestroyed, OnItemDestroyed )

		/** The tooltip to display when the cursor hovers over this widget */
		SLATE_EVENT( FConstructToolTipForAsset, ConstructToolTip )

		/** If false, the tooltip will not be displayed */
		SLATE_ATTRIBUTE( bool, ShouldAllowToolTip )

		/** The string in the title to highlight (used when searching by string) */
		SLATE_ATTRIBUTE( FText, HighlightText )

		/** If true, the thumbnail in this item can be edited */
		SLATE_ATTRIBUTE( bool, ThumbnailEditMode )

		/** The visibility state of the labels used in asset view items */
		SLATE_ATTRIBUTE( EVisibility, LabelVisibility )

		/** Whether the item is selected in the view */
		SLATE_ARGUMENT( FIsSelected, IsSelected )

		/** Delegate for when assets are dropped on this item, if it is a folder */
		SLATE_EVENT( FOnAssetsDragDropped, OnAssetsDragDropped )

		/** Delegate for when asset paths are dropped on this folder, if it is a folder  */
		SLATE_EVENT( FOnPathsDragDropped, OnPathsDragDropped )

		/** Delegate for when a list of files is dropped on this folder (if it is a folder) from an external source */
		SLATE_EVENT( FOnFilesDragDropped, OnFilesDragDropped )

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs );

	/** Handles committing a name change */
	virtual void OnAssetDataChanged() OVERRIDE;

private:
	/** Returns the size of the thumbnail box widget */
	FOptionalSize GetThumbnailBoxSize() const;

	/** Returns the size of the source control state box widget */
	FOptionalSize GetSCCImageSize() const;

	/** Returns the width at which the name label will wrap the name */
	float GetNameTextWrapWidth() const;

private:
	/** The handle to the thumbnail that this item is rendering */
	TSharedPtr<FAssetThumbnail> AssetThumbnail;

	/** The width of the item. Used to enforce a square thumbnail. */
	TAttribute<float> ItemWidth;

	/** The padding allotted for the thumbnail */
	float ThumbnailPadding;
};

/** An item in the asset tile view */
class SAssetColumnItem : public SAssetViewItem
{
public:
	SLATE_BEGIN_ARGS( SAssetColumnItem ) {}

		/** Data for the asset this item represents */
		SLATE_ARGUMENT( TSharedPtr<FAssetViewItem>, AssetItem )

		/** Delegate for when an asset name has entered a rename state */
		SLATE_EVENT( FOnRenameBegin, OnRenameBegin )

		/** Delegate for when an asset name has been entered for an item that is in a rename state */
		SLATE_EVENT( FOnRenameCommit, OnRenameCommit )

		/** Delegate for when an asset name has been entered for an item to verify the name before commit */
		SLATE_EVENT( FOnVerifyRenameCommit, OnVerifyRenameCommit )

		/** Called when any asset item is destroyed. Used in thumbnail management, though it may be used for more so It is in column items for consistency. */
		SLATE_EVENT( FOnItemDestroyed, OnItemDestroyed )

		/** The tooltip to display when the cursor hovers over this widget */
		SLATE_EVENT( FConstructToolTipForAsset, ConstructToolTip )

		/** The string in the title to highlight (used when searching by string) */
		SLATE_ATTRIBUTE( FText, HighlightText )

		/** Delegate for when assets are dropped on this item, if it is a folder */
		SLATE_EVENT( FOnAssetsDragDropped, OnAssetsDragDropped )

		/** Delegate for when asset paths are dropped on this folder, if it is a folder  */
		SLATE_EVENT( FOnPathsDragDropped, OnPathsDragDropped )

		/** Delegate for when a list of files is dropped on this folder (if it is a folder) from an external source */
		SLATE_EVENT( FOnFilesDragDropped, OnFilesDragDropped )

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs );

	/** Generates a widget for a particular column */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName, FIsSelected InIsSelected );

	/** Handles committing a name change */
	virtual void OnAssetDataChanged() OVERRIDE;

	/** Returns the color this item should be tinted with */
	virtual FSlateColor GetAssetColor() const OVERRIDE;

private:
	/** Gets the tool tip text for the name */
	FString GetAssetNameToolTipText() const;

	/** Gets the name of the class of this asset */
	FText GetAssetClassText() const;

	/** Gets the path to this asset */
	FText GetAssetPathText() const;

	/** Gets the value for the specified asset tag in this asset */
	FText GetAssetTagText(FName AssetRegistryTag) const;

private:
	TAttribute<FText> HighlightText;

	TSharedPtr<STextBlock> ClassText;
	TSharedPtr<STextBlock> PathText;
};

class SAssetColumnViewRow : public SMultiColumnTableRow< TSharedPtr<FAssetViewItem> >
{
public:
	SLATE_BEGIN_ARGS( SAssetColumnViewRow )
		{}

		SLATE_EVENT( FOnDragDetected, OnDragDetected )
		SLATE_ARGUMENT( TSharedPtr<SAssetColumnItem>, AssetColumnItem )

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		this->AssetColumnItem = InArgs._AssetColumnItem;
		ensure(this->AssetColumnItem.IsValid());

		SMultiColumnTableRow< TSharedPtr<FAssetViewItem> >::Construct( 
			FSuperRowType::FArguments()
				.Style(FEditorStyle::Get(), "ContentBrowser.AssetListView.TableRow")
				.OnDragDetected(InArgs._OnDragDetected), 
			InOwnerTableView);
		Content = this->AssetColumnItem;
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) OVERRIDE
	{
		if ( this->AssetColumnItem.IsValid() )
		{
			return this->AssetColumnItem->GenerateWidgetForColumn(ColumnName, FIsSelected::CreateSP( this, &SAssetColumnViewRow::IsSelectedExclusively ));
		}
		else
		{
			return SNew(STextBlock) .Text( NSLOCTEXT("AssetView", "InvalidColumnId", "Invalid Column Item") );
		}
		
	}

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE
	{
		SMultiColumnTableRow< TSharedPtr<FAssetViewItem> >::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

		this->AssetColumnItem->Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	}

	TSharedPtr<SAssetColumnItem> AssetColumnItem;
};

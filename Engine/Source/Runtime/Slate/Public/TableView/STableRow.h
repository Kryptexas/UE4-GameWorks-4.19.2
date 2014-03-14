// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Interface for table views to talk to their rows.
 */
class SLATE_API ITableRow
{
	public:
		/**
		 * @param InIndexInList  The index of the item for which this widget was generated
		 */
		virtual void SetIndexInList( int32 InIndexInList ) = 0;

		/** @return true if the corresponding item is expanded; false otherwise*/
		virtual bool IsItemExpanded() const = 0;

		/** Toggle the expansion of the item associated with this row */
		virtual void ToggleExpansion() = 0;

		/** @return how nested the item associated with this row when it is in a TreeView */
		virtual int32 GetIndentLevel() const = 0;

		/** @return Does this item have children? */
		virtual int32 DoesItemHaveChildren() const = 0;

		/** @return this table row as a widget */
		virtual TSharedRef<SWidget> AsWidget() = 0;

		/** @return the content of this table row */
		virtual TSharedPtr<SWidget> GetContent() = 0;

		/** Called when the expander arrow for this row is shift+clicked */
		virtual void Private_OnExpanderArrowShiftClicked() = 0;
};

template <typename ItemType> class SListView;

DECLARE_DELEGATE_OneParam(FOnTableRowDragEnter, FDragDropEvent const&);

DECLARE_DELEGATE_OneParam(FOnTableRowDragLeave, FDragDropEvent const&);

DECLARE_DELEGATE_RetVal_OneParam(FReply, FOnTableRowDrop, FDragDropEvent const&);
/**
 * The ListView is populated by Selectable widgets.
 * A Selectable widget is away of the ListView containing it (OwnerWidget) and holds arbitrary Content (Content).
 * A Selectable works with its corresponding ListView to provide selection functionality.
 */
template<typename ItemType>
class STableRow : public ITableRow, public SBorder
{
	checkAtCompileTime( TIsValidListItem<ItemType>::Value, ItemType_must_be_a_pointer_or_TSharedPtr );

public:


	SLATE_BEGIN_ARGS( STableRow< ItemType > )
		: _Style( &FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row") )
		, _Padding( FMargin(0) )
		, _ShowSelection(true)
		, _Content()
		{}
	
		SLATE_STYLE_ARGUMENT( FTableRowStyle, Style )

		SLATE_EVENT( FOnDragDetected, OnDragDetected )
		SLATE_EVENT( FOnTableRowDragEnter, OnDragEnter )
		SLATE_EVENT( FOnTableRowDragLeave, OnDragLeave )
		SLATE_EVENT( FOnTableRowDrop,      OnDrop )

		SLATE_ATTRIBUTE( FMargin, Padding )
	
		SLATE_ARGUMENT( bool, ShowSelection )
		
		SLATE_DEFAULT_SLOT( typename STableRow<ItemType>::FArguments, Content )

	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		ConstructInternal(InArgs, InOwnerTableView);

		ConstructChildren(
			InOwnerTableView->TableViewMode,
			InArgs._Padding,
			InArgs._Content.Widget
		);
	}

	virtual void ConstructChildren( ETableViewMode::Type InOwnerTableMode, const TAttribute<FMargin>& InPadding, const TSharedRef<SWidget>& InContent )
	{
		this->Content = InContent;

		if ( InOwnerTableMode == ETableViewMode::List || InOwnerTableMode == ETableViewMode::Tile )
		{
			// -- Row is in a ListView or the user --

			// We just need to hold on to this row's content.
			this->ChildSlot
			.Padding( InPadding )
			[
				InContent
			];
		}
		else
		{
			// -- Row is for TreeView --
			
			// Rows in a TreeView need an expander button and some indentation
			this->ChildSlot
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right) .VAlign(VAlign_Fill)
				[
					SNew(SExpanderArrow, SharedThis(this) )
				]
				+SHorizontalBox::Slot()
					.FillWidth(1)
					.Padding( InPadding )
				[
					InContent
				]
			];
		}
	}

	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE
	{
		int32 Result = SBorder::OnPaint(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

		TSharedRef< ITypedTableView<ItemType> > OwnerWidget = OwnerTablePtr.Pin().ToSharedRef();
		const bool bIsActive = OwnerWidget->AsWidget()->HasKeyboardFocus();
		const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
		if ( bIsActive && OwnerWidget->Private_UsesSelectorFocus() && OwnerWidget->Private_HasSelectorFocus( *MyItem ) )
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				&Style->SelectorFocusedBrush,
				MyClippingRect,
				ESlateDrawEffect::None,
				Style->SelectorFocusedBrush.GetTint( InWidgetStyle ) * InWidgetStyle.GetColorAndOpacityTint()
				);
		}

		return Result;
	}

	/**
	 * Called when a mouse button is double clicked.  Override this in derived classes.
	 *
	 * @param  InMyGeometry  Widget geometry
	 * @param  InMouseEvent  Mouse button event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
	{
		if( InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
		{
			TSharedRef< ITypedTableView<ItemType> > OwnerWidget = OwnerTablePtr.Pin().ToSharedRef();

			// Only one item can be double-clicked
			const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );

			// If we're configured to route double-click messages to the owner of the table, then
			// do that here.  Otherwise, we'll toggle expansion.
			const bool bWasHandled = OwnerWidget->Private_OnItemDoubleClicked( *MyItem );
			if( !bWasHandled )
			{
				ToggleExpansion();
			}

			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	/**
	 * See SWidget::OnMouseButtonDown
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param MouseEvent Information about the input event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */	
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{
		TSharedPtr< ITypedTableView<ItemType> > OwnerWidget = OwnerTablePtr.Pin();
		ChangedSelectionOnMouseDown = false;

		check(OwnerWidget.IsValid());

		if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
		{
			switch( OwnerWidget->Private_GetSelectionMode() )
			{
			case ESelectionMode::Single:
				{
					const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
					const bool bIsSelected = OwnerWidget->Private_IsItemSelected( *MyItem );

					// Select the item under the cursor
					if( !bIsSelected )
					{
						OwnerWidget->Private_ClearSelection();
						OwnerWidget->Private_SetItemSelection( *MyItem, true, true );
						ChangedSelectionOnMouseDown = true;
					}

					return FReply::Handled()
						.DetectDrag( SharedThis(this), EKeys::LeftMouseButton )
						.SetKeyboardFocus( OwnerWidget->AsWidget(), EKeyboardFocusCause::Mouse )
						.CaptureMouse( SharedThis(this) );
				}
				break;

			case ESelectionMode::SingleToggle:
				{
					const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
					const bool bIsSelected = OwnerWidget->Private_IsItemSelected( *MyItem );

					if( !bIsSelected )
					{
						OwnerWidget->Private_ClearSelection();
						OwnerWidget->Private_SetItemSelection( *MyItem, true, true );
						ChangedSelectionOnMouseDown = true;
					}

					return FReply::Handled()
						.DetectDrag( SharedThis(this), EKeys::LeftMouseButton )
						.SetKeyboardFocus( OwnerWidget->AsWidget(), EKeyboardFocusCause::Mouse )
						.CaptureMouse( SharedThis(this) );
				}
				break;

			case ESelectionMode::Multi:
				{
					const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
					const bool bIsSelected = OwnerWidget->Private_IsItemSelected( *MyItem );

					check(MyItem != NULL);

					if ( MouseEvent.IsControlDown() )
					{
						OwnerWidget->Private_SetItemSelection( *MyItem, !bIsSelected, true );
						ChangedSelectionOnMouseDown = true;
					}
					else if ( MouseEvent.IsShiftDown() )
					{
						OwnerWidget->Private_SelectRangeFromCurrentTo( *MyItem );
						ChangedSelectionOnMouseDown = true;
					}
					else if ( !bIsSelected )
					{
						OwnerWidget->Private_ClearSelection();
						OwnerWidget->Private_SetItemSelection( *MyItem, true, true );
						ChangedSelectionOnMouseDown = true;
					}

					return FReply::Handled()
						.DetectDrag( SharedThis(this), EKeys::LeftMouseButton )
						.SetKeyboardFocus( OwnerWidget->AsWidget(), EKeyboardFocusCause::Mouse )
						.CaptureMouse( SharedThis(this) );
				}
				break;
			}
		}

		return FReply::Unhandled();
	}
	
	/**
	 * See SWidget::OnMouseButtonUp
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param MouseEvent Information about the input event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{
		TSharedPtr< ITypedTableView<ItemType> > OwnerWidget = OwnerTablePtr.Pin();
		check(OwnerWidget.IsValid());

		TSharedRef< STableViewBase > OwnerTableViewBase = StaticCastSharedPtr< SListView<ItemType> >( OwnerWidget ).ToSharedRef();

		if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
		{
			FReply Reply = FReply::Unhandled().ReleaseMouseCapture();

			if ( ChangedSelectionOnMouseDown )
			{
				Reply = FReply::Handled().ReleaseMouseCapture();
			}

			const bool bIsUnderMouse = MyGeometry.IsUnderLocation(MouseEvent.GetScreenSpacePosition());
			if ( HasMouseCapture() )
			{
				if ( bIsUnderMouse )
				{
					switch( OwnerWidget->Private_GetSelectionMode() )
					{
					case ESelectionMode::SingleToggle:
						{
							if ( !ChangedSelectionOnMouseDown )
							{
								const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
								const bool bIsSelected = OwnerWidget->Private_IsItemSelected( *MyItem );

								OwnerWidget->Private_ClearSelection();
								OwnerWidget->Private_SignalSelectionChanged(ESelectInfo::OnMouseClick);
							}

							Reply = FReply::Handled().ReleaseMouseCapture();
						}
						break;

					case ESelectionMode::Multi:
						{
							if ( !ChangedSelectionOnMouseDown && !MouseEvent.IsControlDown() && !MouseEvent.IsShiftDown() )
							{
								const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
								check(MyItem != NULL);

								const bool bIsSelected = OwnerWidget->Private_IsItemSelected( *MyItem );
								if ( bIsSelected && OwnerWidget->Private_GetNumSelectedItems() > 1 )
								{
									// We are mousing up on a previous selected item;
									// deselect everything but this item.

									OwnerWidget->Private_ClearSelection();
									OwnerWidget->Private_SetItemSelection( *MyItem, true, true );
									OwnerWidget->Private_SignalSelectionChanged(ESelectInfo::OnMouseClick);

									Reply = FReply::Handled().ReleaseMouseCapture();
								}
							}
						}
						break;
					}
				}

				if ( ChangedSelectionOnMouseDown )
				{
					OwnerWidget->Private_SignalSelectionChanged(ESelectInfo::OnMouseClick);
				}

				return Reply;
			}
		}
		else if ( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && !OwnerTableViewBase->IsRightClickScrolling() )
		{
			// Handle selection of items when releasing the right mouse button, but only if the user isn't actively
			// scrolling the view by holding down the right mouse button.

			switch( OwnerWidget->Private_GetSelectionMode() )
			{
			case ESelectionMode::Single:
			case ESelectionMode::SingleToggle:
			case ESelectionMode::Multi:
				{
					// Only one item can be selected at a time
					const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
					const bool bIsSelected = OwnerWidget->Private_IsItemSelected( *MyItem );

					// Select the item under the cursor
					if( !bIsSelected )
					{
						OwnerWidget->Private_ClearSelection();
						OwnerWidget->Private_SetItemSelection( *MyItem, true, true );
						OwnerWidget->Private_SignalSelectionChanged(ESelectInfo::OnMouseClick);
					}

					OwnerWidget->Private_OnItemRightClicked( *MyItem, MouseEvent );

					return FReply::Handled();
				}
				break;
			}
		}

		return FReply::Unhandled();
	}

	virtual FReply OnDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE
	{
		if ( HasMouseCapture() && ChangedSelectionOnMouseDown )
		{
			TSharedPtr< ITypedTableView<ItemType> > OwnerWidget = OwnerTablePtr.Pin();
			OwnerWidget->Private_SignalSelectionChanged(ESelectInfo::OnMouseClick);
		}

		if (OnDragDetected_Handler.IsBound())
		{
			return OnDragDetected_Handler.Execute( MyGeometry, MouseEvent );
		}
		else
		{
			return FReply::Unhandled();
		}
	}

	virtual void OnDragEnter(FGeometry const& MyGeometry, FDragDropEvent const& DragDropEvent) OVERRIDE
	{
		if (OnDragEnter_Handler.IsBound())
		{
			OnDragEnter_Handler.Execute(DragDropEvent);
		}
		else
		{
			SBorder::OnDragEnter(MyGeometry, DragDropEvent);
		}
	}

	virtual void OnDragLeave(FDragDropEvent const& DragDropEvent) OVERRIDE
	{
		if (OnDragLeave_Handler.IsBound())
		{
			OnDragLeave_Handler.Execute(DragDropEvent);
		}
		else
		{
			SBorder::OnDragLeave(DragDropEvent);
		}
	}

	virtual FReply OnDrop(FGeometry const& MyGeometry, FDragDropEvent const& DragDropEvent) OVERRIDE
	{
		if (OnDrop_Handler.IsBound())
		{
			return OnDrop_Handler.Execute(DragDropEvent);
		}
		else
		{
			return SBorder::OnDrop(MyGeometry, DragDropEvent);
		}
	}

	virtual void SetIndexInList( int32 InIndexInList ) OVERRIDE
	{
		IndexInList = InIndexInList;
	}

	virtual bool IsItemExpanded() const OVERRIDE
	{
		TSharedPtr< ITypedTableView<ItemType> > OwnerWidget = OwnerTablePtr.Pin();
		const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
		const bool bIsItemExpanded = OwnerWidget->Private_IsItemExpanded( *MyItem );
		return bIsItemExpanded;
	}

	virtual void ToggleExpansion() OVERRIDE
	{
		TSharedPtr< ITypedTableView<ItemType> > OwnerWidget = OwnerTablePtr.Pin();

		const bool bItemHasChildren = OwnerWidget->Private_DoesItemHaveChildren( IndexInList );
		// Nothing to expand if row being clicked on doesn't have children
		if( bItemHasChildren )
		{
			ItemType MyItem = *(OwnerWidget->Private_ItemFromWidget( this ));
			const bool IsItemExpanded = bItemHasChildren && OwnerWidget->Private_IsItemExpanded( MyItem );
			OwnerWidget->Private_SetItemExpansion( MyItem, !IsItemExpanded );
		}
	}

	virtual int32 GetIndentLevel() const OVERRIDE
	{
		return OwnerTablePtr.Pin()->Private_GetNestingDepth( IndexInList );
	}

	virtual int32 DoesItemHaveChildren() const OVERRIDE
	{
		return OwnerTablePtr.Pin()->Private_DoesItemHaveChildren( IndexInList );
	}

	virtual TSharedRef<SWidget> AsWidget() OVERRIDE
	{
		return SharedThis(this);
	}

	virtual TSharedPtr<SWidget> GetContent() OVERRIDE
	{
		if ( this->Content.IsValid() )
		{
			return this->Content.Pin();
		}
		else
		{
			return TSharedPtr<SWidget>();
		}
	}

	virtual void Private_OnExpanderArrowShiftClicked() OVERRIDE
	{
		TSharedPtr< ITypedTableView<ItemType> > OwnerWidget = OwnerTablePtr.Pin();

		const bool bItemHasChildren = OwnerWidget->Private_DoesItemHaveChildren( IndexInList );
		// Nothing to expand if row being clicked on doesn't have children
		if( bItemHasChildren )
		{
			ItemType MyItem = *(OwnerWidget->Private_ItemFromWidget( this ));
			const bool IsItemExpanded = bItemHasChildren && OwnerWidget->Private_IsItemExpanded( MyItem );
			OwnerWidget->Private_OnExpanderArrowShiftClicked( MyItem, !IsItemExpanded );
		}
	}

	/** @return The border to be drawn around this list item */
	const FSlateBrush* GetBorder() const 
	{
		TSharedPtr< ITypedTableView<ItemType> > OwnerWidget = OwnerTablePtr.Pin();

		const bool bIsActive = OwnerWidget->AsWidget()->HasKeyboardFocus();

		// @todo: Slate Style - make this part of the widget style
		const FSlateBrush* WhiteBox = FCoreStyle::Get().GetBrush("GenericWhiteBox");

		const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );

		check(MyItem);

		const bool bIsSelected = OwnerWidget->Private_IsItemSelected( *MyItem );

		if (bIsSelected && bShowSelection)
		{
			if( bIsActive )
			{
				return IsHovered()
					? &Style->ActiveHoveredBrush
					: &Style->ActiveBrush;
			}
			else
			{
				return IsHovered()
					? &Style->InactiveHoveredBrush
					: &Style->InactiveBrush;
			}
		}
		else
		{
			// Add a slightly lighter background for even rows
			const bool bAllowSelection = OwnerWidget->Private_GetSelectionMode() != ESelectionMode::None;
			if( IndexInList % 2 == 0 )
			{
				return ( IsHovered() && bAllowSelection )
					? &Style->EvenRowBackgroundHoveredBrush
					: &Style->EvenRowBackgroundBrush;

			}
			else
			{
				return ( IsHovered() && bAllowSelection )
					? &Style->OddRowBackgroundHoveredBrush
					: &Style->OddRowBackgroundBrush;

			}
		}
	}

	/** 
	 * Callback to determine if the row is selected or not
	 *
	 * @return		true if selected by owning widget.
	 */
	bool IsSelectedExclusively() const
	{
		TSharedPtr< ITypedTableView< ItemType > > OwnerWidget = OwnerTablePtr.Pin();

		if(!OwnerWidget->AsWidget()->HasKeyboardFocus() || OwnerWidget->Private_GetNumSelectedItems() > 1)
		{
			return false;
		}

		const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
		return OwnerWidget->Private_IsItemSelected( *MyItem );
	}

	/** Protected constructor; SWidgets should only be instantiated via declarative syntax. */
	STableRow()
	: IndexInList(0)
	, bShowSelection(true)
	{
	}

protected:	
	/**
	 * An internal method to construct and setup this row widget (purposely avoids child construction). 
	 * Split out from Construct() so that sub-classes can invoke super construction without invoking 
	 * ConstructChildren() (sub-classes may want to constuct their own children in their own special way).
	 * 
	 * @param  InArgs			Declaration data for this widget.
	 * @param  InOwnerTableView	The table that this row belongs to.
	 */
	void ConstructInternal(FArguments const& InArgs, TSharedRef<STableViewBase> const& InOwnerTableView)
	{
		check(InArgs._Style);
		Style = InArgs._Style;

		this->BorderImage = TAttribute<const FSlateBrush*>( this, &STableRow::GetBorder );

		this->ForegroundColor = TAttribute<FSlateColor>( this, &STableRow::GetForegroundBasedOnSelection );

		this->OnDragDetected_Handler = InArgs._OnDragDetected;
		this->OnDragEnter_Handler = InArgs._OnDragEnter;
		this->OnDragLeave_Handler = InArgs._OnDragLeave;
		this->OnDrop_Handler = InArgs._OnDrop;
		
		this->SetOwnerTableView( InOwnerTableView );

		this->bShowSelection = InArgs._ShowSelection;
	}

	void SetOwnerTableView( TSharedPtr<STableViewBase> OwnerTableView )
	{
		// We want to cast to a ITypedTableView.
		// We cast to a SListView<ItemType> because C++ doesn't know that
		// being a STableView implies being a ITypedTableView.
		// See SListView.
		this->OwnerTablePtr = StaticCastSharedPtr< SListView<ItemType> >(OwnerTableView);
	}

	FSlateColor GetForegroundBasedOnSelection() const
	{
		const TSharedPtr< ITypedTableView<ItemType> > OwnerWidget = OwnerTablePtr.Pin();
		const FSlateColor& NonSelectedForeground = Style->TextColor; 
		const FSlateColor& SelectedForeground = Style->SelectedTextColor;

		if ( !bShowSelection || !OwnerWidget.IsValid() )
		{
			return NonSelectedForeground;
		}

		const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
		const bool bIsSelected = OwnerWidget->Private_IsItemSelected( *MyItem );

		return bIsSelected
			? SelectedForeground
			: NonSelectedForeground;
	}

protected:

	/** The list that owns this Selectable */
	TWeakPtr< ITypedTableView<ItemType> > OwnerTablePtr;

	/** Index of the corresponding data item in the list */
	int32 IndexInList;

	/** Whether or not to visually show that this row is selected */
	bool bShowSelection;

	/** Style used to draw this table row */
	const FTableRowStyle* Style;

	/** Delegate triggered when a user starts to drag a list item */
	FOnDragDetected OnDragDetected_Handler;

	/** Delegate triggered when a user's drag enters the bounds of this list item */
	FOnTableRowDragEnter OnDragEnter_Handler;

	/** Delegate triggered when a user's drag leaves the bounds of this list item */
	FOnTableRowDragLeave OnDragLeave_Handler;

	/** Delegate triggered when a user's drag is dropped in the bounds of this list item */
	FOnTableRowDrop OnDrop_Handler;

	/** The widget in the content slot for this row */
	TWeakPtr<SWidget> Content;

	bool ChangedSelectionOnMouseDown;
};



template<typename ItemType>
class SMultiColumnTableRow : public STableRow<ItemType>
{
public:

	/**
	 * Users of SMultiColumnTableRow would usually some piece of data associated with it.
	 * The type of this data is ItemType; it's the stuff that your TableView (i.e. List or Tree) is visualizing.
	 * The ColumnName tells you which column of the TableView we need to make a widget for.
	 * Make a widget and return it.
	 *
	 * @param ColumnName    A unique ID for a column in this TableView; see SHeaderRow::FColumn for more info.
	 *
	 * @return a widget to represent the contents of a cell in this row of a TableView. 
	 */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) = 0;

	SLATE_BEGIN_ARGS( SMultiColumnTableRow< ItemType > )
		: _Style( &FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row") )
		, _Padding( FMargin(0) )
		, _ShowSelection(true)
		{}
	
		SLATE_STYLE_ARGUMENT( FTableRowStyle, Style )

		SLATE_EVENT( FOnDragDetected, OnDragDetected )
		SLATE_EVENT( FOnTableRowDragEnter, OnDragEnter )
		SLATE_EVENT( FOnTableRowDragLeave, OnDragLeave )
		SLATE_EVENT( FOnTableRowDrop,      OnDrop )

		SLATE_ATTRIBUTE( FMargin, Padding )
	
		SLATE_ARGUMENT( bool, ShowSelection )
		
	SLATE_END_ARGS()

	typedef SMultiColumnTableRow< ItemType > FSuperRowType;
	typedef typename STableRow<ItemType>::FArguments FTableRowArgs;

protected:
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView )
	{
		STableRow<ItemType>::Construct(
			FTableRowArgs()
			.Style(InArgs._Style)
			.Padding(InArgs._Padding)
			.ShowSelection(InArgs._ShowSelection)
			.OnDragDetected(InArgs._OnDragDetected)
			.OnDragEnter(InArgs._OnDragEnter)
			.OnDragLeave(InArgs._OnDragLeave)
			.OnDrop(InArgs._OnDrop)
			.Content()
			[
				SAssignNew( Box, SHorizontalBox )
			]
		
			, OwnerTableView );

		// Sign up for notifications about changes to the HeaderRow
		TSharedPtr< SHeaderRow > HeaderRow = OwnerTableView->GetHeaderRow();
		check( HeaderRow.IsValid() );
		HeaderRow->OnColumnsChanged()->AddSP( this, &SMultiColumnTableRow<ItemType>::GenerateColumns );

		// Populate the row with user-generated content
		this->GenerateColumns( HeaderRow.ToSharedRef() );
	}

	virtual void ConstructChildren( ETableViewMode::Type InOwnerTableMode, const TAttribute<FMargin>& InPadding, const TSharedRef<SWidget>& InContent ) OVERRIDE
	{
		STableRow<ItemType>::Content = InContent;

		// MultiColumnRows let the user decide which column should contain the expander/indenter item.
		this->ChildSlot
		.Padding( InPadding )
		[
			InContent
		];
	}

	void GenerateColumns( const TSharedRef<SHeaderRow>& InColumnHeaders )
	{
		Box->ClearChildren();
		const TIndirectArray<SHeaderRow::FColumn>& Columns = InColumnHeaders->GetColumns();
		const int32 NumColumns = Columns.Num();
		TMap< FName, TSharedRef< SWidget > > NewColumnIdToSlotContents;

		for( int32 ColumnIndex = 0; ColumnIndex < NumColumns; ++ColumnIndex )
		{
			const SHeaderRow::FColumn& Column = Columns[ColumnIndex];
			const TSharedRef< SWidget >* const ExistingWidget = ColumnIdToSlotContents.Find( Column.ColumnId );

			TSharedRef< SWidget > CellContents = SNullWidget::NullWidget;
			if ( ExistingWidget != NULL )
			{
				CellContents = *ExistingWidget;
			}
			else
			{
				CellContents = GenerateWidgetForColumn( Column.ColumnId );
			}

			if ( Column.SizeRule == EColumnSizeMode::Fixed )
			{
				Box->AddSlot()
				.AutoWidth()
				[
					SNew( SBox )
					.WidthOverride( Column.Width.Get() )
					.HAlign(Column.CellHAlignment)
					.VAlign(Column.CellVAlignment)
					.Content()
					[
						CellContents
					]
				];
			}
			else
			{
				TAttribute<float> WidthBinding;
				WidthBinding.BindRaw( &Column, &SHeaderRow::FColumn::GetWidth );

				SHorizontalBox::FSlot& NewSlot = Box->AddSlot()
				.HAlign(Column.CellHAlignment)
				.VAlign(Column.CellVAlignment)
				.FillWidth( WidthBinding )
				[
					CellContents
				];
			}

			NewColumnIdToSlotContents.Add( Column.ColumnId, CellContents );
		}

		ColumnIdToSlotContents = NewColumnIdToSlotContents;
	}

	void ClearCellCache()
	{
		ColumnIdToSlotContents.Empty();
	}


private:
	
	TSharedPtr<SHorizontalBox> Box;
	TMap< FName, TSharedRef< SWidget > > ColumnIdToSlotContents;
};
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

 #include "Slate.h"
 #include "SWidgetReflector.h"

 /**
  * The WidgetReflector captures layout information and widget hierarchy structure into a tree of FReflectorNode.
  * The node also contains useful visualization and debug info.
  */
class FReflectorNode
{
public:
	/**
	 * FReflectorNodes must be constructed as shared pointers.
	 *
	 * @param InWidgetGeometry   Optional widget and associated geometry which this node should represent
	 */
	static TSharedRef<FReflectorNode> New(const FArrangedWidget& InWidgetGeometry = FArrangedWidget( SNullWidget::NullWidget, FGeometry() ))
	{
		return MakeShareable( new FReflectorNode(InWidgetGeometry) );
	}

	/**
	 * Capture all the children of the supplied widget along with their layout results
	 * Note that we include both visible and invisible children!
	 * 
	 * @param InWidgetGeometry   Widget and geometry whose children to capture in the snapshot.
	 */
	static TSharedRef<FReflectorNode> NewTreeFrom( const FArrangedWidget& InWidgetGeometry )
	{
		TSharedRef<FReflectorNode> NewNode = MakeShareable( new FReflectorNode(InWidgetGeometry) );

		FArrangedChildren ArrangedChildren(EVisibility::All);
		NewNode->Widget.Pin()->ArrangeChildren( NewNode->Geometry, ArrangedChildren );
		for( int32 WidgetIndex=0; WidgetIndex < ArrangedChildren.Num(); ++WidgetIndex )
		{
			// Note that we include both visible and invisible children!
			NewNode->ChildNodes.Add( FReflectorNode::NewTreeFrom( ArrangedChildren(WidgetIndex) )  );
		}
		return NewNode;
	}

	/**
	 * Locate all the widgets from a widget path in a list of ReflectorNodes and their children.
	 *
	 * @param CandidateNodes    A list of FReflectorNodes that represent widgets.
	 * @param WidgetPathToFind  We want to find all reflector nodes corresponding to widgets in this path
	 * @param SearchResult      An array that gets results put in it
	 * @param NodeIndexToFind   Index of the widget in the path that we are currently looking for; we are done when we've found all of them
	 */
	static void FindWidgetPath( const TArray< TSharedPtr<FReflectorNode> >& CandidateNodes, const FWidgetPath& WidgetPathToFind, TArray< TSharedPtr<FReflectorNode> >& SearchResult, int32 NodeIndexToFind = 0 )
	{
		if (NodeIndexToFind < WidgetPathToFind.Widgets.Num())
		{
			const FArrangedWidget& WidgetToFind = WidgetPathToFind.Widgets(NodeIndexToFind);

			for ( int32 NodeIndex=0; NodeIndex < CandidateNodes.Num(); ++NodeIndex )
			{
				if ( CandidateNodes[NodeIndex]->Widget.Pin() == WidgetPathToFind.Widgets(NodeIndexToFind).Widget )
				{
					SearchResult.Add( CandidateNodes[NodeIndex] );
					FindWidgetPath( CandidateNodes[NodeIndex]->ChildNodes, WidgetPathToFind, SearchResult, NodeIndexToFind+1 );				
				}
			}
		}
	}

	/** The widget which this node describes */
	TWeakPtr<SWidget> Widget;
	/** The geometry of the widget */
	FGeometry Geometry;
	/** ReflectorNodes for the Widget's children. */
	TArray< TSharedPtr< FReflectorNode > > ChildNodes;	
	/** A tint that is applied to text in order to provide visual hints */
	FLinearColor Tint;
	/** Should we visualize this node */
	bool bVisualizeThisNode;

protected:
	/**
	 * FReflectorNode must be constructor through static methods.
	 *
	 * @param InWidgetGeometry   Widget and associated geometry that this node will represent
	 */
	FReflectorNode( const FArrangedWidget& InWidgetGeometry )
	: Widget( InWidgetGeometry.Widget )
	, Geometry( InWidgetGeometry.Geometry )
	, Tint(FLinearColor(1,1,1))
	, bVisualizeThisNode(true)
	{
	}


 };

/**
 * Widget that visualizes the contents of a FReflectorNode
 */
class SLATE_API SReflectorTreeWidgetItem : public SMultiColumnTableRow< TSharedPtr<FReflectorNode> >
{
public:
	SLATE_BEGIN_ARGS(SReflectorTreeWidgetItem)
		: _WidgetInfoToVisualize()
		, _SourceCodeAccessor()
		{}

		SLATE_ARGUMENT( TSharedPtr<FReflectorNode>, WidgetInfoToVisualize )
		SLATE_ARGUMENT( FAccessSourceCode, SourceCodeAccessor )
	SLATE_END_ARGS()

	/**
	 * Construct child widgets that comprise this widget.
	 *
	 * @param InArgs  Declaration from which to construct this widget
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		this->WidgetInfo = InArgs._WidgetInfoToVisualize;
		this->OnAccessSourceCode = InArgs._SourceCodeAccessor;

		SMultiColumnTableRow< TSharedPtr<FReflectorNode> >::Construct( SMultiColumnTableRow< TSharedPtr<FReflectorNode> >::FArguments().Padding(1), InOwnerTableView );
	}

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) OVERRIDE
	{
		if (ColumnName == TEXT("WidgetName"))
		{
			return SNew(SHorizontalBox)
			
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SExpanderArrow, SharedThis(this))
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2, 0)
				.VAlign(VAlign_Center)
				[
					SNew( STextBlock )
						.Text( this, &SReflectorTreeWidgetItem::GetWidgetType )
						.ColorAndOpacity( this, &SReflectorTreeWidgetItem::GetTint )
				];
		}
		else if (ColumnName == TEXT("WidgetInfo"))
		{
			return SNew(SBox)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding( FMargin( 2, 0 ) )
				[
					SNew( SHyperlink )
						.Text( this, &SReflectorTreeWidgetItem::GetReadableLocation )
						.OnNavigate( this, &SReflectorTreeWidgetItem::HandleHyperlinkNavigate )
				];
		}
		else if (ColumnName == "Visibility" )
		{
			return SNew(SBox)
				.HAlign( HAlign_Center )
				.VAlign(VAlign_Center)
				.Padding( FMargin( 2, 0 ) )
				[
					SNew(STextBlock)
						.Text( this, &SReflectorTreeWidgetItem::GetVisibilityAsString )
				];
		}
		else if (ColumnName == "ForegroundColor")
		{
			TSharedPtr<SWidget> ThisWidget = WidgetInfo.Get()->Widget.Pin();
			FSlateColor Foreground = (ThisWidget.IsValid())
				? ThisWidget->GetForegroundColor()
				: FSlateColor::UseForeground();

			return SNew(SBorder)
				// Show unset color as an empty space.
				.Visibility( Foreground.IsColorSpecified() ? EVisibility::Visible : EVisibility::Hidden )
				// Show a checkerboard background so we can see alpha values well
				.BorderImage( FCoreStyle::Get().GetBrush("Checkerboard") )
				.VAlign(VAlign_Center)
				.Padding( FMargin( 2, 0 ) )
				[
					// Show a color block
					SNew(SColorBlock)
						.Color( Foreground.GetSpecifiedColor() )
						.Size(FVector2D(16,16))
				];
		}
		else
		{
			return SNullWidget::NullWidget;
		}
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

protected:
	/** @return String representation of the widget we are visualizing */
	FString GetWidgetType() const
	{
		return WidgetInfo.Get()->Widget.IsValid()
			? WidgetInfo.Get()->Widget.Pin()->GetTypeAsString()
			: TEXT("Null Widget");
	}
	
	FString GetReadableLocation() const
	{
		return WidgetInfo.Get()->Widget.IsValid()
			? WidgetInfo.Get()->Widget.Pin()->GetReadableLocation()
			: FString();
	}

	FString GetWidgetFileAndLineNumber() const
	{
		return WidgetInfo.Get()->Widget.IsValid()
			? WidgetInfo.Get()->Widget.Pin()->GetParsableFileAndLineNumber()
			: FString();
	}

	FString GetVisibilityAsString() const
	{
		TSharedPtr<SWidget> TheWidget = WidgetInfo.Get()->Widget.Pin();
		return TheWidget.IsValid()
			? TheWidget->GetVisibility().ToString()
			: FString();
	}

	/** @return The tint of the reflector node */
	FSlateColor GetTint () const
	{
		return WidgetInfo.Get()->Tint;
	}

	void HandleHyperlinkNavigate()
	{
		OnAccessSourceCode.ExecuteIfBound(GetWidgetFileAndLineNumber());
	}

	/** The info about the widget that we are visualizing */
	TAttribute< TSharedPtr<FReflectorNode> > WidgetInfo;

	FAccessSourceCode OnAccessSourceCode;
 };


class SLATE_API SReflectorToolTipWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SReflectorToolTipWidget )
		: _WidgetInfoToVisualize()
	{}

	SLATE_ARGUMENT( TSharedPtr<FReflectorNode>, WidgetInfoToVisualize )

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs )
	{
		this->WidgetInfo = InArgs._WidgetInfoToVisualize;

		ChildSlot
		[
			SNew( SGridPanel )
			.FillColumn( 1, 1.0f )

			// Desired Size
			+ SGridPanel::Slot( 0, 0 )
			[
				SNew( STextBlock )
				.Text( NSLOCTEXT( "WidgetReflector", "DesiredSize", "Desired Size" ) )
			]

			// Desired Size Value
			+ SGridPanel::Slot( 1, 0 )
			.Padding(5, 0, 0, 0)
			[
				SNew( STextBlock )
				.Text( this, &SReflectorToolTipWidget::GetWidgetsDesiredSize )
			]

			// Actual Size
			+ SGridPanel::Slot( 0, 1 )
			[
				SNew( STextBlock )
				.Text( NSLOCTEXT( "WidgetReflector", "ActualSize", "Actual Size" ) )
			]

			// Actual Size Value
			+ SGridPanel::Slot( 1, 1 )
			.Padding( 5, 0, 0, 0 )
			[
				SNew( STextBlock )
				.Text( this, &SReflectorToolTipWidget::GetWidgetActualSize )
			]

			// Size Info
			+ SGridPanel::Slot( 0, 2 )
			[
				SNew( STextBlock )
				.Text( NSLOCTEXT( "WidgetReflector", "SizeInfo", "Size Info" ) )
			]

			// Size Info Value
			+ SGridPanel::Slot( 1, 2 )
			.Padding( 5, 0, 0, 0 )
			[
				SNew( STextBlock )
				.Text( this, &SReflectorToolTipWidget::GetSizeInfo )
			]
		];
	}

private:
	FString GetWidgetsDesiredSize() const
	{
		TSharedPtr<SWidget> TheWidget = WidgetInfo.Get()->Widget.Pin();
		return TheWidget.IsValid()
			? TheWidget->GetDesiredSize().ToString()
			: FString();
	}

	FString GetWidgetActualSize() const
	{
		return WidgetInfo.Get()->Geometry.Size.ToString();
	}

	/** @return String representation of the widget's geometry */
	FString GetSizeInfo() const
	{
		TSharedPtr<SWidget> TheWidget = WidgetInfo.Get()->Widget.Pin();

		return TheWidget.IsValid()
			? WidgetInfo.Get()->Geometry.ToString()
			: FString();
	}

private:

	/** The info about the widget that we are visualizing */
	TAttribute< TSharedPtr<FReflectorNode> > WidgetInfo;
};


/** The actual implementation of the reflector */
class SWidgetReflectorImpl : public SWidgetReflector
{
public:
	SLATE_BEGIN_ARGS(SWidgetReflectorImpl){}

	SLATE_END_ARGS()

	/** Create widgets that comprise this WidgetReflector implementation */
	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct(const FArguments& InArgs)
	{
		this->ChildSlot
		[
			SNew(SBorder)
			.BorderImage( FCoreStyle::Get().GetBrush( "ToolPanel.GroupBorder" ) )
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew( STextBlock )
						. Text( NSLOCTEXT("WidgetReflector", "AppScale", "Application Scale: ").ToString() )
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew( SSpinBox<float> )
						. Value(this, &SWidgetReflectorImpl::GetAppScaleValue)
						. MinValue(0.1f)
						. MaxValue(3.0f)
						. Delta(0.01f)
						. OnValueChanged( this, &SWidgetReflectorImpl::OnAppscaleSliderChanged )
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(5)
					[
						// Check box that controls LIVE MODE
						SNew(SCheckBox)
						.IsChecked( this, &SWidgetReflectorImpl::ShouldShowFocus )
						.OnCheckStateChanged( this, &SWidgetReflectorImpl::ShowFocus_OnIsCheckedChanged)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("WidgetReflector", "ShowFocus", "Show Focus").ToString())
						]
					]
					+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(5)
					[
						// Check box that controls PICKING A WIDGET TO INSPECT
						SNew(SButton)
						.OnClicked(this, &SWidgetReflectorImpl::LiveMode_OnIsCheckedChanged)
						.ButtonColorAndOpacity( this, &SWidgetReflectorImpl::GetPickButtonColor )
						[
							SNew(STextBlock)
							.Text(this, &SWidgetReflectorImpl::GetPickButtonText)
						]
					]
				
				]			
				+ SVerticalBox::Slot()
				.FillHeight(1)
				[
					// The tree view that shows all the info that we capture.
					SAssignNew( ReflectorTree, SReflectorTree )
					.ItemHeight( 24 )
					.TreeItemsSource( &ReflectorTreeRoot )
					.OnGenerateRow( this, &SWidgetReflectorImpl::GenerateWidgetForReflectorNode )
					.OnGetChildren( this, &SWidgetReflectorImpl::GetChildrenForWidget )
					.OnSelectionChanged( this, &SWidgetReflectorImpl::SelectedNodesChanged )
					.HeaderRow
					(
						SNew(SHeaderRow)
						+ SHeaderRow::Column( "WidgetName" ).DefaultLabel(NSLOCTEXT("WidgetReflector", "WidgetName", "Widget Name").ToString()).FillWidth( 0.50f )
						+ SHeaderRow::Column( "ForegroundColor" ).DefaultLabel(NSLOCTEXT("WidgetReflector", "ForegroundColor", "Foreground Color").ToString()).FixedWidth( 20 )
						+ SHeaderRow::Column( "Visibility" ).DefaultLabel( NSLOCTEXT( "WidgetReflector", "Visibility", "Visibility" ).ToString() ).FixedWidth( 125 )
						+ SHeaderRow::Column( "WidgetInfo" ).DefaultLabel( NSLOCTEXT( "WidgetReflector", "WidgetInfo", "Widget Info" ).ToString() ).FillWidth( 0.30 )
					)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					// Framerate counter
					SNew( STextBlock )
					.Text( this, &SWidgetReflectorImpl::GetFrameRateAsString )
				]
			]
		];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	/** Widgets must be constructed using the MAKE macro */
	SWidgetReflectorImpl()
	: bShowFocus(false)
	, bIsPicking(false)
	{

	}
	
	/** Called when the user wants to change the scaling in this slate application */
	void OnAppscaleSliderChanged( float NewValue )
	{
		FSlateApplication::Get().SetApplicationScale( NewValue );
	}

	/** @return the current scale of the slate application */
	float GetAppScaleValue() const
	{
		return FSlateApplication::Get().GetApplicationScale();
	}

	/**
	 * Ticks this widget.  Override in derived classes, but always call the parent implementation.
	 *
	 * @param  AllottedGeometry The space allotted for this widget
	 * @param  InCurrentTime  Current absolute real time
	 * @param  InDeltaTime  Real time passed since last tick
	 */
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		ReflectorTree->RequestTreeRefresh();
	}

	/** The reflector uses a tree that observes FReflectorNodes */
	typedef STreeView< TSharedPtr<FReflectorNode> > SReflectorTree;

	/**
	 * Draw the widget path to the picked widget as the widgets' outlines.
	 *
	 * @param InWidgetsToVisualize   A widget path whose widgets' outlines to draw
	 * @param OutDrawElements        A list of draw elements; we will add the output outlines into it
	 * @param LayerId                the maximum layer achieved in OutDrawElements so far
	 *
	 * @return The maximum layerid we achieved while painting
	 */
	int32 VisualizePickAsRectangles(const FWidgetPath& InWidgetsToVisualize, FSlateWindowElementList& OutDrawElements, int32 LayerId)
	{
		const FLinearColor TopmostWidgetColor(1,0,0);
		const FLinearColor LeafmostWidgetColor(0,1,0);

		for(int32 WidgetIndex=0; WidgetIndex < InWidgetsToVisualize.Widgets.Num(); ++WidgetIndex)
		{
			const FArrangedWidget& WidgetGeometry = InWidgetsToVisualize.Widgets(WidgetIndex);

			const float ColorFactor = static_cast<float>(WidgetIndex)/InWidgetsToVisualize.Widgets.Num();
			const FLinearColor Tint(1 - ColorFactor, ColorFactor, 0, 1 );

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				++LayerId,
				FPaintGeometry(
					WidgetGeometry.Geometry.AbsolutePosition - InWidgetsToVisualize.TopLevelWindow->GetPositionInScreen(),
					WidgetGeometry.Geometry.Size * WidgetGeometry.Geometry.Scale,
					WidgetGeometry.Geometry.Scale),
				FCoreStyle::Get().GetBrush(TEXT("Debug.Border")),
				InWidgetsToVisualize.TopLevelWindow->GetClippingRectangleInWindow(),
				ESlateDrawEffect::None,
				FMath::Lerp( TopmostWidgetColor, LeafmostWidgetColor, ColorFactor )
			);
		}

		return LayerId;
	}

	/**
	 * Draw an outline for the specified nodes
	 *
	 * @param InNodesToDraw          A widget path whose widgets' outlines to draw
	 * @param WindowGeometry         The geometry of the window in which to draw
	 * @param OutDrawElements        A list of draw elements; we will add the output outlines into it
	 * @param LayerId                the maximum layer achieved in OutDrawElements so far
	 *
	 * @return The maximum layerid we achieved while painting
	 */
	int32 VisualizeSelectedNodesAsRectangles(const TArray< TSharedPtr<FReflectorNode> >& InNodesToDraw, const TSharedRef<SWindow>& VisualizeInWindow, FSlateWindowElementList& OutDrawElements, int32 LayerId)
	{
		for (int32 NodeIndex=0; NodeIndex < InNodesToDraw.Num(); ++NodeIndex)
		{
			const TSharedPtr<FReflectorNode>& NodeToDraw = InNodesToDraw[NodeIndex];

			const FLinearColor Tint(0,1,0);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				++LayerId,
				FPaintGeometry(
					NodeToDraw->Geometry.AbsolutePosition - VisualizeInWindow->GetPositionInScreen(),
					NodeToDraw->Geometry.Size * NodeToDraw->Geometry.Scale,
					NodeToDraw->Geometry.Scale),
				FCoreStyle::Get().GetBrush(TEXT("Debug.Border")),
				VisualizeInWindow->GetClippingRectangleInWindow(),
				ESlateDrawEffect::None,
				NodeToDraw->Tint
			);
		}

		return LayerId;
	}

	/**
	 * Mark the provided reflector nodes such that they stand out in the tree and are visible.
	 *
	 * @param WidgetPathToObserve   The nodes to mark.
	 */
	void VisualizeAsTree( const TArray< TSharedPtr<FReflectorNode> >& WidgetPathToVisualize )
	{
		const FLinearColor TopmostWidgetColor(1,0,0);
		const FLinearColor LeafmostWidgetColor(0,1,0);
		for( int32 WidgetIndex=0; WidgetIndex<WidgetPathToVisualize.Num(); ++WidgetIndex )
		{
			TSharedPtr<FReflectorNode> CurWidget = WidgetPathToVisualize[ WidgetIndex ];

			// Tint the item based on depth in picked path
			const float ColorFactor = static_cast<float>(WidgetIndex)/WidgetPathToVisualize.Num();
			CurWidget->Tint = FMath::Lerp( TopmostWidgetColor, LeafmostWidgetColor, ColorFactor );

			// Make sure the user can see the picked path in the tree.
			ReflectorTree->SetItemExpansion(CurWidget, true);
		}

		ReflectorTree->RequestScrollIntoView( WidgetPathToVisualize.Last() );
		ReflectorTree->SetSelection( WidgetPathToVisualize.Last() );
	}

	/** Called when the user has picked a widget to observe. */
	virtual void OnWidgetPicked()
	{
		bIsPicking = false;
	}

	/** @return true if we are visualizing the focused widgets */
	virtual bool IsShowingFocus() const
	{
		return bShowFocus;
	}

	/** @return True if the user is in the process of selecting a widget. */
	virtual bool IsInPickingMode() const
	{
		return bIsPicking;
	}

	/** @return True if we should be inspecting widgets and visualizing their layout */
	virtual bool IsVisualizingLayoutUnderCursor() const
	{
		return bIsPicking;
	}

	/** Invoked when the selection in the reflector tree has changed */
	void SelectedNodesChanged( TSharedPtr< FReflectorNode >, ESelectInfo::Type /*SelectInfo*/ )
	{
		SelectedNodes = ReflectorTree->GetSelectedItems();
	}

	/**
	 * Take a snapshot of the UI pertaining to the widget that the user is hovering and visualize it.
	 * If we are not taking a snapshot, draw the overlay from a previous snapshot, if possible.
	 *
	 * @param InWidgetsToVisualize  WidgetPath that the cursor is currently over; could be null.
	 * @param OutDrawElements       List of draw elements to which we will add a visualization overlay
	 * @param LayerId               The maximum layer id attained in the draw element list so far.
	 *
	 * @return The maximum layer ID that we attained while painting the overlay.
	 */
	virtual int32 Visualize( const FWidgetPath& InWidgetsToVisualize, FSlateWindowElementList& OutDrawElements, int32 LayerId )
	{
		const bool bAttemptingToVisualizeReflector = InWidgetsToVisualize.ContainsWidget( SharedThis(this) );

		if( !InWidgetsToVisualize.IsValid() )
		{
			//
			TSharedPtr<SWidget> WindowWidget = ReflectorTreeRoot[0]->Widget.Pin();
			TSharedPtr<SWindow> Window = StaticCastSharedPtr<SWindow>(WindowWidget);
			return VisualizeSelectedNodesAsRectangles(SelectedNodes, Window.ToSharedRef(), OutDrawElements, LayerId);
		}
		else if (!bAttemptingToVisualizeReflector)
		{
			SetWidgetsToVisualize(InWidgetsToVisualize);
			return VisualizePickAsRectangles(InWidgetsToVisualize, OutDrawElements, LayerId);
		}		
		else
		{
			return LayerId;
		}
	}

	/** @param InWidgetsToVisualize  A Widget Path to inspect via the reflector */	
	virtual void SetWidgetsToVisualize( const FWidgetPath& InWidgetsToVisualize )
	{
		ReflectorTreeRoot.Empty();

		if ( InWidgetsToVisualize.IsValid() )
		{
			ReflectorTreeRoot.Add( FReflectorNode::NewTreeFrom(InWidgetsToVisualize.Widgets(0)) );
			PickedPath.Empty();
			FReflectorNode::FindWidgetPath( ReflectorTreeRoot, InWidgetsToVisualize, PickedPath );
			VisualizeAsTree(PickedPath);
		}
	}
	
	/** @param InDelegate A delegate to access source code with */
	virtual void SetSourceAccessDelegate( FAccessSourceCode InDelegate )
	{
		SourceAccessDelegate = InDelegate;
	}

	/**
	 * @param ThisWindow    Do we want to draw something for this window?
	 *
	 * @return true if we want to draw something for this window; false otherwise
	 */
	virtual bool ReflectorNeedsToDrawIn( TSharedRef<SWindow> ThisWindow ) const
	{
		return ( SelectedNodes.Num() > 0 && ReflectorTreeRoot.Num() > 0 && ReflectorTreeRoot[0]->Widget.Pin() == ThisWindow );
	}

private:

	FSlateColor GetPickButtonColor() const
	{
		return bIsPicking
			? FCoreStyle::Get().GetSlateColor("SelectionColor")
			: FCoreStyle::Get().GetSlateColor("DefaultForeground");
	}

	FString GetPickButtonText() const
	{
		static const FString NotPicking = NSLOCTEXT("WidgetReflector", "PickWidget", "Pick Widget").ToString();
		static const FString Picking = NSLOCTEXT("WidgetReflector", "PickingWidget", "Picking (Esc to Stop)").ToString();

		return bIsPicking
			? Picking
			: NotPicking;
	}

	/**
	 * Given a reflector node, generate a visual representation
	 *
	 * @param InReflectorNode   The FReflectorNode to visualize
	 * @param OwnerTable        The TableView (tree view) that is creating this table row.
	 *
	 * @return The widget represetnation of the reflector node
	 */
	TSharedRef<ITableRow> GenerateWidgetForReflectorNode(TSharedPtr<FReflectorNode> InReflectorNode, const TSharedRef<STableViewBase>& OwnerTable)
	{
		return
			SNew( SReflectorTreeWidgetItem, OwnerTable )
			.WidgetInfoToVisualize( InReflectorNode )
			.ToolTip( GenerateToolTipForReflectorNode( InReflectorNode ) )
			.SourceCodeAccessor( SourceAccessDelegate );
	}

	TSharedRef<SToolTip> GenerateToolTipForReflectorNode( TSharedPtr<FReflectorNode> InReflectorNode )
	{
		return SNew(SToolTip)
		[
			SNew(SReflectorToolTipWidget)
			.WidgetInfoToVisualize( InReflectorNode )
		];
	}
	
	ESlateCheckBoxState::Type IsInLiveMode() const
	{
		return bIsPicking ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	/** The user toggled a */
	FReply LiveMode_OnIsCheckedChanged( )
	{
		bIsPicking = !bIsPicking;
		if(bIsPicking)
		{
			bShowFocus = false;
		}
		return FReply::Handled();
	}

	void GetChildrenForWidget( TSharedPtr<FReflectorNode> InWidgetGeometry, TArray< TSharedPtr<FReflectorNode> >& OutChildren )
	{
		OutChildren = InWidgetGeometry->ChildNodes;
	}

	FString GetFrameRateAsString() const
	{

		FString MyString;
#if 0 // the new stats system does not support this
		MyString = FString::Printf( TEXT( "FPS: %0.2f (%0.2f ms)" ), (float)( 1.0f / FPSCounter.GetAverage() ), (float)FPSCounter.GetAverage() * 1000.0f );
#endif
		return MyString;
	}

	ESlateCheckBoxState::Type ShouldShowFocus() const
	{
		return bShowFocus ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	void ShowFocus_OnIsCheckedChanged( ESlateCheckBoxState::Type NewValue )
	{
		bShowFocus = (NewValue != ESlateCheckBoxState::Unchecked);
		if (bShowFocus)
		{
			bIsPicking = false;
		}
	}

private:

	TSharedPtr<SReflectorTree> ReflectorTree;

	TArray< TSharedPtr<FReflectorNode> > SelectedNodes;
	TArray< TSharedPtr<FReflectorNode> > ReflectorTreeRoot;
	TArray< TSharedPtr<FReflectorNode> > PickedPath;

	SSplitter::FSlot* WidgetInfoLocation;

	FAccessSourceCode SourceAccessDelegate;

	bool bShowFocus;
	bool bIsPicking;
 };



/** @return A new widget reflector */
TSharedRef<SWidgetReflector> SWidgetReflector::Make()
{
	TSharedRef<SWidgetReflectorImpl> NewWidgetReflector = SNew( SWidgetReflectorImpl );
	return NewWidgetReflector;
}

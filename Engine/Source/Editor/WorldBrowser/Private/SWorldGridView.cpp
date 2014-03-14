// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "WorldBrowserPrivatePCH.h"

#include "EdGraphUtilities.h"
#include "SWorldGridView.h"
#include "SWorldGridItem.h"

#define LOCTEXT_NAMESPACE "WorldBrowser"

struct FWorldZoomLevelsContainer 
	: public FZoomLevelsContainer
{
	float	GetZoomAmount(int32 InZoomLevel) const OVERRIDE
	{
		return 1.f/FMath::Square(GetNumZoomLevels() - InZoomLevel + 1)*2.f;
	}

	int32 GetNearestZoomLevel( float InZoomAmount ) const OVERRIDE
	{
		for (int32 ZoomLevelIndex=0; ZoomLevelIndex < GetNumZoomLevels(); ++ZoomLevelIndex)
		{
			if (InZoomAmount <= GetZoomAmount(ZoomLevelIndex))
			{
				return ZoomLevelIndex;
			}
		}

		return GetDefaultZoomLevel();
	}

	FString GetZoomPrettyString(int32 InZoomLevel) const OVERRIDE
	{
		return FString::Printf(TEXT("%.5f"), GetZoomAmount(InZoomLevel));
	}

	int32	GetNumZoomLevels() const OVERRIDE
	{
		return 60;
	}

	int32	GetDefaultZoomLevel() const OVERRIDE
	{
		return GetNumZoomLevels() - 10;
	}

	EGraphRenderingLOD::Type GetLOD(int32 InZoomLevel) const OVERRIDE
	{
		return EGraphRenderingLOD::DefaultDetail;
	}
};


//////////////////////////////////////////////////////////////////////////
// SWorldGridView
SWorldLevelsGridView::SWorldLevelsGridView()
	: CommandList(MakeShareable(new FUICommandList))
	, bHasScrollRequest(false)
	, bIsFirstTickCall(true)
	, bHasNodeInteraction(true)
	, SnappingDistance(20.f)
{

	SharedThumbnailRT = new FSlateTextureRenderTarget2DResource(
					FLinearColor::Black, 
					512, 
					512, 
					PF_B8G8R8A8, SF_Point, TA_Wrap, TA_Wrap, 0.0f
				);
	BeginInitResource(SharedThumbnailRT);
}

SWorldLevelsGridView::~SWorldLevelsGridView()
{
	BeginReleaseResource(SharedThumbnailRT);
	FlushRenderingCommands();
	delete SharedThumbnailRT;
		
	WorldModel->SelectionChanged.RemoveAll(this);
	WorldModel->CollectionChanged.RemoveAll(this);
}

void SWorldLevelsGridView::Construct(const FArguments& InArgs)
{
	ZoomLevels = new FWorldZoomLevelsContainer();

	SNodePanel::Construct();
	
	//// bind commands
	//const FWorldTileCommands& Commands = FWorldTileCommands::Get();
	//FUICommandList& ActionList = *CommandList;

	//ActionList.MapAction(Commands.FitToSelection,
	//	FExecuteAction::CreateSP(this, &SWorldLevelsGridView::FitToSelection_Executed),
	//	FCanExecuteAction::CreateSP(this, &SWorldLevelsGridView::AreAnyItemsSelected));

	//ActionList.MapAction(Commands.MoveLevelLeft,
	//	FExecuteAction::CreateSP(this, &SWorldLevelsGridView::MoveLevelLeft_Executed),
	//	FCanExecuteAction::CreateSP(this, &SWorldLevelsGridView::AreAnyItemsSelected));

	//ActionList.MapAction(Commands.MoveLevelRight,
	//	FExecuteAction::CreateSP(this, &SWorldLevelsGridView::MoveLevelRight_Executed),
	//	FCanExecuteAction::CreateSP(this, &SWorldLevelsGridView::AreAnyItemsSelected));

	//ActionList.MapAction(Commands.MoveLevelUp,
	//	FExecuteAction::CreateSP(this, &SWorldLevelsGridView::MoveLevelUp_Executed),
	//	FCanExecuteAction::CreateSP(this, &SWorldLevelsGridView::AreAnyItemsSelected));

	//ActionList.MapAction(Commands.MoveLevelDown,
	//	FExecuteAction::CreateSP(this, &SWorldLevelsGridView::MoveLevelDown_Executed),
	//	FCanExecuteAction::CreateSP(this, &SWorldLevelsGridView::AreAnyItemsSelected));

	//
	BounceCurve.AddCurve(0.0f, 1.0f);
	BounceCurve.Play();
	WorldModel = InArgs._InWorldModel;
	bUpdatingSelection = false;
	
	WorldModel->SelectionChanged.AddSP(this, &SWorldLevelsGridView::OnUpdateSelection);
	WorldModel->CollectionChanged.AddSP(this, &SWorldLevelsGridView::RefreshView);
	SelectionManager.OnSelectionChanged.BindSP(this, &SWorldLevelsGridView::OnSelectionChanged);
	
	RefreshView();
}

FVector2D SWorldLevelsGridView::CursorToWorldPosition(const FGeometry& InGeometry, FVector2D InAbsoluteCursorPosition)
{
	FVector2D ViewSpacePosition = (InAbsoluteCursorPosition - InGeometry.AbsolutePosition)/InGeometry.Scale;
	return PanelCoordToGraphCoord(ViewSpacePosition);
}

FVector2D SWorldLevelsGridView::GetMarqueeWorldSize() const
{
	return WorldMarqueeSize;
}

FVector2D SWorldLevelsGridView::GetMouseWorldLocation() const
{
	return WorldMouseLocation;
}

void SWorldLevelsGridView::FitToSelection_Executed()
{
	FVector2D MinCorner, MaxCorner;
	if (GetBoundsForNodes(true, MinCorner, MaxCorner, 0.f))
	{
		RequestScrollTo((MaxCorner + MinCorner)*0.5f, MaxCorner - MinCorner, true);
	}
}

bool SWorldLevelsGridView::AreAnyItemsSelected() const
{
	return SelectionManager.AreAnyNodesSelected();
}

void SWorldLevelsGridView::AddItem(const TSharedPtr<FLevelModel>& LevelModel)
{
	auto NewNode = SNew(SWorldGridItem)
						.InWorldModel(WorldModel)
						.InItemModel(LevelModel)
						.ThumbnailRenderTarget(SharedThumbnailRT);
	
	AddGraphNode(NewNode);
}

void SWorldLevelsGridView::RemoveItem(const TSharedPtr<FLevelModel>& LevelModel)
{
	TSharedRef<SNode>* pItem = NodeToWidgetLookup.Find(LevelModel->GetNodeObject());
	if (pItem == NULL)
	{
		return;
	}

	Children.Remove(*pItem);
	VisibleChildren.Remove(*pItem);
	NodeToWidgetLookup.Remove(LevelModel->GetNodeObject());
}


void SWorldLevelsGridView::RefreshView()
{
	RemoveAllNodes();

	FLevelModelList AllLevels = WorldModel->GetAllLevels();
	for (auto It = AllLevels.CreateConstIterator(); It; ++It)
	{
		AddItem(*It);
	}
}

void SWorldLevelsGridView::OnUpdateSelection()
{
	if (bUpdatingSelection)
	{
		return;
	}

	bUpdatingSelection = true;

	SelectionManager.ClearSelectionSet();
	FLevelModelList SelectedLevels = WorldModel->GetSelectedLevels();
	for (auto It = SelectedLevels.CreateConstIterator(); It; ++It)
	{
		SelectionManager.SetNodeSelection((*It)->GetNodeObject(), true);
	}

	if (SelectionManager.AreAnyNodesSelected())
	{
		FVector2D MinCorner, MaxCorner;
		if (GetBoundsForNodes(true, MinCorner, MaxCorner, 0.f))
		{
			RequestScrollTo((MaxCorner + MinCorner)*0.5f, MaxCorner - MinCorner);
		}
	}
	bUpdatingSelection = false;
}

void SWorldLevelsGridView::RequestScrollTo(FVector2D InLocation, FVector2D InArea, bool bAllowZoomIn)
{
	bHasScrollRequest = true;
	RequestedScrollLocation = InLocation;
	RequestedZoomArea = InArea;
	RequestedAllowZoomIn = bAllowZoomIn;
}

void SWorldLevelsGridView::OnNewItemAdded(TSharedPtr<FLevelModel> NewItem)
{
	RefreshView();
}

TSharedPtr<SWidget> SWorldLevelsGridView::OnSummonContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (WorldModel->IsReadOnly())
	{
		return SNullWidget::NullWidget;
	}

	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	ArrangeChildren(MyGeometry, ArrangedChildren);

	const int32 NodeUnderMouseIndex = SWidget::FindChildUnderMouse( ArrangedChildren, MouseEvent );
	if (NodeUnderMouseIndex != INDEX_NONE)
	{
		// PRESSING ON A NODE!
		const FArrangedWidget& NodeGeometry = ArrangedChildren(NodeUnderMouseIndex);
		const FVector2D MousePositionInNode = NodeGeometry.Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		TSharedRef<SNode> NodeWidgetUnderMouse = StaticCastSharedRef<SNode>( NodeGeometry.Widget );

		if (NodeWidgetUnderMouse->CanBeSelected(MousePositionInNode))
		{
			if (!SelectionManager.IsNodeSelected(NodeWidgetUnderMouse->GetObjectBeingDisplayed()))
			{
				SelectionManager.SelectSingleNode(NodeWidgetUnderMouse->GetObjectBeingDisplayed());
			}
		}
	}
	else
	{
		SelectionManager.ClearSelectionSet();
	}
	
	// Summon context menu
	FMenuBuilder MenuBuilder(true, WorldModel->GetCommandList());
	WorldModel->BuildGridMenu(MenuBuilder);
	TSharedPtr<SWidget> MenuWidget = MenuBuilder.MakeWidget();

	FSlateApplication::Get().PushMenu(
		AsShared(),
		MenuWidget.ToSharedRef(),
		MouseEvent.GetScreenSpacePosition(),
		FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu )
		);

	return MenuWidget;
}

void SWorldLevelsGridView::OnSelectionChanged(const FGraphPanelSelectionSet& SelectedNodes)
{
	if (bUpdatingSelection)
	{
		return;
	}
	
	bUpdatingSelection = true;
	FLevelModelList SelectedLevels;
	
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		TSharedRef<SNode>* pWidget = NodeToWidgetLookup.Find(*NodeIt);
		if (pWidget != NULL)
		{
			TSharedRef<SWorldGridItem> Item = StaticCastSharedRef<SWorldGridItem>(*pWidget);
			SelectedLevels.Add(Item->GetLevelModel());
		}
	}
	
	WorldModel->SetSelectedLevels(SelectedLevels);
	bUpdatingSelection = false;
}

void SWorldLevelsGridView::PopulateVisibleChildren(const FGeometry& AllottedGeometry)
{
	VisibleChildren.Empty();

	FSlateRect PanelRect = AllottedGeometry.GetRect();
	FVector2D ViewStartPos = PanelCoordToGraphCoord(FVector2D(PanelRect.Left, PanelRect.Top));
	FVector2D ViewEndPos = PanelCoordToGraphCoord(FVector2D(PanelRect.Right, PanelRect.Bottom));
	FSlateRect ViewRect(ViewStartPos, ViewEndPos);

	for (int32 ChildIndex=0; ChildIndex < Children.Num(); ++ChildIndex)
	{
		const auto Child = StaticCastSharedRef<SWorldGridItem>(Children[ChildIndex]);
		
		if (WorldModel->PassesAllFilters(Child->GetLevelModel()))
		{
			FSlateRect ChildRect = Child->GetItemRect();
			FVector2D ChildSize = ChildRect.GetSize();

			if (ChildSize.X > 0.f && 
				ChildSize.Y > 0.f && 
				FSlateRect::DoRectanglesIntersect(ChildRect, ViewRect))
			{
				VisibleChildren.Add(Child);
			}
		}
	}

	// Sort tiles such that smaller and selected tiles will be drawn on top of other tiles
	struct FVisibleTilesSorter
	{
		FVisibleTilesSorter(const FLevelCollectionModel& InWorldModel) : WorldModel(InWorldModel)
		{}
		bool operator()(const TSharedRef<SNodePanel::SNode>& A,
						const TSharedRef<SNodePanel::SNode>& B) const
		{
			TSharedRef<SWorldGridItem> ItemA = StaticCastSharedRef<SWorldGridItem>(A);
			TSharedRef<SWorldGridItem> ItemB = StaticCastSharedRef<SWorldGridItem>(B);
			return WorldModel.CompareLevelsZOrder(ItemA->GetLevelModel(), ItemB->GetLevelModel());
		}
		const FLevelCollectionModel& WorldModel;
	};

	VisibleChildren.Sort(FVisibleTilesSorter(*WorldModel.Get()));
}

void SWorldLevelsGridView::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	for (int32 ChildIndex=0; ChildIndex < VisibleChildren.Num(); ++ChildIndex)
	{
		const auto Child = StaticCastSharedRef<SWorldGridItem>(VisibleChildren[ChildIndex]);
		const EVisibility ChildVisibility = Child->GetVisibility();

		if (ArrangedChildren.Accepts(ChildVisibility))
		{
			FVector2D ChildPos = Child->GetPosition();
					
			ArrangedChildren.AddWidget(ChildVisibility,
				AllottedGeometry.MakeChild(Child,
				ChildPos - GetViewOffset(),
				Child->GetDesiredSize(), GetZoomAmount()
				));
		}
	}
}

uint32 SWorldLevelsGridView::PaintBackground(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, uint32 LayerId) const
{
	FVector2D ScreenWorldOrigin = GraphCoordToPanelCoord(FVector2D(0, 0));
	FSlateRect ScreenRect = AllottedGeometry.GetRect();
	
	// Vertical line
	if (ScreenWorldOrigin.X > ScreenRect.Left &&
		ScreenWorldOrigin.X < ScreenRect.Right)
	{
		TArray<FVector2D> LinePoints;
		LinePoints.Add(FVector2D(ScreenWorldOrigin.X, ScreenRect.Top));
		LinePoints.Add(FVector2D(ScreenWorldOrigin.X, ScreenRect.Bottom));
		
		FSlateDrawElement::MakeLines( 
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			MyClippingRect,
			ESlateDrawEffect::None,
			FColor(100, 100, 100));
	}

	// Horizontal line
	if (ScreenWorldOrigin.Y > ScreenRect.Top &&
		ScreenWorldOrigin.Y < ScreenRect.Bottom)
	{
		TArray<FVector2D> LinePoints;
		LinePoints.Add(FVector2D(ScreenRect.Left, ScreenWorldOrigin.Y));
		LinePoints.Add(FVector2D(ScreenRect.Right, ScreenWorldOrigin.Y));
		
		FSlateDrawElement::MakeLines( 
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			MyClippingRect,
			ESlateDrawEffect::None,
			FColor(100, 100, 100));
	}

	return LayerId + 1;
}

int32 SWorldLevelsGridView::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, 
							FSlateWindowElementList& OutDrawElements, int32 LayerId, 
							const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// First paint the background
	{
		LayerId = PaintBackground(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId);
		LayerId++;
	}

	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	ArrangeChildren(AllottedGeometry, ArrangedChildren);

	// Draw the child nodes

	// When drawing a marquee, need a preview of what the selection will be.
	const auto* SelectionToVisualize = &(SelectionManager.SelectedNodes);
	FGraphPanelSelectionSet SelectionPreview;
	if (Marquee.IsValid())
	{			
		ApplyMarqueeSelection(Marquee, SelectionManager.SelectedNodes, SelectionPreview);
		SelectionToVisualize = &SelectionPreview;
	}
	
	int32 NodesLayerId = LayerId;

	for (int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
	{
		FArrangedWidget& CurWidget = ArrangedChildren(ChildIndex);
		TSharedRef<SWorldGridItem> ChildNode = StaticCastSharedRef<SWorldGridItem>(CurWidget.Widget);
		
		ChildNode->bAffectedByMarquee = SelectionToVisualize->Contains(ChildNode->GetObjectBeingDisplayed());
		LayerId = CurWidget.Widget->OnPaint(CurWidget.Geometry, MyClippingRect, OutDrawElements, NodesLayerId, InWidgetStyle, ShouldBeEnabled(bParentEnabled));
		ChildNode->bAffectedByMarquee = false;
	}
		
	// Draw editable world bounds
	if (!WorldModel->IsSimulating())
	{
		float ScreenSpaceSize = FLevelCollectionModel::EditableAxisLength()*GetZoomAmount()*2.f;
		FVector2D PaintSize = FVector2D(ScreenSpaceSize, ScreenSpaceSize);
		FVector2D PaintPosition = GraphCoordToPanelCoord(FVector2D::ZeroVector) - (PaintSize*0.5f);
		FPaintGeometry EditableArea = AllottedGeometry.ToPaintGeometry(PaintPosition, PaintSize);
		EditableArea.DrawScale = 0.2f;
		FLinearColor PaintColor = FLinearColor::Red;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			++LayerId,
			EditableArea,
			FEditorStyle::GetBrush(TEXT("Graph.CompactNode.ShadowSelected")),
			MyClippingRect,
			ESlateDrawEffect::None,
			PaintColor
			);
	}
		
	// Draw the marquee selection rectangle
	PaintMarquee(AllottedGeometry, MyClippingRect, OutDrawElements, ++LayerId);

	// Draw the software cursor
	PaintSoftwareCursor(AllottedGeometry, MyClippingRect, OutDrawElements, ++LayerId);

	if(WorldModel->IsSimulating())
	{
		// Draw a surrounding indicator when PIE is active, to make it clear that the graph is read-only, etc...
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FEditorStyle::GetBrush(TEXT("Graph.PlayInEditor")),
			MyClippingRect
			);

		// Draw a current camera position
		{
			const FSlateBrush* CameraImage = FEditorStyle::GetBrush(TEXT("WorldBrowser.SimulationViewPositon"));
			FVector ObserverPosition = WorldModel->GetObserverPosition();
			FVector2D ObserverPositionScreen = GraphCoordToPanelCoord(FVector2D(ObserverPosition.X, ObserverPosition.Y));

			FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry(
				ObserverPositionScreen - CameraImage->ImageSize*0.5f, 
				CameraImage->ImageSize
				);
						
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				++LayerId,
				PaintGeometry,
				CameraImage,
				MyClippingRect
			);
		}
	}

	return LayerId;
}

void SWorldLevelsGridView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SNodePanel::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// scroll to world center on first open
	if (bIsFirstTickCall)
	{
		bIsFirstTickCall = false;
		ViewOffset-= AllottedGeometry.Size*0.5f/GetZoomAmount();
	}

	FVector2D CursorPosition = FSlateApplication::Get().GetCursorPos();

	// Update cached variables
	WorldMouseLocation = CursorToWorldPosition(AllottedGeometry, CursorPosition);
	WorldMarqueeSize = Marquee.Rect.GetSize()/AllottedGeometry.Scale;
			
	// Update streaming preview data
	const bool bShowPotentiallyVisibleLevels = FSlateApplication::Get().GetModifierKeys().IsAltDown() && 
												AllottedGeometry.IsUnderLocation(CursorPosition);
	
	//WorldModel->UpdateStreamingPreview(WorldMouseLocation, bShowPotentiallyVisibleLevels);
			
	// deffered scroll and zooming requests
	if (bHasScrollRequest)
	{
		bHasScrollRequest = false;

		// zoom to
		FVector2D SizeWithZoom = RequestedZoomArea*ZoomLevels->GetZoomAmount(ZoomLevel);
		if (RequestedAllowZoomIn || 
			(SizeWithZoom.X >= AllottedGeometry.Size.X || SizeWithZoom.Y >= AllottedGeometry.Size.Y))
		{
			// maximum zoom out by default
			ZoomLevel = ZoomLevels->GetDefaultZoomLevel();
			// expand zoom area little bit, so zooming will fit original area not so tight
			RequestedZoomArea*= 1.2f;
			// find more suitable zoom value
			for (int32 Zoom = 0; Zoom < ZoomLevels->GetDefaultZoomLevel(); ++Zoom)
			{
				SizeWithZoom = RequestedZoomArea*ZoomLevels->GetZoomAmount(Zoom);
				if (SizeWithZoom.X >= AllottedGeometry.Size.X || SizeWithZoom.Y >= AllottedGeometry.Size.Y)
				{
					ZoomLevel = Zoom;
					break;
				}
			}
		}

		// scroll to
		ViewOffset = RequestedScrollLocation - AllottedGeometry.Size*0.5f/GetZoomAmount();
	}
}

// The system calls this method to notify the widget that a mouse moved within it. This event is bubbled.
FReply SWorldLevelsGridView::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const bool bIsRightMouseButtonDown = MouseEvent.IsMouseButtonDown( EKeys::RightMouseButton );
	const bool bIsLeftMouseButtonDown = MouseEvent.IsMouseButtonDown( EKeys::LeftMouseButton );

	PastePosition = PanelCoordToGraphCoord( MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ) );

	if ( this->HasMouseCapture() )
	{
		const FVector2D CursorDelta = MouseEvent.GetCursorDelta();
		// Track how much the mouse moved since the mouse down.
		TotalMouseDelta += CursorDelta.Size();

		if (bIsRightMouseButtonDown)
		{
			FReply ReplyState = FReply::Handled();

			if( !CursorDelta.IsZero() )
			{
				bShowSoftwareCursor = true;
			}

			// Panning and mouse is outside of panel? Pasting should just go to the screen center.
			PastePosition = PanelCoordToGraphCoord( 0.5 * MyGeometry.Size );

			this->bIsPanning = true;
			ViewOffset -= CursorDelta / GetZoomAmount();

			return ReplyState;
		}
		else if (bIsLeftMouseButtonDown)
		{
			TSharedPtr<SNode> NodeBeingDragged = NodeUnderMousePtr.Pin();

			if ( IsEditable.Get() )
			{
				// Update the amount to pan panel
				UpdateViewOffset(MyGeometry, MouseEvent.GetScreenSpacePosition());

				const bool bCursorInDeadZone = TotalMouseDelta <= SlatePanTriggerDistance;

				if ( NodeBeingDragged.IsValid() )
				{
					if ( !bCursorInDeadZone )
					{
						// Note, NodeGrabOffset() comes from the node itself, so it's already scaled correctly.
						FVector2D AnchorNodeNewPos = PanelCoordToGraphCoord( MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ) ) - NodeGrabOffset;

						// Dragging an unselected node automatically selects it.
						SelectionManager.StartDraggingNode(NodeBeingDragged->GetObjectBeingDisplayed(), MouseEvent);

						// Move all the selected nodes.
						{
							const FVector2D AnchorNodeOldPos = NodeBeingDragged->GetPosition();
							const FVector2D DeltaPos = AnchorNodeNewPos - AnchorNodeOldPos;
							if (DeltaPos.Size() > KINDA_SMALL_NUMBER)
							{
								MoveSelectedNodes(NodeBeingDragged, AnchorNodeNewPos);
							}
						}
					}

					return FReply::Handled();
				}
			}

			if ( !NodeBeingDragged.IsValid() )
			{
				// We are marquee selecting
				const FVector2D GraphMousePos = PanelCoordToGraphCoord( MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ) );
				Marquee.Rect.UpdateEndPoint(GraphMousePos);

				FindNodesAffectedByMarquee( /*out*/ Marquee.AffectedNodes );
				return FReply::Handled();
			}
		}
	}

	return FReply::Unhandled();
}


void SWorldLevelsGridView::OnBeginNodeInteraction(const TSharedRef<SNode>& InNodeToDrag, const FVector2D& GrabOffset)
{
	bHasNodeInteraction = true;
	SNodePanel::OnBeginNodeInteraction(InNodeToDrag, GrabOffset);
}

void SWorldLevelsGridView::OnEndNodeInteraction(const TSharedRef<SNode>& InNodeDragged)
{
	const SWorldGridItem& Item = static_cast<const SWorldGridItem&>(InNodeDragged.Get());
	if (Item.IsItemEditable())
	{
		FVector2D AbsoluteDelta = Item.GetLevelModel()->GetLevelTranslationDelta();
		FIntPoint IntAbsoluteDelta = FIntPoint(AbsoluteDelta.X, AbsoluteDelta.Y);

		// Reset stored translation delta to 0
		WorldModel->UpdateTranslationDelta(WorldModel->GetSelectedLevels(), FVector2D::ZeroVector, 0.f);
	
		// In case we have non zero dragging delta, translate selected levels 
		if (IntAbsoluteDelta != FIntPoint::ZeroValue)
		{
			WorldModel->TranslateLevels(WorldModel->GetSelectedLevels(), IntAbsoluteDelta);
		}
	}
	
	bHasNodeInteraction = false;

	SNodePanel::OnEndNodeInteraction(InNodeDragged);
}

void SWorldLevelsGridView::MoveSelectedNodes(const TSharedPtr<SNode>& InNodeToDrag, FVector2D NewPosition)
{
	auto ItemDragged = StaticCastSharedPtr<SWorldGridItem>(InNodeToDrag);
	
	if (ItemDragged->IsItemEditable())
	{
		bool bEnableSnapping = (FSlateApplication::Get().GetModifierKeys().IsControlDown() == false);
		float SnappingDistanceWorld = (bEnableSnapping ? SnappingDistance/GetZoomAmount() : 0.f);
		
		FVector2D StartPosition = ItemDragged->GetPosition() - ItemDragged->GetLevelModel()->GetLevelTranslationDelta();
		FVector2D AbsoluteDelta = NewPosition - StartPosition;

		WorldModel->UpdateTranslationDelta(WorldModel->GetSelectedLevels(), AbsoluteDelta, SnappingDistanceWorld);
	}
}

bool SWorldLevelsGridView::OnHandleLeftMouseRelease(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return SNodePanel::OnHandleLeftMouseRelease(MyGeometry, MouseEvent);
}

FReply SWorldLevelsGridView::OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	if (CommandList->ProcessCommandBindings(InKeyboardEvent))
	{
		return FReply::Handled();
	}
	
	if (WorldModel->GetCommandList()->ProcessCommandBindings(InKeyboardEvent))
	{
		return FReply::Handled();
	}

	return SNodePanel::OnKeyDown(MyGeometry, InKeyboardEvent);
}

bool SWorldLevelsGridView::SupportsKeyboardFocus() const
{
	return true;
}

void SWorldLevelsGridView::MoveLevelLeft_Executed()
{
	if (!bHasNodeInteraction)
	{
		WorldModel->TranslateLevels(WorldModel->GetSelectedLevels(), FIntPoint(-1, 0));
	}
}

void SWorldLevelsGridView::MoveLevelRight_Executed()
{
	if (!bHasNodeInteraction)
	{
		WorldModel->TranslateLevels(WorldModel->GetSelectedLevels(), FIntPoint(+1, 0));
	}
}

void SWorldLevelsGridView::MoveLevelUp_Executed()
{
	if (!bHasNodeInteraction)
	{
		WorldModel->TranslateLevels(WorldModel->GetSelectedLevels(), FIntPoint(0, -1));
	}
}

void SWorldLevelsGridView::MoveLevelDown_Executed()
{
	if (!bHasNodeInteraction)
	{
		WorldModel->TranslateLevels(WorldModel->GetSelectedLevels(), FIntPoint(0, +1));
	}
}

#undef LOCTEXT_NAMESPACE
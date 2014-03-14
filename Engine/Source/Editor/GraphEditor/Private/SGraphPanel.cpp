// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GraphEditorCommon.h"
#include "NodeFactory.h"

#include "Editor/UnrealEd/Public/DragAndDrop/ActorDragDropGraphEdOp.h"
#include "Editor/UnrealEd/Public/DragAndDrop/AssetDragDropOp.h"
#include "Editor/UnrealEd/Public/DragAndDrop/LevelDragDropOp.h"
#include "ConnectionDrawingPolicy.h"

#include "AssetSelection.h"
#include "ComponentAssetBroker.h"

#include "KismetNodes/KismetNodeInfoContext.h"
#include "GraphDiffControl.h"

DEFINE_LOG_CATEGORY_STATIC(LogGraphPanel, Log, All);

/**
 * Construct a widget
 *
 * @param InArgs    The declaration describing how the widgets should be constructed.
 */
void SGraphPanel::Construct( const SGraphPanel::FArguments& InArgs )
{
	SNodePanel::Construct();

	this->OnGetContextMenuFor = InArgs._OnGetContextMenuFor;
	this->GraphObj = InArgs._GraphObj;
	this->GraphObjToDiff = InArgs._GraphObjToDiff;
	this->SelectionManager.OnSelectionChanged = InArgs._OnSelectionChanged;
	this->IsEditable = InArgs._IsEditable;
	this->OnNodeDoubleClicked = InArgs._OnNodeDoubleClicked;
	this->OnDropActor = InArgs._OnDropActor;
	this->OnDropStreamingLevel = InArgs._OnDropStreamingLevel;
	this->OnVerifyTextCommit = InArgs._OnVerifyTextCommit;
	this->OnTextCommitted = InArgs._OnTextCommitted;
	this->OnSpawnNodeByShortcut = InArgs._OnSpawnNodeByShortcut;
	this->OnUpdateGraphPanel = InArgs._OnUpdateGraphPanel;
	this->OnDisallowedPinConnection = InArgs._OnDisallowedPinConnection;

	this->bPreservePinPreviewConnection = false;
	this->PinVisibility = SGraphEditor::Pin_Show;

	CachedAllottedGeometryScaledSize = FVector2D(160, 120);
	if (InArgs._InitialZoomToFit)
	{
		ZoomToFit(/*bOnlySelection=*/ false);
		bTeleportInsteadOfScrollingWhenZoomingToFit = true;
	}

	BounceCurve.AddCurve(0.0f, 1.0f);
	BounceCurve.Play();

	// Register for notifications
	MyRegisteredGraphChangedDelegate = FOnGraphChanged::FDelegate::CreateSP(this, &SGraphPanel::OnGraphChanged);
	this->GraphObj->AddOnGraphChangedHandler(MyRegisteredGraphChangedDelegate);

	bShowPIENotification = InArgs._ShowPIENotification;
}

SGraphPanel::~SGraphPanel()
{
	this->GraphObj->RemoveOnGraphChangedHandler(MyRegisteredGraphChangedDelegate);
}

//////////////////////////////////////////////////////////////////////////

/**
 * The widget should respond by populating the OutDrawElements array with FDrawElements 
 * that represent it and any of its children.
 *
 * @param AllottedGeometry  The FGeometry that describes an area in which the widget should appear.
 * @param MyClippingRect    The clipping rectangle allocated for this widget and its children.
 * @param OutDrawElements   A list of FDrawElements to populate with the output.
 * @param LayerId           The Layer onto which this widget should be rendered.
 * @param InColorAndOpacity Color and Opacity to be applied to all the descendants of the widget being painted
 * @param bParentEnabled	True if the parent of this widget is enabled.
 *
 * @return The maximum layer ID attained by this widget or any of its children.
 */

int32 SGraphPanel::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
#if SLATE_HD_STATS
	SCOPE_CYCLE_COUNTER( STAT_SlateOnPaint_SGraphPanel );
#endif

	CachedAllottedGeometryScaledSize = AllottedGeometry.Size * AllottedGeometry.Scale;

	//Style used for objects that are the same between revisions
	FWidgetStyle FadedStyle = InWidgetStyle;
	FadedStyle.BlendColorAndOpacityTint(FLinearColor(0.45f,0.45f,0.45f,0.45f));

	// First paint the background
	const UEditorExperimentalSettings& Options = *GetDefault<UEditorExperimentalSettings>();

	const FSlateBrush* BackgroundImage = FEditorStyle::GetBrush(TEXT("Graph.Panel.SolidBackground"));
	PaintBackgroundAsLines(BackgroundImage, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId);

	const float ZoomFactor = AllottedGeometry.Scale * GetZoomAmount();

	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	this->ArrangeChildren(AllottedGeometry, ArrangedChildren);

	// Determine some 'global' settings based on current LOD
	const bool bDrawScaledCommentBubblesThisFrame = GetCurrentLOD() > EGraphRenderingLOD::LowestDetail;
	const bool bDrawUnscaledCommentBubblesThisFrame = GetCurrentLOD() <= EGraphRenderingLOD::MediumDetail;
	const bool bDrawShadowsThisFrame = GetCurrentLOD() > EGraphRenderingLOD::LowestDetail;

	// Because we paint multiple children, we must track the maximum layer id that they produced in case one of our parents
	// wants to an overlay for all of its contents.

	// Save LayerId for comment boxes to ensure they always appear below nodes & wires
	const int32 CommentNodeShadowLayerId = LayerId++;
	const int32 CommentNodeLayerId = LayerId++;

	// Save a LayerId for wires, which appear below nodes but above comments
	// We will draw them later, along with the arrows which appear above nodes.
	const int32 WireLayerId = LayerId++;

	const int32 NodeShadowsLayerId = LayerId;
	const int32 NodeLayerId = NodeShadowsLayerId + 1;
	int32 MaxLayerId = NodeLayerId;

	const FVector2D NodeShadowSize = FEditorStyle::GetVector(TEXT("Graph.Node.ShadowSize"));
	const UEdGraphSchema* Schema = GraphObj->GetSchema();

	// Draw the child nodes
	{
		// When drawing a marquee, need a preview of what the selection will be.
		const FGraphPanelSelectionSet* SelectionToVisualize = &(SelectionManager.SelectedNodes);
		FGraphPanelSelectionSet SelectionPreview;
		if ( Marquee.IsValid() )
		{			
			ApplyMarqueeSelection(Marquee, SelectionManager.SelectedNodes, SelectionPreview);
			SelectionToVisualize = &SelectionPreview;
		}

		// Context for rendering node infos
		FKismetNodeInfoContext Context(GraphObj);

		TArray<FGraphDiffControl::FNodeMatch> NodeMatches;
		for (int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
		{
			FArrangedWidget& CurWidget = ArrangedChildren(ChildIndex);
			TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(CurWidget.Widget);
			
			// Examine node to see what layers we should be drawing in
			int32 ShadowLayerId = NodeShadowsLayerId;
			int32 ChildLayerId = NodeLayerId;

			// If a comment node, draw in the dedicated comment slots
			{
				UObject* NodeObj = ChildNode->GetObjectBeingDisplayed();
				if (NodeObj && NodeObj->IsA(UEdGraphNode_Comment::StaticClass()))
				{
					ShadowLayerId = CommentNodeShadowLayerId;
					ChildLayerId = CommentNodeLayerId;
				}
			}


			const bool bNodeIsVisible = FSlateRect::DoRectanglesIntersect( CurWidget.Geometry.GetClippingRect(), MyClippingRect );

			if (bNodeIsVisible)
			{
				const bool bSelected = SelectionToVisualize->Contains( StaticCastSharedRef<SNodePanel::SNode>(CurWidget.Widget)->GetObjectBeingDisplayed() );

				// Handle Node renaming once the node is visible
				if( bSelected && ChildNode->IsRenamePending() )
				{
					ChildNode->ApplyRename();
				}

				// Draw the node's shadow.
				if (bDrawShadowsThisFrame || bSelected)
				{
					const FSlateBrush* ShadowBrush = ChildNode->GetShadowBrush(bSelected);
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						ShadowLayerId,
						CurWidget.Geometry.ToInflatedPaintGeometry(NodeShadowSize),
						ShadowBrush,
						MyClippingRect
						);
				}

				// Draw the comments and information popups for this node, if it has any.
				{
					float CommentBubbleY = 0.0f;

					const FString NodeComment = ChildNode->GetNodeComment();
					if (!NodeComment.IsEmpty())
					{
						// Comment bubbles have different LOD behavior:
						//   A comment box comment (bScaleComments=false) will only be shown when zoomed out (the title bar is readable instead when up close)
						//   A per-node comment (bScaleComments=true) will only be show when zoomed in (it gets too small to read)
						const bool bScaleComments = ChildNode->ShouldScaleNodeComment();
						const bool bShowCommentBubble = bScaleComments ? bDrawScaledCommentBubblesThisFrame : bDrawUnscaledCommentBubblesThisFrame;

						if (bShowCommentBubble)
						{
							FGeometry CommentGeometry = CurWidget.Geometry;
							if (!bScaleComments)
							{
								CommentGeometry.Scale = 1.0f;
							}
							PaintComment(NodeComment, CommentGeometry, MyClippingRect, OutDrawElements, ChildLayerId, ChildNode->GetNodeCommentColor().GetColor( InWidgetStyle ), /*inout*/ CommentBubbleY, InWidgetStyle);
						}
					}

					Context.bSelected = bSelected;
					TArray<FGraphInformationPopupInfo> Popups;

					{
						ChildNode->GetNodeInfoPopups(&Context, /*out*/ Popups);
					}

					for (int32 PopupIndex = 0; PopupIndex < Popups.Num(); ++PopupIndex)
					{
						FGraphInformationPopupInfo& Popup = Popups[PopupIndex];
						PaintComment(Popup.Message, CurWidget.Geometry, MyClippingRect, OutDrawElements, ChildLayerId, Popup.BackgroundColor, /*inout*/ CommentBubbleY, InWidgetStyle);
					}
				}

				int32 CurWidgetsMaxLayerId;
				{
					UEdGraphNode* NodeObj = Cast<UEdGraphNode>(ChildNode->GetObjectBeingDisplayed());

					/** When diffing nodes, nodes that are different between revisions are opaque, nodes that have not changed are faded */
					FGraphDiffControl::FNodeMatch NodeMatch = FGraphDiffControl::FindNodeMatch(GraphObjToDiff, NodeObj, NodeMatches);
					if (NodeMatch.IsValid())
					{
						NodeMatches.Add(NodeMatch);
					}
					const bool bNodeIsDifferent = (!GraphObjToDiff || NodeMatch.Diff());

					/* When dragging off a pin, we want to duck the alpha of some nodes */
					TSharedPtr< SGraphPin > OnlyStartPin = (1 == PreviewConnectorFromPins.Num()) ? PreviewConnectorFromPins[0] : TSharedPtr< SGraphPin >();
					const bool bNodeIsNotUsableInCurrentContext = Schema->FadeNodeWhenDraggingOffPin(NodeObj, OnlyStartPin.IsValid() ? OnlyStartPin.Get()->GetPinObj() : NULL);
					const FWidgetStyle& NodeStyleToUse = (bNodeIsDifferent && !bNodeIsNotUsableInCurrentContext)? InWidgetStyle : FadedStyle;

					// Draw the node.O
					CurWidgetsMaxLayerId = CurWidget.Widget->OnPaint( CurWidget.Geometry, MyClippingRect, OutDrawElements, ChildLayerId, NodeStyleToUse, ShouldBeEnabled( bParentEnabled ) );
				}

				// Draw the node's overlay, if it has one.
				{
					// Get its size
					const FVector2D WidgetSize = CurWidget.Geometry.Size;

					TArray<FOverlayBrushInfo> OverlayBrushes;
					ChildNode->GetOverlayBrushes(bSelected, WidgetSize, /*out*/ OverlayBrushes);

					for (int32 BrushIndex = 0; BrushIndex < OverlayBrushes.Num(); ++BrushIndex)
					{
						FOverlayBrushInfo& OverlayInfo = OverlayBrushes[BrushIndex];
						const FSlateBrush* OverlayBrush = OverlayInfo.Brush;
						if(OverlayBrush != NULL)
						{
							FPaintGeometry BouncedGeometry = CurWidget.Geometry.ToPaintGeometry(OverlayInfo.OverlayOffset, OverlayBrush->ImageSize, 1.f);

							// Handle bouncing
							const float BounceValue = FMath::Sin(2.0f * PI * BounceCurve.GetLerpLooping());
							BouncedGeometry.DrawPosition += (OverlayInfo.AnimationEnvelope * BounceValue * ZoomFactor);

							CurWidgetsMaxLayerId++;
							FSlateDrawElement::MakeBox(
								OutDrawElements,
								CurWidgetsMaxLayerId,
								BouncedGeometry,
								OverlayBrush,
								MyClippingRect
								);
						}

					}
				}

				MaxLayerId = FMath::Max( MaxLayerId, CurWidgetsMaxLayerId + 1 );
			}
		}
	}

	MaxLayerId += 1;


	// Draw connections between pins 
	if (Children.Num() > 0 )
	{

		//@TODO: Pull this into a factory like the pin and node ones
		FConnectionDrawingPolicy* ConnectionDrawingPolicy;
		{
			ConnectionDrawingPolicy = Schema->CreateConnectionDrawingPolicy(WireLayerId, MaxLayerId, ZoomFactor, MyClippingRect, OutDrawElements, GraphObj);
			if (!ConnectionDrawingPolicy)
			{
				if (Schema->IsA(UAnimationGraphSchema::StaticClass()))
				{
					ConnectionDrawingPolicy = new FAnimGraphConnectionDrawingPolicy(WireLayerId, MaxLayerId, ZoomFactor, MyClippingRect, OutDrawElements, GraphObj);
				}
				else if (Schema->IsA(UAnimationStateMachineSchema::StaticClass()))
				{
					ConnectionDrawingPolicy = new FStateMachineConnectionDrawingPolicy(WireLayerId, MaxLayerId, ZoomFactor, MyClippingRect, OutDrawElements, GraphObj);
				}
				else if (Schema->IsA(UEdGraphSchema_K2::StaticClass()))
				{
					ConnectionDrawingPolicy = new FKismetConnectionDrawingPolicy(WireLayerId, MaxLayerId, ZoomFactor, MyClippingRect, OutDrawElements, GraphObj);
				}
				else if (Schema->IsA(USoundCueGraphSchema::StaticClass()))
				{
					ConnectionDrawingPolicy = new FSoundCueGraphConnectionDrawingPolicy(WireLayerId, MaxLayerId, ZoomFactor, MyClippingRect, OutDrawElements, GraphObj);
				}
				else if (Schema->IsA(UMaterialGraphSchema::StaticClass()))
				{
					ConnectionDrawingPolicy = new FMaterialGraphConnectionDrawingPolicy(WireLayerId, MaxLayerId, ZoomFactor, MyClippingRect, OutDrawElements, GraphObj);
				}
				else
				{
					ConnectionDrawingPolicy = new FConnectionDrawingPolicy(WireLayerId, MaxLayerId, ZoomFactor, MyClippingRect, OutDrawElements);
				}
			}
		}
		ConnectionDrawingPolicy->SetHoveredPins(CurrentHoveredPins, PreviewConnectorFromPins, TimeSinceMouseEnteredPin);
		ConnectionDrawingPolicy->SetMarkedPin(MarkedPin);

		// Get the set of pins for all children and synthesize geometry for culled out pins so lines can be drawn to them.
		TMap<TSharedRef<SWidget>, FArrangedWidget> PinGeometries;
		TSet< TSharedRef<SWidget> > VisiblePins;
		for (int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
		{
			TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(Children[ChildIndex]);

			// If this is a culled node, approximate the pin geometry to the corner of the node it is within
			if (IsNodeCulled(ChildNode, AllottedGeometry))
			{
				TArray< TSharedRef<SWidget> > NodePins;
				ChildNode->GetPins(NodePins);

				const FVector2D NodeLoc = ChildNode->GetPosition();
				const FGeometry SynthesizedNodeGeometry(GraphCoordToPanelCoord(NodeLoc), AllottedGeometry.AbsolutePosition, FVector2D::ZeroVector, 1.f);

				for (TArray< TSharedRef<SWidget> >::TConstIterator NodePinIterator(NodePins); NodePinIterator; ++NodePinIterator)
				{
					const SGraphPin& PinWidget = static_cast<const SGraphPin&>((*NodePinIterator).Get());
					FVector2D PinLoc = NodeLoc + PinWidget.GetNodeOffset();

					const FGeometry SynthesizedPinGeometry(GraphCoordToPanelCoord(PinLoc), AllottedGeometry.AbsolutePosition, FVector2D::ZeroVector, 1.f);
					PinGeometries.Add(*NodePinIterator, FArrangedWidget(*NodePinIterator, SynthesizedPinGeometry));
				}

				// Also add synthesized geometries for culled nodes
				ArrangedChildren.AddWidget( FArrangedWidget(ChildNode, SynthesizedNodeGeometry) );
			}
			else
			{
				ChildNode->GetPins(VisiblePins);
			}
		}

		// Now get the pin geometry for all visible children and append it to the PinGeometries map
		TMap<TSharedRef<SWidget>, FArrangedWidget> VisiblePinGeometries;
		{
			this->FindChildGeometries(AllottedGeometry, VisiblePins, VisiblePinGeometries);
			PinGeometries.Append(VisiblePinGeometries);
		}

		// Draw preview connections (only connected on one end)
		if (PreviewConnectorFromPins.Num() > 0)
		{
			for (TArray< TSharedPtr<SGraphPin> >::TConstIterator StartPinIterator(PreviewConnectorFromPins); StartPinIterator; ++StartPinIterator)
			{
				TSharedPtr< SGraphPin > CurrentStartPin = *StartPinIterator;
				const FArrangedWidget* PinGeometry = PinGeometries.Find( CurrentStartPin.ToSharedRef() );

				if (PinGeometry != NULL)
				{
					FVector2D StartPoint;
					FVector2D EndPoint;

					if (CurrentStartPin->GetDirection() == EGPD_Input)
					{
						StartPoint = AllottedGeometry.AbsolutePosition + PreviewConnectorEndpoint;
						EndPoint = FGeometryHelper::VerticalMiddleLeftOf( PinGeometry->Geometry ) - FVector2D(ConnectionDrawingPolicy->ArrowRadius.X, 0);
					}
					else
					{
						StartPoint = FGeometryHelper::VerticalMiddleRightOf( PinGeometry->Geometry );
						EndPoint = AllottedGeometry.AbsolutePosition + PreviewConnectorEndpoint;
					}

					ConnectionDrawingPolicy->DrawPreviewConnector(PinGeometry->Geometry, StartPoint, EndPoint, CurrentStartPin.Get()->GetPinObj());
				}

				//@TODO: Re-evaluate this incompatible mojo; it's mutating every pin state every frame to accomplish a visual effect
				ConnectionDrawingPolicy->SetIncompatiblePinDrawState(CurrentStartPin, VisiblePins);
			}
		}
		else
		{
			//@TODO: Re-evaluate this incompatible mojo; it's mutating every pin state every frame to accomplish a visual effect
			ConnectionDrawingPolicy->ResetIncompatiblePinDrawState(VisiblePins);
		}

		// Draw all regular connections
		ConnectionDrawingPolicy->Draw(PinGeometries, ArrangedChildren);

		delete ConnectionDrawingPolicy;
	}

	// Draw a shadow overlay around the edges of the graph
	++MaxLayerId;
	PaintSurroundSunkenShadow(FEditorStyle::GetBrush(TEXT("Graph.Shadow")), AllottedGeometry, MyClippingRect, OutDrawElements, MaxLayerId);

	// Draw a surrounding indicator when PIE is active, to make it clear that the graph is read-only, etc...
	if (bShowPIENotification && (GEditor->bIsSimulatingInEditor || GEditor->PlayWorld != NULL))
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			MaxLayerId,
			AllottedGeometry.ToPaintGeometry(),
			FEditorStyle::GetBrush(TEXT("Graph.PlayInEditor")),
			MyClippingRect
			);
	}

	// Draw the marquee selection rectangle
	PaintMarquee(AllottedGeometry, MyClippingRect, OutDrawElements, MaxLayerId);

	// Draw the software cursor
	++MaxLayerId;
	PaintSoftwareCursor(AllottedGeometry, MyClippingRect, OutDrawElements, MaxLayerId);

	return MaxLayerId;
}

bool SGraphPanel::SupportsKeyboardFocus() const
{
	return true;
}

void SGraphPanel::UpdateSelectedNodesPositions (FVector2D PositionIncrement)
{
	for (FGraphPanelSelectionSet::TIterator NodeIt(SelectionManager.SelectedNodes); NodeIt; ++NodeIt)
	{
		TSharedRef<SNode>* pWidget = NodeToWidgetLookup.Find(*NodeIt);
		if (pWidget != NULL)
		{
			SNode& Widget = pWidget->Get();
			Widget.MoveTo(Widget.GetPosition() + PositionIncrement);
		}
	}
}

FReply SGraphPanel::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	if( IsEditable.Get() )
	{
		if( InKeyboardEvent.GetKey() == EKeys::Up  ||InKeyboardEvent.GetKey() ==  EKeys::NumPadEight )
		{
			UpdateSelectedNodesPositions(FVector2D(0.0f,-GetSnapGridSize()));
			return FReply::Handled();
		}
		if( InKeyboardEvent.GetKey() ==  EKeys::Down || InKeyboardEvent.GetKey() ==  EKeys::NumPadTwo )
		{
			UpdateSelectedNodesPositions(FVector2D(0.0f,GetSnapGridSize()));
			return FReply::Handled();
		}
		if( InKeyboardEvent.GetKey() ==  EKeys::Right || InKeyboardEvent.GetKey() ==  EKeys::NumPadSix )
		{
			UpdateSelectedNodesPositions(FVector2D(GetSnapGridSize(),0.0f));
			return FReply::Handled();
		}
		if( InKeyboardEvent.GetKey() ==  EKeys::Left || InKeyboardEvent.GetKey() ==  EKeys::NumPadFour )
		{
			UpdateSelectedNodesPositions(FVector2D(-GetSnapGridSize(),0.0f));
			return FReply::Handled();
		}
		if ( InKeyboardEvent.GetKey() ==  EKeys::Subtract )
		{
			ChangeZoomLevel(-1, CachedAllottedGeometryScaledSize / 2.f, InKeyboardEvent.IsControlDown());
			return FReply::Handled();
		}
		if ( InKeyboardEvent.GetKey() ==  EKeys::Add )
		{
			ChangeZoomLevel(+1, CachedAllottedGeometryScaledSize / 2.f, InKeyboardEvent.IsControlDown());
			return FReply::Handled();
		}

	}

	return SNodePanel::OnKeyDown(MyGeometry, InKeyboardEvent);
}


void SGraphPanel::GetAllPins(TSet< TSharedRef<SWidget> >& AllPins)
{
	// Get the set of pins for all children
	for (int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
	{
		TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(Children[ChildIndex]);
		ChildNode->GetPins(AllPins);
	}
}

void SGraphPanel::AddPinToHoverSet(UEdGraphPin* HoveredPin)
{
	CurrentHoveredPins.Add(HoveredPin);
	TimeSinceMouseEnteredPin = FSlateApplication::Get().GetCurrentTime();
}

void SGraphPanel::RemovePinFromHoverSet(UEdGraphPin* UnhoveredPin)
{
	CurrentHoveredPins.Remove(UnhoveredPin);
	TimeSinceMouseLeftPin = FSlateApplication::Get().GetCurrentTime();
}

void SGraphPanel::ArrangeChildrenForContextMenuSummon(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
{
	// First pass nodes
	for (int32 ChildIndex = 0; ChildIndex < VisibleChildren.Num(); ++ChildIndex)
	{
		const TSharedRef<SNode>& SomeChild = VisibleChildren[ChildIndex];
		if (!SomeChild->RequiresSecondPassLayout())
		{
			ArrangedChildren.AddWidget( AllottedGeometry.MakeChild( SomeChild, SomeChild->GetPosition() - ViewOffset, SomeChild->GetDesiredSizeForMarquee(), GetZoomAmount() ) );
		}
	}

	// Second pass nodes
	for (int32 ChildIndex = 0; ChildIndex < VisibleChildren.Num(); ++ChildIndex)
	{
		const TSharedRef<SNode>& SomeChild = VisibleChildren[ChildIndex];
		if (SomeChild->RequiresSecondPassLayout())
		{
			SomeChild->PerformSecondPassLayout(NodeToWidgetLookup);
			ArrangedChildren.AddWidget( AllottedGeometry.MakeChild( SomeChild, SomeChild->GetPosition() - ViewOffset, SomeChild->GetDesiredSizeForMarquee(), GetZoomAmount() ) );
		}
	}
}

TSharedPtr<SWidget> SGraphPanel::OnSummonContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	//Editability is up to the user to consider for menu options
	{
		// If we didn't drag very far, summon a context menu.
		// Figure out what's under the mouse: Node, Pin or just the Panel, and summon the context menu for that.
		UEdGraphNode* NodeUnderCursor = NULL;
		UEdGraphPin* PinUnderCursor = NULL;
		{
			FArrangedChildren ArrangedNodes(EVisibility::Visible);
			this->ArrangeChildrenForContextMenuSummon(MyGeometry, ArrangedNodes);
			const int32 HoveredNodeIndex = SWidget::FindChildUnderMouse( ArrangedNodes, MouseEvent );
			if (HoveredNodeIndex != INDEX_NONE)
			{
				const FArrangedWidget& HoveredNode = ArrangedNodes(HoveredNodeIndex);
				const TSharedRef<SGraphNode>& GraphNode = StaticCastSharedRef<SGraphNode>(HoveredNode.Widget);
				NodeUnderCursor = GraphNode->GetNodeObj();

				// Selection should switch to this code if it isn't already selected.
				// When multiple nodes are selected, we do nothing, provided that the
				// node for which the context menu is being created is in the selection set.
				if (!SelectionManager.IsNodeSelected(GraphNode->GetObjectBeingDisplayed()))
				{
					SelectionManager.SelectSingleNode(GraphNode->GetObjectBeingDisplayed());
				}

				const TSharedPtr<SGraphPin> HoveredPin = GraphNode->GetHoveredPin( HoveredNode.Geometry, MouseEvent );
				if (HoveredPin.IsValid())
				{
					PinUnderCursor = HoveredPin->GetPinObj();
				}
			}				
		}

		const FVector2D NodeAddPosition = PanelCoordToGraphCoord( MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) );
		TArray<UEdGraphPin*> NoSourcePins;

		return SummonContextMenu( MouseEvent.GetScreenSpacePosition(), NodeAddPosition, NodeUnderCursor, PinUnderCursor, NoSourcePins);
	}

	return TSharedPtr<SWidget>();
}


bool SGraphPanel::OnHandleLeftMouseRelease(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const bool bIsMakingConnection = (PreviewConnectorFromPins.Num() > 0) && PreviewConnectorFromPins[0].IsValid() && IsEditable.Get();
	if (bIsMakingConnection)
	{
		TSet< TSharedRef<SWidget> > AllConnectors;
		for (int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
		{
			//@FINDME:
			TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(Children[ChildIndex]);
			ChildNode->GetPins(AllConnectors);
		}

		TMap<TSharedRef<SWidget>, FArrangedWidget> PinGeometries;
		this->FindChildGeometries(MyGeometry, AllConnectors, PinGeometries);

		bool bHandledDrop = false;
		TSet<UEdGraphNode*> NodeList;
		for ( TMap<TSharedRef<SWidget>, FArrangedWidget>::TIterator SomePinIt(PinGeometries); !bHandledDrop && SomePinIt; ++SomePinIt )
		{
			FArrangedWidget& PinWidgetGeometry = SomePinIt.Value();
			if( PinWidgetGeometry.Geometry.IsUnderLocation( MouseEvent.GetScreenSpacePosition() ) )
			{
				SGraphPin& TargetPin = static_cast<SGraphPin&>( PinWidgetGeometry.Widget.Get() );

				if (PreviewConnectorFromPins[0]->TryHandlePinConnection(TargetPin))
				{
					NodeList.Add(TargetPin.GetPinObj()->GetOwningNode());
					NodeList.Add(PreviewConnectorFromPins[0]->GetPinObj()->GetOwningNode());
				}
				bHandledDrop = true;
			}
		}

		// No longer make a connection for a pin; we just connected or failed to connect.
		OnStopMakingConnection(/*bForceStop=*/ true);

		return true;
	}
	else
	{
		return false;
	}
}

void SGraphPanel::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FGraphEditorDragDropAction>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FGraphEditorDragDropAction> DragConnectionOp = StaticCastSharedPtr<FGraphEditorDragDropAction>(DragDropEvent.GetOperation());
		DragConnectionOp->SetHoveredGraph( SharedThis(this) );
	}
}

void SGraphPanel::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FGraphEditorDragDropAction>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FGraphEditorDragDropAction> DragConnectionOp = StaticCastSharedPtr<FGraphEditorDragDropAction>(DragDropEvent.GetOperation());
		DragConnectionOp->SetHoveredGraph( TSharedPtr<SGraphPanel>(NULL) );
	}
	else if( DragDrop::IsTypeMatch<FAssetDragDropOp>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FAssetDragDropOp> AssetOp = StaticCastSharedPtr<FAssetDragDropOp>(DragDropEvent.GetOperation());
		AssetOp->ClearTooltip();
	}
}

FReply SGraphPanel::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if( DragDrop::IsTypeMatch<FGraphEditorDragDropAction>(DragDropEvent.GetOperation()) )
	{
		PreviewConnectorEndpoint = MyGeometry.AbsoluteToLocal( DragDropEvent.GetScreenSpacePosition() );
		return FReply::Handled();
	}
	else if(DragDrop::IsTypeMatch<FExternalDragOperation>( DragDropEvent.GetOperation() ) )
	{
		return AssetUtil::CanHandleAssetDrag(DragDropEvent);
	}
	else if( DragDrop::IsTypeMatch<FAssetDragDropOp>(DragDropEvent.GetOperation()) )
	{
		if(GraphObj != NULL && GraphObj->GetSchema())
		{
			TSharedPtr<FAssetDragDropOp> AssetOp = StaticCastSharedPtr<FAssetDragDropOp>(DragDropEvent.GetOperation());
			bool bOkIcon = false;
			FString TooltipText;
			GraphObj->GetSchema()->GetAssetsGraphHoverMessage(AssetOp->AssetData, GraphObj, TooltipText, bOkIcon);
			const FSlateBrush* TooltipIcon = bOkIcon ? FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK")) : FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));;
			AssetOp->SetTooltip(TooltipText, TooltipIcon);
		}
		return FReply::Handled();
	} 
	else
	{
		return FReply::Unhandled();
	}
}

FReply SGraphPanel::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	const FVector2D NodeAddPosition = PanelCoordToGraphCoord( MyGeometry.AbsoluteToLocal( DragDropEvent.GetScreenSpacePosition() ) );

	FSlateApplication::Get().SetKeyboardFocus(AsShared(), EKeyboardFocusCause::SetDirectly);

	if (DragDrop::IsTypeMatch<FGraphEditorDragDropAction>(DragDropEvent.GetOperation()))
	{
		check(GraphObj);
		TSharedPtr<FGraphEditorDragDropAction> DragConn = StaticCastSharedPtr<FGraphEditorDragDropAction>(DragDropEvent.GetOperation());
		return DragConn->DroppedOnPanel( SharedThis( this ), DragDropEvent.GetScreenSpacePosition(), NodeAddPosition, *GraphObj);
	}
	else if (DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>(DragDropEvent.GetOperation()))
	{
		TSharedPtr<FActorDragDropGraphEdOp> ActorOp = StaticCastSharedPtr<FActorDragDropGraphEdOp>(DragDropEvent.GetOperation());
		OnDropActor.ExecuteIfBound(ActorOp->Actors, GraphObj, NodeAddPosition);
		return FReply::Handled();
	}

	else if (DragDrop::IsTypeMatch<FLevelDragDropOp>(DragDropEvent.GetOperation()))
	{
		TSharedPtr<FLevelDragDropOp> LevelOp = StaticCastSharedPtr<FLevelDragDropOp>(DragDropEvent.GetOperation());
		OnDropStreamingLevel.ExecuteIfBound(LevelOp->StreamingLevelsToDrop, GraphObj, NodeAddPosition);
		return FReply::Handled();
	}
	else
	{
		if(GraphObj != NULL && GraphObj->GetSchema() != NULL)
		{
			TArray< FAssetData > DroppedAssetData = AssetUtil::ExtractAssetDataFromDrag( DragDropEvent );

			if ( DroppedAssetData.Num() > 0 )
			{
				GraphObj->GetSchema()->DroppedAssetsOnGraph( DroppedAssetData, NodeAddPosition, GraphObj );
				return FReply::Handled();
			}
		}

		return FReply::Unhandled();
	}
}

void SGraphPanel::OnBeginMakingConnection( const TSharedRef<SGraphPin>& InOriginatingPin )
{
	PreviewConnectorFromPins.Add(InOriginatingPin);
}

void SGraphPanel::OnStopMakingConnection(bool bForceStop)
{
	if (bForceStop || !bPreservePinPreviewConnection)
	{
		PreviewConnectorFromPins.Reset();
		bPreservePinPreviewConnection = false;
	}
}

void SGraphPanel::PreservePinPreviewUntilForced()
{
	bPreservePinPreviewConnection = true;
}

/** Add a slot to the CanvasPanel dynamically */
void SGraphPanel::AddGraphNode( const TSharedRef<SNodePanel::SNode>& NodeToAdd )
{
	TSharedRef<SGraphNode> GraphNode = StaticCastSharedRef<SGraphNode>(NodeToAdd);
	GraphNode->SetOwner( SharedThis(this) );

	const UEdGraphNode* Node = GraphNode->GetNodeObj();
	if (Node && Node->IsA( UEdGraphNode_Comment::StaticClass()))
	{
		SNodePanel::AddGraphNodeToBack(NodeToAdd);
	}
	else
	{
		SNodePanel::AddGraphNode(NodeToAdd);
	}
}

void SGraphPanel::RemoveAllNodes()
{
	CurrentHoveredPins.Empty();
	SNodePanel::RemoveAllNodes();
}

TSharedPtr<SWidget> SGraphPanel::SummonContextMenu( const FVector2D& WhereToSummon, const FVector2D& WhereToAddNode, UEdGraphNode* ForNode, UEdGraphPin* ForPin, const TArray<UEdGraphPin*>& DragFromPins )
{
	if (OnGetContextMenuFor.IsBound())
	{
		FActionMenuContent FocusedContent = OnGetContextMenuFor.Execute( WhereToAddNode, ForNode, ForPin, DragFromPins );

		TSharedRef<SWidget> MenuContent =
			SNew( SBorder )
			.BorderImage( FEditorStyle::GetBrush("Menu.Background") )
			[
				FocusedContent.Content
			];
		
		FSlateApplication::Get().PushMenu(
			AsShared(),
			MenuContent,
			WhereToSummon,
			FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu )
			);

		return FocusedContent.WidgetToFocus;
	}

	return TSharedPtr<SWidget>();
}

void SGraphPanel::AttachGraphEvents(TSharedPtr<SGraphNode> CreatedSubNode)
{
	check(CreatedSubNode.IsValid());
	CreatedSubNode->SetIsEditable(IsEditable);
	CreatedSubNode->SetDoubleClickEvent(OnNodeDoubleClicked);
	CreatedSubNode->SetVerifyTextCommitEvent(OnVerifyTextCommit);
	CreatedSubNode->SetTextCommittedEvent(OnTextCommitted);
}

void SGraphPanel::AddNode (UEdGraphNode* Node)
{
	TSharedPtr<SGraphNode> NewNode = FNodeFactory::CreateNodeWidget(Node);
	check(NewNode.IsValid());

	FEdGraphEditAction* GraphAction = UserAddedNodes.Find(Node);
	bool bWasUserAdded = GraphAction? true : false;

	NewNode->SetIsEditable(IsEditable);
	NewNode->SetDoubleClickEvent(OnNodeDoubleClicked);
	NewNode->SetVerifyTextCommitEvent(OnVerifyTextCommit);
	NewNode->SetTextCommittedEvent(OnTextCommitted);
	NewNode->SetDisallowedPinConnectionEvent(OnDisallowedPinConnection);

	this->AddGraphNode
	(
		NewNode.ToSharedRef()
	);

	if (bWasUserAdded)
	{
		// Add the node to visible children, this allows focus to occur on sub-widgets for naming purposes.
		VisibleChildren.Add(NewNode.ToSharedRef());

		NewNode->PlaySpawnEffect();

		// Do not Select nodes unless they are marked for selection
		if((GraphAction->Action & GRAPHACTION_SelectNode) != 0)
		{
			SelectAndCenterObject(Node, false);
		}

		NewNode->UpdateGraphNode();
		NewNode->RequestRename();
	}
	else
	{
		NewNode->UpdateGraphNode();
	}
}

void SGraphPanel::Update()
{
	// Add widgets for all the nodes that don't have one.
	if(GraphObj != NULL)
	{
		int32 NumNodesAdded = 0;

		// Scan for all missing nodes
		for ( int32 NodeIndex = 0; NodeIndex < GraphObj->Nodes.Num(); ++NodeIndex )
		{
			UEdGraphNode* Node = GraphObj->Nodes[NodeIndex];
			if (Node)
			{
				AddNode(Node);
				++NumNodesAdded;
			}
			else
			{
				UE_LOG(LogGraphPanel, Warning, TEXT("Found NULL Node in GraphObj array. A node type has been deleted without creating an ActiveClassRedictor to K2Node_DeadClass."));
			}
		}
	}
	else
	{
		RemoveAllNodes();
	}

	// Clean out set of added nodes
	UserAddedNodes.Empty();

	// Invoke any delegate methods
	OnUpdateGraphPanel.ExecuteIfBound();
}

// Purges the existing visual representation (typically followed by an Update call in the next tick)
void SGraphPanel::PurgeVisualRepresentation()
{
	RemoveAllNodes();
}

bool SGraphPanel::IsNodeTitleVisible(const class UEdGraphNode* Node, bool bRequestRename)
{
	bool bTitleVisible = false;
	TSharedRef<SNode>* pWidget = NodeToWidgetLookup.Find(Node);

	if (pWidget != NULL)
	{
		TWeakPtr<SGraphNode> GraphNode = StaticCastSharedRef<SGraphNode>(*pWidget);
		if(GraphNode.IsValid() && !HasMouseCapture())
		{
			FSlateRect TitleRect = GraphNode.Pin()->GetTitleRect();
			const FVector2D TopLeft = FVector2D( TitleRect.Left, TitleRect.Top );
			const FVector2D BottomRight = FVector2D( TitleRect.Right, TitleRect.Bottom );

			if( IsRectVisible( TopLeft, BottomRight ))
			{
				bTitleVisible = true;
			}
			else if( bRequestRename )
			{
				bTitleVisible = JumpToRect( TopLeft, BottomRight );
			}

			if( bTitleVisible && bRequestRename )
			{
				GraphNode.Pin()->RequestRename();
			}
		}
	}
	return bTitleVisible;
}

bool SGraphPanel::IsRectVisible(const FVector2D &TopLeft, const FVector2D &BottomRight)
{
	return TopLeft >= PanelCoordToGraphCoord( FVector2D::ZeroVector ) && BottomRight <= PanelCoordToGraphCoord( CachedAllottedGeometryScaledSize );
}

bool SGraphPanel::JumpToRect(const FVector2D &TopLeft, const FVector2D &BottomRight)
{
	ZoomTargetTopLeft = TopLeft;
	ZoomTargetBottomRight = BottomRight;
	bDeferredZoomingToFit = true;
	return true;
}

void SGraphPanel::JumpToNode(const UEdGraphNode* JumpToMe, bool bRequestRename)
{
	bDeferredZoomingToFit = false;

	if (JumpToMe != NULL)
	{
		if (bRequestRename)
		{
			TSharedRef<SNode>* pWidget = NodeToWidgetLookup.Find(JumpToMe);
			if (pWidget != NULL)
			{
				TSharedRef<SGraphNode> GraphNode = StaticCastSharedRef<SGraphNode>(*pWidget);
				GraphNode->RequestRename();
			}
		}
		// Select this node, and request that we jump to it.
		SelectAndCenterObject(JumpToMe, true);
	}
}


void SGraphPanel::JumpToPin(const UEdGraphPin* JumpToMe)
{
	if (JumpToMe != NULL)
	{
		JumpToNode(JumpToMe->GetOwningNode(), false);
	}
}

void SGraphPanel::OnGraphChanged ( const FEdGraphEditAction& EditAction)
{
	if ((EditAction.Graph == GraphObj ) 
		&& (EditAction.Action & GRAPHACTION_AddNode) 
		&& (EditAction.Node != NULL)
		// We do not want to mark it as a UserAddedNode for graphs that do not currently have focus,
		// this causes each one to want to do the effects and rename, which causes problems.
		&& (HasKeyboardFocus() || HasFocusedDescendants()))
	{
		UserAddedNodes.Add(EditAction.Node, EditAction);
	}
}

void SGraphPanel::NotifyGraphChanged ( const FEdGraphEditAction& EditAction)
{
	// Forward call
	OnGraphChanged(EditAction);
}

void SGraphPanel::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject( GraphObj );
	Collector.AddReferencedObject( GraphObjToDiff );
}

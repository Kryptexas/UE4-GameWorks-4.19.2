// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GraphEditorCommon.h"
#include "SGraphNodeComment.h"
//#include "TextWrapperHelpers.h"

namespace SCommentNodeDefs
{

	/** Size of the hit result border for the window borders */
	/* L, T, R, B */
	static const FSlateRect HitResultBorderSize(10,10,10,10);

	/** Minimum resize width for comment */
	static const float MinWidth = 30.0;

	/** Minimum resize height for comment */
	static const float MinHeight = 30.0;

	/** TitleBarColor = CommnetColor * TitleBarColorMultiplier */
	static const float TitleBarColorMultiplier = 0.6f;

	/** Titlebar Offset - taken from the widget borders in UpdateGraphNode */
	static const FSlateRect TitleBarOffset(13,8,-3,0);
}


void SGraphNodeComment::Construct(const FArguments& InArgs, UEdGraphNode* InNode)
{
	this->GraphNode = InNode;
	this->bIsSelected = false;

	// Set up animation
	{
		ZoomCurve = SpawnAnim.AddCurve(0, 0.1f);
		FadeCurve = SpawnAnim.AddCurve(0.15f, 0.15f);
	}

	// Cache these values so they do not force a re-build of the node next tick.
	CachedCommentTitle = GetNodeComment();
	CachedWidth = InNode->NodeWidth;

	this->UpdateGraphNode();

	// Pull out sizes
	UserSize.X = InNode->NodeWidth;
	UserSize.Y = InNode->NodeHeight;

	MouseZone = CWZ_NotInWindow;
	bUserIsDragging = false;
}

bool SGraphNodeComment::InSelectionArea(ECommentWindowZone InMouseZone) const
{
	return (	(InMouseZone == CWZ_RightBorder) || (InMouseZone == CWZ_BottomBorder) || (InMouseZone == CWZ_BottomRightBorder) || 
				(InMouseZone == CWZ_LeftBorder) || (InMouseZone == CWZ_TopBorder) || (InMouseZone == CWZ_TopLeftBorder) ||
				(InMouseZone == CWZ_TopRightBorder) || (InMouseZone == CWZ_BottomLeftBorder) );
}

FCursorReply SGraphNodeComment::OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const
{
	if (MouseZone == CWZ_RightBorder || MouseZone == CWZ_LeftBorder)
	{
		// right/left of comment
		return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
	}
	else if (MouseZone == CWZ_BottomRightBorder || MouseZone == CWZ_TopLeftBorder)
	{
		// bottom right / top left hand corner
		return FCursorReply::Cursor( EMouseCursor::ResizeSouthEast );
	}
	else if (MouseZone == CWZ_BottomBorder || MouseZone == CWZ_TopBorder)
	{
		// bottom / top of comment
		return FCursorReply::Cursor(EMouseCursor::ResizeUpDown);
	}
	else if (MouseZone == CWZ_BottomLeftBorder || MouseZone == CWZ_TopRightBorder)
	{
		// bottom left / top right hand corner
		return FCursorReply::Cursor( EMouseCursor::ResizeSouthWest );
	}
	else if (MouseZone == CWZ_TitleBar) 
	{
		return FCursorReply::Cursor(EMouseCursor::CardinalCross);
	}

	return FCursorReply::Unhandled();
}

FReply SGraphNodeComment::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FVector2D LocalMouseCoordinates = MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() );
	
	if (bUserIsDragging)
	{
		FVector2D GraphSpaceCoordinates = NodeCoordToGraphCoord( MouseEvent.GetScreenSpacePosition() );
		FVector2D OldGraphSpaceCoordinates = NodeCoordToGraphCoord( MouseEvent.GetLastScreenSpacePosition() );
		FVector2D Delta = GraphSpaceCoordinates - OldGraphSpaceCoordinates;
		

		//Clamp delta value based on resizing direction
		if( MouseZone == CWZ_LeftBorder || MouseZone == CWZ_RightBorder )
		{
			Delta.Y = 0.0f;
		}
		else if( MouseZone == CWZ_TopBorder || MouseZone == CWZ_BottomBorder )
		{
			Delta.X = 0.0f;
		}

		//Resize node delta value
		FVector2D DeltaNodeSize = Delta;

		//Modify node size delta value based on resizing direction
		if(	(MouseZone == CWZ_LeftBorder) || (MouseZone == CWZ_TopBorder) || (MouseZone == CWZ_TopLeftBorder) )
		{
			DeltaNodeSize = -DeltaNodeSize;
		}
		else if( MouseZone == CWZ_TopRightBorder )
		{
			DeltaNodeSize.Y = -DeltaNodeSize.Y;
		}
		else if( MouseZone == CWZ_BottomLeftBorder )
		{
			DeltaNodeSize.X = -DeltaNodeSize.X;
		}

		StoredUserSize.X += DeltaNodeSize.X;
		StoredUserSize.Y += DeltaNodeSize.Y;

		const FVector2D PreviousUserSize = UserSize;
		const float SnapSize = SNodePanel::GetSnapGridSize();
		UserSize.X = SnapSize * FMath::Round(StoredUserSize.X/SnapSize);
		UserSize.Y = SnapSize * FMath::Round(StoredUserSize.Y/SnapSize);

		FVector2D DeltaNodePos(0,0);
		//Modify node position (resizing top and left sides)
		if( MouseZone != CWZ_BottomBorder && MouseZone != CWZ_RightBorder && MouseZone != CWZ_BottomRightBorder )
		{
			//Delta value to move graph node position
			DeltaNodePos = PreviousUserSize - UserSize;

			//Clamp position delta based on resizing direction
			if( MouseZone == CWZ_BottomLeftBorder )
			{
				DeltaNodePos.Y = 0.0f;
			}
			else if( MouseZone == CWZ_TopRightBorder )
			{
				DeltaNodePos.X = 0.0f;
			}
		}

		if(UserSize.Y < SCommentNodeDefs::MinHeight)
		{
			UserSize.Y = SCommentNodeDefs::MinHeight;
			DeltaNodePos.Y = GetCorrectedNodePosition().Y - GetPosition().Y;
		}

		if(UserSize.X < SCommentNodeDefs::MinWidth) 
		{
			UserSize.X = SCommentNodeDefs::MinWidth;
			DeltaNodePos.X = GetCorrectedNodePosition().X - GetPosition().X;
		}

		SGraphNode::MoveTo( GetPosition() + DeltaNodePos );
	}
	else
	{
		MouseZone = FindMouseZone(LocalMouseCoordinates);
	}

	return SGraphNode::OnMouseMove( MyGeometry, MouseEvent );
}

void SGraphNodeComment::InitNodeAnchorPoint()
{
	NodeAnchorPoint = GetPosition();
	
	if( (MouseZone == CWZ_LeftBorder) || (MouseZone == CWZ_TopBorder) || (MouseZone == CWZ_TopLeftBorder) )
	{
		NodeAnchorPoint += UserSize;
	}
	else if( MouseZone == CWZ_BottomLeftBorder )
	{
		NodeAnchorPoint.X += UserSize.X;
	}
	else if( MouseZone == CWZ_TopRightBorder )
	{
		NodeAnchorPoint.Y += UserSize.Y;
	}
}


FVector2D SGraphNodeComment::GetCorrectedNodePosition() const
{
	FVector2D CorrectedPos = NodeAnchorPoint;
	
	if( (MouseZone == CWZ_LeftBorder) || (MouseZone == CWZ_TopBorder) || (MouseZone == CWZ_TopLeftBorder) )
	{
		CorrectedPos -= UserSize;
	}
	else if( MouseZone == CWZ_BottomLeftBorder )
	{
		CorrectedPos.X -= UserSize.X;
	}
	else if( MouseZone == CWZ_TopRightBorder )
	{
		CorrectedPos.Y -= UserSize.Y;
	}

	return CorrectedPos;
}

SGraphNodeComment::ECommentWindowZone SGraphNodeComment::FindMouseZone(const FVector2D& LocalMouseCoordinates) const
{
	ECommentWindowZone InMouseZone = CWZ_NotInWindow;

	const float InverseZoomFactor = 1.0f / FMath::Min(GetOwnerPanel()->GetZoomAmount(), 1.0f);
	const FSlateRect HitResultBorderSize(
		SCommentNodeDefs::HitResultBorderSize.Left * InverseZoomFactor,
		SCommentNodeDefs::HitResultBorderSize.Right * InverseZoomFactor,
		SCommentNodeDefs::HitResultBorderSize.Top * InverseZoomFactor, 
		SCommentNodeDefs::HitResultBorderSize.Bottom * InverseZoomFactor);

	const float TitleBarHeight = TitleBar.IsValid() ? TitleBar->GetDesiredSize().Y : 0.0f;

	// Test for hit in location of 'grab' zone
	if (LocalMouseCoordinates.Y > ( UserSize.Y - HitResultBorderSize.Bottom))
	{
		InMouseZone = CWZ_BottomBorder;
	}
	else if (LocalMouseCoordinates.Y <= (HitResultBorderSize.Top))
	{
		InMouseZone = CWZ_TopBorder;
	}
	else if (LocalMouseCoordinates.Y <= (HitResultBorderSize.Top + TitleBarHeight))
	{
		InMouseZone = CWZ_TitleBar;
	}

	if (LocalMouseCoordinates.X > (UserSize.X - HitResultBorderSize.Right))
	{
		if (InMouseZone == CWZ_BottomBorder)
		{
			InMouseZone = CWZ_BottomRightBorder;
		}
		else if (InMouseZone == CWZ_TopBorder)
		{
			InMouseZone = CWZ_TopRightBorder;
		}
		else
		{
			InMouseZone = CWZ_RightBorder;
		}
	}
	else if (LocalMouseCoordinates.X <= HitResultBorderSize.Left)
	{
		if (InMouseZone == CWZ_TopBorder)
		{
			InMouseZone = CWZ_TopLeftBorder;
		}
		else if (InMouseZone == CWZ_BottomBorder)
		{
			InMouseZone = CWZ_BottomLeftBorder;
		}
		else
		{
			InMouseZone = CWZ_LeftBorder;
		}
	}
	
	// Test for hit on rest of frame
	if (InMouseZone == CWZ_NotInWindow)
	{
		if (LocalMouseCoordinates.Y > HitResultBorderSize.Top) 
		{
			InMouseZone = CWZ_InWindow;
		}
		else if (LocalMouseCoordinates.X > HitResultBorderSize.Left) 
		{
			InMouseZone = CWZ_InWindow;
		}
	}

	return InMouseZone;
}

void SGraphNodeComment::OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{	
	// Determine the zone the mouse is in
	FVector2D LocalMouseCoordinates = MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() );
	MouseZone = FindMouseZone(LocalMouseCoordinates);

	SCompoundWidget::OnMouseEnter( MyGeometry, MouseEvent );
}

void SGraphNodeComment::OnMouseLeave( const FPointerEvent& MouseEvent ) 
{
	if( !bUserIsDragging )
	{
		// Reset our mouse zone
		MouseZone = CWZ_NotInWindow;
		SCompoundWidget::OnMouseLeave( MouseEvent );
	}
}

void SGraphNodeComment::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphNode::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	const int32 CurrentWidth = static_cast<int32>(UserSize.X);
	const FString CurrentCommentTitle = GetNodeComment();

	if (( CurrentCommentTitle != CachedCommentTitle ) || ( CurrentWidth != CachedWidth ))
	{
		CachedCommentTitle = CurrentCommentTitle;
		CachedWidth = CurrentWidth;

		UpdateGraphNode();
	}
}

FReply SGraphNodeComment::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	return FReply::Unhandled();
}

void SGraphNodeComment::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{

}

float SGraphNodeComment::GetWrapAt() const
{
	return (float)(CachedWidth - 16);
}

bool SGraphNodeComment::IsNameReadOnly() const
{
	return !IsEditable.Get() || SGraphNode::IsNameReadOnly();
}

void SGraphNodeComment::UpdateGraphNode()
{
	// No pins in a comment box
	InputPins.Empty();
	OutputPins.Empty();

	// Avoid standard box model too
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	TSharedPtr<SWidget> ErrorText = SetupErrorReporting();

	bool bIsSet = GraphNode->IsA(UEdGraphNode_Comment::StaticClass());
	this->ContentScale.Bind( this, &SGraphNode::GetContentScale );
	this->ChildSlot
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("Kismet.Comment.Background") )
			.ColorAndOpacity( FLinearColor::White )
			.BorderBackgroundColor( this, &SGraphNodeComment::GetCommentBodyColor )
			.Padding(  FMargin(3.0f) )
			[
				SNew(SVerticalBox)
				.ToolTipText( this, &SGraphNode::GetNodeTooltip )
				+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				[
					SAssignNew(TitleBar, SBorder)
					.BorderImage( FEditorStyle::GetBrush("Graph.Node.TitleBackground") )
					.BorderBackgroundColor( this, &SGraphNodeComment::GetCommentTitleBarColor )
					.Padding( FMargin(10,5,5,3) )
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SAssignNew(InlineEditableText, SInlineEditableTextBlock)
							.Style( FEditorStyle::Get(), "Graph.CommentBlock.TitleInlineEditableText" )
							.Text( this, &SGraphNodeComment::GetEditableNodeTitleAsText )
							.OnVerifyTextChanged(this, &SGraphNodeComment::OnVerifyNameTextChanged)
							.OnTextCommitted(this, &SGraphNodeComment::OnNameTextCommited)
							.IsReadOnly( this, &SGraphNodeComment::IsNameReadOnly )
							.IsSelected( this, &SGraphNodeComment::IsSelectedExclusively )
							.WrapTextAt( this, &SGraphNodeComment::GetWrapAt )
						]
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(1.0f)
				[
					ErrorText->AsShared()
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					// NODE CONTENT AREA
					SNew(SBorder)
					.BorderImage( FEditorStyle::GetBrush("NoBorder") )
				]
			]			
		];
}

FVector2D SGraphNodeComment::ComputeDesiredSize() const
{
	return UserSize;
}

FReply SGraphNodeComment::OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	// If user double-clicked in the title bar area
	if(FindMouseZone(InMyGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition())) == CWZ_TitleBar && IsEditable.Get())
	{
		// Request a rename
		RequestRename();

		// Set the keyboard focus
		if(!HasKeyboardFocus())
		{
			FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EKeyboardFocusCause::SetDirectly);
		}

		return FReply::Handled();
	}
	else
	{
		// Otherwise let the base class handle it
		return SGraphNode::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
	}
}

FReply SGraphNodeComment::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ( InSelectionArea() && (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) && IsEditable.Get() ) 
	{
		bUserIsDragging = true;
		StoredUserSize = UserSize;

		//Find node anchor point
		InitNodeAnchorPoint();

		return FReply::Handled().CaptureMouse( SharedThis(this) );
	}
	else
	{
		HandleSelection(true, true);
		return FReply::Unhandled();
	}
}

FReply SGraphNodeComment::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ( (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) && bUserIsDragging )
	{
		bUserIsDragging = false;

		// Resize the node	
		UserSize.X = FPlatformMath::Round(UserSize.X);
		UserSize.Y =FPlatformMath::Round(UserSize.Y);

		GetNodeObj()->ResizeNode(UserSize);
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

void SGraphNodeComment::HandleSelection(bool bSelected, bool bUpdateNodesUnderComment) const
{
	if ((!this->bIsSelected && bSelected) || bUpdateNodesUnderComment)
	{
		SGraphNodeComment* Comment = const_cast<SGraphNodeComment*> (this);

		UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(GraphNode);

		if (CommentNode)
		{

			// Get our geo
			const FVector2D NodePosition = GetPosition();
			const FVector2D NodeSize = GetDesiredSize();
			const FSlateRect CommentRect( NodePosition.X, NodePosition.Y, NodePosition.X + NodeSize.X, NodePosition.Y + NodeSize.Y );

			TSharedPtr< SGraphPanel > Panel = Comment->GetOwnerPanel();
			FChildren* PanelChildren = Panel->GetChildren();
			int32 NumChildren = PanelChildren->Num();
			CommentNode->ClearNodesUnderComment();

			for ( int32 NodeIndex=0; NodeIndex < NumChildren; ++NodeIndex )
			{
				const TSharedRef<SGraphNode> SomeNodeWidget = StaticCastSharedRef<SGraphNode>(PanelChildren->GetChildAt(NodeIndex));

				UObject* GraphObject = SomeNodeWidget->GetObjectBeingDisplayed();

				const FVector2D SomeNodePosition = SomeNodeWidget->GetPosition();
				const FVector2D SomeNodeSize = SomeNodeWidget->GetDesiredSize();

				const FSlateRect NodeGeometryGraphSpace( SomeNodePosition.X, SomeNodePosition.Y, SomeNodePosition.X + SomeNodeSize.X, SomeNodePosition.Y + SomeNodeSize.Y );
				if ( FSlateRect::IsRectangleContained( CommentRect, NodeGeometryGraphSpace ) )
				{
					CommentNode->AddNodeUnderComment(GraphObject);
				}
			}
		}
	}
	this->bIsSelected = bSelected;
}

const FSlateBrush* SGraphNodeComment::GetShadowBrush(bool bSelected) const
{
	HandleSelection(bSelected);
	return SGraphNode::GetShadowBrush(bSelected);
}

void SGraphNodeComment::GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const
{
	const float Fudge = 3.0f;

	HandleSelection(bSelected);

	FOverlayBrushInfo HandleBrush = FEditorStyle::GetBrush( TEXT("Kismet.Comment.Handle") );

	HandleBrush.OverlayOffset.X = WidgetSize.X - HandleBrush.Brush->ImageSize.X - Fudge;
	HandleBrush.OverlayOffset.Y = WidgetSize.Y - HandleBrush.Brush->ImageSize.Y - Fudge;

	Brushes.Add(HandleBrush);
	return SGraphNode::GetOverlayBrushes(bSelected, WidgetSize, Brushes);
}

bool SGraphNodeComment::OnHitTest( const FGeometry& MyGeometry, FVector2D InAbsoluteCursorPosition )
{
	FVector2D LocalMouseCoordinates = MyGeometry.AbsoluteToLocal( InAbsoluteCursorPosition );
	ECommentWindowZone InMouseZone = FindMouseZone(LocalMouseCoordinates);
	
	//@TODO: really want to return true for CWZ_InWindow as well, *as long as* there are no other comment bubbles underneath that return one of the edges
	// This is a brute force way to allow nested comment boxes to work usefully
	return (InMouseZone != CWZ_NotInWindow) && (InMouseZone != CWZ_InWindow);
}

void SGraphNodeComment::MoveTo( const FVector2D& NewPosition ) 
{
	FVector2D PositionDelta = NewPosition - GetPosition();
	SGraphNode::MoveTo(NewPosition);


	// Don't drag note content if either of the shift keys are down.
	FModifierKeysState KeysState = FSlateApplication::Get().GetModifierKeys();
	if(!KeysState.IsShiftDown())
	{
		UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(GraphNode);
		if (CommentNode && CommentNode->MoveMode == ECommentBoxMode::GroupMovement)
		{
			// Now update any nodes which are touching the comment but *not* selected
			// Selected nodes will be moved as part of the normal selection code
			TSharedPtr< SGraphPanel > Panel = GetOwnerPanel();

			for (FCommentNodeSet::TConstIterator NodeIt( CommentNode->GetNodesUnderComment() ); NodeIt; ++NodeIt)
			{
				if (UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
				{
					if ( !Panel->SelectionManager.IsNodeSelected(Node))
					{
						Node->NodePosX += PositionDelta.X;
						Node->NodePosY += PositionDelta.Y;
					}
				}
			}
		}
	}
}

bool SGraphNodeComment::ShouldScaleNodeComment() const 
{
	return false;
}

FSlateColor SGraphNodeComment::GetCommentBodyColor() const
{
	UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(GraphNode);

	if (CommentNode)
	{
		return CommentNode->CommentColor;
	}
	else
	{
		return FLinearColor::White;
	}
}

FSlateColor SGraphNodeComment::GetCommentTitleBarColor() const
{
	UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(GraphNode);
	if (CommentNode)
	{
		const FLinearColor Color = CommentNode->CommentColor * SCommentNodeDefs::TitleBarColorMultiplier;
		return FLinearColor(Color.R, Color.G, Color.B);
	}
	else
	{
		const FLinearColor Color = FLinearColor::White * SCommentNodeDefs::TitleBarColorMultiplier;
		return FLinearColor(Color.R, Color.G, Color.B);
	}
}

bool SGraphNodeComment::CanBeSelected(const FVector2D& MousePositionInNode) const
{
	const ECommentWindowZone InMouseZone = FindMouseZone(MousePositionInNode);
	return CWZ_TitleBar == InMouseZone;
}

FVector2D SGraphNodeComment::GetDesiredSizeForMarquee() const
{
	const float TitleBarHeight = TitleBar.IsValid() ? TitleBar->GetDesiredSize().Y : 0.0f;
	return FVector2D(UserSize.X, TitleBarHeight);
}

FSlateRect SGraphNodeComment::GetTitleRect() const
{
	const FVector2D NodePosition = GetPosition();
	FVector2D NodeSize  = TitleBar.IsValid() ? TitleBar->GetDesiredSize() : GetDesiredSize();
	return FSlateRect( NodePosition.X, NodePosition.Y, NodePosition.X + NodeSize.X, NodePosition.Y + NodeSize.Y ) + SCommentNodeDefs::TitleBarOffset;
}

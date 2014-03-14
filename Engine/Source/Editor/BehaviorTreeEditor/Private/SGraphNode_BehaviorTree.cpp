// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BehaviorTreeEditorPrivatePCH.h"
#include "SGraphPreviewer.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "NodeFactory.h"
#include "SGraphNode.h"
#include "SGraphNode_BehaviorTree.h"
#include "SGraphPin.h"
#include "ScopedTransaction.h"
#include "BehaviorTreeColors.h"

/////////////////////////////////////////////////////
// SBehaviorTreePin

class SBehaviorTreePin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SBehaviorTreePin){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);
protected:
	// Begin SGraphPin interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() OVERRIDE;

	/** @return The color that we should use to draw this pin */
	virtual FSlateColor GetPinColor() const OVERRIDE;

	// End SGraphPin interface

	const FSlateBrush* GetPinBorder() const;
};

void SBehaviorTreePin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	this->SetCursor( EMouseCursor::Default );

	bShowLabel = true;
	IsEditable = true;

	GraphPinObj = InPin;
	check(GraphPinObj != NULL);

	const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
	check(Schema);

	SBorder::Construct( SBorder::FArguments()
		.BorderImage( this, &SBehaviorTreePin::GetPinBorder )
		.BorderBackgroundColor( this, &SBehaviorTreePin::GetPinColor )
		.OnMouseButtonDown( this, &SBehaviorTreePin::OnPinMouseDown )
		.Cursor( this, &SBehaviorTreePin::GetPinCursor )
		.Padding(FMargin(5.0f))
		);
}

TSharedRef<SWidget>	SBehaviorTreePin::GetDefaultValueWidget()
{
	return SNew(STextBlock);
}

const FSlateBrush* SBehaviorTreePin::GetPinBorder() const
{
	return FEditorStyle::GetBrush(TEXT("Graph.StateNode.Body"));
}

FSlateColor SBehaviorTreePin::GetPinColor() const
{
	return 
		GraphPinObj->bIsDiffing ? BehaviorTreeColors::Pin::Diff :
		IsHovered() ? BehaviorTreeColors::Pin::Hover :
		(GraphPinObj->PinType.PinCategory == UBehaviorTreeEditorTypes::PinCategory_SingleComposite) ? BehaviorTreeColors::Pin::CompositeOnly :
		(GraphPinObj->PinType.PinCategory == UBehaviorTreeEditorTypes::PinCategory_SingleTask) ? BehaviorTreeColors::Pin::TaskOnly :
		(GraphPinObj->PinType.PinCategory == UBehaviorTreeEditorTypes::PinCategory_SingleNode) ? BehaviorTreeColors::Pin::SingleNode :
		BehaviorTreeColors::Pin::Default;
}

/////////////////////////////////////////////////////
// SGraphNode_BehaviorTree

void SGraphNode_BehaviorTree::Construct(const FArguments& InArgs, UBehaviorTreeGraphNode* InNode)
{
	DebuggerStateDuration = 0.0f;
	DebuggerStateCounter = INDEX_NONE;
	bSuppressDebuggerTriggers = false;

	this->GraphNode = InNode;

	this->SetCursor(EMouseCursor::CardinalCross);
	
	this->UpdateGraphNode();
}

void SGraphNode_BehaviorTree::AddDecorator(TSharedPtr<SGraphNode> DecoratorWidget)
{
	DecoratorsBox->AddSlot().AutoHeight()
	[
		DecoratorWidget.ToSharedRef()
	];
	DecoratorWidgets.Add(DecoratorWidget);
}

void SGraphNode_BehaviorTree::AddService(TSharedPtr<SGraphNode> ServiceWidget)
{
	ServicesBox->AddSlot().AutoHeight()
	[
		ServiceWidget.ToSharedRef()
	];
	ServicesWidgets.Add(ServiceWidget);
}

FSlateColor SGraphNode_BehaviorTree::GetBorderBackgroundColor() const
{
	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(GraphNode);
	const bool bIsInDebuggerActiveState = BTGraphNode && BTGraphNode->bDebuggerMarkCurrentlyActive;
	const bool bIsInDebuggerPrevState = BTGraphNode && BTGraphNode->bDebuggerMarkPreviouslyActive;
	const bool bSelectedSubNode = BTGraphNode->ParentNode && GetOwnerPanel()->SelectionManager.SelectedNodes.Contains(GraphNode);
	
	UBTNode* NodeInstance = BTGraphNode ? Cast<UBTNode>(BTGraphNode->NodeInstance) : NULL;
	const bool bIsDisconnected = NodeInstance && NodeInstance->GetExecutionIndex() == MAX_uint16;
	const bool bIsService = BTGraphNode->IsA(UBehaviorTreeGraphNode_Service::StaticClass());
	const bool bIsBrokenWithParent = bIsService ? 
		BTGraphNode->ParentNode != NULL && BTGraphNode->ParentNode->Services.Find(BTGraphNode) == INDEX_NONE ? true : false :
		BTGraphNode->ParentNode != NULL && BTGraphNode->ParentNode->Decorators.Find(BTGraphNode) == INDEX_NONE ? true : 
		(BTGraphNode->NodeInstance != NULL && (Cast<UBTNode>(BTGraphNode->NodeInstance->GetOuter()) == NULL && Cast<UBehaviorTree>(BTGraphNode->NodeInstance->GetOuter())==NULL)) ? true : false;

	if (FBehaviorTreeDebugger::IsPIENotSimulating() && BTGraphNode)
	{
		if (BTGraphNode->bHighlightInAbortRange0)
		{
			return BehaviorTreeColors::NodeBorder::HighlightAbortRange0;
		}
		else if (BTGraphNode->bHighlightInAbortRange1)
		{
			return BehaviorTreeColors::NodeBorder::HighlightAbortRange1;
		}
		else if (BTGraphNode->bHighlightInSearchTree)
		{
			return BehaviorTreeColors::NodeBorder::QuickFind;
		}
	}

	return bSelectedSubNode ? BehaviorTreeColors::NodeBorder::Selected : 
		bIsBrokenWithParent ? BehaviorTreeColors::NodeBorder::BrokenWithParent :
		bIsDisconnected ? BehaviorTreeColors::NodeBorder::Disconnected :
		bIsInDebuggerActiveState ? BehaviorTreeColors::NodeBorder::ActiveDebugging :
		bIsInDebuggerPrevState ? BehaviorTreeColors::NodeBorder::InactiveDebugging :
		BehaviorTreeColors::NodeBorder::Inactive;
}

FSlateColor SGraphNode_BehaviorTree::GetBackgroundColor() const
{
	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(GraphNode);
	UBehaviorTreeGraphNode_Decorator* BTGraph_Decorator = Cast<UBehaviorTreeGraphNode_Decorator>(GraphNode);
	const bool bIsActiveForDebugger = BTGraphNode ?
		!bSuppressDebuggerColor && (BTGraphNode->bDebuggerMarkCurrentlyActive || BTGraphNode->bDebuggerMarkPreviouslyActive) :
		false;

	FLinearColor NodeColor = BehaviorTreeColors::NodeBody::Default;
	if (BTGraph_Decorator || Cast<UBehaviorTreeGraphNode_CompositeDecorator>(GraphNode))
	{
		NodeColor = bIsActiveForDebugger ? BehaviorTreeColors::Debugger::ActiveDecorator : BehaviorTreeColors::NodeBody::Decorator;
	}
	else if (Cast<UBehaviorTreeGraphNode_Task>(GraphNode))
	{
		NodeColor = BehaviorTreeColors::NodeBody::Task;
	}
	else if (Cast<UBehaviorTreeGraphNode_Composite>(GraphNode))
	{
		NodeColor = BehaviorTreeColors::NodeBody::Composite;
	}
	else if (Cast<UBehaviorTreeGraphNode_Service>(GraphNode))
	{
		NodeColor = bIsActiveForDebugger ? BehaviorTreeColors::Debugger::ActiveService : BehaviorTreeColors::NodeBody::Service;
	}

	return (FlashAlpha > 0.0f) ? FMath::Lerp(NodeColor, FlashColor, FlashAlpha) : NodeColor;
}

void SGraphNode_BehaviorTree::UpdateGraphNode()
{
	bDragMarkerVisible = false;
	InputPins.Empty();
	OutputPins.Empty();

	if (DecoratorsBox.IsValid())
	{
		DecoratorsBox->ClearChildren();
	} 
	else
	{
		SAssignNew(DecoratorsBox,SVerticalBox);
	}

	if (ServicesBox.IsValid())
	{
		ServicesBox->ClearChildren();
	}
	else
	{
		SAssignNew(ServicesBox,SVerticalBox);
	}

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	DecoratorWidgets.Reset();
	ServicesWidgets.Reset();
	OutputPinBox.Reset();

	UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(GraphNode);

	if (BTNode)
	{
		for (int32 i = 0; i < BTNode->Decorators.Num(); i++)
		{
			if (BTNode->Decorators[i])
			{
				TSharedPtr<SGraphNode> NewNode = FNodeFactory::CreateNodeWidget(BTNode->Decorators[i]);
				if (OwnerGraphPanelPtr.IsValid())
				{
					NewNode->SetOwner(OwnerGraphPanelPtr.Pin().ToSharedRef());
					OwnerGraphPanelPtr.Pin()->AttachGraphEvents(NewNode);
				}
				AddDecorator(NewNode);
				NewNode->UpdateGraphNode();
			}
		}

		for (int32 i = 0; i < BTNode->Services.Num(); i++)
		{
			if (BTNode->Services[i])
			{
				TSharedPtr<SGraphNode> NewNode = FNodeFactory::CreateNodeWidget(BTNode->Services[i]);
				if (OwnerGraphPanelPtr.IsValid())
				{
					NewNode->SetOwner(OwnerGraphPanelPtr.Pin().ToSharedRef());
					OwnerGraphPanelPtr.Pin()->AttachGraphEvents(NewNode);
				}
				AddService(NewNode);
				NewNode->UpdateGraphNode();
			}
		}
	}

	TSharedPtr<SErrorText> ErrorText;
	TSharedPtr<STextBlock> DescriptionText; 
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);
	float NodePadding = 10.0f;

	if (Cast<UBehaviorTreeGraphNode_Decorator>(GraphNode) || Cast<UBehaviorTreeGraphNode_CompositeDecorator>(GraphNode) || Cast<UBehaviorTreeGraphNode_Service>(GraphNode))
	{
		NodePadding = 2.0f;
	}

	this->ContentScale.Bind( this, &SGraphNode::GetContentScale );
	this->ChildSlot
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush( "Graph.StateNode.Body" ) )
			.Padding(0)
			.BorderBackgroundColor( this, &SGraphNode_BehaviorTree::GetBorderBackgroundColor )
			.OnMouseButtonDown(this, &SGraphNode_BehaviorTree::OnMouseDown)
			.OnMouseButtonUp(this, &SGraphNode_BehaviorTree::OnMouseUp)
			[
				SNew(SOverlay)
				.ToolTipText( this, &SGraphNode::GetNodeTooltip )
				// INPUT PIN AREA
				+SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				[
					SAssignNew(LeftNodeBox, SVerticalBox)
				]

				// OUTPUT PIN AREA
				+SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Bottom)
				[
					SAssignNew(RightNodeBox, SVerticalBox)
					+SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.Padding(20.0f,0.0f)
					.FillHeight(1.0f)
					[
						SAssignNew(OutputPinBox, SHorizontalBox)
					]
				]

				// STATE NAME AREA
				+SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				.Padding(NodePadding)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						DecoratorsBox.ToSharedRef()
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBorder)
						.BorderImage( FEditorStyle::GetBrush("BTEditor.Graph.BTNode.Body") )
						.BorderBackgroundColor( this, &SGraphNode_BehaviorTree::GetBackgroundColor )
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						.Visibility(EVisibility::SelfHitTestInvisible)
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							.HAlign(HAlign_Fill)
							.VAlign(VAlign_Fill)
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SHorizontalBox)
									+SHorizontalBox::Slot()
									.AutoWidth()
									[
										// POPUP ERROR MESSAGE
										SAssignNew(ErrorText, SErrorText )
										.BackgroundColor( this, &SGraphNode_BehaviorTree::GetErrorColor )
										.ToolTipText( this, &SGraphNode_BehaviorTree::GetErrorMsgToolTip )
									]
									+SHorizontalBox::Slot()
									.AspectRatio()
									[
										SNew(SImage)
										.Image(this, &SGraphNode_BehaviorTree::GetNameIcon)
									]
									+SHorizontalBox::Slot()
									.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
									[
										SNew(SVerticalBox)
										+SVerticalBox::Slot()
										.AutoHeight()
										[
											SAssignNew(InlineEditableText, SInlineEditableTextBlock)
											.Style( FEditorStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText" )
											.Text( NodeTitle.Get(), &SNodeTitle::GetHeadTitle )
											.OnVerifyTextChanged(this, &SGraphNode_BehaviorTree::OnVerifyNameTextChanged)
											.OnTextCommitted(this, &SGraphNode_BehaviorTree::OnNameTextCommited)
											.IsReadOnly( this, &SGraphNode_BehaviorTree::IsNameReadOnly )
											.IsSelected(this, &SGraphNode_BehaviorTree::IsSelectedExclusively)
										]
										+SVerticalBox::Slot()
										.AutoHeight()
										[
											NodeTitle.ToSharedRef()
										]
									]
								]
								+SVerticalBox::Slot()
								.AutoHeight()
								[
									// DESCRIPTION MESSAGE
									SAssignNew(DescriptionText, STextBlock )
									.Text(this, &SGraphNode_BehaviorTree::GetDescription)
								]
							]
							+SOverlay::Slot()
							.HAlign(HAlign_Right)
							.VAlign(VAlign_Fill)
							[
								SNew(SBorder)
								.BorderImage( FEditorStyle::GetBrush("BTEditor.Graph.BTNode.Body") )
								.BorderBackgroundColor(BehaviorTreeColors::Debugger::SearchFailed)
								.Padding(FMargin(4.0f, 0.0f))
								.Visibility(this, &SGraphNode_BehaviorTree::GetDebuggerSearchFailedMarkerVisibility)
							]
						]
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(10.0f,0,0,0))
					[
						ServicesBox.ToSharedRef()
					]
				]
				//drag marker overlay
				+SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				[
					SNew(SBorder)
					.BorderBackgroundColor(BehaviorTreeColors::Action::DragMarker)
					.ColorAndOpacity(BehaviorTreeColors::Action::DragMarker)
					.BorderImage(FEditorStyle::GetBrush("Graph.BTNode.Body"))
					.Visibility(this, &SGraphNode_BehaviorTree::GetDragOverMarkerVisibility)
					[
						SNew(SBox)
						.HeightOverride(4)
					]
				]
			]
		];

	ErrorReporting = ErrorText;
	ErrorReporting->SetError(ErrorMsg);
	CreatePinWidgets();
}

EVisibility SGraphNode_BehaviorTree::GetDebuggerSearchFailedMarkerVisibility() const
{
	UBehaviorTreeGraphNode_Decorator* MyNode = Cast<UBehaviorTreeGraphNode_Decorator>(GraphNode);
	return MyNode && MyNode->bDebuggerMarkSearchFailed ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

void SGraphNode_BehaviorTree::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SGraphNode::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	CachedPosition = AllottedGeometry.AbsolutePosition / AllottedGeometry.Scale;

	if (bIsMouseDown)
	{
		MouseDownTime += InDeltaTime;
	}

	UBehaviorTreeGraphNode* MyNode = Cast<UBehaviorTreeGraphNode>(GraphNode);
	if (MyNode && MyNode->DebuggerUpdateCounter != DebuggerStateCounter)
	{
		DebuggerStateCounter = MyNode->DebuggerUpdateCounter;
		DebuggerStateDuration = 0.0f;
		bSuppressDebuggerColor = false;
		bSuppressDebuggerTriggers = false;
	}

	DebuggerStateDuration += InDeltaTime;

	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(GraphNode);
	float NewFlashAlpha = 0.0f;
	TriggerOffsets.Reset();

	if (BTGraphNode && FBehaviorTreeDebugger::IsPlaySessionPaused())
	{
		const float SearchPathDelay = 0.5f;
		const float SearchPathBlink = 1.0f;
		const float SearchPathBlinkFreq = 10.0f;
		const float SearchPathKeepTime = 2.0f;
		const float ActiveFlashDuration = 0.2f;

		const bool bHasResult = BTGraphNode->bDebuggerMarkSearchSucceeded || BTGraphNode->bDebuggerMarkSearchFailed;
		const bool bHasTriggers = !bSuppressDebuggerTriggers && (BTGraphNode->bDebuggerMarkSearchTrigger || BTGraphNode->bDebuggerMarkSearchFailedTrigger);
		if (bHasResult || bHasTriggers)
		{
			const float FlashStartTime = BTGraphNode->DebuggerSearchPathIndex * SearchPathDelay;
			const float FlashStopTime = (BTGraphNode->DebuggerSearchPathSize * SearchPathDelay) + SearchPathKeepTime;
			
			UBehaviorTreeGraphNode_Decorator* BTGraph_Decorator = Cast<UBehaviorTreeGraphNode_Decorator>(GraphNode);
			UBehaviorTreeGraphNode_CompositeDecorator* BTGraph_CompDecorator = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(GraphNode);

			bSuppressDebuggerColor = (DebuggerStateDuration < FlashStopTime);
			if (bSuppressDebuggerColor)
			{
				if (bHasResult && (BTGraph_Decorator || BTGraph_CompDecorator))
				{
					NewFlashAlpha =
						(DebuggerStateDuration > FlashStartTime + SearchPathBlink) ? 1.0f :
						(FMath::Trunc(DebuggerStateDuration * SearchPathBlinkFreq) % 2) ? 1.0f : 0.0f;
				} 
			}

			FlashColor = BTGraphNode->bDebuggerMarkSearchSucceeded ?
				BehaviorTreeColors::Debugger::SearchSucceeded :
				BehaviorTreeColors::Debugger::SearchFailed;
		}
		else if (BTGraphNode->bDebuggerMarkFlashActive)
		{
			NewFlashAlpha = (DebuggerStateDuration < ActiveFlashDuration) ?
				FMath::Square(1.0f - (DebuggerStateDuration / ActiveFlashDuration)) : 
				0.0f;

			FlashColor = BehaviorTreeColors::Debugger::TaskFlash;
		}

		if (bHasTriggers)
		{
			// find decorator that caused restart
			for (int32 i = 0; i < DecoratorWidgets.Num(); i++)
			{
				if (DecoratorWidgets[i].IsValid())
				{
					SGraphNode_BehaviorTree* TestSNode = (SGraphNode_BehaviorTree*)DecoratorWidgets[i].Get();
					UBehaviorTreeGraphNode* ChildNode = Cast<UBehaviorTreeGraphNode>(TestSNode->GraphNode);
					if (ChildNode && (ChildNode->bDebuggerMarkSearchFailedTrigger || ChildNode->bDebuggerMarkSearchTrigger))
					{
						TriggerOffsets.Add(FNodeBounds(TestSNode->GetCachedPosition() - CachedPosition, TestSNode->GetDesiredSize()));
					}
				}
			}

			// when it wasn't any of them, add node itself to triggers (e.g. parallel's main task)
			if (DecoratorWidgets.Num() == 0)
			{
				TriggerOffsets.Add(FNodeBounds(FVector2D(0,0),GetDesiredSize()));
			}
		}
	}
	FlashAlpha = NewFlashAlpha;
}

FReply SGraphNode_BehaviorTree::OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	if (!MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && bIsMouseDown)
	{
		bIsMouseDown = false;
		MouseDownTime = 0;
	}

	if (bIsMouseDown && MouseDownTime > 0.1f && FBehaviorTreeDebugger::IsPIENotSimulating())
	{
		//if we are holding mouse over a subnode
		if (Cast<UBehaviorTreeGraphNode>(GraphNode)->ParentNode)
		{
			const TSharedRef<SGraphPanel>& Panel = this->GetOwnerPanel().ToSharedRef();
			const TSharedRef<SGraphNode>& Node = SharedThis(this);
			return FReply::Handled().BeginDragDrop(FDragNode::New(Panel, Node));
		}
	}
	return FReply::Unhandled();
}

FReply SGraphNode_BehaviorTree::OnMouseUp(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	bIsMouseDown = false;
	MouseDownTime = 0;
	return FReply::Unhandled();
}

TSharedPtr<SGraphNode> SGraphNode_BehaviorTree::GetSubNodeUnderCursor(const FGeometry& WidgetGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<SGraphNode> ResultNode;

	// We just need to find the one WidgetToFind among our descendants.
	TSet< TSharedRef<SWidget> > SubWidgetsSet;

	for (int32 i=0; i < DecoratorWidgets.Num(); i++)
	{
		SubWidgetsSet.Add(DecoratorWidgets[i].ToSharedRef());
	}
	for (int32 i=0; i < ServicesWidgets.Num(); i++)
	{
		SubWidgetsSet.Add(ServicesWidgets[i].ToSharedRef());
	}
	TMap<TSharedRef<SWidget>, FArrangedWidget> Result;

	FindChildGeometries(WidgetGeometry, SubWidgetsSet, Result);

	if ( Result.Num() > 0 )
	{
		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		Result.GenerateValueArray( ArrangedChildren.GetInternalArray() );
		int32 HoveredIndex = SWidget::FindChildUnderMouse( ArrangedChildren, MouseEvent );
		if ( HoveredIndex != INDEX_NONE )
		{
			ResultNode = StaticCastSharedRef<SGraphNode>(ArrangedChildren(HoveredIndex).Widget);
		}
	}

	return ResultNode;
}

FReply SGraphNode_BehaviorTree::OnMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	bIsMouseDown = true;

	if (Cast<UBehaviorTreeGraphNode>(GraphNode)->ParentNode)
	{
		GetOwnerPanel()->SelectionManager.ClickedOnNode(GraphNode,MouseEvent);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SGraphNode_BehaviorTree::OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	return SGraphNode::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent );
}

void SGraphNode_BehaviorTree::SetDragMarker(bool bEnabled)
{
	bDragMarkerVisible = bEnabled;
}

EVisibility SGraphNode_BehaviorTree::GetDragOverMarkerVisibility() const
{
	return bDragMarkerVisible ? EVisibility::Visible : EVisibility::Collapsed; 
}

void SGraphNode_BehaviorTree::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	// Is someone dragging a node?
	if ( DragDrop::IsTypeMatch<FDragNode>(DragDropEvent.GetOperation()) )
	{
		// Inform the Drag and Drop operation that we are hovering over this node.
		TSharedPtr<FDragNode> DragConnectionOp = StaticCastSharedPtr<FDragNode>(DragDropEvent.GetOperation());
		TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, DragDropEvent);
		DragConnectionOp->SetHoveredNode( SubNode.IsValid() ? SubNode : SharedThis(this) );
		if (DragConnectionOp->IsValidOperation() && Cast<UBehaviorTreeGraphNode>(GraphNode)->ParentNode)
		{
			SetDragMarker(true);
		}
	}
	
	SGraphNode::OnDragEnter(MyGeometry, DragDropEvent);
}

FReply SGraphNode_BehaviorTree::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	// Is someone dragging a node?
	if ( DragDrop::IsTypeMatch<FDragNode>(DragDropEvent.GetOperation()) )
	{
		// Inform the Drag and Drop operation that we are hovering over this node.
		TSharedPtr<FDragNode> DragConnectionOp = StaticCastSharedPtr<FDragNode>(DragDropEvent.GetOperation());
		TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, DragDropEvent);
		DragConnectionOp->SetHoveredNode( SubNode.IsValid() ? SubNode : SharedThis(this) );
	}
	return SGraphNode::OnDragOver(MyGeometry, DragDropEvent);
}

void SGraphNode_BehaviorTree::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FDragNode>(DragDropEvent.GetOperation()) )
	{
		// Inform the Drag and Drop operation that we are not hovering any pins
		TSharedPtr<FDragNode> DragConnectionOp = StaticCastSharedPtr<FDragNode>(DragDropEvent.GetOperation());
		DragConnectionOp->SetHoveredNode( TSharedPtr<SGraphNode>(NULL) );
	}
	SetDragMarker(false);

	SGraphNode::OnDragLeave(DragDropEvent);
}

FReply SGraphNode_BehaviorTree::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FDragNode>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FDragNode> DragNodeOp = StaticCastSharedPtr<FDragNode>(DragDropEvent.GetOperation());
		if (!DragNodeOp->IsValidOperation())
		{
			return FReply::Handled();
		}
		//dropped on decorator, route to parent
		if (Cast<UBehaviorTreeGraphNode>(GraphNode)->ParentNode != NULL)
		{
			return FReply::Unhandled();
		}

		const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "GraphEd_DragDropNode", "Drag&Drop Node") );

		const TArray< TSharedRef<SGraphNode> >& DraggedNodes = DragNodeOp->GetNodes();
		bool bReorderOperation = true;

		for (int32 i = 0; i < DraggedNodes.Num(); i++)
		{
			bool bIsDraggedNodeService = Cast<UBehaviorTreeGraphNode_Service>(DraggedNodes[i]->GetNodeObj()) != NULL;

			UBehaviorTreeGraphNode* ParentNode = (Cast<UBehaviorTreeGraphNode>(DraggedNodes[i]->GetNodeObj()))->ParentNode;
			if (bReorderOperation && ParentNode != Cast<UBehaviorTreeGraphNode>(GraphNode))
			{
				bReorderOperation = false;
			}

			if (ParentNode)
			{
				ParentNode->Modify();
				TArray<UBehaviorTreeGraphNode*> & SubNodes = bIsDraggedNodeService ? ParentNode->Services : ParentNode->Decorators;
				for (int32 j = 0; j < SubNodes.Num(); j++)
				{
					if (SubNodes[j] == DraggedNodes[i]->GetNodeObj())
					{
						SubNodes.RemoveAt(j);
					}
				}
			}
		}

		UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(GraphNode);

		int32 InsertIndex = -1;
		TArray< TSharedPtr<SGraphNode> > SubNodesWidgets;
		SubNodesWidgets.Append(ServicesWidgets);
		SubNodesWidgets.Append(DecoratorWidgets);
		
		for (int32 i=0; i < SubNodesWidgets.Num(); i++)
		{
			TSharedPtr<SGraphNode_BehaviorTree> BTNodeWidget = StaticCastSharedPtr<SGraphNode_BehaviorTree>(SubNodesWidgets[i]);
			if (BTNodeWidget->bDragMarkerVisible)
			{
				TArray<UBehaviorTreeGraphNode*> & SubNodes = BTNodeWidget->GetNodeObj()->IsA(UBehaviorTreeGraphNode_Service::StaticClass()) ? BTNode->Services : BTNode->Decorators;
				InsertIndex = SubNodes.IndexOfByKey(BTNodeWidget->GetNodeObj());
				break;
			}
		}
		
		for (int32 k = 0; k < DraggedNodes.Num(); k++)
		{
			UBehaviorTreeGraphNode* DraggedNode = Cast<UBehaviorTreeGraphNode>(DraggedNodes[k]->GetNodeObj());
			bool bIsDraggedNodeService = Cast<UBehaviorTreeGraphNode_Service>(DraggedNode) != NULL;
			TArray<UBehaviorTreeGraphNode*> & SubNodes = bIsDraggedNodeService ? BTNode->Services : BTNode->Decorators;
			DraggedNode->Modify();
			DraggedNode->ParentNode = BTNode;
			BTNode->Modify();
			InsertIndex > -1 ? SubNodes.Insert(DraggedNode, InsertIndex) : SubNodes.Add(DraggedNode);
		}

		
		if (bReorderOperation) // if updating this node is enough, do it
		{
			UpdateGraphNode();
		}
		else // if decorator was dragged between nodes, rebuild graph
		{
			GraphNode->GetGraph()->NotifyGraphChanged();

			UBehaviorTreeGraph* MyGraph = Cast<UBehaviorTreeGraph>(GraphNode->GetGraph());
			if (MyGraph)
			{
				FAbortDrawHelper EmptyMode;

				MyGraph->UpdateAsset(UBehaviorTreeGraph::ClearDebuggerFlags);
				MyGraph->UpdateAbortHighlight(EmptyMode, EmptyMode);
			}
		}
	}

	return SGraphNode::OnDrop(MyGeometry, DragDropEvent);
}

FString	SGraphNode_BehaviorTree::GetDescription() const
{
	UBehaviorTreeGraphNode* StateNode = CastChecked<UBehaviorTreeGraphNode>(GraphNode);
	UEdGraphPin* MyInputPin = StateNode->GetInputPin();
	UEdGraphPin* MyParentOutputPin = NULL;
	if (MyInputPin != NULL && MyInputPin->LinkedTo.Num() > 0)
	{
		MyParentOutputPin = MyInputPin->LinkedTo[0];
	}

	int32 Index = 0;

	if (GEditor->bIsSimulatingInEditor || GEditor->PlayWorld != NULL)
	{
		// show execution index (debugging purposes)
		UBTNode* BTNode = Cast<UBTNode>(StateNode->NodeInstance);
		Index = (BTNode && BTNode->GetExecutionIndex() < 0xffff) ? BTNode->GetExecutionIndex() : -1;

		// special case: range of execution indices in composite decorator node
		UBehaviorTreeGraphNode_CompositeDecorator* CompDecorator = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(GraphNode);
		if (CompDecorator && CompDecorator->FirstExecutionIndex != CompDecorator->LastExecutionIndex)
		{
			return FString::Printf( TEXT("(%d..%d) %s"),
				CompDecorator->FirstExecutionIndex, CompDecorator->LastExecutionIndex, *StateNode->GetDescription());
		}
	}
	else
	{
		// show child index
	if (MyParentOutputPin != NULL)
	{
		for (Index = 0; Index < MyParentOutputPin->LinkedTo.Num(); ++Index)
		{
			if (MyParentOutputPin->LinkedTo[Index] == MyInputPin)
			{
				break;
			}
		}
	}

	if (!MyParentOutputPin || MyParentOutputPin->LinkedTo.Num() < 2 )
	{
		return FString::Printf( TEXT("%s"), StateNode ? *StateNode->GetDescription() : TEXT(""));
	}
	}

	return FString::Printf( TEXT("(%d) %s"), Index, StateNode ? *StateNode->GetDescription() : TEXT(""));
}

FString SGraphNode_BehaviorTree::GetPinTooltip(UEdGraphPin* GraphPinObj) const
{
	FString HoverText;

	check(GraphPinObj != nullptr);
	UEdGraphNode* GraphNode = GraphPinObj->GetOwningNode();
	if (GraphNode != nullptr)
	{
		GraphNode->GetPinHoverText(*GraphPinObj, /*out*/ HoverText);
	}

	return HoverText;
}


void SGraphNode_BehaviorTree::CreatePinWidgets()
{
	UBehaviorTreeGraphNode* StateNode = CastChecked<UBehaviorTreeGraphNode>(GraphNode);

	for (int32 PinIdx = 0; PinIdx < StateNode->Pins.Num(); PinIdx++)
	{
		UEdGraphPin* MyPin = StateNode->Pins[PinIdx];
		if (!MyPin->bHidden)
	{
			TSharedPtr<SGraphPin> NewPin = SNew(SBehaviorTreePin, MyPin)
				.ToolTipText( this, &SGraphNode_BehaviorTree::GetPinTooltip, MyPin);

		NewPin->SetIsEditable(IsEditable);

			AddPin(NewPin.ToSharedRef());
	}
	}
}

void SGraphNode_BehaviorTree::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner( SharedThis(this) );

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
	if (bAdvancedParameter)
	{
		PinToAdd->SetVisibility( TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced) );
	}

	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		LeftNodeBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			.Padding(20.0f,0.0f)
			[
				PinToAdd
			];
		InputPins.Add(PinToAdd);
	}
	else // Direction == EEdGraphPinDirection::EGPD_Output
	{
		const bool bIsSingleTaskPin = PinObj && (PinObj->PinType.PinCategory == UBehaviorTreeEditorTypes::PinCategory_SingleTask);
		if (bIsSingleTaskPin)
		{
			OutputPinBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
				.FillWidth(0.4f)
				.Padding(0,0,20.0f,0)
				[
					PinToAdd
				];
		}
		else
		{
			OutputPinBox->AddSlot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.FillWidth(1.0f)
			[
				PinToAdd
			];
		}
		OutputPins.Add(PinToAdd);
	}
}

TSharedPtr<SToolTip> SGraphNode_BehaviorTree::GetComplexTooltip()
{
	UBehaviorTreeGraphNode_CompositeDecorator* DecoratorNode = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(GraphNode);
	if (DecoratorNode && DecoratorNode->GetBoundGraph())
	{
		return SNew(SToolTip)
			[
				SNew(SGraphPreviewer, DecoratorNode->GetBoundGraph())
				.CornerOverlayText(this, &SGraphNode_BehaviorTree::GetPreviewCornerText)
			];
	}
	return NULL;
}

FString SGraphNode_BehaviorTree::GetPreviewCornerText() const
{
	return FString(TEXT("Composite Decorator"));
}

const FSlateBrush* SGraphNode_BehaviorTree::GetNameIcon() const
{
	UBehaviorTreeGraphNode_CompositeDecorator* CompDecorator = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(GraphNode);
	UBehaviorTreeGraphNode_Decorator* Decorator = Cast<UBehaviorTreeGraphNode_Decorator>(GraphNode);
	UBTDecorator* DecoratorInstance = Decorator ? Cast<UBTDecorator>(Decorator->NodeInstance) : NULL;
	
	if ((DecoratorInstance && DecoratorInstance->GetFlowAbortMode() != EBTFlowAbortMode::None) ||
		(CompDecorator && CompDecorator->bCanAbortFlow))
	{
		return FEditorStyle::GetBrush( TEXT("BTEditor.Graph.FlowControl.Icon") );
	}
	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(GraphNode);
	return BTGraphNode != NULL ? FEditorStyle::GetBrush(BTGraphNode->GetNameIcon()) : FEditorStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Icon"));
}

void SGraphNode_BehaviorTree::GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const
{
	UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(GraphNode);
	if (BTNode == NULL)
	{
		return;
	}

	if (BTNode->bHasBreakpoint)
	{
		FOverlayBrushInfo BreakpointOverlayInfo;
		BreakpointOverlayInfo.Brush = BTNode->bIsBreakpointEnabled ?
			FEditorStyle::GetBrush(TEXT("BTEditor.DebuggerOverlay.Breakpoint.Enabled")) :
			FEditorStyle::GetBrush(TEXT("BTEditor.DebuggerOverlay.Breakpoint.Disabled"));

		if (BreakpointOverlayInfo.Brush)
		{
			BreakpointOverlayInfo.OverlayOffset -= BreakpointOverlayInfo.Brush->ImageSize / 2.f;
		}

		Brushes.Add(BreakpointOverlayInfo);
	}

	if (FBehaviorTreeDebugger::IsPlaySessionPaused())
	{
		if (BTNode->bDebuggerMarkBreakpointTrigger || (BTNode->bDebuggerMarkCurrentlyActive && BTNode->IsA(UBehaviorTreeGraphNode_Task::StaticClass())))
		{
			FOverlayBrushInfo IPOverlayInfo;

			IPOverlayInfo.Brush = BTNode->bDebuggerMarkBreakpointTrigger ? FEditorStyle::GetBrush(TEXT("BTEditor.DebuggerOverlay.BreakOnBreakpointPointer")) : 
				FEditorStyle::GetBrush(TEXT("BTEditor.DebuggerOverlay.ActiveNodePointer"));
			if (IPOverlayInfo.Brush)
			{
				float Overlap = 10.f;
				IPOverlayInfo.OverlayOffset.X = (WidgetSize.X/2.f) - (IPOverlayInfo.Brush->ImageSize.X/2.f);
				IPOverlayInfo.OverlayOffset.Y = (Overlap - IPOverlayInfo.Brush->ImageSize.Y);
			}

			IPOverlayInfo.AnimationEnvelope = FVector2D(0.f, 10.f);
			Brushes.Add(IPOverlayInfo);
		}

		if (TriggerOffsets.Num())
		{
			FOverlayBrushInfo IPOverlayInfo;

			IPOverlayInfo.Brush = FEditorStyle::GetBrush(BTNode->bDebuggerMarkSearchTrigger ?
				TEXT("BTEditor.DebuggerOverlay.SearchTriggerPointer") :
				TEXT("BTEditor.DebuggerOverlay.FailedTriggerPointer") );

			if (IPOverlayInfo.Brush)
			{
				for (int32 i = 0; i < TriggerOffsets.Num(); i++)
				{
					IPOverlayInfo.OverlayOffset.X = -IPOverlayInfo.Brush->ImageSize.X;
					IPOverlayInfo.OverlayOffset.Y = TriggerOffsets[i].Position.Y + TriggerOffsets[i].Size.Y / 2 - IPOverlayInfo.Brush->ImageSize.Y / 2;

					IPOverlayInfo.AnimationEnvelope = FVector2D(10.f, 0.f);
					Brushes.Add(IPOverlayInfo);
				}
			}
		}
	}
}

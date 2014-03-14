// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GraphEditorCommon.h"
#include "NodeFactory.h"
#include "TokenizedMessage.h"
#include "Editor/UnrealEd/Public/DragAndDrop/ActorDragDropGraphEdOp.h"
#include "Editor/UnrealEd/Public/DragAndDrop/AssetDragDropOp.h"
#include "Editor/Persona/Public/BoneDragDropOp.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "SLevelOfDetailBranchNode.h"
#include "IDocumentation.h"

/////////////////////////////////////////////////////
// SNodeTitle

void SNodeTitle::Construct(const FArguments& InArgs, UEdGraphNode* InNode)
{
	GraphNode = InNode;
	CachedString = FString();

	ExtraLineStyle = InArgs._ExtraLineStyle;

	// If the user set the text, use it, otherwise use the node title by default
	if(InArgs._Text.IsSet())
	{
		TitleText = InArgs._Text;
	}
	else
	{
		TitleText = TAttribute<FString>(this, &SNodeTitle::GetNodeTitle);
	}
	CachedString = TitleText.Get();
	RebuildWidget();
}

void SNodeTitle::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Checks to see if the cached string is valid, and if not, updates it.
	FString CurrentNodeTitle = TitleText.Get();
	if (CurrentNodeTitle != CachedString)
	{
		CachedString = CurrentNodeTitle;
		RebuildWidget();
	}
}

FString SNodeTitle::GetNodeTitle() const
{
	return (GraphNode != NULL)
		? GraphNode->GetNodeTitle(ENodeTitleType::FullTitle)
		: NSLOCTEXT("GraphEditor", "NullNode", "Null Node").ToString();
}

FText SNodeTitle::GetHeadTitle() const
{
	return GraphNode->bCanRenameNode? FText::FromString(GraphNode->GetNodeTitle(ENodeTitleType::EditableTitle)) : CachedHeadTitle;
}

void SNodeTitle::RebuildWidget()
{
	// Create the box to contain the lines
	TSharedPtr<SVerticalBox> VerticalBox;
	this->ChildSlot
	[
		SAssignNew(VerticalBox, SVerticalBox)
	];

	// Break the title into lines
	TArray<FString> Lines;
	CachedString.ParseIntoArray(&Lines, TEXT("\n"), false);

	if(Lines.Num())
	{
		CachedHeadTitle = FText::FromString(Lines[0]);
	}

	// Make a separate widget for each line, using a less obvious style for subsequent lines
	for (int32 Index = 1; Index < Lines.Num(); ++Index)
	{
		VerticalBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.TextStyle( FEditorStyle::Get(), ExtraLineStyle )
			.Text(Lines[Index])
		];
	}
}


/////////////////////////////////////////////////////
// SGraphNode

// Check whether drag and drop functionality is permitted on the given node
bool SGraphNode::CanAllowInteractionUsingDragDropOp( const UEdGraphNode* GraphNodePtr, const TSharedPtr<FActorDragDropOp>& DragDropOp )
{
	bool bReturn = false;

	//Allow interaction only if this node is a literal type object.
	//Only change actor reference if a single actor reference is dragged from the outliner.
	if( GraphNodePtr->IsA( UK2Node_Literal::StaticClass() )  && DragDropOp->Actors.Num() == 1 )
	{
		bReturn = true;
	}
	return bReturn;
}

void SGraphNode::SetIsEditable(TAttribute<bool> InIsEditable)
{
	IsEditable = InIsEditable;
}

/** Set event when node is double clicked */
void SGraphNode::SetDoubleClickEvent(FSingleNodeEvent InDoubleClickEvent)
{
	OnDoubleClick = InDoubleClickEvent;
}

void SGraphNode::SetVerifyTextCommitEvent(FOnNodeVerifyTextCommit InOnVerifyTextCommit)
{
	OnVerifyTextCommit = InOnVerifyTextCommit;
}

void SGraphNode::SetTextCommittedEvent(FOnNodeTextCommitted InOnTextCommitted)
{
	OnTextCommitted = InOnTextCommitted;
}

void SGraphNode::SetDisallowedPinConnectionEvent(SGraphEditor::FOnDisallowedPinConnection InOnDisallowedPinConnection)
{
	OnDisallowedPinConnection = InOnDisallowedPinConnection;
}

void SGraphNode::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	// Is someone dragging a connection?
	if ( DragDrop::IsTypeMatch<FGraphEditorDragDropAction>(DragDropEvent.GetOperation()) )
	{
		// Inform the Drag and Drop operation that we are hovering over this pin.
		TSharedPtr<FGraphEditorDragDropAction> DragConnectionOp = StaticCastSharedPtr<FGraphEditorDragDropAction>(DragDropEvent.GetOperation());
		DragConnectionOp->SetHoveredNode( SharedThis(this) );
	}
	else if( DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FActorDragDropGraphEdOp> DragConnectionOp = StaticCastSharedPtr<FActorDragDropGraphEdOp>(DragDropEvent.GetOperation());
		if( GraphNode->IsA( UK2Node_Literal::StaticClass() ) )
		{
			//Show tool tip only if a single actor is dragged
			if( DragConnectionOp->Actors.Num() == 1 )
			{
				UK2Node_Literal* LiteralNode = CastChecked< UK2Node_Literal > ( GraphNode );

				//Check whether this node is already referencing the same actor dragged from outliner
				if( LiteralNode->GetObjectRef() != DragConnectionOp->Actors[0].Get() )
				{
					DragConnectionOp->SetToolTip(FActorDragDropGraphEdOp::ToolTip_Compatible);
				}
			}
			else
			{
				//For more that one actor dragged on to a literal node, show tooltip as incompatible
				DragConnectionOp->SetToolTip(FActorDragDropGraphEdOp::ToolTip_MultipleSelection_Incompatible);
			}
		}
		else
		{
			DragConnectionOp->SetToolTip( (DragConnectionOp->Actors.Num() == 1) ? FActorDragDropGraphEdOp::ToolTip_Incompatible : FActorDragDropGraphEdOp::ToolTip_MultipleSelection_Incompatible);
		}
	}
	else if ( DragDrop::IsTypeMatch<FBoneDragDropOp>(DragDropEvent.GetOperation()) )
	{
		//@TODO: A2REMOVAL: No support for A3 nodes handling this drag-drop op yet!
	}
}

void SGraphNode::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
	// Is someone dragging a connection?
	if ( DragDrop::IsTypeMatch<FGraphEditorDragDropAction>(DragDropEvent.GetOperation()) )
	{
		// Inform the Drag and Drop operation that we are not hovering any pins
		TSharedPtr<FGraphEditorDragDropAction> DragConnectionOp = StaticCastSharedPtr<FGraphEditorDragDropAction>(DragDropEvent.GetOperation());
		DragConnectionOp->SetHoveredNode( TSharedPtr<SGraphNode>(NULL) );
	}
	else if( DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>(DragDropEvent.GetOperation()) )
	{
		//Default tool tip
		TSharedPtr<FActorDragDropGraphEdOp> DragConnectionOp = StaticCastSharedPtr<FActorDragDropGraphEdOp>(DragDropEvent.GetOperation());
		DragConnectionOp->SetToolTip(FActorDragDropGraphEdOp::ToolTip_Default);
	}
	else if( DragDrop::IsTypeMatch<FAssetDragDropOp>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FAssetDragDropOp> AssetOp = StaticCastSharedPtr<FAssetDragDropOp>(DragDropEvent.GetOperation());
		AssetOp->ClearTooltip();
	}
	else if ( DragDrop::IsTypeMatch<FBoneDragDropOp>(DragDropEvent.GetOperation()) )
	{
		//@TODO: A2REMOVAL: No support for A3 nodes handling this drag-drop op yet!
	}
}

FReply SGraphNode::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if( DragDrop::IsTypeMatch<FAssetDragDropOp>(DragDropEvent.GetOperation()) )
	{
		if(GraphNode != NULL && GraphNode->GetSchema() != NULL)
		{
			TSharedPtr<FAssetDragDropOp> AssetOp = StaticCastSharedPtr<FAssetDragDropOp>(DragDropEvent.GetOperation());
			bool bOkIcon = false;
			FString TooltipText;
			GraphNode->GetSchema()->GetAssetsNodeHoverMessage(AssetOp->AssetData, GraphNode, TooltipText, bOkIcon);
			const FSlateBrush* TooltipIcon = bOkIcon ? FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK")) : FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));;
			AssetOp->SetTooltip(TooltipText, TooltipIcon);
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

/** Given a coordinate in SGraphPanel space (i.e. panel widget space), return the same coordinate in graph space while taking zoom and panning into account */
FVector2D SGraphNode::NodeCoordToGraphCoord( const FVector2D& NodeSpaceCoordinate ) const
{
	TSharedPtr<SGraphPanel> OwnerCanvas = OwnerGraphPanelPtr.Pin();
	if (OwnerCanvas.IsValid())
	{
		//@TODO: NodeSpaceCoordinate != PanelCoordinate
		FVector2D PanelSpaceCoordinate = NodeSpaceCoordinate;
		return OwnerCanvas->PanelCoordToGraphCoord( PanelSpaceCoordinate );
	}
	else
	{
		return FVector2D::ZeroVector;
	}
}

FReply SGraphNode::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	// Is someone dropping a connection onto this node?
	if ( DragDrop::IsTypeMatch<FGraphEditorDragDropAction>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FGraphEditorDragDropAction> DragConnectionOp = StaticCastSharedPtr<FGraphEditorDragDropAction>(DragDropEvent.GetOperation());

		const FVector2D NodeAddPosition = NodeCoordToGraphCoord( MyGeometry.AbsoluteToLocal( DragDropEvent.GetScreenSpacePosition() ) );

		FReply Result = DragConnectionOp->DroppedOnNode(DragDropEvent.GetScreenSpacePosition(), NodeAddPosition);
		
		if (Result.IsEventHandled() && (GraphNode != NULL))
		{
			GraphNode->GetGraph()->NotifyGraphChanged();
		}
		return Result;
	}
	else if( DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FActorDragDropGraphEdOp> DragConnectionOp = StaticCastSharedPtr<FActorDragDropGraphEdOp>(DragDropEvent.GetOperation());
		if( CanAllowInteractionUsingDragDropOp( GraphNode, DragConnectionOp ) )
		{
			UK2Node_Literal* LiteralNode = CastChecked< UK2Node_Literal > ( GraphNode );

			//Check whether this node is already referencing the same actor
			if( LiteralNode->GetObjectRef() != DragConnectionOp->Actors[0].Get() )
			{
				//Replace literal node's object reference
				LiteralNode->SetObjectRef( DragConnectionOp->Actors[ 0 ].Get() );

				UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(CastChecked<UEdGraph>(GraphNode->GetOuter()));
				FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			}
		}
		return FReply::Handled();
	}
	else if( DragDrop::IsTypeMatch<FAssetDragDropOp>(DragDropEvent.GetOperation()) )
	{
		UEdGraphNode* Node = GetNodeObj();
		if(Node != NULL && Node->GetSchema() != NULL)
		{
			TSharedPtr<FAssetDragDropOp> AssetOp = StaticCastSharedPtr<FAssetDragDropOp>(DragDropEvent.GetOperation());
			Node->GetSchema()->DroppedAssetsOnNode(AssetOp->AssetData, DragDropEvent.GetScreenSpacePosition(), Node);
		}
		return FReply::Handled();
	} 
	else if ( DragDrop::IsTypeMatch<FBoneDragDropOp>(DragDropEvent.GetOperation()) )
	{
		//@TODO: A2REMOVAL: No support for A3 nodes handling this drag-drop op yet!
	}
	return FReply::Unhandled();
}

/**
 * The system calls this method to notify the widget that a mouse button was pressed within it. This event is bubbled.
 *
 * @param MyGeometry The Geometry of the widget receiving the event
 * @param MouseEvent Information about the input event
 *
 * @return Whether the event was handled along with possible requests for the system to take action.
 */
FReply SGraphNode::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Unhandled();
}

// The system calls this method to notify the widget that a mouse button was release within it. This event is bubbled.
FReply SGraphNode::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Unhandled();
}

// Called when a mouse button is double clicked.  Override this in derived classes
FReply SGraphNode::OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	OnDoubleClick.ExecuteIfBound(GraphNode);
	return FReply::Handled();
}

TSharedPtr<SToolTip> SGraphNode::GetToolTip()
{
	TSharedPtr<SToolTip> CurrentTooltip = SWidget::GetToolTip();
	if (!CurrentTooltip.IsValid())
	{
		TSharedPtr<SToolTip> ComplexTooltip = GetComplexTooltip();
		if (ComplexTooltip.IsValid())
		{
			SetToolTip(ComplexTooltip);
			bProvidedComplexTooltip = true;
		}
	}

	return SWidget::GetToolTip();
}

void SGraphNode::OnToolTipClosing()
{
	if (bProvidedComplexTooltip)
	{
		SetToolTip(NULL);
		bProvidedComplexTooltip = false;
	}
}

void SGraphNode::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	CachedUnscaledPosition = AllottedGeometry.AbsolutePosition/AllottedGeometry.Scale;

	SNodePanel::SNode::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

bool SGraphNode::IsSelectedExclusively() const
{
	TSharedPtr<SGraphPanel> OwnerPanel = OwnerGraphPanelPtr.Pin();

	if(!OwnerPanel->HasKeyboardFocus() || OwnerPanel->SelectionManager.GetSelectedNodes().Num() > 1)
	{
		return false;
	}

	return OwnerPanel->SelectionManager.IsNodeSelected(GraphNode);
}

/** @param OwnerPanel  The GraphPanel that this node belongs to */
void SGraphNode::SetOwner( const TSharedRef<SGraphPanel>& OwnerPanel )
{
	check( !OwnerGraphPanelPtr.IsValid() );
	OwnerGraphPanelPtr = OwnerPanel;
	GraphNode->NodeWidget = SharedThis(this);

	/*Once we have an owner, and if hide Unused pins is enabled, we need to remake our pins to drop the hidden ones*/
	if(OwnerGraphPanelPtr.Pin()->GetPinVisibility() != SGraphEditor::Pin_Show 
		&& LeftNodeBox.IsValid()
		&& RightNodeBox.IsValid())
	{
		this->LeftNodeBox->ClearChildren();
		this->RightNodeBox->ClearChildren();
		CreatePinWidgets();
	}
}

/** @param NewPosition  The Node should be relocated to this position in the graph panel */
void SGraphNode::MoveTo( const FVector2D& NewPosition )
{
	if (GraphNode)
	{
		if (!RequiresSecondPassLayout())
		{
			GraphNode->NodePosX = NewPosition.X;
			GraphNode->NodePosY = NewPosition.Y;
			GraphNode->MarkPackageDirty();
		}
	}
}

/** @return the Node's position within the graph */
FVector2D SGraphNode::GetPosition() const
{
	return FVector2D( GraphNode->NodePosX, GraphNode->NodePosY );
}

FString SGraphNode::GetEditableNodeTitle() const
{
	if (GraphNode != NULL)
	{
		// Trying to catch a non-reproducible crash in this function
		check(GraphNode->IsValidLowLevel());
	}

	if(GraphNode)
	{
		return GraphNode->GetNodeTitle(ENodeTitleType::EditableTitle);
	}
	return NSLOCTEXT("GraphEditor", "NullNode", "Null Node").ToString();

	// Get the portion of the node that is actually editable text (may be a subsection of the title, or something else entirely)
	return (GraphNode != NULL)
		? GraphNode->GetNodeTitle(ENodeTitleType::EditableTitle)
		: NSLOCTEXT("GraphEditor", "NullNode", "Null Node").ToString();
}

FText SGraphNode::GetEditableNodeTitleAsText() const
{
	FString NewString = GetEditableNodeTitle();
	return FText::FromString(NewString);
}

FString SGraphNode::GetNodeComment() const
{
	return GetNodeObj()->NodeComment;
}

UObject* SGraphNode::GetObjectBeingDisplayed() const
{
	return GetNodeObj();
}

FSlateColor SGraphNode::GetNodeTitleColor() const
{
	FLinearColor NodeTitleColor = GraphNode->IsDeprecated() ? FLinearColor::Red : GetNodeObj()->GetNodeTitleColor();
	NodeTitleColor.A = FadeCurve.GetLerp();
	return NodeTitleColor;
}

FSlateColor SGraphNode::GetNodeCommentColor() const
{
	return GetNodeObj()->GetNodeCommentColor();
}

/** @return the tooltip to display when over the node */
FText SGraphNode::GetNodeTooltip() const
{
	if (GraphNode != NULL)
	{
		FText TooltipText = FText::FromString(GraphNode->GetTooltip());

		if (UEdGraph* Graph = GraphNode->GetGraph())
		{
			// If the node resides in an intermediate graph, show the UObject name for debug purposes
			if (Graph->HasAnyFlags(RF_Transient))
			{
				TooltipText = FText::FromString(FString::Printf(TEXT("%s\n\n%s"), *GraphNode->GetName(), *TooltipText.ToString()));
			}
		}

		if (TooltipText.IsEmpty())
		{
			TooltipText =  FText::FromString(GraphNode->GetNodeTitle(ENodeTitleType::FullTitle));
		}

		return TooltipText;
	}
	else
	{
		return NSLOCTEXT("GraphEditor", "InvalidGraphNode", "<Invalid graph node>");
	}
}


/** @return the node being observed by this widget*/
UEdGraphNode* SGraphNode::GetNodeObj() const
{
	return GraphNode;
}

TSharedPtr<SGraphPanel> SGraphNode::GetOwnerPanel() const
{
	return OwnerGraphPanelPtr.Pin();
}

void SGraphNode::UpdateErrorInfo()
{
	//Check for node errors/warnings
	if (GraphNode->bHasCompilerMessage)
	{
		if (GraphNode->ErrorType <= EMessageSeverity::Error)
		{
			ErrorMsg = FString( TEXT("ERROR!") );
			ErrorColor = FEditorStyle::GetColor("ErrorReporting.BackgroundColor");
		}
		else if (GraphNode->ErrorType <= EMessageSeverity::Warning)
		{
			ErrorMsg = FString( TEXT("WARNING!") );
			ErrorColor = FEditorStyle::GetColor("ErrorReporting.WarningBackgroundColor");
		}
		else
		{
			ErrorMsg = FString( TEXT("NOTE") );
			ErrorColor = FEditorStyle::GetColor("InfoReporting.BackgroundColor");
		}
	}
	else 
	{
		ErrorColor = FLinearColor(0,0,0);
		ErrorMsg.Empty();
	}
}

TSharedPtr<SWidget>	SGraphNode::SetupErrorReporting()
{
	TSharedPtr<SErrorText> ErrorText;

	UpdateErrorInfo();

	// generate widget
	SAssignNew(ErrorText, SErrorText )
			.BackgroundColor( this, &SGraphNode::GetErrorColor )
			.ToolTipText( this, &SGraphNode::GetErrorMsgToolTip );

	ErrorReporting = ErrorText;
	ErrorReporting->SetError(ErrorMsg);

	return ErrorText;
}

TSharedRef<SWidget> SGraphNode::CreateTitleWidget(TSharedPtr<SNodeTitle> NodeTitle)
{
	return SAssignNew(InlineEditableText, SInlineEditableTextBlock)
		.Style(FEditorStyle::Get(), "Graph.Node.NodeTitleInlineEditableText")
		.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
		.OnVerifyTextChanged(this, &SGraphNode::OnVerifyNameTextChanged)
		.OnTextCommitted(this, &SGraphNode::OnNameTextCommited)
		.IsReadOnly(this, &SGraphNode::IsNameReadOnly)
		.IsSelected(this, &SGraphNode::IsSelectedExclusively);
}

/**
 * Update this GraphNode to match the data that it is observing
 */
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGraphNode::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();
	
	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	//
	//             ______________________
	//            |      TITLE AREA      |
	//            +-------+------+-------+
	//            | (>) L |      | R (>) |
	//            | (>) E |      | I (>) |
	//            | (>) F |      | G (>) |
	//            | (>) T |      | H (>) |
	//            |       |      | T (>) |
	//            |_______|______|_______|
	//
	TSharedPtr<SVerticalBox> MainVerticalBox;
	TSharedPtr<SWidget> ErrorText = SetupErrorReporting();

	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	// Get node icon
	FLinearColor IconColor = FLinearColor::White;
	const FSlateBrush* IconBrush = NULL;
	if (GraphNode != NULL && GraphNode->ShowPaletteIconOnNode())
	{
		IconBrush = FEditorStyle::GetBrush(GraphNode->GetPaletteIcon(IconColor));
	}

	TSharedRef<SOverlay> DefaultTitleAreaWidget =
		SNew(SOverlay)
		+SOverlay::Slot()
		[
			SNew(SImage)
			.Image( FEditorStyle::GetBrush("Graph.Node.TitleGloss") )
		]
		+SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("Graph.Node.ColorSpill") )
			// The extra margin on the right
			// is for making the color spill stretch well past the node title
			.Padding( FMargin(10,5,30,3) )
			.BorderBackgroundColor( this, &SGraphNode::GetNodeTitleColor )
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Top)
				.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
				.AutoWidth()
				[
					SNew(SImage)
					.Image(IconBrush)
					.ColorAndOpacity(IconColor)
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						CreateTitleWidget(NodeTitle)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						NodeTitle.ToSharedRef()
					]
				]
			]
		]
		+SOverlay::Slot()
		.VAlign(VAlign_Top)
		[
			SNew(SBorder)
			.Visibility(EVisibility::HitTestInvisible)			
			.BorderImage( FEditorStyle::GetBrush( "Graph.Node.TitleHighlight" ) )
			[
				SNew(SSpacer)
				.Size(FVector2D(20,20))
			]
		];

	SetDefaultTitleAreaWidget(DefaultTitleAreaWidget);

	TSharedRef<SWidget> TitleAreaWidget = 
		SNew(SLevelOfDetailBranchNode)
		.UseLowDetailSlot(this, &SGraphNode::UseLowDetailNodeTitles)
		.LowDetail()
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("Graph.Node.ColorSpill") )
			.Padding( FMargin(75.0f, 22.0f) ) // Saving enough space for a 'typical' title so the transition isn't quite so abrupt
			.BorderBackgroundColor( this, &SGraphNode::GetNodeTitleColor )
		]
		.HighDetail()
		[
			DefaultTitleAreaWidget
		];

	
	if (!SWidget::GetToolTip().IsValid())
	{
		TSharedRef<SToolTip> DefaultToolTip = IDocumentation::Get()->CreateToolTip( TAttribute< FText >( this, &SGraphNode::GetNodeTooltip ), NULL, GraphNode->GetDocumentationLink(), GraphNode->GetDocumentationExcerptName() );
		SetToolTip(DefaultToolTip);
	}

	TSharedPtr<SVerticalBox> InnerVerticalBox;
	this->ContentScale.Bind( this, &SGraphNode::GetContentScale );
	this->ChildSlot
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(MainVerticalBox, SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage( FEditorStyle::GetBrush( "Graph.Node.Body" ) )
				.Padding(0)
				[
					SAssignNew(InnerVerticalBox, SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Top)
					[
						TitleAreaWidget
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
					.VAlign(VAlign_Top)
					[
						CreateNodeContentArea()
					]
				]
			]			
		];

	CreateBelowWidgetControls(MainVerticalBox);
	CreatePinWidgets();
	CreateBelowPinControls(InnerVerticalBox);
	CreateAdvancedViewArrow(InnerVerticalBox);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


TSharedRef<SWidget> SGraphNode::CreateNodeContentArea()
{
	// NODE CONTENT AREA
	return SNew(SBorder)
		.BorderImage( FEditorStyle::GetBrush("NoBorder") )
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding( FMargin(0,3) )
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.FillWidth(1.0f)
			[
				// LEFT
				SAssignNew(LeftNodeBox, SVerticalBox)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				// RIGHT
				SAssignNew(RightNodeBox, SVerticalBox)
			]
		];
}

/** Returns visibility of AdvancedViewButton */
EVisibility SGraphNode::AdvancedViewArrowVisibility() const
{
	const bool bShowAdvancedViewArrow = GraphNode && (ENodeAdvancedPins::NoPins != GraphNode->AdvancedPinDisplay);
	return bShowAdvancedViewArrow ? EVisibility::Visible : EVisibility::Collapsed;
}

void SGraphNode::OnAdvancedViewChanged( const ESlateCheckBoxState::Type NewCheckedState )
{
	if(GraphNode && (ENodeAdvancedPins::NoPins != GraphNode->AdvancedPinDisplay))
	{
		const bool bAdvancedPinsHidden = (NewCheckedState != ESlateCheckBoxState::Checked);
		GraphNode->AdvancedPinDisplay = bAdvancedPinsHidden ? ENodeAdvancedPins::Hidden : ENodeAdvancedPins::Shown;
	}
}

ESlateCheckBoxState::Type SGraphNode::IsAdvancedViewChecked() const
{
	const bool bAdvancedPinsHidden = GraphNode && (ENodeAdvancedPins::Hidden == GraphNode->AdvancedPinDisplay);
	return bAdvancedPinsHidden ? ESlateCheckBoxState::Unchecked : ESlateCheckBoxState::Checked;
}

const FSlateBrush* SGraphNode::GetAdvancedViewArrow() const
{
	const bool bAdvancedPinsHidden = GraphNode && (ENodeAdvancedPins::Hidden == GraphNode->AdvancedPinDisplay);
	return FEditorStyle::GetBrush(bAdvancedPinsHidden ? TEXT("Kismet.TitleBarEditor.ArrowDown") : TEXT("Kismet.TitleBarEditor.ArrowUp"));
}

/** Create widget to show/hide advanced pins */
void SGraphNode::CreateAdvancedViewArrow(TSharedPtr<SVerticalBox> MainBox)
{
	const bool bHidePins = OwnerGraphPanelPtr.IsValid() && (OwnerGraphPanelPtr.Pin()->GetPinVisibility() != SGraphEditor::Pin_Show);
	const bool bAnyAdvancedPin = GraphNode && (ENodeAdvancedPins::NoPins != GraphNode->AdvancedPinDisplay);
	if(!bHidePins && GraphNode && MainBox.IsValid())
	{
		MainBox->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.Padding(3, 0, 3, 3)
		[
			SNew(SCheckBox)
			.Visibility(this, &SGraphNode::AdvancedViewArrowVisibility)
			.OnCheckStateChanged( this, &SGraphNode::OnAdvancedViewChanged )
			.IsChecked( this, &SGraphNode::IsAdvancedViewChecked )
			.Cursor(EMouseCursor::Default)
			.Style(FEditorStyle::Get(), "Graph.Node.AdvancedView")
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					. Image(this, &SGraphNode::GetAdvancedViewArrow)
				]
			]
		];
	}
}

void SGraphNode::CreateStandardPinWidget(UEdGraphPin* CurPin)
{
	const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(GraphNode->GetSchema());

	bool bHideNoConnectionPins = false;
	bool bHideNoConnectionNoDefaultPins = false;

	// Not allowed to hide exec pins 
	const bool bCanHidePin = (K2Schema && (CurPin->PinType.PinCategory != K2Schema->PC_Exec));

	if (OwnerGraphPanelPtr.IsValid() && bCanHidePin)
	{
		bHideNoConnectionPins = OwnerGraphPanelPtr.Pin()->GetPinVisibility() == SGraphEditor::Pin_HideNoConnection;
		bHideNoConnectionNoDefaultPins = OwnerGraphPanelPtr.Pin()->GetPinVisibility() == SGraphEditor::Pin_HideNoConnectionNoDefault;
	}

	const bool bIsOutputPin= CurPin->Direction == EGPD_Output;
	const bool bPinHasDefaultValue = !CurPin->DefaultValue.IsEmpty() || (CurPin->DefaultObject != NULL);
	const bool bIsSelfTarget = K2Schema && (CurPin->PinType.PinCategory == K2Schema->PC_Object) && (CurPin->PinName == K2Schema->PN_Self);
	const bool bPinHasValidDefault = !bIsOutputPin && (bPinHasDefaultValue || bIsSelfTarget);
	const bool bPinHasConections = CurPin->LinkedTo.Num() > 0;

	const bool bPinDesiresToBeHidden = CurPin->bHidden || (bHideNoConnectionPins && !bPinHasConections) || (bHideNoConnectionNoDefaultPins && !bPinHasConections && !bPinHasValidDefault); 

	// No matter how strong the desire, a pin with connections can never be hidden!
	const bool bShowPin = !bPinDesiresToBeHidden || bPinHasConections;

	if (bShowPin)
	{
		TSharedPtr<SGraphPin> NewPin = CreatePinWidget(CurPin);
		check(NewPin.IsValid());
		NewPin->SetIsEditable(IsEditable);

		this->AddPin(NewPin.ToSharedRef());
	}
}

void SGraphNode::CreatePinWidgets()
{
	// Create Pin widgets for each of the pins.
	for (int32 PinIndex = 0; PinIndex < GraphNode->Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* CurPin = GraphNode->Pins[PinIndex];

		CreateStandardPinWidget(CurPin);
	}
}

TSharedPtr<SGraphPin> SGraphNode::CreatePinWidget(UEdGraphPin* Pin) const
{
	return FNodeFactory::CreatePinWidget(Pin);
}

/**
 * Add a new pin to this graph node. The pin must be newly created.
 *
 * @param PinToAdd   A new pin to add to this GraphNode.
 */
void SGraphNode::AddPin( const TSharedRef<SGraphPin>& PinToAdd )
{	
	PinToAdd->SetOwner( SharedThis(this) );

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
	if(bAdvancedParameter)
	{
		PinToAdd->SetVisibility( TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced) );
	}
	
	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		LeftNodeBox->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(10,4)
		[
			PinToAdd
		];
		InputPins.Add(PinToAdd);
	}
	else // Direction == EEdGraphPinDirection::EGPD_Output
	{
		RightNodeBox->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(10,4)
		[
			PinToAdd
		];
		OutputPins.Add(PinToAdd);
	}
}

/**
 * Get all the pins found on this node.
 *
 * @param AllPins  The set of pins found on this node.
 */
void SGraphNode::GetPins( TSet< TSharedRef<SWidget> >& AllPins ) const
{

	for( int32 PinIndex=0; PinIndex < this->InputPins.Num(); ++PinIndex )
	{
		AllPins.Add(InputPins[PinIndex]);
	}


	for( int32 PinIndex=0; PinIndex < this->OutputPins.Num(); ++PinIndex )
	{
		AllPins.Add(OutputPins[PinIndex]);
	}

}

void SGraphNode::GetPins( TArray< TSharedRef<SWidget> >& AllPins ) const
{

	for( int32 PinIndex=0; PinIndex < this->InputPins.Num(); ++PinIndex )
	{
		AllPins.Add(InputPins[PinIndex]);
	}


	for( int32 PinIndex=0; PinIndex < this->OutputPins.Num(); ++PinIndex )
	{
		AllPins.Add(OutputPins[PinIndex]);
	}

}

TSharedPtr<SGraphPin> SGraphNode::GetHoveredPin( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) const
{
	// We just need to find the one WidgetToFind among our descendants.
	TSet< TSharedRef<SWidget> > MyPins;
	{
		GetPins( MyPins );
	}
	TMap<TSharedRef<SWidget>, FArrangedWidget> Result;

	FindChildGeometries(MyGeometry, MyPins, Result);
	
	if ( Result.Num() > 0 )
	{
		FArrangedChildren ArrangedPins(EVisibility::Visible);
		Result.GenerateValueArray( ArrangedPins.GetInternalArray() );
		int32 HoveredPinIndex = SWidget::FindChildUnderMouse( ArrangedPins, MouseEvent );
		if ( HoveredPinIndex != INDEX_NONE )
		{
			return StaticCastSharedRef<SGraphPin>(ArrangedPins(HoveredPinIndex).Widget);
		}
	}
	
	return TSharedPtr<SGraphPin>();
}

TSharedPtr<SGraphPin> SGraphNode::FindWidgetForPin( UEdGraphPin* ThePin ) const
{
	// Search input or output pins?
	const TArray< TSharedRef<SGraphPin> > &PinsToSearch = (ThePin->Direction == EGPD_Input) ? InputPins : OutputPins;

	// Actually search for the widget
	for( int32 PinIndex=0; PinIndex < PinsToSearch.Num(); ++PinIndex )
	{
		if ( PinsToSearch[PinIndex]->GetPinObj() == ThePin )
		{
			return PinsToSearch[PinIndex];
		}
	}

	return TSharedPtr<SGraphPin>(NULL);
}

void SGraphNode::PlaySpawnEffect()
{
	SpawnAnim.Play();
}

FVector2D SGraphNode::GetContentScale() const
{
	const float CurZoomValue = ZoomCurve.GetLerp();
	return FVector2D( CurZoomValue, CurZoomValue );
}

FLinearColor SGraphNode::GetColorAndOpacity() const
{
	return FLinearColor(1,1,1,FadeCurve.GetLerp());
}

FLinearColor SGraphNode::GetPinLabelColorAndOpacity() const
{
	return FLinearColor(0,0,0,FadeCurve.GetLerp());
}


SGraphNode::SGraphNode()
	: bProvidedComplexTooltip(false)
	, ErrorColor( FLinearColor::White )
	, bRenameIsPending( false )
	, CachedUnscaledPosition( FVector2D::ZeroVector )
{
	// Set up animation
	{
		ZoomCurve = SpawnAnim.AddCurve(0, 0.1f);
		FadeCurve = SpawnAnim.AddCurve(0.15f, 0.15f);
		SpawnAnim.JumpToEnd();
	}
}

void SGraphNode::PositionThisNodeBetweenOtherNodes(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup, UEdGraphNode* PreviousNode, UEdGraphNode* NextNode, float HeightAboveWire) const
{
	if ((PreviousNode != NULL) && (NextNode != NULL))
	{
		TSet<UEdGraphNode*> PrevNodes;
		PrevNodes.Add(PreviousNode);

		TSet<UEdGraphNode*> NextNodes;
		NextNodes.Add(NextNode);

		PositionThisNodeBetweenOtherNodes(NodeToWidgetLookup, PrevNodes, NextNodes, HeightAboveWire);
	}
}

void SGraphNode::PositionThisNodeBetweenOtherNodes(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup, TSet<UEdGraphNode*>& PreviousNodes, TSet<UEdGraphNode*>& NextNodes, float HeightAboveWire) const
{
	// Find the previous position centroid
	FVector2D PrevPos(0.0f, 0.0f);
	for (auto NodeIt = PreviousNodes.CreateConstIterator(); NodeIt; ++NodeIt)
	{
		UEdGraphNode* PreviousNode = *NodeIt;
		const FVector2D CornerPos(PreviousNode->NodePosX, PreviousNode->NodePosY);
		PrevPos += CornerPos + NodeToWidgetLookup.FindChecked(PreviousNode)->GetDesiredSize() * 0.5f;
	}

	// Find the next position centroid
	FVector2D NextPos(0.0f, 0.0f);
	for (auto NodeIt = NextNodes.CreateConstIterator(); NodeIt; ++NodeIt)
	{
		UEdGraphNode* NextNode = *NodeIt;
		const FVector2D CornerPos(NextNode->NodePosX, NextNode->NodePosY);
		NextPos += CornerPos + NodeToWidgetLookup.FindChecked(NextNode)->GetDesiredSize() * 0.5f;
	}

	PositionThisNodeBetweenOtherNodes(PrevPos, NextPos, HeightAboveWire);
}

void SGraphNode::PositionThisNodeBetweenOtherNodes(const FVector2D& PrevPos, const FVector2D& NextPos, float HeightAboveWire) const
{
	const FVector2D DesiredSize = GetDesiredSize();

	FVector2D DeltaPos(NextPos - PrevPos);
	if (DeltaPos.IsNearlyZero())
	{
		DeltaPos = FVector2D(10.0f, 0.0f);
	}

	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).SafeNormal();

	const FVector2D SlidingCapsuleBias = FVector2D::ZeroVector;//(0.5f * FMath::Sin(Normal.X * (float)HALF_PI) * DesiredSize.X, 0.0f);

	const FVector2D NewCenter = PrevPos + (0.5f * DeltaPos) + (HeightAboveWire * Normal) + SlidingCapsuleBias;

	// Now we need to adjust the new center by the node size and zoom factor
	const FVector2D NewCorner = NewCenter - (0.5f * DesiredSize);

	GraphNode->NodePosX = NewCorner.X;
	GraphNode->NodePosY = NewCorner.Y;
}

FString SGraphNode::GetErrorMsgToolTip( ) const
{
	return GraphNode->ErrorMsg;
}

bool SGraphNode::ShouldScaleNodeComment() const
{
	return true;
}

bool SGraphNode::IsNameReadOnly() const
{
	return !GraphNode->bCanRenameNode;
}

bool SGraphNode::OnVerifyNameTextChanged ( const FText& InText, FText& OutErrorMessage ) 
{
	bool bValid(true);

	if( GetEditableNodeTitle() != InText.ToString() && OnVerifyTextCommit.IsBound() )
	{
		bValid = OnVerifyTextCommit.Execute(InText, GraphNode);
	}

	OutErrorMessage = FText::FromString(TEXT("Error"));

	//UpdateErrorInfo();
	//ErrorReporting->SetError(ErrorMsg);

	return bValid;
}

void SGraphNode::OnNameTextCommited ( const FText& InText, ETextCommit::Type CommitInfo ) 
{
	OnTextCommitted.ExecuteIfBound(InText, CommitInfo, GraphNode);
	
	UpdateErrorInfo();
	ErrorReporting->SetError(ErrorMsg);
}

void SGraphNode::RequestRename()
{
	if ((GraphNode != NULL) && GraphNode->bCanRenameNode)
	{
		bRenameIsPending = true;
	}
}

void SGraphNode::ApplyRename()
{
	if (bRenameIsPending)
	{
		bRenameIsPending = false;
		InlineEditableText->EnterEditingMode();
	}
}

FSlateRect SGraphNode::GetTitleRect() const
{
	const FVector2D NodePosition = GetPosition();
	const FVector2D NodeSize = GraphNode ? InlineEditableText->GetDesiredSize() : GetDesiredSize();

	return FSlateRect( NodePosition.X, NodePosition.Y + NodeSize.Y, NodePosition.X + NodeSize.X, NodePosition.Y );
}

void SGraphNode::NotifyDisallowedPinConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	OnDisallowedPinConnection.ExecuteIfBound(PinA, PinB);
}

bool SGraphNode::UseLowDetailNodeTitles() const
{
	if (const SGraphPanel* MyOwnerPanel = GetOwnerPanel().Get())
	{
		return (MyOwnerPanel->GetCurrentLOD() <= EGraphRenderingLOD::LowestDetail) && !InlineEditableText->IsInEditMode();
	}
	else
	{
		return false;
	}
}

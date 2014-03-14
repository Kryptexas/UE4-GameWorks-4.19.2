// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GraphEditorCommon.h"
#include "SGraphNodeMaterialBase.h"
#include "ScopedTransaction.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"

/**
* Simple representation of the backbuffer that the preview canvas renders to
* This class may only be accessed from the render thread
*/
class FSlateBackBufferTarget : public FRenderTarget
{
public:
	/** FRenderTarget interface */
	virtual FIntPoint GetSizeXY() const
	{
		return ClippingRect.Size();
	}

	/** Sets the texture that this target renders to */
	void SetRenderTargetTexture( FTexture2DRHIRef& InRHIRef )
	{
		RenderTargetTextureRHI = InRHIRef;
	}

	/** Clears the render target texture */
	void ClearRenderTargetTexture()
	{
		RenderTargetTextureRHI.SafeRelease();
	}

	/** Sets the viewport rect for the render target */
	void SetViewRect( const FIntRect& InViewRect ) 
	{ 
		ViewRect = InViewRect;
	}

	/** Gets the viewport rect for the render target */
	const FIntRect& GetViewRect() const 
	{
		return ViewRect;
	}

	/** Sets the clipping rect for the render target */
	void SetClippingRect( const FIntRect& InClippingRect ) 
	{ 
		ClippingRect = InClippingRect;
	}

	/** Gets the clipping rect for the render target */
	const FIntRect& GetClippingRect() const 
	{
		return ClippingRect;
	}
private:
	FIntRect ViewRect;
	FIntRect ClippingRect;
};

/*-----------------------------------------------------------------------------
   FPreviewViewport
-----------------------------------------------------------------------------*/

FPreviewViewport::FPreviewViewport(class UMaterialGraphNode* InNode)
	: MaterialNode(InNode)
	, PreviewElement( new FPreviewElement )
{
}

FPreviewViewport::~FPreviewViewport()
{
	// Pass the preview element to the render thread so that it's deleted after it's shown for the last time
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
	(
		SafeDeletePreviewElement,
		FThreadSafePreviewPtr, PreviewElementPtr, PreviewElement,
		{
			PreviewElementPtr.Reset();
		}
	);
}

void FPreviewViewport::OnDrawViewport( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, class FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled )
{
	FSlateRect SlateCanvasRect = AllottedGeometry.GetClippingRect();
	FSlateRect ClippedCanvasRect = SlateCanvasRect.IntersectionWith(MyClippingRect);

	FIntRect CanvasRect(
		FMath::Trunc( FMath::Max(0.0f, SlateCanvasRect.Left) ),
		FMath::Trunc( FMath::Max(0.0f, SlateCanvasRect.Top) ),
		FMath::Trunc( FMath::Max(0.0f, SlateCanvasRect.Right) ), 
		FMath::Trunc( FMath::Max(0.0f, SlateCanvasRect.Bottom) ) );

	FIntRect ClippingRect(
		FMath::Trunc( FMath::Max(0.0f, ClippedCanvasRect.Left) ),
		FMath::Trunc( FMath::Max(0.0f, ClippedCanvasRect.Top) ),
		FMath::Trunc( FMath::Max(0.0f, ClippedCanvasRect.Right) ), 
		FMath::Trunc( FMath::Max(0.0f, ClippedCanvasRect.Bottom) ) );

	bool bIsRealtime = MaterialNode->RealtimeDelegate.IsBound() ? MaterialNode->RealtimeDelegate.Execute() : false;

	if (PreviewElement->BeginRenderingCanvas( CanvasRect, ClippingRect, MaterialNode->GetExpressionPreview(), bIsRealtime ))
	{
		// Draw above everything else
		uint32 PreviewLayer = MAX_uint32;//LayerId+1;//
		FSlateDrawElement::MakeCustom( OutDrawElements, PreviewLayer, PreviewElement );
	}
}

FIntPoint FPreviewViewport::GetSize() const
{
	return FIntPoint(96,96);
}

/////////////////////////////////////////////////////
// FPreviewElement

FPreviewElement::FPreviewElement()
	: ExpressionPreview(NULL)
	, RenderTarget(new FSlateBackBufferTarget)
	, bIsRealtime(false)
{
}

FPreviewElement::~FPreviewElement()
{
	delete RenderTarget;
}

bool FPreviewElement::BeginRenderingCanvas( const FIntRect& InCanvasRect, const FIntRect& InClippingRect, FMaterialRenderProxy* InExpressionPreview, bool bInIsRealtime )
{
	if(InCanvasRect.Size().X > 0 && InCanvasRect.Size().Y > 0 && InClippingRect.Size().X > 0 && InClippingRect.Size().Y > 0 && InExpressionPreview != NULL)
	{
		/**
		 * Struct to contain all info that needs to be passed to the render thread
		 */
		struct FPreviewRenderInfo
		{
			/** Size of the Canvas tile */
			FIntRect CanvasRect;
			/** How to clip the canvas tile */
			FIntRect ClippingRect;
			/** Render proxy for the expression preview */
			FMaterialRenderProxy* ExpressionPreview;
			/** Whether preview is using realtime values */
			bool bIsRealtime;
		};

		FPreviewRenderInfo RenderInfo;
		RenderInfo.CanvasRect = InCanvasRect;
		RenderInfo.ClippingRect = InClippingRect;
		RenderInfo.ExpressionPreview = InExpressionPreview;
		RenderInfo.bIsRealtime = bInIsRealtime;

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
			(
			BeginRenderingPreviewCanvas,
			FPreviewElement*, PreviewElement, this, 
			FPreviewRenderInfo, InRenderInfo, RenderInfo,
		{
			PreviewElement->RenderTarget->SetViewRect(InRenderInfo.CanvasRect);
			PreviewElement->RenderTarget->SetClippingRect(InRenderInfo.ClippingRect);
			PreviewElement->ExpressionPreview = InRenderInfo.ExpressionPreview;
			PreviewElement->bIsRealtime = InRenderInfo.bIsRealtime;
		}
		);
		return true;
	}

	return false;
}

void FPreviewElement::DrawRenderThread( const void* InWindowBackBuffer )
{
	// Clip the canvas to avoid having to set UV values
	FIntRect ClippingRect = RenderTarget->GetClippingRect();
	RHISetScissorRect(	true,
						ClippingRect.Min.X,
						ClippingRect.Min.Y,
						ClippingRect.Max.X,
						ClippingRect.Max.Y);
	RenderTarget->SetRenderTargetTexture( *(FTexture2DRHIRef*)InWindowBackBuffer );
	{
		// Check realtime mode for whether to pass current time to canvas
		float CurrentTime = bIsRealtime ? (GCurrentTime - GStartTime) : 0.0f;
		float DeltaTime = bIsRealtime ? GDeltaTime : 0.0f;

		FCanvas Canvas(RenderTarget, NULL, CurrentTime, CurrentTime, DeltaTime);
		{
			Canvas.SetAllowedModes( 0 );
			Canvas.SetRenderTargetRect( RenderTarget->GetViewRect() );

			FCanvasTileItem TileItem( FVector2D::ZeroVector, ExpressionPreview , RenderTarget->GetSizeXY());
			Canvas.DrawItem( TileItem );
		}
		Canvas.Flush(true);
	}
	RenderTarget->ClearRenderTargetTexture();
	RHISetScissorRect(false,0,0,0,0);
}

/////////////////////////////////////////////////////
// SGraphNodeMaterialBase

void SGraphNodeMaterialBase::Construct(const FArguments& InArgs, UMaterialGraphNode* InNode)
{
	this->GraphNode = InNode;
	this->MaterialNode = InNode;

	this->SetCursor(EMouseCursor::CardinalCross);

	// Surely we don't need to do this as SGraphPanel calls it after construction
	//this->UpdateGraphNode();
}

void SGraphNodeMaterialBase::CreatePinWidgets()
{
	// Create Pin widgets for each of the pins.
	for( int32 PinIndex=0; PinIndex < GraphNode->Pins.Num(); ++PinIndex )
	{
		UEdGraphPin* CurPin = GraphNode->Pins[PinIndex];

		bool bHideNoConnectionPins = false;

		if (OwnerGraphPanelPtr.IsValid())
		{
			bHideNoConnectionPins = OwnerGraphPanelPtr.Pin()->GetPinVisibility() == SGraphEditor::Pin_HideNoConnection;
		}

		const bool bPinHasConections = CurPin->LinkedTo.Num() > 0;

		const bool bPinDesiresToBeHidden = CurPin->bHidden || (bHideNoConnectionPins && !bPinHasConections); 

		if (!bPinDesiresToBeHidden)
		{
			TSharedPtr<SGraphPin> NewPin = CreatePinWidget(CurPin);
			check(NewPin.IsValid());
			NewPin->SetIsEditable(IsEditable);

			this->AddPin(NewPin.ToSharedRef());
		}
	}
}

void SGraphNodeMaterialBase::MoveTo(const FVector2D& NewPosition)
{
	SGraphNode::MoveTo(NewPosition);

	MaterialNode->MaterialExpression->MaterialExpressionEditorX = MaterialNode->NodePosX;
	MaterialNode->MaterialExpression->MaterialExpressionEditorY = MaterialNode->NodePosY;
	MaterialNode->MaterialExpression->MarkPackageDirty();
	MaterialNode->MaterialDirtyDelegate.ExecuteIfBound();
}

void SGraphNodeMaterialBase::AddPin( const TSharedRef<SGraphPin>& PinToAdd )
{
	PinToAdd->SetOwner( SharedThis(this) );

	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		LeftNodeBox->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(5,4,0,4)
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
		.Padding(0,4,5,4)
		[
			PinToAdd
		];
		OutputPins.Add(PinToAdd);
	}
}

void SGraphNodeMaterialBase::CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox)
{
	if (GraphNode && MainBox.IsValid())
	{
		int32 LeftPinCount = InputPins.Num();
		int32 RightPinCount = OutputPins.Num();

		// Place preview widget based on where the least pins are
		if (LeftPinCount < RightPinCount || RightPinCount == 0)
		{
			LeftNodeBox->AddSlot()
			.AutoHeight()
			[
				CreatePreviewWidget()
			];
		}
		else if (LeftPinCount > RightPinCount)
		{
			RightNodeBox->AddSlot()
			.AutoHeight()
			[
				CreatePreviewWidget()
			];
		}
		else
		{
			MainBox->AddSlot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					CreatePreviewWidget()
				]
			];
		}
	}
}

void SGraphNodeMaterialBase::SetDefaultTitleAreaWidget(TSharedRef<SOverlay> DefaultTitleAreaWidget)
{
	if (!MaterialNode->MaterialExpression->bHidePreviewWindow)
	{
		DefaultTitleAreaWidget->AddSlot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(FMargin(5))
		[
			SNew(SCheckBox)
			.OnCheckStateChanged( this, &SGraphNodeMaterialBase::OnExpressionPreviewChanged )
			.IsChecked( IsExpressionPreviewChecked() )
			.Cursor(EMouseCursor::Default)
			.Style(FEditorStyle::Get(), "Graph.Node.AdvancedView")
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					. Image(GetExpressionPreviewArrow())
				]
			]
		];
	}
}

TSharedRef<SWidget> SGraphNodeMaterialBase::CreateNodeContentArea()
{
	// NODE CONTENT AREA
	return SNew(SBorder)
		.BorderImage( FEditorStyle::GetBrush("NoBorder") )
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
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

TSharedRef<SWidget> SGraphNodeMaterialBase::CreatePreviewWidget()
{
	PreviewViewport.Reset();

	// if this node should currently show a preview
	if (!MaterialNode->MaterialExpression->bHidePreviewWindow && !MaterialNode->MaterialExpression->bCollapsed)
	{
		const float ExpressionPreviewSize = 106.0f;
		const float CentralPadding = 5.0f;

		TSharedPtr<SViewport> ViewportWidget = 
			SNew( SViewport )
			.EnableGammaCorrection(false);

		PreviewViewport = MakeShareable(new FPreviewViewport(MaterialNode));

		// The viewport widget needs an interface so it knows what should render
		ViewportWidget->SetViewportInterface( PreviewViewport.ToSharedRef() );

		return SNew(SBox)
			.WidthOverride(ExpressionPreviewSize)
			.HeightOverride(ExpressionPreviewSize)
			.Visibility(ExpressionPreviewVisibility())
			[
				SNew(SBorder)
				.Padding(CentralPadding)
				.BorderImage( FEditorStyle::GetBrush("NoBorder") )
				[
					ViewportWidget.ToSharedRef()
				]
			];
	}

	return SNullWidget::NullWidget;
}

EVisibility SGraphNodeMaterialBase::ExpressionPreviewVisibility() const
{
	UMaterialExpression* MaterialExpression = MaterialNode->MaterialExpression;
	const bool bShowPreview = !MaterialExpression->bHidePreviewWindow && !MaterialExpression->bCollapsed;
	return bShowPreview ? EVisibility::Visible : EVisibility::Collapsed;
}

void SGraphNodeMaterialBase::OnExpressionPreviewChanged( const ESlateCheckBoxState::Type NewCheckedState )
{
	UMaterialExpression* MaterialExpression = MaterialNode->MaterialExpression;
	const bool bCollapsed = (NewCheckedState != ESlateCheckBoxState::Checked);
	if (MaterialExpression->bCollapsed != bCollapsed)
	{
		UMaterialGraph* MaterialGraph = CastChecked<UMaterialGraph>(MaterialNode->GetGraph());
		MaterialGraph->ToggleCollapsedDelegate.ExecuteIfBound(MaterialExpression);

		// Update the graph node so that preview viewport is created
		UpdateGraphNode();
	}
}

ESlateCheckBoxState::Type SGraphNodeMaterialBase::IsExpressionPreviewChecked() const
{
	return MaterialNode->MaterialExpression->bCollapsed ? ESlateCheckBoxState::Unchecked : ESlateCheckBoxState::Checked;
}

const FSlateBrush* SGraphNodeMaterialBase::GetExpressionPreviewArrow() const
{
	return FEditorStyle::GetBrush(MaterialNode->MaterialExpression->bCollapsed ? TEXT("Kismet.TitleBarEditor.ArrowDown") : TEXT("Kismet.TitleBarEditor.ArrowUp"));
}

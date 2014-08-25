// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "SDesignerView.h"

#include "BlueprintEditor.h"
#include "SKismetInspector.h"
#include "BlueprintEditorUtils.h"

#include "WidgetTemplateDragDropOp.h"
#include "SZoomPan.h"

#define LOCTEXT_NAMESPACE "UMG"

const float HoveredAnimationTime = 0.150f;

class FSelectedWidgetDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FSelectedWidgetDragDropOp, FDecoratedDragDropOp)

	TMap<FName, FString> ExportedSlotProperties;

	FWidgetReference Widget;

	static TSharedRef<FSelectedWidgetDragDropOp> New(FWidgetReference InWidget);
};

TSharedRef<FSelectedWidgetDragDropOp> FSelectedWidgetDragDropOp::New(FWidgetReference InWidget)
{
	TSharedRef<FSelectedWidgetDragDropOp> Operation = MakeShareable(new FSelectedWidgetDragDropOp());
	Operation->Widget = InWidget;
	Operation->DefaultHoverText = FText::FromString( InWidget.GetTemplate()->GetLabel() );
	Operation->CurrentHoverText = FText::FromString( InWidget.GetTemplate()->GetLabel() );
	Operation->Construct();

	FWidgetBlueprintEditorUtils::ExportPropertiesToText(InWidget.GetTemplate()->Slot, Operation->ExportedSlotProperties);

	return Operation;
}

//////////////////////////////////////////////////////////////////////////

static bool LocateWidgetsUnderCursor_Helper(FArrangedWidget& Candidate, FVector2D InAbsoluteCursorLocation, FArrangedChildren& OutWidgetsUnderCursor, bool bIgnoreEnabledStatus)
{
	const bool bCandidateUnderCursor =
		// Candidate is physically under the cursor
		Candidate.Geometry.IsUnderLocation(InAbsoluteCursorLocation) &&
		// Candidate actually considers itself hit by this test
		( Candidate.Widget->OnHitTest(Candidate.Geometry, InAbsoluteCursorLocation) );

	bool bHitAnyWidget = false;
	if ( bCandidateUnderCursor )
	{
		// The candidate widget is under the mouse
		OutWidgetsUnderCursor.AddWidget(Candidate);

		// Check to see if we were asked to still allow children to be hit test visible
		bool bHitChildWidget = false;
		
		if ( Candidate.Widget->GetVisibility().AreChildrenHitTestVisible() )//!= 0 || OutWidgetsUnderCursor. )
		{
			FArrangedChildren ArrangedChildren(OutWidgetsUnderCursor.GetFilter());
			Candidate.Widget->ArrangeChildren(Candidate.Geometry, ArrangedChildren);

			// A widget's children are implicitly Z-ordered from first to last
			for ( int32 ChildIndex = ArrangedChildren.Num() - 1; !bHitChildWidget && ChildIndex >= 0; --ChildIndex )
			{
				FArrangedWidget& SomeChild = ArrangedChildren[ChildIndex];
				bHitChildWidget = ( SomeChild.Widget->IsEnabled() || bIgnoreEnabledStatus ) && LocateWidgetsUnderCursor_Helper(SomeChild, InAbsoluteCursorLocation, OutWidgetsUnderCursor, bIgnoreEnabledStatus);
			}
		}

		// If we hit a child widget or we hit our candidate widget then we'll append our widgets
		const bool bHitCandidateWidget = OutWidgetsUnderCursor.Accepts(Candidate.Widget->GetVisibility()) &&
			Candidate.Widget->GetVisibility().AreChildrenHitTestVisible();
		
		bHitAnyWidget = bHitChildWidget || bHitCandidateWidget;
		if ( !bHitAnyWidget )
		{
			// No child widgets were hit, and even though the cursor was over our candidate widget, the candidate
			// widget was not hit-testable, so we won't report it
			check(OutWidgetsUnderCursor.Last() == Candidate);
			OutWidgetsUnderCursor.Remove(OutWidgetsUnderCursor.Num() - 1);
		}
	}

	return bHitAnyWidget;
}

/////////////////////////////////////////////////////
// SDesignerView

void SDesignerView::Construct(const FArguments& InArgs, TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor)
{
	ScopedTransaction = NULL;

	PreviewWidget = NULL;
	DropPreviewWidget = NULL;
	DropPreviewParent = NULL;
	BlueprintEditor = InBlueprintEditor;
	CurrentHandle = DH_NONE;

	//TODO UMG Store as a setting
	//PreviewWidth = 1920;
	//PreviewHeight = 1080;
	PreviewWidth = 1280;
	PreviewHeight = 720;

	HoverTime = 0;

	bMouseDown = false;
	bMovingExistingWidget = false;

	DragDirections.Init((int32)DH_MAX);
	DragDirections[DH_TOP_LEFT] = FVector2D(-1, -1);
	DragDirections[DH_TOP_CENTER] = FVector2D(0, -1);
	DragDirections[DH_TOP_RIGHT] = FVector2D(1, -1);

	DragDirections[DH_MIDDLE_LEFT] = FVector2D(-1, 0);
	DragDirections[DH_MIDDLE_RIGHT] = FVector2D(1, 0);

	DragDirections[DH_BOTTOM_LEFT] = FVector2D(-1, 1);
	DragDirections[DH_BOTTOM_CENTER] = FVector2D(0, 1);
	DragDirections[DH_BOTTOM_RIGHT] = FVector2D(1, 1);

	// TODO UMG - Register these with the module through some public interface to allow for new extensions to be registered.
	Register(MakeShareable(new FVerticalSlotExtension()));
	Register(MakeShareable(new FHorizontalSlotExtension()));
	Register(MakeShareable(new FCanvasSlotExtension()));
	Register(MakeShareable(new FUniformGridSlotExtension()));
	Register(MakeShareable(new FGridSlotExtension()));

	UWidgetBlueprint* Blueprint = GetBlueprint();
	Blueprint->OnChanged().AddSP(this, &SDesignerView::OnBlueprintChanged);

	SDesignSurface::Construct(SDesignSurface::FArguments()
		.AllowContinousZoomInterpolation(false)
		.Content()
		[
			SNew(SOverlay)

			// The bottom layer of the overlay where the actual preview widget appears.
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(PreviewHitTestRoot, SZoomPan)
				.Visibility(EVisibility::HitTestInvisible)
				.ZoomAmount(this, &SDesignerView::GetZoomAmount)
				.ViewOffset(this, &SDesignerView::GetViewOffset)
				[
					SNew(SBox)
					.WidthOverride(this, &SDesignerView::GetPreviewWidth)
					.HeightOverride(this, &SDesignerView::GetPreviewHeight)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.Visibility(EVisibility::SelfHitTestInvisible)
					[
						SAssignNew(PreviewSurface, SDPIScaler)
						.DPIScale(this, &SDesignerView::GetPreviewDPIScale)
						.Visibility(EVisibility::SelfHitTestInvisible)
					]
				]
			]

			// A layer in the overlay where we put all the user intractable widgets, like the reorder widgets.
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(ExtensionWidgetCanvas, SCanvas)
				.Visibility(EVisibility::SelfHitTestInvisible)
			]

			// Top bar with buttons for changing the designer
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6, 2, 0, 0)
				[
					SNew(STextBlock)
					.TextStyle(FEditorStyle::Get(), "Graph.ZoomText")
					.Text(this, &SDesignerView::GetZoomText)
					.ColorAndOpacity(this, &SDesignerView::GetZoomTextColorAndOpacity)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SSpacer)
					.Size(FVector2D(1, 1))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
					.ToolTipText(LOCTEXT("ZoomToFit_ToolTip", "Zoom To Fit"))
					.OnClicked(this, &SDesignerView::HandleZoomToFitClicked)
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("UMGEditor.ZoomToFit"))
					]
				]
				
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SComboButton)
					.ButtonStyle(FEditorStyle::Get(), "PropertyEditor.AssetComboStyle")
					.ForegroundColor(FEditorStyle::GetColor("PropertyEditor.AssetName.ColorAndOpacity"))
					.OnGetMenuContent(this, &SDesignerView::GetAspectMenu)
					.ContentPadding(2.0f)
					.ButtonContent()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AspectRatio", "Aspect Ratio"))
					]
				]
			]
		]
	);

	BlueprintEditor.Pin()->OnSelectedWidgetsChanged.AddRaw(this, &SDesignerView::OnEditorSelectionChanged);

	ZoomToFit(/*bInstantZoom*/ true);
}

SDesignerView::~SDesignerView()
{
	UWidgetBlueprint* Blueprint = GetBlueprint();
	if ( Blueprint )
	{
		Blueprint->OnChanged().RemoveAll(this);
	}

	if ( BlueprintEditor.IsValid() )
	{
		BlueprintEditor.Pin()->OnSelectedWidgetsChanged.RemoveAll(this);
	}
}

float SDesignerView::GetPreviewScale() const
{
	return GetZoomAmount() * GetPreviewDPIScale();
}

FOptionalSize SDesignerView::GetPreviewWidth() const
{
	return (float)PreviewWidth;
}

FOptionalSize SDesignerView::GetPreviewHeight() const
{
	return (float)PreviewHeight;
}

float SDesignerView::GetPreviewDPIScale() const
{
	return GetDefault<URendererSettings>(URendererSettings::StaticClass())->GetDPIScaleBasedOnSize(FIntPoint(PreviewWidth, PreviewHeight));
}

FSlateRect SDesignerView::ComputeAreaBounds() const
{
	return FSlateRect(0, 0, PreviewWidth, PreviewHeight);
}

void SDesignerView::OnEditorSelectionChanged()
{
	TSet<FWidgetReference> PendingSelectedWidgets = BlueprintEditor.Pin()->GetSelectedWidgets();

	// Notify all widgets that are no longer selected.
	for ( FWidgetReference& WidgetRef : SelectedWidgets )
	{
		if ( WidgetRef.IsValid() && !PendingSelectedWidgets.Contains(WidgetRef) )
		{
			WidgetRef.GetPreview()->Deselect();
		}
	}

	// Notify all widgets that are now selected.
	for ( FWidgetReference& WidgetRef : PendingSelectedWidgets )
	{
		if ( WidgetRef.IsValid() && !SelectedWidgets.Contains(WidgetRef) )
		{
			WidgetRef.GetPreview()->Select();
		}
	}

	SelectedWidgets = PendingSelectedWidgets;

	if ( SelectedWidgets.Num() > 0 )
	{
		for ( FWidgetReference& Widget : SelectedWidgets )
		{
			SelectedWidget = Widget;
			break;
		}
	}
	else
	{
		SelectedWidget = FWidgetReference();
	}

	CreateExtensionWidgetsForSelection();
}

void SDesignerView::CreateExtensionWidgetsForSelection()
{
	// Remove all the current extension widgets
	ExtensionWidgetCanvas->ClearChildren();

	TArray<FWidgetReference> Selected;
	if ( SelectedWidget.IsValid() )
	{
		Selected.Add(SelectedWidget);
	}

	TArray< TSharedRef<FDesignerSurfaceElement> > ExtensionElements;

	// Build extension widgets for new selection
	for ( TSharedRef<FDesignerExtension>& Ext : DesignerExtensions )
	{
		if ( Ext->CanExtendSelection(Selected) )
		{
			Ext->ExtendSelection(Selected, ExtensionElements);
		}
	}

	// Add Widgets to designer surface
	for ( TSharedRef<FDesignerSurfaceElement>& ExtElement : ExtensionElements )
	{
		ExtensionWidgetCanvas->AddSlot()
			.Position(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(this, &SDesignerView::GetExtensionPosition, ExtElement)))
			.Size(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(this, &SDesignerView::GetExtensionSize, ExtElement)))
			[
				ExtElement->GetWidget()
			];
	}
}

FVector2D SDesignerView::GetExtensionPosition(TSharedRef<FDesignerSurfaceElement> ExtensionElement) const
{
	const FVector2D TopLeft = CachedDesignerWidgetLocation;
	const FVector2D Size = CachedDesignerWidgetSize * GetZoomAmount() * GetPreviewDPIScale();

	FVector2D FinalPosition(0, 0);

	switch ( ExtensionElement->GetLocation() )
	{
	case EExtensionLayoutLocation::Absolute:
		break;

	case EExtensionLayoutLocation::TopLeft:
		FinalPosition = TopLeft;
		break;
	case EExtensionLayoutLocation::TopCenter:
		FinalPosition = TopLeft + FVector2D(Size.X * 0.5f, 0);
		break;
	case EExtensionLayoutLocation::TopRight:
		FinalPosition = TopLeft + FVector2D(Size.X, 0);
		break;

	case EExtensionLayoutLocation::CenterLeft:
		FinalPosition = TopLeft + FVector2D(0, Size.Y * 0.5f);
		break;
	case EExtensionLayoutLocation::CenterCenter:
		FinalPosition = TopLeft + FVector2D(Size.X * 0.5f, Size.Y * 0.5f);
		break;
	case EExtensionLayoutLocation::CenterRight:
		FinalPosition = TopLeft + FVector2D(Size.X, Size.Y * 0.5f);
		break;

	case EExtensionLayoutLocation::BottomLeft:
		FinalPosition = TopLeft + FVector2D(0, Size.Y);
		break;
	case EExtensionLayoutLocation::BottomCenter:
		FinalPosition = TopLeft + FVector2D(Size.X * 0.5f, Size.Y);
		break;
	case EExtensionLayoutLocation::BottomRight:
		FinalPosition = TopLeft + Size;
		break;
	}

	return FinalPosition + ExtensionElement->GetOffset();
}

FVector2D SDesignerView::GetExtensionSize(TSharedRef<FDesignerSurfaceElement> ExtensionElement) const
{
	return ExtensionElement->GetWidget()->GetDesiredSize();
}

UWidgetBlueprint* SDesignerView::GetBlueprint() const
{
	if ( BlueprintEditor.IsValid() )
	{
		UBlueprint* BP = BlueprintEditor.Pin()->GetBlueprintObj();
		return Cast<UWidgetBlueprint>(BP);
	}

	return NULL;
}

void SDesignerView::Register(TSharedRef<FDesignerExtension> Extension)
{
	Extension->Initialize(this, GetBlueprint());
	DesignerExtensions.Add(Extension);
}

void SDesignerView::OnBlueprintChanged(UBlueprint* InBlueprint)
{
	if ( InBlueprint )
	{
		
	}
}

void SDesignerView::OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent)
{
	if ( !ensure(ObjectBeingModified) )
	{
		return;
	}

	//UpdatePreview(InBlueprint);
}

FWidgetReference SDesignerView::GetWidgetAtCursor(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FArrangedWidget& ArrangedWidget)
{
	//@TODO UMG Make it so you can request dropable widgets only, to find the first parentable.

	FArrangedChildren Children(EVisibility::All);

	PreviewHitTestRoot->SetVisibility(EVisibility::Visible);
	FArrangedWidget WindowWidgetGeometry(PreviewHitTestRoot.ToSharedRef(), MyGeometry);
	LocateWidgetsUnderCursor_Helper(WindowWidgetGeometry, MouseEvent.GetScreenSpacePosition(), Children, true);

	PreviewHitTestRoot->SetVisibility(EVisibility::HitTestInvisible);

	UUserWidget* WidgetActor = BlueprintEditor.Pin()->GetPreview();
	if ( WidgetActor )
	{
		UWidget* Preview = NULL;

		for ( int32 ChildIndex = Children.Num() - 1; ChildIndex >= 0; ChildIndex-- )
		{
			FArrangedWidget& Child = Children.GetInternalArray()[ChildIndex];
			Preview = WidgetActor->GetWidgetHandle(Child.Widget);
			
			// Ignore the drop preview widget when doing widget picking
			if (Preview == DropPreviewWidget)
			{
				Preview = NULL;
				continue;
			}
			
			if ( Preview )
			{
				ArrangedWidget = Child;
				break;
			}
		}

		if ( Preview )
		{
			return FWidgetReference::FromPreview(BlueprintEditor.Pin(), Preview);
		}
	}

	return FWidgetReference();
}

FReply SDesignerView::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SDesignSurface::OnMouseButtonDown(MyGeometry, MouseEvent);

	CurrentHandle = HitTestDragHandles(MyGeometry, MouseEvent);

	if ( CurrentHandle != DH_NONE )
	{
		BeginTransaction(LOCTEXT("ResizeWidget", "Resize Widget"));
		return FReply::Handled().PreventThrottling().CaptureMouse(AsShared());
	}

	//TODO UMG Undoable Selection
	FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
	FWidgetReference NewSelectedWidget = GetWidgetAtCursor(MyGeometry, MouseEvent, ArrangedWidget);
	SelectedWidgetContextMenuLocation = ArrangedWidget.Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	if ( NewSelectedWidget.IsValid() )
	{
		//@TODO UMG primary FBlueprintEditor needs to be inherited and selection control needs to be centralized.
		// Set the template as selected in the details panel
		TSet<FWidgetReference> SelectedTemplates;
		SelectedTemplates.Add(NewSelectedWidget);
		BlueprintEditor.Pin()->SelectWidgets(SelectedTemplates);

		if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
		{
			bMouseDown = true;
		}
	}

	// Capture mouse for the drag handle and general mouse actions
	return FReply::Handled().PreventThrottling().SetKeyboardFocus(AsShared(), EKeyboardFocusCause::Mouse).CaptureMouse(AsShared());
}

FReply SDesignerView::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		bMouseDown = false;
		bMovingExistingWidget = false;

		if ( CurrentHandle != DH_NONE )
		{
			EndTransaction();
			CurrentHandle = DH_NONE;
		}
	}
	else if ( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
	{
		if ( !bIsPanning )
		{
			ShowContextMenu(MyGeometry, MouseEvent);
		}
	}

	SDesignSurface::OnMouseButtonUp(MyGeometry, MouseEvent);

	return FReply::Handled().ReleaseMouseCapture();
}

FReply SDesignerView::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if ( MouseEvent.GetCursorDelta().IsZero() )
	{
		return FReply::Unhandled();
	}

	FReply SurfaceHandled = SDesignSurface::OnMouseMove(MyGeometry, MouseEvent);
	if ( SurfaceHandled.IsEventHandled() )
	{
		return SurfaceHandled;
	}

	// Update the resizing based on the mouse movement
	if ( CurrentHandle != DH_NONE )
	{
		if ( SelectedWidget.IsValid() )
		{
			//@TODO UMG - Implement some system to query slots to know if they support dragging so we can provide visual feedback by hiding handles that wouldn't work and such.
			//SelectedTemplate->Slot->CanResize(DH_TOP_CENTER)

			UWidget* Template = SelectedWidget.GetTemplate();
			UWidget* Preview = SelectedWidget.GetPreview();

			if ( Preview->Slot )
			{
				Preview->Slot->Resize(DragDirections[CurrentHandle], MouseEvent.GetCursorDelta() * ( 1.0f / GetPreviewScale() ));
			}

			if ( Template->Slot )
			{
				Template->Slot->Resize(DragDirections[CurrentHandle], MouseEvent.GetCursorDelta() * ( 1.0f / GetPreviewScale() ));
			}

			FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());

			return FReply::Handled().PreventThrottling();
		}
	}

	if ( bMouseDown )
	{
		if ( SelectedWidget.IsValid() && !bMovingExistingWidget )
		{
			const bool bIsRootWidget = SelectedWidget.GetTemplate()->GetParent() == NULL;
			if ( !bIsRootWidget )
			{
				bMovingExistingWidget = true;
				//Drag selected widgets
				return FReply::Handled().DetectDrag(AsShared(), EKeys::LeftMouseButton);
			}
		}
	}
	
	if ( CurrentHandle == DH_NONE )
	{
		// Update the hovered widget under the mouse
		FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
		FWidgetReference NewHoveredWidget = GetWidgetAtCursor(MyGeometry, MouseEvent, ArrangedWidget);
		if ( !( NewHoveredWidget == HoveredWidget ) )
		{
			HoveredWidget = NewHoveredWidget;
			HoverTime = 0;
		}
	}

	return FReply::Unhandled();
}

void SDesignerView::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	HoveredWidget = FWidgetReference();
	HoverTime = 0;
}

void SDesignerView::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	HoveredWidget = FWidgetReference();
	HoverTime = 0;
}

FReply SDesignerView::OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	BlueprintEditor.Pin()->PasteDropLocation = FVector2D(0, 0);

	if ( BlueprintEditor.Pin()->WidgetCommandList->ProcessCommandBindings(InKeyboardEvent) )
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SDesignerView::ShowContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FMenuBuilder MenuBuilder(true, NULL);

	FWidgetBlueprintEditorUtils::CreateWidgetContextMenu(MenuBuilder, BlueprintEditor.Pin().ToSharedRef(), SelectedWidgetContextMenuLocation);

	TSharedPtr<SWidget> MenuContent = MenuBuilder.MakeWidget();

	if ( MenuContent.IsValid() )
	{
		FVector2D SummonLocation = MouseEvent.GetScreenSpacePosition();
		FSlateApplication::Get().PushMenu(AsShared(), MenuContent.ToSharedRef(), SummonLocation, FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
	}
}

void SDesignerView::CacheSelectedWidgetGeometry()
{
	if ( SelectedSlateWidget.IsValid() )
	{
		TSharedRef<SWidget> Widget = SelectedSlateWidget.Pin().ToSharedRef();

		FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
		FDesignTimeUtils::GetArrangedWidgetRelativeToParent(Widget, AsShared(), ArrangedWidget);

		CachedDesignerWidgetLocation = ArrangedWidget.Geometry.AbsolutePosition;
		CachedDesignerWidgetSize = ArrangedWidget.Geometry.Size;
	}
}

int32 SDesignerView::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	SDesignSurface::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	LayerId += 1000;

	TSet<FWidgetReference> Selected;
	Selected.Add(SelectedWidget);

	// Allow the extensions to paint anything they want.
	for ( const TSharedRef<FDesignerExtension>& Ext : DesignerExtensions )
	{
		Ext->Paint(Selected, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	// Don't draw the hovered effect if it's also the selected widget
	if ( HoveredSlateWidget.IsValid() && HoveredSlateWidget != SelectedSlateWidget )
	{
		TSharedRef<SWidget> Widget = HoveredSlateWidget.Pin().ToSharedRef();

		FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
		FDesignTimeUtils::GetArrangedWidgetRelativeToWindow(Widget, ArrangedWidget);

		// Draw hovered effect
		// Azure = 0x007FFF
		const FLinearColor HoveredTint(0, 0.5, 1, FMath::Clamp(HoverTime / HoveredAnimationTime, 0.0f, 1.0f));

		FPaintGeometry HoveredGeometry(
			ArrangedWidget.Geometry.AbsolutePosition,
			ArrangedWidget.Geometry.Size * ArrangedWidget.Geometry.Scale,
			ArrangedWidget.Geometry.Scale);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			HoveredGeometry,
			FCoreStyle::Get().GetBrush(TEXT("Debug.Border")),
			MyClippingRect,
			ESlateDrawEffect::None,
			HoveredTint
			);
	}

	if ( SelectedSlateWidget.IsValid() )
	{
		TSharedRef<SWidget> Widget = SelectedSlateWidget.Pin().ToSharedRef();

		FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
		FDesignTimeUtils::GetArrangedWidgetRelativeToWindow(Widget, ArrangedWidget);

		const FLinearColor Tint(0, 1, 0);

		// Draw selection effect
		FPaintGeometry SelectionGeometry(
			ArrangedWidget.Geometry.AbsolutePosition,
			ArrangedWidget.Geometry.Size * ArrangedWidget.Geometry.Scale,
			ArrangedWidget.Geometry.Scale);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			SelectionGeometry,
			FCoreStyle::Get().GetBrush(TEXT("Debug.Border")),
			MyClippingRect,
			ESlateDrawEffect::None,
			Tint
		);

		DrawDragHandles(SelectionGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle);

		//TODO UMG Move this to a function.
		// Draw anchors.
		if ( SelectedWidget.IsValid() && SelectedWidget.GetTemplate()->Slot )
		{
			const float X = AllottedGeometry.AbsolutePosition.X;
			const float Y = AllottedGeometry.AbsolutePosition.Y;
			const float Width = AllottedGeometry.Size.X;
			const float Height = AllottedGeometry.Size.Y;

			float Selection_X = SelectionGeometry.DrawPosition.X;
			float Selection_Y = SelectionGeometry.DrawPosition.Y;
			float Selection_Width = SelectionGeometry.DrawSize.X;
			float Selection_Height = SelectionGeometry.DrawSize.Y;

			const FVector2D LeftRightSize = FVector2D(32, 16);
			const FVector2D TopBottomSize = FVector2D(16, 32);

			// @TODO UMG - Don't use the curve editors brushes
			const FSlateBrush* KeyBrush = FEditorStyle::GetBrush("CurveEd.CurveKey");
			FLinearColor KeyColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.f);

			// Normalized offset of the handle, 0, no offset, 1 move the handle completely out by its own size.
			//const float HandleOffset = 1.0f;

			if ( UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SelectedWidget.GetTemplate()->Slot) )
			{
				// Left
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					++LayerId,
					FPaintGeometry(FVector2D(X + Width * CanvasSlot->LayoutData.Anchors.Minimum.X - LeftRightSize.X * 0.5f, SelectionGeometry.DrawPosition.Y + Selection_Height * 0.5f - LeftRightSize.Y * 0.5f), LeftRightSize, 1.0f),
					FEditorStyle::Get().GetBrush("UMGEditor.AnchorLeftRight"),
					MyClippingRect,
					ESlateDrawEffect::None,
					KeyBrush->GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint() * KeyColor
					);

				// Right
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					++LayerId,
					FPaintGeometry(FVector2D(X + Width * CanvasSlot->LayoutData.Anchors.Maximum.X - LeftRightSize.X * 0.5f, SelectionGeometry.DrawPosition.Y + Selection_Height * 0.5f - LeftRightSize.Y * 0.5f), LeftRightSize, 1.0f),
					FEditorStyle::Get().GetBrush("UMGEditor.AnchorLeftRight"),
					MyClippingRect,
					ESlateDrawEffect::None,
					KeyBrush->GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint() * KeyColor
					);


				// Top
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					++LayerId,
					FPaintGeometry(FVector2D(SelectionGeometry.DrawPosition.X + Selection_Width * 0.5f - TopBottomSize.X * 0.5f, Y + Height * CanvasSlot->LayoutData.Anchors.Minimum.Y - TopBottomSize.Y * 0.5f), TopBottomSize, 1.0f),
					FEditorStyle::Get().GetBrush("UMGEditor.AnchorTopBottom"),
					MyClippingRect,
					ESlateDrawEffect::None,
					KeyBrush->GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint() * KeyColor
					);

				// Bottom
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					++LayerId,
					FPaintGeometry(FVector2D(SelectionGeometry.DrawPosition.X + Selection_Width * 0.5f - TopBottomSize.X * 0.5f, Y + Height * CanvasSlot->LayoutData.Anchors.Maximum.Y - TopBottomSize.Y * 0.5f), TopBottomSize, 1.0f),
					FEditorStyle::Get().GetBrush("UMGEditor.AnchorTopBottom"),
					MyClippingRect,
					ESlateDrawEffect::None,
					KeyBrush->GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint() * KeyColor
					);
			}
		}
	}

	return LayerId;
}

void SDesignerView::DrawDragHandles(const FPaintGeometry& SelectionGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle) const
{
	if ( SelectedWidget.IsValid() && SelectedWidget.GetTemplate()->Slot )
	{
		float X = SelectionGeometry.DrawPosition.X;
		float Y = SelectionGeometry.DrawPosition.Y;
		float Width = SelectionGeometry.DrawSize.X;
		float Height = SelectionGeometry.DrawSize.Y;

		// @TODO UMG Handles should come from the slot/container to express how its slots can be transformed.
		TArray<FVector2D> Handles;
		Handles.Add(FVector2D(X, Y));					// Top - Left
		Handles.Add(FVector2D(X + Width * 0.5f, Y));	// Top - Middle
		Handles.Add(FVector2D(X + Width, Y));			// Top - Right

		Handles.Add(FVector2D(X, Y + Height * 0.5f));			// Middle - Left
		Handles.Add(FVector2D(X + Width, Y + Height * 0.5f));	// Middle - Right

		Handles.Add(FVector2D(X, Y + Height));					// Bottom - Left
		Handles.Add(FVector2D(X + Width * 0.5f, Y + Height));	// Bottom - Middle
		Handles.Add(FVector2D(X + Width, Y + Height));			// Bottom - Right

		const FVector2D HandleSize = FVector2D(10, 10);

		// @TODO UMG - Don't use the curve editors brushes
		const FSlateBrush* KeyBrush = FEditorStyle::GetBrush("CurveEd.CurveKey");
		FLinearColor KeyColor = InWidgetStyle.GetColorAndOpacityTint();// IsEditingEnabled() ? InWidgetStyle.GetColorAndOpacityTint() : FLinearColor(0.1f, 0.1f, 0.1f, 1.f);

		for ( int32 HandleIndex = 0; HandleIndex < Handles.Num(); HandleIndex++ )
		{
			const FVector2D& Handle = Handles[HandleIndex];
			if ( !SelectedWidget.GetTemplate()->Slot->CanResize(DragDirections[HandleIndex]) )
			{
				// This isn't a valid handle
				continue;
			}

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				++LayerId,
				FPaintGeometry(FVector2D(Handle.X - HandleSize.X * 0.5f, Handle.Y - HandleSize.Y * 0.5f), HandleSize, 1.0f),
				KeyBrush,
				MyClippingRect,
				ESlateDrawEffect::None,
				KeyBrush->GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint() * KeyColor
				);
		}
	}
}

SDesignerView::DragHandle SDesignerView::HitTestDragHandles(const FGeometry& AllottedGeometry, const FPointerEvent& PointerEvent) const
{
	FVector2D LocalPointer = AllottedGeometry.AbsoluteToLocal(PointerEvent.GetScreenSpacePosition());

	if ( SelectedWidget.IsValid() && SelectedWidget.GetTemplate()->Slot && SelectedSlateWidget.IsValid() )
	{
		TSharedRef<SWidget> Widget = SelectedSlateWidget.Pin().ToSharedRef();

		FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
		FDesignTimeUtils::GetArrangedWidget(Widget, ArrangedWidget);

		FVector2D WidgetLocalPosition = AllottedGeometry.AbsoluteToLocal(ArrangedWidget.Geometry.AbsolutePosition);
		float X = WidgetLocalPosition.X;
		float Y = WidgetLocalPosition.Y;
		float Width = ArrangedWidget.Geometry.Size.X * GetZoomAmount() * GetPreviewDPIScale();
		float Height = ArrangedWidget.Geometry.Size.Y * GetZoomAmount() * GetPreviewDPIScale();

		TArray<FVector2D> Handles;
		Handles.Add(FVector2D(X, Y));					// Top - Left
		Handles.Add(FVector2D(X + Width * 0.5f, Y));	// Top - Middle
		Handles.Add(FVector2D(X + Width, Y));			// Top - Right

		Handles.Add(FVector2D(X, Y + Height * 0.5f));			// Middle - Left
		Handles.Add(FVector2D(X + Width, Y + Height * 0.5f));	// Middle - Right

		Handles.Add(FVector2D(X, Y + Height));					// Bottom - Left
		Handles.Add(FVector2D(X + Width * 0.5f, Y + Height));	// Bottom - Middle
		Handles.Add(FVector2D(X + Width, Y + Height));			// Bottom - Right

		const FVector2D HandleSize = FVector2D(10, 10);

		int32 i = 0;
		for ( FVector2D Handle : Handles )
		{
			FSlateRect Rect(FVector2D(Handle.X - HandleSize.X * 0.5f, Handle.Y - HandleSize.Y * 0.5f), Handle + HandleSize);
			if ( Rect.ContainsPoint(LocalPointer) )
			{
				if ( !SelectedWidget.GetTemplate()->Slot->CanResize(DragDirections[i]) )
				{
					// This isn't a valid handle
					break;
				}

				return (DragHandle)i;
			}
			i++;
		}
	}

	return DH_NONE;
}

FCursorReply SDesignerView::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	// Hit test the drag handles
	switch ( HitTestDragHandles(MyGeometry, CursorEvent) )
	{
	case DH_TOP_LEFT:
	case DH_BOTTOM_RIGHT:
		return FCursorReply::Cursor(EMouseCursor::ResizeSouthEast);
	case DH_TOP_RIGHT:
	case DH_BOTTOM_LEFT:
		return FCursorReply::Cursor(EMouseCursor::ResizeSouthWest);
	case DH_TOP_CENTER:
	case DH_BOTTOM_CENTER:
		return FCursorReply::Cursor(EMouseCursor::ResizeUpDown);
	case DH_MIDDLE_LEFT:
	case DH_MIDDLE_RIGHT:
		return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
	}

	return FCursorReply::Unhandled();
}

void SDesignerView::UpdatePreviewWidget()
{
	UUserWidget* LatestPreviewWidget = BlueprintEditor.Pin()->GetPreview();

	if ( LatestPreviewWidget != PreviewWidget )
	{
		PreviewWidget = LatestPreviewWidget;
		if ( PreviewWidget )
		{
			const bool bAbsoluteLayout = false;
			const bool bModal = false;
			const bool bShowCursor = false;
			TSharedPtr<SWidget> OutUserWidget;
			TSharedRef<SWidget> CurrentWidget = PreviewWidget->MakeViewportWidget(bAbsoluteLayout, bModal, bShowCursor, OutUserWidget);
			CurrentWidget->SlatePrepass();

			if ( CurrentWidget != PreviewSlateWidget.Pin() )
			{
				PreviewSlateWidget = CurrentWidget;
				PreviewSurface->SetContent(CurrentWidget);
			}

			// Notify all selected widgets that they are selected, because there are new preview objects
			// state may have been lost so this will recreate it if the widget does something special when
			// selected.
			for ( FWidgetReference& WidgetRef : SelectedWidgets )
			{
				if ( WidgetRef.IsValid() )
				{
					WidgetRef.GetPreview()->Select();
				}
			}
		}
		else
		{
			ChildSlot
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NoWidgetPreview", "No Widget Preview"))
				]
			];
		}
	}
}

void SDesignerView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	HoverTime += InDeltaTime;

	UpdatePreviewWidget();

	// Update the selected widget to match the selected template.
	if ( PreviewWidget )
	{
		if ( SelectedWidget.IsValid() )
		{
			// Set the selected widget so that we can draw the highlight
			SelectedSlateWidget = PreviewWidget->GetWidgetFromName(SelectedWidget.GetTemplate()->GetName());
		}
		else
		{
			SelectedSlateWidget.Reset();
		}

		if ( HoveredWidget.IsValid() )
		{
			HoveredSlateWidget = PreviewWidget->GetWidgetFromName(HoveredWidget.GetTemplate()->GetName());
		}
		else
		{
			HoveredSlateWidget.Reset();
		}
	}

	CacheSelectedWidgetGeometry();

	// Tick all designer extensions in case they need to update widgets
	for ( const TSharedRef<FDesignerExtension>& Ext : DesignerExtensions )
	{
		Ext->Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	}

	SDesignSurface::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

FReply SDesignerView::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if ( SelectedWidget.IsValid() )
	{
		return FReply::Handled().BeginDragDrop(FSelectedWidgetDragDropOp::New(SelectedWidget));
	}

	return FReply::Unhandled();
}

void SDesignerView::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	//@TODO UMG Drop Feedback
}

void SDesignerView::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDecoratedDragDropOp> DecoratedDragDropOp = DragDropEvent.GetOperationAs<FDecoratedDragDropOp>();
	if ( DecoratedDragDropOp.IsValid() )
	{
		DecoratedDragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());
		DecoratedDragDropOp->ResetToDefaultToolTip();
	}
	
	if (DropPreviewWidget)
	{
		if ( DropPreviewParent )
		{
			DropPreviewParent->RemoveChild(DropPreviewWidget);
		}

		UWidgetBlueprint* BP = GetBlueprint();
		BP->WidgetTree->RemoveWidget(DropPreviewWidget);
		DropPreviewWidget = NULL;
	}
}

FReply SDesignerView::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	UWidgetBlueprint* BP = GetBlueprint();
	
	if (DropPreviewWidget)
	{
		if (DropPreviewParent)
		{
			DropPreviewParent->RemoveChild(DropPreviewWidget);
		}
		
		BP->WidgetTree->RemoveWidget(DropPreviewWidget);
		DropPreviewWidget = NULL;
	}
	
	const bool bIsPreview = true;
	DropPreviewWidget = ProcessDropAndAddWidget(MyGeometry, DragDropEvent, bIsPreview);
	if ( DropPreviewWidget )
	{
		//@TODO UMG Drop Feedback
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

UWidget* SDesignerView::ProcessDropAndAddWidget(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, bool bIsPreview)
{
	// In order to prevent the GetWidgetAtCursor code from picking the widget we're about to move, we need to mark it
	// as the drop preview widget before any other code can run.
	TSharedPtr<FSelectedWidgetDragDropOp> SelectedDragDropOp = DragDropEvent.GetOperationAs<FSelectedWidgetDragDropOp>();
	if ( SelectedDragDropOp.IsValid() )
	{
		DropPreviewWidget = SelectedDragDropOp->Widget.GetPreview();
	}

	UWidgetBlueprint* BP = GetBlueprint();

	if ( DropPreviewWidget )
	{
		if ( DropPreviewParent )
		{
			DropPreviewParent->RemoveChild(DropPreviewWidget);
		}

		BP->WidgetTree->RemoveWidget(DropPreviewWidget);
		DropPreviewWidget = NULL;
	}

	FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
	FWidgetReference WidgetUnderCursor = GetWidgetAtCursor(MyGeometry, DragDropEvent, ArrangedWidget);

	UWidget* Target = NULL;
	if ( WidgetUnderCursor.IsValid() )
	{
		Target = bIsPreview ? WidgetUnderCursor.GetPreview() : WidgetUnderCursor.GetTemplate();
	}

	TSharedPtr<FWidgetTemplateDragDropOp> TemplateDragDropOp = DragDropEvent.GetOperationAs<FWidgetTemplateDragDropOp>();
	if ( TemplateDragDropOp.IsValid() )
	{
		TemplateDragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());

		// If there's no root widget go ahead and add the widget into the root slot.
		if ( BP->WidgetTree->RootWidget == NULL )
		{
			FScopedTransaction Transaction(LOCTEXT("Designer_AddWidget", "Add Widget"));

			if ( !bIsPreview )
			{
				BP->WidgetTree->SetFlags(RF_Transactional);
				BP->WidgetTree->Modify();
			}

			// TODO UMG This method isn't great, maybe the user widget should just be a canvas.

			// Add it to the root if there are no other widgets to add it to.
			UWidget* Widget = TemplateDragDropOp->Template->Create(BP->WidgetTree);
			Widget->IsDesignTime(true);

			BP->WidgetTree->RootWidget = Widget;

			SelectedWidget = FWidgetReference::FromTemplate(BlueprintEditor.Pin(), Widget);

			DropPreviewParent = NULL;

			if ( bIsPreview )
			{
				Transaction.Cancel();
			}

			return Widget;
		}
		// If there's already a root widget we need to try and place our widget into a parent widget that we've picked against
		else if ( Target && Target->IsA(UPanelWidget::StaticClass()) )
		{
			UPanelWidget* Parent = Cast<UPanelWidget>(Target);

			FScopedTransaction Transaction(LOCTEXT("Designer_AddWidget", "Add Widget"));

			// If this isn't a preview operation we need to modify a few things to properly undo the operation.
			if ( !bIsPreview )
			{
				Parent->SetFlags(RF_Transactional);
				Parent->Modify();

				BP->WidgetTree->SetFlags(RF_Transactional);
				BP->WidgetTree->Modify();
			}

			// Construct the widget and mark it for design time rendering.
			UWidget* Widget = TemplateDragDropOp->Template->Create(BP->WidgetTree);
			Widget->IsDesignTime(true);

			// Determine local position inside the parent widget and add the widget to the slot.
			FVector2D LocalPosition = ArrangedWidget.Geometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
			if ( UPanelSlot* Slot = Parent->AddChild(Widget) )
			{
				// HACK UMG - This seems like a bad idea to call TakeWidget
				TSharedPtr<SWidget> SlateWidget = Widget->TakeWidget();
				SlateWidget->SlatePrepass();
				const FVector2D& WidgetDesiredSize = SlateWidget->GetDesiredSize();

				static const FVector2D MinimumDefaultSize(20, 20);
				FVector2D LocalSize = FVector2D(FMath::Max(WidgetDesiredSize.X, MinimumDefaultSize.X), FMath::Max(WidgetDesiredSize.Y, MinimumDefaultSize.Y));

				Slot->SetDesiredPosition(LocalPosition);
				Slot->SetDesiredSize(LocalSize);

				DropPreviewParent = Parent;

				if ( bIsPreview )
				{
					Transaction.Cancel();
				}

				return Widget;
			}
			else
			{
				TemplateDragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);

				// TODO UMG ERROR Slot can not be created because maybe the max children has been reached.
				//          Maybe we can traverse the hierarchy and add it to the first parent that will accept it?
			}

			if ( bIsPreview )
			{
				Transaction.Cancel();
			}
		}
		else
		{
			TemplateDragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);
		}
	}

	// Attempt to deal with moving widgets from a drag operation.
	if ( SelectedDragDropOp.IsValid() )
	{
		SelectedDragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());

		if ( Target && Target->IsA(UPanelWidget::StaticClass()) )
		{
			UPanelWidget* NewParent = Cast<UPanelWidget>(Target);

			FScopedTransaction Transaction(LOCTEXT("Designer_MoveWidget", "Move Widget"));

			// If this isn't a preview operation we need to modify a few things to properly undo the operation.
			if ( !bIsPreview )
			{
				NewParent->SetFlags(RF_Transactional);
				NewParent->Modify();

				BP->WidgetTree->SetFlags(RF_Transactional);
				BP->WidgetTree->Modify();
			}

			UWidget* Widget = bIsPreview ? SelectedDragDropOp->Widget.GetPreview() : SelectedDragDropOp->Widget.GetTemplate();

			if ( Widget->GetParent() )
			{
				if ( !bIsPreview )
				{
					Widget->GetParent()->Modify();
				}

				Widget->GetParent()->RemoveChild(Widget);
			}

			FVector2D LocalPosition = ArrangedWidget.Geometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
			if ( UPanelSlot* Slot = NewParent->AddChild(Widget) )
			{
				FWidgetBlueprintEditorUtils::ImportPropertiesFromText(Slot, SelectedDragDropOp->ExportedSlotProperties);

				Slot->SetDesiredPosition(LocalPosition - SelectedWidgetContextMenuLocation);

				DropPreviewParent = NewParent;

				if ( bIsPreview )
				{
					Transaction.Cancel();
				}

				return Widget;
			}
			else
			{
				SelectedDragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);

				// TODO UMG ERROR Slot can not be created because maybe the max children has been reached.
				//          Maybe we can traverse the hierarchy and add it to the first parent that will accept it?
			}

			if ( bIsPreview )
			{
				Transaction.Cancel();
			}
		}
		else
		{
			SelectedDragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);
		}
	}
	
	return NULL;
}

FReply SDesignerView::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	bMouseDown = false;
	bMovingExistingWidget = false;

	UWidgetBlueprint* BP = GetBlueprint();
	
	if (DropPreviewWidget)
	{
		if (DropPreviewParent)
		{
			DropPreviewParent->RemoveChild(DropPreviewWidget);
		}
		
		BP->WidgetTree->RemoveWidget(DropPreviewWidget);
		DropPreviewWidget = NULL;
	}
	
	const bool bIsPreview = false;
	UWidget* Widget = ProcessDropAndAddWidget(MyGeometry, DragDropEvent, bIsPreview);
	if ( Widget )
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);

		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

void SDesignerView::HandleCommonResolutionSelected(int32 Width, int32 Height)
{
	PreviewWidth = Width;
	PreviewHeight = Height;
}

void SDesignerView::AddScreenResolutionSection(FMenuBuilder& MenuBuilder, const TArray<FPlayScreenResolution>& Resolutions, const FText& SectionName)
{
	MenuBuilder.BeginSection(NAME_None, SectionName);
	{
		for ( auto Iter = Resolutions.CreateConstIterator(); Iter; ++Iter )
		{
			FUIAction Action(FExecuteAction::CreateRaw(this, &SDesignerView::HandleCommonResolutionSelected, Iter->Width, Iter->Height));

			FInternationalization& I18N = FInternationalization::Get();

			FFormatNamedArguments Args;
			Args.Add(TEXT("Width"), FText::AsNumber(Iter->Width, NULL, I18N.GetInvariantCulture()));
			Args.Add(TEXT("Height"), FText::AsNumber(Iter->Height, NULL, I18N.GetInvariantCulture()));
			Args.Add(TEXT("AspectRatio"), FText::FromString(Iter->AspectRatio));

			MenuBuilder.AddMenuEntry(FText::FromString(Iter->Description), FText::Format(LOCTEXT("CommonResolutionFormat", "{Width} x {Height} (AspectRatio)"), Args), FSlateIcon(), Action);
		}
	}
	MenuBuilder.EndSection();
}

TSharedRef<SWidget> SDesignerView::GetAspectMenu()
{
	const ULevelEditorPlaySettings* PlaySettings = GetDefault<ULevelEditorPlaySettings>();
	FMenuBuilder MenuBuilder(true, NULL);

	AddScreenResolutionSection(MenuBuilder, PlaySettings->PhoneScreenResolutions, LOCTEXT("CommonPhonesSectionHeader", "Phones"));
	AddScreenResolutionSection(MenuBuilder, PlaySettings->TabletScreenResolutions, LOCTEXT("CommonTabletsSectionHeader", "Tablets"));
	AddScreenResolutionSection(MenuBuilder, PlaySettings->LaptopScreenResolutions, LOCTEXT("CommonLaptopsSectionHeader", "Laptops"));
	AddScreenResolutionSection(MenuBuilder, PlaySettings->MonitorScreenResolutions, LOCTEXT("CommoMonitorsSectionHeader", "Monitors"));
	AddScreenResolutionSection(MenuBuilder, PlaySettings->TelevisionScreenResolutions, LOCTEXT("CommonTelevesionsSectionHeader", "Televisions"));

	return MenuBuilder.MakeWidget();
}

void SDesignerView::BeginTransaction(const FText& SessionName)
{
	if ( ScopedTransaction == NULL )
	{
		ScopedTransaction = new FScopedTransaction(SessionName);
	}

	if ( SelectedWidget.IsValid() )
	{
		SelectedWidget.GetPreview()->Modify();
		SelectedWidget.GetTemplate()->Modify();
	}
}

void SDesignerView::EndTransaction()
{
	if ( ScopedTransaction != NULL )
	{
		delete ScopedTransaction;
		ScopedTransaction = NULL;
	}
}

FReply SDesignerView::HandleZoomToFitClicked()
{
	ZoomToFit(/*bInstantZoom*/ false);
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

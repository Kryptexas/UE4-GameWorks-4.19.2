// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SUMGDesigner.h"
#include "BlueprintEditor.h"
#include "SKismetInspector.h"
#include "BlueprintEditorUtils.h"

#include "WidgetBlueprintEditor.h"

#include "WidgetTemplateDragDropOp.h"

#define LOCTEXT_NAMESPACE "UMG"

const float HoveredAnimationTime = 0.150f;

class FSelectedWidgetDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FSelectedWidgetDragDropOp, FDecoratedDragDropOp)

	TMap<FName, FString> ExportedSlotProperties;

	FWidgetReference Widget;

	static TSharedRef<FSelectedWidgetDragDropOp> New(FWidgetReference InWidget);

	//virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;
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
				FArrangedWidget& SomeChild = ArrangedChildren(ChildIndex);
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
// SUMGDesigner

void SUMGDesigner::Construct(const FArguments& InArgs, TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor)
{
	ScopedTransaction = NULL;

	PreviewWidget = NULL;
	DropPreviewWidget = NULL;
	DropPreviewParent = NULL;
	BlueprintEditor = InBlueprintEditor;
	CurrentHandle = DH_NONE;

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

	Register(MakeShareable(new FVerticalSlotExtension()));
	Register(MakeShareable(new FHorizontalSlotExtension()));
	Register(MakeShareable(new FCanvasSlotExtension()));
	Register(MakeShareable(new FUniformGridSlotExtension()));

	UWidgetBlueprint* Blueprint = GetBlueprint();
	Blueprint->OnChanged().AddSP(this, &SUMGDesigner::OnBlueprintChanged);

	SDesignSurface::Construct(SDesignSurface::FArguments()
		.Content()
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(PreviewSurface, SBorder)
				.Visibility(EVisibility::HitTestInvisible)
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(ExtensionWidgetCanvas, SCanvas)
				.Visibility(EVisibility::SelfHitTestInvisible)
			]
		]
	);

	BlueprintEditor.Pin()->OnSelectedWidgetsChanged.AddRaw(this, &SUMGDesigner::OnEditorSelectionChanged);
}

SUMGDesigner::~SUMGDesigner()
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

void SUMGDesigner::OnEditorSelectionChanged()
{
	SelectedWidgets = BlueprintEditor.Pin()->GetSelectedWidgets();

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

void SUMGDesigner::CreateExtensionWidgetsForSelection()
{
	// Remove all the current extension widgets
	ExtensionWidgetCanvas->ClearChildren();

	ExtensionWidgets.Reset();

	TArray<FWidgetReference> Selected;
	if ( SelectedWidget.IsValid() )
	{
		Selected.Add(SelectedWidget);
	}

	// Build extension widgets for new selection
	for ( TSharedRef<FDesignerExtension>& Ext : DesignerExtensions )
	{
		Ext->BuildWidgetsForSelection(Selected, ExtensionWidgets);
	}

	//TODO UMG We need to have a better layout system for extensions.
	//         currently everything is just laid out across the top.

	TSharedRef<SHorizontalBox> ExtensionBox = SNew(SHorizontalBox);

	// Add Widgets to designer surface
	for ( TSharedRef<SWidget>& ExWidget : ExtensionWidgets )
	{
		ExtensionBox->AddSlot()
			.AutoWidth()
			[
				ExWidget
			];
	}

	ExtensionWidgetCanvas->AddSlot()
		.Position(TAttribute<FVector2D>(this, &SUMGDesigner::GetCachedSelectionDesignerWidgetsLocation))
		.Size(FVector2D(100, 25))
		[
			ExtensionBox
		];
}

UWidgetBlueprint* SUMGDesigner::GetBlueprint() const
{
	if ( BlueprintEditor.IsValid() )
	{
		UBlueprint* BP = BlueprintEditor.Pin()->GetBlueprintObj();
		return Cast<UWidgetBlueprint>(BP);
	}

	return NULL;
}

void SUMGDesigner::Register(TSharedRef<FDesignerExtension> Extension)
{
	Extension->Initialize(GetBlueprint());
	DesignerExtensions.Add(Extension);
}

void SUMGDesigner::OnBlueprintChanged(UBlueprint* InBlueprint)
{
	if ( InBlueprint )
	{
		
	}
}

void SUMGDesigner::OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent)
{
	if ( !ensure(ObjectBeingModified) )
	{
		return;
	}

	//UpdatePreview(InBlueprint);
}

FWidgetReference SUMGDesigner::GetWidgetAtCursor(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FArrangedWidget& ArrangedWidget)
{
	//@TODO UMG Make it so you can request dropable widgets only, to find the first parentable.

	FArrangedChildren Children(EVisibility::All);

	PreviewSurface->SetVisibility(EVisibility::Visible);
	FArrangedWidget WindowWidgetGeometry(PreviewSurface.ToSharedRef(), MyGeometry);
	LocateWidgetsUnderCursor_Helper(WindowWidgetGeometry, MouseEvent.GetScreenSpacePosition(), Children, true);

	PreviewSurface->SetVisibility(EVisibility::HitTestInvisible);

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

FReply SUMGDesigner::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	CurrentHandle = HitTestDragHandles(MyGeometry, MouseEvent);

	if ( CurrentHandle != DH_NONE )
	{
		BeginTransaction(LOCTEXT("ResizeWidget", "Resize Widget"));
		return FReply::Handled().PreventThrottling().CaptureMouse(AsShared());
	}

	//TODO UMG Undoable Selection
	FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
	SelectedWidget = GetWidgetAtCursor(MyGeometry, MouseEvent, ArrangedWidget);
	SelectedWidgetContextMenuLocation = ArrangedWidget.Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	if ( SelectedWidget.IsValid() )
	{
		//@TODO UMG primary FBlueprintEditor needs to be inherited and selection control needs to be centralized.
		// Set the template as selected in the details panel
		TSet<FWidgetReference> SelectedTemplates;
		SelectedTemplates.Add(SelectedWidget);
		BlueprintEditor.Pin()->SelectWidgets(SelectedTemplates);

		if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
		{
			bMouseDown = true;
		}
	}

	// Capture mouse for the drag handle and general mouse actions
	return FReply::Handled().PreventThrottling().CaptureMouse(AsShared());
}

FReply SUMGDesigner::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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
		ShowContextMenu(MyGeometry, MouseEvent);
	}

	return FReply::Handled().ReleaseMouseCapture();
}

FReply SUMGDesigner::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if ( MouseEvent.GetCursorDelta().IsZero() )
	{
		return FReply::Unhandled();
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
				Preview->Slot->Resize(DragDirections[CurrentHandle], MouseEvent.GetCursorDelta());
			}

			if ( Template->Slot )
			{
				Template->Slot->Resize(DragDirections[CurrentHandle], MouseEvent.GetCursorDelta());
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

void SUMGDesigner::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	HoveredWidget = FWidgetReference();
	HoverTime = 0;
}

void SUMGDesigner::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	HoveredWidget = FWidgetReference();
	HoverTime = 0;
}

FReply SUMGDesigner::OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	BlueprintEditor.Pin()->PasteDropLocation = FVector2D(0, 0);

	if ( BlueprintEditor.Pin()->WidgetCommandList->ProcessCommandBindings(InKeyboardEvent) )
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SUMGDesigner::ShowContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

bool SUMGDesigner::GetArrangedWidget(TSharedRef<SWidget> Widget, FArrangedWidget& ArrangedWidget) const
{
	// We can't screenshot the widget unless there's a valid window handle to draw it in.
	TSharedPtr<SWindow> WidgetWindow = FSlateApplication::Get().FindWidgetWindow(Widget);
	if ( !WidgetWindow.IsValid() )
	{
		return false;
	}

	TSharedRef<SWindow> CurrentWindowRef = WidgetWindow.ToSharedRef();

	FWidgetPath WidgetPath;
	if ( FSlateApplication::Get().GeneratePathToWidgetUnchecked(Widget, WidgetPath) )
	{
		ArrangedWidget = WidgetPath.FindArrangedWidget(Widget);
		return true;
	}

	return false;
}

bool SUMGDesigner::GetArrangedWidgetRelativeToWindow(TSharedRef<SWidget> Widget, FArrangedWidget& ArrangedWidget) const
{
	// We can't screenshot the widget unless there's a valid window handle to draw it in.
	TSharedPtr<SWindow> WidgetWindow = FSlateApplication::Get().FindWidgetWindow(Widget);
	if ( !WidgetWindow.IsValid() )
	{
		return false;
	}

	TSharedRef<SWindow> CurrentWindowRef = WidgetWindow.ToSharedRef();

	FWidgetPath WidgetPath;
	if ( FSlateApplication::Get().GeneratePathToWidgetUnchecked(Widget, WidgetPath) )
	{
		ArrangedWidget = WidgetPath.FindArrangedWidget(Widget);
		ArrangedWidget.Geometry.AbsolutePosition -= CurrentWindowRef->GetPositionInScreen();
		return true;
	}

	return false;
}

bool SUMGDesigner::GetArrangedWidgetRelativeToDesigner(TSharedRef<SWidget> Widget, FArrangedWidget& ArrangedWidget) const
{
	FWidgetPath WidgetPath;
	if ( FSlateApplication::Get().GeneratePathToWidgetUnchecked(Widget, WidgetPath) )
	{
		FArrangedWidget ArrangedDesigner = WidgetPath.FindArrangedWidget(this->AsShared());

		ArrangedWidget = WidgetPath.FindArrangedWidget(Widget);
		ArrangedWidget.Geometry.AbsolutePosition -= ArrangedDesigner.Geometry.AbsolutePosition;
		return true;
	}

	return false;
}

FVector2D SUMGDesigner::GetCachedSelectionDesignerWidgetsLocation() const
{
	return CachedDesignerWidgetLocation;
}

FVector2D SUMGDesigner::GetSelectionDesignerWidgetsLocation() const
{
	if ( SelectedSlateWidget.IsValid() )
	{
		TSharedRef<SWidget> Widget = SelectedSlateWidget.Pin().ToSharedRef();

		FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
		GetArrangedWidgetRelativeToDesigner(Widget, ArrangedWidget);

		return ArrangedWidget.Geometry.AbsolutePosition - FVector2D(0, 25);
	}

	return FVector2D(0, 0);
}

int32 SUMGDesigner::OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	SDesignSurface::OnPaint(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

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
		GetArrangedWidgetRelativeToWindow(Widget, ArrangedWidget);

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
		GetArrangedWidgetRelativeToWindow(Widget, ArrangedWidget);

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

			if ( UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SelectedWidget.GetTemplate()->Slot) )
			{
				//Set("UMGEditor.AnchorCenter", new IMAGE_BRUSH("Icons/umg_anchor_center", Icon16x16));
				//Set("UMGEditor.AnchorTopBottom", new IMAGE_BRUSH("Icons/umg_anchor_top_bottom", FVector2D(16, 32)));
				//Set("UMGEditor.AnchorLeftRight", new IMAGE_BRUSH("Icons/umg_anchor_left_right", FVector2D(32, 16)));

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

void SUMGDesigner::DrawDragHandles(const FPaintGeometry& SelectionGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle) const
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

SUMGDesigner::DragHandle SUMGDesigner::HitTestDragHandles(const FGeometry& AllottedGeometry, const FPointerEvent& PointerEvent) const
{
	FVector2D LocalPointer = AllottedGeometry.AbsoluteToLocal(PointerEvent.GetScreenSpacePosition());

	if ( SelectedWidget.IsValid() && SelectedWidget.GetTemplate()->Slot && SelectedSlateWidget.IsValid() )
	{
		TSharedRef<SWidget> Widget = SelectedSlateWidget.Pin().ToSharedRef();

		FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
		GetArrangedWidget(Widget, ArrangedWidget);

		FVector2D WidgetLocalPosition = AllottedGeometry.AbsoluteToLocal(ArrangedWidget.Geometry.AbsolutePosition);
		float X = WidgetLocalPosition.X;
		float Y = WidgetLocalPosition.Y;
		float Width = ArrangedWidget.Geometry.Size.X;
		float Height = ArrangedWidget.Geometry.Size.Y;

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

FCursorReply SUMGDesigner::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
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

void SUMGDesigner::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	HoverTime += InDeltaTime;

	UUserWidget* LatestPreviewWidget = BlueprintEditor.Pin()->GetPreview();

	if ( LatestPreviewWidget != PreviewWidget )
	{
		PreviewWidget = LatestPreviewWidget;
		if ( PreviewWidget )
		{
			TSharedRef<SWidget> CurrentWidget = PreviewWidget->MakeFullScreenWidget();
			CurrentWidget->SlatePrepass();

			if ( CurrentWidget != PreviewSlateWidget.Pin() )
			{
				PreviewSlateWidget = CurrentWidget;
				PreviewSurface->SetContent(CurrentWidget);
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

	// Update the selected widget to match the selected template.
	if ( LatestPreviewWidget )
	{
		if ( SelectedWidget.IsValid() )
		{
			// Set the selected widget so that we can draw the highlight
			SelectedSlateWidget = LatestPreviewWidget->GetWidgetFromName(SelectedWidget.GetTemplate()->GetName());
		}
		else
		{
			SelectedSlateWidget.Reset();
		}

		if ( HoveredWidget.IsValid() )
		{
			HoveredSlateWidget = LatestPreviewWidget->GetWidgetFromName(HoveredWidget.GetTemplate()->GetName());
		}
		else
		{
			HoveredSlateWidget.Reset();
		}
	}

	CachedDesignerWidgetLocation = GetSelectionDesignerWidgetsLocation();

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

FReply SUMGDesigner::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if ( SelectedWidget.IsValid() )
	{
		return FReply::Handled().BeginDragDrop(FSelectedWidgetDragDropOp::New(SelectedWidget));
	}

	return FReply::Unhandled();
}

void SUMGDesigner::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	//@TODO UMG Drop Feedback
}

void SUMGDesigner::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FWidgetTemplateDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FWidgetTemplateDragDropOp>();
	if ( DragDropOp.IsValid() )
	{
		DragDropOp->ResetToDefaultToolTip();
	}
	
	if (DropPreviewWidget)
	{
		if ( DropPreviewParent )
		{
			DropPreviewParent->RemoveChild(DropPreviewWidget);
		}

		UWidgetBlueprint* BP = GetBlueprint();
		const bool bIsRecursive = false;
		BP->WidgetTree->RemoveWidget(DropPreviewWidget, bIsRecursive);
		DropPreviewWidget = NULL;
	}
}

FReply SUMGDesigner::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	UWidgetBlueprint* BP = GetBlueprint();
	
	if (DropPreviewWidget)
	{
		if (DropPreviewParent)
		{
			DropPreviewParent->RemoveChild(DropPreviewWidget);
		}
		
		const bool bIsRecursive = false;
		BP->WidgetTree->RemoveWidget(DropPreviewWidget, bIsRecursive);
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

UWidget* SUMGDesigner::ProcessDropAndAddWidget(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, bool bIsPreview)
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

		const bool bIsRecursive = false;
		BP->WidgetTree->RemoveWidget(DropPreviewWidget, bIsRecursive);
		DropPreviewWidget = NULL;
	}

	FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
	FWidgetReference WidgetUnderCursor = GetWidgetAtCursor(MyGeometry, DragDropEvent, ArrangedWidget);

	UWidget* Target = NULL;
	if ( WidgetUnderCursor.IsValid() )
	{
		Target = bIsPreview ? WidgetUnderCursor.GetPreview() : WidgetUnderCursor.GetTemplate();
	}

	TSharedPtr<FWidgetTemplateDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FWidgetTemplateDragDropOp>();
	if ( DragDropOp.IsValid() )
	{		
		if ( Target )
		{
			if ( Target->IsA(UPanelWidget::StaticClass()) )
			{
				UPanelWidget* Parent = Cast<UPanelWidget>(Target);

				FScopedTransaction Transaction(LOCTEXT("Designer_AddWidget", "Add Widget"));

				if ( !bIsPreview )
				{
					Parent->SetFlags(RF_Transactional);
					Parent->Modify();

					BP->WidgetTree->SetFlags(RF_Transactional);
					BP->WidgetTree->Modify();
				}
				
				UWidget* Widget = DragDropOp->Template->Create(BP->WidgetTree);
				Widget->IsDesignTime(true);
			
				FVector2D LocalPosition = ArrangedWidget.Geometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
				if ( UPanelSlot* Slot = Parent->AddChild(Widget) )
				{
					Slot->SetDesiredPosition(LocalPosition);
					Slot->SetDesiredSize(FVector2D(150, 30));
					//@TODO UMG When we add a child blindly we need to default the slot size to the preferred size of the widget if the container supports such things.
					//@TODO UMG We may need a desired size canvas, where the slots have no size, they only give you position, alternatively, maybe slots that don't clip, so center is still easy.

					DropPreviewParent = Parent;

					if ( bIsPreview )
					{
						Transaction.Cancel();
					}

					return Widget;
				}
				else
				{
					// TODO UMG ERROR Slot can not be created because maybe the max children has been reached.
					//          Maybe we can traverse the hierarchy and add it to the first parent that will accept it?
				}

				if ( bIsPreview )
				{
					Transaction.Cancel();
				}
			}
			//else if ( BP->WidgetTree->RootWidget != NULL && BP->WidgetTree->RootWidget.Child == 1 )
			//{
			//	UWidget* Widget = DragDropOp->Template->Create(BP->WidgetTree);
			//	Widget->IsDesignTime(true);
			//	
			//	DropPreviewParent = NULL;
			//	
			//	return Widget;
			//}
		}
		else
		{
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
				UWidget* Widget = DragDropOp->Template->Create(BP->WidgetTree);
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
		}
	}

	// Attempt to deal with moving widgets from a drag operation.
	if ( SelectedDragDropOp.IsValid() )
	{
		if ( Target )
		{
			if ( Target->IsA(UPanelWidget::StaticClass()) )
			{
				UPanelWidget* NewParent = Cast<UPanelWidget>(Target);

				FScopedTransaction Transaction(LOCTEXT("Designer_MoveWidget", "Move Widget"));

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

					//TODO UMG Migrate existing slot info
					Slot->SetDesiredPosition(LocalPosition - SelectedWidgetContextMenuLocation);
					//Slot->SetDesiredSize(FVector2D(150, 30));
					//@TODO UMG When we add a child blindly we need to default the slot size to the preferred size of the widget if the container supports such things.
					//@TODO UMG We may need a desired size canvas, where the slots have no size, they only give you position, alternatively, maybe slots that don't clip, so center is still easy.

					DropPreviewParent = NewParent;

					if ( bIsPreview )
					{
						Transaction.Cancel();
					}

					return Widget;
				}
				else
				{
					// TODO UMG ERROR Slot can not be created because maybe the max children has been reached.
					//          Maybe we can traverse the hierarchy and add it to the first parent that will accept it?
				}

				if ( bIsPreview )
				{
					Transaction.Cancel();
				}
			}
		}
	}
	
	return NULL;
}

FReply SUMGDesigner::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
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
		
		const bool bIsRecursive = false;
		BP->WidgetTree->RemoveWidget(DropPreviewWidget, bIsRecursive);
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

void SUMGDesigner::BeginTransaction(const FText& SessionName)
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

void SUMGDesigner::EndTransaction()
{
	if ( ScopedTransaction != NULL )
	{
		delete ScopedTransaction;
		ScopedTransaction = NULL;
	}
}

#undef LOCTEXT_NAMESPACE

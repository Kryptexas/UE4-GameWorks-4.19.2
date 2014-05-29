// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SUMGDesigner.h"
#include "BlueprintEditor.h"
#include "SKismetInspector.h"
#include "BlueprintEditorUtils.h"

#include "WidgetBlueprintEditor.h"

#include "WidgetTemplateDragDropOp.h"

#define LOCTEXT_NAMESPACE "UMG"

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
		//if ( ( Candidate.Widget->GetVisibility().AreChildrenHitTestVisible() ) != 0 || OutWidgetsUnderCursor. )
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
		const bool bHitCandidateWidget = OutWidgetsUnderCursor.Accepts(Candidate.Widget->GetVisibility());
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
	PreviewWidgetActor = NULL;
	BlueprintEditor = InBlueprintEditor;
	CurrentHandle = DH_NONE;
	SelectedTemplate = NULL;

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
}

SUMGDesigner::~SUMGDesigner()
{
	UWidgetBlueprint* Blueprint = GetBlueprint();
	if ( Blueprint )
	{
		Blueprint->OnChanged().RemoveAll(this);
	}
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

void SUMGDesigner::ShowDetailsForObjects(TArray<UWidget*> Widgets)
{
	// @TODO COde duplication

	// Convert the selection set to an array of UObject* pointers
	FString InspectorTitle;
	TArray<UObject*> InspectorObjects;
	InspectorObjects.Empty(Widgets.Num());
	for ( UWidget* Widget : Widgets )
	{
		//if ( NodePtr->CanEditDefaults() )
		{
			InspectorTitle = "Widget";// Widget->GetDisplayString();
			InspectorObjects.Add(Widget);
		}
	}

	UWidgetBlueprint* Blueprint = GetBlueprint();

	// Update the details panel
	SKismetInspector::FShowDetailsOptions Options(InspectorTitle, true);
	BlueprintEditor.Pin()->GetInspector()->ShowDetailsForObjects(InspectorObjects, Options);
}

UWidget* SUMGDesigner::GetTemplateAtCursor(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FArrangedWidget& ArrangedWidget)
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
		UWidget* PreviewHandle = NULL;
		for ( int32 ChildIndex = Children.Num() - 1; ChildIndex >= 0; ChildIndex-- )
		{
			FArrangedWidget& Child = Children.GetInternalArray()[ChildIndex];
			PreviewHandle = WidgetActor->GetWidgetHandle(Child.Widget);
			if ( PreviewHandle )
			{
				ArrangedWidget = Child;
				break;
			}
		}

		UWidgetBlueprint* Blueprint = GetBlueprint();

		if ( PreviewHandle )
		{
			FString Name = PreviewHandle->GetName();
			UWidget* Template = Blueprint->WidgetTree->FindWidget(Name);
			return Template;
		}
	}

	return NULL;
}

FReply SUMGDesigner::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	CurrentHandle = HitTestDragHandles(MyGeometry, MouseEvent);

	if ( CurrentHandle == DH_NONE )
	{
		FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
		SelectedTemplate = GetTemplateAtCursor(MyGeometry, MouseEvent, ArrangedWidget);

		if ( SelectedTemplate )
		{
			//@TODO UMG primary FBlueprintEditor needs to be inherited and selection control needs to be centralized.
			// Set the template as selected in the details panel
			TArray<UWidget*> SelectedTemplates;
			SelectedTemplates.Add(SelectedTemplate);
			ShowDetailsForObjects(SelectedTemplates);

			// Remove all the current extension widgets
			ExtensionWidgetCanvas->ClearChildren();

			ExtensionWidgets.Reset();

			// Build extension widgets for new selection
			for ( TSharedRef<FDesignerExtension>& Ext : DesignerExtensions )
			{
				Ext->BuildWidgetsForSelection(SelectedTemplates, ExtensionWidgets);
			}

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
				.Position(TAttribute<FVector2D>(this, &SUMGDesigner::GetSelectionDesignerWidgetsLocation))
				.Size(FVector2D(100, 24))
				[
					ExtensionBox
				];
		}
	}
	else
	{
		return FReply::Handled().PreventThrottling().CaptureMouse(AsShared());
	}

	return FReply::Handled();
}

FReply SUMGDesigner::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	CurrentHandle = DH_NONE;
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SUMGDesigner::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if ( CurrentHandle != DH_NONE )
	{
		if ( SelectedTemplate->Slot && !MouseEvent.GetCursorDelta().IsZero() )
		{
			//@TODO UMG - Implement some system to query slots to know if they support dragging so we can provide visual feedback by hiding handles that wouldnt work and such.
			//SelectedTemplate->Slot->CanResize(DH_TOP_CENTER)

			SelectedTemplate->Slot->Resize(DragDirections[CurrentHandle], MouseEvent.GetCursorDelta());
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
			return FReply::Handled().PreventThrottling();
		}
	}

	return FReply::Unhandled();
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

FVector2D SUMGDesigner::GetSelectionDesignerWidgetsLocation() const
{
	if ( SelectedWidget.IsValid() )
	{
		TSharedRef<SWidget> Widget = SelectedWidget.Pin().ToSharedRef();

		FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
		GetArrangedWidgetRelativeToDesigner(Widget, ArrangedWidget);

		return ArrangedWidget.Geometry.AbsolutePosition - FVector2D(0, 25);
	}

	return FVector2D(0, 0);
}

int32 SUMGDesigner::OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	SDesignSurface::OnPaint(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if ( SelectedWidget.IsValid() )
	{
		TSharedRef<SWidget> Widget = SelectedWidget.Pin().ToSharedRef();

		FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
		GetArrangedWidgetRelativeToWindow(Widget, ArrangedWidget);

		const FLinearColor Tint(0, 1, 0);

		LayerId += 100;

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
	}

	return LayerId;
}

void SUMGDesigner::DrawDragHandles(const FPaintGeometry& SelectionGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle) const
{
	if ( SelectedTemplate && SelectedTemplate->Slot )
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
			if ( !SelectedTemplate->Slot->CanResize(DragDirections[HandleIndex]) )
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

	if ( SelectedTemplate && SelectedTemplate->Slot && SelectedWidget.IsValid() )
	{
		TSharedRef<SWidget> Widget = SelectedWidget.Pin().ToSharedRef();

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
				if ( !SelectedTemplate->Slot->CanResize(DragDirections[i]) )
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
	UUserWidget* WidgetActor = BlueprintEditor.Pin()->GetPreview();

	if ( WidgetActor != PreviewWidgetActor )
	{
		PreviewWidgetActor = WidgetActor;
		if ( PreviewWidgetActor )
		{
			TSharedRef<SWidget> CurrentWidget = PreviewWidgetActor->MakeFullScreenWidget();

			if ( CurrentWidget != PreviewWidget.Pin() )
			{
				PreviewWidget = CurrentWidget;
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
					.Text(LOCTEXT("NoWrappedWidget", "No Widget Preview"))
				]
			];
		}
	}

	// Update the selected widget to match the selected template.
	if ( SelectedTemplate )
	{
		if ( WidgetActor )
		{
			// Set the selected widget so that we can draw the highlight
			//@TODO UMG Don't store the transient Widget as the selected object, store the template component and lookup the widget from that.
			//SelectedWidget = ArrangedWidget.Widget;

			SelectedWidget = WidgetActor->GetWidgetFromName(SelectedTemplate->GetName());
		}
	}

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
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
}

FReply SUMGDesigner::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FWidgetTemplateDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FWidgetTemplateDragDropOp>();
	if ( DragDropOp.IsValid() )
	{
		//@TODO UMG Drop Feedback
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SUMGDesigner::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FWidgetTemplateDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FWidgetTemplateDragDropOp>();
	if ( DragDropOp.IsValid() )
	{
		FArrangedWidget ArrangedWidget(SNullWidget::NullWidget, FGeometry());
		UWidget* Template = GetTemplateAtCursor(MyGeometry, DragDropEvent, ArrangedWidget);

		UWidgetBlueprint* BP = GetBlueprint();

		if ( Template && Template->IsA(UPanelWidget::StaticClass()) )
		{
			UPanelWidget* Parent = Cast<UPanelWidget>(Template);

			UWidget* Widget = DragDropOp->Template->Create(BP->WidgetTree);

			FVector2D LocalPosition = ArrangedWidget.Geometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
			Parent->AddChild(Widget, LocalPosition);
			//@TODO UMG When we add a child blindly we need to default the slot size to the preferred size of the widget if the container supports such things.
			//@TODO UMG We may need a desired size canvas, where the slots have no size, they only give you position, alternatively, maybe slots that don't clip, so center is still easy.

			// Update the selected template to be the newly created one.
			SelectedTemplate = Widget;

			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);

			return FReply::Handled();
		}
		else if ( BP->WidgetTree->WidgetTemplates.Num() == 0 )
		{
			// No existing templates so just create it and make it the root widget.
			UWidget* Widget = DragDropOp->Template->Create(BP->WidgetTree);
			SelectedTemplate = Widget;

			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);

			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}


#undef LOCTEXT_NAMESPACE

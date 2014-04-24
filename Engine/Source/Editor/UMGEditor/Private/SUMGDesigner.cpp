// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SUMGDesigner.h"

#include "BlueprintEditor.h"
#include "SKismetInspector.h"

#define LOCTEXT_NAMESPACE "UMG"

//class SDesignerWidget : public SBorder
//{
//	SLATE_BEGIN_ARGS(SDesignerWidget) {}
//		/** Slot for the wrapped content (optional) */
//		SLATE_DEFAULT_SLOT(FArguments, Content)
//	SLATE_END_ARGS()
//
//	void Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor)
//	{
//		LastPreviewActor = NULL;
//		BlueprintEditor = InBlueprintEditor;
//
//		SBorder::Construct(SBorder::FArguments()
//			.BorderImage(FStyleDefaults::GetNoBrush())
//			.Padding(0.0f)
//			[
//
//				InArgs._Content.Widget
//			]);
//		}
//	}
//
//	//virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
//	//virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
///*virtual bool OnHitTest(const FGeometry& MyGeometry, FVector2D InAbsoluteCursorPosition) OVERRIDE;
//{
//return true;
//}*/
//
//protected:
//	TWeakPtr<FBlueprintEditor> BlueprintEditor;
//	TWeakObjectPtr<AActor> LastPreviewActor;
//};


//////////////////////////////////////////////////////////////////////////

FWorkflowApplicationModeExtender BlueprintEditorExtenderDelegate;

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
		if ( ( Candidate.Widget->GetVisibility().AreChildrenHitTestVisible() ) != 0 )
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
		const bool bHitCandidateWidget = Candidate.Widget->GetVisibility().IsHitTestVisible();
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

void SUMGDesigner::Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor)
{
	PreviewWidgetActor = NULL;
	BlueprintEditor = InBlueprintEditor;

	ChildSlot
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
			SNew(STextBlock)
		]
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

void SUMGDesigner::OnBlueprintChanged(UBlueprint* InBlueprint)
{
	if ( InBlueprint )
	{
		
	}
}

void SUMGDesigner::OnObjectPropertyChanged(UObject* ObjectBeingModified)
{
	if ( !ensure(ObjectBeingModified) )
	{
		return;
	}
}

void SUMGDesigner::ShowDetailsForObjects(TArray<USlateWrapperComponent*> Widgets)
{
	// @TODO COde duplication

	// Convert the selection set to an array of UObject* pointers
	FString InspectorTitle;
	TArray<UObject*> InspectorObjects;
	InspectorObjects.Empty(Widgets.Num());
	for ( USlateWrapperComponent* Widget : Widgets )
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

FReply SUMGDesigner::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FVector2D LocalMouseCoordinates = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	FArrangedChildren Children(EVisibility::All);

	PreviewSurface->SetVisibility(EVisibility::Visible);
	FArrangedWidget WindowWidgetGeometry(PreviewSurface.ToSharedRef(), MyGeometry);
	LocateWidgetsUnderCursor_Helper(WindowWidgetGeometry, MouseEvent.GetScreenSpacePosition(), Children, true);

	PreviewSurface->SetVisibility(EVisibility::HitTestInvisible);

	AUserWidget* WidgetActor = PreviewWidgetActor.Get();
	if ( WidgetActor )
	{
		USlateWrapperComponent* PreviewHandle = NULL;
		for ( int32 ChildIndex = Children.Num() - 1; ChildIndex >= 0; ChildIndex-- )
		{
			FArrangedWidget& Child = Children.GetInternalArray()[ChildIndex];
			PreviewHandle = WidgetActor->GetWidgetHandle(Child.Widget);
			if ( PreviewHandle )
			{
				break;
			}
		}

		UWidgetBlueprint* Blueprint = GetBlueprint();
		// TODO we have the widgets runtime handle, we need to look at the handle's name and find the corresponding
		// design time handle and select it!
	}

	return FReply::Handled();
}

void SUMGDesigner::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	AActor* PreviewActor = BlueprintEditor.Pin()->GetPreviewActor();
	AUserWidget* WidgetActor = Cast<AUserWidget>(PreviewActor);
	PreviewWidgetActor = WidgetActor;

	if (WidgetActor)
	{
		TSharedRef<SWidget> CurrentWidget = WidgetActor->GetRootWidget();

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
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NoWrappedWidget", "No actor; Open the viewport and tab back"))
			]
		];
	}

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

#undef LOCTEXT_NAMESPACE

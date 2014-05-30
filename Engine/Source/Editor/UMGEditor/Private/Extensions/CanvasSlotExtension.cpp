// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

FCanvasSlotExtension::FCanvasSlotExtension()
	: bDragging(false)
{
	ExtensionId = FName(TEXT("CanvasSlot"));
}

bool FCanvasSlotExtension::IsActive(const TArray< FSelectedWidget >& Selection)
{
	for ( const FSelectedWidget& Widget : Selection )
	{
		if ( !Widget.Template->Slot || !Widget.Template->Slot->IsA(UCanvasPanelSlot::StaticClass()) )
		{
			return false;
		}
	}

	return Selection.Num() == 1;
}

void FCanvasSlotExtension::BuildWidgetsForSelection(const TArray< FSelectedWidget >& Selection, TArray< TSharedRef<SWidget> >& Widgets)
{
	SelectionCache = Selection;

	if ( !IsActive(Selection) )
	{
		return;
	}

	MoveHandle =
		SNew(SBorder)
		//.BorderImage(FEditorStyle::Get().GetBrush("ToolBar.Background"))
		//.BorderBackgroundColor(FLinearColor(0,0,0))
		.OnMouseButtonDown(this, &FCanvasSlotExtension::HandleBeginDrag)
		.OnMouseButtonUp(this, &FCanvasSlotExtension::HandleEndDrag)
		.OnMouseMove(this, &FCanvasSlotExtension::HandleDragging)
		.Padding(FMargin(0))
		[
			SNew(SImage)
			.Image(FCoreStyle::Get().GetBrush("SoftwareCursor_CardinalCross"))
		];

	Widgets.Add(MoveHandle.ToSharedRef());
}

FReply FCanvasSlotExtension::HandleBeginDrag(const FGeometry& Geometry, const FPointerEvent& Event)
{
	bDragging = true;
	return FReply::Handled().CaptureMouse(MoveHandle.ToSharedRef());
}

FReply FCanvasSlotExtension::HandleEndDrag(const FGeometry& Geometry, const FPointerEvent& Event)
{
	bDragging = false;

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	return FReply::Handled().ReleaseMouseCapture();
}

FReply FCanvasSlotExtension::HandleDragging(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if ( bDragging )
	{
		for ( FSelectedWidget& Selection : SelectionCache )
		{
			MoveByAmount(Selection.Preview, Event.GetCursorDelta());
			MoveByAmount(Selection.Template, Event.GetCursorDelta());
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void FCanvasSlotExtension::MoveByAmount(UWidget* Widget, FVector2D Delta)
{
	if ( !Delta.IsZero() )
	{
		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
		UCanvasPanelComponent* Parent = Cast<UCanvasPanelComponent>(CanvasSlot->Parent);

		CanvasSlot->SetPosition(CanvasSlot->Position + Delta);
	}
}

#undef LOCTEXT_NAMESPACE

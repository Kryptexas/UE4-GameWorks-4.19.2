// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "Slate/SlateBrushAsset.h"
#include "WidgetBlueprintLibrary.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UWidgetBlueprintLibrary

UWidgetBlueprintLibrary::UWidgetBlueprintLibrary(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

UUserWidget* UWidgetBlueprintLibrary::Create(UObject* WorldContextObject, TSubclassOf<UUserWidget> WidgetType, APlayerController* OwningPlayer)
{
	if ( OwningPlayer == NULL )
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
		return CreateWidget<UUserWidget>(World, WidgetType);
	}
	else
	{
		return CreateWidget<UUserWidget>(OwningPlayer, WidgetType);
	}
}

UDragDropOperation* UWidgetBlueprintLibrary::CreateDragDropOperation(TSubclassOf<UDragDropOperation> Operation)
{
	if ( Operation )
	{
		return ConstructObject<UDragDropOperation>(Operation);
	}
	else
	{
		return ConstructObject<UDragDropOperation>(UDragDropOperation::StaticClass());
	}
}

void UWidgetBlueprintLibrary::SetFocusToGameViewport()
{
	FSlateApplication::Get().SetFocusToGameViewport();
}

void UWidgetBlueprintLibrary::DrawBox(UPARAM(ref) FPaintContext& Context, FVector2D Position, FVector2D Size, USlateBrushAsset* Brush, FLinearColor Tint)
{
	Context.MaxLayer++;

	if ( Brush )
	{
		FSlateDrawElement::MakeBox(
			Context.OutDrawElements,
			Context.MaxLayer,
			Context.AllottedGeometry.ToPaintGeometry(Position, Size),
			&Brush->Brush,
			Context.MyClippingRect,
			ESlateDrawEffect::None,
			Tint);
	}
}

void UWidgetBlueprintLibrary::DrawLine(UPARAM(ref) FPaintContext& Context, FVector2D PositionA, FVector2D PositionB, float Thickness, FLinearColor Tint, bool bAntiAlias)
{
	Context.MaxLayer++;

	TArray<FVector2D> Points;
	Points.Add(PositionA);
	Points.Add(PositionB);

	FSlateDrawElement::MakeLines(
		Context.OutDrawElements,
		Context.MaxLayer,
		Context.AllottedGeometry.ToPaintGeometry(),
		Points,
		Context.MyClippingRect,
		ESlateDrawEffect::None,
		Tint,
		bAntiAlias);
}

void UWidgetBlueprintLibrary::DrawText(UPARAM(ref) FPaintContext& Context, const FString& InString, FVector2D Position, FLinearColor Tint)
{
	Context.MaxLayer++;

	//TODO UMG Create a font asset usable as a UFont or as a slate font asset.
	FSlateFontInfo FontInfo = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText").Font;
	
	FSlateDrawElement::MakeText(
		Context.OutDrawElements,
		Context.MaxLayer,
		Context.AllottedGeometry.ToPaintGeometry(),
		InString,
		FontInfo,
		Context.MyClippingRect,
		ESlateDrawEffect::None,
		Tint);
}

FEventReply UWidgetBlueprintLibrary::Handled()
{
	FEventReply Reply;
	Reply.NativeReply = FReply::Handled();

	return Reply;
}

FEventReply UWidgetBlueprintLibrary::Unhandled()
{
	FEventReply Reply;
	Reply.NativeReply = FReply::Unhandled();

	return Reply;
}

FEventReply UWidgetBlueprintLibrary::CaptureMouse(UPARAM(ref) FEventReply& Reply, UWidget* CapturingWidget)
{
	if ( CapturingWidget )
	{
		TSharedPtr<SWidget> CapturingSlateWidget = CapturingWidget->GetCachedWidget();
		if ( CapturingSlateWidget.IsValid() )
		{
			Reply.NativeReply = Reply.NativeReply.CaptureMouse(CapturingSlateWidget.ToSharedRef());
		}
	}

	return Reply;
}

FEventReply UWidgetBlueprintLibrary::ReleaseMouseCapture(UPARAM(ref) FEventReply& Reply)
{
	Reply.NativeReply = Reply.NativeReply.ReleaseMouseCapture();

	return Reply;
}

FEventReply UWidgetBlueprintLibrary::CaptureJoystick(UPARAM(ref) FEventReply& Reply, UWidget* CapturingWidget, bool bInAllJoysticks/* = false*/)
{
	if ( CapturingWidget )
	{
		TSharedPtr<SWidget> CapturingSlateWidget = CapturingWidget->GetCachedWidget();
		if ( CapturingSlateWidget.IsValid() )
		{
			Reply.NativeReply = Reply.NativeReply.CaptureJoystick(CapturingSlateWidget.ToSharedRef(), bInAllJoysticks);
		}
	}

	return Reply;
}

FEventReply UWidgetBlueprintLibrary::ReleaseJoystickCapture(UPARAM(ref) FEventReply& Reply, bool bInAllJoysticks /*= false*/)
{
	Reply.NativeReply = Reply.NativeReply.ReleaseJoystickCapture(bInAllJoysticks);

	return Reply;
}

FEventReply UWidgetBlueprintLibrary::DetectDrag(UPARAM(ref) FEventReply& Reply, UWidget* WidgetDetectingDrag, FKey DragKey)
{
	if ( WidgetDetectingDrag )
	{
		TSharedPtr<SWidget> SlateWidgetDetectingDrag = WidgetDetectingDrag->GetCachedWidget();
		if ( SlateWidgetDetectingDrag.IsValid() )
		{
			Reply.NativeReply = Reply.NativeReply.DetectDrag(SlateWidgetDetectingDrag.ToSharedRef(), DragKey);
		}
	}

	return Reply;
}

FEventReply UWidgetBlueprintLibrary::DetectDragIfPressed(const FPointerEvent& PointerEvent, UWidget* WidgetDetectingDrag, FKey DragKey)
{
	if ( PointerEvent.GetEffectingButton() == DragKey )
	{
		FEventReply Reply = UWidgetBlueprintLibrary::Handled();
		return UWidgetBlueprintLibrary::DetectDrag(Reply, WidgetDetectingDrag, DragKey);
	}

	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UWidgetBlueprintLibrary::EndDragDrop(UPARAM(ref) FEventReply& Reply)
{
	Reply.NativeReply = Reply.NativeReply.EndDragDrop();

	return Reply;
}

#undef LOCTEXT_NAMESPACE
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

UUserWidget* UWidgetBlueprintLibrary::Create(UObject* WorldContextObject, TSubclassOf<UUserWidget> WidgetType)
{
	if ( WidgetType == NULL || WidgetType->HasAnyClassFlags(CLASS_Abstract) )
	{
		// TODO UMG script Error?
		return NULL;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	UUserWidget* NewWidget = ConstructObject<UUserWidget>(WidgetType, World->GetCurrentLevel());
	NewWidget->SetFlags(RF_Transactional);
	
	return NewWidget;
}

//void UWidgetBlueprintLibrary::Popup(UObject* WorldContextObject, UWidget* PopupWidget, int32 ZIndex)
//{
//	if ( !(WorldContextObject || PopupWidget) )
//	{
//		return;
//	}
//
//	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
//	if ( World && World->IsGameWorld() )
//	{
//		TSharedRef<SWidget> SlatePopupWidget = PopupWidget->GetWidget();
//
//		UGameViewportClient* Viewport = World->GetGameViewport();
//		Viewport->AddViewportWidgetContent(SlatePopupWidget, ZIndex);
//
//		TWeakPtr<SViewport> GameViewportWidget = Viewport->GetGameViewport()->GetViewportWidget();
//		if ( GameViewportWidget.IsValid() )
//		{
//			GameViewportWidget.Pin()->SetWidgetToFocusOnActivate(SlatePopupWidget);
//			FSlateApplication::Get().SetKeyboardFocus(SlatePopupWidget);
//		}
//
//		
//
//		//TSharedPtr<SWeakWidget> PopupWrapperPtr;
//		//PopupLayerSlot = &PopupWindow->AddPopupLayerSlot()
//		//	.DesktopPosition(NewPosition)
//		//	.WidthOverride(NewWindowSize.X)
//		//	[
//		//		SAssignNew(PopupWrapperPtr, SWeakWidget)
//		//		.PossiblyNullContent
//		//		(
//		//		MenuContentRef
//		//		)
//		//	];
//
//		// We want to support dismissing the popup widget when the user clicks outside it.
//		//FSlateApplication::Get().GetPopupSupport().RegisterClickNotification(PopupWrapperPtr.ToSharedRef(), FOnClickedOutside::CreateSP(this, &SMenuAnchor::OnClickedOutsidePopup));
//		//}
//
//		//if ( bFocusMenu )
//		//{
//		//	FSlateApplication::Get().SetKeyboardFocus(MenuContentRef, EKeyboardFocusCause::SetDirectly);
//		//}
//	}
//}

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

#undef LOCTEXT_NAMESPACE
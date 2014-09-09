// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "Slate/SlateBrushAsset.h"
#include "WidgetLayoutLibrary.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UWidgetLayoutLibrary

UWidgetLayoutLibrary::UWidgetLayoutLibrary(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

float UWidgetLayoutLibrary::GetViewportScale(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	if ( World && World->IsGameWorld() )
	{
		if ( UGameViewportClient* ViewportClient = World->GetGameViewport() )
		{
			FVector2D ViewportSize;
			ViewportClient->GetViewportSize(ViewportSize);
			return GetDefault<URendererSettings>(URendererSettings::StaticClass())->GetDPIScaleBasedOnSize(FIntPoint(ViewportSize.X, ViewportSize.Y));
		}
	}

	return 1;
}

FVector2D UWidgetLayoutLibrary::GetViewportSize(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	if ( World && World->IsGameWorld() )
	{
		if ( UGameViewportClient* ViewportClient = World->GetGameViewport() )
		{
			FVector2D ViewportSize;
			ViewportClient->GetViewportSize(ViewportSize);
			return ViewportSize;
		}
	}

	return FVector2D(1, 1);
}

UCanvasPanelSlot* UWidgetLayoutLibrary::SlotAsCanvasSlot(UWidget* ChildWidget)
{
	return Cast<UCanvasPanelSlot>(ChildWidget->Slot);
}

UGridSlot* UWidgetLayoutLibrary::SlotAsGridSlot(UWidget* ChildWidget)
{
	return Cast<UGridSlot>(ChildWidget->Slot);
}

UUniformGridSlot* UWidgetLayoutLibrary::SlotAsUniformGridSlot(UWidget* ChildWidget)
{
	return Cast<UUniformGridSlot>(ChildWidget->Slot);
}

#undef LOCTEXT_NAMESPACE
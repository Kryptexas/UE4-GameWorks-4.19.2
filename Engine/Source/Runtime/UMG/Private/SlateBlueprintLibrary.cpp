// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "Slate/SlateBrushAsset.h"
#include "Runtime/Engine/Classes/Engine/UserInterfaceSettings.h"
#include "SlateBlueprintLibrary.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// USlateBlueprintLibrary

USlateBlueprintLibrary::USlateBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

bool USlateBlueprintLibrary::IsUnderLocation(const FGeometry& Geometry, const FVector2D& AbsoluteCoordinate)
{
	return Geometry.IsUnderLocation(AbsoluteCoordinate);
}

FVector2D USlateBlueprintLibrary::AbsoluteToLocal(const FGeometry& Geometry, FVector2D AbsoluteCoordinate)
{
	return Geometry.AbsoluteToLocal(AbsoluteCoordinate);
}

FVector2D USlateBlueprintLibrary::LocalToAbsolute(const FGeometry& Geometry, FVector2D LocalCoordinate)
{
	return Geometry.LocalToAbsolute(LocalCoordinate);
}

FVector2D USlateBlueprintLibrary::GetLocalSize(const FGeometry& Geometry)
{
	return Geometry.GetLocalSize();
}

void USlateBlueprintLibrary::LocalToViewport(UObject* WorldContextObject, const FGeometry& Geometry, FVector2D LocalCoordinate, FVector2D& ScreenPosition, FVector2D& ViewportPosition)
{
	FVector2D AbsoluteCoordinate = Geometry.LocalToAbsolute(LocalCoordinate);
	return AbsoluteToViewport(WorldContextObject, Geometry, AbsoluteCoordinate, ScreenPosition, ViewportPosition);
}

void USlateBlueprintLibrary::AbsoluteToViewport(UObject* WorldContextObject, const FGeometry& Geometry, FVector2D AbsoluteCoordinate, FVector2D& ScreenPosition, FVector2D& ViewportPosition)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	if ( World && World->IsGameWorld() )
	{
		if ( UGameViewportClient* ViewportClient = World->GetGameViewport() )
		{
			if ( FViewport* Viewport = ViewportClient->Viewport )
			{
				FVector2D ViewportSize;
				ViewportClient->GetViewportSize(ViewportSize);

				FVector2D AbsolutePositionPostDPI = Viewport->VirtualDesktopPixelToViewport(FIntPoint((int32)AbsoluteCoordinate.X, (int32)AbsoluteCoordinate.Y)) * ViewportSize;

				float CurrentViewportScale = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(FIntPoint(ViewportSize.X, ViewportSize.Y));

				ScreenPosition = AbsolutePositionPostDPI;
				ViewportPosition = AbsolutePositionPostDPI / CurrentViewportScale;

				return;
			}
		}
	}

	ScreenPosition = FVector2D(0, 0);
	ViewportPosition = FVector2D(0, 0);
}

#undef LOCTEXT_NAMESPACE
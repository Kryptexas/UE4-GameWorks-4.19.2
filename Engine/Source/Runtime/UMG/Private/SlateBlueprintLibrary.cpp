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
	AbsoluteToViewport(WorldContextObject, Geometry, AbsoluteCoordinate, ScreenPosition, ViewportPosition);
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

				FVector2D PixelLocation = Viewport->VirtualDesktopPixelToViewport(FIntPoint((int32)AbsoluteCoordinate.X, (int32)AbsoluteCoordinate.Y)) * ViewportSize;

				float CurrentViewportScale = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(ViewportSize.X, ViewportSize.Y));

				// If the user has configured a resolution quality we need to multiply
				// the pixels by the resolution quality to arrive at the true position in
				// the viewport, as the rendered image will be stretched to fill whatever
				// size the viewport is at.
				Scalability::FQualityLevels ScalabilityQuality = Scalability::GetQualityLevels();
				const float QualityScale = ( ScalabilityQuality.ResolutionQuality / 100.0f );

				// Remove the resolution quality scale.
				ScreenPosition = PixelLocation * QualityScale;

				// Remove DPI Scaling.
				ViewportPosition = PixelLocation / CurrentViewportScale;

				return;
			}
		}
	}

	ScreenPosition = FVector2D(0, 0);
	ViewportPosition = FVector2D(0, 0);
}

#undef LOCTEXT_NAMESPACE
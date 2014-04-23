// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CanvasPanelComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UCanvasPanelComponent : public USlateNonLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface
#endif

	// USceneComponent interface
	virtual bool HasAnySockets() const OVERRIDE;
	virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = RTS_World) const OVERRIDE;
	virtual void QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const OVERRIDE;
	// End of USceneComponent interface

protected:
	/** The items placed on the canvas */
	UPROPERTY(EditAnywhere, Category=Slots)
	TArray<class UCanvasPanelSlot*> Slots;

	/** The desired size of the canvas */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FVector2D DesiredCanvasSize;

	TWeakPtr<class SFixedSizeCanvas> MyCanvas;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	// USlateNonLeafWidgetComponent interface
	virtual void OnKnownChildrenChanged() OVERRIDE;
	// End of USlateNonLeafWidgetComponent
};

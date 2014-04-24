// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CanvasPanelComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UCanvasPanelComponent : public USlateNonLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

	/** The items placed on the canvas */
	UPROPERTY(EditAnywhere, EditInline, Category=Slots)
	TArray<UCanvasPanelSlot*> Slots;

	/** The desired size of the canvas */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FVector2D DesiredCanvasSize;

	UCanvasPanelSlot* AddSlot(USlateWrapperComponent* Content);

	// USlateNonLeafWidgetComponent
	virtual int32 GetChildrenCount() const OVERRIDE;
	virtual USlateWrapperComponent* GetChildAt(int32 Index) const OVERRIDE;
	// End USlateNonLeafWidgetComponent

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface

	// USlateWrapperComponent interface
	virtual void ConnectEditorData() OVERRIDE;
	// End USlateWrapperComponent interface
#endif

protected:

	TWeakPtr<class SFixedSizeCanvas> MyCanvas;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface
};

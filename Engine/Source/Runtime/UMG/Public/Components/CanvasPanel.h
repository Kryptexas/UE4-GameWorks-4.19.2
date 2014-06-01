// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CanvasPanel.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UCanvasPanel : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/** The items placed on the canvas */
	UPROPERTY(EditAnywhere, EditInline, Category=Slots)
	TArray<UCanvasPanelSlot*> Slots;

	/** The desired size of the canvas */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FVector2D DesiredCanvasSize;

	UCanvasPanelSlot* AddSlot(UWidget* Content);
	virtual bool AddChild(UWidget* Child, FVector2D Position) OVERRIDE;

	// UPanelWidget
	virtual int32 GetChildrenCount() const OVERRIDE;
	virtual UWidget* GetChildAt(int32 Index) const OVERRIDE;
	// End UPanelWidget

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface

	// UWidget interface
	virtual void ConnectEditorData() OVERRIDE;
	// End UWidget interface
#endif

protected:

	TWeakPtr<class SFixedSizeCanvas> MyCanvas;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of UWidget interface
};

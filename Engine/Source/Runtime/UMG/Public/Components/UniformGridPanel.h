// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UniformGridPanel.generated.h"

/** A panel that evenly divides up available space between all of its children. */
UCLASS(meta=( Category="Panel" ), ClassGroup=UserInterface)
class UMG_API UUniformGridPanel : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/** The items placed on the grid */
	UPROPERTY()
	TArray<UUniformGridSlot*> Slots;

	/** Padding given to each slot */
	UPROPERTY(EditDefaultsOnly, Category=Layout)
	FMargin SlotPadding;

	/** The minimum desired width of the slots */
	UPROPERTY(EditDefaultsOnly, Category=Layout)
	float MinDesiredSlotWidth;

	/** The minimum desired height of the slots */
	UPROPERTY(EditDefaultsOnly, Category=Layout)
	float MinDesiredSlotHeight;

	//TODO UMG Add ways to make adding slots callable by blueprints.

	/** Adds content to the panel, and constructs a new slot to hold it. */
	UUniformGridSlot* AddSlot(UWidget* Content);

	// UPanelWidget
	virtual int32 GetChildrenCount() const OVERRIDE;
	virtual UWidget* GetChildAt(int32 Index) const OVERRIDE;
	virtual bool AddChild(UWidget* Child, FVector2D Position) OVERRIDE;
	virtual bool RemoveChild(UWidget* Child) OVERRIDE;
	virtual void ReplaceChildAt(int32 Index, UWidget* Child) OVERRIDE;
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

	TWeakPtr<class SUniformGridPanel> MyUniformGridPanel;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of UWidget interface
};

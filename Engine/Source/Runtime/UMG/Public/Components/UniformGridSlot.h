// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWrapperTypes.h"

#include "UniformGridSlot.generated.h"

/**
 * A slot for UUniformGridPanel, these slots all share the same size as the largest slot
 * in the grid.
 */
UCLASS()
class UMG_API UUniformGridSlot : public UPanelSlot
{
	GENERATED_UCLASS_BODY()

public:

	/** The alignment of the object horizontally. */
	UPROPERTY(EditDefaultsOnly, Category=Layout)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** The alignment of the object vertically. */
	UPROPERTY(EditDefaultsOnly, Category=Layout)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;
	
	/** The row index of the cell this slot is in */
	UPROPERTY(EditDefaultsOnly, Category=Layout)
	int32 Row;
	
	/** The column index of the cell this slot is in */
	UPROPERTY(EditDefaultsOnly, Category=Layout)
	int32 Column;

	/** Sets the row index of the slot, this determines what cell the slot is in the panel */
	UFUNCTION(BlueprintCallable, Category=Layout)
	void SetRow(int32 InRow);

	/** Sets the column index of the slot, this determines what cell the slot is in the panel */
	UFUNCTION(BlueprintCallable, Category=Layout)
	void SetColumn(int32 InColumn);

	// UPanelSlot interface
	virtual void SyncronizeProperties() override;
	// End of UPanelSlot interface

	/** Builds the underlying FSlot for the Slate layout panel. */
	void BuildSlot(TSharedRef<SUniformGridPanel> GridPanel);

private:
	//TODO UMG Slots should hold weak or shared refs to slots.

	/** A raw pointer to the slot to allow us to adjust the size, padding...etc at runtime. */
	SUniformGridPanel::FSlot* Slot;
};
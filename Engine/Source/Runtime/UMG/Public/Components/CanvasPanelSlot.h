// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Anchors.h"

#include "CanvasPanelSlot.generated.h"

UCLASS()
class UMG_API UCanvasPanelSlot : public UPanelSlot
{
	GENERATED_UCLASS_BODY()

	/** Offset. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FMargin Offset;
	
	/** Anchors. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FAnchors Anchors;

	/** Alignment. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FVector2D Alignment;

	void BuildSlot(TSharedRef<SConstraintCanvas> Canvas);

	virtual void Resize(const FVector2D& Direction, const FVector2D& Amount) OVERRIDE;

	virtual bool CanResize(const FVector2D& Direction) const OVERRIDE;

	/** Sets the position of the slot */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetOffset(FMargin InOffset);
	
	/** Sets the anchors on the slot */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetAnchors(FAnchors InAnchors);

	/** Sets the alignment on the slot */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetAlignment(FVector2D InAlignment);

private:
	SConstraintCanvas::FSlot* Slot;
};
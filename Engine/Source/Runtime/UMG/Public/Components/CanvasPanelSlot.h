// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Anchors.h"

#include "CanvasPanelSlot.generated.h"

USTRUCT()
struct FAnchorData
{
public:
	GENERATED_USTRUCT_BODY()

	/** Offset. */
	UPROPERTY(EditAnywhere, Category=AnchorData)
	FMargin Offsets;
	
	/** Anchors. */
	UPROPERTY(EditAnywhere, Category=AnchorData)
	FAnchors Anchors;

	/** Alignment. */
	UPROPERTY(EditAnywhere, Category=AnchorData)
	FVector2D Alignment;
};

UCLASS()
class UMG_API UCanvasPanelSlot : public UPanelSlot
{
	GENERATED_UCLASS_BODY()

	/** The anchoring information for the slot */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FAnchorData LayoutData;

	/** The order priority this widget is rendered in.  Higher values are rendered last (and so they will appear to be on top). */
	UPROPERTY(EditDefaultsOnly, Category=AnchorData)
	int32 ZOrder;

	void BuildSlot(TSharedRef<SConstraintCanvas> Canvas);

	virtual void SetDesiredPosition(FVector2D InPosition) override;

	virtual void SetDesiredSize(FVector2D InSize) override;

	virtual void Resize(const FVector2D& Direction, const FVector2D& Amount) override;

	virtual bool CanResize(const FVector2D& Direction) const override;

	/** Sets the position of the slot */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetOffset(FMargin InOffset);
	
	/** Sets the anchors on the slot */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetAnchors(FAnchors InAnchors);

	/** Sets the alignment on the slot */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetAlignment(FVector2D InAlignment);

	/** Sets the z-order on the slot */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetZOrder(int32 InZOrder);

	// UPanelSlot interface
	virtual void SyncronizeProperties() override;
	// End of UPanelSlot interface

#if WITH_EDITOR
	// UObject interface
	virtual void PreEditChange(UProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent);
	// End of UObject interface
#endif

private:
	SConstraintCanvas::FSlot* Slot;

#if WITH_EDITOR
	FGeometry PreEditGeometry;
	FAnchorData PreEditLayoutData;
#endif
};
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Anchors.h"

#include "CanvasPanelSlot.generated.h"

USTRUCT(BlueprintType)
struct FAnchorData
{
public:
	GENERATED_USTRUCT_BODY()

public:

	/** Offset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AnchorData)
	FMargin Offsets;
	
	/** Anchors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AnchorData)
	FAnchors Anchors;

	/** Alignment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AnchorData)
	FVector2D Alignment;
};

UCLASS()
class UMG_API UCanvasPanelSlot : public UPanelSlot
{
	GENERATED_UCLASS_BODY()

	/** The anchoring information for the slot */
	UPROPERTY(EditDefaultsOnly, Category="Layout (Canvas Slot)")
	FAnchorData LayoutData;

	/** The order priority this widget is rendered in.  Higher values are rendered last (and so they will appear to be on top). */
	UPROPERTY(EditDefaultsOnly, Category="Layout (Canvas Slot)")
	int32 ZOrder;

	void BuildSlot(TSharedRef<SConstraintCanvas> Canvas);

	virtual void SetDesiredPosition(FVector2D InPosition) override;

	virtual void SetDesiredSize(FVector2D InSize) override;

	virtual void Resize(const FVector2D& Direction, const FVector2D& Amount) override;

	virtual bool CanResize(const FVector2D& Direction) const override;

	/** Sets the position of the slot */
	UFUNCTION(BlueprintCallable, Category="Layout (Canvas Slot)")
	void SetPosition(FVector2D InPosition);

	/** Sets the size of the slot */
	UFUNCTION(BlueprintCallable, Category="Layout (Canvas Slot)")
	void SetSize(FVector2D InSize);

	/** Sets the position of the slot */
	UFUNCTION(BlueprintCallable, Category="Layout (Canvas Slot)")
	void SetOffsets(FMargin InOffset);
	
	/** Sets the anchors on the slot */
	UFUNCTION(BlueprintCallable, Category="Layout (Canvas Slot)")
	void SetAnchors(FAnchors InAnchors);

	/** Sets the alignment on the slot */
	UFUNCTION(BlueprintCallable, Category="Layout (Canvas Slot)")
	void SetAlignment(FVector2D InAlignment);

	/** Sets the z-order on the slot */
	UFUNCTION(BlueprintCallable, Category="Layout (Canvas Slot)")
	void SetZOrder(int32 InZOrder);

	// UPanelSlot interface
	virtual void SynchronizeProperties() override;
	// End of UPanelSlot interface

	virtual void ReleaseNativeWidget() override;

#if WITH_EDITOR
	// UObject interface
	virtual void PreEditChange(UProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent);
	// End of UObject interface

	/** Stores the current layout information about the slot and parent canvas. */
	void SaveBaseLayout();

	/** Compares the saved base layout against the current state.  Updates the necessary properties to maintain a stable position. */
	void RebaseLayout(bool PreserveSize = true);
#endif

private:
	SConstraintCanvas::FSlot* Slot;

#if WITH_EDITOR
	FGeometry PreEditGeometry;
	FAnchorData PreEditLayoutData;
#endif
};
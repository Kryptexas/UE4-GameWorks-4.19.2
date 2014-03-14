// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GridPanelComponent.generated.h"


USTRUCT()
struct SCWRUNTIME_API FGridPanelSlot
{
	GENERATED_USTRUCT_BODY()

	/** Name of the slot (use this to attach children to the slot) */
	UPROPERTY(VisibleAnywhere, Category=Appearance)
	FName SlotName;

	/** The row position within the grid. */
	UPROPERTY(EditAnywhere, Category=Appearance, meta=(ClampMin = "0"))
	int32 Row;

	/** The column position within the grid. */
	UPROPERTY(EditAnywhere, Category=Appearance, meta=(ClampMin = "0"))
	int32 Column;

	/** How many rows this this slot spans over */
	UPROPERTY(EditAnywhere, Category=Appearance, meta=(ClampMin = "1"))
	int32 RowSpan;

	/** How many columns this slot spans over */
	UPROPERTY(EditAnywhere, Category=Appearance, meta=(ClampMin = "1"))
	int32 ColumnSpan;

	/** Horizontal positioning of child within the allocated slot */
	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** Vertical positioning of child within the allocated slot */
	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	/** The padding to add around the child. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin Padding;

	FIntPoint AsPoint() const
	{
		return FIntPoint(Column, Row);
	}

	FGridPanelSlot()
		: SlotName(NAME_None) 
		, Row(0)
		, Column(0)
		, RowSpan(1)
		, ColumnSpan(1)
		, HorizontalAlignment(HAlign_Fill)
		, VerticalAlignment(VAlign_Fill)
		, Padding(FMargin(0.0f))
	{
	}
};


UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class SCWRUNTIME_API UGridPanelComponent : public USlateNonLeafWidgetComponent
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
	UPROPERTY(EditAnywhere, Category=Slots)
	TArray<FGridPanelSlot> Slots;

	TWeakPtr<class SGridPanel> MyGrid;

	TSet<FIntPoint> UniqueSlotCoordinates;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	// USlateNonLeafWidgetComponent interface
	virtual void OnKnownChildrenChanged() OVERRIDE;
	// End of USlateNonLeafWidgetComponent

	void RebuildUniqueSlotTable();
	void EnsureSlotIsUnique(FGridPanelSlot& SlotConfig);
};

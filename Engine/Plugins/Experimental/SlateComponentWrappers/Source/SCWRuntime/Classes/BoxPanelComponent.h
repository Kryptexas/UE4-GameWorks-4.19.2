// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BoxPanelComponent.generated.h"


USTRUCT()
struct SCWRUNTIME_API FBoxPanelSlot
{
	GENERATED_USTRUCT_BODY()

	/** Name of the slot (use this to attach children to the slot) */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FName SlotName;

	/** How much space this slot should occupy in the direction of the panel. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateChildSize SlotSize;

	/** Horizontal positioning of child within the allocated slot */
	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** Vertical positioning of child within the allocated slot */
	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	/** The padding to add around the child. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin Padding;

	/** Does this slot have a maximum size? */
	UPROPERTY(EditAnywhere, Category=Appearance)
	bool bHasMaximumSize;

	/** The maximum size that this slot can be */
	UPROPERTY(EditAnywhere, Category=Appearance, meta=(EditCondition="bHasMaximumSize"))
	float MaxSize;

	FBoxPanelSlot()
		: SlotName(NAME_None) 
		, SlotSize( FSlateChildSize() )
		, HorizontalAlignment( HAlign_Fill )
		, VerticalAlignment( VAlign_Fill )
		, Padding( FMargin(0.0f) )
		, bHasMaximumSize(false)
		, MaxSize( 50.0f )
	{
	}
};


UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class SCWRUNTIME_API UBoxPanelComponent : public USlateNonLeafWidgetComponent
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
	TArray<FBoxPanelSlot> Slots;

	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EOrientation> Orientation;

	TWeakPtr<class SConcreteBoxPanel> MyBox;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	// USlateNonLeafWidgetComponent interface
	virtual void OnKnownChildrenChanged() OVERRIDE;
	// End of USlateNonLeafWidgetComponent
};

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CanvasPanelComponent.generated.h"


USTRUCT()
struct SCWRUNTIME_API FCanvasPanelSlot
{
	GENERATED_USTRUCT_BODY()

	/** Name of the slot (use this to attach children to the slot) */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FName SlotName;

	/** Position. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FVector2D Position;

	/** Size. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FVector2D Size;

	/**
	* Horizontal pivot position
	*  Given a top aligned slot, where '+' represents the 
	*  anchor point defined by PositionAttr.
	*  
	*   Left				Center				Right
		+ _ _ _ _            _ _ + _ _          _ _ _ _ +
		|		  |		   | 		   |	  |		    |
		| _ _ _ _ |        | _ _ _ _ _ |	  | _ _ _ _ |
	* 
	*  Note: FILL is NOT supported.
	*/
	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/**
	* Vertical pivot position
	*   Given a left aligned slot, where '+' represents the 
	*   anchor point defined by PositionAttr.
	*  
	*   Top					Center			  Bottom
	*	+_ _ _ _ _		 _ _ _ _ _		 _ _ _ _ _ 
	*	|         |		| 		  |		|		  |
	*	|         |     +		  |		|		  |
	*	| _ _ _ _ |		| _ _ _ _ |		+ _ _ _ _ |
	* 
	*  Note: FILL is NOT supported.
	*/
	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	FCanvasPanelSlot()
		: SlotName(NAME_None) 
		, Position(FVector2D::ZeroVector)
		, Size(FVector2D(1.0f, 1.0f))
		, HorizontalAlignment(HAlign_Left)
		, VerticalAlignment(VAlign_Top)
	{
	}
};


UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class SCWRUNTIME_API UCanvasPanelComponent : public USlateNonLeafWidgetComponent
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
	TArray<FCanvasPanelSlot> Slots;

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

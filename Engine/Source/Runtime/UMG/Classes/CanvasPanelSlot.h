// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CanvasPanelSlot.generated.h"

USTRUCT()
struct UMG_API FCanvasSlotData
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

	FCanvasSlotData()
		: SlotName(NAME_None) 
		, Position(FVector2D::ZeroVector)
		, Size(FVector2D(1.0f, 1.0f))
		, HorizontalAlignment(HAlign_Left)
		, VerticalAlignment(VAlign_Top)
	{
	}
};

UCLASS()
class UMG_API UCanvasPanelSlot : public UPanelSlot
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Slot)
	FCanvasSlotData Data;
};
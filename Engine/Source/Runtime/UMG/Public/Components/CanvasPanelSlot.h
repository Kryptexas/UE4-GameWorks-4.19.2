// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CanvasPanelSlot.generated.h"

UCLASS()
class UMG_API UCanvasPanelSlot : public UPanelSlot
{
	GENERATED_UCLASS_BODY()

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
};
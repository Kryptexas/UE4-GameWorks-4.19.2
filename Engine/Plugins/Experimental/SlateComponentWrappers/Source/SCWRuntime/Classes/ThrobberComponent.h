// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ThrobberComponent.generated.h"

/** A Throbber widget that shows several zooming circles in a row. */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class SCWRUNTIME_API UThrobberComponent : public USlateLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

	/** How many pieces there are */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance, meta=(ClampMin = "1", UIMin = "1", UIMax = "8"))
	int32 NumberOfPieces;

	/** Should the pieces animate horizontally? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	bool bAnimateHorizontally;

	/** Should the pieces animate vertically? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	bool bAnimateVertically;

	/** Should the pieces animate their opacity? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	bool bAnimateOpacity;

	/** Image to use for each segment of the throbber */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FSlateBrush PieceImage;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface
};

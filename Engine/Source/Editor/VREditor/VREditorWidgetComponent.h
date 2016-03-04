// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/WidgetComponent.h"
#include "VREditorWidgetComponent.generated.h"

/**
 * A specialized WidgetComponent for the VREditor
 */
UCLASS(hidedropdown)
class UVREditorWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	/** Default constructor that sets up CDO properties */
	UVREditorWidgetComponent(const FObjectInitializer& ObjectInitializer);

	void SetAllowDrawing(bool Value) { bAllowDrawing = Value; }
	bool GetAllowDrawing() const { return bAllowDrawing; }

protected:
	virtual bool ShouldDrawWidget() const override;

	virtual void DrawWidgetToRenderTarget(float DeltaTime) override;

private:
	/**
	 * Controls if we draw, the VREditorWidget allows for manual enabling or 
	 * disabling of updating the slate widget.
	 */
	UPROPERTY()
	bool bAllowDrawing;

	/**
	 * Recorders if we've drawn at least once, that way we can always draw the first
	 * frame then go manual after that.
	 */
	UPROPERTY()
	bool bHasEverDrawn;
};
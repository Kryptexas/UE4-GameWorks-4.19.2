// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __DRAGTOOL_MEASURE_H__
#define __DRAGTOOL_MEASURE_H__

#include "EditorDragTools.h"

/**
 * Draws a line in the current viewport and displays the distance between
 * its start/end points in the center of it.
 */
class FDragTool_Measure : public FDragTool
{
public:
	explicit FDragTool_Measure(FEditorViewportClient* InViewportClient);

	/**
	 * Starts a mouse drag behavior.  The start location is snapped to the editor constraints if bUseSnapping is true.
	 *
	 * @param	InViewportClient	The viewport client in which the drag event occurred.
	 * @param	InStart				Where the mouse was when the drag started (world space).
	 * @param	InStartScreen		Where the mouse was when the drag started (screen space).
	 */
	virtual void StartDrag(FEditorViewportClient* InViewportClient, const FVector& InStart, const FVector2D& InStartScreen) OVERRIDE;

	/* Updates the drag tool's end location with the specified delta.  The end location is
	 * snapped to the editor constraints if bUseSnapping is true.
	 *
	 * @param	InDelta		A delta of mouse movement.
	 */
	virtual void AddDelta(const FVector& InDelta) OVERRIDE;

	virtual void Render(const FSceneView* View,FCanvas* Canvas) OVERRIDE;

private:
	/**
	 * Sets the End member explicitly from the current cursor position, overriding the calculation based on
	 * the deltas passed to AddDelta.
	 */
	void SetEndWorldPositionFromCursor();

	FEditorViewportClient* ViewportClient;
};

#endif // __DRAGTOOL_MEASURE_H__

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DebugRenderSceneProxy.h: Useful scene proxy for rendering non performance-critical information.


=============================================================================*/

#include "EnginePrivate.h"
#include "DebugRenderSceneProxy.h"

// FPrimitiveSceneProxy interface.

/** 
* Draw the scene proxy as a dynamic element
*
* @param	PDI - draw interface to render to
* @param	View - current view
*/
void FDebugRenderSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View)
{
	QUICK_SCOPE_CYCLE_COUNTER( STAT_DebugRenderSceneProxy_DrawDynamicElements );

	// Draw Lines
	for(int32 LineIdx=0; LineIdx<Lines.Num(); LineIdx++)
	{
		FDebugLine& Line = Lines[LineIdx];

		PDI->DrawLine(Line.Start, Line.End, Line.Color, SDPG_World);
	}

	// Draw Arrows
	for(int32 LineIdx=0; LineIdx<ArrowLines.Num(); LineIdx++)
	{
		FArrowLine &Line = ArrowLines[LineIdx];

		DrawLineArrow(PDI, Line.Start, Line.End, Line.Color, 8.0f);
	}

	// Draw Cylinders
	for(int32 CylinderIdx=0; CylinderIdx<Cylinders.Num(); CylinderIdx++)
	{
		FWireCylinder& Cylinder = Cylinders[CylinderIdx];

		DrawWireCylinder( PDI, Cylinder.Base, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1),
			Cylinder.Color, Cylinder.Radius, Cylinder.HalfHeight, 16, SDPG_World );
	}

	// Draw Stars
	for(int32 StarIdx=0; StarIdx<Stars.Num(); StarIdx++)
	{
		FWireStar& Star = Stars[StarIdx];

		DrawWireStar(PDI, Star.Position, Star.Size, Star.Color, SDPG_World);
	}

	// Draw Dashed Lines
	for(int32 DashIdx=0; DashIdx<DashedLines.Num(); DashIdx++)
	{
		FDashedLine& Dash = DashedLines[DashIdx];

		DrawDashedLine(PDI, Dash.Start, Dash.End, Dash.Color, Dash.DashSize, SDPG_World);
	}

	// Draw Boxes
	for(int32 BoxIdx=0; BoxIdx<WireBoxes.Num(); BoxIdx++)
	{
		FDebugBox& Box = WireBoxes[BoxIdx];

		DrawWireBox( PDI, Box.Box, Box.Color, SDPG_World);
	}

}



/**
* Draws a line with an arrow at the end.
*
* @param Start		Starting point of the line.
* @param End		Ending point of the line.
* @param Color		Color of the line.
* @param Mag		Size of the arrow.
*/
void FDebugRenderSceneProxy::DrawLineArrow(FPrimitiveDrawInterface* PDI,const FVector &Start,const FVector &End,const FColor &Color,float Mag)
{
	// draw a pretty arrow
	FVector Dir = End - Start;
	const float DirMag = Dir.Size();
	Dir /= DirMag;
	FVector YAxis, ZAxis;
	Dir.FindBestAxisVectors(YAxis,ZAxis);
	FMatrix ArrowTM(Dir,YAxis,ZAxis,Start);
	DrawDirectionalArrow(PDI,ArrowTM,Color,DirMag,Mag,SDPG_World);
}

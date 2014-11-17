// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DebugRenderSceneProxy.h: Useful scene proxy for rendering non performance-critical information.


=============================================================================*/

#include "EnginePrivate.h"
#include "DebugRenderSceneProxy.h"

// FPrimitiveSceneProxy interface.

FDebugRenderSceneProxy::FDebugRenderSceneProxy(const UPrimitiveComponent* InComponent) 
	: FPrimitiveSceneProxy(InComponent)
	, ViewFlagIndex(uint32(FEngineShowFlags::FindIndexByName(TEXT("Game"))))
	, TextWithoutShadowDistance(1500)
{

}

void FDebugRenderSceneProxy::RegisterDebugDrawDelgate()
{
	DebugTextDrawingDelegate = FDebugDrawDelegate::CreateRaw(this, &FDebugRenderSceneProxy::DrawDebugLabels);
	UDebugDrawService::Register(*ViewFlagName, DebugTextDrawingDelegate);
}

void FDebugRenderSceneProxy::UnregisterDebugDrawDelgate()
{
	if (DebugTextDrawingDelegate.IsBound())
	{
		UDebugDrawService::Unregister(DebugTextDrawingDelegate);
	}
}

uint32 FDebugRenderSceneProxy::GetAllocatedSize(void) const 
{ 
	return	FPrimitiveSceneProxy::GetAllocatedSize() + 
		Cylinders.GetAllocatedSize() + 
		ArrowLines.GetAllocatedSize() + 
		Stars.GetAllocatedSize() + 
		DashedLines.GetAllocatedSize() + 
		Lines.GetAllocatedSize() + 
		WireBoxes.GetAllocatedSize() + 
		SolidSpheres.GetAllocatedSize() +
		Texts.GetAllocatedSize();
}


void FDebugRenderSceneProxy::DrawDebugLabels(UCanvas* Canvas, APlayerController*)
{
	const FColor OldDrawColor = Canvas->DrawColor;
	const FFontRenderInfo FontRenderInfo = Canvas->CreateFontRenderInfo(true, false);
	const FFontRenderInfo FontRenderInfoWithShadow = Canvas->CreateFontRenderInfo(true, true);

	Canvas->SetDrawColor(FColor::White);

	UFont* RenderFont = GEngine->GetSmallFont();

	const FSceneView* View = Canvas->SceneView;
	for (auto It = Texts.CreateConstIterator(); It; ++It)
	{
		if (PointInView(It->Location, View))
		{
			const FVector ScreenLoc = Canvas->Project(It->Location);
			const FFontRenderInfo& FontInfo = TextWithoutShadowDistance >= 0 ? (PointWithinCorrectDistance(It->Location, View, TextWithoutShadowDistance) ? FontRenderInfoWithShadow : FontRenderInfo) : FontRenderInfo;
			Canvas->DrawText(RenderFont, It->Text, ScreenLoc.X, ScreenLoc.Y, 1, 1, FontInfo);
		}
	}

	Canvas->SetDrawColor(OldDrawColor);
}

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

	// Draw solid spheres
	struct FMaterialCache
	{
		FMaterialCache() {}

		FMaterialRenderProxy* operator[](FLinearColor Color) const
		{
			FMaterialRenderProxy* MeshColor = NULL;
			const uint32 HashKey = GetTypeHash(Color);
			if (MeshColorInstances.Contains(HashKey))
			{
				MeshColor = *MeshColorInstances.Find(HashKey);
			}
			else
			{
				MeshColor = new(FMemStack::Get()) FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(false), Color);
				MeshColorInstances.Add(HashKey, MeshColor);
			}

			return MeshColor;
		}

		mutable TMap<uint32, FMaterialRenderProxy*> MeshColorInstances;
	};
	const FMaterialCache MaterialCache;

	for (auto It = SolidSpheres.CreateConstIterator(); It; ++It)
	{
		if (PointInView(It->Location, View))
		{
			DrawSphere(PDI, It->Location, FVector(It->Radius), 10, 7, MaterialCache[It->Color], SDPG_World);
		}
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

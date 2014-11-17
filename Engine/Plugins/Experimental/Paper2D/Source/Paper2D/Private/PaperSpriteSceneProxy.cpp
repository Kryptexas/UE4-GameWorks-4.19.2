// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperSpriteSceneProxy.h"
#include "PhysicsEngine/BodySetup2D.h"

//////////////////////////////////////////////////////////////////////////
// FPaperSpriteSceneProxy

FPaperSpriteSceneProxy::FPaperSpriteSceneProxy(const UPaperSpriteComponent* InComponent)
	: FPaperRenderSceneProxy(InComponent)
	, SourceSprite(nullptr)
{
	WireframeColor = InComponent->GetWireframeColor();
	Material = InComponent->GetMaterial(0);
	AlternateMaterial = InComponent->GetMaterial(1);
	MaterialSplitIndex = INDEX_NONE;
	MaterialRelevance = InComponent->GetMaterialRelevance();

	SourceSprite = InComponent->SourceSprite; //@TODO: This is totally not threadsafe, and won't keep up to date if the actor's sprite changes, etc....
}

void FPaperSpriteSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View)
{
	if (SourceSprite != nullptr)
	{
		// Show 3D physics
		if ((View->Family->EngineShowFlags.Collision /*@TODO: && bIsCollisionEnabled*/) && AllowDebugViewmodes())
		{
			if (UBodySetup2D* BodySetup2D = Cast<UBodySetup2D>(SourceSprite->BodySetup))
			{
				//@TODO: Draw 2D debugging geometry
			}
			else if (UBodySetup* BodySetup = SourceSprite->BodySetup)
			{
				if (FMath::Abs(GetLocalToWorld().Determinant()) < SMALL_NUMBER)
				{
					// Catch this here or otherwise GeomTransform below will assert
					// This spams so commented out
					//UE_LOG(LogStaticMesh, Log, TEXT("Zero scaling not supported (%s)"), *StaticMesh->GetPathName());
				}
				else
				{
					// Make a material for drawing solid collision stuff
					const UMaterial* LevelColorationMaterial = View->Family->EngineShowFlags.Lighting 
						? GEngine->ShadedLevelColorationLitMaterial : GEngine->ShadedLevelColorationUnlitMaterial;

					const FColoredMaterialRenderProxy CollisionMaterialInstance(
						LevelColorationMaterial->GetRenderProxy(IsSelected(), IsHovered()),
						WireframeColor
						);

					// Draw the static mesh's body setup.

					// Get transform without scaling.
					FTransform GeomTransform(GetLocalToWorld());

					// In old wireframe collision mode, always draw the wireframe highlighted (selected or not).
					bool bDrawWireSelected = IsSelected();
					if (View->Family->EngineShowFlags.Collision)
					{
						bDrawWireSelected = true;
					}

					// Differentiate the color based on bBlockNonZeroExtent.  Helps greatly with skimming a level for optimization opportunities.
					const FColor CollisionColor = FColor(220,149,223,255);

					const bool bUseSeparateColorPerHull = (Owner == NULL);
					const bool bDrawSolid = false;
					BodySetup->AggGeom.DrawAggGeom(PDI, GeomTransform, GetSelectionColor(CollisionColor, bDrawWireSelected, IsHovered()), &CollisionMaterialInstance, bUseSeparateColorPerHull, bDrawSolid);
				}
			}
		}
	}

	FPaperRenderSceneProxy::DrawDynamicElements(PDI, View);
}

void FPaperSpriteSceneProxy::SetSprite_RenderThread(const FSpriteDrawCallRecord& NewDynamicData, int32 SplitIndex)
{
	BatchedSprites.Empty();
	AlternateBatchedSprites.Empty();

	if (SplitIndex != INDEX_NONE)
	{
		FSpriteDrawCallRecord& Record = *new (BatchedSprites) FSpriteDrawCallRecord;
		FSpriteDrawCallRecord& AltRecord = *new (AlternateBatchedSprites) FSpriteDrawCallRecord;
		
		Record.Color = NewDynamicData.Color;
		Record.Destination = NewDynamicData.Destination;
		Record.Texture = NewDynamicData.Texture;
		Record.RenderVerts.Append(NewDynamicData.RenderVerts.GetTypedData(), SplitIndex);

		AltRecord.Color = NewDynamicData.Color;
		AltRecord.Destination = NewDynamicData.Destination;
		AltRecord.Texture = NewDynamicData.Texture;
		AltRecord.RenderVerts.Append(NewDynamicData.RenderVerts.GetTypedData() + SplitIndex, NewDynamicData.RenderVerts.Num() - SplitIndex);
	}
	else
	{
		FSpriteDrawCallRecord& Record = *new (BatchedSprites) FSpriteDrawCallRecord;
		Record = NewDynamicData;
	}
}

void FPaperSpriteSceneProxy::DrawDynamicElements_RichMesh(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor)
{
	if (Material != nullptr)
	{
		DrawBatch(PDI, View, bUseOverrideColor, OverrideColor, Material, BatchedSprites);
	}

	if ((AlternateMaterial != nullptr) && (AlternateBatchedSprites.Num() > 0))
	{
		DrawBatch(PDI, View, bUseOverrideColor, OverrideColor, AlternateMaterial, AlternateBatchedSprites);
	}
}

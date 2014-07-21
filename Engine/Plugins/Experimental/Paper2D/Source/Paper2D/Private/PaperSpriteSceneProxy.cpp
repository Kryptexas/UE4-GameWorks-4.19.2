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
	SourceSprite = InComponent->SourceSprite; //@TODO: This is totally not threadsafe, and won't keep up to date if the actor's sprite changes, etc....
	
	if (SourceSprite)
	{
		Material = SourceSprite->GetDefaultMaterial();
	}
		
	if (InComponent->MaterialOverride)
	{
		Material = InComponent->MaterialOverride;
	}

	if (Material)
	{
		MaterialRelevance = Material->GetRelevance();
	}
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

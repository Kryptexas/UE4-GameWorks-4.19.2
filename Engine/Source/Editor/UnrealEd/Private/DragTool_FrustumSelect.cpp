// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "DragTool_FrustumSelect.h"
#include "ActorEditorUtils.h"
#include "ScopedTransaction.h"
#include "HModel.h"

///////////////////////////////////////////////////////////////////////////////
//
// FDragTool_FrustumSelect
//
///////////////////////////////////////////////////////////////////////////////

/** 
 * Updates the drag tool's end location with the specified delta.  The end location is
 * snapped to the editor constraints if bUseSnapping is true.
 *
 * @param	InDelta		A delta of mouse movement.
 */
void FDragTool_ActorFrustumSelect::AddDelta( const FVector& InDelta )
{
	FIntPoint MousePos;
	ViewportClient->Viewport->GetMousePos(MousePos);

	EndWk = FVector(MousePos);
	End = EndWk;

	const bool bUseHoverFeedback = GEditor != NULL && GetDefault<ULevelEditorViewportSettings>()->bEnableViewportHoverFeedback;
}


void FDragTool_ActorFrustumSelect::StartDrag(FEditorViewportClient* InViewportClient, const FVector& InStart, const FVector2D& InStartScreen)
{
	FDragTool::StartDrag(InViewportClient, InStart, InStartScreen);

	ViewportClient = InViewportClient;

	const bool bUseHoverFeedback = GEditor != NULL && GetDefault<ULevelEditorViewportSettings>()->bEnableViewportHoverFeedback;

	// Remove any active hover objects
	FLevelEditorViewportClient::ClearHoverFromObjects();

	FIntPoint MousePos;
	InViewportClient->Viewport->GetMousePos(MousePos);

	Start = FVector(InStartScreen.X, InStartScreen.Y, 0);
	End = EndWk = Start;

}

/**
* Ends a mouse drag behavior (the user has let go of the mouse button).
*/
void FDragTool_ActorFrustumSelect::EndDrag()
{
	FEditorModeTools& EdModeTools = GEditorModeTools();
	const bool bGeometryMode = EdModeTools.IsModeActive( FBuiltinEditorModes::EM_Geometry );

	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(ViewportClient->Viewport, ViewportClient->GetScene(), ViewportClient->EngineShowFlags ));
	FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);

	// Generate a frustum out of the dragged box
	FConvexVolume Frustum;
	CalculateFrustum( SceneView, Frustum, true );

	FScopedTransaction Transaction( NSLOCTEXT("ActorFrustumSelect", "MarqueeSelectTransation", "Marquee Select" ) );

	bool bShouldSelect = true;

	if( bAltDown )
	{
		// If control is down remove from selection
		bShouldSelect = false;
	}
	else if( !bShiftDown )
	{
		// If the user is selecting, but isn't hold down SHIFT, remove all current selections.
		GEditorModeTools().SelectNone();
	}

	// Does an actor have to be fully contained in the box to be selected
	const bool bStrictDragSelection = GetDefault<ULevelEditorViewportSettings>()->bStrictBoxSelection;
	bool bSelectionChanged = false;

	// Let the editor mode try to handle the selection.
	const bool bEditorModeHandledSelection = GEditorModeTools().FrustumSelect( Frustum, bLeftMouseButtonDown );

	if( !bEditorModeHandledSelection )
	{
		if( bAltDown )
		{
			// If control is down remove from selection
			bShouldSelect = false;
		}
		else if( !bShiftDown )
		{
			// If the user is selecting, but isn't hold down SHIFT, remove all current selections.
			GEditor->SelectNone( true, true );
		}
		

		const int32 ViewportSizeX = ViewportClient->Viewport->GetSizeXY().X;
		const int32 ViewportSizeY = ViewportClient->Viewport->GetSizeXY().Y;

		if( Start.X > End.X )
		{
			Swap( Start.X, End.X );
		}

		if( Start.Y > End.Y )
		{
			Swap( Start.Y, End.Y );
		}

		// Extend the endpoint of the rect to get the actual line
		FIntRect BoxRect( FIntPoint( FMath::Max( 0.0f, Start.X ), FMath::Max( 0.0f, Start.Y ) ), FIntPoint( FMath::Min(ViewportSizeX, FMath::Trunc(End.X+1)), FMath::Min( ViewportSizeY, FMath::Trunc(End.Y+1) ) ) );

		const TArray<FColor>& RawHitProxyData = ViewportClient->Viewport->GetRawHitProxyData(BoxRect);

		TSet<AActor*> HitActors;
		TSet<UModel*> HitModels;


		// Lower the resolution with massive box selects
		int32 Step = (BoxRect.Width() > 500 && BoxRect.Height() > 500) ? 4 : 1;


		for (int32 Y = BoxRect.Min.Y; Y < BoxRect.Max.Y; Y = Y < BoxRect.Max.Y - 1 ? FMath::Min(BoxRect.Max.Y-1, Y+Step) : ++Y )
		{
			const FColor* SourceData = &RawHitProxyData[Y * ViewportSizeX];
			for (int32 X = BoxRect.Min.X; X < BoxRect.Max.X; X = X < BoxRect.Max.X-1 ? FMath::Min(BoxRect.Max.X-1, X + Step) : ++X )
			{
				FHitProxyId HitProxyId(SourceData[X]);
				HHitProxy* HitProxy = GetHitProxyById(HitProxyId);

				if (HitProxy)
				{
					if( HitProxy->IsA(HActor::StaticGetType()) )
					{
						AActor* Actor = ((HActor*)HitProxy)->Actor;
						if (Actor)
						{
							HitActors.Add(Actor);
						}
					}
					else if( HitProxy->IsA(HModel::StaticGetType()) )
					{
						HitModels.Add( ((HModel*)HitProxy)->GetModel() );
					}
					else if( HitProxy->IsA(HBSPBrushVert::StaticGetType()) )
					{
						HBSPBrushVert* HitBSPBrushVert = ((HBSPBrushVert*)HitProxy);
						if( HitBSPBrushVert->Brush.IsValid() )
						{
							HitActors.Add( HitBSPBrushVert->Brush.Get() );
						}
					}
				}

			}
		}


		if (HitModels.Num() > 0)
		{
			// Check every model to see if its BSP surfaces should be selected
			for (auto It = HitModels.CreateConstIterator(); It; ++It)
			{
				UModel& Model = **It;
				// Check every node in the model
				for (int32 NodeIndex = 0; NodeIndex < Model.Nodes.Num(); NodeIndex++)
				{
					if (IntersectsFrustum(Model, NodeIndex, Frustum, bStrictDragSelection))
					{
						uint32 SurfaceIndex = Model.Nodes[NodeIndex].iSurf;
						FBspSurf& Surf = Model.Surfs[SurfaceIndex];
						HitActors.Add( Surf.Actor );
					}
				}
			}
		}

		if( HitActors.Num() > 0 )
		{
			for( auto It = HitActors.CreateConstIterator(); It; ++It )
			{
				AActor* Actor = *It;
				if( bStrictDragSelection && IntersectsFrustum( *Actor, Frustum, bStrictDragSelection ) )
				{
					GEditor->SelectActor(Actor, bShouldSelect, false);
					bSelectionChanged = true;
				}
				else if( !bStrictDragSelection )
				{
					GEditor->SelectActor(Actor, bShouldSelect, false);
					bSelectionChanged = true;
				}
			}
		}

		if(bSelectionChanged)
		{
			// If any selections were made.  Notify that now.
			GEditor->NoteSelectionChange();
		}
	}

	// Clear any hovered objects that might have been created while dragging
	FLevelEditorViewportClient::ClearHoverFromObjects();

	FDragTool::EndDrag();
}

void FDragTool_ActorFrustumSelect::Render(const FSceneView* View, FCanvas* Canvas )
{
	FCanvasBoxItem BoxItem( FVector2D(Start.X, Start.Y), FVector2D(End.X-Start.X, End.Y-Start.Y) );
	BoxItem.SetColor( FLinearColor::White );
	Canvas->DrawItem( BoxItem );
}

/** 
 * Returns true if the mesh on the component has vertices which intersect the frustum
 *
 * @param InComponent			The static mesh or skeletal mesh component to check
 * @param InFrustum				The frustum to check against.
 * @param bUseStrictSelection	true if all the vertices must be entirely within the frustum
 */
bool FDragTool_ActorFrustumSelect::IntersectsVertices( UPrimitiveComponent& InComponent, const FConvexVolume& InFrustum, bool bUseStrictSelection ) const
{
	bool bAlreadyProcessed = false;
	bool bResult = false;
	UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(&InComponent);
	USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(&InComponent);

	if( StaticMeshComponent && StaticMeshComponent->StaticMesh && ViewportClient->EngineShowFlags.StaticMeshes )
	{
		check( StaticMeshComponent->StaticMesh->RenderData );
		if( StaticMeshComponent->StaticMesh->RenderData->LODResources.Num() > 0 )
		{
			const FStaticMeshLODResources& LODModel = StaticMeshComponent->StaticMesh->RenderData->LODResources[0];
			uint32 NumVertices = LODModel.VertexBuffer.GetNumVertices();
			for (uint32 VertexIndex = 0; VertexIndex < NumVertices; ++VertexIndex)
			{
				const FVector& LocalPosition = LODModel.PositionVertexBuffer.VertexPosition(VertexIndex);
				const FVector& WorldPosition = StaticMeshComponent->ComponentToWorld.TransformPosition(LocalPosition);
				bool bLocationIntersected = InFrustum.IntersectBox(WorldPosition, FVector::ZeroVector);
				if (bLocationIntersected && !bUseStrictSelection)
				{
					bResult = true;
					bAlreadyProcessed = true;
					break;
				}
				else if (!bLocationIntersected && bUseStrictSelection)
				{
					bResult = false;
					bAlreadyProcessed = true;
					break;
				}
			}
		}

		if (!bAlreadyProcessed && bUseStrictSelection)
		{
			bResult = true;
			bAlreadyProcessed = true;
		}
	}
	else if( SkeletalMeshComponent && SkeletalMeshComponent->MeshObject && ViewportClient->EngineShowFlags.SkeletalMeshes )
	{
		FSkeletalMeshResource* SkelMeshResource = SkeletalMeshComponent->GetSkeletalMeshResource();
		check(SkelMeshResource);
		check( SkelMeshResource->LODModels.Num() > 0 );

		const FStaticLODModel& LODModel = SkelMeshResource->LODModels[0];
		for( int32 ChunkIndex = 0 ; ChunkIndex < LODModel.Chunks.Num() && !bAlreadyProcessed; ++ChunkIndex )
		{
			const FSkelMeshChunk& Chunk = LODModel.Chunks[ChunkIndex];
			for( int32 VertexIndex = 0; VertexIndex < Chunk.RigidVertices.Num(); ++VertexIndex )
			{
				const FVector Location = SkeletalMeshComponent->ComponentToWorld.TransformPosition( Chunk.RigidVertices[VertexIndex].Position );
				const bool bLocationIntersected = InFrustum.IntersectBox( Location, FVector::ZeroVector );

				// If the selection box doesn't have to encompass the entire component and a skeletal mesh vertex has intersected with
				// the selection box, this component is being touched by the selection box
				if( bLocationIntersected && !bUseStrictSelection )
				{
					bResult = true;
					bAlreadyProcessed = true;
					break;
				}

				// If the selection box has to encompass the entire component and a skeletal mesh vertex didn't intersect with the selection
				// box, this component does not qualify
				else if ( !bLocationIntersected && bUseStrictSelection )
				{
					bResult = false;
					bAlreadyProcessed = true;
					break;
				}
			}

			for( int32 VertexIndex = 0 ; VertexIndex < Chunk.SoftVertices.Num() && !bAlreadyProcessed ; ++VertexIndex )
			{
				const FVector Location = SkeletalMeshComponent->ComponentToWorld.TransformPosition( Chunk.SoftVertices[VertexIndex].Position );
				const bool bLocationIntersected = InFrustum.IntersectBox( Location, FVector::ZeroVector );

				// If the selection box doesn't have to encompass the entire component and a skeletal mesh vertex has intersected with
				// the selection box, this component is being touched by the selection box
				if( bLocationIntersected && !bUseStrictSelection )
				{
					bResult = true;
					bAlreadyProcessed = true;
					break;
				}

				// If the selection box has to encompass the entire component and a skeletal mesh vertex didn't intersect with the selection
				// box, this component does not qualify
				else if ( !bLocationIntersected && bUseStrictSelection  )
				{
					bResult = false;
					bAlreadyProcessed = true;
					break;
				}
			}

			// If the selection box has to encompass all of the component and none of the component's verts failed the intersection test, this component
			// is consider touching
			if ( !bAlreadyProcessed && bUseStrictSelection )
			{
				bResult = true;
				bAlreadyProcessed = true;
			}
		}
	}

	return bResult;
}

bool FDragTool_ActorFrustumSelect::IntersectsFrustum( AActor& InActor, const FConvexVolume& InFrustum, bool bUseStrictSelection ) const
{	
	bool bActorHitByBox = false;
	bool bActorFullyContained = false;

	// Check for special cases (like certain show flags that might hide an actor)
	bool bActorIsHiddenByShowFlags = false;

	FLevelEditorViewportClient* LevelViewportClient = NULL;
	if ( ViewportClient->IsLevelEditorClient()  )
	{
		LevelViewportClient = (FLevelEditorViewportClient*)ViewportClient;
	}

	// Check to see that volume actors are visible in the viewport
	if( LevelViewportClient && InActor.IsA(AVolume::StaticClass()) && (!LevelViewportClient->EngineShowFlags.Volumes || !LevelViewportClient->IsVolumeVisibleInViewport(InActor) ) )
	{
		bActorIsHiddenByShowFlags = true;
	}

	// Never drag-select hidden actors or builder brushes. 
	if( !bActorIsHiddenByShowFlags && !InActor.IsHiddenEd() && !FActorEditorUtils::IsABuilderBrush(&InActor) )
	{
		// Determine if the user would prefer to use strict selection or not
		const bool bStrictDragSelection = GetDefault<ULevelEditorViewportSettings>()->bStrictBoxSelection;

		// Iterate over all actor components, selecting out primitive components
		TArray<UPrimitiveComponent*> Components;
		InActor.GetComponents(Components);

		for( int32 ComponentIndex = 0 ; ComponentIndex < Components.Num() ; ++ComponentIndex )
		{
			UPrimitiveComponent* PrimitiveComponent = Components[ComponentIndex];
			if ( PrimitiveComponent->IsRegistered() && PrimitiveComponent->IsVisibleInEditor() )
			{
				FVector Origin = PrimitiveComponent->Bounds.Origin;
				FVector Extent;
				if( PrimitiveComponent->IsA(UDrawSphereComponent::StaticClass()) )
				{
					// Do not select by DrawLight Radii or Cones
					continue;
				}
				else if( PrimitiveComponent->IsA(UNavLinkRenderingComponent::StaticClass() ) )
				{
					continue;
				}
				if(PrimitiveComponent->IsA(UBillboardComponent::StaticClass()) && Cast<UBillboardComponent>(PrimitiveComponent)->Sprite != NULL)
				{
					// Use the size of the sprite itself rather than its box extent
					UBillboardComponent* SpriteComponent = Cast<UBillboardComponent>(PrimitiveComponent);
					float Scale = SpriteComponent->ComponentToWorld.GetMaximumAxisScale();
					Extent = FVector(Scale * SpriteComponent->Sprite->GetSizeX()/2.0f, Scale * SpriteComponent->Sprite->GetSizeY()/2.0f, 0.0f);
				}
				else
				{
					Extent = PrimitiveComponent->Bounds.BoxExtent;
				}


				UBrushComponent* BrushComponent = Cast<UBrushComponent>( PrimitiveComponent );
				if( BrushComponent && BrushComponent->Brush && BrushComponent->Brush->Polys )
				{
					bool bAlreadyProcessed = false;

					// Need to check each vert of the brush separately
					for( int32 PolyIndex = 0 ; PolyIndex < BrushComponent->Brush->Polys->Element.Num() && !bAlreadyProcessed ; ++PolyIndex )
					{
						const FPoly& Poly = BrushComponent->Brush->Polys->Element[ PolyIndex ];

						for( int32 VertexIndex = 0 ; VertexIndex < Poly.Vertices.Num() ; ++VertexIndex )
						{
							const FVector Location = BrushComponent->ComponentToWorld.TransformPosition( Poly.Vertices[VertexIndex] );
							const bool bIntersect = InFrustum.IntersectBox( Location, FVector::ZeroVector );

							if( bIntersect && !bStrictDragSelection )
							{
								// If we intersected a vertex and we dont require the box to encompass the entire component
								// then the actor should be selected and we can stop checking
								bAlreadyProcessed = true;
								bActorHitByBox = true;
								break;
							}
							else if( !bIntersect && bStrictDragSelection )
							{
								// If we didnt intersect a vertex but we require the box to encompass the entire component
								// then this test failed and we can stop checking
								bAlreadyProcessed = true;
								bActorHitByBox = false;
								break;
							}
						}
					}

					if( bStrictDragSelection && !bAlreadyProcessed )
					{
						// If we are here then every vert was intersected so we should select the actor
						bActorHitByBox = true;
						bAlreadyProcessed = true;
					}

					if( bAlreadyProcessed )
					{
						// if this component has been processed, dont bother checking other components
						break;
					}
				}
				else
				{
					if ( InFrustum.IntersectBox( Origin, Extent, bActorFullyContained) 
						&& (!bStrictDragSelection || bActorFullyContained))
					{
						if( PrimitiveComponent->IsA( UStaticMeshComponent::StaticClass() ) || PrimitiveComponent->IsA( USkeletalMeshComponent::StaticClass() ) )
						{
							// Check each vertex on the component's mesh for intersection
							bActorHitByBox = IntersectsVertices( *PrimitiveComponent, InFrustum, bUseStrictSelection );
						}
						else
						{
							// Use the bounding box check for all other components
							bActorHitByBox = true;
						}
							
						if( bActorHitByBox )
						{
							// no need to continue checking other components
							break;
						}
					}
				}
			}
		}
	}

	return bActorHitByBox;
}

/** 
 * Returns true if the provided BSP node intersects with the provided frustum 
 *
 * @param InModel				The model containing BSP nodes to check
 * @param NodeIndex				The index to a BSP node in the model.  This node is used for the bounds check.
 * @param InFrustum				The frustum to check against.
 * @param bUseStrictSelection	true if the node must be entirely within the frustum
 */
bool FDragTool_ActorFrustumSelect::IntersectsFrustum( const UModel& InModel, int32 NodeIndex, const FConvexVolume& InFrustum, bool bUseStrictSelection ) const
{
	FBox NodeBB;
	// Get a bounding box of the node being checked
	InModel.GetNodeBoundingBox( InModel.Nodes[NodeIndex], NodeBB );

	bool bFullyContained = false;

	// Does the box intersect the frustum
	bool bIntersects = InFrustum.IntersectBox( NodeBB.GetCenter(), NodeBB.GetExtent(), bFullyContained );

	return bIntersects && (!bUseStrictSelection || bUseStrictSelection && bFullyContained );
}

/** 
 * Calculates a frustum to check actors against 
 * 
 * @param OutFrustum		The created frustum
 * @param bUseBoxFrustum	If true a frustum out of the current dragged box will be created.  false will use the view frustum.
 */
void FDragTool_ActorFrustumSelect::CalculateFrustum( FSceneView* View, FConvexVolume& OutFrustum, bool bUseBoxFrustum )
{
	if( bUseBoxFrustum )
	{
		FVector CamPoint = ViewportClient->GetViewLocation();
		FVector BoxPoint1, BoxPoint2, BoxPoint3, BoxPoint4;
		FVector WorldDir1, WorldDir2, WorldDir3, WorldDir4;
		// Deproject the four corners of the selection box
		FVector2D Point1(FMath::Min(Start.X, End.X), FMath::Min(Start.Y, End.Y)); // Upper Left Corner
		FVector2D Point2(FMath::Max(Start.X, End.X), FMath::Min(Start.Y, End.Y)); // Upper Right Corner
		FVector2D Point3(FMath::Max(Start.X, End.X), FMath::Max(Start.Y, End.Y)); // Lower Right Corner
		FVector2D Point4(FMath::Min(Start.X, End.X), FMath::Max(Start.Y, End.Y)); // Lower Left Corner
		View->DeprojectFVector2D(Point1, BoxPoint1, WorldDir1);
		View->DeprojectFVector2D(Point2, BoxPoint2, WorldDir2);
		View->DeprojectFVector2D(Point3, BoxPoint3, WorldDir3);
		View->DeprojectFVector2D(Point4, BoxPoint4, WorldDir4);
		// Use the camera position and the selection box to create the bounding planes
		FPlane TopPlane(BoxPoint1, BoxPoint2, CamPoint); // Top Plane
		FPlane RightPlane(BoxPoint2, BoxPoint3, CamPoint); // Right Plane
		FPlane BottomPlane(BoxPoint3, BoxPoint4, CamPoint); // Bottom Plane
		FPlane LeftPlane(BoxPoint4, BoxPoint1, CamPoint); // Left Plane

		// Try to get all six planes to create a frustum
		FPlane NearPlane;
		FPlane FarPlane;
		OutFrustum.Planes.Empty();
		if ( View->ViewProjectionMatrix.GetFrustumNearPlane(NearPlane) )
		{
			OutFrustum.Planes.Add(NearPlane);
		}
		if ( View->ViewProjectionMatrix.GetFrustumFarPlane(FarPlane) )
		{
			OutFrustum.Planes.Add(FarPlane);
		}
		OutFrustum.Planes.Add(TopPlane);
		OutFrustum.Planes.Add(RightPlane);
		OutFrustum.Planes.Add(BottomPlane);
		OutFrustum.Planes.Add(LeftPlane);
		OutFrustum.Init();
	}
	else
	{
		OutFrustum = View->ViewFrustum;
		OutFrustum.Init();
	}
}

/** Adds a hover effect to the passed in actor */
void FDragTool_ActorFrustumSelect::AddHoverEffect( AActor& InActor )
{
	FViewportHoverTarget HoverTarget( &InActor );
	FLevelEditorViewportClient::AddHoverEffect( HoverTarget );
	FLevelEditorViewportClient::HoveredObjects.Add( HoverTarget );
}

/** Removes a hover effect from the passed in actor */
void FDragTool_ActorFrustumSelect::RemoveHoverEffect( AActor& InActor  )
{
	FViewportHoverTarget HoverTarget( &InActor );
	FSetElementId Id = FLevelEditorViewportClient::HoveredObjects.FindId( HoverTarget );
	if( Id.IsValidId() )
	{
		FLevelEditorViewportClient::RemoveHoverEffect( HoverTarget );
		FLevelEditorViewportClient::HoveredObjects.Remove( Id );
	}
}

/** Adds a hover effect to the passed in bsp surface */
void FDragTool_ActorFrustumSelect::AddHoverEffect( UModel& InModel, int32 SurfIndex )
{
	FViewportHoverTarget HoverTarget( &InModel, SurfIndex );
	FLevelEditorViewportClient::AddHoverEffect( HoverTarget );
	FLevelEditorViewportClient::HoveredObjects.Add( HoverTarget );
}

/** Removes a hover effect from the passed in bsp surface */
void FDragTool_ActorFrustumSelect::RemoveHoverEffect( UModel& InModel, int32 SurfIndex )
{
	FViewportHoverTarget HoverTarget( &InModel, SurfIndex );
	FSetElementId Id = FLevelEditorViewportClient::HoveredObjects.FindId( HoverTarget );
	if( Id.IsValidId() )
	{
		FLevelEditorViewportClient::RemoveHoverEffect( HoverTarget );
		FLevelEditorViewportClient::HoveredObjects.Remove( Id );
	}
}

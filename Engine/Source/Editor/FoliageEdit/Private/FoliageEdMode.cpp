// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	FoliageEdMode.cpp: Foliage editing mode
================================================================================*/

#include "UnrealEd.h"
#include "ObjectTools.h"
#include "FoliageEdMode.h"
#include "ScopedTransaction.h"
#include "EngineFoliageClasses.h"

#include "FoliageEdModeToolkit.h"
#include "ModuleManager.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Toolkits/ToolkitManager.h"

#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"

//Slate dependencies
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/LevelEditor/Public/SLevelViewport.h"

#define FOLIAGE_SNAP_TRACE (10000.f)

//
// FEdModeFoliage
//

/** Constructor */
FEdModeFoliage::FEdModeFoliage() 
:	FEdMode()
,	bToolActive(false)
,	SelectionIFA(NULL)
,	bCanAltDrag(false)
{
	ID = FBuiltinEditorModes::EM_Foliage;
	Name = NSLOCTEXT("EditorModes", "FoliageMode", "Foliage");
	IconBrush = FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.FoliageMode", "LevelEditor.FoliageMode.Small");
	bVisible = true;
	PriorityOrder = 400;

	// Load resources and construct brush component
	UMaterial* BrushMaterial = NULL;
	UStaticMesh* StaticMesh = NULL;
	if (!IsRunningCommandlet())
	{
		BrushMaterial = LoadObject<UMaterial>(NULL, TEXT("/Engine/EditorLandscapeResources/FoliageBrushSphereMaterial.FoliageBrushSphereMaterial"), NULL, LOAD_None, NULL);
		StaticMesh = LoadObject<UStaticMesh>(NULL,TEXT("/Engine/EngineMeshes/Sphere.Sphere"), NULL, LOAD_None, NULL);
	}

	SphereBrushComponent = ConstructObject<UStaticMeshComponent>(UStaticMeshComponent::StaticClass());
	SphereBrushComponent->StaticMesh = StaticMesh;
	SphereBrushComponent->Materials.Add(BrushMaterial);
	SphereBrushComponent->SetAbsolute(true, true, true);

	bBrushTraceValid = false;
	BrushLocation = FVector::ZeroVector;
}


/** Destructor */
FEdModeFoliage::~FEdModeFoliage()
{
	// Save UI settings to config file
	UISettings.Save();
}


/** FGCObject interface */
void FEdModeFoliage::AddReferencedObjects( FReferenceCollector& Collector )
{
	// Call parent implementation
	FEdMode::AddReferencedObjects( Collector );

	Collector.AddReferencedObject(SphereBrushComponent);
}

/** FEdMode: Called when the mode is entered */
void FEdModeFoliage::Enter()
{
	FEdMode::Enter();

	// Clear any selection in case the instanced foliage actor is selected
	GEditor->SelectNone(false, true);

	// Load UI settings from config file
	UISettings.Load();

	// Force real-time viewports.  We'll back up the current viewport state so we can restore it when the
	// user exits this mode.
	const bool bWantRealTime = true;
	const bool bRememberCurrentState = true;
	ForceRealTimeViewports( bWantRealTime, bRememberCurrentState );

	// Reapply selection visualization on any foliage items
	if( UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected() )
	{
		SelectionIFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
		SelectionIFA->ApplySelectionToComponents(true);
	}
	else
	{
		SelectionIFA = NULL;
	}

	if (!Toolkit.IsValid())
	{
		// @todo: Remove this assumption when we make modes per level editor instead of global
		auto ToolkitHost = FModuleManager::LoadModuleChecked< FLevelEditorModule >( "LevelEditor" ).GetFirstLevelEditor();
		Toolkit = MakeShareable(new FFoliageEdModeToolkit);
		Toolkit->Init(ToolkitHost);
	}

	// Fixup any broken clusters
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
	for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(IFA->FoliageMeshes); MeshIt; ++MeshIt )
	{
		FFoliageMeshInfo& MeshInfo = MeshIt.Value();
		for( int32 ClusterIdx=0;ClusterIdx<MeshInfo.InstanceClusters.Num();ClusterIdx++ )
		{
			if( MeshInfo.InstanceClusters[ClusterIdx].ClusterComponent == NULL )
			{
				MeshInfo.ReallocateClusters(IFA, MeshIt.Key());
				break;
			}
		}
	}
}

/** FEdMode: Called when the mode is exited */
void FEdModeFoliage::Exit()
{
	FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
	Toolkit.Reset();

	// Remove the brush
	SphereBrushComponent->UnregisterComponent();

	// Restore real-time viewport state if we changed it
	const bool bWantRealTime = false;
	const bool bRememberCurrentState = false;
	ForceRealTimeViewports( bWantRealTime, bRememberCurrentState );

	// Clear the cache (safety, should be empty!)
	LandscapeLayerCaches.Empty();

	// Save UI settings to config file
	UISettings.Save();

	// Clear the placed level info
	FoliageMeshList.Empty();

	// Clear selection visualization on any foliage items
	if( SelectionIFA && (UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected()) )
	{
		SelectionIFA->ApplySelectionToComponents(false);
	}

	// Call base Exit method to ensure proper cleanup
	FEdMode::Exit();
}

void FEdModeFoliage::PostUndo()
{
	FEdMode::PostUndo();

	StaticCastSharedPtr<FFoliageEdModeToolkit>(Toolkit)->PostUndo();
}

/** When the user changes the active streaming level with the level browser */
void FEdModeFoliage::NotifyNewCurrentLevel()
{
	// Remove any selections in the old level and reapply for the new level
	if( UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected() )
	{
		if( SelectionIFA )
		{
			SelectionIFA->ApplySelectionToComponents(false);
		}
		SelectionIFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
		SelectionIFA->ApplySelectionToComponents(true);
	}
}

/** When the user changes the current tool in the UI */
void FEdModeFoliage::NotifyToolChanged()
{
	if( UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected() )
	{
		if( SelectionIFA == NULL )
		{
			SelectionIFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
			SelectionIFA->ApplySelectionToComponents(true);
		}
	}
	else
	{
		if( SelectionIFA )
		{
			SelectionIFA->ApplySelectionToComponents(false);
		}
		SelectionIFA = NULL;
	}
}

void FEdModeFoliage::NotifyMapRebuild(uint32 MapChangeFlags) const
{
	if(MapChangeEventFlags::MapRebuild & MapChangeFlags)
	{
		AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
		if( IFA )
		{
			IFA->MapRebuild();
		}
	}
}

bool FEdModeFoliage::DisallowMouseDeltaTracking() const
{
	// We never want to use the mouse delta tracker while painting
	return bToolActive;
}

/** FEdMode: Called once per frame */
void FEdModeFoliage::Tick(FLevelEditorViewportClient* ViewportClient,float DeltaTime)
{
	if( IsCtrlDown(ViewportClient->Viewport) && bToolActive )
	{
		ApplyBrush(ViewportClient);
	}

	FEdMode::Tick(ViewportClient,DeltaTime);

	if( UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected() )
	{
		// Update pivot
		AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
		FEditorModeTools& Tools = GEditorModeTools();
		Tools.PivotLocation = Tools.SnappedLocation = IFA->GetSelectionLocation();
	}
	
	// Update the position and size of the brush component
	if( bBrushTraceValid && (UISettings.GetPaintToolSelected() || UISettings.GetReapplyToolSelected() || UISettings.GetLassoSelectToolSelected()) )
	{
		// Scale adjustment is due to default sphere SM size.
		FTransform BrushTransform = FTransform( FQuat::Identity, BrushLocation, FVector(UISettings.GetRadius() * 0.00625f) );
		SphereBrushComponent->SetRelativeTransform(BrushTransform);

		if( !SphereBrushComponent->IsRegistered() )
		{
			SphereBrushComponent->RegisterComponentWithWorld(ViewportClient->GetWorld());
		}
	}
	else
	{
		if( SphereBrushComponent->IsRegistered() )
		{
			SphereBrushComponent->UnregisterComponent();
		}
	}
}

/** Trace under the mouse cursor and update brush position */
void FEdModeFoliage::FoliageBrushTrace( FLevelEditorViewportClient* ViewportClient, int32 MouseX, int32 MouseY )
{
	bBrushTraceValid = false;
	if( UISettings.GetPaintToolSelected() || UISettings.GetReapplyToolSelected() || UISettings.GetLassoSelectToolSelected() )
	{
		// Compute a world space ray from the screen space mouse coordinates
		FSceneViewFamilyContext ViewFamily( FSceneViewFamily::ConstructionValues(
			ViewportClient->Viewport, 
			ViewportClient->GetScene(),
			ViewportClient->EngineShowFlags )
			.SetRealtimeUpdate( ViewportClient->IsRealtime() ));
		FSceneView* View = ViewportClient->CalcSceneView( &ViewFamily );
		FViewportCursorLocation MouseViewportRay( View, ViewportClient, MouseX, MouseY );

		FVector Start = MouseViewportRay.GetOrigin();
		BrushTraceDirection = MouseViewportRay.GetDirection();
		FVector End = Start + WORLD_MAX * BrushTraceDirection;

		FHitResult Hit;
		UWorld* World = ViewportClient->GetWorld();
		if ( World->LineTraceSingle(Hit, Start, End, FCollisionQueryParams(true), FCollisionObjectQueryParams(ECC_WorldStatic)) )
		{
			// Check filters
			UPrimitiveComponent* PrimComp = Hit.Component.Get();
			if( (PrimComp &&		 
				(PrimComp->GetOutermost() != World->GetCurrentLevel()->GetOutermost() ||
				(!UISettings.bFilterLandscape && PrimComp->IsA(ULandscapeHeightfieldCollisionComponent::StaticClass())) ||
				(!UISettings.bFilterStaticMesh && PrimComp->IsA(UStaticMeshComponent::StaticClass())) ||
				(!UISettings.bFilterBSP && PrimComp->IsA(UModelComponent::StaticClass()))
				)))
			{
				bBrushTraceValid = false;
			}
			else
			{
				// Adjust the sphere brush
				BrushLocation = Hit.Location;
				bBrushTraceValid = true;
			}
		}
	}
}

/**
 * Called when the mouse is moved over the viewport
 *
 * @param	InViewportClient	Level editor viewport client that captured the mouse input
 * @param	InViewport			Viewport that captured the mouse input
 * @param	InMouseX			New mouse cursor X coordinate
 * @param	InMouseY			New mouse cursor Y coordinate
 *
 * @return	true if input was handled
 */
bool FEdModeFoliage::MouseMove( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY )
{
	FoliageBrushTrace(ViewportClient, MouseX, MouseY );
	return false;
}

/**
 * Called when the mouse is moved while a window input capture is in effect
 *
 * @param	InViewportClient	Level editor viewport client that captured the mouse input
 * @param	InViewport			Viewport that captured the mouse input
 * @param	InMouseX			New mouse cursor X coordinate
 * @param	InMouseY			New mouse cursor Y coordinate
 *
 * @return	true if input was handled
 */
bool FEdModeFoliage::CapturedMouseMove( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY )
{
	FoliageBrushTrace(ViewportClient, MouseX, MouseY );
	return false;
}

void FEdModeFoliage::GetRandomVectorInBrush( FVector& OutStart, FVector& OutEnd )
{
	// Find Rx and Ry inside the unit circle
	float Ru, Rv;
	do 
	{
		Ru = 2.f * FMath::FRand() - 1.f;
		Rv = 2.f * FMath::FRand() - 1.f;
	} while ( FMath::Square(Ru) + FMath::Square(Rv) > 1.f );
	
	// find random point in circle thru brush location parallel to screen surface
	FVector U, V;
	BrushTraceDirection.FindBestAxisVectors(U,V);
	FVector Point = Ru * U + Rv * V;

	// find distance to surface of sphere brush from this point
	float Rw = FMath::Sqrt( 1.f - (FMath::Square(Ru) + FMath::Square(Rv)) );

	OutStart = BrushLocation + UISettings.GetRadius() * (Ru * U + Rv * V - Rw * BrushTraceDirection);
	OutEnd   = BrushLocation + UISettings.GetRadius() * (Ru * U + Rv * V + Rw * BrushTraceDirection);
}


// Number of buckets for layer weight histogram distribution.
#define NUM_INSTANCE_BUCKETS 10

bool CheckCollisionWithWorld(UInstancedFoliageSettings* MeshSettings, const FFoliageInstance& Inst, const FVector& HitNormal, const FVector& HitLocation,UWorld* InWorld)
{
	FMatrix InstTransform = Inst.GetInstanceTransform();
	FVector LocalHit = InstTransform.InverseTransformPosition(HitLocation);

	if ( MeshSettings->CollisionWithWorld )
	{
		// Check for overhanging ledge
		{
			FVector LocalSamplePos[4] = {	FVector(MeshSettings->LowBoundOriginRadius.Z, 0, 0), 
				FVector(-MeshSettings->LowBoundOriginRadius.Z, 0, 0),
				FVector(0, MeshSettings->LowBoundOriginRadius.Z, 0),
				FVector(0, -MeshSettings->LowBoundOriginRadius.Z, 0)
			};

			for (uint32 i = 0; i < 4; ++i)
			{
				FHitResult Hit;
				FVector SamplePos = InstTransform.TransformPosition(FVector(MeshSettings->LowBoundOriginRadius.X, MeshSettings->LowBoundOriginRadius.Y, 2.f) + LocalSamplePos[i]);
				float WorldRadius = (MeshSettings->LowBoundOriginRadius.Z + 2.f)*FMath::Max(Inst.DrawScale3D.X, Inst.DrawScale3D.Y);
				FVector NormalVector = MeshSettings->AlignToNormal ? HitNormal : FVector(0, 0, 1);
				if ( InWorld->LineTraceSingle(Hit, SamplePos, SamplePos - NormalVector*WorldRadius, FCollisionQueryParams(true), FCollisionObjectQueryParams(ECC_WorldStatic)) )
				{
					if (LocalHit.Z - Inst.ZOffset < MeshSettings->LowBoundOriginRadius.Z)
					{
						continue;
					}
				}
				return false;
			}
		}

		// Check collision with Bounding Box
		{
			FBox MeshBox = MeshSettings->MeshBounds.GetBox();
			MeshBox.Min.Z = FMath::Min(MeshBox.Max.Z, LocalHit.Z + MeshSettings->MeshBounds.BoxExtent.Z * 0.05f);
			FBoxSphereBounds ShrinkBound(MeshBox);
			FBoxSphereBounds WorldBound = ShrinkBound.TransformBy(InstTransform);
			//::DrawDebugBox(InWorld, WorldBound.Origin, WorldBound.BoxExtent, FColor(255, 0, 0), true, 10.f);
			static FName NAME_FoliageCollisionWithWorld = FName(TEXT("FoliageCollisionWithWorld"));
			if ( InWorld->OverlapTest(WorldBound.Origin, FQuat(Inst.Rotation), FCollisionShape::MakeBox(ShrinkBound.BoxExtent * Inst.DrawScale3D * MeshSettings->CollisionScale), FCollisionQueryParams(NAME_FoliageCollisionWithWorld, false), FCollisionObjectQueryParams(ECC_WorldStatic) ) )
			{
				return false;
			}
		}
	}

	return true;
}

// Struct to hold potential instances we've randomly sampled
struct FPotentialInstance
{
	FVector HitLocation;
	FVector HitNormal;
	UPrimitiveComponent* HitComponent;
	float HitWeight;
	
	FPotentialInstance(FVector InHitLocation, FVector InHitNormal, UPrimitiveComponent* InHitComponent, float InHitWeight)
	:	HitLocation(InHitLocation)
	,	HitNormal(InHitNormal)
	,	HitComponent(InHitComponent)
	,	HitWeight(InHitWeight)
	{}

	bool PlaceInstance(UInstancedFoliageSettings* MeshSettings, FFoliageInstance& Inst,UWorld* InWorld)
	{
		if( MeshSettings->UniformScale )
		{
			float Scale = MeshSettings->ScaleMinX + FMath::FRand() * (MeshSettings->ScaleMaxX - MeshSettings->ScaleMinX);
			Inst.DrawScale3D = FVector(Scale,Scale,Scale);
		}
		else
		{
			float LockRand = FMath::FRand();
			Inst.DrawScale3D.X = MeshSettings->ScaleMinX + (MeshSettings->LockScaleX ? LockRand : FMath::FRand()) * (MeshSettings->ScaleMaxX - MeshSettings->ScaleMinX);
			Inst.DrawScale3D.Y = MeshSettings->ScaleMinY + (MeshSettings->LockScaleY ? LockRand : FMath::FRand()) * (MeshSettings->ScaleMaxY - MeshSettings->ScaleMinY);
			Inst.DrawScale3D.Z = MeshSettings->ScaleMinZ + (MeshSettings->LockScaleZ ? LockRand : FMath::FRand()) * (MeshSettings->ScaleMaxZ - MeshSettings->ScaleMinZ);
		}

		Inst.ZOffset = MeshSettings->ZOffsetMin + FMath::FRand() * (MeshSettings->ZOffsetMax - MeshSettings->ZOffsetMin);

		Inst.Location = HitLocation;

		// Random yaw and optional random pitch up to the maximum
		Inst.Rotation = FRotator(FMath::FRand() * MeshSettings->RandomPitchAngle,0.f,0.f);

		if( MeshSettings->RandomYaw )
		{
			Inst.Rotation.Yaw = FMath::FRand() * 360.f;
		}
		else
		{
			Inst.Flags |= FOLIAGE_NoRandomYaw;
		}

		if( MeshSettings->AlignToNormal )
		{
			Inst.AlignToNormal( HitNormal, MeshSettings->AlignMaxAngle );
		}

		// Apply the Z offset in local space
		if( FMath::Abs(Inst.ZOffset) > KINDA_SMALL_NUMBER )
		{
			Inst.Location = Inst.GetInstanceTransform().TransformPosition(FVector(0,0,Inst.ZOffset));
		}

		Inst.Base = HitComponent;

		return CheckCollisionWithWorld(MeshSettings, Inst, HitNormal, HitLocation,InWorld);
	}
};

static bool CheckLocationForPotentialInstance(FFoliageMeshInfo& MeshInfo, const UInstancedFoliageSettings* MeshSettings, float DensityCheckRadius, const FVector& Location, const FVector& Normal, TArray<FVector>& PotentialInstanceLocations, FFoliageInstanceHash& PotentialInstanceHash )
{
	// Check slope
	if( (MeshSettings->GroundSlope > 0.f && Normal.Z <= FMath::Cos( PI * MeshSettings->GroundSlope / 180.f)-SMALL_NUMBER) ||
		(MeshSettings->GroundSlope < 0.f && Normal.Z >= FMath::Cos(-PI * MeshSettings->GroundSlope / 180.f)+SMALL_NUMBER) )
	{
		return false;
	}

	// Check height range
	if( Location.Z < MeshSettings->HeightMin || Location.Z > MeshSettings->HeightMax )
	{
		return false;
	}

	// Check existing instances. Use the Density radius rather than the minimum radius
	if( MeshInfo.CheckForOverlappingSphere(FSphere(Location, DensityCheckRadius)) )
	{
		return false;
	}

	// Check if we're too close to any other instances
	if( MeshSettings->Radius > 0.f )
	{
		// Check with other potential instances we're about to add.
		bool bFoundOverlapping = false;
		float RadiusSquared = FMath::Square(DensityCheckRadius/*MeshSettings->Radius*/);

		TSet<int32> TempInstances;
		PotentialInstanceHash.GetInstancesOverlappingBox(FBox::BuildAABB(Location, FVector(MeshSettings->Radius,MeshSettings->Radius,MeshSettings->Radius)), TempInstances );
		for( TSet<int32>::TConstIterator It(TempInstances); It; ++It )
		{
			if( (PotentialInstanceLocations[*It] - Location).SizeSquared() < RadiusSquared )
			{
				bFoundOverlapping = true;
				break;
			}
		}
		if( bFoundOverlapping )
		{
			return false;
		}
	}

	int32 PotentialIdx = PotentialInstanceLocations.Add(Location);
	PotentialInstanceHash.InsertInstance( Location, PotentialIdx );
	return true;
}

static bool CheckVertexColor(const UInstancedFoliageSettings* MeshSettings, const FColor& VertexColor)
{
	uint8 ColorChannel;
	switch( MeshSettings->VertexColorMask )
	{
	case FOLIAGEVERTEXCOLORMASK_Red:
		ColorChannel = VertexColor.R;
		break;
	case FOLIAGEVERTEXCOLORMASK_Green:
		ColorChannel = VertexColor.G;
		break;
	case FOLIAGEVERTEXCOLORMASK_Blue:
		ColorChannel = VertexColor.B;
		break;
	case FOLIAGEVERTEXCOLORMASK_Alpha:
		ColorChannel = VertexColor.A;
		break;
	default:
		return true;
	}

	if( MeshSettings->VertexColorMaskInvert )
	{
		if( ColorChannel > FMath::Round(MeshSettings->VertexColorMaskThreshold * 255.f) )
		{
			return false;
		}
	}
	else
	{
		if( ColorChannel < FMath::Round(MeshSettings->VertexColorMaskThreshold * 255.f) )
		{
			return false;
		}
	}

	return true;
}


/** Add instances inside the brush to match DesiredInstanceCount */
void FEdModeFoliage::AddInstancesForBrush( UWorld* InWorld, AInstancedFoliageActor* IFA, UStaticMesh* StaticMesh, FFoliageMeshInfo& MeshInfo, int32 DesiredInstanceCount, TArray<int32>& ExistingInstances, float Pressure )
{
	checkf( InWorld == IFA->GetWorld() , TEXT("Warning:World does not match Foliage world") );
	UInstancedFoliageSettings* MeshSettings = MeshInfo.Settings;
	if( DesiredInstanceCount > ExistingInstances.Num() )
	{
		int32 ExistingInstanceBuckets[NUM_INSTANCE_BUCKETS];
		FMemory::Memzero(ExistingInstanceBuckets, sizeof(ExistingInstanceBuckets));

		// Cache store mapping between component and weight data
		TMap<ULandscapeComponent*, TArray<uint8> >* LandscapeLayerCache = NULL;

		FName LandscapeLayerName = MeshSettings->LandscapeLayer;
		if( LandscapeLayerName != NAME_None )
		{
			LandscapeLayerCache = &LandscapeLayerCaches.FindOrAdd(LandscapeLayerName);

			// Find the landscape weights of existing ExistingInstances
			for( int32 Idx=0;Idx<ExistingInstances.Num();Idx++ )
			{
				FFoliageInstance& Instance = MeshInfo.Instances[ExistingInstances[Idx]];
				ULandscapeHeightfieldCollisionComponent* HitLandscapeCollision = Cast<ULandscapeHeightfieldCollisionComponent>(Instance.Base);
				if( HitLandscapeCollision )
				{
					ULandscapeComponent* HitLandscape = HitLandscapeCollision->RenderComponent.Get();
					if( HitLandscape )
					{
						TArray<uint8>* LayerCache = &LandscapeLayerCache->FindOrAdd(HitLandscape);
						// TODO: FName to LayerInfo?
						float HitWeight = HitLandscape->GetLayerWeightAtLocation( Instance.Location, HitLandscape->GetLandscapeInfo()->GetLayerInfoByName(LandscapeLayerName), LayerCache );

						// Add count to bucket.
						ExistingInstanceBuckets[FMath::Round(HitWeight * (float)(NUM_INSTANCE_BUCKETS-1))]++;
					}
				}
			}
		}
		else
		{
			// When not tied to a layer, put all the ExistingInstances in the last bucket.
			ExistingInstanceBuckets[NUM_INSTANCE_BUCKETS-1] = ExistingInstances.Num();
		}

		// We calculate a set of potential ExistingInstances for the brush area.
		TArray<FPotentialInstance> PotentialInstanceBuckets[NUM_INSTANCE_BUCKETS];
		FMemory::Memzero(PotentialInstanceBuckets, sizeof(PotentialInstanceBuckets));

		// Quick lookup of potential instance locations, used for overlapping check.
		TArray<FVector> PotentialInstanceLocations;
		FFoliageInstanceHash PotentialInstanceHash(7);	// use 128x128 cell size, as the brush radius is typically small.
		PotentialInstanceLocations.Empty(DesiredInstanceCount);

		// Radius where we expect to have a single instance, given the density rules
		const float DensityCheckRadius = FMath::Max<float>( FMath::Sqrt( (1000.f*1000.f) / (PI * MeshSettings->Density) ), MeshSettings->Radius );

		for( int32 DesiredIdx=0;DesiredIdx<DesiredInstanceCount;DesiredIdx++ )
		{
			FVector Start, End;

			GetRandomVectorInBrush(Start, End);
			
			FHitResult Hit;
			static FName NAME_AddInstancesForBrush = FName(TEXT("AddInstancesForBrush"));
			FCollisionQueryParams TraceParams(NAME_AddInstancesForBrush, true);
			if( MeshSettings->VertexColorMask != FOLIAGEVERTEXCOLORMASK_Disabled )
			{
				TraceParams.bReturnFaceIndex = true;
			}
			if( InWorld->LineTraceSingle(Hit, Start, End, TraceParams, FCollisionObjectQueryParams(ECC_WorldStatic))) 
			{
				// Check filters
				UPrimitiveComponent* PrimComp = Hit.Component.Get();
				if( (PrimComp &&		 
					(PrimComp->GetOutermost() != InWorld->GetCurrentLevel()->GetOutermost() ||
					(!UISettings.bFilterLandscape && PrimComp->IsA(ULandscapeHeightfieldCollisionComponent::StaticClass())) ||
					(!UISettings.bFilterStaticMesh && PrimComp->IsA(UStaticMeshComponent::StaticClass())) ||
					(!UISettings.bFilterBSP && PrimComp->IsA(UModelComponent::StaticClass()))
					)))
				{
					continue;
				}

				if( !CheckLocationForPotentialInstance( MeshInfo, MeshSettings, DensityCheckRadius, Hit.Location, Hit.Normal, PotentialInstanceLocations, PotentialInstanceHash ) )
				{
					continue;
				}	

				// Check vertex color mask
				if( MeshSettings->VertexColorMask != FOLIAGEVERTEXCOLORMASK_Disabled && Hit.FaceIndex != INDEX_NONE )
				{
					UStaticMeshComponent* HitStaticMeshComponent = Cast<UStaticMeshComponent>(Hit.Component.Get());
					if( HitStaticMeshComponent )
					{
						FColor VertexColor;
						if( GetStaticMeshVertexColorForHit( HitStaticMeshComponent, Hit.FaceIndex, Hit.Location, VertexColor ) )
						{
							if( !CheckVertexColor(MeshSettings, VertexColor) )
							{
								continue;
							}
						}
					}
				}

				// Check landscape layer
				float HitWeight = 1.f;
				if( LandscapeLayerName != NAME_None )
				{
					ULandscapeHeightfieldCollisionComponent* HitLandscapeCollision = Cast<ULandscapeHeightfieldCollisionComponent>(Hit.Component.Get());
					if( HitLandscapeCollision )
					{
						ULandscapeComponent* HitLandscape = HitLandscapeCollision->RenderComponent.Get();
						if( HitLandscape )
						{
							TArray<uint8>* LayerCache = &LandscapeLayerCache->FindOrAdd(HitLandscape);
							// TODO: FName to LayerInfo?
							HitWeight = HitLandscape->GetLayerWeightAtLocation( Hit.Location, HitLandscape->GetLandscapeInfo()->GetLayerInfoByName(LandscapeLayerName), LayerCache );

							// Reject instance randomly in proportion to weight
							if( HitWeight <= FMath::FRand() )
							{
								continue;
							}
						}					
					}
				}

				new(PotentialInstanceBuckets[FMath::Round(HitWeight * (float)(NUM_INSTANCE_BUCKETS-1))]) FPotentialInstance( Hit.Location, Hit.Normal, Hit.Component.Get(), HitWeight );
			}
		}

		for( int32 BucketIdx = 0; BucketIdx < NUM_INSTANCE_BUCKETS; BucketIdx++ )
		{
			TArray<FPotentialInstance>& PotentialInstances = PotentialInstanceBuckets[BucketIdx];
			float BucketFraction = (float)(BucketIdx+1) / (float)NUM_INSTANCE_BUCKETS;

			// We use the number that actually succeeded in placement (due to parameters) as the target
			// for the number that should be in the brush region.
			int32 AdditionalInstances = FMath::Clamp<int32>( FMath::Round( BucketFraction * (float)(PotentialInstances.Num() - ExistingInstanceBuckets[BucketIdx]) * Pressure), 0, PotentialInstances.Num() );
			for( int32 Idx=0;Idx<AdditionalInstances;Idx++ )
			{
				FFoliageInstance Inst;
				if (PotentialInstances[Idx].PlaceInstance(MeshSettings, Inst, InWorld ))
				{
					MeshInfo.AddInstance( IFA, StaticMesh, Inst );
				}
			}
		}
	}
}

/** Remove instances inside the brush to match DesiredInstanceCount */
void FEdModeFoliage::RemoveInstancesForBrush( AInstancedFoliageActor* IFA, FFoliageMeshInfo& MeshInfo, int32 DesiredInstanceCount, TArray<int32>& ExistingInstances, float Pressure )
{
	int32 InstancesToRemove = FMath::Round( (float)(ExistingInstances.Num() - DesiredInstanceCount) * Pressure);
	int32 InstancesToKeep = ExistingInstances.Num() - InstancesToRemove;
	if( InstancesToKeep > 0 )
	{						
		// Remove InstancesToKeep random ExistingInstances from the array to leave those ExistingInstances behind, and delete all the rest
		for( int32 i=0;i<InstancesToKeep;i++ )
		{
			ExistingInstances.RemoveSwap(FMath::Rand() % ExistingInstances.Num());
		}
	}

	if( !UISettings.bFilterLandscape || !UISettings.bFilterStaticMesh || !UISettings.bFilterBSP )
	{
		// Filter ExistingInstances
		for( int32 Idx=0;Idx<ExistingInstances.Num();Idx++ )
		{
			UActorComponent* Base = MeshInfo.Instances[ExistingInstances[Idx]].Base;

			// Check if instance is candidate for removal based on filter settings
			if( (Base && (
				(!UISettings.bFilterLandscape && Base->IsA(ULandscapeHeightfieldCollisionComponent::StaticClass())) ||
				(!UISettings.bFilterStaticMesh && Base->IsA(UStaticMeshComponent::StaticClass())) ||
				(!UISettings.bFilterBSP && Base->IsA(UModelComponent::StaticClass()))
				)))
			{
				// Instance should not be removed, so remove it from the removal list.
				ExistingInstances.RemoveSwap(Idx);
				Idx--;
			}
		}
	}

	// Remove ExistingInstances to reduce it to desired count
	if( ExistingInstances.Num() > 0 )
	{
		MeshInfo.RemoveInstances( IFA, ExistingInstances );
	}
}

/** Reapply instance settings to exiting instances */
void FEdModeFoliage::ReapplyInstancesForBrush( UWorld* InWorld, AInstancedFoliageActor* IFA, FFoliageMeshInfo& MeshInfo, TArray<int32>& ExistingInstances )
{
	checkf( InWorld == IFA->GetWorld() , TEXT("Warning:World does not match Foliage world") );
	UInstancedFoliageSettings* MeshSettings = MeshInfo.Settings;
	bool bUpdated=false;

	TArray<int32> UpdatedInstances;
	TSet<int32> InstancesToDelete;

	// Setup cache if we're reapplying landscape layer weights
	FName LandscapeLayerName = MeshSettings->LandscapeLayer;
	TMap<ULandscapeComponent*, TArray<uint8> >* LandscapeLayerCache = NULL;
	if( MeshSettings->ReapplyLandscapeLayer && LandscapeLayerName != NAME_None )
	{
		LandscapeLayerCache = &LandscapeLayerCaches.FindOrAdd(LandscapeLayerName);
	}

	IFA->Modify();

	for( int32 Idx=0;Idx<ExistingInstances.Num();Idx++ )
	{
		int32 InstanceIndex = ExistingInstances[Idx];
		FFoliageInstance& Instance = MeshInfo.Instances[InstanceIndex];

		if( (Instance.Flags & FOLIAGE_Readjusted) == 0 )
		{
			// record that we've made changes to this instance already, so we don't touch it again.
			Instance.Flags |= FOLIAGE_Readjusted;

			bool bReapplyLocation = false;

			// remove any Z offset first, so the offset is reapplied to any new 
			if( FMath::Abs(Instance.ZOffset) > KINDA_SMALL_NUMBER )
			{
				MeshInfo.InstanceHash->RemoveInstance(Instance.Location, InstanceIndex);
				Instance.Location = Instance.GetInstanceTransform().TransformPosition(FVector(0,0,-Instance.ZOffset));
				bReapplyLocation = true;
			}


			// Defer normal reapplication 
			bool bReapplyNormal = false;

			// Reapply normal alignment
			if( MeshSettings->ReapplyAlignToNormal )
			{
				if( MeshSettings->AlignToNormal )
				{
					if( (Instance.Flags & FOLIAGE_AlignToNormal) == 0 )
					{
						bReapplyNormal = true;
						bUpdated = true;
					}
				}
				else
				{
					if( Instance.Flags & FOLIAGE_AlignToNormal )
					{
						Instance.Rotation = Instance.PreAlignRotation;
						Instance.Flags &= (~FOLIAGE_AlignToNormal);
						bUpdated = true;
					}
				}
			}

			// Reapply random yaw
			if( MeshSettings->ReapplyRandomYaw )
			{
				if( MeshSettings->RandomYaw )
				{
					if( Instance.Flags & FOLIAGE_NoRandomYaw )
					{
						// See if we need to remove any normal alignment first
						if( !bReapplyNormal && (Instance.Flags & FOLIAGE_AlignToNormal) )
						{
							Instance.Rotation = Instance.PreAlignRotation;
							bReapplyNormal = true;
						}
						Instance.Rotation.Yaw = FMath::FRand() * 360.f;
						Instance.Flags &= (~FOLIAGE_NoRandomYaw);
						bUpdated = true;
					}
				}
				else
				{
					if( (Instance.Flags & FOLIAGE_NoRandomYaw)==0 )
					{
						// See if we need to remove any normal alignment first
						if( !bReapplyNormal && (Instance.Flags & FOLIAGE_AlignToNormal) )
						{
							Instance.Rotation = Instance.PreAlignRotation;
							bReapplyNormal = true;
						}
						Instance.Rotation.Yaw = 0.f;
						Instance.Flags |= FOLIAGE_NoRandomYaw;
						bUpdated = true;
					}
				}
			}

			// Reapply random pitch angle
			if( MeshSettings->ReapplyRandomPitchAngle )
			{
				// See if we need to remove any normal alignment first
				if( !bReapplyNormal && (Instance.Flags & FOLIAGE_AlignToNormal) )
				{
					Instance.Rotation = Instance.PreAlignRotation;
					bReapplyNormal = true;
				}

				Instance.Rotation.Pitch = FMath::FRand() * MeshSettings->RandomPitchAngle;
				Instance.Flags |= FOLIAGE_NoRandomYaw;

				bUpdated = true;
			}

			// Reapply scale
			if( MeshSettings->UniformScale )
			{
				if( MeshSettings->ReapplyScaleX )
				{
					float Scale = MeshSettings->ScaleMinX + FMath::FRand() * (MeshSettings->ScaleMaxX - MeshSettings->ScaleMinX);
					Instance.DrawScale3D = FVector(Scale,Scale,Scale);
					bUpdated = true;
				}
			}
			else
			{
				float LockRand;
				// If we're doing axis scale locking, get an existing scale for a locked axis that we're not changing, for use as the locked scale value.
				if( MeshSettings->LockScaleX && !MeshSettings->ReapplyScaleX && (MeshSettings->ScaleMaxX - MeshSettings->ScaleMinX) > KINDA_SMALL_NUMBER )
				{
					LockRand = (Instance.DrawScale3D.X - MeshSettings->ScaleMinX) / (MeshSettings->ScaleMaxX - MeshSettings->ScaleMinX);
				}
				else
				if( MeshSettings->LockScaleY && !MeshSettings->ReapplyScaleY && (MeshSettings->ScaleMaxY - MeshSettings->ScaleMinY) > KINDA_SMALL_NUMBER )
				{
					LockRand = (Instance.DrawScale3D.Y - MeshSettings->ScaleMinY) / (MeshSettings->ScaleMaxY - MeshSettings->ScaleMinY);
				}
				else
				if( MeshSettings->LockScaleZ && !MeshSettings->ReapplyScaleZ && (MeshSettings->ScaleMaxZ - MeshSettings->ScaleMinZ) > KINDA_SMALL_NUMBER )
				{
					LockRand = (Instance.DrawScale3D.Z - MeshSettings->ScaleMinZ) / (MeshSettings->ScaleMaxZ - MeshSettings->ScaleMinZ);
				}
				else
				{
					LockRand = FMath::FRand();
				}

				if( MeshSettings->ReapplyScaleX )
				{
					Instance.DrawScale3D.X = MeshSettings->ScaleMinX + (MeshSettings->LockScaleX ? LockRand : FMath::FRand()) * (MeshSettings->ScaleMaxX - MeshSettings->ScaleMinX);
					bUpdated = true;
				}

				if( MeshSettings->ReapplyScaleY )
				{
					Instance.DrawScale3D.Y = MeshSettings->ScaleMinY + (MeshSettings->LockScaleY ? LockRand : FMath::FRand()) * (MeshSettings->ScaleMaxY - MeshSettings->ScaleMinY);
					bUpdated = true;
				}

				if( MeshSettings->ReapplyScaleZ )
				{
					Instance.DrawScale3D.Z = MeshSettings->ScaleMinZ + (MeshSettings->LockScaleZ ? LockRand : FMath::FRand()) * (MeshSettings->ScaleMaxZ - MeshSettings->ScaleMinZ);
					bUpdated = true;
				}
			}

			if( MeshSettings->ReapplyZOffset )
			{
				Instance.ZOffset = MeshSettings->ZOffsetMin + FMath::FRand() * (MeshSettings->ZOffsetMax - MeshSettings->ZOffsetMin);
				bUpdated = true;
			}

			// Find a ground normal for either normal or ground slope check.
			if( bReapplyNormal || MeshSettings->ReapplyGroundSlope || MeshSettings->ReapplyVertexColorMask || (MeshSettings->ReapplyLandscapeLayer && LandscapeLayerName != NAME_None) )
			{
				FHitResult Hit;
				static const FName NAME_ReapplyInstancesForBrush = TEXT("ReapplyInstancesForBrush");

				// trace along the mesh's Z axis.
				FVector ZAxis = Instance.Rotation.Quaternion().GetAxisZ();
				FVector Start = Instance.Location + 16.f * ZAxis;
				FVector End   = Instance.Location - 16.f * ZAxis;
				FCollisionQueryParams TraceParams(NAME_ReapplyInstancesForBrush, true);
				if( MeshSettings->ReapplyVertexColorMask && MeshSettings->VertexColorMask != FOLIAGEVERTEXCOLORMASK_Disabled )
				{
					TraceParams.bReturnFaceIndex = true;
				}
				if( InWorld->LineTraceSingle(Hit, Start, End, TraceParams, FCollisionObjectQueryParams(ECC_WorldStatic))) 
				{
					// Reapply the normal
					if( bReapplyNormal )
					{
						Instance.PreAlignRotation = Instance.Rotation;
						Instance.AlignToNormal( Hit.Normal, MeshSettings->AlignMaxAngle );
					}

					// Cull instances that don't meet the ground slope check.
					if( MeshSettings->ReapplyGroundSlope )
					{
						if( (MeshSettings->GroundSlope > 0.f && Hit.Normal.Z <= FMath::Cos (PI * MeshSettings->GroundSlope / 180.f)-SMALL_NUMBER) ||
							(MeshSettings->GroundSlope < 0.f && Hit.Normal.Z >= FMath::Cos(-PI * MeshSettings->GroundSlope / 180.f)+SMALL_NUMBER) )
						{
							InstancesToDelete.Add(InstanceIndex);
							continue;
						}
					}

					// Cull instances for the landscape layer
					if( MeshSettings->ReapplyLandscapeLayer )
					{
						float HitWeight = 1.f;
						ULandscapeHeightfieldCollisionComponent* HitLandscapeCollision = Cast<ULandscapeHeightfieldCollisionComponent>(Hit.Component.Get());
						if( HitLandscapeCollision )
						{
							ULandscapeComponent* HitLandscape = HitLandscapeCollision->RenderComponent.Get();
							if( HitLandscape )
							{
								TArray<uint8>* LayerCache = &LandscapeLayerCache->FindOrAdd(HitLandscape);
								// TODO: FName to LayerInfo?
								HitWeight = HitLandscape->GetLayerWeightAtLocation( Hit.Location, HitLandscape->GetLandscapeInfo()->GetLayerInfoByName(LandscapeLayerName), LayerCache );

								// Reject instance randomly in proportion to weight
								if( HitWeight <= FMath::FRand() )
								{
									InstancesToDelete.Add(InstanceIndex);
									continue;
								}
							}					
						}
					}

					// Reapply vertex color mask
					if( MeshSettings->ReapplyVertexColorMask )
					{
						if( MeshSettings->VertexColorMask != FOLIAGEVERTEXCOLORMASK_Disabled && Hit.FaceIndex != INDEX_NONE )
						{
							UStaticMeshComponent* HitStaticMeshComponent = Cast<UStaticMeshComponent>(Hit.Component.Get());
							if( HitStaticMeshComponent )
							{
								FColor VertexColor;
								if( GetStaticMeshVertexColorForHit( HitStaticMeshComponent, Hit.FaceIndex, Hit.Location, VertexColor ) )
								{
									if( !CheckVertexColor(MeshSettings, VertexColor) )
									{
										InstancesToDelete.Add(InstanceIndex);
										continue;
									}
								}
							}
						}
					}
				}
			}

			// Cull instances that don't meet the height range
			if( MeshSettings->ReapplyHeight )
			{
				if( Instance.Location.Z < MeshSettings->HeightMin || Instance.Location.Z > MeshSettings->HeightMax )
				{
					InstancesToDelete.Add(InstanceIndex);
					continue;
				}
			}

			if( bUpdated && FMath::Abs(Instance.ZOffset) > KINDA_SMALL_NUMBER )
			{
				// Reapply the Z offset in new local space
				Instance.Location = Instance.GetInstanceTransform().TransformPosition(FVector(0,0,Instance.ZOffset));
			}

			// Re-add to the hash
			if( bReapplyLocation )
			{
				MeshInfo.InstanceHash->InsertInstance(Instance.Location, InstanceIndex);
			}

			// Cull overlapping based on radius
			if( MeshSettings->ReapplyRadius && MeshSettings->Radius > 0.f )
			{
				if( MeshInfo.CheckForOverlappingInstanceExcluding(InstanceIndex, MeshSettings->Radius, InstancesToDelete) )
				{
					InstancesToDelete.Add(InstanceIndex);
					continue;
				}
			}

			// Remove mesh collide with world
			if( MeshSettings->ReapplyCollisionWithWorld )
			{
				FHitResult Hit;
				static const FName NAME_ReapplyInstancesForBrush = TEXT("ReapplyCollisionWithWorld");
				FVector Start = Instance.Location + FVector(0.f,0.f,16.f);
				FVector End   = Instance.Location - FVector(0.f,0.f,16.f);
				if( InWorld->LineTraceSingle(Hit, Start, End, FCollisionQueryParams(NAME_ReapplyInstancesForBrush, true), FCollisionObjectQueryParams(ECC_WorldStatic))) 
				{
					if (!CheckCollisionWithWorld(MeshSettings, Instance, Hit.Normal, Hit.Location, InWorld))
					{
						InstancesToDelete.Add(InstanceIndex);
						continue;
					}
				}
				else
				{
					InstancesToDelete.Add(InstanceIndex);
				}
			}

			if( bUpdated )
			{
				UpdatedInstances.Add(InstanceIndex);
			}		
		}
	}

	if( UpdatedInstances.Num() > 0 )
	{
		MeshInfo.PostUpdateInstances(IFA, UpdatedInstances);
		IFA->RegisterAllComponents();
	}
	
	if( InstancesToDelete.Num() )
	{
		MeshInfo.RemoveInstances(IFA, InstancesToDelete.Array());
	}
}

void FEdModeFoliage::ApplyBrush( FLevelEditorViewportClient* ViewportClient )
{
	if( !bBrushTraceValid )
	{
		return;
	}

	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());

	float BrushArea = PI * FMath::Square(UISettings.GetRadius());

	// Tablet pressure
	float Pressure = ViewportClient->Viewport->IsPenActive() ? ViewportClient->Viewport->GetTabletPressure() : 1.f;

	// Cache a copy of the world pointer
	UWorld* World = ViewportClient->GetWorld();

	for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(IFA->FoliageMeshes); MeshIt; ++MeshIt )
	{
		FFoliageMeshInfo& MeshInfo = MeshIt.Value();
		UInstancedFoliageSettings* MeshSettings = MeshInfo.Settings;

		if( MeshSettings->IsSelected )
		{
			// Find the instances already in the area.
			TArray<int32> Instances;
			FSphere BrushSphere(BrushLocation,UISettings.GetRadius());
			MeshInfo.GetInstancesInsideSphere( BrushSphere, Instances );

			if( UISettings.GetLassoSelectToolSelected() )
			{
				// Shift unpaints
				MeshInfo.SelectInstances( IFA, !IsShiftDown(ViewportClient->Viewport), Instances);
			}
			else
			if( UISettings.GetReapplyToolSelected() )
			{
				if( MeshSettings->ReapplyDensity )
				{
					// Adjust instance density
					FMeshInfoSnapshot* SnapShot = InstanceSnapshot.Find(MeshIt.Key());
					if( SnapShot ) 
					{
						// Use snapshot to determine number of instances at the start of the brush stroke
						int32 NewInstanceCount = FMath::Round( (float)SnapShot->CountInstancesInsideSphere(BrushSphere) * MeshSettings->ReapplyDensityAmount );
						if( MeshSettings->ReapplyDensityAmount > 1.f && NewInstanceCount > Instances.Num() )
						{
							AddInstancesForBrush( World , IFA, MeshIt.Key(), MeshInfo, NewInstanceCount, Instances, Pressure );
						}
						else
						if( MeshSettings->ReapplyDensityAmount < 1.f && NewInstanceCount < Instances.Num() )			
						{
							RemoveInstancesForBrush( IFA, MeshInfo, NewInstanceCount, Instances, Pressure );
						}
					}
				}

				// Reapply any settings checked by the user
				ReapplyInstancesForBrush( World , IFA, MeshInfo, Instances );
			}
			else
			if( UISettings.GetPaintToolSelected() )
			{	
				// Shift unpaints
				if( IsShiftDown(ViewportClient->Viewport) )
				{
					int32 DesiredInstanceCount =  FMath::Round(BrushArea * MeshSettings->Density * UISettings.GetUnpaintDensity() / (1000.f*1000.f));

					if( DesiredInstanceCount < Instances.Num() )
					{
						RemoveInstancesForBrush( IFA, MeshInfo, DesiredInstanceCount, Instances, Pressure );
					}
				}
				else
				{
					// This is the total set of instances disregarding parameters like slope, height or layer.
					float DesiredInstanceCountFloat = BrushArea * MeshSettings->Density * UISettings.GetPaintDensity() / (1000.f*1000.f);

					// Allow a single instance with a random chance, if the brush is smaller than the density
					int32 DesiredInstanceCount = DesiredInstanceCountFloat > 1.f ? FMath::Round(DesiredInstanceCountFloat) : FMath::FRand() < DesiredInstanceCountFloat ? 1 : 0;

					AddInstancesForBrush( World, IFA, MeshIt.Key(), MeshInfo, DesiredInstanceCount, Instances, Pressure );
				}
			}
		}
	}
	if( UISettings.GetLassoSelectToolSelected() )
	{
		IFA->CheckSelection();
		FEditorModeTools& Tools = GEditorModeTools();
		Tools.PivotLocation = Tools.SnappedLocation = IFA->GetSelectionLocation();
		IFA->MarkComponentsRenderStateDirty();
	}
}

struct FFoliagePaintBucketTriangle
{
	FFoliagePaintBucketTriangle(const FTransform& InLocalToWorld, const FVector& InVertex0, const FVector& InVertex1, const FVector& InVertex2, const FColor InColor0, const FColor InColor1, const FColor InColor2)
	{
		Vertex = InLocalToWorld.TransformPosition(InVertex0);
		Vector1 = InLocalToWorld.TransformPosition(InVertex1) - Vertex;
		Vector2 = InLocalToWorld.TransformPosition(InVertex2) - Vertex;
		VertexColor[0] = InColor0;
		VertexColor[1] = InColor1;
		VertexColor[2] = InColor2;

		WorldNormal = InLocalToWorld.GetDeterminant() >= 0.f ? Vector2 ^ Vector1 : Vector1 ^ Vector2;
		float WorldNormalSize = WorldNormal.Size();
		Area = WorldNormalSize * 0.5f;
		if( WorldNormalSize > SMALL_NUMBER )
		{
			WorldNormal /= WorldNormalSize;
		}
	}

	void GetRandomPoint(FVector& OutPoint, FColor& OutBaryVertexColor)
	{
		// Sample parallelogram
		float x = FMath::FRand();
		float y = FMath::FRand();

		// Flip if we're outside the triangle
		if ( x + y > 1.f )
		{ 
			x = 1.f-x;
			y = 1.f-y;
		}

		OutBaryVertexColor = (1.f-x-y) * VertexColor[0] + x * VertexColor[1] + y * VertexColor[2];
		OutPoint = Vertex + x * Vector1 + y * Vector2;
	}

	FVector	Vertex;
	FVector Vector1;
	FVector Vector2;
	FVector WorldNormal;
	float Area;
	FColor VertexColor[3];
};

/** Apply paint bucket to actor */
void FEdModeFoliage::ApplyPaintBucket(AActor* Actor, bool bRemove)
{
	// Apply only to current level
	if( !Actor->GetLevel()->IsCurrentLevel() )
	{
		return;
	}

	if( bRemove )
	{
		// Remove all instances of the selected meshes
		AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());

		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);

		for( int32 ComponentIdx=0;ComponentIdx<Components.Num();ComponentIdx++ )
		{
			for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(IFA->FoliageMeshes); MeshIt; ++MeshIt )
			{
				FFoliageMeshInfo& MeshInfo = MeshIt.Value();
				UInstancedFoliageSettings* MeshSettings = MeshInfo.Settings;

				if( MeshSettings->IsSelected )
				{
					FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(Components[ComponentIdx]);
					if( ComponentHashInfo )
					{
						TArray<int32> InstancesToRemove = ComponentHashInfo->Instances.Array();
						MeshInfo.RemoveInstances( IFA, InstancesToRemove );
					}
				}
			}
		}
	}
	else
	{
		TMap<UPrimitiveComponent*, TArray<FFoliagePaintBucketTriangle> > ComponentPotentialTriangles;

		FTransform LocalToWorld = Actor->GetRootComponent()->ComponentToWorld;

		// Check all the components of the hit actor
		TArray<UStaticMeshComponent*> StaticMeshComponents;
		Actor->GetComponents(StaticMeshComponents);

		for( int32 ComponentIdx=0;ComponentIdx<StaticMeshComponents.Num();ComponentIdx++ )
		{
			UStaticMeshComponent* StaticMeshComponent = StaticMeshComponents[ComponentIdx];
			if( UISettings.bFilterStaticMesh && StaticMeshComponent->StaticMesh && StaticMeshComponent->StaticMesh->RenderData )
			{
				UStaticMesh* StaticMesh = StaticMeshComponent->StaticMesh;
				FStaticMeshLODResources& LODModel = StaticMesh->RenderData->LODResources[0];
				TArray<FFoliagePaintBucketTriangle>& PotentialTriangles = ComponentPotentialTriangles.Add(StaticMeshComponent, TArray<FFoliagePaintBucketTriangle>() );

				bool bHasInstancedColorData = false;
				const FStaticMeshComponentLODInfo* InstanceMeshLODInfo = NULL;
				if( StaticMeshComponent->LODData.Num() > 0 )
				{		
					InstanceMeshLODInfo = StaticMeshComponent->LODData.GetTypedData();
					bHasInstancedColorData = InstanceMeshLODInfo->PaintedVertices.Num() == LODModel.VertexBuffer.GetNumVertices();
				}

				const bool bHasColorData = bHasInstancedColorData || LODModel.ColorVertexBuffer.GetNumVertices();

				// Get the raw triangle data for this static mesh
				FIndexArrayView Indices = LODModel.IndexBuffer.GetArrayView();
				const FPositionVertexBuffer& PositionVertexBuffer = LODModel.PositionVertexBuffer;
				const FColorVertexBuffer& ColorVertexBuffer = LODModel.ColorVertexBuffer;

				// Build a mapping of vertex positions to vertex colors.  Using a TMap will allow for fast lookups so we can match new static mesh vertices with existing colors 
				for( int32 Idx = 0; Idx < Indices.Num(); Idx+=3 )
				{
					const int32 Index0 = Indices[Idx];
					const int32 Index1 = Indices[Idx+1];
					const int32 Index2 = Indices[Idx+2];

					new(PotentialTriangles) FFoliagePaintBucketTriangle(LocalToWorld
						, PositionVertexBuffer.VertexPosition(Index0)
						, PositionVertexBuffer.VertexPosition(Index1)
						, PositionVertexBuffer.VertexPosition(Index2)
						, bHasInstancedColorData ? InstanceMeshLODInfo->PaintedVertices[Index0].Color : (bHasColorData ? ColorVertexBuffer.VertexColor(Index0) : FColor::White)
						, bHasInstancedColorData ? InstanceMeshLODInfo->PaintedVertices[Index1].Color : (bHasColorData ? ColorVertexBuffer.VertexColor(Index1) : FColor::White)
						, bHasInstancedColorData ? InstanceMeshLODInfo->PaintedVertices[Index2].Color : (bHasColorData ? ColorVertexBuffer.VertexColor(Index2) : FColor::White)
						);
				}
			}
		}

		// Place foliage
		AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());

		for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(IFA->FoliageMeshes); MeshIt; ++MeshIt )
		{
			FFoliageMeshInfo& MeshInfo = MeshIt.Value();
			UInstancedFoliageSettings* MeshSettings = MeshInfo.Settings;

			if( MeshSettings->IsSelected )
			{
				// Quick lookup of potential instance locations, used for overlapping check.
				TArray<FVector> PotentialInstanceLocations;
				FFoliageInstanceHash PotentialInstanceHash(7);	// use 128x128 cell size, as the brush radius is typically small.
				TArray<FPotentialInstance> InstancesToPlace;

				// Radius where we expect to have a single instance, given the density rules
				const float DensityCheckRadius = FMath::Max<float>( FMath::Sqrt( (1000.f*1000.f) / (PI * MeshSettings->Density * UISettings.GetPaintDensity() ) ), MeshSettings->Radius );

				for( TMap<UPrimitiveComponent*, TArray<FFoliagePaintBucketTriangle> >::TIterator ComponentIt(ComponentPotentialTriangles); ComponentIt; ++ComponentIt )
				{
					UPrimitiveComponent* Component = ComponentIt.Key();
					TArray<FFoliagePaintBucketTriangle>& PotentialTriangles = ComponentIt.Value();

					for( int32 TriIdx=0;TriIdx<PotentialTriangles.Num();TriIdx++ )
					{
						FFoliagePaintBucketTriangle& Triangle = PotentialTriangles[TriIdx];

						// Check if we can reject this triangle based on normal.
						if( (MeshSettings->GroundSlope > 0.f && Triangle.WorldNormal.Z <= FMath::Cos( PI * MeshSettings->GroundSlope / 180.f)-SMALL_NUMBER) ||
							(MeshSettings->GroundSlope < 0.f && Triangle.WorldNormal.Z >= FMath::Cos(-PI * MeshSettings->GroundSlope / 180.f)+SMALL_NUMBER) )
						{
							continue;
						}

						// This is the total set of instances disregarding parameters like slope, height or layer.
						float DesiredInstanceCountFloat = Triangle.Area * MeshSettings->Density * UISettings.GetPaintDensity() / (1000.f*1000.f);

						// Allow a single instance with a random chance, if the brush is smaller than the density
						int32 DesiredInstanceCount = DesiredInstanceCountFloat > 1.f ? FMath::Round(DesiredInstanceCountFloat) : FMath::FRand() < DesiredInstanceCountFloat ? 1 : 0;
				
						for( int32 Idx=0;Idx<DesiredInstanceCount;Idx++ )
						{
							FVector InstLocation;
							FColor VertexColor;
							Triangle.GetRandomPoint(InstLocation, VertexColor);

							// Check color mask and filters at this location
							if( !CheckVertexColor(MeshSettings, VertexColor) ||
								!CheckLocationForPotentialInstance( MeshInfo, MeshSettings, DensityCheckRadius, InstLocation, Triangle.WorldNormal, PotentialInstanceLocations, PotentialInstanceHash ) )
							{
								continue;
							}
													
							new(InstancesToPlace) FPotentialInstance( InstLocation, Triangle.WorldNormal, Component, 1.f );
						}
					}
				}

				// We need a world pointer
				UWorld* World = Actor->GetWorld();
				check(World);
				// Place instances
				for( int32 Idx=0;Idx<InstancesToPlace.Num();Idx++ )
				{
					FFoliageInstance Inst;
					if (InstancesToPlace[Idx].PlaceInstance(MeshSettings, Inst, World ))
					{
						MeshInfo.AddInstance( IFA, MeshIt.Key(), Inst );
					}
				}
			}
		}
	}
}

bool FEdModeFoliage::GetStaticMeshVertexColorForHit( UStaticMeshComponent* InStaticMeshComponent, int32 InTriangleIndex, const FVector& InHitLocation, FColor& OutVertexColor ) const
{
	UStaticMesh* StaticMesh = InStaticMeshComponent->StaticMesh;

	if( StaticMesh == NULL || StaticMesh->RenderData == NULL )
	{
		return false;
	}

	FStaticMeshLODResources& LODModel = StaticMesh->RenderData->LODResources[0];
	bool bHasInstancedColorData = false;
	const FStaticMeshComponentLODInfo* InstanceMeshLODInfo = NULL;
	if( InStaticMeshComponent->LODData.Num() > 0 )
	{		
		InstanceMeshLODInfo = InStaticMeshComponent->LODData.GetTypedData();
		bHasInstancedColorData = InstanceMeshLODInfo->PaintedVertices.Num() == LODModel.VertexBuffer.GetNumVertices();
	}

	const FColorVertexBuffer& ColorVertexBuffer = LODModel.ColorVertexBuffer;

	// no vertex color data
	if( !bHasInstancedColorData && ColorVertexBuffer.GetNumVertices() == 0 )
	{
		return false;
	}

	// Get the raw triangle data for this static mesh
	FIndexArrayView Indices = LODModel.IndexBuffer.GetArrayView();
	const FPositionVertexBuffer& PositionVertexBuffer = LODModel.PositionVertexBuffer;

	int32 SectionFirstTriIndex = 0;
	for(TArray<FStaticMeshSection>::TConstIterator SectionIt(LODModel.Sections);SectionIt;++SectionIt)
	{
		const FStaticMeshSection& Section = *SectionIt;

		if (Section.bEnableCollision)
		{
			int32 NextSectionTriIndex = SectionFirstTriIndex + Section.NumTriangles;
			if( InTriangleIndex >= SectionFirstTriIndex && InTriangleIndex < NextSectionTriIndex )
			{

				int32 IndexBufferIdx = (InTriangleIndex - SectionFirstTriIndex) * 3 + Section.FirstIndex;

				// Look up the triangle vertex indices
				int32 Index0 = Indices[IndexBufferIdx];
				int32 Index1 = Indices[IndexBufferIdx+1];
				int32 Index2 = Indices[IndexBufferIdx+2];

				// Lookup the triangle world positions and colors.
				FVector WorldVert0 = InStaticMeshComponent->ComponentToWorld.TransformPosition(PositionVertexBuffer.VertexPosition(Index0));
				FVector WorldVert1 = InStaticMeshComponent->ComponentToWorld.TransformPosition(PositionVertexBuffer.VertexPosition(Index1));
				FVector WorldVert2 = InStaticMeshComponent->ComponentToWorld.TransformPosition(PositionVertexBuffer.VertexPosition(Index2));

				FLinearColor Color0;
				FLinearColor Color1;
				FLinearColor Color2;

				if( bHasInstancedColorData )
				{
					Color0 = InstanceMeshLODInfo->PaintedVertices[Index0].Color.ReinterpretAsLinear();
					Color1 = InstanceMeshLODInfo->PaintedVertices[Index1].Color.ReinterpretAsLinear();
					Color2 = InstanceMeshLODInfo->PaintedVertices[Index2].Color.ReinterpretAsLinear();
				}
				else
				{
					Color0 = ColorVertexBuffer.VertexColor(Index0).ReinterpretAsLinear();
					Color1 = ColorVertexBuffer.VertexColor(Index1).ReinterpretAsLinear();
					Color2 = ColorVertexBuffer.VertexColor(Index2).ReinterpretAsLinear();
				}

				// find the barycentric coordiantes of the hit location, so we can interpolate the vertex colors
				FVector BaryCoords = FMath::GetBaryCentric2D(InHitLocation, WorldVert0, WorldVert1, WorldVert2);

				FLinearColor InterpColor = BaryCoords.X * Color0 + BaryCoords.Y * Color1 + BaryCoords.Z * Color2;

				// convert back to FColor.
				OutVertexColor = InterpColor.ToFColor(false);

				return true;
			}

			SectionFirstTriIndex = NextSectionTriIndex;
		}
	}
	
	return false;
}



/** Get list of meshs for current level for UI */

TArray<struct FFoliageMeshUIInfo>& FEdModeFoliage::GetFoliageMeshList()
{
	UpdateFoliageMeshList(); 
	return FoliageMeshList;
}

void FEdModeFoliage::UpdateFoliageMeshList()
{
	FoliageMeshList.Empty();

	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());

	for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(IFA->FoliageMeshes); MeshIt; ++MeshIt )
	{
		new(FoliageMeshList) FFoliageMeshUIInfo(MeshIt.Key(), &MeshIt.Value());
	}

	struct FCompareFFoliageMeshUIInfo
	{
		FORCEINLINE bool operator()( const FFoliageMeshUIInfo& A, const FFoliageMeshUIInfo& B ) const
		{
			return A.MeshInfo->Settings->DisplayOrder < B.MeshInfo->Settings->DisplayOrder;
		}
	};
	FoliageMeshList.Sort( FCompareFFoliageMeshUIInfo() );
}

/** Add a new mesh */
void FEdModeFoliage::AddFoliageMesh(UStaticMesh* StaticMesh)
{
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
	if( IFA->FoliageMeshes.Find(StaticMesh) == NULL )
	{
		IFA->AddMesh(StaticMesh);

		// Update mesh list.
		UpdateFoliageMeshList();
	}
}

/** Remove a mesh */
bool FEdModeFoliage::RemoveFoliageMesh(UStaticMesh* StaticMesh)
{
	bool bMeshRemoved = false;

	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
	FFoliageMeshInfo* MeshInfo = IFA->FoliageMeshes.Find(StaticMesh);
	if( MeshInfo != NULL )
	{
		int32 InstancesNum = MeshInfo->Instances.Num() - MeshInfo->FreeInstanceIndices.Num();
		FText Message = FText::Format( NSLOCTEXT("UnrealEd", "FoliageMode_DeleteMesh", "Are you sure you want to remove all {0} instances of {1} from this level?"), FText::AsNumber( InstancesNum ), FText::FromString( StaticMesh->GetName() ) );
		
		if( InstancesNum == 0 || EAppReturnType::Yes == FMessageDialog::Open( EAppMsgType::YesNo, Message ) )
		{
			GEditor->BeginTransaction( NSLOCTEXT("UnrealEd", "FoliageMode_RemoveMeshTransaction", "Foliage Editing: Remove Mesh") );
			IFA->RemoveMesh(StaticMesh);
			GEditor->EndTransaction();

			bMeshRemoved = true;
		}		

		// Update mesh list.
		UpdateFoliageMeshList();
	}

	return bMeshRemoved;
}

/** Bake instances to StaticMeshActors */
void FEdModeFoliage::BakeFoliage(UStaticMesh* StaticMesh, bool bSelectedOnly)
{
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
	FFoliageMeshInfo* MeshInfo = IFA->FoliageMeshes.Find(StaticMesh);
	if( MeshInfo != NULL )
	{
		TArray<int32> InstancesToConvert;
		if( bSelectedOnly )
		{
			InstancesToConvert = MeshInfo->SelectedIndices;	
		}
		else
		{
			for( int32 InstanceIdx=0;InstanceIdx<MeshInfo->Instances.Num();InstanceIdx++ )
			{
				InstancesToConvert.Add(InstanceIdx);
			}
		}

		// Convert
		for( int32 Idx=0;Idx<InstancesToConvert.Num();Idx++ )
		{
			FFoliageInstance& Instance = MeshInfo->Instances[InstancesToConvert[Idx]];
			// We need a world in which to spawn the actor. Use the one from the original instance.
			UWorld* World = IFA->GetWorld();
			check(World != NULL);
			AStaticMeshActor* SMA = World->SpawnActor<AStaticMeshActor>( Instance.Location, Instance.Rotation );
			SMA->StaticMeshComponent->StaticMesh = StaticMesh;
			SMA->GetRootComponent()->SetRelativeScale3D(Instance.DrawScale3D);	
			SMA->MarkComponentsRenderStateDirty();
		}

		// Remove
		MeshInfo->RemoveInstances(IFA, InstancesToConvert);
	}
}

/** Copy the settings object for this static mesh */
UInstancedFoliageSettings* FEdModeFoliage::CopySettingsObject(UStaticMesh* StaticMesh)
{
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
	FFoliageMeshInfo* MeshInfo = IFA->FoliageMeshes.Find(StaticMesh);
	if( MeshInfo )
	{
		GEditor->BeginTransaction( NSLOCTEXT("UnrealEd", "FoliageMode_SettingsObjectTransaction", "Foliage Editing: Settings Object") );
		IFA->Modify();
		MeshInfo->Settings = Cast<UInstancedFoliageSettings>(StaticDuplicateObject(MeshInfo->Settings,IFA,NULL));
		MeshInfo->Settings->ClearFlags(RF_Standalone|RF_Public);
		GEditor->EndTransaction();

		return MeshInfo->Settings;
	}

	return NULL;
}

/** Replace the settings object for this static mesh with the one specified */
void FEdModeFoliage::ReplaceSettingsObject(UStaticMesh* StaticMesh, UInstancedFoliageSettings* NewSettings)
{
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
	FFoliageMeshInfo* MeshInfo = IFA->FoliageMeshes.Find(StaticMesh);

	if( MeshInfo )
	{
		GEditor->BeginTransaction( NSLOCTEXT("UnrealEd", "FoliageMode_SettingsObjectTransaction", "Foliage Editing: Settings Object") );
		IFA->Modify();
		MeshInfo->Settings = NewSettings;
		GEditor->EndTransaction();
	}
}

/** Save the settings object */
UInstancedFoliageSettings* FEdModeFoliage::SaveSettingsObject(const FText& InSettingsPackageName, UStaticMesh* StaticMesh)
{
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
	FFoliageMeshInfo* MeshInfo = IFA->FoliageMeshes.Find(StaticMesh);

	if( MeshInfo )
	{
		FString PackageName = InSettingsPackageName.ToString();
		UPackage* Package = CreatePackage(NULL, *PackageName);
		check(Package);

		GEditor->BeginTransaction( NSLOCTEXT("UnrealEd", "FoliageMode_SettingsObjectTransaction", "Foliage Editing: Settings Object") );
		IFA->Modify();
		MeshInfo->Settings = Cast<UInstancedFoliageSettings>(StaticDuplicateObject(MeshInfo->Settings,Package,*FPackageName::GetLongPackageAssetName(PackageName)));
		MeshInfo->Settings->SetFlags(RF_Standalone|RF_Public);
		GEditor->EndTransaction();
			
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(MeshInfo->Settings);

		return MeshInfo->Settings;	
	}

	return NULL;

}

/** Reapply cluster settings to all the instances */
void FEdModeFoliage::ReallocateClusters(UStaticMesh* StaticMesh)
{
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
	FFoliageMeshInfo* MeshInfo = IFA->FoliageMeshes.Find(StaticMesh);
	if( MeshInfo != NULL )
	{
		MeshInfo->ReallocateClusters(IFA, StaticMesh);
	}
}

/** Replace a mesh with another one */
bool FEdModeFoliage::ReplaceStaticMesh(UStaticMesh* OldStaticMesh, UStaticMesh* NewStaticMesh, bool& bOutMeshMerged)
{
	bOutMeshMerged = false;

	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
	FFoliageMeshInfo* OldMeshInfo = IFA->FoliageMeshes.Find(OldStaticMesh);

	if( OldMeshInfo != NULL && OldStaticMesh != NewStaticMesh )
	{
		int32 InstancesNum = OldMeshInfo->Instances.Num() - OldMeshInfo->FreeInstanceIndices.Num();

		// Look for the new mesh in the mesh list, and either create a new mesh or merge the instances.
		FFoliageMeshInfo* NewMeshInfo = IFA->FoliageMeshes.Find(NewStaticMesh);
		if( NewMeshInfo == NULL )
		{
			FText Message = FText::Format( NSLOCTEXT("UnrealEd", "FoliageMode_ReplaceMesh", "Are you sure you want to replace all {0} instances of {1} with {2}?"), FText::AsNumber( InstancesNum ), FText::FromString( OldStaticMesh->GetName() ), FText::FromString( NewStaticMesh->GetName() ) );

			if( InstancesNum > 0 && EAppReturnType::Yes != FMessageDialog::Open(EAppMsgType::YesNo, Message ) )
			{
				return false;
			}

			GEditor->BeginTransaction( NSLOCTEXT("UnrealEd", "FoliageMode_ChangeStaticMeshTransaction", "Foliage Editing: Change StaticMesh") );
			IFA->Modify();
			NewMeshInfo = IFA->AddMesh(NewStaticMesh);
			NewMeshInfo->Settings->DisplayOrder          = OldMeshInfo->Settings->DisplayOrder;
			NewMeshInfo->Settings->ShowNothing          = OldMeshInfo->Settings->ShowNothing;
			NewMeshInfo->Settings->ShowPaintSettings    = OldMeshInfo->Settings->ShowPaintSettings;
			NewMeshInfo->Settings->ShowInstanceSettings = OldMeshInfo->Settings->ShowInstanceSettings;
		}
		else
		if( InstancesNum > 0 &&
			EAppReturnType::Yes != FMessageDialog::Open(EAppMsgType::YesNo,
				FText::Format(NSLOCTEXT("UnrealEd", "FoliageMode_ReplaceMeshMerge", "Are you sure you want to merge all {0} instances of {1} into {2}?"),
					FText::AsNumber(InstancesNum), FText::FromString(OldStaticMesh->GetName()), FText::FromString(NewStaticMesh->GetName()))) )
		{
			return false;
		}
		else
		{
			bOutMeshMerged = true;

			GEditor->BeginTransaction( NSLOCTEXT("UnrealEd", "FoliageMode_ChangeStaticMeshTransaction", "Foliage Editing: Change StaticMesh") );
			IFA->Modify();
		}

		if( InstancesNum > 0 )
		{
			// copy instances from old to new.
			for( int32 Idx=0;Idx<OldMeshInfo->Instances.Num();Idx++ )
			{
				if( OldMeshInfo->Instances[Idx].ClusterIndex != -1 )
				{
					NewMeshInfo->AddInstance( IFA, NewStaticMesh, OldMeshInfo->Instances[Idx] );
				}
			}
		}

		// Remove the old mesh.
		IFA->RemoveMesh(OldStaticMesh);

		GEditor->EndTransaction();
		
		// Update mesh list.
		UpdateFoliageMeshList();
	}
	return true;
}


/** FEdMode: Called when a key is pressed */
bool FEdModeFoliage::InputKey( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event )
{
	if( UISettings.GetPaintToolSelected() || UISettings.GetReapplyToolSelected() || UISettings.GetLassoSelectToolSelected() )
	{
		if( Key == EKeys::LeftMouseButton && Event == IE_Pressed && IsCtrlDown(Viewport) )
		{
			if( !bToolActive )
			{
				GEditor->BeginTransaction( NSLOCTEXT("UnrealEd", "FoliageMode_EditTransaction", "Foliage Editing") );
				InstanceSnapshot.Empty();

				// Special setup beginning a stroke with the Reapply tool
				// Necessary so we don't keep reapplying settings over and over for the same instances.
				if( UISettings.GetReapplyToolSelected() )
				{
					AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
					for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(IFA->FoliageMeshes); MeshIt; ++MeshIt )
					{
						FFoliageMeshInfo& MeshInfo = MeshIt.Value();
						if( MeshInfo.Settings->IsSelected )
						{
							// Take a snapshot of all the locations
							InstanceSnapshot.Add( MeshIt.Key(), FMeshInfoSnapshot(&MeshInfo) ) ;

							// Clear the "FOLIAGE_Readjusted" flag
							for( int32 Idx=0;Idx<MeshInfo.Instances.Num();Idx++ )
							{
								MeshInfo.Instances[Idx].Flags &= (~FOLIAGE_Readjusted);
							}							
						}
					}
				}
			}
			ApplyBrush(ViewportClient);
			bToolActive = true;
			return true;
		}

		if( Event == IE_Released && bToolActive && (Key == EKeys::LeftMouseButton || Key==EKeys::LeftControl || Key==EKeys::RightControl) )
		{
			//Set the cursor position to that of the slate cursor so it wont snap back
			Viewport->SetPreCaptureMousePosFromSlateCursor();
			GEditor->EndTransaction();
			InstanceSnapshot.Empty();
			LandscapeLayerCaches.Empty();
			bToolActive = false;
			return true;
		}
	}

	if( UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected() )
	{
		if( Event == IE_Pressed )
		{
			if( Key == EKeys::Delete )
			{
				GEditor->BeginTransaction( NSLOCTEXT("UnrealEd", "FoliageMode_EditTransaction", "Foliage Editing") );
				AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
				IFA->Modify();
				for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(IFA->FoliageMeshes); MeshIt; ++MeshIt )
				{
					FFoliageMeshInfo& Mesh = MeshIt.Value();
					if( Mesh.SelectedIndices.Num() > 0 )
					{
						TArray<int32> InstancesToDelete = Mesh.SelectedIndices;
						Mesh.RemoveInstances(IFA, InstancesToDelete);
					}
				}
				GEditor->EndTransaction();

				return true;
			}
			else
			if( Key == EKeys::End )
			{
				// Snap instances to ground
				AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
				IFA->Modify();
				bool bMovedInstance = false;
				for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(IFA->FoliageMeshes); MeshIt; ++MeshIt )
				{
					FFoliageMeshInfo& Mesh = MeshIt.Value();
					
					Mesh.PreMoveInstances(IFA, Mesh.SelectedIndices);

					UWorld* World = ViewportClient->GetWorld();
					for( int32 SelectionIdx=0;SelectionIdx<Mesh.SelectedIndices.Num();SelectionIdx++ )
					{
						int32 InstanceIdx = Mesh.SelectedIndices[SelectionIdx];

						FFoliageInstance& Instance = Mesh.Instances[InstanceIdx];

						FVector Start = Instance.Location;
						FVector End = Instance.Location - FVector(0.f,0.f,FOLIAGE_SNAP_TRACE);

						FHitResult Hit;
						if( World->LineTraceSingle(Hit, Start, End, FCollisionQueryParams(FName("FoliageSnap"), true), FCollisionObjectQueryParams(ECC_WorldStatic))) 
						{
							// Check current level
							if( (Hit.Component.IsValid() && Hit.Component.Get()->GetOutermost() == World->GetCurrentLevel()->GetOutermost()) ||
								(Hit.GetActor() && Hit.GetActor()->IsA(AWorldSettings::StaticClass())) )
							{
								Instance.Location = Hit.Location;
								Instance.ZOffset = 0.f;
								Instance.Base = Hit.Component.Get();

								if( Instance.Flags & FOLIAGE_AlignToNormal )
								{
									// Remove previous alignment and align to new normal.
									Instance.Rotation = Instance.PreAlignRotation;
									Instance.AlignToNormal( Hit.Normal, Mesh.Settings->AlignMaxAngle );
								}
							}
						}
						bMovedInstance = true;
					}

					Mesh.PostMoveInstances(IFA, Mesh.SelectedIndices);
				}

				if( bMovedInstance )
				{
					FEditorModeTools& Tools = GEditorModeTools();
					Tools.PivotLocation = Tools.SnappedLocation = IFA->GetSelectionLocation();
					IFA->MarkComponentsRenderStateDirty();
				}

				return true;
			}
		}
	}

	return false;
}

/** FEdMode: Render the foliage edit mode */
void FEdModeFoliage::Render( const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI )
{
	/** Call parent implementation */
	FEdMode::Render( View, Viewport, PDI );
}


/** FEdMode: Render HUD elements for this tool */
void FEdModeFoliage::DrawHUD( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas )
{
}

/** FEdMode: Check to see if an actor can be selected in this mode - no side effects */
bool FEdModeFoliage::IsSelectionAllowed( AActor* InActor, bool bInSelection ) const
{
	return false;
}

/** FEdMode: Handling SelectActor */
bool FEdModeFoliage::Select( AActor* InActor, bool bInSelected )
{
	// return true if you filter that selection
	// however - return false if we are trying to deselect so that it will infact do the deselection
	if( bInSelected == false )
	{
		return false;
	}
	return true;
}

/** FEdMode: Called when the currently selected actor has changed */
void FEdModeFoliage::ActorSelectionChangeNotify()
{
}


/** Forces real-time perspective viewports */
void FEdModeFoliage::ForceRealTimeViewports( const bool bEnable, const bool bStoreCurrentState )
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor");
	TSharedPtr< ILevelViewport > ViewportWindow = LevelEditorModule.GetFirstActiveViewport();
	if (ViewportWindow.IsValid())
	{
		FLevelEditorViewportClient &Viewport = ViewportWindow->GetLevelViewportClient();
		if( Viewport.IsPerspective() )
		{				
			if( bEnable )
			{
				Viewport.SetRealtime( bEnable, bStoreCurrentState );
			}
			else
			{
				const bool bAllowDisable = true;
				Viewport.RestoreRealtime( bAllowDisable );
			}

		}
	}
}

bool FEdModeFoliage::HandleClick(FLevelEditorViewportClient* InViewportClient, HHitProxy *HitProxy, const FViewportClick &Click )
{
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());

	if( UISettings.GetSelectToolSelected() )
	{
		if( HitProxy && HitProxy->IsA(HInstancedStaticMeshInstance::StaticGetType()) )
		{
			HInstancedStaticMeshInstance* SMIProxy = ( ( HInstancedStaticMeshInstance* )HitProxy );
		
			IFA->SelectInstance( SMIProxy->Component, SMIProxy->InstanceIndex, Click.IsControlDown() );

			// Update pivot
			FEditorModeTools& Tools = GEditorModeTools();
			Tools.PivotLocation = Tools.SnappedLocation = IFA->GetSelectionLocation();
		}
		else
		{
			if( !Click.IsControlDown() )
			{
				// Select none if not trying to toggle
				IFA->SelectInstance( NULL, -1, false );
			}
		}

		return true;
	}
	else
	if( UISettings.GetPaintBucketToolSelected() || UISettings.GetReapplyPaintBucketToolSelected() )
	{
		if( HitProxy && HitProxy->IsA(HActor::StaticGetType()) && Click.IsControlDown() )
		{
			GEditor->BeginTransaction( NSLOCTEXT("UnrealEd", "FoliageMode_EditTransaction", "Foliage Editing") );
			ApplyPaintBucket( ((HActor*)HitProxy)->Actor, Click.IsShiftDown() );
			GEditor->EndTransaction();
		}

		return true;
	}

	return FEdMode::HandleClick(InViewportClient, HitProxy, Click);
}

FVector FEdModeFoliage::GetWidgetLocation() const
{
	return FEdMode::GetWidgetLocation();
}

/** FEdMode: Called when a mouse button is pressed */
bool FEdModeFoliage::StartTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if( UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected() )
	{
		// Update pivot
		AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
		FEditorModeTools& Tools = GEditorModeTools();
		Tools.PivotLocation = Tools.SnappedLocation = IFA->GetSelectionLocation();

		GEditor->BeginTransaction( NSLOCTEXT("UnrealEd", "FoliageMode_EditTransaction", "Foliage Editing") );
		
		bCanAltDrag = true;
	
		return true;
	}
	return FEdMode::StartTracking(InViewportClient, InViewport);
}

/** FEdMode: Called when the a mouse button is released */
bool FEdModeFoliage::EndTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if( UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected() )
	{
		GEditor->EndTransaction();
		return true;
	}
	return FEdMode::EndTracking(InViewportClient, InViewport);
}

/** FEdMode: Called when mouse drag input it applied */
bool FEdModeFoliage::InputDelta( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale )
{
	bool bFoundSelection = false;
	
	bool bAltDown = InViewport->KeyState(EKeys::LeftAlt) || InViewport->KeyState(EKeys::RightAlt);

	if( InViewportClient->GetCurrentWidgetAxis() != EAxisList::None && (UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected()) )
	{
		AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld());
		IFA->Modify();
		for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(IFA->FoliageMeshes); MeshIt; ++MeshIt )
		{
			FFoliageMeshInfo& MeshInfo = MeshIt.Value();
			bFoundSelection |= MeshInfo.SelectedIndices.Num() > 0;

			if( bAltDown && bCanAltDrag && (InViewportClient->GetCurrentWidgetAxis() & EAxisList::XYZ)  )
			{
				MeshInfo.DuplicateInstances(IFA, MeshIt.Key(), MeshInfo.SelectedIndices);
			}
			
			MeshInfo.PreMoveInstances(IFA, MeshInfo.SelectedIndices);

			for( int32 SelectionIdx=0;SelectionIdx<MeshInfo.SelectedIndices.Num();SelectionIdx++ )
			{
				int32 InstanceIndex = MeshInfo.SelectedIndices[SelectionIdx];
				FFoliageInstance& Instance = MeshInfo.Instances[InstanceIndex];
				Instance.Location += InDrag;
				Instance.ZOffset = 0.f;
				Instance.Rotation += InRot;
				Instance.DrawScale3D += InScale;
			}

			MeshInfo.PostMoveInstances(IFA, MeshInfo.SelectedIndices);
		}

		// Only allow alt-drag on first InputDelta
		bCanAltDrag = false;

		if( bFoundSelection )
		{
			IFA->MarkComponentsRenderStateDirty();
			return true;
		}
	}

	return FEdMode::InputDelta(InViewportClient,InViewport,InDrag,InRot,InScale);
}

bool FEdModeFoliage::AllowWidgetMove()
{
	return ShouldDrawWidget();
}

bool FEdModeFoliage::UsesTransformWidget() const
{
	return ShouldDrawWidget();
}

bool FEdModeFoliage::ShouldDrawWidget() const
{
	return (UISettings.GetSelectToolSelected() || (UISettings.GetLassoSelectToolSelected() && !bToolActive)) && AInstancedFoliageActor::GetInstancedFoliageActor(GetWorld())->SelectedMesh != NULL;
}

EAxisList::Type FEdModeFoliage::GetWidgetAxisToDraw( FWidget::EWidgetMode InWidgetMode ) const
{
	switch(InWidgetMode)
	{
	case FWidget::WM_Translate:
	case FWidget::WM_Rotate:
	case FWidget::WM_Scale:
		return EAxisList::XYZ;
	default:
		return EAxisList::None;
	}
}

/** Load UI settings from ini file */
void FFoliageUISettings::Load()
{
	FString WindowPositionString;
	if( GConfig->GetString( TEXT("FoliageEdit"), TEXT("WindowPosition"), WindowPositionString, GEditorUserSettingsIni ) )
	{
		TArray<FString> PositionValues;
		if( WindowPositionString.ParseIntoArray( &PositionValues, TEXT( "," ), true ) == 4 )
		{
			WindowX = FCString::Atoi( *PositionValues[0] );
			WindowY = FCString::Atoi( *PositionValues[1] );
			WindowWidth = FCString::Atoi( *PositionValues[2] );
			WindowHeight = FCString::Atoi( *PositionValues[3] );
		}
	}

	GConfig->GetFloat( TEXT("FoliageEdit"), TEXT("Radius"), Radius, GEditorUserSettingsIni );
	GConfig->GetFloat( TEXT("FoliageEdit"), TEXT("PaintDensity"), PaintDensity, GEditorUserSettingsIni );
	GConfig->GetFloat( TEXT("FoliageEdit"), TEXT("UnpaintDensity"), UnpaintDensity, GEditorUserSettingsIni );
	GConfig->GetBool( TEXT("FoliageEdit"), TEXT("bFilterLandscape"), bFilterLandscape, GEditorUserSettingsIni );
	GConfig->GetBool( TEXT("FoliageEdit"), TEXT("bFilterStaticMesh"), bFilterStaticMesh, GEditorUserSettingsIni );
	GConfig->GetBool( TEXT("FoliageEdit"), TEXT("bFilterBSP"), bFilterBSP, GEditorUserSettingsIni );
}

/** Save UI settings to ini file */
void FFoliageUISettings::Save()
{
	FString WindowPositionString = FString::Printf(TEXT("%d,%d,%d,%d"), WindowX, WindowY, WindowWidth, WindowHeight );
	GConfig->SetString( TEXT("FoliageEdit"), TEXT("WindowPosition"), *WindowPositionString, GEditorUserSettingsIni );

	GConfig->SetFloat( TEXT("FoliageEdit"), TEXT("Radius"), Radius, GEditorUserSettingsIni );
	GConfig->SetFloat( TEXT("FoliageEdit"), TEXT("PaintDensity"), PaintDensity, GEditorUserSettingsIni );
	GConfig->SetFloat( TEXT("FoliageEdit"), TEXT("UnpaintDensity"), UnpaintDensity, GEditorUserSettingsIni );
	GConfig->SetBool( TEXT("FoliageEdit"), TEXT("bFilterLandscape"), bFilterLandscape, GEditorUserSettingsIni );
	GConfig->SetBool( TEXT("FoliageEdit"), TEXT("bFilterStaticMesh"), bFilterStaticMesh, GEditorUserSettingsIni );
	GConfig->SetBool( TEXT("FoliageEdit"), TEXT("bFilterBSP"), bFilterBSP, GEditorUserSettingsIni );
}

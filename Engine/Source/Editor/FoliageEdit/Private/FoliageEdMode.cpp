// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "StaticMeshResources.h"
#include "ObjectTools.h"
#include "FoliageEdMode.h"
#include "ScopedTransaction.h"

#include "FoliageEdModeToolkit.h"
#include "ModuleManager.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Toolkits/ToolkitManager.h"

#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"

//Slate dependencies
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/LevelEditor/Public/SLevelViewport.h"
#include "Editor/UnrealEd/Public/Dialogs/DlgPickAssetPath.h"

// Classes
#include "InstancedFoliageActor.h"
#include "FoliageType.h"
#include "LandscapeProxy.h"
#include "LandscapeComponent.h"
#include "LandscapeHeightfieldCollisionComponent.h"
#include "LandscapeInfo.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/CollisionProfile.h"
#include "Components/ModelComponent.h"
#include "EngineUtils.h"
#include "LandscapeDataAccess.h"
#include "FoliageType_InstancedStaticMesh.h"


#define LOCTEXT_NAMESPACE "FoliageEdMode"
#define FOLIAGE_SNAP_TRACE (10000.f)

//
// FFoliageMeshInfo iterator
//
class FFoliageMeshInfoIterator
{
private:
	const UWorld*				World;
	const UFoliageType*			FoliageType;
	FFoliageMeshInfo*			CurrentMeshInfo;
	AInstancedFoliageActor*		CurrentIFA;
	int32						LevelIdx;

public:
	FFoliageMeshInfoIterator(const UWorld* InWorld, const UFoliageType* InFoliageType)
		: World(InWorld)
		, FoliageType(InFoliageType)
		, CurrentMeshInfo(nullptr)
		, CurrentIFA(nullptr)
	{
		// shortcut for non-assets
		if (!InFoliageType->IsAsset())
		{
			LevelIdx = World->GetNumLevels();
			AInstancedFoliageActor* IFA = Cast<AInstancedFoliageActor>(FoliageType->GetOuter());
			if (IFA->GetLevel()->bIsVisible)
			{
				CurrentIFA = IFA;
				CurrentMeshInfo = CurrentIFA->FindMesh(FoliageType);
			}
		}
		else
		{
			LevelIdx= -1;
			++(*this);
		}
	}

	void operator++()
	{
		const int32 NumLevels = World->GetNumLevels();
		int32 LocalLevelIdx = LevelIdx;
		
		while (++LocalLevelIdx < NumLevels)
		{
			ULevel* Level = World->GetLevel(LocalLevelIdx);
			if (Level && Level->bIsVisible)
			{
				CurrentIFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(Level);
				if (CurrentIFA)
				{
					CurrentMeshInfo = CurrentIFA->FindMesh(FoliageType);
					if (CurrentMeshInfo)
					{
						LevelIdx = LocalLevelIdx;
						return;
					}
				}
			}
		}

		CurrentMeshInfo = nullptr;
		CurrentIFA = nullptr;
	}

	FORCEINLINE FFoliageMeshInfo* operator*()
	{
		check(CurrentMeshInfo);
		return CurrentMeshInfo;
	}
	
	FORCEINLINE operator bool() const
	{
		return CurrentMeshInfo != nullptr;
	}

	FORCEINLINE AInstancedFoliageActor* GetActor()
	{
		return CurrentIFA;
	}
};


//
// FEdModeFoliage
//

/** Constructor */
FEdModeFoliage::FEdModeFoliage()
	: FEdMode()
	, bToolActive(false)
	, bCanAltDrag(false)
{
	// Load resources and construct brush component
	UMaterial* BrushMaterial = nullptr;
	UStaticMesh* StaticMesh = nullptr;
	if (!IsRunningCommandlet())
	{
		BrushMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EditorLandscapeResources/FoliageBrushSphereMaterial.FoliageBrushSphereMaterial"), nullptr, LOAD_None, nullptr);
		StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/Sphere.Sphere"), nullptr, LOAD_None, nullptr);
	}

	SphereBrushComponent = NewObject<UStaticMeshComponent>(GetTransientPackage(), TEXT("SphereBrushComponent"));
	SphereBrushComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SphereBrushComponent->SetCollisionObjectType(ECC_WorldDynamic);
	SphereBrushComponent->StaticMesh = StaticMesh;
	SphereBrushComponent->OverrideMaterials.Add(BrushMaterial);
	SphereBrushComponent->SetAbsolute(true, true, true);
	SphereBrushComponent->CastShadow = false;

	bBrushTraceValid = false;
	BrushLocation = FVector::ZeroVector;
}


/** Destructor */
FEdModeFoliage::~FEdModeFoliage()
{
	// Save UI settings to config file
	UISettings.Save();
	FEditorDelegates::MapChange.RemoveAll(this);
}


/** FGCObject interface */
void FEdModeFoliage::AddReferencedObjects(FReferenceCollector& Collector)
{
	// Call parent implementation
	FEdMode::AddReferencedObjects(Collector);

	Collector.AddReferencedObject(SphereBrushComponent);
}

/** FEdMode: Called when the mode is entered */
void FEdModeFoliage::Enter()
{
	FEdMode::Enter();

	// Clear any selection in case the instanced foliage actor is selected
	GEditor->SelectNone(true, true);

	// Load UI settings from config file
	UISettings.Load();

	// Bind to editor callbacks
	FEditorDelegates::NewCurrentLevel.AddSP(this, &FEdModeFoliage::NotifyNewCurrentLevel);
	FWorldDelegates::LevelAddedToWorld.AddSP(this, &FEdModeFoliage::NotifyLevelAddedToWorld);
	FWorldDelegates::LevelRemovedFromWorld.AddSP(this, &FEdModeFoliage::NotifyLevelRemovedFromWorld);

	// Force real-time viewports.  We'll back up the current viewport state so we can restore it when the
	// user exits this mode.
	const bool bWantRealTime = true;
	const bool bRememberCurrentState = true;
	ForceRealTimeViewports(bWantRealTime, bRememberCurrentState);

	if (!Toolkit.IsValid())
	{
		Toolkit = MakeShareable(new FFoliageEdModeToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
	}
	
	if (UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected())
	{
		ApplySelectionToComponents(GetWorld(), true);
	}
		
	// Update UI
	NotifyNewCurrentLevel();
}

/** FEdMode: Called when the mode is exited */
void FEdModeFoliage::Exit()
{
	FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
	Toolkit.Reset();

	//
	FEditorDelegates::NewCurrentLevel.RemoveAll(this);
	FWorldDelegates::LevelAddedToWorld.RemoveAll(this);
	FWorldDelegates::LevelRemovedFromWorld.RemoveAll(this);
	
	FoliageMeshList.Empty();
	
	// Remove the brush
	SphereBrushComponent->UnregisterComponent();

	// Restore real-time viewport state if we changed it
	const bool bWantRealTime = false;
	const bool bRememberCurrentState = false;
	ForceRealTimeViewports(bWantRealTime, bRememberCurrentState);

	// Clear the cache (safety, should be empty!)
	LandscapeLayerCaches.Empty();

	// Save UI settings to config file
	UISettings.Save();

	// Clear selection visualization on any foliage items
	ApplySelectionToComponents(GetWorld(), false);

	// Call base Exit method to ensure proper cleanup
	FEdMode::Exit();
}

void FEdModeFoliage::PostUndo()
{
	FEdMode::PostUndo();

	PopulateFoliageMeshList();
}

/** When the user changes the active streaming level with the level browser */
void FEdModeFoliage::NotifyNewCurrentLevel()
{			
	PopulateFoliageMeshList();
}

void FEdModeFoliage::NotifyLevelAddedToWorld(ULevel* InLevel, UWorld* InWorld)
{
	PopulateFoliageMeshList();
}

void FEdModeFoliage::NotifyLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	PopulateFoliageMeshList();
}

/** When the user changes the current tool in the UI */
void FEdModeFoliage::NotifyToolChanged()
{
	if (UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected())
	{
		ApplySelectionToComponents(GetWorld(), true);
	}
	else
	{
		ApplySelectionToComponents(GetWorld(), false);
	}
}

bool FEdModeFoliage::DisallowMouseDeltaTracking() const
{
	// We never want to use the mouse delta tracker while painting
	return bToolActive;
}

/** FEdMode: Called once per frame */
void FEdModeFoliage::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	if (bToolActive)
	{
		ApplyBrush(ViewportClient);
	}

	FEdMode::Tick(ViewportClient, DeltaTime);

	if (UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected())
	{
		// Update pivot
		UpdateWidgetLocationToInstanceSelection();
	}

	// Update the position and size of the brush component
	if (bBrushTraceValid && (UISettings.GetPaintToolSelected() || UISettings.GetReapplyToolSelected() || UISettings.GetLassoSelectToolSelected()))
	{
		// Scale adjustment is due to default sphere SM size.
		FTransform BrushTransform = FTransform(FQuat::Identity, BrushLocation, FVector(UISettings.GetRadius() * 0.00625f));
		SphereBrushComponent->SetRelativeTransform(BrushTransform);

		if (!SphereBrushComponent->IsRegistered())
		{
			SphereBrushComponent->RegisterComponentWithWorld(ViewportClient->GetWorld());
		}
	}
	else
	{
		if (SphereBrushComponent->IsRegistered())
		{
			SphereBrushComponent->UnregisterComponent();
		}
	}
}

/** Trace under the mouse cursor and update brush position */
void FEdModeFoliage::FoliageBrushTrace(FEditorViewportClient* ViewportClient, int32 MouseX, int32 MouseY)
{
	bBrushTraceValid = false;
	if (!ViewportClient->IsMovingCamera())
	{
		if (UISettings.GetPaintToolSelected() || UISettings.GetReapplyToolSelected() || UISettings.GetLassoSelectToolSelected())
		{
			// Compute a world space ray from the screen space mouse coordinates
			FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags)
				.SetRealtimeUpdate(ViewportClient->IsRealtime()));
			FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);
			FViewportCursorLocation MouseViewportRay(View, ViewportClient, MouseX, MouseY);

			FVector Start = MouseViewportRay.GetOrigin();
			BrushTraceDirection = MouseViewportRay.GetDirection();
			FVector End = Start + WORLD_MAX * BrushTraceDirection;

			FHitResult Hit;
			UWorld* World = ViewportClient->GetWorld();
			static FName NAME_FoliageBrush = FName(TEXT("FoliageBrush"));
			if (AInstancedFoliageActor::FoliageTrace(World, Hit, FDesiredFoliageInstance(Start, End), NAME_FoliageBrush))
			{
				// Check filters
				UPrimitiveComponent* PrimComp = Hit.Component.Get();
				UMaterialInterface* Material = PrimComp ? PrimComp->GetMaterial(0) : nullptr;

				if (PrimComp &&
					(UISettings.bFilterLandscape || !PrimComp->IsA(ULandscapeHeightfieldCollisionComponent::StaticClass())) &&
					(UISettings.bFilterStaticMesh || !PrimComp->IsA(UStaticMeshComponent::StaticClass())) &&
					(UISettings.bFilterBSP || !PrimComp->IsA(UModelComponent::StaticClass())) &&
					(UISettings.bFilterTranslucent || !Material || !IsTranslucentBlendMode(Material->GetBlendMode())) &&
					(CanPaint(PrimComp->GetComponentLevel())))
				{
					// Adjust the sphere brush
					BrushLocation = Hit.Location;
					bBrushTraceValid = true;
				}
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
bool FEdModeFoliage::MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY)
{
	FoliageBrushTrace(ViewportClient, MouseX, MouseY);
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
bool FEdModeFoliage::CapturedMouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY)
{
	FoliageBrushTrace(ViewportClient, MouseX, MouseY);
	return false;
}

void FEdModeFoliage::GetRandomVectorInBrush(FVector& OutStart, FVector& OutEnd)
{
	// Find Rx and Ry inside the unit circle
	float Ru = (2.f * FMath::FRand() - 1.f);
	float Rv = (2.f * FMath::FRand() - 1.f) * FMath::Sqrt(1.f - FMath::Square(Ru));

	// find random point in circle thru brush location parallel to screen surface
	FVector U, V;
	BrushTraceDirection.FindBestAxisVectors(U, V);
	FVector Point = Ru * U + Rv * V;

	// find distance to surface of sphere brush from this point
	float Rw = FMath::Sqrt(1.f - (FMath::Square(Ru) + FMath::Square(Rv)));

	OutStart = BrushLocation + UISettings.GetRadius() * (Ru * U + Rv * V - Rw * BrushTraceDirection);
	OutEnd = BrushLocation + UISettings.GetRadius() * (Ru * U + Rv * V + Rw * BrushTraceDirection);
}

/** This does not check for overlaps or density */
static bool CheckLocationForPotentialInstance_ThreadSafe(const UFoliageType* Settings, const FVector& Location, const FVector& Normal)
{
	// Check height range
	if (!Settings->Height.Contains(Location.Z))
	{
		return false;
	}

	// Check slope
	const float MaxNormalAngle = FMath::Cos(FMath::DegreesToRadians(Settings->GroundSlopeAngle.Max));
	const float MinNormalAngle = FMath::Cos(FMath::DegreesToRadians(Settings->GroundSlopeAngle.Min));
	if (MaxNormalAngle > Normal.Z || MinNormalAngle < Normal.Z)	//keep in mind Hit.ImpactNormal.Z is (0,0,1) dot normal. However, ground slope is with relation to plane vector, not plane normal - so we swap comparisons
	{
		return false;
	}

	return true;
}

static bool CheckForOverlappingSphere(AInstancedFoliageActor* IFA, const UFoliageType* Settings, const FSphere& Sphere)
{
	if (IFA)
	{
		FFoliageMeshInfo* MeshInfo = IFA->FindMesh(Settings);
		if (MeshInfo)
		{
			return MeshInfo->CheckForOverlappingSphere(Sphere);
		}
	}
	
	return false;
}

// Returns whether or not there is are any instances overlapping the sphere specified
static bool CheckForOverlappingSphere(const UWorld* InWorld, const UFoliageType* Settings, const FSphere& Sphere)
{
	for (FFoliageMeshInfoIterator It(InWorld, Settings); It; ++It)
	{
		FFoliageMeshInfo* MeshInfo = (*It);
		if (MeshInfo->CheckForOverlappingSphere(Sphere))
		{
			return true;
		}
	}
	
	return false;
}

static bool CheckLocationForPotentialInstance(const UWorld* InWorld, const UFoliageType* Settings, float DensityCheckRadius, const FVector& Location, const FVector& Normal, TArray<FVector>& PotentialInstanceLocations, FFoliageInstanceHash& PotentialInstanceHash)
{
	if (CheckLocationForPotentialInstance_ThreadSafe(Settings, Location, Normal) == false)
	{
		return false;
	}

	// Check existing instances. Use the Density radius rather than the minimum radius
	if (CheckForOverlappingSphere(InWorld, Settings, FSphere(Location, DensityCheckRadius)))
	{
		return false;
	}

	// Check if we're too close to any other instances
	if (Settings->Radius > 0.f)
	{
		// Check with other potential instances we're about to add.
		bool bFoundOverlapping = false;
		float RadiusSquared = FMath::Square(DensityCheckRadius/*Settings->Radius*/);

		auto TempInstances = PotentialInstanceHash.GetInstancesOverlappingBox(FBox::BuildAABB(Location, FVector(Settings->Radius)));
		for (int32 Idx : TempInstances)
		{
			if ((PotentialInstanceLocations[Idx] - Location).SizeSquared() < RadiusSquared)
			{
				bFoundOverlapping = true;
				break;
			}
		}
		if (bFoundOverlapping)
		{
			return false;
		}
	}

	int32 PotentialIdx = PotentialInstanceLocations.Add(Location);
	PotentialInstanceHash.InsertInstance(Location, PotentialIdx);

	return true;
}

static bool CheckVertexColor(const UFoliageType* Settings, const FColor& VertexColor)
{
	uint8 ColorChannel;
	switch (Settings->VertexColorMask)
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

	if (Settings->VertexColorMaskInvert)
	{
		if (ColorChannel > FMath::RoundToInt(Settings->VertexColorMaskThreshold * 255.f))
		{
			return false;
		}
	}
	else
	{
		if (ColorChannel < FMath::RoundToInt(Settings->VertexColorMaskThreshold * 255.f))
		{
			return false;
		}
	}

	return true;
}

bool LandscapeLayersValid(const UFoliageType* Settings)
{
	bool bValid = false;
	for (FName LayerName : Settings->LandscapeLayers)
	{
		bValid |= LayerName != NAME_None;
	}

	return bValid;
}

bool GetMaxHitWeight(const FVector& Location, UActorComponent* Component, const UFoliageType* Settings, FEdModeFoliage::LandscapeLayerCacheData* LandscapeLayerCaches, float& OutMaxHitWeight)
{
	float MaxHitWeight = 0.f;
	if (ULandscapeHeightfieldCollisionComponent* HitLandscapeCollision = Cast<ULandscapeHeightfieldCollisionComponent>(Component))
	{
		if (ULandscapeComponent* HitLandscape = HitLandscapeCollision->RenderComponent.Get())
		{
			for (const FName LandscapeLayerName : Settings->LandscapeLayers)
			{
				// Cache store mapping between component and weight data
				TMap<ULandscapeComponent*, TArray<uint8> >* LandscapeLayerCache = &LandscapeLayerCaches->FindOrAdd(LandscapeLayerName);;
				TArray<uint8>* LayerCache = &LandscapeLayerCache->FindOrAdd(HitLandscape);
				// TODO: FName to LayerInfo?
				const float HitWeight = HitLandscape->GetLayerWeightAtLocation(Location, HitLandscape->GetLandscapeInfo()->GetLayerInfoByName(LandscapeLayerName), LayerCache);
				MaxHitWeight = FMath::Max(MaxHitWeight, HitWeight);
			}

			OutMaxHitWeight = MaxHitWeight;
			return true;
		}
	}

	return false;
}

bool FilterByWeight(float Weight, const UFoliageType* Settings)
{
	const float WeightNeeded = FMath::Max(Settings->MinimumLayerWeight, FMath::FRand());
	return Weight < FMath::Max(SMALL_NUMBER, WeightNeeded);
}

bool GeometryFilterCheck(const FHitResult& Hit, const UWorld* InWorld, const FDesiredFoliageInstance& DesiredInst, const FFoliageUISettings* UISettings)
{
	if (UPrimitiveComponent* PrimComp = Hit.Component.Get())
	{
		if (DesiredInst.PlacementMode == EFoliagePlacementMode::Manual)
		{
			UMaterialInterface* Material = PrimComp ? PrimComp->GetMaterial(0) : nullptr;
			if ((!UISettings->bFilterLandscape && PrimComp->IsA(ULandscapeHeightfieldCollisionComponent::StaticClass())) ||
				(!UISettings->bFilterStaticMesh && PrimComp->IsA(UStaticMeshComponent::StaticClass())) ||
				(!UISettings->bFilterBSP && PrimComp->IsA(UModelComponent::StaticClass())) ||
				(!UISettings->bFilterTranslucent && Material && IsTranslucentBlendMode(Material->GetBlendMode()))
				)
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool FEdModeFoliage::VertexMaskCheck(const FHitResult& Hit, const UFoliageType* Settings)
{
	if (Settings->VertexColorMask != FOLIAGEVERTEXCOLORMASK_Disabled && Hit.FaceIndex != INDEX_NONE)
	{
		if (UStaticMeshComponent* HitStaticMeshComponent = Cast<UStaticMeshComponent>(Hit.Component.Get()))
		{
			FColor VertexColor;
			if (FEdModeFoliage::GetStaticMeshVertexColorForHit(HitStaticMeshComponent, Hit.FaceIndex, Hit.ImpactPoint, VertexColor))
			{
				if (!CheckVertexColor(Settings, VertexColor))
				{
					return false;
				}
			}
		}
	}

	return true;
}

bool LandscapeLayerCheck(const FHitResult& Hit, const UFoliageType* Settings, FEdModeFoliage::LandscapeLayerCacheData& LandscapeLayersCache, float& OutHitWeight)
{
	OutHitWeight = 1.f;
	if (LandscapeLayersValid(Settings) && GetMaxHitWeight(Hit.ImpactPoint, Hit.Component.Get(), Settings, &LandscapeLayersCache, OutHitWeight))
	{
		// Reject instance randomly in proportion to weight
		if (FilterByWeight(OutHitWeight, Settings))
		{
			return false;
		}
	}

	return true;
}

void FEdModeFoliage::CalculatePotentialInstances_ThreadSafe(const UWorld* InWorld, const UFoliageType* Settings, const TArray<FDesiredFoliageInstance>* DesiredInstances, TArray<FPotentialInstance> OutPotentialInstances[NUM_INSTANCE_BUCKETS], const FFoliageUISettings* UISettings, const int32 StartIdx, const int32 LastIdx)
{
	LandscapeLayerCacheData LocalCache;

	// Reserve space in buckets for a potential instances 
	for (int32 BucketIdx = 0; BucketIdx < NUM_INSTANCE_BUCKETS; ++BucketIdx)
	{
		auto& Bucket = OutPotentialInstances[BucketIdx];
		Bucket.Reserve(DesiredInstances->Num());
	}

	for (int32 InstanceIdx = StartIdx; InstanceIdx <= LastIdx; ++InstanceIdx)
	{
		const FDesiredFoliageInstance& DesiredInst = (*DesiredInstances)[InstanceIdx];
		FHitResult Hit;
		static FName NAME_AddFoliageInstances = FName(TEXT("AddFoliageInstances"));
		if (AInstancedFoliageActor::FoliageTrace(InWorld, Hit, DesiredInst, NAME_AddFoliageInstances, true))
		{
			float HitWeight = 1.f;
			const bool bValidInstance = GeometryFilterCheck(Hit, InWorld, DesiredInst, UISettings)
										&& CheckLocationForPotentialInstance_ThreadSafe(Settings, Hit.ImpactPoint, Hit.ImpactNormal)
										&& VertexMaskCheck(Hit, Settings)
										&& LandscapeLayerCheck(Hit, Settings, LocalCache, HitWeight);

			if (bValidInstance)
			{
				const int32 BucketIndex = FMath::RoundToInt(HitWeight * (float)(NUM_INSTANCE_BUCKETS - 1));
				OutPotentialInstances[BucketIndex].Add(FPotentialInstance(Hit.ImpactPoint, Hit.ImpactNormal, Hit.Component.Get(), HitWeight, DesiredInst));
			}
		}
	}
}

void FEdModeFoliage::CalculatePotentialInstances(const UWorld* InWorld, const UFoliageType* Settings, const TArray<FDesiredFoliageInstance>& DesiredInstances, TArray<FPotentialInstance> OutPotentialInstances[NUM_INSTANCE_BUCKETS], LandscapeLayerCacheData* LandscapeLayerCachesPtr, const FFoliageUISettings* UISettings)
{
	LandscapeLayerCacheData LocalCache;
	LandscapeLayerCachesPtr = LandscapeLayerCachesPtr ? LandscapeLayerCachesPtr : &LocalCache;

	// Quick lookup of potential instance locations, used for overlapping check.
	TArray<FVector> PotentialInstanceLocations;
	FFoliageInstanceHash PotentialInstanceHash(7);	// use 128x128 cell size, things like brush radius are typically small
	PotentialInstanceLocations.Empty(DesiredInstances.Num());

	// Reserve space in buckets for a potential instances 
	for (int32 BucketIdx = 0; BucketIdx < NUM_INSTANCE_BUCKETS; ++BucketIdx)
	{
		auto& Bucket = OutPotentialInstances[BucketIdx];
		Bucket.Reserve(DesiredInstances.Num());
	}

	// Radius where we expect to have a single instance, given the density rules
	const float DensityCheckRadius = FMath::Max<float>(FMath::Sqrt((1000.f*1000.f) / (PI * Settings->Density)), Settings->Radius);

	for (const FDesiredFoliageInstance& DesiredInst : DesiredInstances)
	{
		FHitResult Hit;
		static FName NAME_AddFoliageInstances = FName(TEXT("AddFoliageInstances"));
		if (AInstancedFoliageActor::FoliageTrace(InWorld, Hit, DesiredInst, NAME_AddFoliageInstances, true))
		{
			float HitWeight = 1.f;

			UPrimitiveComponent* InstanceBase = Hit.GetComponent();
			check(InstanceBase);

			ULevel* TargetLevel = InstanceBase->GetComponentLevel();
			// We can paint into new level only if FoliageType is shared
			if (!CanPaint(Settings, TargetLevel))
			{
				continue;
			}

			if (!GeometryFilterCheck(Hit, InWorld, DesiredInst, UISettings))
			{
				continue;
			}
			
			const bool bValidInstance =	CheckLocationForPotentialInstance(InWorld, Settings, DensityCheckRadius, Hit.ImpactPoint, Hit.ImpactNormal, PotentialInstanceLocations, PotentialInstanceHash)
										&& VertexMaskCheck(Hit, Settings)
										&& LandscapeLayerCheck(Hit, Settings, LocalCache, HitWeight);
			if (bValidInstance)
			{
				const int32 BucketIndex = FMath::RoundToInt(HitWeight * (float)(NUM_INSTANCE_BUCKETS - 1));
				OutPotentialInstances[BucketIndex].Add(FPotentialInstance(Hit.ImpactPoint, Hit.ImpactNormal, InstanceBase, HitWeight, DesiredInst));
			}
		}
	}
}

void FEdModeFoliage::AddInstances(UWorld* InWorld, const TArray<FDesiredFoliageInstance>& DesiredInstances)
{
	TMap<const UFoliageType*, TArray<FDesiredFoliageInstance>> SettingsInstancesMap;
	for (const FDesiredFoliageInstance& DesiredInst : DesiredInstances)
	{
		TArray<FDesiredFoliageInstance>& Instances = SettingsInstancesMap.FindOrAdd(DesiredInst.FoliageType);
		Instances.Add(DesiredInst);
	}

	for (auto It = SettingsInstancesMap.CreateConstIterator(); It; ++It)
	{
		const UFoliageType* FoliageType = It.Key();

		const TArray<FDesiredFoliageInstance>& Instances = It.Value();
		AddInstancesImp(InWorld, FoliageType, Instances);
	}
}

static void SpawnFoliageInstance(UWorld* InWorld, const UFoliageType* Settings, const FFoliageInstance& Instance, UActorComponent* BaseComponent)
{
	// We always spawn instances in base component level
	ULevel* TargetLevel = BaseComponent->GetComponentLevel();
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(TargetLevel, true);
	
	FFoliageMeshInfo* MeshInfo;
	UFoliageType* FoliageSettings = IFA->AddFoliageType(Settings, &MeshInfo);

	MeshInfo->AddInstance(IFA, FoliageSettings, Instance, BaseComponent);
}

FFoliageMeshInfo* UpdateMeshSettings(AInstancedFoliageActor* IFA, const FDesiredFoliageInstance& DesiredInstance)
{
	FFoliageMeshInfo* MeshInfo = nullptr;
	//We have to add the mesh into the foliage actor. This assumes the foliage type is just a concrete InstancedStaticMesh.
	//With the new tree system coming this will not be true and you'll need to traverse the tree and insert the needed meshes
	if (const UFoliageType_InstancedStaticMesh* Settings = Cast<UFoliageType_InstancedStaticMesh>(DesiredInstance.FoliageType))
	{
		if (UStaticMesh* StaticMesh = Settings->GetStaticMesh())
		{
			if (UFoliageType* ExistingSettings = IFA->GetSettingsForMesh(StaticMesh))
			{
				MeshInfo = IFA->UpdateMeshSettings(StaticMesh, Settings);
			}
			else
			{
				MeshInfo = IFA->AddMesh(Settings->GetStaticMesh(), nullptr, Settings);
			}
		}
	}

	return MeshInfo;
}

void FEdModeFoliage::AddInstancesImp(UWorld* InWorld, const UFoliageType* Settings, const TArray<FDesiredFoliageInstance>& DesiredInstances, const TArray<int32>& ExistingInstanceBuckets, const float Pressure, LandscapeLayerCacheData* LandscapeLayerCachesPtr, const FFoliageUISettings* UISettings)
{
	if (DesiredInstances.Num() == 0)
	{
		return;
	}

	TArray<FPotentialInstance> PotentialInstanceBuckets[NUM_INSTANCE_BUCKETS];
	if (DesiredInstances[0].PlacementMode == EFoliagePlacementMode::Manual)
	{
		CalculatePotentialInstances(InWorld, Settings, DesiredInstances, PotentialInstanceBuckets, LandscapeLayerCachesPtr, UISettings);
	}
	else
	{
		//@TODO: actual threaded part coming, need parts of this refactor sooner for content team
		CalculatePotentialInstances_ThreadSafe(InWorld, Settings, &DesiredInstances, PotentialInstanceBuckets, nullptr, 0, DesiredInstances.Num() - 1);

		TMap<AInstancedFoliageActor*, FFoliageMeshInfo*> UpdatedIFAs;	//we want to override any existing mesh settings with the procedural settings.
		for (auto& Bucket : PotentialInstanceBuckets)
		{
			for (FPotentialInstance& PotentialInst : Bucket)
			{
				// New foliage instances always go into base component level
				AInstancedFoliageActor* TargetIFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(PotentialInst.HitComponent->GetComponentLevel(), true);
				FFoliageMeshInfo* MeshInfo = nullptr;
				if (FFoliageMeshInfo** MeshInfoResult = UpdatedIFAs.Find(TargetIFA))
				{
					MeshInfo = *MeshInfoResult;
				}
				else
				{
					MeshInfo = UpdateMeshSettings(TargetIFA, PotentialInst.DesiredInstance);
					UpdatedIFAs.Add(TargetIFA, MeshInfo);
				}
			}
		}
	}

	for (int32 BucketIdx = 0; BucketIdx < NUM_INSTANCE_BUCKETS; BucketIdx++)
	{
		TArray<FPotentialInstance>& PotentialInstances = PotentialInstanceBuckets[BucketIdx];
		float BucketFraction = (float)(BucketIdx + 1) / (float)NUM_INSTANCE_BUCKETS;

		// We use the number that actually succeeded in placement (due to parameters) as the target
		// for the number that should be in the brush region.
		const int32 BucketOffset = (ExistingInstanceBuckets.Num() ? ExistingInstanceBuckets[BucketIdx] : 0);
		int32 AdditionalInstances = FMath::Clamp<int32>(FMath::RoundToInt(BucketFraction * (float)(PotentialInstances.Num() - BucketOffset) * Pressure), 0, PotentialInstances.Num());
		for (int32 Idx = 0; Idx < AdditionalInstances; Idx++)
		{
			FPotentialInstance& PotentialInstance = PotentialInstances[Idx];
			FFoliageInstance Inst;
			if (PotentialInstance.PlaceInstance(InWorld, Settings, Inst))
			{
				Inst.ProceduralGuid = PotentialInstance.DesiredInstance.ProceduralGuid;
				
				SpawnFoliageInstance(InWorld, Settings, Inst, PotentialInstance.HitComponent);
			}
		}
	}
}

/** Add instances inside the brush to match DesiredInstanceCount */
void FEdModeFoliage::AddInstancesForBrush(UWorld* InWorld, const UFoliageType* Settings, const FSphere& BrushSphere, int32 DesiredInstanceCount, float Pressure)
{
	UWorld* World = GetWorld();
	const bool bHasValidLandscapeLayers = LandscapeLayersValid(Settings);
		
	TArray<int32> ExistingInstanceBuckets;
	ExistingInstanceBuckets.AddZeroed(NUM_INSTANCE_BUCKETS);
	int32 NumExistingInstances = 0;

	for (FFoliageMeshInfoIterator It(World, Settings); It; ++It)
	{
		FFoliageMeshInfo* MeshInfo = (*It);
		TArray<int32> ExistingInstances;
		MeshInfo->GetInstancesInsideSphere(BrushSphere, ExistingInstances);
		NumExistingInstances+= ExistingInstances.Num();
		
		if (bHasValidLandscapeLayers)
		{
			// Find the landscape weights of existing ExistingInstances
			for (int32 Idx : ExistingInstances)
			{
				FFoliageInstance& Instance = MeshInfo->Instances[Idx];
				auto InstanceBasePtr = It.GetActor()->InstanceBaseCache.GetInstanceBasePtr(Instance.BaseId);
				float HitWeight;
				if (GetMaxHitWeight(Instance.Location, InstanceBasePtr.Get(), Settings, &LandscapeLayerCaches, HitWeight))
				{
					// Add count to bucket.
					ExistingInstanceBuckets[FMath::RoundToInt(HitWeight * (float)(NUM_INSTANCE_BUCKETS - 1))]++;
				}
			}
		}
		else
		{
			// When not tied to a layer, put all the ExistingInstances in the last bucket.
			ExistingInstanceBuckets[NUM_INSTANCE_BUCKETS - 1] = NumExistingInstances;
		}
	}

	if (DesiredInstanceCount > NumExistingInstances)
	{
		TArray<FDesiredFoliageInstance> DesiredInstances;	//we compute instances for the brush
		DesiredInstances.Reserve(DesiredInstanceCount);

		for (int32 DesiredIdx = 0; DesiredIdx < DesiredInstanceCount; DesiredIdx++)
		{
			FVector Start, End;
			GetRandomVectorInBrush(Start, End);
			FDesiredFoliageInstance* DesiredInstance = new (DesiredInstances)FDesiredFoliageInstance(Start, End);
		}

		AddInstancesImp(InWorld, Settings, DesiredInstances, ExistingInstanceBuckets, Pressure, &LandscapeLayerCaches, &UISettings);
	}
}

/** Remove instances inside the brush to match DesiredInstanceCount */
void FEdModeFoliage::RemoveInstancesForBrush(UWorld* InWorld, const UFoliageType* Settings, const FSphere& BrushSphere, int32 DesiredInstanceCount, float Pressure)
{
	for (FFoliageMeshInfoIterator It(InWorld, Settings); It; ++It)
	{
		FFoliageMeshInfo* MeshInfo = (*It);
		AInstancedFoliageActor* IFA = It.GetActor();

		TArray<int32> PotentialInstancesToRemove;
		MeshInfo->GetInstancesInsideSphere(BrushSphere, PotentialInstancesToRemove);
		if (PotentialInstancesToRemove.Num() == 0)
		{
			continue;
		}
		
		int32 InstancesToRemove = FMath::RoundToInt((float)(PotentialInstancesToRemove.Num() - DesiredInstanceCount) * Pressure);
		int32 InstancesToKeep = PotentialInstancesToRemove.Num() - InstancesToRemove;
		if (InstancesToKeep > 0)
		{
			// Remove InstancesToKeep random PotentialInstancesToRemove from the array to leave those PotentialInstancesToRemove behind, and delete all the rest
			for (int32 i = 0; i < InstancesToKeep; i++)
			{
				PotentialInstancesToRemove.RemoveAtSwap(FMath::Rand() % PotentialInstancesToRemove.Num());
			}
		}

		if (!UISettings.bFilterLandscape || !UISettings.bFilterStaticMesh || !UISettings.bFilterBSP || !UISettings.bFilterTranslucent)
		{
			// Filter PotentialInstancesToRemove
			for (int32 Idx = 0; Idx < PotentialInstancesToRemove.Num(); Idx++)
			{
				auto BaseId = MeshInfo->Instances[PotentialInstancesToRemove[Idx]].BaseId;
				auto BasePtr = IFA->InstanceBaseCache.GetInstanceBasePtr(BaseId);
				UPrimitiveComponent* Base = Cast<UPrimitiveComponent>(BasePtr.Get());
				UMaterialInterface* Material = Base ? Base->GetMaterial(0) : nullptr;

				// Check if instance is candidate for removal based on filter settings
				if (Base && (
					(!UISettings.bFilterLandscape && Base->IsA(ULandscapeHeightfieldCollisionComponent::StaticClass())) ||
					(!UISettings.bFilterStaticMesh && Base->IsA(UStaticMeshComponent::StaticClass())) ||
					(!UISettings.bFilterBSP && Base->IsA(UModelComponent::StaticClass())) ||
					(!UISettings.bFilterTranslucent && Material && IsTranslucentBlendMode(Material->GetBlendMode()))
					))
				{
					// Instance should not be removed, so remove it from the removal list.
					PotentialInstancesToRemove.RemoveAtSwap(Idx);
					Idx--;
				}
			}
		}

		// Remove PotentialInstancesToRemove to reduce it to desired count
		if (PotentialInstancesToRemove.Num() > 0)
		{
			MeshInfo->RemoveInstances(IFA, PotentialInstancesToRemove);
		}
	}
}

void FEdModeFoliage::SelectInstancesForBrush(UWorld* InWorld, const UFoliageType* Settings, const FSphere& BrushSphere, bool bSelect)
{
	for (FFoliageMeshInfoIterator It(InWorld, Settings); It; ++It)
	{
		FFoliageMeshInfo* MeshInfo = (*It);
		AInstancedFoliageActor* IFA = It.GetActor();

		TArray<int32> Instances;
		MeshInfo->GetInstancesInsideSphere(BrushSphere, Instances);
		if (Instances.Num() == 0)
		{
			continue;
		}

		MeshInfo->SelectInstances(IFA, bSelect, Instances);
	}
}

void FEdModeFoliage::SelectInstances(UWorld* InWorld, bool bSelect)
{
	for (auto& FoliageMeshUI : FoliageMeshList)
	{	
		UFoliageType* Settings = FoliageMeshUI->Settings;	
		
		if (bSelect && !Settings->IsSelected)
		{
			continue;
		}

		SelectInstances(InWorld, Settings, bSelect);
	}
}
	
void FEdModeFoliage::SelectInstances(UWorld* InWorld, const UFoliageType* Settings, bool bSelect)
{
	for (FFoliageMeshInfoIterator It(InWorld, Settings); It; ++It)
	{
		FFoliageMeshInfo* MeshInfo = (*It);
		AInstancedFoliageActor* IFA = It.GetActor();

		MeshInfo->SelectInstances(IFA, bSelect);
	}
}

void FEdModeFoliage::ApplySelectionToComponents(UWorld* InWorld, bool bApply)
{
	const int32 NumLevels = InWorld->GetNumLevels();
	for (int32 LevelIdx = 0; LevelIdx < NumLevels; ++LevelIdx)
	{
		ULevel* Level = InWorld->GetLevel(LevelIdx);
		AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(Level);
		if (IFA)
		{
			IFA->ApplySelectionToComponents(bApply);
		}
	}
}

void FEdModeFoliage::TransformSelectedInstances(UWorld* InWorld, const FVector& InDrag, const FRotator& InRot, const FVector& InScale, bool bDuplicate)
{
	const int32 NumLevels = InWorld->GetNumLevels();
	for (int32 LevelIdx = 0; LevelIdx < NumLevels; ++LevelIdx)
	{
		ULevel* Level = InWorld->GetLevel(LevelIdx);
		AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(Level);
		if (IFA)
		{
			bool bFoundSelection = false;

			for (auto& MeshPair : IFA->FoliageMeshes)
			{
				FFoliageMeshInfo& MeshInfo = *MeshPair.Value;
				TArray<int32> SelectedIndices = MeshInfo.SelectedIndices.Array();

				if (SelectedIndices.Num() > 0)
				{
					// Mark actor once we found selection
					if (!bFoundSelection)
					{
						IFA->Modify();
						bFoundSelection = true;
					}
					
					if (bDuplicate)
					{
						MeshInfo.DuplicateInstances(IFA, MeshPair.Key, SelectedIndices);
					}

					MeshInfo.PreMoveInstances(IFA, SelectedIndices);

					for (int32 SelectedInstanceIdx : SelectedIndices)
					{
						FFoliageInstance& Instance = MeshInfo.Instances[SelectedInstanceIdx];
						Instance.Location += InDrag;
						Instance.ZOffset = 0.f;
						Instance.Rotation += InRot;
						Instance.DrawScale3D += InScale;
					}

					MeshInfo.PostMoveInstances(IFA, SelectedIndices);
				}
			}

			if (bFoundSelection)
			{
				IFA->MarkComponentsRenderStateDirty();
			}
		}
	}
}

AInstancedFoliageActor* FEdModeFoliage::GetSelectionLocation(UWorld* InWorld, FVector& OutLocation) const
{
	// Prefer current level
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForCurrentLevel(InWorld);
	if (IFA && IFA->GetSelectionLocation(OutLocation))
	{
		return IFA;
	}
	
	// Go through all sub-levels
	const int32 NumLevels = InWorld->GetNumLevels();
	for (int32 LevelIdx = 0; LevelIdx < NumLevels; ++LevelIdx)
	{
		ULevel* Level = InWorld->GetLevel(LevelIdx);
		if (Level != InWorld->GetCurrentLevel())
		{
			AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(Level);
			if (IFA && IFA->GetSelectionLocation(OutLocation))
			{
				return IFA;
			}
		}
	}
	
	return nullptr;
}

void FEdModeFoliage::UpdateWidgetLocationToInstanceSelection()
{
	FVector SelectionLocation = FVector::ZeroVector;
	AInstancedFoliageActor* IFA = GetSelectionLocation(GetWorld(), SelectionLocation);
	Owner->PivotLocation = Owner->SnappedLocation = SelectionLocation;
	//if (IFA)
	//{
	//	IFA->MarkComponentsRenderStateDirty();
	//}
}

void FEdModeFoliage::RemoveSelectedInstances(UWorld* InWorld)
{
	GEditor->BeginTransaction(NSLOCTEXT("UnrealEd", "FoliageMode_EditTransaction", "Foliage Editing"));
	
	const int32 NumLevels = InWorld->GetNumLevels();
	for (int32 LevelIdx = 0; LevelIdx < NumLevels; ++LevelIdx)
	{
		ULevel* Level = InWorld->GetLevel(LevelIdx);
		AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(Level);
		if (IFA)
		{
			bool bHasSelection = false;
			for (auto& MeshPair : IFA->FoliageMeshes)
			{
				if (MeshPair.Value->SelectedIndices.Num())
				{
					bHasSelection = true;
					break;
				}
			}

			if (bHasSelection)
			{
				IFA->Modify();
				for (auto& MeshPair : IFA->FoliageMeshes)
				{
					FFoliageMeshInfo& Mesh = *MeshPair.Value;
					if (Mesh.SelectedIndices.Num() > 0)
					{
						TArray<int32> InstancesToDelete = Mesh.SelectedIndices.Array();
						Mesh.RemoveInstances(IFA, InstancesToDelete);
					
						OnInstanceCountUpdated(MeshPair.Key);
					}
				}
			}
		}
	}

	GEditor->EndTransaction();
}

void FEdModeFoliage::ReapplyInstancesForBrush(UWorld* InWorld, const UFoliageType* Settings, const FSphere& BrushSphere)
{
	for (FFoliageMeshInfoIterator It(InWorld, Settings); It; ++It)
	{
		FFoliageMeshInfo* MeshInfo = (*It);
		AInstancedFoliageActor* IFA = It.GetActor();

		ReapplyInstancesForBrush(InWorld, IFA, Settings, MeshInfo, BrushSphere);
	}
}

/** Reapply instance settings to exiting instances */
void FEdModeFoliage::ReapplyInstancesForBrush(UWorld* InWorld, AInstancedFoliageActor* IFA, const UFoliageType* Settings, FFoliageMeshInfo* MeshInfo, const FSphere& BrushSphere)
{
	TArray<int32> ExistingInstances;
	MeshInfo->GetInstancesInsideSphere(BrushSphere, ExistingInstances);
		
	bool bUpdated = false;
	TArray<int32> UpdatedInstances;
	TSet<int32> InstancesToDelete;

	IFA->Modify();

	for (int32 Idx = 0; Idx < ExistingInstances.Num(); Idx++)
	{
		int32 InstanceIndex = ExistingInstances[Idx];
		FFoliageInstance& Instance = MeshInfo->Instances[InstanceIndex];

		if ((Instance.Flags & FOLIAGE_Readjusted) == 0)
		{
			// record that we've made changes to this instance already, so we don't touch it again.
			Instance.Flags |= FOLIAGE_Readjusted;

			// See if we need to update the location in the instance hash
			bool bReapplyLocation = false;
			FVector OldInstanceLocation = Instance.Location;

			// remove any Z offset first, so the offset is reapplied to any new 
			if (FMath::Abs(Instance.ZOffset) > KINDA_SMALL_NUMBER)
			{
				Instance.Location = Instance.GetInstanceWorldTransform().TransformPosition(FVector(0, 0, -Instance.ZOffset));
				bReapplyLocation = true;
			}


			// Defer normal reapplication 
			bool bReapplyNormal = false;

			// Reapply normal alignment
			if (Settings->ReapplyAlignToNormal)
			{
				if (Settings->AlignToNormal)
				{
					if ((Instance.Flags & FOLIAGE_AlignToNormal) == 0)
					{
						bReapplyNormal = true;
						bUpdated = true;
					}
				}
				else
				{
					if (Instance.Flags & FOLIAGE_AlignToNormal)
					{
						Instance.Rotation = Instance.PreAlignRotation;
						Instance.Flags &= (~FOLIAGE_AlignToNormal);
						bUpdated = true;
					}
				}
			}

			// Reapply random yaw
			if (Settings->ReapplyRandomYaw)
			{
				if (Settings->RandomYaw)
				{
					if (Instance.Flags & FOLIAGE_NoRandomYaw)
					{
						// See if we need to remove any normal alignment first
						if (!bReapplyNormal && (Instance.Flags & FOLIAGE_AlignToNormal))
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
					if ((Instance.Flags & FOLIAGE_NoRandomYaw) == 0)
					{
						// See if we need to remove any normal alignment first
						if (!bReapplyNormal && (Instance.Flags & FOLIAGE_AlignToNormal))
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
			if (Settings->ReapplyRandomPitchAngle)
			{
				// See if we need to remove any normal alignment first
				if (!bReapplyNormal && (Instance.Flags & FOLIAGE_AlignToNormal))
				{
					Instance.Rotation = Instance.PreAlignRotation;
					bReapplyNormal = true;
				}

				Instance.Rotation.Pitch = FMath::FRand() * Settings->RandomPitchAngle;
				Instance.Flags |= FOLIAGE_NoRandomYaw;

				bUpdated = true;
			}

			// Reapply scale
			FVector NewScale = Settings->GetRandomScale();
			
			if (Settings->ReapplyScaleX)
			{
				if (Settings->Scaling == EFoliageScaling::Uniform)
				{
					Instance.DrawScale3D = NewScale;
				}
				else
				{
					Instance.DrawScale3D.X = NewScale.X;
				}
				bUpdated = true;
			}

			if (Settings->ReapplyScaleY)
			{
				Instance.DrawScale3D.Y = NewScale.Y;
				bUpdated = true;
			}

			if (Settings->ReapplyScaleY)
			{
				Instance.DrawScale3D.Z = NewScale.Z;
				bUpdated = true;
			}

			// Reapply ZOffset
			if (Settings->ReapplyZOffset)
			{
				Instance.ZOffset = Settings->ZOffset.Interpolate(FMath::FRand());
				bUpdated = true;
			}

			// Find a ground normal for either normal or ground slope check.
			if (bReapplyNormal || Settings->ReapplyGroundSlope || Settings->ReapplyVertexColorMask || (Settings->ReapplyLandscapeLayer && LandscapeLayersValid(Settings)))
			{
				FHitResult Hit;
				static const FName NAME_ReapplyInstancesForBrush = TEXT("ReapplyInstancesForBrush");

				// trace along the mesh's Z axis.
				FVector ZAxis = Instance.Rotation.Quaternion().GetAxisZ();
				FVector Start = Instance.Location + 16.f * ZAxis;
				FVector End = Instance.Location - 16.f * ZAxis;
				if (AInstancedFoliageActor::FoliageTrace(InWorld, Hit, FDesiredFoliageInstance(Start, End), NAME_ReapplyInstancesForBrush, true))
				{
					// Reapply the normal
					if (bReapplyNormal)
					{
						Instance.PreAlignRotation = Instance.Rotation;
						Instance.AlignToNormal(Hit.Normal, Settings->AlignMaxAngle);
					}

					// Cull instances that don't meet the ground slope check.
					if (Settings->ReapplyGroundSlope)
					{
						const float MaxNormalAngle = FMath::Cos(FMath::DegreesToRadians(Settings->GroundSlopeAngle.Max));
						const float MinNormalAngle = FMath::Cos(FMath::DegreesToRadians(Settings->GroundSlopeAngle.Min));
						if (MaxNormalAngle > Hit.Normal.Z || MinNormalAngle < Hit.Normal.Z)
						{
							InstancesToDelete.Add(InstanceIndex);
							if (bReapplyLocation)
							{
								// restore the location so the hash removal will succeed
								Instance.Location = OldInstanceLocation;
							}
							continue;
						}
					}

					// Cull instances for the landscape layer
					if (Settings->ReapplyLandscapeLayer && LandscapeLayersValid(Settings))
					{
						float HitWeight = 1.f;
						if (GetMaxHitWeight(Hit.Location, Hit.GetComponent(), Settings, &LandscapeLayerCaches, HitWeight))
						{
							if (FilterByWeight(HitWeight, Settings))
							{
								InstancesToDelete.Add(InstanceIndex);
								if (bReapplyLocation)
								{
									// restore the location so the hash removal will succeed
									Instance.Location = OldInstanceLocation;
								}
								continue;
							}
						}
					}

					// Reapply vertex color mask
					if (Settings->ReapplyVertexColorMask)
					{
						if (Settings->VertexColorMask != FOLIAGEVERTEXCOLORMASK_Disabled && Hit.FaceIndex != INDEX_NONE)
						{
							UStaticMeshComponent* HitStaticMeshComponent = Cast<UStaticMeshComponent>(Hit.Component.Get());
							if (HitStaticMeshComponent)
							{
								FColor VertexColor;
								if (GetStaticMeshVertexColorForHit(HitStaticMeshComponent, Hit.FaceIndex, Hit.Location, VertexColor))
								{
									if (!CheckVertexColor(Settings, VertexColor))
									{
										InstancesToDelete.Add(InstanceIndex);
										if (bReapplyLocation)
										{
											// restore the location so the hash removal will succeed
											Instance.Location = OldInstanceLocation;
										}
										continue;
									}
								}
							}
						}
					}
				}
			}

			// Cull instances that don't meet the height range
			if (Settings->ReapplyHeight)
			{
				if (!Settings->Height.Contains(Instance.Location.Z))
				{
					InstancesToDelete.Add(InstanceIndex);
					if (bReapplyLocation)
					{
						// restore the location so the hash removal will succeed
						Instance.Location = OldInstanceLocation;
					}
					continue;
				}
			}

			if (bUpdated && FMath::Abs(Instance.ZOffset) > KINDA_SMALL_NUMBER)
			{
				// Reapply the Z offset in new local space
				Instance.Location = Instance.GetInstanceWorldTransform().TransformPosition(FVector(0, 0, Instance.ZOffset));
				bReapplyLocation = true;
			}

			// Update the hash
			if (bReapplyLocation)
			{
				MeshInfo->InstanceHash->RemoveInstance(OldInstanceLocation, InstanceIndex);
				MeshInfo->InstanceHash->InsertInstance(Instance.Location, InstanceIndex);
			}

			// Cull overlapping based on radius
			if (Settings->ReapplyRadius && Settings->Radius > 0.f)
			{
				if (MeshInfo->CheckForOverlappingInstanceExcluding(InstanceIndex, Settings->Radius, InstancesToDelete))
				{
					InstancesToDelete.Add(InstanceIndex);
					continue;
				}
			}

			// Remove mesh collide with world
			if (Settings->ReapplyCollisionWithWorld)
			{
				FHitResult Hit;
				static const FName NAME_ReapplyInstancesForBrush = TEXT("ReapplyCollisionWithWorld");
				FVector Start = Instance.Location + FVector(0.f, 0.f, 16.f);
				FVector End = Instance.Location - FVector(0.f, 0.f, 16.f);
				if (AInstancedFoliageActor::FoliageTrace(InWorld, Hit, FDesiredFoliageInstance(Start, End), NAME_ReapplyInstancesForBrush))
				{
					if (!AInstancedFoliageActor::CheckCollisionWithWorld(InWorld, Settings, Instance, Hit.Normal, Hit.Location))
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

			if (bUpdated)
			{
				UpdatedInstances.Add(InstanceIndex);
			}
		}
	}

	if (UpdatedInstances.Num() > 0)
	{
		MeshInfo->PostUpdateInstances(IFA, UpdatedInstances);
		IFA->RegisterAllComponents();
	}

	if (InstancesToDelete.Num())
	{
		MeshInfo->RemoveInstances(IFA, InstancesToDelete.Array());
	}
}

void FEdModeFoliage::ReapplyInstancesDensityForBrush(UWorld* InWorld, const UFoliageType* Settings, const FSphere& BrushSphere, float Pressure)
{
	// TODO: make it work with seamless painting
	
	//// Adjust instance density
	//int32 SnapShotInstanceCount = 0;
	//TArray<FMeshInfoSnapshot*> SnapShotList;
	//InstanceSnapshot.MultiFindPointer(Settings, SnapShotList);

	//if (SnapShotList.Num())
	//{
	//	// Use snapshot to determine number of instances at the start of the brush stroke
	//	for (auto* SnapShot : SnapShotList)
	//	{
	//		SnapShotInstanceCount+= SnapShot->CountInstancesInsideSphere(BrushSphere);
	//	}
	//			
	//	int32 NewInstanceCount = FMath::RoundToInt((float)SnapShotInstanceCount * Settings->ReapplyDensityAmount);
	//	if (Settings->ReapplyDensityAmount > 1.f && NewInstanceCount > Instances.Num())
	//	{
	//		AddInstancesForBrush(InWorld, Settings, BrushSphere, );
	//	}
	//	else if (Settings->ReapplyDensityAmount < 1.f && NewInstanceCount < Instances.Num())
	//	{
	//		RemoveInstancesForBrush(IFA, *MeshInfo, NewInstanceCount, Instances, Pressure);
	//	}
	//}
}

void FEdModeFoliage::PreApplyBrush()
{
	InstanceSnapshot.Empty();
	
	UWorld* World = GetWorld();
	// Special setup beginning a stroke with the Reapply tool
	// Necessary so we don't keep reapplying settings over and over for the same instances.
	if (UISettings.GetReapplyToolSelected())
	{
		for (auto& FoliageMeshUI : FoliageMeshList)
		{	
			UFoliageType* Settings = FoliageMeshUI->Settings;	
		
			if (!Settings->IsSelected)
			{
				continue;
			}

			for (FFoliageMeshInfoIterator It(World, Settings); It; ++It)
			{
				FFoliageMeshInfo* MeshInfo = (*It);
				
				// Take a snapshot of all the locations
				InstanceSnapshot.Add(Settings, FMeshInfoSnapshot(MeshInfo));

				// Clear the "FOLIAGE_Readjusted" flag
				for (auto& Instance : MeshInfo->Instances)
				{
					Instance.Flags &= (~FOLIAGE_Readjusted);
				}
			}
		}
	}
}

void FEdModeFoliage::ApplyBrush(FEditorViewportClient* ViewportClient)
{
	if (!bBrushTraceValid || ViewportClient != GCurrentLevelEditingViewportClient)
	{
		return;
	}
	
	float BrushArea = PI * FMath::Square(UISettings.GetRadius());

	// Tablet pressure
	float Pressure = ViewportClient->Viewport->IsPenActive() ? ViewportClient->Viewport->GetTabletPressure() : 1.f;

	// Cache a copy of the world pointer
	UWorld* World = ViewportClient->GetWorld();
		
	for (auto& FoliageMeshUI : FoliageMeshList)
	{	
		UFoliageType* Settings = FoliageMeshUI->Settings;	
		
		if (!Settings->IsSelected)
		{
			continue;
		}

		FSphere BrushSphere(BrushLocation, UISettings.GetRadius());
					
		if (UISettings.GetLassoSelectToolSelected())
		{
			// Shift unpaints
			SelectInstancesForBrush(World, Settings, BrushSphere, !IsShiftDown(ViewportClient->Viewport));
		}
		else if (UISettings.GetReapplyToolSelected())
		{
			// Reapply any settings checked by the user
			ReapplyInstancesForBrush(World, Settings, BrushSphere);
		}
		else if (UISettings.GetPaintToolSelected())
		{
			// Shift unpaints
			if (IsShiftDown(ViewportClient->Viewport))
			{
				int32 DesiredInstanceCount = FMath::RoundToInt(BrushArea * Settings->Density * UISettings.GetUnpaintDensity() / (1000.f*1000.f));
					
				RemoveInstancesForBrush(World, Settings, BrushSphere, DesiredInstanceCount, Pressure);
			}
			else
			{
				// This is the total set of instances disregarding parameters like slope, height or layer.
				float DesiredInstanceCountFloat = BrushArea * Settings->Density * UISettings.GetPaintDensity() / (1000.f*1000.f);
				// Allow a single instance with a random chance, if the brush is smaller than the density
				int32 DesiredInstanceCount = DesiredInstanceCountFloat > 1.f ? FMath::RoundToInt(DesiredInstanceCountFloat) : FMath::FRand() < DesiredInstanceCountFloat ? 1 : 0;
					
				AddInstancesForBrush(World, Settings, BrushSphere, DesiredInstanceCount, Pressure);
			}
		}
		
		OnInstanceCountUpdated(Settings);
	}

	if (UISettings.GetLassoSelectToolSelected())
	{
		UpdateWidgetLocationToInstanceSelection();
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
		if (WorldNormalSize > SMALL_NUMBER)
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
		if (x + y > 1.f)
		{
			x = 1.f - x;
			y = 1.f - y;
		}

		OutBaryVertexColor = (1.f - x - y) * VertexColor[0] + x * VertexColor[1] + y * VertexColor[2];
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
void FEdModeFoliage::ApplyPaintBucket_Remove(AActor* Actor)
{
	UWorld* World = Actor->GetWorld();

	TInlineComponentArray<UActorComponent*> Components;
	Actor->GetComponents(Components);

	// Remove all instances of the selected meshes
	for (const auto& MeshUIInfo : FoliageMeshList)
	{
		UFoliageType* FoliageType = MeshUIInfo->Settings;
		if (!FoliageType->IsSelected)
		{
			continue;
		}

		// Go through all FoliageActors in the world and delete 
		for (FFoliageMeshInfoIterator It(World, FoliageType); It; ++It)
		{
			AInstancedFoliageActor* IFA = It.GetActor();
			
			for (auto Component : Components)
			{
				IFA->DeleteInstancesForComponent(Component, FoliageType);
			}
		}

		OnInstanceCountUpdated(FoliageType);
	}
}

/** Apply paint bucket to actor */
void FEdModeFoliage::ApplyPaintBucket_Add(AActor* Actor)
{
	UWorld* World = Actor->GetWorld();
	TMap<UPrimitiveComponent*, TArray<FFoliagePaintBucketTriangle> > ComponentPotentialTriangles;

	// Check all the components of the hit actor
	TInlineComponentArray<UStaticMeshComponent*> StaticMeshComponents;
	Actor->GetComponents(StaticMeshComponents);

	for (auto StaticMeshComponent : StaticMeshComponents)
	{
		UMaterialInterface* Material = StaticMeshComponent->GetMaterial(0);

		if (UISettings.bFilterStaticMesh && StaticMeshComponent->StaticMesh && StaticMeshComponent->StaticMesh->RenderData &&
			(UISettings.bFilterTranslucent || !Material || !IsTranslucentBlendMode(Material->GetBlendMode())))
		{
			UStaticMesh* StaticMesh = StaticMeshComponent->StaticMesh;
			FStaticMeshLODResources& LODModel = StaticMesh->RenderData->LODResources[0];
			TArray<FFoliagePaintBucketTriangle>& PotentialTriangles = ComponentPotentialTriangles.Add(StaticMeshComponent, TArray<FFoliagePaintBucketTriangle>());

			bool bHasInstancedColorData = false;
			const FStaticMeshComponentLODInfo* InstanceMeshLODInfo = nullptr;
			if (StaticMeshComponent->LODData.Num() > 0)
			{
				InstanceMeshLODInfo = StaticMeshComponent->LODData.GetData();
				bHasInstancedColorData = InstanceMeshLODInfo->PaintedVertices.Num() == LODModel.VertexBuffer.GetNumVertices();
			}

			const bool bHasColorData = bHasInstancedColorData || LODModel.ColorVertexBuffer.GetNumVertices();

			// Get the raw triangle data for this static mesh
			FTransform LocalToWorld = StaticMeshComponent->ComponentToWorld;
			FIndexArrayView Indices = LODModel.IndexBuffer.GetArrayView();
			const FPositionVertexBuffer& PositionVertexBuffer = LODModel.PositionVertexBuffer;
			const FColorVertexBuffer& ColorVertexBuffer = LODModel.ColorVertexBuffer;

			if (USplineMeshComponent* SplineMesh = Cast<USplineMeshComponent>(StaticMeshComponent))
			{
				// Transform spline mesh verts correctly
				FVector Mask = FVector(1, 1, 1);
				USplineMeshComponent::GetAxisValue(Mask, SplineMesh->ForwardAxis) = 0;

				for (int32 Idx = 0; Idx < Indices.Num(); Idx += 3)
				{
					const int32 Index0 = Indices[Idx];
					const int32 Index1 = Indices[Idx + 1];
					const int32 Index2 = Indices[Idx + 2];

					const FVector Vert0 = SplineMesh->CalcSliceTransform(USplineMeshComponent::GetAxisValue(PositionVertexBuffer.VertexPosition(Index0), SplineMesh->ForwardAxis)).TransformPosition(PositionVertexBuffer.VertexPosition(Index0) * Mask);
					const FVector Vert1 = SplineMesh->CalcSliceTransform(USplineMeshComponent::GetAxisValue(PositionVertexBuffer.VertexPosition(Index1), SplineMesh->ForwardAxis)).TransformPosition(PositionVertexBuffer.VertexPosition(Index1) * Mask);
					const FVector Vert2 = SplineMesh->CalcSliceTransform(USplineMeshComponent::GetAxisValue(PositionVertexBuffer.VertexPosition(Index2), SplineMesh->ForwardAxis)).TransformPosition(PositionVertexBuffer.VertexPosition(Index2) * Mask);

					new(PotentialTriangles)FFoliagePaintBucketTriangle(LocalToWorld
						, Vert0
						, Vert1
						, Vert2
						, bHasInstancedColorData ? InstanceMeshLODInfo->PaintedVertices[Index0].Color : (bHasColorData ? ColorVertexBuffer.VertexColor(Index0) : FColor::White)
						, bHasInstancedColorData ? InstanceMeshLODInfo->PaintedVertices[Index1].Color : (bHasColorData ? ColorVertexBuffer.VertexColor(Index1) : FColor::White)
						, bHasInstancedColorData ? InstanceMeshLODInfo->PaintedVertices[Index2].Color : (bHasColorData ? ColorVertexBuffer.VertexColor(Index2) : FColor::White)
						);
				}
			}
			else
			{
				// Build a mapping of vertex positions to vertex colors.  Using a TMap will allow for fast lookups so we can match new static mesh vertices with existing colors 
				for (int32 Idx = 0; Idx < Indices.Num(); Idx += 3)
				{
					const int32 Index0 = Indices[Idx];
					const int32 Index1 = Indices[Idx + 1];
					const int32 Index2 = Indices[Idx + 2];

					new(PotentialTriangles)FFoliagePaintBucketTriangle(LocalToWorld
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
	}

	for (const auto& MeshUIInfo : FoliageMeshList)
	{
		UFoliageType* Settings = MeshUIInfo->Settings;
		if (!Settings->IsSelected)
		{
			continue;
		}

		// Quick lookup of potential instance locations, used for overlapping check.
		TArray<FVector> PotentialInstanceLocations;
		FFoliageInstanceHash PotentialInstanceHash(7);	// use 128x128 cell size, as the brush radius is typically small.
		TArray<FPotentialInstance> InstancesToPlace;

		// Radius where we expect to have a single instance, given the density rules
		const float DensityCheckRadius = FMath::Max<float>(FMath::Sqrt((1000.f*1000.f) / (PI * Settings->Density * UISettings.GetPaintDensity())), Settings->Radius);

		for (TMap<UPrimitiveComponent*, TArray<FFoliagePaintBucketTriangle> >::TIterator ComponentIt(ComponentPotentialTriangles); ComponentIt; ++ComponentIt)
		{
			UPrimitiveComponent* Component = ComponentIt.Key();
			TArray<FFoliagePaintBucketTriangle>& PotentialTriangles = ComponentIt.Value();

			for (int32 TriIdx = 0; TriIdx<PotentialTriangles.Num(); TriIdx++)
			{
				FFoliagePaintBucketTriangle& Triangle = PotentialTriangles[TriIdx];

				// Check if we can reject this triangle based on normal.
				const float MaxNormalAngle = FMath::Cos(FMath::DegreesToRadians(Settings->GroundSlopeAngle.Max));
				const float MinNormalAngle = FMath::Cos(FMath::DegreesToRadians(Settings->GroundSlopeAngle.Min));
				if (Triangle.WorldNormal.Z < MaxNormalAngle || Triangle.WorldNormal.Z > MinNormalAngle)
				{
					continue;
				}

				// This is the total set of instances disregarding parameters like slope, height or layer.
				float DesiredInstanceCountFloat = Triangle.Area * Settings->Density * UISettings.GetPaintDensity() / (1000.f*1000.f);

				// Allow a single instance with a random chance, if the brush is smaller than the density
				int32 DesiredInstanceCount = DesiredInstanceCountFloat > 1.f ? FMath::RoundToInt(DesiredInstanceCountFloat) : FMath::FRand() < DesiredInstanceCountFloat ? 1 : 0;

				for (int32 Idx = 0; Idx < DesiredInstanceCount; Idx++)
				{
					FVector InstLocation;
					FColor VertexColor;
					Triangle.GetRandomPoint(InstLocation, VertexColor);

					// Check color mask and filters at this location
					if (!CheckVertexColor(Settings, VertexColor) ||
						!CheckLocationForPotentialInstance(World, Settings, DensityCheckRadius, InstLocation, Triangle.WorldNormal, PotentialInstanceLocations, PotentialInstanceHash))
					{
						continue;
					}

					new(InstancesToPlace)FPotentialInstance(InstLocation, Triangle.WorldNormal, Component, 1.f);
				}
			}
		}
		
		// Place instances
		for (FPotentialInstance& PotentialInstance : InstancesToPlace)
		{
			FFoliageInstance Inst;
			if (PotentialInstance.PlaceInstance(World, Settings, Inst))
			{
				SpawnFoliageInstance(World, Settings, Inst, PotentialInstance.HitComponent);
			}
		}

		//
		OnInstanceCountUpdated(Settings);
	}
}

bool FEdModeFoliage::GetStaticMeshVertexColorForHit(const UStaticMeshComponent* InStaticMeshComponent, int32 InTriangleIndex, const FVector& InHitLocation, FColor& OutVertexColor)
{
	const UStaticMesh* StaticMesh = InStaticMeshComponent->StaticMesh;

	if (StaticMesh == nullptr || StaticMesh->RenderData == nullptr)
	{
		return false;
	}

	const FStaticMeshLODResources& LODModel = StaticMesh->RenderData->LODResources[0];
	bool bHasInstancedColorData = false;
	const FStaticMeshComponentLODInfo* InstanceMeshLODInfo = nullptr;
	if (InStaticMeshComponent->LODData.Num() > 0)
	{
		InstanceMeshLODInfo = InStaticMeshComponent->LODData.GetData();
		bHasInstancedColorData = InstanceMeshLODInfo->PaintedVertices.Num() == LODModel.VertexBuffer.GetNumVertices();
	}

	const FColorVertexBuffer& ColorVertexBuffer = LODModel.ColorVertexBuffer;

	// no vertex color data
	if (!bHasInstancedColorData && ColorVertexBuffer.GetNumVertices() == 0)
	{
		return false;
	}

	// Get the raw triangle data for this static mesh
	FIndexArrayView Indices = LODModel.IndexBuffer.GetArrayView();
	const FPositionVertexBuffer& PositionVertexBuffer = LODModel.PositionVertexBuffer;

	int32 SectionFirstTriIndex = 0;
	for (TArray<FStaticMeshSection>::TConstIterator SectionIt(LODModel.Sections); SectionIt; ++SectionIt)
	{
		const FStaticMeshSection& Section = *SectionIt;

		if (Section.bEnableCollision)
		{
			int32 NextSectionTriIndex = SectionFirstTriIndex + Section.NumTriangles;
			if (InTriangleIndex >= SectionFirstTriIndex && InTriangleIndex < NextSectionTriIndex)
			{

				int32 IndexBufferIdx = (InTriangleIndex - SectionFirstTriIndex) * 3 + Section.FirstIndex;

				// Look up the triangle vertex indices
				int32 Index0 = Indices[IndexBufferIdx];
				int32 Index1 = Indices[IndexBufferIdx + 1];
				int32 Index2 = Indices[IndexBufferIdx + 2];

				// Lookup the triangle world positions and colors.
				FVector WorldVert0 = InStaticMeshComponent->ComponentToWorld.TransformPosition(PositionVertexBuffer.VertexPosition(Index0));
				FVector WorldVert1 = InStaticMeshComponent->ComponentToWorld.TransformPosition(PositionVertexBuffer.VertexPosition(Index1));
				FVector WorldVert2 = InStaticMeshComponent->ComponentToWorld.TransformPosition(PositionVertexBuffer.VertexPosition(Index2));

				FLinearColor Color0;
				FLinearColor Color1;
				FLinearColor Color2;

				if (bHasInstancedColorData)
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

void FEdModeFoliage::SnapSelectedInstancesToGround(UWorld* InWorld)
{
	bool bMovedInstance = false;
	
	const int32 NumLevels = InWorld->GetNumLevels();
	for (int32 LevelIdx = 0; LevelIdx < NumLevels; ++LevelIdx)
	{
		ULevel* Level = InWorld->GetLevel(LevelIdx);
		AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(Level);
		if (IFA)
		{
			bool bFoundSelection = false;

			for (auto& MeshPair : IFA->FoliageMeshes)
			{
				FFoliageMeshInfo& MeshInfo = *MeshPair.Value;
				TArray<int32> SelectedIndices = MeshInfo.SelectedIndices.Array();

				if (SelectedIndices.Num() > 0)
				{
					// Mark actor once we found selection
					if (!bFoundSelection)
					{
						IFA->Modify();
						bFoundSelection = true;
					}

					MeshInfo.PreMoveInstances(IFA, SelectedIndices);

					for (int32 InstanceIndex : SelectedIndices)
					{
						bMovedInstance|= SnapInstanceToGround(IFA, MeshPair.Key->AlignMaxAngle, MeshInfo, InstanceIndex);
					}

					MeshInfo.PostMoveInstances(IFA, SelectedIndices);
				}
			}
		}
	}

	if (bMovedInstance)
	{
		UpdateWidgetLocationToInstanceSelection();
	}
}

bool FEdModeFoliage::SnapInstanceToGround(AInstancedFoliageActor* InIFA, float AlignMaxAngle, FFoliageMeshInfo& Mesh, int32 InstanceIdx)
{
	FFoliageInstance& Instance = Mesh.Instances[InstanceIdx];
	FVector Start = Instance.Location;
	FVector End = Instance.Location - FVector(0.f, 0.f, FOLIAGE_SNAP_TRACE);

	FHitResult Hit;
	static FName NAME_FoliageSnap = FName("FoliageSnap");
	if (AInstancedFoliageActor::FoliageTrace(InIFA->GetWorld(), Hit, FDesiredFoliageInstance(Start, End), NAME_FoliageSnap))
	{
		UPrimitiveComponent* HitComponent = Hit.Component.Get();

		// We cannot be based on an a blueprint component as these will disappear when the construction script is re-run
		if (HitComponent->IsCreatedByConstructionScript())
		{
			HitComponent = nullptr;
		}
																
		// Find BSP brush 
		UModelComponent* ModelComponent = Cast<UModelComponent>(HitComponent);
		if (ModelComponent)
		{
			ABrush* BrushActor = ModelComponent->GetModel()->FindBrush(Hit.Location);
			if (BrushActor)
			{
				HitComponent = BrushActor->GetBrushComponent();
			}
		}

		// Set new base
		auto NewBaseId = InIFA->InstanceBaseCache.AddInstanceBaseId(HitComponent);
		Mesh.RemoveFromBaseHash(InstanceIdx);
		Instance.BaseId = NewBaseId;
		Mesh.AddToBaseHash(InstanceIdx);
		Instance.Location = Hit.Location;
		Instance.ZOffset = 0.f;
													
		if (Instance.Flags & FOLIAGE_AlignToNormal)
		{
			// Remove previous alignment and align to new normal.
			Instance.Rotation = Instance.PreAlignRotation;
			Instance.AlignToNormal(Hit.Normal, AlignMaxAngle);
		}

		return true;
	}

	return false;
}

TArray<FFoliageMeshUIInfoPtr>& FEdModeFoliage::GetFoliageMeshList()
{
	return FoliageMeshList;
}

void FEdModeFoliage::PopulateFoliageMeshList()
{
	FoliageMeshList.Empty();
	
	// Collect set of all available Settings
	UWorld* World = GetWorld();
	ULevel* CurrentLevel = World->GetCurrentLevel();
	const int32 NumLevels = World->GetNumLevels();
	
	for (int32 LevelIdx = 0; LevelIdx < NumLevels; ++LevelIdx)
	{
		ULevel* Level = World->GetLevel(LevelIdx);
		if (Level && Level->bIsVisible)
		{
			AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(Level);
			if (IFA)
			{
				for (auto& MeshPair : IFA->FoliageMeshes)
				{
					if (!CanPaint(MeshPair.Key, CurrentLevel))
					{
						continue;
					}
										
					int32 ElementIdx = FoliageMeshList.IndexOfByPredicate([&](const FFoliageMeshUIInfoPtr& Item) {
						return Item->Settings == MeshPair.Key;
					});

					if (ElementIdx == INDEX_NONE)
					{
						ElementIdx = FoliageMeshList.Add(MakeShareable(new FFoliageMeshUIInfo(MeshPair.Key)));
					}

					FoliageMeshList[ElementIdx]->InstanceCountTotal+= MeshPair.Value->GetInstanceCount();
					if (Level == World->GetCurrentLevel())
					{
						FoliageMeshList[ElementIdx]->InstanceCountCurrentLevel+= MeshPair.Value->GetInstanceCount();
					}
				}
			}
		}
	}

	auto CompareDisplayOrder = [](const FFoliageMeshUIInfoPtr& A, const FFoliageMeshUIInfoPtr& B)
	{
		return A->Settings->DisplayOrder < B->Settings->DisplayOrder;
	};

	FoliageMeshList.Sort(CompareDisplayOrder);

	StaticCastSharedPtr<FFoliageEdModeToolkit>(Toolkit)->RefreshFullList();
}

void FEdModeFoliage::OnInstanceCountUpdated(const UFoliageType* FoliageType)
{
	int32 EntryIndex = FoliageMeshList.IndexOfByPredicate([&](const FFoliageMeshUIInfoPtr& UIInfoPtr) {
		return UIInfoPtr->Settings == FoliageType;
	});
	
	if (EntryIndex == INDEX_NONE)
	{
		return;
	}
	
	int32 InstanceCountTotal = 0;
	int32 InstanceCountCurrentLevel = 0;
	UWorld* World = GetWorld();
	ULevel* CurrentLevel = World->GetCurrentLevel();

	for (FFoliageMeshInfoIterator It(World, FoliageType); It; ++It)
	{
		FFoliageMeshInfo* MeshInfo = (*It);
		InstanceCountTotal+= MeshInfo->Instances.Num();
		if (It.GetActor()->GetLevel() == CurrentLevel)
		{
			InstanceCountCurrentLevel = MeshInfo->Instances.Num();
		}
	}
	
	//
	FoliageMeshList[EntryIndex]->InstanceCountTotal = InstanceCountTotal;
	FoliageMeshList[EntryIndex]->InstanceCountCurrentLevel = InstanceCountCurrentLevel;
}

bool FEdModeFoliage::CanPaint(const ULevel* InLevel)
{
	for (const auto& MeshUIPtr : FoliageMeshList)
	{
		if (MeshUIPtr->Settings->IsSelected && CanPaint(MeshUIPtr->Settings, InLevel))
		{
			return true;
		}
	}

	return false;
}

bool FEdModeFoliage::CanPaint(const UFoliageType* FoliageType, const ULevel* InLevel)
{
	// Non-shared objects can be painted only into their own level
	// Assets can be painted everywhere
	if (FoliageType->IsAsset() || FoliageType->GetOutermost() == InLevel->GetOutermost())
	{
		return true;
	}
	
	return false;
}

UFoliageType* FEdModeFoliage::AddFoliageAsset(UObject* InAssset)
{
	UFoliageType* FoliageType = nullptr;
	
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(InAssset);
	if (StaticMesh)
	{
		AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForCurrentLevel(GetWorld(), true);
		FoliageType = IFA->GetSettingsForMesh(StaticMesh);
		if (!FoliageType)
		{
			IFA->AddMesh(StaticMesh, &FoliageType);
		}
	}
	else 
	{
		FoliageType = Cast<UFoliageType>(InAssset);
		if (FoliageType)
		{
			AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForCurrentLevel(GetWorld(), true);
			FoliageType = IFA->AddFoliageType(FoliageType);
		}
	}
	
	if (FoliageType)
	{
		PopulateFoliageMeshList();
	}
	
	return FoliageType;
}

/** Remove a mesh */
bool FEdModeFoliage::RemoveFoliageType(UFoliageType** FoliageTypeList, int32 Num)
{
	TArray<AInstancedFoliageActor*> IFAList;
	// Find all foliage actors that have any of these types
	UWorld* World = GetWorld();
	for (int32 FoliageTypeIdx = 0; FoliageTypeIdx < Num; ++FoliageTypeIdx)
	{
		UFoliageType* FoliageType = FoliageTypeList[FoliageTypeIdx];
		for (FFoliageMeshInfoIterator It(World, FoliageType); It; ++It)
		{
			IFAList.Add(It.GetActor());
		}
	}

	if (IFAList.Num())
	{
		GEditor->BeginTransaction(NSLOCTEXT("UnrealEd", "FoliageMode_RemoveMeshTransaction", "Foliage Editing: Remove Mesh"));
		for (AInstancedFoliageActor* IFA : IFAList)
		{
			IFA->RemoveFoliageType(FoliageTypeList, Num);
		}
		GEditor->EndTransaction();

		PopulateFoliageMeshList();
		return true;
	}

	return false;
}

/** Bake instances to StaticMeshActors */
void FEdModeFoliage::BakeFoliage(UFoliageType* Settings, bool bSelectedOnly)
{
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForCurrentLevel(GetWorld());
	if (IFA == nullptr)
	{
		return;
	}

	FFoliageMeshInfo* MeshInfo = IFA->FindMesh(Settings);
	if (MeshInfo != nullptr)
	{
		TArray<int32> InstancesToConvert;
		if (bSelectedOnly)
		{
			InstancesToConvert = MeshInfo->SelectedIndices.Array();
		}
		else
		{
			for (int32 InstanceIdx = 0; InstanceIdx < MeshInfo->Instances.Num(); InstanceIdx++)
			{
				InstancesToConvert.Add(InstanceIdx);
			}
		}

		// Convert
		for (int32 Idx = 0; Idx < InstancesToConvert.Num(); Idx++)
		{
			FFoliageInstance& Instance = MeshInfo->Instances[InstancesToConvert[Idx]];
			// We need a world in which to spawn the actor. Use the one from the original instance.
			UWorld* World = IFA->GetWorld();
			check(World != nullptr);
			AStaticMeshActor* SMA = World->SpawnActor<AStaticMeshActor>(Instance.Location, Instance.Rotation);
			SMA->GetStaticMeshComponent()->StaticMesh = Settings->GetStaticMesh();
			SMA->GetRootComponent()->SetRelativeScale3D(Instance.DrawScale3D);
			SMA->MarkComponentsRenderStateDirty();
		}

		// Remove
		MeshInfo->RemoveInstances(IFA, InstancesToConvert);
	}
}

/** Copy the settings object for this static mesh */
UFoliageType* FEdModeFoliage::CopySettingsObject(UFoliageType* Settings)
{
	FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "FoliageMode_DuplicateSettingsObject", "Foliage Editing: Copy Settings Object"));

	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForCurrentLevel(GetWorld());
	IFA->Modify();

	TUniqueObj<FFoliageMeshInfo> MeshInfo;
	if (IFA->FoliageMeshes.RemoveAndCopyValue(Settings, MeshInfo))
	{
		Settings = (UFoliageType*)StaticDuplicateObject(Settings, IFA, nullptr, RF_AllFlags & ~(RF_Standalone | RF_Public));
		IFA->FoliageMeshes.Add(Settings, MoveTemp(MeshInfo));
		return Settings;
	}
	else
	{
		Transaction.Cancel();
	}

	return nullptr;
}

/** Replace the settings object for this static mesh with the one specified */
void FEdModeFoliage::ReplaceSettingsObject(UFoliageType* OldSettings, UFoliageType* NewSettings)
{
	FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "FoliageMode_ReplaceSettingsObject", "Foliage Editing: Replace Settings Object"));

	UWorld* World = GetWorld();
	for (FFoliageMeshInfoIterator It(World, OldSettings); It; ++It)
	{
		AInstancedFoliageActor* IFA = It.GetActor();
			
		IFA->Modify();
		TUniqueObj<FFoliageMeshInfo> OldMeshInfo;
		IFA->FoliageMeshes.RemoveAndCopyValue(OldSettings, OldMeshInfo);

		// Append instances if new foliage type is already exists in this actor
		// Otherwise just replace key entry for instances
		TUniqueObj<FFoliageMeshInfo>* NewMeshInfo = IFA->FoliageMeshes.Find(NewSettings);
		if (NewMeshInfo)
		{
			(*NewMeshInfo)->Instances.Append(OldMeshInfo->Instances);
			// Old component needs to go
			if (OldMeshInfo->Component != nullptr)
			{
				OldMeshInfo->Component->bAutoRegister = false;
				OldMeshInfo->Component = nullptr;
			}
			(*NewMeshInfo)->ReallocateClusters(IFA, NewSettings);
		}
		else
		{
			IFA->FoliageMeshes.Add(NewSettings, MoveTemp(OldMeshInfo))->ReallocateClusters(IFA, NewSettings);
		}
	}
			
	PopulateFoliageMeshList();
}

UFoliageType* FEdModeFoliage::SaveSettingsObject(UFoliageType* Settings)
{
	FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "FoliageMode_SaveSettingsObject", "Foliage Editing: Save Settings Object"));

	UFoliageType* SettingsToSave = nullptr;

	if (!Settings->IsAsset())
	{
		FString PackageName;
		UStaticMesh* StaticMesh = Settings->GetStaticMesh();
		if (StaticMesh)
		{
			// Build default settings asset name and path
			PackageName = FPackageName::GetLongPackagePath(StaticMesh->GetOutermost()->GetName()) + TEXT("/") + StaticMesh->GetName() + TEXT("_settings");
		}
		
		TSharedRef<SDlgPickAssetPath> SettingDlg =
			SNew(SDlgPickAssetPath)
			.Title(LOCTEXT("SettingsDialogTitle", "Choose Location for Foliage Settings Asset"))
			.DefaultAssetPath(FText::FromString(PackageName));

		if (SettingDlg->ShowModal() != EAppReturnType::Cancel)
		{
			PackageName = SettingDlg->GetFullAssetPath().ToString();
			UPackage* Package = CreatePackage(nullptr, *PackageName);
			SettingsToSave = Cast<UFoliageType>(StaticDuplicateObject(Settings, Package, *FPackageName::GetLongPackageAssetName(PackageName)));
			SettingsToSave->SetFlags(RF_Standalone | RF_Public);
			SettingsToSave->Modify();

			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(SettingsToSave);

			ReplaceSettingsObject(Settings, SettingsToSave);
		}
	}
	else
	{
		SettingsToSave = Settings;
	}
	
	// Save to disk
	if (SettingsToSave)
	{
		TArray<UPackage*> PackagesToSave; 
		PackagesToSave.Add(SettingsToSave->GetOutermost());
		const bool bCheckDirty = false;
		const bool bPromptToSave = false;
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave);
	}
		
	return SettingsToSave;
}

/** Reapply cluster settings to all the instances */
void FEdModeFoliage::ReallocateClusters(UFoliageType* Settings)
{
	UWorld* World = GetWorld();
	for (FFoliageMeshInfoIterator It(World, Settings); It; ++It)
	{
		FFoliageMeshInfo* MeshInfo = (*It);
		MeshInfo->ReallocateClusters(It.GetActor(), Settings);
	}
}

/** FEdMode: Called when a key is pressed */
bool FEdModeFoliage::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (UISettings.GetPaintToolSelected() || UISettings.GetReapplyToolSelected() || UISettings.GetLassoSelectToolSelected())
	{
		// Require Ctrl or not as per user preference
		ELandscapeFoliageEditorControlType FoliageEditorControlType = GetDefault<ULevelEditorViewportSettings>()->FoliageEditorControlType;

		if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
		{
			// Only activate tool if we're not already moving the camera and we're not trying to drag a transform widget
			// Not using "if (!ViewportClient->IsMovingCamera())" because it's wrong in ortho viewports :D
			bool bMovingCamera = Viewport->KeyState(EKeys::MiddleMouseButton) || Viewport->KeyState(EKeys::RightMouseButton) || IsAltDown(Viewport);

			if ((Viewport->IsPenActive() && Viewport->GetTabletPressure() > 0.f) ||
				(!bMovingCamera && ViewportClient->GetCurrentWidgetAxis() == EAxisList::None &&
					(FoliageEditorControlType == ELandscapeFoliageEditorControlType::IgnoreCtrl ||
					 (FoliageEditorControlType == ELandscapeFoliageEditorControlType::RequireCtrl   && IsCtrlDown(Viewport)) ||
					 (FoliageEditorControlType == ELandscapeFoliageEditorControlType::RequireNoCtrl && !IsCtrlDown(Viewport)))))
			{
				if (!bToolActive)
				{
					GEditor->BeginTransaction(NSLOCTEXT("UnrealEd", "FoliageMode_EditTransaction", "Foliage Editing"));
					PreApplyBrush();
					ApplyBrush(ViewportClient);
					bToolActive = true;
					return true;
				}
			}
		}

		if (bToolActive && Event == IE_Released &&
			(Key == EKeys::LeftMouseButton || (FoliageEditorControlType == ELandscapeFoliageEditorControlType::RequireCtrl && (Key == EKeys::LeftControl || Key == EKeys::RightControl))))
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

	if (UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected())
	{
		if (Event == IE_Pressed)
		{
			if (Key == EKeys::Platform_Delete)
			{
				RemoveSelectedInstances(GetWorld());
				return true;
			}
			else if (Key == EKeys::End)
			{
				SnapSelectedInstancesToGround(GetWorld());
				return true;
			}
		}
	}

	return false;
}

/** FEdMode: Render the foliage edit mode */
void FEdModeFoliage::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	/** Call parent implementation */
	FEdMode::Render(View, Viewport, PDI);
}


/** FEdMode: Render HUD elements for this tool */
void FEdModeFoliage::DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
}

/** FEdMode: Check to see if an actor can be selected in this mode - no side effects */
bool FEdModeFoliage::IsSelectionAllowed(AActor* InActor, bool bInSelection) const
{
	return false;
}

/** FEdMode: Handling SelectActor */
bool FEdModeFoliage::Select(AActor* InActor, bool bInSelected)
{
	// return true if you filter that selection
	// however - return false if we are trying to deselect so that it will infact do the deselection
	if (bInSelected == false)
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
void FEdModeFoliage::ForceRealTimeViewports(const bool bEnable, const bool bStoreCurrentState)
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr< ILevelViewport > ViewportWindow = LevelEditorModule.GetFirstActiveViewport();
	if (ViewportWindow.IsValid())
	{
		FEditorViewportClient &Viewport = ViewportWindow->GetLevelViewportClient();
		if (Viewport.IsPerspective())
		{
			if (bEnable)
			{
				Viewport.SetRealtime(bEnable, bStoreCurrentState);
			}
			else
			{
				const bool bAllowDisable = true;
				Viewport.RestoreRealtime(bAllowDisable);
			}

		}
	}
}

bool FEdModeFoliage::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy *HitProxy, const FViewportClick &Click)
{
	if (UISettings.GetSelectToolSelected())
	{
		if (HitProxy && HitProxy->IsA(HInstancedStaticMeshInstance::StaticGetType()))
		{
			HInstancedStaticMeshInstance* SMIProxy = ((HInstancedStaticMeshInstance*)HitProxy);
			AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(SMIProxy->Component->GetComponentLevel());
			if (IFA)
			{
				IFA->SelectInstance(SMIProxy->Component, SMIProxy->InstanceIndex, Click.IsControlDown());
				// Update pivot
				UpdateWidgetLocationToInstanceSelection();
			}
		}
		else
		{
			if (!Click.IsControlDown())
			{
				// Select none if not trying to toggle
				SelectInstances(GetWorld(), false);
			}
		}

		return true;
	}
	else if (UISettings.GetPaintBucketToolSelected() || UISettings.GetReapplyPaintBucketToolSelected())
	{
		if (HitProxy && HitProxy->IsA(HActor::StaticGetType()))
		{
			GEditor->BeginTransaction(NSLOCTEXT("UnrealEd", "FoliageMode_EditTransaction", "Foliage Editing"));
			
			if (Click.IsShiftDown())
			{
				ApplyPaintBucket_Remove(((HActor*)HitProxy)->Actor);
			}
			else
			{
				ApplyPaintBucket_Add(((HActor*)HitProxy)->Actor);
			}
						
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
bool FEdModeFoliage::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if (UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected())
	{
		// Update pivot
		UpdateWidgetLocationToInstanceSelection();

		GEditor->BeginTransaction(NSLOCTEXT("UnrealEd", "FoliageMode_EditTransaction", "Foliage Editing"));

		bCanAltDrag = true;

		return true;
	}
	return FEdMode::StartTracking(InViewportClient, InViewport);
}

/** FEdMode: Called when the a mouse button is released */
bool FEdModeFoliage::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if (UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected())
	{
		GEditor->EndTransaction();
		return true;
	}
	return FEdMode::EndTracking(InViewportClient, InViewport);
}

/** FEdMode: Called when mouse drag input it applied */
bool FEdModeFoliage::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	bool bFoundSelection = false;

	bool bAltDown = InViewport->KeyState(EKeys::LeftAlt) || InViewport->KeyState(EKeys::RightAlt);

	if (InViewportClient->GetCurrentWidgetAxis() != EAxisList::None && (UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected()))
	{
		const bool bDuplicateInstances = (bAltDown && bCanAltDrag && (InViewportClient->GetCurrentWidgetAxis() & EAxisList::XYZ));
		
		TransformSelectedInstances(GetWorld(), InDrag, InRot, InScale, bDuplicateInstances);

		// Only allow alt-drag on first InputDelta
		bCanAltDrag = false;
	}

	return FEdMode::InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
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
	if (UISettings.GetSelectToolSelected() || (UISettings.GetLassoSelectToolSelected() && !bToolActive))
	{
		FVector Location;
		return GetSelectionLocation(GetWorld(), Location) != nullptr;
	}
	return false;
}

EAxisList::Type FEdModeFoliage::GetWidgetAxisToDraw(FWidget::EWidgetMode InWidgetMode) const
{
	switch (InWidgetMode)
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
	if (GConfig->GetString(TEXT("FoliageEdit"), TEXT("WindowPosition"), WindowPositionString, GEditorUserSettingsIni))
	{
		TArray<FString> PositionValues;
		if (WindowPositionString.ParseIntoArray(&PositionValues, TEXT(","), true) == 4)
		{
			WindowX = FCString::Atoi(*PositionValues[0]);
			WindowY = FCString::Atoi(*PositionValues[1]);
			WindowWidth = FCString::Atoi(*PositionValues[2]);
			WindowHeight = FCString::Atoi(*PositionValues[3]);
		}
	}

	GConfig->GetFloat(TEXT("FoliageEdit"), TEXT("Radius"), Radius, GEditorUserSettingsIni);
	GConfig->GetFloat(TEXT("FoliageEdit"), TEXT("PaintDensity"), PaintDensity, GEditorUserSettingsIni);
	GConfig->GetFloat(TEXT("FoliageEdit"), TEXT("UnpaintDensity"), UnpaintDensity, GEditorUserSettingsIni);
	GConfig->GetBool(TEXT("FoliageEdit"), TEXT("bFilterLandscape"), bFilterLandscape, GEditorUserSettingsIni);
	GConfig->GetBool(TEXT("FoliageEdit"), TEXT("bFilterStaticMesh"), bFilterStaticMesh, GEditorUserSettingsIni);
	GConfig->GetBool(TEXT("FoliageEdit"), TEXT("bFilterBSP"), bFilterBSP, GEditorUserSettingsIni);
	GConfig->GetBool(TEXT("FoliageEdit"), TEXT("bFilterTranslucent"), bFilterTranslucent, GEditorUserSettingsIni);
}

/** Save UI settings to ini file */
void FFoliageUISettings::Save()
{
	FString WindowPositionString = FString::Printf(TEXT("%d,%d,%d,%d"), WindowX, WindowY, WindowWidth, WindowHeight);
	GConfig->SetString(TEXT("FoliageEdit"), TEXT("WindowPosition"), *WindowPositionString, GEditorUserSettingsIni);

	GConfig->SetFloat(TEXT("FoliageEdit"), TEXT("Radius"), Radius, GEditorUserSettingsIni);
	GConfig->SetFloat(TEXT("FoliageEdit"), TEXT("PaintDensity"), PaintDensity, GEditorUserSettingsIni);
	GConfig->SetFloat(TEXT("FoliageEdit"), TEXT("UnpaintDensity"), UnpaintDensity, GEditorUserSettingsIni);
	GConfig->SetBool(TEXT("FoliageEdit"), TEXT("bFilterLandscape"), bFilterLandscape, GEditorUserSettingsIni);
	GConfig->SetBool(TEXT("FoliageEdit"), TEXT("bFilterStaticMesh"), bFilterStaticMesh, GEditorUserSettingsIni);
	GConfig->SetBool(TEXT("FoliageEdit"), TEXT("bFilterBSP"), bFilterBSP, GEditorUserSettingsIni);
	GConfig->SetBool(TEXT("FoliageEdit"), TEXT("bFilterTranslucent"), bFilterTranslucent, GEditorUserSettingsIni);
}

#undef LOCTEXT_NAMESPACE
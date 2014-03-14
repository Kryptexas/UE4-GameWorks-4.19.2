// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
Landscape.cpp: Terrain rendering
=============================================================================*/

#include "EnginePrivate.h"
#include "Landscape/LandscapeDataAccess.h"
#include "Landscape/LandscapeRender.h"
#include "Landscape/LandscapeRenderMobile.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "MapErrors.h"
#include "DerivedDataCacheInterface.h"
#include "TargetPlatform.h"

// Set this to 0 to disable landscape cooking and thus disable it on device.
#define ENABLE_LANDSCAPE_COOKING 1

#define LOCTEXT_NAMESPACE "Landscape"

static void PrintNumLandscapeShadows()
{
	int32 NumComponents = 0;
	int32 NumShadowCasters = 0;
	for (TObjectIterator<ULandscapeComponent> It; It; ++It)
	{
		ULandscapeComponent* LC = *It;
		NumComponents++;
		if (LC->CastShadow && LC->bCastDynamicShadow)
		{
			NumShadowCasters++;
		}
	}
	UE_LOG(LogConsoleResponse,Display,TEXT("%d/%d landscape components cast shadows"),
		NumShadowCasters, NumComponents);
}

FAutoConsoleCommand CmdPrintNumLandscapeShadows(
	TEXT("ls.PrintNumLandscapeShadows"),
	TEXT("Prints the number of landscape components that cast shadows."),
	FConsoleCommandDelegate::CreateStatic(PrintNumLandscapeShadows)
	);

ULandscapeComponent::ULandscapeComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BodyInstance.bEnableCollision_DEPRECATED = true;
	SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	CastShadow = true;
	bUseAsOccluder = true;
	bAllowCullDistanceVolume = false;
	CollisionMipLevel = 0;
	StaticLightingResolution = 0.f; // Default value 0 means no overriding
#if WITH_EDITORONLY_DATA
	bNeedPostUndo = false;
#endif // WITH_EDITORONLY_DATA
	HeightmapScaleBias = FVector4(0.0f, 0.0f, 0.0f, 1.0f);

	WeightmapScaleBias = FVector4(0.0f, 0.0f, 0.0f, 1.0f);

	bBoundsChangeTriggersStreamingDataRebuild = true;
	ForcedLOD = -1;
	// Neighbor LOD and LODBias are saved in BYTE, so need to convert to range [-128:127]
	NeighborLOD[0] = 255;
	NeighborLOD[1] = 255;
	NeighborLOD[2] = 255;
	NeighborLOD[3] = 255;
	NeighborLOD[4] = 255;
	NeighborLOD[5] = 255;
	NeighborLOD[6] = 255;
	NeighborLOD[7] = 255;
	LODBias = 0;
	NeighborLODBias[0] = 128;
	NeighborLODBias[1] = 128;
	NeighborLODBias[2] = 128;
	NeighborLODBias[3] = 128;
	NeighborLODBias[4] = 128;
	NeighborLODBias[5] = 128;
	NeighborLODBias[6] = 128;
	NeighborLODBias[7] = 128;
	
	Mobility = EComponentMobility::Static;

	EditToolRenderData = NULL;

	LpvBiasMultiplier = 0.0f; // Bias is 0 for landscape, since it's single sided
}

void ULandscapeComponent::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	ULandscapeComponent* This = CastChecked<ULandscapeComponent>(InThis);
	if(This->LightMap != NULL)
	{
		This->LightMap->AddReferencedObjects(Collector);
	}
	if (This->ShadowMap != NULL)
	{
		This->ShadowMap->AddReferencedObjects(Collector);
	}
	Super::AddReferencedObjects(This, Collector);
}

void ULandscapeComponent::Serialize(FArchive& Ar)
{
#if ENABLE_LANDSCAPE_COOKING && WITH_EDITOR
	// Saving for cooking path
	if (Ar.IsCooking() && Ar.IsSaving() && !HasAnyFlags(RF_ClassDefaultObject))
	{
		if (!Ar.CookingTarget()->SupportsFeature(ETargetPlatformFeatures::VertexShaderTextureSampling) )
		{
			if (!PlatformData.HasValidPlatformData())
			{
				GeneratePlatformVertexData();
				MaterialInstance = Cast<UMaterialInstanceConstant>(GeneratePlatformPixelData(WeightmapTextures, true));
				HeightmapTexture = NULL;
			}
		}
	}
#endif

	Super::Serialize(Ar);
	
	Ar << LightMap;
	if (Ar.UE4Ver() >= VER_UE4_PRECOMPUTED_SHADOW_MAPS_BSP)
	{
		Ar << ShadowMap;
	}

#if WITH_EDITOR
	if ( Ar.IsTransacting() )
	{
		if (EditToolRenderData)
		{
			Ar << EditToolRenderData->SelectedType;
		}
		else
		{
			int32 TempV = 0;
			Ar << TempV;
		}
	}
#endif

	if (Ar.UE4Ver() < VER_UE4_CHANGED_IRRELEVANT_LIGHT_GUIDS)
	{
		// Irrelevant lights were incorrect before VER_UE4_CHANGED_IRRELEVANT_LIGHT_GUIDS
		IrrelevantLights.Empty();
	}

	bool bCooked = false;

	if (Ar.UE4Ver() >= VER_UE4_LANDSCAPE_PLATFORMDATA_COOKING && !HasAnyFlags(RF_ClassDefaultObject))
	{
		bCooked = Ar.IsCooking();
		// Save a bool indicating whether this is cooked data
		// This is needed when loading cooked data, to know to serialize differently
		Ar << bCooked;
	}

	if (FPlatformProperties::RequiresCookedData() && !bCooked && Ar.IsLoading())
	{
		UE_LOG(LogLandscape, Fatal, TEXT("This platform requires cooked packages, and this landscape does not contain cooked data %s."), *GetName());
	}

#if ENABLE_LANDSCAPE_COOKING
	if (bCooked)
	{
		// Saving for cooking path
		if (Ar.IsCooking() && !Ar.CookingTarget()->SupportsFeature(ETargetPlatformFeatures::VertexShaderTextureSampling))
		{
			bool bValid = PlatformData.HasValidPlatformData();
			Ar << bValid;
			if (bValid)
			{
				Ar << PlatformData;
			}
		}
		else if (!FPlatformProperties::SupportsVertexShaderTextureSampling())
		{
			// Loading cooked data path
			bool bValid = false;
			Ar << bValid;

			if (bValid)
			{
				Ar << PlatformData;
			}
		}
	}
#endif
}

#if WITH_EDITOR
UMaterialInterface* ULandscapeComponent::GetLandscapeMaterial() const
{
	if (OverrideMaterial)
	{
		return OverrideMaterial;
	}
	ALandscapeProxy* Proxy = GetLandscapeProxy();
	if (Proxy)
	{
		return Proxy->GetLandscapeMaterial();
	}
	return UMaterial::GetDefaultMaterial(MD_Surface);
}

UMaterialInterface* ULandscapeComponent::GetLandscapeHoleMaterial() const
{
	if (OverrideHoleMaterial)
	{
		return OverrideHoleMaterial;
	}
	ALandscapeProxy* Proxy = GetLandscapeProxy();
	if (Proxy)
	{
		return Proxy->GetLandscapeHoleMaterial();
	}
	return UMaterial::GetDefaultMaterial(MD_Surface);
}

bool ULandscapeComponent::ComponentHasVisibilityPainted() const
{
	for( int32 LayerIdx=0;LayerIdx < WeightmapLayerAllocations.Num();LayerIdx++ )
	{
		if( WeightmapLayerAllocations[LayerIdx].LayerInfo == ALandscapeProxy::DataLayer )
		{
			return true;
		}
	}

	return false;
}

FString ULandscapeComponent::GetLayerAllocationKey(bool bMobile /* = false */) const
{
	UMaterialInterface* LandscapeMaterial = GetLandscapeMaterial();
	if (!LandscapeMaterial)
	{
		return FString();
	}

	FString Result = LandscapeMaterial->GetPathName();

	// Sort the allocations
	TArray<FString> LayerStrings;
	for( int32 LayerIdx=0;LayerIdx < WeightmapLayerAllocations.Num();LayerIdx++ )
	{
		new(LayerStrings) FString( *FString::Printf(TEXT("_%s_%d"), *WeightmapLayerAllocations[LayerIdx].GetLayerName().ToString(), bMobile ? 0 : WeightmapLayerAllocations[LayerIdx].WeightmapTextureIndex) );
	}
/**
 * Generate a key for this component's layer allocations to use with MaterialInstanceConstantMap.
 */
	LayerStrings.Sort( TGreater<FString>() );

	for( int32 LayerIdx=0;LayerIdx < LayerStrings.Num();LayerIdx++ )
	{
		Result += LayerStrings[LayerIdx];
	}
	return Result;
}

void ULandscapeComponent::GetLayerDebugColorKey(int32& R, int32& G, int32& B) const
{
	ULandscapeInfo* Info = GetLandscapeInfo(false);
	if (Info)
	{
		R = INDEX_NONE, G = INDEX_NONE, B = INDEX_NONE;

		for (auto It = Info->Layers.CreateConstIterator(); It; It++)
		{
			const FLandscapeInfoLayerSettings& LayerStruct = *It;
			if (LayerStruct.DebugColorChannel > 0
				&& LayerStruct.LayerInfoObj)
			{
				for (int32 LayerIdx = 0; LayerIdx < WeightmapLayerAllocations.Num(); LayerIdx++)
				{
					if (WeightmapLayerAllocations[LayerIdx].LayerInfo == LayerStruct.LayerInfoObj)
					{
						if (LayerStruct.DebugColorChannel & 1) // R
						{
							R = (WeightmapLayerAllocations[LayerIdx].WeightmapTextureIndex*4+WeightmapLayerAllocations[LayerIdx].WeightmapTextureChannel);
						}
						if (LayerStruct.DebugColorChannel & 2) // G
						{
							G = (WeightmapLayerAllocations[LayerIdx].WeightmapTextureIndex*4+WeightmapLayerAllocations[LayerIdx].WeightmapTextureChannel);
						}
						if (LayerStruct.DebugColorChannel & 4) // B
						{
							B = (WeightmapLayerAllocations[LayerIdx].WeightmapTextureIndex*4+WeightmapLayerAllocations[LayerIdx].WeightmapTextureChannel);
						}
						break;
					}
				}
			}
		}
	}
}
#endif	//WITH_EDITOR

ULandscapeMeshCollisionComponent::ULandscapeMeshCollisionComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// make landscape always create? 
	bAlwaysCreatePhysicsState = true;
}

ULandscapeInfo::ULandscapeInfo(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
#if WITH_EDITOR
	bIsValid = false;
#endif
}

#if WITH_EDITOR
void ULandscapeInfo::UpdateDebugColorMaterial()
{
	FlushRenderingCommands();
	//GWarn->BeginSlowTask( *FString::Printf(TEXT("Compiling layer color combinations for %s"), *GetName()), true);
	
	for (auto It = XYtoComponentMap.CreateIterator(); It; ++It )
	{
		ULandscapeComponent* Comp = It.Value();
		if (Comp && Comp->EditToolRenderData)
		{
			Comp->EditToolRenderData->UpdateDebugColorMaterial();
		}
	}
	FlushRenderingCommands();
	//GWarn->EndSlowTask();
}

void ULandscapeComponent::PostLoad()
{
	Super::PostLoad();

	// Ensure that the component's lighting settings matches the actor's.
	ALandscapeProxy* LandscapeProxy = GetLandscapeProxy();
	if (ensure(LandscapeProxy))
	{
		bCastStaticShadow = LandscapeProxy->bCastStaticShadow;
		
		// convert from deprecated layer names to direct LayerInfo references
		static const FName DataWeightmapName = FName("__DataLayer__");
		for (int32 i = 0; i < WeightmapLayerAllocations.Num(); i++)
		{
			if (WeightmapLayerAllocations[i].LayerName_DEPRECATED != NAME_None)
			{
				if (WeightmapLayerAllocations[i].LayerName_DEPRECATED == DataWeightmapName)
				{
					WeightmapLayerAllocations[i].LayerInfo = ALandscapeProxy::DataLayer;
				}
				else
				{
					FLandscapeLayerStruct* Layer = LandscapeProxy->GetLayerInfo_Deprecated(WeightmapLayerAllocations[i].LayerName_DEPRECATED);
					WeightmapLayerAllocations[i].LayerInfo = Layer ? Layer->LayerInfoObj : NULL;
				}
				WeightmapLayerAllocations[i].LayerName_DEPRECATED = NAME_None;
			}
		}
	}

#if WITH_EDITOR
	if( GIsEditor && !HasAnyFlags(RF_ClassDefaultObject) )
	{
		// Remove standalone flags from data textures to ensure data is unloaded in the editor when reverting an unsaved level.
		// Previous version of landscape set these flags on creation.
		if( HeightmapTexture && HeightmapTexture->HasAnyFlags(RF_Standalone) )
		{
			HeightmapTexture->ClearFlags(RF_Standalone);
		}
		for( int32 Idx=0;Idx<WeightmapTextures.Num();Idx++ )
		{
			if( WeightmapTextures[Idx] && WeightmapTextures[Idx]->HasAnyFlags(RF_Standalone) )
			{
				WeightmapTextures[Idx]->ClearFlags(RF_Standalone);
			}
		}

		if (GetLinkerUE4Version() < VER_UE4_FIXUP_TERRAIN_LAYER_NODES)
		{
			UpdateMaterialInstances();
		}
	}
#endif

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (!CachedLocalBox.IsValid && CachedBoxSphereBounds_DEPRECATED.SphereRadius > 0)
		{
			USceneComponent* LandscapeRoot = LandscapeProxy->GetRootComponent();
			check(LandscapeRoot == AttachParent);

			// Component isn't attached so we can't use its LocalToWorld
			FTransform ComponentLtWTransform = FTransform(RelativeRotation, RelativeLocation, RelativeScale3D) * FTransform(LandscapeRoot->RelativeRotation, LandscapeRoot->RelativeLocation, LandscapeRoot->RelativeScale3D);

			// This is good enough. The exact box will be calculated during painting.
			CachedLocalBox = CachedBoxSphereBounds_DEPRECATED.GetBox().InverseTransformBy(ComponentLtWTransform);
		}

		// If we're loading on a platform that doesn't require cooked data, attempt to load missing data from the DDC
		if (!FPlatformProperties::RequiresCookedData() && GRHIFeatureLevel == ERHIFeatureLevel::ES2)
		{
			// Only check the DDC if we don't already have it loaded
			if (!PlatformData.HasValidPlatformData())
			{
				// Attempt to load the ES2 landscape data from the DDC
				if (!PlatformData.LoadFromDDC(StateId))
				{
					// Height Data is available after loading height map, so need to defer it
					UE_LOG(LogLandscape, Warning, TEXT("Attempt to access the DDC when there is none available on component '%s'."), *GetFullName());
				}
			}
		}
	}
}

#endif // WITH_EDITOR

ALandscape::ALandscape(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
#if WITH_EDITORONLY_DATA
	if ((SpriteComponent != NULL) && !IsRunningCommandlet())
	{
		// Structure to hold one-time initialization
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> TerrainTexture;
			FName ID_Landscape;
			FText NAME_Landscape;
			FConstructorStatics()
				: TerrainTexture(TEXT("/Engine/EditorResources/S_Terrain.S_Terrain"))
				, ID_Landscape(TEXT("Landscape"))
				, NAME_Landscape(NSLOCTEXT( "SpriteCategory", "Landscape", "Landscape" ))

			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		SpriteComponent->Sprite = ConstructorStatics.TerrainTexture.Get();
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Landscape;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Landscape;
	}
#endif // WITH_EDITORONLY_DATA

	bHidden = false;
	StaticLightingResolution = 1.0f;
	StreamingDistanceMultiplier = 1.0f;
	bIsProxy = false;

#if WITH_EDITORONLY_DATA
	bLockLocation = false;
#endif // WITH_EDITORONLY_DATA
}

ALandscape* ALandscape::GetLandscapeActor()
{
	return this;
}

ALandscapeProxy::ALandscapeProxy(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> TerrainTexture;
		ConstructorHelpers::FObjectFinderOptional<ULandscapeLayerInfoObject> DataLayer;
		FConstructorStatics()
			: TerrainTexture(TEXT("Texture2D'/Engine/EditorResources/S_Terrain.S_Terrain'"))
			, DataLayer(TEXT("LandscapeLayerInfoObject'/Engine/EditorLandscapeResources/DataLayer.DataLayer'"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;


	TSubobjectPtr<USceneComponent> SceneComponent = PCIP.CreateDefaultSubobject<USceneComponent>(this, TEXT("RootComponent0"));;
	RootComponent = SceneComponent;
	RootComponent->RelativeScale3D = FVector(128.0f, 128.0f, 256.0f); // Old default scale, preserved for compatibility. See ULandscapeEditorObject::NewLandscape_Scale
	RootComponent->Mobility = EComponentMobility::Static;
	LandscapeSectionOffset = FIntPoint::ZeroValue;
	
#if WITH_EDITORONLY_DATA
	if (SpriteComponent)
	{
		SpriteComponent->Sprite = ConstructorStatics.TerrainTexture.Get();
		SpriteComponent->AttachParent = RootComponent;
		SpriteComponent->bAbsoluteScale = true;
	}
#endif

	StaticLightingResolution = 1.0f;
	StreamingDistanceMultiplier = 1.0f;
	bHidden = false;
	bIsProxy = true;
	MaxLODLevel = -1;
#if WITH_EDITORONLY_DATA
	bLockLocation = true;
	bIsMovingToLevel = false;
#endif // WITH_EDITORONLY_DATA
	LODDistanceFactor = 1.0f;
	bCastStaticShadow = true;
	bUsedForNavigation = true;
	CollisionThickness = 16;
	BodyInstance.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
#if WITH_EDITORONLY_DATA
	MaxPaintedLayersPerComponent = 0;
#endif

	if (DataLayer==NULL)
	{
		DataLayer = ConstructorStatics.DataLayer.Get();
		check(DataLayer);
		DataLayer->AddToRoot();
	}
}

ALandscape* ALandscapeProxy::GetLandscapeActor()
{
#if WITH_EDITORONLY_DATA
	return LandscapeActor.Get();
#else
	return 0;
#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITOR
ULandscapeInfo* ALandscapeProxy::GetLandscapeInfo(bool bSpawnNewActor /*= true*/)
{
	// LandscapeInfo generate
	if (GIsEditor)
	{
		UWorld* OwningWorld = GetWorld();
		if (OwningWorld)
		{
			ULandscapeInfo* LandscapeInfo = OwningWorld->LandscapeInfoMap.FindRef(LandscapeGuid);
			if (!LandscapeInfo && bSpawnNewActor && !HasAnyFlags(RF_BeginDestroyed))
			{
				LandscapeInfo = ConstructObject<ULandscapeInfo>(ULandscapeInfo::StaticClass(), OwningWorld, NAME_None, RF_Transactional | RF_Transient);
				OwningWorld->Modify(false);
				OwningWorld->LandscapeInfoMap.Add(LandscapeGuid, LandscapeInfo);
			}

			return LandscapeInfo;
		}
	}

	return NULL;
}
#endif

ALandscape* ULandscapeComponent::GetLandscapeActor() const
{
	ALandscapeProxy* Landscape = GetLandscapeProxy();
	if (Landscape)
	{
		return Landscape->GetLandscapeActor();
	}
	return NULL;
}

ALandscapeProxy* ULandscapeComponent::GetLandscapeProxy() const
{
	return CastChecked<ALandscapeProxy>(GetOuter());
}

FIntPoint ULandscapeComponent::GetSectionBase() const
{
	return FIntPoint(SectionBaseX, SectionBaseY);
}

void ULandscapeComponent::SetSectionBase(FIntPoint InSectionBase)
{
	SectionBaseX = InSectionBase.X;
	SectionBaseY = InSectionBase.Y;
}

#if WITH_EDITOR
ULandscapeInfo* ULandscapeComponent::GetLandscapeInfo(bool bSpawnNewActor /*= true*/) const
{
	if (GetLandscapeProxy())
	{
		return GetLandscapeProxy()->GetLandscapeInfo(bSpawnNewActor);
	}
	return NULL;
}
#endif

void ULandscapeComponent::BeginDestroy()
{
	Super::BeginDestroy();

#if WITH_EDITOR
	if (EditToolRenderData != NULL)
	{
		// Ask render thread to destroy EditToolRenderData
		EditToolRenderData->Cleanup();
		EditToolRenderData = NULL;
	}

	if( GIsEditor && !HasAnyFlags(RF_ClassDefaultObject) )
	{
		ALandscapeProxy* Proxy = GetLandscapeProxy();

		// Remove any weightmap allocations from the Landscape Actor's map
		for( int32 LayerIdx=0;LayerIdx < WeightmapLayerAllocations.Num();LayerIdx++ )
		{
			int32 WeightmapIndex = WeightmapLayerAllocations[LayerIdx].WeightmapTextureIndex;
			if( WeightmapTextures.IsValidIndex(WeightmapIndex) )
			{
				UTexture2D* WeightmapTexture = WeightmapTextures[WeightmapIndex];
				FLandscapeWeightmapUsage* Usage = Proxy->WeightmapUsageMap.Find(WeightmapTexture);
				if( Usage != NULL )
				{
					Usage->ChannelUsage[WeightmapLayerAllocations[LayerIdx].WeightmapTextureChannel] = NULL;

					if( Usage->FreeChannelCount()==4 )
					{
						Proxy->WeightmapUsageMap.Remove(WeightmapTexture);
					}
				}
			}
		}
	}
#endif
}

FPrimitiveSceneProxy* ULandscapeComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* Proxy = NULL;
	if (GRHIFeatureLevel >= ERHIFeatureLevel::SM3)
	{
#if WITH_EDITOR
		if( EditToolRenderData == NULL )
		{
			EditToolRenderData = new FLandscapeEditToolRenderData(this);
		}
		Proxy = new FLandscapeComponentSceneProxy(this, EditToolRenderData);
#else
		Proxy = new FLandscapeComponentSceneProxy(this, NULL);
#endif
	}
	else if (GRHIFeatureLevel >= ERHIFeatureLevel::ES2)
	{
#if WITH_EDITOR
		if( !PlatformData.HasValidPlatformData() ) // Deferred generation
		{
			// Try to reload the ES2 landscape data from the DDC
			if (!PlatformData.LoadFromDDC(StateId))
			{
				// Generate and save to the derived data cache
				GeneratePlatformVertexData();

				if (PlatformData.HasValidPlatformData())
				{
					PlatformData.SaveToDDC(StateId);
				}
			}
		}

		if (PlatformData.HasValidPlatformData())
		{
			if( EditToolRenderData == NULL )
			{
				EditToolRenderData = new FLandscapeEditToolRenderData(this);
			}

			Proxy = new FLandscapeComponentSceneProxyMobile(this, EditToolRenderData);
		}
#else
		if (PlatformData.HasValidPlatformData())
		{
			Proxy = new FLandscapeComponentSceneProxyMobile(this, NULL);
		}
#endif
	}
	return Proxy;
}

void ULandscapeComponent::DestroyComponent()
{
	ALandscapeProxy* Proxy = GetLandscapeProxy();
	if (Proxy)
	{
		Proxy->LandscapeComponents.Remove(this);
	}

	Super::DestroyComponent();
}

FBoxSphereBounds ULandscapeComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	return FBoxSphereBounds(CachedLocalBox.TransformBy(LocalToWorld));
}

void ULandscapeComponent::OnRegister()
{
	Super::OnRegister();

#if WITH_EDITOR
	ULandscapeInfo* Info = GetLandscapeInfo(false);
	if (Info)
	{
		Info->RegisterActorComponent(this);
	}
#endif
}

void ULandscapeComponent::OnUnregister()
{
	Super::OnUnregister();

#if WITH_EDITOR
	ULandscapeInfo* Info = GetLandscapeInfo(false);
	if (Info)
	{
		Info->UnregisterActorComponent(this);
	}
#endif
}

void ULandscapeComponent::GetUsedMaterials( TArray<UMaterialInterface*>& OutMaterials ) const
{
	if( MaterialInstance != NULL )
	{
		OutMaterials.Add(MaterialInstance);
	}
}

void ALandscapeProxy::RegisterAllComponents()
{
	Super::RegisterAllComponents();

	// Landscape was added to world 
	// We might need to update shared landscape data
	if (GIsEditor && GetWorld() && !GetWorld()->IsPlayInEditor() )
	{
		GEngine->DeferredCommands.AddUnique(TEXT("UpdateLandscapeEditorData"));
	}
}

void ALandscapeProxy::UnregisterAllComponents()
{
	Super::UnregisterAllComponents();

	// Landscape was removed from world 
	// We might need to update shared landscape data
	if (GEngine && GIsEditor && GetWorld() && !GetWorld()->IsPlayInEditor())
	{
		GEngine->DeferredCommands.AddUnique(TEXT("UpdateLandscapeEditorData"));
	}
}

bool ALandscapeProxy::UpdateNavigationRelevancy() 
{
	SetNavigationRelevancy(bUsedForNavigation);
	
	return bUsedForNavigation; 
}

FBox ALandscapeProxy::GetComponentsBoundingBox(bool bNonColliding) const 
{
	FBox Bounds = Super::GetComponentsBoundingBox(bNonColliding);

	USceneComponent* LandscapeRoot = GetRootComponent();

	// can't really tell if it breaks anything, but is needed for navmesh generation
	if (bUsedForNavigation)
	{
		for (auto CollisionComponentIt( CollisionComponents.CreateConstIterator() ); CollisionComponentIt; ++CollisionComponentIt)
		{
			const ULandscapeHeightfieldCollisionComponent* Component = *CollisionComponentIt;
			if (Component != NULL)
			{
				// Component may not be attached, so we'll calculate bounds manually
				FTransform ComponentLtWTransform = FTransform(Component->RelativeRotation, Component->RelativeLocation, Component->RelativeScale3D) *
					FTransform(LandscapeRoot->RelativeRotation, LandscapeRoot->RelativeLocation, LandscapeRoot->RelativeScale3D);

				Bounds += Component->CalcBounds(ComponentLtWTransform).GetBox();
			}
		}
	}
	return Bounds;
}

// FLandscapeWeightmapUsage serializer
FArchive& operator<<( FArchive& Ar, FLandscapeWeightmapUsage& U )
{
	return Ar << U.ChannelUsage[0] << U.ChannelUsage[1] << U.ChannelUsage[2] << U.ChannelUsage[3];
}

FArchive& operator<<( FArchive& Ar, FLandscapeAddCollision& U )
{
#if WITH_EDITORONLY_DATA
	return Ar << U.Corners[0] << U.Corners[1] << U.Corners[2] << U.Corners[3];
#else
	return Ar;
#endif // WITH_EDITORONLY_DATA
}

FArchive& operator<<( FArchive& Ar, FLandscapeLayerStruct*& L )
{
	if (L)
	{
		Ar << L->LayerInfoObj;
#if WITH_EDITORONLY_DATA
		return Ar << L->ThumbnailMIC;
#else
		return Ar;
#endif // WITH_EDITORONLY_DATA
	}
	return Ar;
}

void ULandscapeInfo::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// We do not serialize XYtoComponentMap as we don't want these references to hold a component in memory.
	// The references are automatically cleaned up by the components' BeginDestroy method.
	if (Ar.IsTransacting())
	{
		Ar << XYtoComponentMap;
		Ar << XYtoAddCollisionMap;
		Ar << SelectedComponents;
		Ar << SelectedRegion;
		Ar << SelectedRegionComponents;
	}
}

void ALandscape::PostLoad()
{
	if (!LandscapeGuid.IsValid())
	{
		LandscapeGuid = FGuid::NewGuid();
	}

	Super::PostLoad();
}

void ULandscapeInfo::BeginDestroy()
{
	Super::BeginDestroy();

#if WITH_EDITOR
	if( DataInterface )
	{
		delete DataInterface;
		DataInterface = NULL;
	}
#endif
}

#if WITH_EDITOR

void ALandscape::CheckForErrors()
{
}

#endif

void ALandscapeProxy::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

#if WITH_EDITOR
	if( Ar.IsTransacting() )
	{
		Ar << WeightmapUsageMap;
	}
#endif
}

void ALandscapeProxy::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	ALandscapeProxy* This = CastChecked<ALandscapeProxy>(InThis);

	Super::AddReferencedObjects(InThis, Collector);

	for (auto It = This->MaterialInstanceConstantMap.CreateIterator(); It; ++It)
	{
		Collector.AddReferencedObject(It.Value(), This);
	}

	for (auto It = This->WeightmapUsageMap.CreateIterator(); It; ++It)
	{
		Collector.AddReferencedObject(It.Key(), This);
		Collector.AddReferencedObject(It.Value().ChannelUsage[0], This);
		Collector.AddReferencedObject(It.Value().ChannelUsage[1], This);
		Collector.AddReferencedObject(It.Value().ChannelUsage[2], This);
		Collector.AddReferencedObject(It.Value().ChannelUsage[3], This);
	}
}

#if WITH_EDITOR
void ALandscapeProxy::PreEditUndo()
{
	Super::PreEditUndo();
	if (GIsEditor)
	{
		// Remove all layer info for this Proxy
		ULandscapeInfo* LandscapeInfo = GetLandscapeInfo(false);
		if (LandscapeInfo)
		{
			LandscapeInfo->UpdateLayerInfoMap(this, true);
		}
	}
}

void ALandscapeProxy::PostEditUndo()
{
	Super::PostEditUndo();
	if (GIsEditor && GetWorld() && !GetWorld()->IsPlayInEditor())
	{
		// defer LandscapeInfo setup
		GEngine->DeferredCommands.AddUnique(TEXT("UpdateLandscapeEditorData"));
		GEngine->DeferredCommands.AddUnique(TEXT("RestoreLandscapeLayerInfos"));
	}
}

void ULandscapeInfo::PostEditUndo()
{
	Super::PostEditUndo();
	ULandscapeInfo::RecreateLandscapeInfo(CastChecked<UWorld>(GetOuter()), true);
}

FName FLandscapeInfoLayerSettings::GetLayerName() const
{
	checkSlow(LayerInfoObj == NULL || LayerInfoObj->LayerName == LayerName);

	return LayerName;
}

FLandscapeEditorLayerSettings& FLandscapeInfoLayerSettings::GetEditorSettings() const
{
	check(LayerInfoObj);

	ULandscapeInfo* LandscapeInfo = Owner->GetLandscapeInfo();
	return LandscapeInfo->GetLayerEditorSettings(LayerInfoObj);
}

FLandscapeEditorLayerSettings& ULandscapeInfo::GetLayerEditorSettings(ULandscapeLayerInfoObject* LayerInfo) const
{
	auto& LayerSettingsList = GetLandscapeProxy()->EditorLayerSettings;
	FLandscapeEditorLayerSettings* EditorLayerSettings = LayerSettingsList.FindByKey(LayerInfo);
	if (EditorLayerSettings)
	{
		return *EditorLayerSettings;
	}
	else
	{
		int32 Index = LayerSettingsList.Add(LayerInfo);
		return LayerSettingsList[Index];
	}
}

void ULandscapeInfo::CreateLayerEditorSettingsFor(ULandscapeLayerInfoObject* LayerInfo)
{
	ALandscape* Landscape = LandscapeActor.Get();
	if (Landscape != NULL)
	{
		FLandscapeEditorLayerSettings* EditorLayerSettings = Landscape->EditorLayerSettings.FindByKey(LayerInfo);
		if (EditorLayerSettings == NULL)
		{
			Landscape->Modify();
			Landscape->EditorLayerSettings.Add(LayerInfo);
		}
	}

	for (auto It = Proxies.CreateConstIterator(); It; ++It)
	{
		ALandscapeProxy* Proxy = *It;
		FLandscapeEditorLayerSettings* EditorLayerSettings = Proxy->EditorLayerSettings.FindByKey(LayerInfo);
		if (EditorLayerSettings == NULL)
		{
			Proxy->Modify();
			Proxy->EditorLayerSettings.Add(LayerInfo);
		}
	}
}

ULandscapeLayerInfoObject* ULandscapeInfo::GetLayerInfoByName(FName LayerName, ALandscapeProxy* Owner /*= NULL*/) const
{
	ULandscapeLayerInfoObject* LayerInfo = NULL;
	for (int32 j = 0; j < Layers.Num(); j++)
	{
		if (Layers[j].LayerInfoObj && Layers[j].LayerInfoObj->LayerName == LayerName
			&& (Owner == NULL || Layers[j].Owner == Owner))
		{
			LayerInfo = Layers[j].LayerInfoObj;
		}
	}
	return LayerInfo;
}

int32 ULandscapeInfo::GetLayerInfoIndex(ULandscapeLayerInfoObject* LayerInfo, ALandscapeProxy* Owner /*= NULL*/) const
{
	for (int32 j = 0; j < Layers.Num(); j++)
	{
		if (Layers[j].LayerInfoObj && Layers[j].LayerInfoObj == LayerInfo
			&& (Owner == NULL || Layers[j].Owner == Owner))
		{
			return j;
		}
	}

	return INDEX_NONE;
}

int32 ULandscapeInfo::GetLayerInfoIndex(FName LayerName, ALandscapeProxy* Owner /*= NULL*/) const
{
	for (int32 j = 0; j < Layers.Num(); j++)
	{
		if (Layers[j].GetLayerName() == LayerName
			&& (Owner == NULL || Layers[j].Owner == Owner))
		{
			return j;
		}
	}

	return INDEX_NONE;
}


bool ULandscapeInfo::UpdateLayerInfoMap(ALandscapeProxy* Proxy /*= NULL*/, bool bInvalidate /*= false*/)
{
	bool bHasCollision = false;
	if (GIsEditor)
	{
		bIsValid = !bInvalidate;
	
		if (Proxy)
		{
			if (bInvalidate)
			{
				// this is a horribly dangerous combination of parameters...

				for (int32 i = 0; i < Layers.Num(); i++)
				{
					if (Layers[i].Owner == Proxy)
					{
						Layers.RemoveAt(i--);
					}
				}
			}
			else // Proxy && !bInvalidate
			{
				TArray<FName> LayerNames = Proxy->GetLayersFromMaterial();

				// Validate any existing layer infos owned by this proxy
				for (int32 i = 0; i < Layers.Num(); i++)
				{
					if (Layers[i].Owner == Proxy)
					{
						Layers[i].bValid = LayerNames.Contains(Layers[i].GetLayerName());
					}
				}

				// Add placeholders for any unused material layers
				for (int32 i = 0; i < LayerNames.Num(); i++)
				{
					int32 LayerInfoIndex = GetLayerInfoIndex(LayerNames[i]);
					if (LayerInfoIndex == INDEX_NONE)
					{
						FLandscapeInfoLayerSettings LayerSettings(LayerNames[i], Proxy);
						LayerSettings.bValid = true;
						Layers.Add(LayerSettings);
					}
				}

				// Populate from layers used in components
				for (int32 ComponentIndex = 0; ComponentIndex < Proxy->LandscapeComponents.Num(); ComponentIndex++)
				{
					ULandscapeComponent* Component = Proxy->LandscapeComponents[ComponentIndex];

					// Add layers from per-component override materials
					if (Component->OverrideMaterial != NULL)
					{
						TArray<FName> ComponentLayerNames = Proxy->GetLayersFromMaterial(Component->OverrideMaterial);
						for (int32 i = 0; i < ComponentLayerNames.Num(); i++)
						{
							int32 LayerInfoIndex = GetLayerInfoIndex(ComponentLayerNames[i]);
							if (LayerInfoIndex == INDEX_NONE)
							{
								FLandscapeInfoLayerSettings LayerSettings(ComponentLayerNames[i], Proxy);
								LayerSettings.bValid = true;
								Layers.Add(LayerSettings);
							}
						}
					}

					for (int32 AllocationIndex = 0; AllocationIndex < Component->WeightmapLayerAllocations.Num(); AllocationIndex++)
					{
						ULandscapeLayerInfoObject* LayerInfo = Component->WeightmapLayerAllocations[AllocationIndex].LayerInfo;
						if (LayerInfo)
						{
							int32 LayerInfoIndex = GetLayerInfoIndex(LayerInfo);
							bool bValid = LayerNames.Contains(LayerInfo->LayerName);

							if (LayerInfoIndex != INDEX_NONE)
							{
								FLandscapeInfoLayerSettings& LayerSettings = Layers[LayerInfoIndex];

								// Valid layer infos take precedence over invalid ones
								// Landscape Actors take precedence over Proxies
								if ( (bValid && !LayerSettings.bValid)
									|| (bValid == LayerSettings.bValid && !Proxy->bIsProxy))
								{
									LayerSettings.Owner = Proxy;
									LayerSettings.bValid = bValid;
									LayerSettings.ThumbnailMIC = NULL;
								}
							}
							else
							{
								// handle existing placeholder layers
								LayerInfoIndex = GetLayerInfoIndex(LayerInfo->LayerName);
								if (LayerInfoIndex != INDEX_NONE)
								{
									FLandscapeInfoLayerSettings& LayerSettings = Layers[LayerInfoIndex];

									//if (LayerSettings.Owner == Proxy)
									{
										LayerSettings.Owner = Proxy;
										LayerSettings.LayerInfoObj = LayerInfo;
										LayerSettings.bValid = bValid;
										LayerSettings.ThumbnailMIC = NULL;
									}
								}
								else
								{
										FLandscapeInfoLayerSettings LayerSettings(LayerInfo, Proxy);
										LayerSettings.bValid = bValid;
										Layers.Add(LayerSettings);
								}
							}
						}
					}
				}

				// Add any layer infos cached in the actor
				Proxy->EditorLayerSettings.Remove(NULL);
				for (int32 i = 0; i < Proxy->EditorLayerSettings.Num(); i++)
				{
					FLandscapeEditorLayerSettings& LayerInfo = Proxy->EditorLayerSettings[i];
					if (LayerNames.Contains(LayerInfo.LayerInfoObj->LayerName))
					{
						// intentionally using the layer name here so we don't add layer infos from
						// the cache that have the same name as an actual assignment from a component above
						int32 LayerInfoIndex = GetLayerInfoIndex(LayerInfo.LayerInfoObj->LayerName);
						if (LayerInfoIndex != INDEX_NONE)
						{
							FLandscapeInfoLayerSettings& LayerSettings = Layers[LayerInfoIndex];
							if (LayerSettings.LayerInfoObj == NULL)
							{
								LayerSettings.Owner = Proxy;
								LayerSettings.LayerInfoObj = LayerInfo.LayerInfoObj;
								LayerSettings.bValid = true;
							}
						}
					}
					else
					{
						Proxy->Modify();
						Proxy->EditorLayerSettings.RemoveAt(i--);
					}
				}
			}
		}
		else // !Proxy
		{
			Layers.Empty();

			if (!bInvalidate)
			{
				if (LandscapeActor.IsValid() && !LandscapeActor->IsPendingKillPending())
				{
					checkSlow(LandscapeActor->GetLandscapeInfo() == this);
					UpdateLayerInfoMap(LandscapeActor.Get(), false);
				}
				for (auto It = Proxies.CreateIterator(); It; ++It)
				{
					ALandscapeProxy* LandscapeProxy = *It;
					checkSlow(LandscapeProxy && LandscapeProxy->GetLandscapeInfo() == this);
					UpdateLayerInfoMap(LandscapeProxy, false);
				}
			}
		}

		//if (GCallbackEvent)
		//{
		//	GCallbackEvent->Send( CALLBACK_EditorPostModal );
		//}
	}
	return bHasCollision;
}
#endif

void ALandscapeProxy::PostLoad()
{
	Super::PostLoad();

	// Temporary
	if( ComponentSizeQuads == 0 && LandscapeComponents.Num() > 0 )
	{
		ULandscapeComponent* Comp = LandscapeComponents[0];
		if( Comp )
		{
			ComponentSizeQuads = Comp->ComponentSizeQuads;
			SubsectionSizeQuads = Comp->SubsectionSizeQuads;	
			NumSubsections = Comp->NumSubsections;
		}
	}

	if (IsTemplate() == false)
	{
		BodyInstance.FixupData(this);
	}

#if WITH_EDITOR
	if ( GetLinker() && (GetLinker()->UE4Ver() < VER_UE4_LANDSCAPE_COMPONENT_LAZY_REFERENCES) || LandscapeComponents.Num() != CollisionComponents.Num() )
	{
		// Need to clean up invalid collision components
		RecreateCollisionComponents();
	}

	EditorLayerSettings.Remove(NULL);

	if (EditorCachedLayerInfos_DEPRECATED.Num() > 0)
	{
		for (int32 i = 0; i < EditorCachedLayerInfos_DEPRECATED.Num(); i++)
		{
			EditorLayerSettings.Add(EditorCachedLayerInfos_DEPRECATED[i]);
		}
		EditorCachedLayerInfos_DEPRECATED.Empty();
	}

	if (GIsEditor && !GetWorld()->IsPlayInEditor())
	{
		// defer LandscapeInfo setup
		GEngine->DeferredCommands.AddUnique(TEXT("UpdateLandscapeEditorData -warnings"));
	}
#endif
}

#if WITH_EDITOR
void ALandscape::Destroyed()
{
	Super::Destroyed();

	if (SplineComponent)
	{
		SplineComponent->ModifySplines();
	}
}

void ALandscapeProxy::Destroyed()
{
	Super::Destroyed();

	if (GIsEditor)
	{
		ULandscapeInfo::RecreateLandscapeInfo(GetWorld(), false);
	}
}
#endif

#if WITH_EDITOR
void ALandscapeProxy::GetSharedProperties(ALandscapeProxy* Landscape)
{
	if (GIsEditor && Landscape)
	{
		Modify();

		LandscapeGuid = Landscape->LandscapeGuid;

		//@todo UE4 merge, landscape, this needs work
		RootComponent->SetRelativeScale3D(Landscape->GetRootComponent()->GetComponentToWorld().GetScale3D());

		//PrePivot = Landscape->PrePivot;
		StaticLightingResolution = Landscape->StaticLightingResolution;
		bCastStaticShadow = Landscape->bCastStaticShadow;
		ComponentSizeQuads = Landscape->ComponentSizeQuads;
		NumSubsections = Landscape->NumSubsections;
		SubsectionSizeQuads = Landscape->SubsectionSizeQuads;
		MaxLODLevel = Landscape->MaxLODLevel;
		LODDistanceFactor = Landscape->LODDistanceFactor;
		if (!LandscapeMaterial)
		{
			LandscapeMaterial = Landscape->LandscapeMaterial;
		}
		if (!LandscapeHoleMaterial)
		{
			LandscapeHoleMaterial = Landscape->LandscapeHoleMaterial;
		}
		if (LandscapeMaterial == Landscape->LandscapeMaterial)
		{
			EditorLayerSettings = Landscape->EditorLayerSettings;
		}
		if (!DefaultPhysMaterial)
		{
			DefaultPhysMaterial = Landscape->DefaultPhysMaterial;
		}
		LightmassSettings = Landscape->LightmassSettings;
	}
}

FTransform ALandscapeProxy::LandscapeActorToWorld() const
{
	FTransform TM = ActorToWorld();
	// Add this proxy landscape section offset to obtain landscape actor transform
	TM.AddToTranslation(TM.TransformVector(-FVector(LandscapeSectionOffset)));
	return TM;
}

void ALandscapeProxy::SetAbsoluteSectionBase(FIntPoint InSectionBase)
{
	FIntPoint Difference = InSectionBase - LandscapeSectionOffset;
	LandscapeSectionOffset = InSectionBase;
	
	for (int32 CompIdx = 0; CompIdx < LandscapeComponents.Num(); CompIdx++)
	{
		ULandscapeComponent* Comp = LandscapeComponents[CompIdx];
		if (Comp)
		{
			FIntPoint AbsoluteSectionBase = Comp->GetSectionBase() + Difference;
			Comp->SetSectionBase(AbsoluteSectionBase);
		}
	}
	
	for (int32 CompIdx = 0; CompIdx < CollisionComponents.Num(); CompIdx++)
	{
		ULandscapeHeightfieldCollisionComponent* Comp = CollisionComponents[CompIdx];
		if (Comp)
		{
			FIntPoint AbsoluteSectionBase = Comp->GetSectionBase() + Difference;
			Comp->SetSectionBase(AbsoluteSectionBase);
		}
	}
}

FIntPoint ALandscapeProxy::GetSectionBaseOffset() const
{
	return LandscapeSectionOffset;
}

void ALandscapeProxy::RecreateComponentsState()
{
	for(int32 ComponentIndex = 0; ComponentIndex < LandscapeComponents.Num(); ComponentIndex++ )
	{
		ULandscapeComponent* Comp = LandscapeComponents[ComponentIndex];
		if( Comp )
		{
			Comp->UpdateComponentToWorld();
			Comp->UpdateCachedBounds();
			Comp->UpdateBounds();
			Comp->RecreateRenderState_Concurrent(); // @todo UE4 jackp just render state needs update?
		}
	}

	for(int32 ComponentIndex = 0; ComponentIndex < CollisionComponents.Num(); ComponentIndex++ )
	{
		ULandscapeHeightfieldCollisionComponent* Comp = CollisionComponents[ComponentIndex];
		if( Comp )
		{
			Comp->UpdateComponentToWorld();
			Comp->RecreateCollision();
		}
	}
}

UMaterialInterface* ALandscapeProxy::GetLandscapeMaterial() const
{
	if (LandscapeMaterial)
	{
		return LandscapeMaterial;
	}
	else if (LandscapeActor)
	{
		return LandscapeActor->GetLandscapeMaterial();
	}
	return UMaterial::GetDefaultMaterial(MD_Surface);
}

UMaterialInterface* ALandscapeProxy::GetLandscapeHoleMaterial() const
{
	if (LandscapeMaterial)
	{
		return LandscapeMaterial;
	}
	else if (LandscapeActor)
	{
		return LandscapeActor->GetLandscapeHoleMaterial();
	}
	return UMaterial::GetDefaultMaterial(MD_Surface);
}

UMaterialInterface* ALandscape::GetLandscapeMaterial() const
{
	if (LandscapeMaterial)
	{
		return LandscapeMaterial;
	}
	return UMaterial::GetDefaultMaterial(MD_Surface);
}

UMaterialInterface* ALandscape::GetLandscapeHoleMaterial() const
{
	if (LandscapeHoleMaterial)
	{
		return LandscapeHoleMaterial;
	}
	return UMaterial::GetDefaultMaterial(MD_Surface);
}

void ALandscape::PreSave()
{
	Super::PreSave();
	//ULandscapeInfo* Info = GetLandscapeInfo();
	//if (GIsEditor && Info && !IsRunningCommandlet())
	//{
	//	for (TSet<ALandscapeProxy*>::TIterator It(Info->Proxies); It; ++It)
	//	{
	//		ALandscapeProxy* Proxy = *It;
	//		if (!ensure(Proxy->LandscapeActor == this))
	//		{
	//			Proxy->LandscapeActor = this;
	//			Proxy->GetSharedProperties(this);
	//		}
	//	}
	//}
}

ALandscapeProxy* ULandscapeInfo::GetCurrentLevelLandscapeProxy(bool bRegistered) const
{
	ALandscape* Landscape = LandscapeActor.Get();
	
	if (Landscape && (!bRegistered || Landscape->GetRootComponent()->IsRegistered()))
	{
		UWorld* LandscapeWorld = Landscape->GetWorld();
		if (LandscapeWorld && 
			LandscapeWorld->GetCurrentLevel() == Landscape->GetOuter())
		{
			return Landscape;
		}
	}
		
	for (auto It = Proxies.CreateConstIterator(); It; ++It)
	{
		ALandscapeProxy* LandscapeProxy = (*It);
		
		if (LandscapeProxy && (!bRegistered || LandscapeProxy->GetRootComponent()->IsRegistered()))
		{
			UWorld* ProxyWorld = LandscapeProxy->GetWorld();
			if (ProxyWorld && 
				ProxyWorld->GetCurrentLevel() == LandscapeProxy->GetOuter())
			{
				return LandscapeProxy;
			}
		}
	}

	return NULL;
}

ALandscapeProxy* ULandscapeInfo::GetLandscapeProxy() const
{
	// Mostly this Proxy used to calculate transformations
	// in Editor all proxies of same landscape actor have root components in same locations
	// so it doesn't really matter which proxy we return here
	
	// prefer LandscapeActor in case it is loaded
	ALandscape* Landscape = LandscapeActor.Get();
	if (Landscape != NULL &&
		Landscape->GetRootComponent()->IsRegistered())
	{
		return Landscape;
	}
	
	// prefer current level proxy 
	ALandscapeProxy* Proxy = GetCurrentLevelLandscapeProxy(true);
	if (Proxy != NULL)
	{
		return Proxy;
	}

	// any proxy in the world
	for (auto It = Proxies.CreateConstIterator(); It; ++It)
	{
		Proxy = (*It);
		if (Proxy != NULL &&
			Proxy->GetRootComponent()->IsRegistered())
		{
			return Proxy;
		}
	}

	return NULL;
}

void ULandscapeInfo::RegisterActor(ALandscapeProxy* Proxy, bool bMapCheck)
{
	// do not pass here invalid actors
	check(Proxy->GetLandscapeGuid().IsValid());
	UWorld* OwningWorld = CastChecked<UWorld>(GetOuter());
	
	// in case this Info object is not initialized yet
	// initialized it with properties from passed actor
	if (LandscapeGuid.IsValid() == false ||
		(GetLandscapeProxy() == NULL && ensure(LandscapeGuid == Proxy->GetLandscapeGuid())))
	{
		LandscapeGuid			= Proxy->GetLandscapeGuid();
		ComponentSizeQuads		= Proxy->ComponentSizeQuads;
		ComponentNumSubsections	= Proxy->NumSubsections;
		SubsectionSizeQuads		= Proxy->SubsectionSizeQuads;
		DrawScale				= Proxy->GetRootComponent()->RelativeScale3D;
	}
	
	// check that passed actor matches all shared parameters
	check(LandscapeGuid == Proxy->GetLandscapeGuid());
	check(ComponentSizeQuads == Proxy->ComponentSizeQuads);
	check(ComponentNumSubsections == Proxy->NumSubsections);
	check(SubsectionSizeQuads == Proxy->SubsectionSizeQuads);
	check(DrawScale == Proxy->GetRootComponent()->RelativeScale3D);

	// register
	ALandscape* Landscape = Cast<ALandscape>(Proxy);
	if (Landscape)
	{
		LandscapeActor = Landscape;
		// In world composition user is not allowed to move landscape in editor, only through WorldBrowser 
		LandscapeActor->bLockLocation = (OwningWorld->WorldComposition != NULL);
		
		// update proxies reference actor
		for (auto It = Proxies.CreateConstIterator(); It; ++It)
		{
			(*It)->LandscapeActor = LandscapeActor;
		}
	}
	else
	{
		Proxies.Add(Proxy);
		Proxy->LandscapeActor = LandscapeActor;
	}

	//
	// add proxy components to the XY map
	//
	for (int32 CompIdx = 0; CompIdx < Proxy->LandscapeComponents.Num(); ++CompIdx)
	{
		RegisterActorComponent(Proxy->LandscapeComponents[CompIdx], bMapCheck);
	}
}
	
void ULandscapeInfo::UnregisterActor(ALandscapeProxy* Proxy)
{
	ALandscape* Landscape = Cast<ALandscape>(Proxy);
	if (Landscape)
	{
		check(LandscapeActor.Get() == Landscape);
		LandscapeActor = 0;

		// update proxies reference to landscape actor
		for (auto It = Proxies.CreateConstIterator(); It; ++It)
		{
			(*It)->LandscapeActor = 0;
		}
	}
	else
	{
		Proxies.Remove(Proxy);
		Proxy->LandscapeActor = 0;
	}

	// remove proxy components from the XY map
	for (int32 CompIdx = 0; CompIdx < Proxy->LandscapeComponents.Num(); ++CompIdx)
	{
		UnregisterActorComponent(Proxy->LandscapeComponents[CompIdx]);
	}
			
	XYtoComponentMap.Compact();
}

void ULandscapeInfo::RegisterActorComponent(ULandscapeComponent* Component, bool bMapCheck)
{
	// Do not register components which are not part of the world
	if (Component == NULL ||
		Component->IsRegistered() == false)
	{
		return;
	}

	check(Component != NULL);
	FIntPoint ComponentKey = Component->GetSectionBase()/Component->ComponentSizeQuads;
	auto RegisteredComponent = XYtoComponentMap.FindRef(ComponentKey);

	if (RegisteredComponent == NULL)
	{
		XYtoComponentMap.Add(ComponentKey, Component);
	}
	else if (bMapCheck)
	{
		ALandscapeProxy* Proxy = Component->GetLandscapeProxy();
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ProxyName"), FText::FromString(Proxy->GetName()));
		Arguments.Add(TEXT("XLocation"), Component->GetSectionBase().X);
		Arguments.Add(TEXT("YLocation"), Component->GetSectionBase().Y);
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(Proxy))
			->AddToken(FTextToken::Create(FText::Format( LOCTEXT( "MapCheck_Message_LandscapeComponentPostLoad_Warning", "Landscape ({ProxyName}) has overlapping render components at location ({XLocation}, {YLocation})." ), Arguments )))
			->AddToken(FMapErrorToken::Create(FMapErrors::LandscapeComponentPostLoad_Warning));

		// Show MapCheck window
		FMessageLog("MapCheck").Open( EMessageSeverity::Warning );
	}
}
	
void ULandscapeInfo::UnregisterActorComponent(ULandscapeComponent* Component)
{
	check(Component != NULL);

	FIntPoint ComponentKey = Component->GetSectionBase()/Component->ComponentSizeQuads;
	auto RegisteredComponent = XYtoComponentMap.FindRef(ComponentKey);

	if (RegisteredComponent == Component)
	{
		XYtoComponentMap.Remove(ComponentKey);
	}

	SelectedComponents.Remove(Component);
	SelectedRegionComponents.Remove(Component);
}

void ULandscapeInfo::Reset()
{
	LandscapeActor.Reset();
	
	Proxies.Empty();
	XYtoComponentMap.Empty();
	XYtoAddCollisionMap.Empty();
	
	//SelectedComponents.Empty();
	//SelectedRegionComponents.Empty();
	//SelectedRegion.Empty();
}

void ULandscapeInfo::FixupProxiesTransform()
{
	UWorld* OwningWorld = CastChecked<UWorld>(GetOuter());
	if (OwningWorld->WorldComposition != NULL)
	{
		return;
	}
		
	ALandscape* Landscape = LandscapeActor.Get();

	if (Landscape == NULL || 
		Landscape->GetRootComponent()->IsRegistered() == false)
	{
		return;
	}
	
	FTransform LandscapeTM = Landscape->LandscapeActorToWorld();
	// Update transformations of all linked landscape proxies
	for (auto It = Proxies.CreateConstIterator(); It; ++It)
	{
		ALandscapeProxy* Proxy = *It;
		FTransform ProxyRelativeTM(FVector(Proxy->LandscapeSectionOffset));
		FTransform ProxyTransform = ProxyRelativeTM*LandscapeTM;
		
		if (!Proxy->GetTransform().Equals(ProxyTransform))
		{
			Proxy->SetActorTransform(ProxyTransform);
			Proxy->RecreateComponentsState();
			
			// Let other systems know that an actor was moved
			GEngine->BroadcastOnActorMoved( Proxy );
		}
	}
}

void ULandscapeInfo::FixupProxiesWeightmaps()
{
	if (LandscapeActor.IsValid())
	{
		LandscapeActor->WeightmapUsageMap.Empty();
		for (int32 CompIdx = 0; CompIdx < LandscapeActor->LandscapeComponents.Num(); ++CompIdx)
		{
			ULandscapeComponent* Comp = LandscapeActor->LandscapeComponents[CompIdx];
			if (Comp)
			{
				Comp->FixupWeightmaps();
			}
		}
	}
		
	for (auto It = Proxies.CreateConstIterator(); It; ++It)
	{
		ALandscapeProxy* Proxy = (*It);
		Proxy->WeightmapUsageMap.Empty();
		for (int32 CompIdx = 0; CompIdx < Proxy->LandscapeComponents.Num(); ++CompIdx)
		{
			ULandscapeComponent* Comp = Proxy->LandscapeComponents[CompIdx];
			if (Comp)
			{
				Comp->FixupWeightmaps();
			}
		}
	}
}

void ULandscapeInfo::RecreateLandscapeInfo(UWorld* InWorld, bool bMapCheck)
{
	check(InWorld);

	// reset all LandscapeInfo objects
	for (auto It = InWorld->LandscapeInfoMap.CreateIterator(); It; ++It)
	{
		It.Value()->Modify();
		It.Value()->Reset();
	}

	TMap<FGuid, TArray<ALandscapeProxy*>> ValidLandscapesMap;
	// Gather all valid landscapes in the world
	for (FActorIterator It(InWorld); It; ++It)
	{
		ALandscapeProxy* Proxy = Cast<ALandscapeProxy>(*It);
		if (Proxy 
			&& Proxy->HasAnyFlags(RF_BeginDestroyed) == false 
			&& Proxy->IsPendingKill() == false
			&& Proxy->IsPendingKillPending() == false)
		{
			ValidLandscapesMap.FindOrAdd(Proxy->GetLandscapeGuid()).Add(Proxy);
		}
	}
	
	// Register gathered landscapes in shared LandscapeInfo map
	for (auto It = ValidLandscapesMap.CreateIterator(); It; ++It)
	{
		auto& LandscapeList = It.Value();
		ALandscapeProxy* FirstVisibleLandscape = NULL;
			
		for (int32 LandscapeIdx = 0; LandscapeIdx < LandscapeList.Num(); LandscapeIdx++)
		{
			ALandscapeProxy* Proxy = LandscapeList[LandscapeIdx];
			if (InWorld->WorldComposition && 
				Proxy->GetRootComponent()->IsRegistered())
			{
				if (FirstVisibleLandscape == NULL)
				{
					FirstVisibleLandscape = Proxy;
				}
				
				// Setup first landscape in the list as origin for section based grid
				// And based on that calculate section coordinates for all landscapes in the list
				FVector Offset = Proxy->GetActorLocation() - FirstVisibleLandscape->GetActorLocation();
				FVector	DrawScale = FirstVisibleLandscape->GetRootComponent()->RelativeScale3D;
				
				FIntPoint QuadsSpaceOffset; 
				QuadsSpaceOffset.X = Offset.X/DrawScale.X;
				QuadsSpaceOffset.Y = Offset.Y/DrawScale.Y;
				Proxy->SetAbsoluteSectionBase(QuadsSpaceOffset);
			}

			// Register gathered landscape actors inside global LandscapeInfo object
			Proxy->GetLandscapeInfo(true)->RegisterActor(Proxy, bMapCheck);
		}
	}

	// Remove empty entries from global LandscapeInfo map
	for (auto It = InWorld->LandscapeInfoMap.CreateIterator(); It; ++It)
	{
		if (It.Value()->GetLandscapeProxy() == NULL)
		{
			It.Value()->MarkPendingKill();
			It.RemoveCurrent();
		}
	}

	// Update layer info maps
	for (auto It = InWorld->LandscapeInfoMap.CreateIterator(); It; ++It)
	{
		ULandscapeInfo* LandscapeInfo = It.Value();
		if (LandscapeInfo)
		{
			LandscapeInfo->UpdateLayerInfoMap();
		}
	}

	// Update add collision
	for (auto It = InWorld->LandscapeInfoMap.CreateIterator(); It; ++It)
	{
		ULandscapeInfo* LandscapeInfo = It.Value();
		if (LandscapeInfo)
		{
			LandscapeInfo->UpdateAllAddCollisions();
		}
	}

	// We need to inform Landscape editor tools about LandscapeInfo updates
	FEditorSupportDelegates::WorldChange.Broadcast();
}


#endif

void ULandscapeComponent::PostInitProperties()
{
	Super::PostInitProperties();

	// Create a new guid in case this is a newly created component
	// If not, this guid will be overwritten when serialized
	FPlatformMisc::CreateGuid(StateId);
}

void ULandscapeComponent::PostDuplicate(bool bDuplicateForPIE)
{
	if (!bDuplicateForPIE)
	{
		// Reset the StateId on duplication since it needs to be unique for each capture.
		// PostDuplicate covers direct calls to StaticDuplicateObject, but not actor duplication (see PostEditImport)
		FPlatformMisc::CreateGuid(StateId);
	}
}

// Generate a new guid to force a recache of all landscape derived data
#define LANDSCAPE_FULL_DERIVEDDATA_VER			TEXT("d17ce2a02ccc11e3aa6e0800200c9a66")

FString FLandscapeComponentDerivedData::GetDDCKeyString(const FGuid& StateId)
{
	return FDerivedDataCacheInterface::BuildCacheKey(TEXT("LS_FULL"), LANDSCAPE_FULL_DERIVEDDATA_VER, *StateId.ToString());
}

void FLandscapeComponentDerivedData::InitializeFromUncompressedData(const TArray<uint8>& UncompressedData)
{
	int32 UncompressedSize = UncompressedData.Num() * UncompressedData.GetTypeSize();

	TArray<uint8> TempCompressedMemory;
	// Compressed can be slightly larger than uncompressed
	TempCompressedMemory.Empty(UncompressedSize * 4 / 3);
	TempCompressedMemory.AddUninitialized(UncompressedSize * 4 / 3);
	int32 CompressedSize = TempCompressedMemory.Num() * TempCompressedMemory.GetTypeSize();

	verify(FCompression::CompressMemory(
		(ECompressionFlags)(COMPRESS_ZLIB | COMPRESS_BiasMemory), 
		TempCompressedMemory.GetData(), 
		CompressedSize, 
		UncompressedData.GetData(), 
		UncompressedSize));

	// Note: change LANDSCAPE_FULL_DERIVEDDATA_VER when modifying the serialization layout
	FMemoryWriter FinalArchive(CompressedLandscapeData, true);
	FinalArchive << UncompressedSize;
	FinalArchive << CompressedSize;
	FinalArchive.Serialize(TempCompressedMemory.GetData(), CompressedSize);
}

void FLandscapeComponentDerivedData::GetUncompressedData(TArray<uint8>& OutUncompressedData)
{
	check(CompressedLandscapeData.Num() > 0);

	FMemoryReader Ar(CompressedLandscapeData);

	// Note: change LANDSCAPE_FULL_DERIVEDDATA_VER when modifying the serialization layout
	int32 UncompressedSize;
	Ar << UncompressedSize;

	int32 CompressedSize;
	Ar << CompressedSize;

	TArray<uint8> CompressedData;
	CompressedData.Empty(CompressedSize);
	CompressedData.AddUninitialized(CompressedSize);
	Ar.Serialize(CompressedData.GetData(), CompressedSize);

	OutUncompressedData.Empty(UncompressedSize);
	OutUncompressedData.AddUninitialized(UncompressedSize);

	verify(FCompression::UncompressMemory((ECompressionFlags)COMPRESS_ZLIB, OutUncompressedData.GetData(), UncompressedSize, CompressedData.GetData(), CompressedSize));

	// free the compressed data now that we have the uncompressed version
	CompressedLandscapeData.Empty();
}

FArchive& operator<<(FArchive& Ar, FLandscapeComponentDerivedData& Data)
{
	check(!Ar.IsSaving() || Data.CompressedLandscapeData.Num() > 0);
	return Ar << Data.CompressedLandscapeData;
}

bool FLandscapeComponentDerivedData::LoadFromDDC(const FGuid& StateId)
{
	return GetDerivedDataCacheRef().GetSynchronous(*GetDDCKeyString(StateId), CompressedLandscapeData);
}

void FLandscapeComponentDerivedData::SaveToDDC(const FGuid& StateId)
{
	check(CompressedLandscapeData.Num() > 0);
	GetDerivedDataCacheRef().Put(*GetDDCKeyString(StateId), CompressedLandscapeData);
}


#undef LOCTEXT_NAMESPACE
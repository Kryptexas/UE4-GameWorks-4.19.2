// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
MapBuildData.cpp
=============================================================================*/

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "LightMap.h"
#include "UObject/UObjectAnnotation.h"
#include "PrecomputedLightVolume.h"
#include "PrecomputedVolumetricLightmap.h"
#include "Engine/MapBuildDataRegistry.h"
#include "ShadowMap.h"
#include "UObject/Package.h"
#include "EngineUtils.h"
#include "Components/ModelComponent.h"
#include "ComponentRecreateRenderStateContext.h"
#include "UObject/RenderingObjectVersion.h"
#include "UObject/ReflectionCaptureObjectVersion.h"
#include "ContentStreaming.h"
#include "Components/ReflectionCaptureComponent.h"
#include "Interfaces/ITargetPlatform.h"

DECLARE_MEMORY_STAT(TEXT("Stationary Light Static Shadowmap"),STAT_StationaryLightBuildData,STATGROUP_MapBuildData);
DECLARE_MEMORY_STAT(TEXT("Reflection Captures"),STAT_ReflectionCaptureBuildData,STATGROUP_MapBuildData);

FArchive& operator<<(FArchive& Ar, FMeshMapBuildData& MeshMapBuildData)
{
	Ar << MeshMapBuildData.LightMap;
	Ar << MeshMapBuildData.ShadowMap;
	Ar << MeshMapBuildData.IrrelevantLights;
	MeshMapBuildData.PerInstanceLightmapData.BulkSerialize(Ar);

	return Ar;
}

ULevel* UWorld::GetActiveLightingScenario() const
{
	for (int32 LevelIndex = 0; LevelIndex < Levels.Num(); LevelIndex++)
	{
		ULevel* LocalLevel = Levels[LevelIndex];

		if (LocalLevel->bIsVisible && LocalLevel->bIsLightingScenario)
		{
			return LocalLevel;
		}
	}

	return NULL;
}

void UWorld::PropagateLightingScenarioChange()
{
	for (FActorIterator It(this); It; ++It)
	{
		TInlineComponentArray<USceneComponent*> Components;
		(*It)->GetComponents(Components);

		for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
		{
			USceneComponent* CurrentComponent = Components[ComponentIndex];
			CurrentComponent->PropagateLightingScenarioChange();
		}
	}

	for (ULevel* Level : GetLevels())
	{
		Level->ReleaseRenderingResources();
		Level->InitializeRenderingResources();

		for (UModelComponent* ModelComponent : Level->ModelComponents)
		{
			ModelComponent->PropagateLightingScenarioChange();
		}
	}

	IStreamingManager::Get().PropagateLightingScenarioChange();
}

UMapBuildDataRegistry* CreateRegistryForLegacyMap(ULevel* Level)
{
	static FName RegistryName(TEXT("MapBuildDataRegistry"));
	// Create a new registry for legacy map build data, but put it in the level's package.  
	// This avoids creating a new package during cooking which the cooker won't know about.
	Level->MapBuildData = NewObject<UMapBuildDataRegistry>(Level->GetOutermost(), RegistryName, RF_NoFlags);
	return Level->MapBuildData;
}

void ULevel::HandleLegacyMapBuildData()
{
	if (GComponentsWithLegacyLightmaps.GetAnnotationMap().Num() > 0 
		|| GLevelsWithLegacyBuildData.GetAnnotationMap().Num() > 0
		|| GLightComponentsWithLegacyBuildData.GetAnnotationMap().Num() > 0)
	{
		FLevelLegacyMapBuildData LegacyLevelData = GLevelsWithLegacyBuildData.GetAndRemoveAnnotation(this);

		UMapBuildDataRegistry* Registry = NULL;
		if (LegacyLevelData.Id != FGuid())
		{
			Registry = CreateRegistryForLegacyMap(this);
			Registry->AddLevelPrecomputedLightVolumeBuildData(LegacyLevelData.Id, LegacyLevelData.Data);
		}

		for (int32 ActorIndex = 0; ActorIndex < Actors.Num(); ActorIndex++)
		{
			if (!Actors[ActorIndex])
			{
				continue;
			}

			TInlineComponentArray<UActorComponent*> Components;
			Actors[ActorIndex]->GetComponents(Components);

			for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
			{
				UActorComponent* CurrentComponent = Components[ComponentIndex];
				FMeshMapBuildLegacyData LegacyMeshData = GComponentsWithLegacyLightmaps.GetAndRemoveAnnotation(CurrentComponent);

				for (int32 EntryIndex = 0; EntryIndex < LegacyMeshData.Data.Num(); EntryIndex++)
				{
					if (!Registry)
					{
						Registry = CreateRegistryForLegacyMap(this);
					}

					FMeshMapBuildData& DestData = Registry->AllocateMeshBuildData(LegacyMeshData.Data[EntryIndex].Key, false);
					DestData = *LegacyMeshData.Data[EntryIndex].Value;
					delete LegacyMeshData.Data[EntryIndex].Value;
				}

				FLightComponentLegacyMapBuildData LegacyLightData = GLightComponentsWithLegacyBuildData.GetAndRemoveAnnotation(CurrentComponent);

				if (LegacyLightData.Id != FGuid())
				{
					FLightComponentMapBuildData& DestData = Registry->FindOrAllocateLightBuildData(LegacyLightData.Id, false);
					DestData = *LegacyLightData.Data;
					delete LegacyLightData.Data;
				}
			}
		}

		for (UModelComponent* ModelComponent : ModelComponents)
		{
			ModelComponent->PropagateLightingScenarioChange();
			FMeshMapBuildLegacyData LegacyData = GComponentsWithLegacyLightmaps.GetAndRemoveAnnotation(ModelComponent);

			for (int32 EntryIndex = 0; EntryIndex < LegacyData.Data.Num(); EntryIndex++)
			{
				if (!Registry)
				{
					Registry = CreateRegistryForLegacyMap(this);
				}

				FMeshMapBuildData& DestData = Registry->AllocateMeshBuildData(LegacyData.Data[EntryIndex].Key, false);
				DestData = *LegacyData.Data[EntryIndex].Value;
				delete LegacyData.Data[EntryIndex].Value;
			}
		}
	}

	if (GReflectionCapturesWithLegacyBuildData.GetAnnotationMap().Num() > 0)
	{
		UMapBuildDataRegistry* Registry = MapBuildData;

		for (int32 ActorIndex = 0; ActorIndex < Actors.Num(); ActorIndex++)
		{
			if (!Actors[ActorIndex])
			{
				continue;
			}

			TInlineComponentArray<UActorComponent*> Components;
			Actors[ActorIndex]->GetComponents(Components);

			for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
			{
				UActorComponent* CurrentComponent = Components[ComponentIndex];
				UReflectionCaptureComponent* ReflectionCapture = Cast<UReflectionCaptureComponent>(CurrentComponent);

				if (ReflectionCapture)
				{
					FReflectionCaptureMapBuildLegacyData LegacyReflectionData = GReflectionCapturesWithLegacyBuildData.GetAndRemoveAnnotation(ReflectionCapture);

					if (!LegacyReflectionData.IsDefault())
					{
						if (!Registry)
						{
							Registry = CreateRegistryForLegacyMap(this);
						}

						FReflectionCaptureMapBuildData& DestData = Registry->AllocateReflectionCaptureBuildData(LegacyReflectionData.Id, false);
						DestData = *LegacyReflectionData.MapBuildData;
						delete LegacyReflectionData.MapBuildData;
					}
				}
			}
		}
	}
}

FMeshMapBuildData::FMeshMapBuildData()
{}

FMeshMapBuildData::~FMeshMapBuildData()
{}

void FMeshMapBuildData::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (LightMap)
	{
		LightMap->AddReferencedObjects(Collector);
	}

	if (ShadowMap)
	{
		ShadowMap->AddReferencedObjects(Collector);
	}
}

void FStaticShadowDepthMapData::Empty()
{
	ShadowMapSizeX = 0;
	ShadowMapSizeY = 0;
	DepthSamples.Empty();
}

FArchive& operator<<(FArchive& Ar, FStaticShadowDepthMapData& ShadowMapData)
{
	Ar << ShadowMapData.WorldToLight;
	Ar << ShadowMapData.ShadowMapSizeX;
	Ar << ShadowMapData.ShadowMapSizeY;
	Ar << ShadowMapData.DepthSamples;

	return Ar;
}

FLightComponentMapBuildData::~FLightComponentMapBuildData()
{
	DEC_DWORD_STAT_BY(STAT_StationaryLightBuildData, DepthMap.GetAllocatedSize());
}

void FLightComponentMapBuildData::FinalizeLoad()
{
	INC_DWORD_STAT_BY(STAT_StationaryLightBuildData, DepthMap.GetAllocatedSize());
}

FArchive& operator<<(FArchive& Ar, FLightComponentMapBuildData& LightBuildData)
{
	Ar << LightBuildData.ShadowMapChannel;
	Ar << LightBuildData.DepthMap;

	if (Ar.IsLoading())
	{
		LightBuildData.FinalizeLoad();
	}

	return Ar;
}

FArchive& operator<<(FArchive& Ar, FReflectionCaptureMapBuildData& ReflectionCaptureMapBuildData)
{
	Ar << ReflectionCaptureMapBuildData.CubemapSize;
	Ar << ReflectionCaptureMapBuildData.AverageBrightness;

	if (Ar.CustomVer(FRenderingObjectVersion::GUID) >= FRenderingObjectVersion::StoreReflectionCaptureBrightnessForCooking)
	{
		Ar << ReflectionCaptureMapBuildData.Brightness;
	}

	static FName FullHDR(TEXT("FullHDR"));
	static FName EncodedHDR(TEXT("EncodedHDR"));

	TArray<FName> Formats;

	if (Ar.IsSaving() && Ar.IsCooking())
	{
		// Get all the reflection capture formats that the target platform wants
		Ar.CookingTarget()->GetReflectionCaptureFormats(Formats);
	}

	if (Formats.Num() == 0 || Formats.Contains(FullHDR))
	{
		Ar << ReflectionCaptureMapBuildData.FullHDRCapturedData;
	}
	else
	{
		TArray<uint8> StrippedData;
		Ar << StrippedData;
	}

	if (Formats.Num() == 0 || Formats.Contains(EncodedHDR))
	{
		if (Ar.IsSaving() 
			&& Ar.IsCooking()
			&& ReflectionCaptureMapBuildData.EncodedHDRCapturedData.Num() == 0
			&& ReflectionCaptureMapBuildData.FullHDRCapturedData.Num() > 0)
		{
			// Encode from HDR as needed
			//@todo - cache in DDC?
			GenerateEncodedHDRData(ReflectionCaptureMapBuildData.FullHDRCapturedData, ReflectionCaptureMapBuildData.CubemapSize, ReflectionCaptureMapBuildData.Brightness, ReflectionCaptureMapBuildData.EncodedHDRCapturedData);
		}

		Ar << ReflectionCaptureMapBuildData.EncodedHDRCapturedData;
	}
	else
	{
		TArray<uint8> StrippedData;
		Ar << StrippedData;
	}

	if (Ar.IsLoading())
	{
		ReflectionCaptureMapBuildData.FinalizeLoad();
	}

	return Ar;
}

FReflectionCaptureMapBuildData::~FReflectionCaptureMapBuildData()
{
	DEC_DWORD_STAT_BY(STAT_ReflectionCaptureBuildData, AllocatedSize);
}

void FReflectionCaptureMapBuildData::FinalizeLoad()
{
	AllocatedSize = FullHDRCapturedData.GetAllocatedSize() + EncodedHDRCapturedData.GetAllocatedSize();
	INC_DWORD_STAT_BY(STAT_ReflectionCaptureBuildData, AllocatedSize);
}


UMapBuildDataRegistry::UMapBuildDataRegistry(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LevelLightingQuality = Quality_MAX;
}

void UMapBuildDataRegistry::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	FStripDataFlags StripFlags(Ar, 0);

	Ar.UsingCustomVersion(FRenderingObjectVersion::GUID);
	Ar.UsingCustomVersion(FReflectionCaptureObjectVersion::GUID);

	if (!StripFlags.IsDataStrippedForServer())
	{
		Ar << MeshBuildData;
		Ar << LevelPrecomputedLightVolumeBuildData;

		if (Ar.CustomVer(FRenderingObjectVersion::GUID) >= FRenderingObjectVersion::VolumetricLightmaps)
		{
			Ar << LevelPrecomputedVolumetricLightmapBuildData;
		}

		Ar << LightBuildData;

		if (Ar.IsSaving())
		{
			for (TMap<FGuid, FReflectionCaptureMapBuildData>::TIterator It(ReflectionCaptureBuildData); It; ++It)
			{
				const FReflectionCaptureMapBuildData& CaptureBuildData = It.Value();
				// Sanity check that every reflection capture entry has valid data for at least one format
				check(CaptureBuildData.FullHDRCapturedData.Num() > 0 || CaptureBuildData.EncodedHDRCapturedData.Num() > 0);
			}
		}

		if (Ar.CustomVer(FReflectionCaptureObjectVersion::GUID) >= FReflectionCaptureObjectVersion::MoveReflectionCaptureDataToMapBuildData)
		{
			Ar << ReflectionCaptureBuildData;
		}
	}
}

void UMapBuildDataRegistry::PostLoad()
{
	Super::PostLoad();

	if (ReflectionCaptureBuildData.Num() > 0 
		// Only strip in PostLoad for cooked platforms.  Uncooked may need to generate encoded HDR data in UReflectionCaptureComponent::OnRegister().
		&& FPlatformProperties::RequiresCookedData())
	{
		// We already stripped unneeded formats during cooking, but some cooking targets require multiple formats to be stored
		// Strip unneeded formats for the current max feature level
		bool bRetainAllFeatureLevelData = GIsEditor && GMaxRHIFeatureLevel >= ERHIFeatureLevel::SM4;
		bool bEncodedDataRequired = bRetainAllFeatureLevelData || (GMaxRHIFeatureLevel == ERHIFeatureLevel::ES2 || GMaxRHIFeatureLevel == ERHIFeatureLevel::ES3_1);
		bool bFullDataRequired = GMaxRHIFeatureLevel >= ERHIFeatureLevel::SM4;

		for (TMap<FGuid, FReflectionCaptureMapBuildData>::TIterator It(ReflectionCaptureBuildData); It; ++It)
		{
			FReflectionCaptureMapBuildData& CaptureBuildData = It.Value();

			if (!bFullDataRequired)
			{
				CaptureBuildData.FullHDRCapturedData.Empty();
			}

			if (!bEncodedDataRequired)
			{
				CaptureBuildData.EncodedHDRCapturedData.Empty();
			}

			check(CaptureBuildData.FullHDRCapturedData.Num() > 0 || CaptureBuildData.EncodedHDRCapturedData.Num() > 0);
		}
	}
}

void UMapBuildDataRegistry::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);

	UMapBuildDataRegistry* TypedThis = Cast<UMapBuildDataRegistry>(InThis);
	check(TypedThis);

	for (TMap<FGuid, FMeshMapBuildData>::TIterator It(TypedThis->MeshBuildData); It; ++It)
	{
		It.Value().AddReferencedObjects(Collector);
	}
}

void UMapBuildDataRegistry::BeginDestroy()
{
	Super::BeginDestroy();

	ReleaseResources();

	// Start a fence to track when BeginReleaseResource has completed
	DestroyFence.BeginFence();
}

bool UMapBuildDataRegistry::IsReadyForFinishDestroy()
{
	return Super::IsReadyForFinishDestroy() && DestroyFence.IsFenceComplete();
}

void UMapBuildDataRegistry::FinishDestroy()
{
	Super::FinishDestroy();

	EmptyLevelData();
}

FMeshMapBuildData& UMapBuildDataRegistry::AllocateMeshBuildData(const FGuid& MeshId, bool bMarkDirty)
{
	check(MeshId.IsValid());

	if (bMarkDirty)
	{
		MarkPackageDirty();
	}

	return MeshBuildData.Add(MeshId, FMeshMapBuildData());
}

const FMeshMapBuildData* UMapBuildDataRegistry::GetMeshBuildData(FGuid MeshId) const
{
	return MeshBuildData.Find(MeshId);
}

FMeshMapBuildData* UMapBuildDataRegistry::GetMeshBuildData(FGuid MeshId)
{
	return MeshBuildData.Find(MeshId);
}

FPrecomputedLightVolumeData& UMapBuildDataRegistry::AllocateLevelPrecomputedLightVolumeBuildData(const FGuid& LevelId)
{
	check(LevelId.IsValid());
	MarkPackageDirty();
	return *LevelPrecomputedLightVolumeBuildData.Add(LevelId, new FPrecomputedLightVolumeData());
}

void UMapBuildDataRegistry::AddLevelPrecomputedLightVolumeBuildData(const FGuid& LevelId, FPrecomputedLightVolumeData* InData)
{
	check(LevelId.IsValid());
	LevelPrecomputedLightVolumeBuildData.Add(LevelId, InData);
}

const FPrecomputedLightVolumeData* UMapBuildDataRegistry::GetLevelPrecomputedLightVolumeBuildData(FGuid LevelId) const
{
	const FPrecomputedLightVolumeData* const * DataPtr = LevelPrecomputedLightVolumeBuildData.Find(LevelId);

	if (DataPtr)
	{
		return *DataPtr;
	}

	return NULL;
}

FPrecomputedLightVolumeData* UMapBuildDataRegistry::GetLevelPrecomputedLightVolumeBuildData(FGuid LevelId)
{
	FPrecomputedLightVolumeData** DataPtr = LevelPrecomputedLightVolumeBuildData.Find(LevelId);

	if (DataPtr)
	{
		return *DataPtr;
	}

	return NULL;
}

FPrecomputedVolumetricLightmapData& UMapBuildDataRegistry::AllocateLevelPrecomputedVolumetricLightmapBuildData(const FGuid& LevelId)
{
	check(LevelId.IsValid());
	MarkPackageDirty();
	return *LevelPrecomputedVolumetricLightmapBuildData.Add(LevelId, new FPrecomputedVolumetricLightmapData());
}

void UMapBuildDataRegistry::AddLevelPrecomputedVolumetricLightmapBuildData(const FGuid& LevelId, FPrecomputedVolumetricLightmapData* InData)
{
	check(LevelId.IsValid());
	LevelPrecomputedVolumetricLightmapBuildData.Add(LevelId, InData);
}

const FPrecomputedVolumetricLightmapData* UMapBuildDataRegistry::GetLevelPrecomputedVolumetricLightmapBuildData(FGuid LevelId) const
{
	const FPrecomputedVolumetricLightmapData* const * DataPtr = LevelPrecomputedVolumetricLightmapBuildData.Find(LevelId);

	if (DataPtr)
	{
		return *DataPtr;
	}

	return NULL;
}

FPrecomputedVolumetricLightmapData* UMapBuildDataRegistry::GetLevelPrecomputedVolumetricLightmapBuildData(FGuid LevelId)
{
	FPrecomputedVolumetricLightmapData** DataPtr = LevelPrecomputedVolumetricLightmapBuildData.Find(LevelId);

	if (DataPtr)
	{
		return *DataPtr;
	}

	return NULL;
}

FLightComponentMapBuildData& UMapBuildDataRegistry::FindOrAllocateLightBuildData(FGuid LightId, bool bMarkDirty)
{
	check(LightId.IsValid());

	if (bMarkDirty)
	{
		MarkPackageDirty();
	}

	return LightBuildData.FindOrAdd(LightId);
}

const FLightComponentMapBuildData* UMapBuildDataRegistry::GetLightBuildData(FGuid LightId) const
{
	return LightBuildData.Find(LightId);
}

FLightComponentMapBuildData* UMapBuildDataRegistry::GetLightBuildData(FGuid LightId)
{
	return LightBuildData.Find(LightId);
}

FReflectionCaptureMapBuildData& UMapBuildDataRegistry::AllocateReflectionCaptureBuildData(const FGuid& CaptureId, bool bMarkDirty)
{
	check(CaptureId.IsValid());

	if (bMarkDirty)
	{
		MarkPackageDirty();
	}

	return ReflectionCaptureBuildData.Add(CaptureId, FReflectionCaptureMapBuildData());
}

const FReflectionCaptureMapBuildData* UMapBuildDataRegistry::GetReflectionCaptureBuildData(FGuid CaptureId) const
{
	return ReflectionCaptureBuildData.Find(CaptureId);
}

FReflectionCaptureMapBuildData* UMapBuildDataRegistry::GetReflectionCaptureBuildData(FGuid CaptureId)
{
	return ReflectionCaptureBuildData.Find(CaptureId);
}

void UMapBuildDataRegistry::InvalidateStaticLighting(UWorld* World, const TSet<FGuid>* ResourcesToKeep)
{
	if (MeshBuildData.Num() > 0 || LightBuildData.Num() > 0)
	{
		FGlobalComponentRecreateRenderStateContext Context;

		if (!ResourcesToKeep || !ResourcesToKeep->Num())
		{
			MeshBuildData.Empty();
			LightBuildData.Empty();
		}
		else // Otherwise keep any resource if it's guid is in ResourcesToKeep.
		{
			TMap<FGuid, FMeshMapBuildData> PrevMeshData;
			TMap<FGuid, FLightComponentMapBuildData> PrevLightData;
			FMemory::Memswap(&MeshBuildData , &PrevMeshData, sizeof(MeshBuildData));
			FMemory::Memswap(&LightBuildData , &PrevLightData, sizeof(LightBuildData));

			for (const FGuid& Guid : *ResourcesToKeep)
			{
				const FMeshMapBuildData* MeshData = PrevMeshData.Find(Guid);
				if (MeshData)
				{
					MeshBuildData.Add(Guid, *MeshData);
					continue;
				}

				const FLightComponentMapBuildData* LightData = PrevLightData.Find(Guid);
				if (LightData)
				{
					LightBuildData.Add(Guid, *LightData);
					continue;
				}
			}
		}

		MarkPackageDirty();
	}

	if (LevelPrecomputedLightVolumeBuildData.Num() > 0 || LevelPrecomputedVolumetricLightmapBuildData.Num() > 0)
	{
		for (int32 LevelIndex = 0; LevelIndex < World->GetNumLevels(); LevelIndex++)
		{
			World->GetLevel(LevelIndex)->ReleaseRenderingResources();
		}

		ReleaseResources(ResourcesToKeep);

		// Make sure the RT has processed the release command before we delete any FPrecomputedLightVolume's
		FlushRenderingCommands();

		EmptyLevelData(ResourcesToKeep);

		MarkPackageDirty();
	}
}

void UMapBuildDataRegistry::InvalidateReflectionCaptures(const TSet<FGuid>* ResourcesToKeep)
{
	if (ReflectionCaptureBuildData.Num() > 0)
	{
		FGlobalComponentRecreateRenderStateContext Context;

		TMap<FGuid, FReflectionCaptureMapBuildData> PrevReflectionCapturedData;
		FMemory::Memswap(&ReflectionCaptureBuildData , &PrevReflectionCapturedData, sizeof(ReflectionCaptureBuildData));

		for (TMap<FGuid, FReflectionCaptureMapBuildData>::TIterator It(PrevReflectionCapturedData); It; ++It)
		{
			// Keep any resource if it's guid is in ResourcesToKeep.
			if (ResourcesToKeep && ResourcesToKeep->Contains(It.Key()))
			{
				ReflectionCaptureBuildData.Add(It.Key(), It.Value());
			}
		}

		MarkPackageDirty();
	}
}

bool UMapBuildDataRegistry::IsLegacyBuildData() const
{
	return GetOutermost()->ContainsMap();
}

void UMapBuildDataRegistry::ReleaseResources(const TSet<FGuid>* ResourcesToKeep)
{
	for (TMap<FGuid, FPrecomputedVolumetricLightmapData*>::TIterator It(LevelPrecomputedVolumetricLightmapBuildData); It; ++It)
	{
		if (!ResourcesToKeep || !ResourcesToKeep->Contains(It.Key()))
		{
			BeginReleaseResource(It.Value());
		}
	}
}

void UMapBuildDataRegistry::EmptyLevelData(const TSet<FGuid>* ResourcesToKeep)
{
	TMap<FGuid, FPrecomputedLightVolumeData*> PrevPrecomputedLightVolumeData;
	TMap<FGuid, FPrecomputedVolumetricLightmapData*> PrevPrecomputedVolumetricLightmapData;
	FMemory::Memswap(&LevelPrecomputedLightVolumeBuildData , &PrevPrecomputedLightVolumeData, sizeof(LevelPrecomputedLightVolumeBuildData));
	FMemory::Memswap(&LevelPrecomputedVolumetricLightmapBuildData , &PrevPrecomputedVolumetricLightmapData, sizeof(LevelPrecomputedVolumetricLightmapBuildData));

	for (TMap<FGuid, FPrecomputedLightVolumeData*>::TIterator It(PrevPrecomputedLightVolumeData); It; ++It)
	{
		// Keep any resource if it's guid is in ResourcesToKeep.
		if (!ResourcesToKeep || !ResourcesToKeep->Contains(It.Key()))
		{
			delete It.Value();
		}
		else
		{
			LevelPrecomputedLightVolumeBuildData.Add(It.Key(), It.Value());
		}
	}

	for (TMap<FGuid, FPrecomputedVolumetricLightmapData*>::TIterator It(PrevPrecomputedVolumetricLightmapData); It; ++It)
	{
		// Keep any resource if it's guid is in ResourcesToKeep.
		if (!ResourcesToKeep || !ResourcesToKeep->Contains(It.Key()))
		{
			delete It.Value();
		}
		else
		{
			LevelPrecomputedVolumetricLightmapBuildData.Add(It.Key(), It.Value());
		}
	}
}

FUObjectAnnotationSparse<FMeshMapBuildLegacyData, true> GComponentsWithLegacyLightmaps;
FUObjectAnnotationSparse<FLevelLegacyMapBuildData, true> GLevelsWithLegacyBuildData;
FUObjectAnnotationSparse<FLightComponentLegacyMapBuildData, true> GLightComponentsWithLegacyBuildData;
FUObjectAnnotationSparse<FReflectionCaptureMapBuildLegacyData, true> GReflectionCapturesWithLegacyBuildData;
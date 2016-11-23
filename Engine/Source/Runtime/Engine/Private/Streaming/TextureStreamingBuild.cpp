// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
TextureStreamingBuild.cpp : Contains definitions to build texture streaming data.
=============================================================================*/

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "Misc/ScopedSlowTask.h"
#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/TextureStreamingTypes.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FeedbackContext.h"
#include "Engine/Texture2D.h"
#include "DebugViewModeMaterialProxy.h"
#include "ShaderCompiler.h"
#include "Engine/StaticMesh.h"
#include "Streaming/TextureStreamingHelpers.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY(TextureStreamingBuild);
#define LOCTEXT_NAMESPACE "TextureStreamingBuild"

static int32 GetNumActorsInWorld(UWorld* InWorld)
{
	int32 ActorCount = 0;
	for (int32 LevelIndex = 0; LevelIndex < InWorld->GetNumLevels(); LevelIndex++)
	{
		ULevel* Level = InWorld->GetLevel(LevelIndex);
		if (!Level)
		{
			continue;
		}

		ActorCount += Level->Actors.Num();
	}
	return ActorCount;
}

bool WaitForShaderCompilation(const FText& Message, FSlowTask& BuildTextureStreamingTask)
{
	FlushRenderingCommands();

	const int32 NumShadersToBeCompiled = GShaderCompilingManager->GetNumRemainingJobs();
	int32 RemainingShaders = NumShadersToBeCompiled;
	if (NumShadersToBeCompiled > 0)
	{
		FScopedSlowTask SlowTask(1.f, Message);

		while (RemainingShaders > 0)
		{
			FPlatformProcess::Sleep(0.01f);
			GShaderCompilingManager->ProcessAsyncResults(false, true);

			const int32 RemainingShadersThisFrame = GShaderCompilingManager->GetNumRemainingJobs();
			if (RemainingShadersThisFrame > 0)
			{
				const int32 NumberOfShadersCompiledThisFrame = RemainingShaders - RemainingShadersThisFrame;

				const float FrameProgress = (float)NumberOfShadersCompiledThisFrame / (float)NumShadersToBeCompiled;
				BuildTextureStreamingTask.EnterProgressFrame(FrameProgress);
				SlowTask.EnterProgressFrame(FrameProgress);
				if (GWarn->ReceivedUserCancel())
				{
					return false;
				}
			}
			RemainingShaders = RemainingShadersThisFrame;
		}
	}
	else
	{
		BuildTextureStreamingTask.EnterProgressFrame();
		if (GWarn->ReceivedUserCancel())
		{
			return false;
		}
	}

	// Extra safety to make sure every shader map is updated
	GShaderCompilingManager->FinishAllCompilation();
	FlushRenderingCommands();

	return true;
}

/** Get the list of all material used in a world 
 *
 * @return true if the operation is a success, false if it was canceled.
 */
ENGINE_API bool GetTextureStreamingBuildMaterials(UWorld* InWorld, OUT TSet<UMaterialInterface*>& OutMaterials, FSlowTask& BuildTextureStreamingTask)
{
#if WITH_EDITORONLY_DATA
	if (!InWorld)
	{
		return false;
	}

	const bool bUseNewMetrics = CVarStreamingUseNewMetrics.GetValueOnGameThread() != 0;
	if (!bUseNewMetrics)
	{
		return true; // Old path use no texture scale.
	}

	const int32 NumActorsInWorld = GetNumActorsInWorld(InWorld);
	if (!NumActorsInWorld)
	{
		BuildTextureStreamingTask.EnterProgressFrame();
		return true;
	}

	const float OneOverNumActorsInWorld = 1.f / (float)NumActorsInWorld;

	FScopedSlowTask SlowTask(1.f, (LOCTEXT("TextureStreamingBuild_GetTextureStreamingBuildMaterials", "Getting materials to rebuild")));

	for (int32 LevelIndex = 0; LevelIndex < InWorld->GetNumLevels(); LevelIndex++)
	{
		ULevel* Level = InWorld->GetLevel(LevelIndex);
		if (!Level)
		{
			continue;
		}

		for (AActor* Actor : Level->Actors)
		{
			BuildTextureStreamingTask.EnterProgressFrame(OneOverNumActorsInWorld);
			SlowTask.EnterProgressFrame(OneOverNumActorsInWorld);
			if (GWarn->ReceivedUserCancel())
			{
				return false;
			}

			// Check the actor after incrementing the progress.
			if (!Actor)
			{
				continue;
			}

			TInlineComponentArray<UPrimitiveComponent*> Primitives;
			Actor->GetComponents<UPrimitiveComponent>(Primitives);

			for (UPrimitiveComponent* Primitive : Primitives)
			{
				if (!Primitive)
				{
					continue;
				}

				TArray<UMaterialInterface*> Materials;
				Primitive->GetUsedMaterials(Materials);

				for (UMaterialInterface* Material : Materials)
				{
					if (Material)
					{
						OutMaterials.Add(Material);
					}
				}
			}
		}
	}
	return true;
#else
	return false;
#endif
}

/**
 * Build Shaders to compute scales per texture.
 *
 * @param QualityLevel		The quality level for the shaders.
 * @param FeatureLevel		The feature level for the shaders.
 * @param bFullRebuild		Clear all debug shaders before generating the new ones..
 * @param bWaitForPreviousShaders Whether to wait for previous shaders to complete.
 * @param Materials			The materials to update, the one that failed compilation will be removed (IN OUT).
 * @return true if the operation is a success, false if it was canceled.
 */
ENGINE_API bool CompileTextureStreamingShaders(EMaterialQualityLevel::Type QualityLevel, ERHIFeatureLevel::Type FeatureLevel, bool bFullRebuild, bool bWaitForPreviousShaders, TSet<UMaterialInterface*>& Materials, FSlowTask& BuildTextureStreamingTask)
{
#if WITH_EDITORONLY_DATA
	if (!GShaderCompilingManager)
	{
		return false;
	}

	check(Materials.Num())

	// Finish compiling pending shaders first.
	if (!bWaitForPreviousShaders)
	{
		FlushRenderingCommands();
	}
	else if (!WaitForShaderCompilation(LOCTEXT("TextureStreamingBuild_FinishPendingShadersCompilation", "Waiting For Pending Shaders Compilation"), BuildTextureStreamingTask))
	{
		return false;
	}

	const double StartTime = FPlatformTime::Seconds();
	const float OneOverNumMaterials = 1.f / (float)Materials.Num();

	if (bFullRebuild)
	{
		FDebugViewModeMaterialProxy::ClearAllShaders();
	}

	TArray<UMaterialInterface*> MaterialsToRemove;
	
	for (UMaterialInterface* MaterialInterface : Materials)
	{
		check(MaterialInterface); // checked for null in GetTextureStreamingBuildMaterials

		const FMaterial* Material = MaterialInterface->GetMaterialResource(FeatureLevel);
		if (!Material)
		{
			continue;
		}

		if (Material->IsUsedWithLandscape())
		{
			UE_LOG(TextureStreamingBuild, Verbose, TEXT("Landscape material %s not supported, skipping shader"), *MaterialInterface->GetName());
			MaterialsToRemove.Add(MaterialInterface);
			continue;
		}

		FDebugViewModeMaterialProxy::AddShader(MaterialInterface, QualityLevel, FeatureLevel, EMaterialShaderMapUsage::DebugViewModeTexCoordScale);
	}

	for (UMaterialInterface* RemovedMaterial : MaterialsToRemove)
	{
		Materials.Remove(RemovedMaterial);
	}

	if (WaitForShaderCompilation(LOCTEXT("TextureStreamingBuild_CompileTextureStreamingShaders", "Compiling Shaders For Material Analysis "), BuildTextureStreamingTask))
	{
		// Check The validity of all shaders, removing invalid entries
		FDebugViewModeMaterialProxy::ValidateAllShaders(Materials);

		UE_LOG(LogLevel, Display, TEXT("Compiling texture streaming analysis materials took %.3f seconds."), FPlatformTime::Seconds() - StartTime);
		return true;
	}
	else
	{
		FDebugViewModeMaterialProxy::ClearAllShaders();
		return false;
	}
#else
	return false;
#endif
}

ENGINE_API bool BuildTextureStreamingComponentData(UWorld* InWorld, EMaterialQualityLevel::Type QualityLevel, ERHIFeatureLevel::Type FeatureLevel, bool bFullRebuild, FSlowTask& BuildTextureStreamingTask)
{
#if WITH_EDITORONLY_DATA
	if (!InWorld)
	{
		return false;
	}

	const int32 NumActorsInWorld = GetNumActorsInWorld(InWorld);
	if (!NumActorsInWorld)
	{
		BuildTextureStreamingTask.EnterProgressFrame();
		// Can't early exit here as Level might need reset.
		// return true;
	}

	const double StartTime = FPlatformTime::Seconds();
	const float OneOverNumActorsInWorld = 1.f / (float)FMath::Max<int32>(NumActorsInWorld, 1); // Prevent div by 0

	// Used to reset per level index for textures.
	TArray<UTexture2D*> AllTextures;
	for (FObjectIterator Iter(UTexture2D::StaticClass()); Iter && bFullRebuild; ++Iter)
	{
		UTexture2D* Texture2D = Cast<UTexture2D>(*Iter);
		if (Texture2D)
		{
			AllTextures.Add(Texture2D);
		}
	}

	FScopedSlowTask SlowTask(1.f, (LOCTEXT("TextureStreamingBuild_ComponentDataUpdate", "Updating Component Data")));
	
	for (int32 LevelIndex = 0; LevelIndex < InWorld->GetNumLevels(); LevelIndex++)
	{
		ULevel* Level = InWorld->GetLevel(LevelIndex);
		if (!Level)
		{
			continue;
		}

		const bool bHadBuildData = Level->StreamingTextureGuids.Num() > 0 || Level->TextureStreamingResourceGuids.Num() > 0;

		Level->NumTextureStreamingUnbuiltComponents = 0;

		// When not rebuilding everything, we can't update those as we don't know how the current build data was computed.
		// Consequently, partial rebuilds are not allowed to recompute everything. What something is missing and can not be built,
		// the BuildStreamingData will return false in which case we increment NumTextureStreamingUnbuiltComponents.
		// This allows to keep track of full rebuild requirements.
		if (bFullRebuild)
		{
			Level->bTextureStreamingRotationChanged = false;
			Level->StreamingTextureGuids.Empty();
			Level->TextureStreamingResourceGuids.Empty();
			Level->NumTextureStreamingDirtyResources = 0; // This is persistent in order to be able to notify if a rebuild is required when running a cooked build.
		}

		TSet<FGuid> ResourceGuids;

		for (AActor* Actor : Level->Actors)
		{
			BuildTextureStreamingTask.EnterProgressFrame(OneOverNumActorsInWorld);
			SlowTask.EnterProgressFrame(OneOverNumActorsInWorld);
			if (GWarn->ReceivedUserCancel())
			{
				return false;
			}

			// Check the actor after incrementing the progress.
			if (!Actor)
			{
				continue; 
			}

			TInlineComponentArray<UPrimitiveComponent*> Primitives;
			Actor->GetComponents<UPrimitiveComponent>(Primitives);

			for (UPrimitiveComponent* Primitive : Primitives)
			{
				//@TODO : Non transactional primitives, like the one created from blueprints, fail to save/load their built data.
				if (!Primitive || !Primitive->HasAnyFlags(RF_Transactional))
				{
					continue;
				}

				if (!Primitive->BuildTextureStreamingData(bFullRebuild ? TSB_MapBuild : TSB_ViewMode, QualityLevel, FeatureLevel, ResourceGuids))
				{
					++Level->NumTextureStreamingUnbuiltComponents;
				}
			}
		}

		if (bFullRebuild)
		{
			// Reset LevelIndex to default for next use and build the level Guid array.
			for (UTexture2D* Texture2D : AllTextures)
			{
				checkSlow(Texture2D);
				Texture2D->LevelIndex = INDEX_NONE;
			}

			// Cleanup the asset references.
			ResourceGuids.Remove(FGuid()); // Remove the invalid guid.
			for (const FGuid& ResourceGuid : ResourceGuids)
			{
				Level->TextureStreamingResourceGuids.Add(ResourceGuid);
			}

			// Mark for resave if and only if rebuilding everything and also if data was updated.
			const bool bHasBuildData = Level->StreamingTextureGuids.Num() > 0 || Level->TextureStreamingResourceGuids.Num() > 0;
			if (bHadBuildData || bHasBuildData)
			{
				Level->MarkPackageDirty();
			}
		}
	}

	// Update TextureStreamer
	ULevel::BuildStreamingData(InWorld);

	UE_LOG(TextureStreamingBuild, Display, TEXT("Build Texture Streaming took %.3f seconds."), FPlatformTime::Seconds() - StartTime);
	return true;
#else
	UE_LOG(TextureStreamingBuild, Fatal,TEXT("Build Texture Streaming should not be called on a console"));
	return false;
#endif
}

#undef LOCTEXT_NAMESPACE

static uint32 PackRelativeBox(const FVector& RefOrigin, const FVector& RefExtent, const FVector& Origin, const FVector& Extent)
{
	const FVector RefMin = RefOrigin - RefExtent;
	// 15.5 and 31.5 have the / 2 scale included 
	const FVector PackScale = FVector(15.5f, 15.5f, 31.5f) / RefExtent.ComponentMax(FVector(KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER));

	const FVector Min = Origin - Extent;
	const FVector Max = Origin + Extent;

	const FVector RelMin = (Min - RefMin) * PackScale;
	const FVector RelMax = (Max - RefMin) * PackScale;

	const uint32 PackedMinX = (uint32)FMath::Clamp<int32>(FMath::FloorToInt(RelMin.X), 0, 31);
	const uint32 PackedMinY = (uint32)FMath::Clamp<int32>(FMath::FloorToInt(RelMin.Y), 0, 31);
	const uint32 PackedMinZ = (uint32)FMath::Clamp<int32>(FMath::FloorToInt(RelMin.Z), 0, 63);

	const uint32 PackedMaxX = (uint32)FMath::Clamp<int32>(FMath::CeilToInt(RelMax.X), 0, 31);
	const uint32 PackedMaxY = (uint32)FMath::Clamp<int32>(FMath::CeilToInt(RelMax.Y), 0, 31);
	const uint32 PackedMaxZ = (uint32)FMath::Clamp<int32>(FMath::CeilToInt(RelMax.Z), 0, 63);

	return PackedMinX | (PackedMinY << 5) | (PackedMinZ << 10) | (PackedMaxX << 16) | (PackedMaxY << 21) | (PackedMaxZ << 26);
}

static void UnpackRelativeBox(const FVector& InRefOrigin, const FVector& InRefExtent, uint32 InPackedRelBox, FVector& OutOrigin, FVector& OutExtent)
{
	const uint32 PackedMinX = InPackedRelBox & 31;
	const uint32 PackedMinY = (InPackedRelBox >> 5) & 31;
	const uint32 PackedMinZ = (InPackedRelBox >> 10) & 63;

	const uint32 PackedMaxX = (InPackedRelBox >> 16) & 31;
	const uint32 PackedMaxY = (InPackedRelBox >> 21) & 31;
	const uint32 PackedMaxZ = (InPackedRelBox >> 26) & 63;

	const FVector RefMin = InRefOrigin - InRefExtent;
	// 15.5 and 31.5 have the / 2 scale included 
	const FVector UnpackScale = InRefExtent.ComponentMax(FVector(KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER)) / FVector(15.5f, 15.5f, 31.5f);

	const FVector Min = FVector((float)PackedMinX, (float)PackedMinY, (float)PackedMinZ) * UnpackScale + RefMin;
	const FVector Max = FVector((float)PackedMaxX, (float)PackedMaxY, (float)PackedMaxZ) * UnpackScale + RefMin;

	OutOrigin = .5 * (Min + Max);
	OutExtent = .5 * (Max - Min);
}

void FStreamingTexturePrimitiveInfo::UnPackFrom(UTexture2D* InTexture, float ExtraScale, const FBoxSphereBounds& RefBounds, const FStreamingTextureBuildInfo& Info, bool bUseRelativeBox)
{
	check(InTexture->LevelIndex == Info.TextureLevelIndex);
	Texture = InTexture;

	if (bUseRelativeBox)
	{
		UnpackRelativeBox(RefBounds.Origin, RefBounds.BoxExtent, Info.PackedRelativeBox, Bounds.Origin, Bounds.BoxExtent);
		Bounds.SphereRadius = Bounds.BoxExtent.Size();
	}
	else
	{
		Bounds = RefBounds;
	}

	TexelFactor = Info.TexelFactor * ExtraScale;
}

void FStreamingTextureBuildInfo::PackFrom(ULevel* Level, const FBoxSphereBounds& RefBounds, const FStreamingTexturePrimitiveInfo& Info)
{
	check(Level);

	PackedRelativeBox = PackRelativeBox(RefBounds.Origin, RefBounds.BoxExtent, Info.Bounds.Origin, Info.Bounds.BoxExtent);

	UTexture2D* Texture2D = CastChecked<UTexture2D>(Info.Texture);
	if (Texture2D->LevelIndex == INDEX_NONE)
	{
		// If this is the first time this texture gets processed in the packing process, encode it.
		Texture2D->LevelIndex = Level->StreamingTextureGuids.Add(Texture2D->GetLightingGuid());
	}
	TextureLevelIndex = (uint16)Texture2D->LevelIndex;

	TexelFactor = Info.TexelFactor;
}

void FStreamingTextureLevelContext::InitFromLevel(const ULevel* InLevel)
{
	if (InLevel)
	{
		// Build the map to convert from a guid to the level index. 
		// The level index of an array is also the level index.
		// @TODO : we could save a indirect array of sorted texture GUID instead of rebuilding it everytime.
		TextureGuidToLevelIndex.Reserve(InLevel->StreamingTextureGuids.Num());
		for (int32 TextureIndex = 0; TextureIndex < InLevel->StreamingTextureGuids.Num(); ++TextureIndex)
		{
			TextureGuidToLevelIndex.Add(InLevel->StreamingTextureGuids[TextureIndex], TextureIndex);
		}

		// Extra transient data for each texture.
		BoundStates.AddZeroed(InLevel->StreamingTextureGuids.Num());
	}
}

FStreamingTextureLevelContext::FStreamingTextureLevelContext(EMaterialQualityLevel::Type InQualityLevel, const UPrimitiveComponent* Primitive)
: bUseRelativeBoxes(false)
, BuildDataTimestamp(0)
, ComponentBuildData(nullptr)
, QualityLevel(InQualityLevel)
, FeatureLevel(GMaxRHIFeatureLevel)
{
	if (Primitive)
	{
		const UWorld* World = Primitive->GetWorld();
		if (World)
		{
			FeatureLevel = World->FeatureLevel;
		}
	}
}

FStreamingTextureLevelContext::FStreamingTextureLevelContext(EMaterialQualityLevel::Type InQualityLevel, const ULevel* InLevel) 
: bUseRelativeBoxes(InLevel ? !InLevel->bTextureStreamingRotationChanged : false)
, BuildDataTimestamp(0)
, ComponentBuildData(nullptr)
, QualityLevel(InQualityLevel)
, FeatureLevel(GMaxRHIFeatureLevel)
{
	if (InLevel)
	{
		const UWorld* World = InLevel->GetWorld();
		if (World)
		{
			FeatureLevel = World->FeatureLevel;
		}
	}

	InitFromLevel(InLevel);
}


FStreamingTextureLevelContext::FStreamingTextureLevelContext(EMaterialQualityLevel::Type InQualityLevel, ERHIFeatureLevel::Type InFeatureLevel, const ULevel* InLevel) 
: bUseRelativeBoxes(InLevel ? !InLevel->bTextureStreamingRotationChanged : false)
, BuildDataTimestamp(0)
, ComponentBuildData(nullptr)
, QualityLevel(InQualityLevel)
, FeatureLevel(GMaxRHIFeatureLevel)
{
	InitFromLevel(InLevel);
}

FStreamingTextureLevelContext::~FStreamingTextureLevelContext()
{
	// Reset the level indices for the next use.
	for (const FTextureBoundState& BoundState : BoundStates)
	{
		if (BoundState.Texture)
		{
			BoundState.Texture->LevelIndex = INDEX_NONE;
		}
	}
}

void FStreamingTextureLevelContext::BindBuildData(const TArray<FStreamingTextureBuildInfo>* BuildData)
{
	// Increment the component timestamp, used to know when a texture is processed by a component for the first time.
	// Using a timestamp allows to not reset state in between components.
	++BuildDataTimestamp;

	if (TextureGuidToLevelIndex.Num() > 0) // No point in binding data if there is no possible remapping.
	{
		// Process the build data in order to be able to map a texture object to the build data entry.
		ComponentBuildData = BuildData;
		if (BuildData && BoundStates.Num() > 0)
		{
			for (int32 Index = 0; Index < BuildData->Num(); ++Index)
			{
				int32 TextureLevelIndex = (*BuildData)[Index].TextureLevelIndex;
				if (BoundStates.IsValidIndex(TextureLevelIndex))
				{
					FTextureBoundState& BoundState = BoundStates[TextureLevelIndex];
					BoundState.BuildDataIndex = Index; // The index of this texture in the component build data.
					BoundState.BuildDataTimestamp = BuildDataTimestamp; // The component timestamp will indicate that the index is valid to be used.
				}
			}
		}
	}
	else
	{
		ComponentBuildData = nullptr;
	}
}

int32* FStreamingTextureLevelContext::GetBuildDataIndexRef(UTexture2D* Texture2D)
{
	if (ComponentBuildData) // If there is some build data to map to.
	{
		if (Texture2D->LevelIndex == INDEX_NONE)
		{
			const int32* LevelIndex = TextureGuidToLevelIndex.Find(Texture2D->GetLightingGuid());
			if (LevelIndex) // If the index is found in the map, the index is valid in BoundStates
			{
				Texture2D->LevelIndex = *LevelIndex;
				BoundStates[*LevelIndex].Texture = Texture2D; // Update the mapping now!
			}
			else // Otherwise add a dummy entry to prevent having to search in the map multiple times.
			{
				Texture2D->LevelIndex = BoundStates.Add(FTextureBoundState(Texture2D));
			}
		}

		FTextureBoundState& BoundState = BoundStates[Texture2D->LevelIndex];
		if (BoundState.BuildDataTimestamp == BuildDataTimestamp)
		{
			return &BoundState.BuildDataIndex; // Only return the bound static if it has data relative to this component.
		}
	}
	return nullptr;
}

void FStreamingTextureLevelContext::ProcessMaterial(const FPrimitiveMaterialInfo& MaterialData, float ComponentScaling, TArray<FStreamingTexturePrimitiveInfo>& OutStreamingTextures)
{
	ensure(MaterialData.IsValid());

	TArray<UTexture*> Textures;
	MaterialData.Material->GetUsedTextures(Textures, QualityLevel, false, FeatureLevel, false);

	for (UTexture* Texture : Textures)
	{
		UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
		if (!IsStreamingTexture(Texture2D))
		{
			continue;
		}

		int32* BuildDataIndex = GetBuildDataIndexRef(Texture2D);
		if (BuildDataIndex)
		{
			if (*BuildDataIndex != INDEX_NONE)
			{
				FStreamingTexturePrimitiveInfo& Info = *new(OutStreamingTextures) FStreamingTexturePrimitiveInfo();
				Info.UnPackFrom(Texture2D, ComponentScaling, MaterialData.Bounds, (*ComponentBuildData)[*BuildDataIndex], bUseRelativeBoxes);

				// Indicate that this texture build data has already been processed.
				// The build data use the merged results of all material so it only needs to be processed once.
				*BuildDataIndex = INDEX_NONE;
			}
		}
		else // Otherwise create an entry using the available data.
		{
			float TextureDensity = MaterialData.Material->GetTextureDensity(Texture->GetFName(), *MaterialData.UVChannelData);

			if (!TextureDensity)
			{
				// Fallback assuming a sampling scale of 1 using the UV channel 0;
				TextureDensity = MaterialData.UVChannelData->LocalUVDensities[0];
			}

			if (TextureDensity)
			{
				FStreamingTexturePrimitiveInfo& Info = *new(OutStreamingTextures) FStreamingTexturePrimitiveInfo();
				Info.Texture = Texture2D;
				Info.TexelFactor = TextureDensity * ComponentScaling;
				Info.Bounds = MaterialData.Bounds;
			}
		}
	}
}

void CheckTextureStreamingBuildValidity(UWorld* InWorld)
{
	if (!InWorld)
	{
		return;
	}

	InWorld->NumTextureStreamingUnbuiltComponents = 0;
	InWorld->NumTextureStreamingDirtyResources = 0;

	if (CVarStreamingCheckBuildStatus.GetValueOnAnyThread() > 0)
	{
		for (int32 LevelIndex = 0; LevelIndex < InWorld->GetNumLevels(); LevelIndex++)
		{
			ULevel* Level = InWorld->GetLevel(LevelIndex);
			if (!Level)
			{
				continue;
			}

#if WITH_EDITORONLY_DATA // Only rebuild the data in editor 
			if (FPlatformProperties::HasEditorOnlyData())
			{
				TSet<FGuid> ResourceGuids;
				Level->NumTextureStreamingUnbuiltComponents = 0;

				for (AActor* Actor : Level->Actors)
				{
					// Check the actor after incrementing the progress.
					if (!Actor)
					{
						continue;
					}

					TInlineComponentArray<UPrimitiveComponent*> Primitives;
					Actor->GetComponents<UPrimitiveComponent>(Primitives);

					for (UPrimitiveComponent* Primitive : Primitives)
					{
						//@TODO : Non transactional primitives, like the one created from blueprints, fail to save/load their built data.
						if (!Primitive || !Primitive->HasAnyFlags(RF_Transactional))
						{
							continue;
						}

						// Quality and feature level irrelevant in validation.
						if (!Primitive->BuildTextureStreamingData(TSB_ValidationOnly, EMaterialQualityLevel::Num, ERHIFeatureLevel::Num, ResourceGuids))
						{
							++Level->NumTextureStreamingUnbuiltComponents;
						}
					}
				}

				for (const FGuid& Guid : Level->TextureStreamingResourceGuids)
				{
					// If some Guid does not exists anymore, that means the resource changed.
					if (!ResourceGuids.Contains(Guid))
					{
						Level->NumTextureStreamingDirtyResources += 1;
					}
					ResourceGuids.Add(Guid);
				}

				// Don't mark package dirty as we avoid marking package dirty unless user changes something.
			}
#endif
			InWorld->NumTextureStreamingUnbuiltComponents += Level->NumTextureStreamingUnbuiltComponents;
			InWorld->NumTextureStreamingDirtyResources += Level->NumTextureStreamingDirtyResources;
		}
	}
}

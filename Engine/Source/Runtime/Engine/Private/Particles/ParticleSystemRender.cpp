// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ParticleSystemRender.cpp: Particle system rendering functions.
=============================================================================*/
#include "EnginePrivate.h"
#include "ParticleDefinitions.h"
#include "EngineMaterialClasses.h"
#include "DiagnosticTable.h"
#include "ParticleResources.h"

/** 
 * Whether to track particle rendering stats.  
 * Enable with the TRACKPARTICLERENDERINGSTATS command. 
 */
bool GTrackParticleRenderingStats = false;

/** Seconds between stat captures. */
float GTimeBetweenParticleRenderStatCaptures = 5.0f;

/** Minimum render time for a single DrawDynamicElements call that should be recorded. */
float GMinParticleDrawTimeToTrack = .0001f;

/** Whether to do LOD calculation on GameThread in game */
extern bool GbEnableGameThreadLODCalculation;

/*-----------------------------------------------------------------------------
	Stats.
-----------------------------------------------------------------------------*/

/** Stats gathered about each UParticleSystem that is rendered. */
struct FParticleTemplateRenderStats
{
	float MaxRenderTime;
	int32 NumDraws;
	int32 NumEmitters;
	int32 NumComponents;
	int32 NumDrawDynamicElements;

	FParticleTemplateRenderStats() :
		NumComponents(0),
		NumDrawDynamicElements(0)
	{}
};

/** Sorts FParticleTemplateRenderStats from longest MaxRenderTime to shortest. */
struct FCompareFParticleTemplateRenderStatsByRenderTime
{
	FORCEINLINE bool operator()( const FParticleTemplateRenderStats& A, const FParticleTemplateRenderStats& B ) const { return B.MaxRenderTime < A.MaxRenderTime; }
};

/** Stats gathered about each UParticleSystemComponent that is rendered. */
struct FParticleComponentRenderStats
{
	float MaxRenderTime;
	int32 NumDraws;
};

/** Sorts FParticleComponentRenderStats from longest MaxRenderTime to shortest. */
struct FCompareFParticleComponentRenderStatsByRenderTime
{
	FORCEINLINE bool operator()( const FParticleComponentRenderStats& A, const FParticleComponentRenderStats& B ) const { return B.MaxRenderTime < A.MaxRenderTime; }
};

/** Global map from UParticleSystem path name to stats about that particle system, used when tracking particle render stats. */
TMap<FString, FParticleTemplateRenderStats> GTemplateRenderStats;

/** Global map from UParticleSystemComponent path name to stats about that component, used when tracking particle render stats. */
TMap<FString, FParticleComponentRenderStats> GComponentRenderStats;

/** 
 * Dumps particle render stats to the game's log directory, and resets the stats tracked so far. 
 * This is hooked up to the DUMPPARTICLERENDERINGSTATS console command.
 */
void DumpParticleRenderingStats(FOutputDevice& Ar)
{
#if STATS
	if (GTrackParticleRenderingStats)
	{
		{
			// Have to keep this filename short enough to be valid for Xbox 360
			const FString TemplateFileName = FString::Printf(TEXT("%sPT-%s.csv"), *FPaths::ProfilingDir(), *FDateTime::Now().ToString());
			FDiagnosticTableViewer ParticleTemplateViewer(*TemplateFileName);

			// Write a row of headings for the table's columns.
			ParticleTemplateViewer.AddColumn(TEXT("MaxRenderTime ms"));
			ParticleTemplateViewer.AddColumn(TEXT("NumEmitters"));
			ParticleTemplateViewer.AddColumn(TEXT("NumDraws"));
			ParticleTemplateViewer.AddColumn(TEXT("Template Name"));
			ParticleTemplateViewer.CycleRow();

			// Sort from longest render time to shortest
			GTemplateRenderStats.ValueSort( FCompareFParticleTemplateRenderStatsByRenderTime() );

			for (TMap<FString, FParticleTemplateRenderStats>::TIterator It(GTemplateRenderStats); It; ++It)
			{	
				const FParticleTemplateRenderStats& Stats = It.Value();
				ParticleTemplateViewer.AddColumn(TEXT("%.2f"), Stats.MaxRenderTime * 1000.0f);
				ParticleTemplateViewer.AddColumn(TEXT("%u"), Stats.NumEmitters);
				ParticleTemplateViewer.AddColumn(TEXT("%u"), Stats.NumDraws);
				ParticleTemplateViewer.AddColumn(*It.Key());
				ParticleTemplateViewer.CycleRow();
			}
			Ar.Logf(TEXT("Template stats saved to %s"), *TemplateFileName);
		}
		
		{
			const FString ComponentFileName = FString::Printf(TEXT("%sPC-%s.csv"), *FPaths::ProfilingDir(), *FDateTime::Now().ToString());
			FDiagnosticTableViewer ParticleComponentViewer(*ComponentFileName);

			// Write a row of headings for the table's columns.
			ParticleComponentViewer.AddColumn(TEXT("MaxRenderTime ms"));
			ParticleComponentViewer.AddColumn(TEXT("NumDraws"));
			ParticleComponentViewer.AddColumn(TEXT("Actor Name"));
			ParticleComponentViewer.CycleRow();

			// Sort from longest render time to shortest
			GComponentRenderStats.ValueSort( FCompareFParticleComponentRenderStatsByRenderTime() );

			for (TMap<FString, FParticleComponentRenderStats>::TIterator It(GComponentRenderStats); It; ++It)
			{	
				const FParticleComponentRenderStats& Stats = It.Value();
				ParticleComponentViewer.AddColumn(TEXT("%.2f"), Stats.MaxRenderTime * 1000.0f);
				ParticleComponentViewer.AddColumn(TEXT("%u"), Stats.NumDraws);
				ParticleComponentViewer.AddColumn(*It.Key());
				ParticleComponentViewer.CycleRow();
			}
			Ar.Logf(TEXT("Component stats saved to %s"), *ComponentFileName);
		}

		GTemplateRenderStats.Empty();
		GComponentRenderStats.Empty();
	}
	else
	{
		Ar.Logf(TEXT("Need to start tracking with TRACKPARTICLERENDERINGSTATS first."));
	}
#endif
}

bool GWantsParticleStatsNextFrame = false;
bool GTrackParticleRenderingStatsForOneFrame = false;

#if STATS

/** Kicks off a particle render stat frame capture, initiated by the DUMPPARTICLEFRAMERENDERINGSTATS console command. */
void ENGINE_API BeginOneFrameParticleStats()
{
	if (GWantsParticleStatsNextFrame)
	{
		UE_LOG(LogParticles, Warning, TEXT("BeginOneFrameParticleStats"));
		GWantsParticleStatsNextFrame = false;
		// Block until the renderer processes all queued commands, because the rendering thread reads from GTrackParticleRenderingStatsForOneFrame
		FlushRenderingCommands();
		GTrackParticleRenderingStatsForOneFrame = true;
	}
}

TMap<FString, FParticleTemplateRenderStats> GOneFrameTemplateRenderStats;

/** 
 * Dumps particle render stats to the game's log directory along with a screenshot. 
 * This is hooked up to the DUMPPARTICLEFRAMERENDERINGSTATS console command.
 */
void ENGINE_API FinishOneFrameParticleStats()
{
	if (GTrackParticleRenderingStatsForOneFrame)
	{
		UE_LOG(LogParticles, Warning, TEXT("FinishOneFrameParticleStats"));

		// Wait until the rendering thread finishes processing the previous frame
		FlushRenderingCommands();

		GTrackParticleRenderingStatsForOneFrame = false;

		const FString PathFromScreenshots = FString(TEXT("Particle")) + FDateTime::Now().ToString();
		const FString WritePath = FPaths::ScreenShotDir() + PathFromScreenshots;

		const bool bDirSuccess = IFileManager::Get().MakeDirectory(*WritePath);

		UE_LOG(LogParticles, Warning, TEXT("IFileManager::MakeDirectory %s %u"), *WritePath, bDirSuccess);

		// Have to keep this filename short enough to be valid for Xbox 360
		const FString TemplateFileName = WritePath / TEXT("ParticleTemplates.csv");
		FDiagnosticTableViewer ParticleTemplateViewer(*TemplateFileName);

		UE_LOG(LogParticles, Warning, TEXT("TemplateFileName %s %u"), *TemplateFileName, ParticleTemplateViewer.OutputStreamIsValid());

		// Write a row of headings for the table's columns.
		ParticleTemplateViewer.AddColumn(TEXT("RenderTime ms"));
		ParticleTemplateViewer.AddColumn(TEXT("NumComponents"));
		ParticleTemplateViewer.AddColumn(TEXT("NumPasses"));
		ParticleTemplateViewer.AddColumn(TEXT("NumEmitters"));
		ParticleTemplateViewer.AddColumn(TEXT("NumDraws"));
		ParticleTemplateViewer.AddColumn(TEXT("Template Name"));
		ParticleTemplateViewer.CycleRow();

		float TotalRenderTime = 0;
		int32 TotalNumComponents = 0;
		int32 TotalNumDrawDynamicElements = 0;
		int32 TotalNumEmitters = 0;
		int32 TotalNumDraws = 0;

		// Sort from longest render time to shortest
		GOneFrameTemplateRenderStats.ValueSort( FCompareFParticleTemplateRenderStatsByRenderTime() );

		for (TMap<FString, FParticleTemplateRenderStats>::TIterator It(GOneFrameTemplateRenderStats); It; ++It)
		{	
			const FParticleTemplateRenderStats& Stats = It.Value();
			TotalRenderTime += Stats.MaxRenderTime;
			TotalNumComponents += Stats.NumComponents;
			TotalNumDrawDynamicElements += Stats.NumDrawDynamicElements;
			TotalNumEmitters += Stats.NumEmitters;
			TotalNumDraws += Stats.NumDraws;
			ParticleTemplateViewer.AddColumn(TEXT("%.2f"), Stats.MaxRenderTime * 1000.0f);
			ParticleTemplateViewer.AddColumn(TEXT("%u"), Stats.NumComponents);
			ParticleTemplateViewer.AddColumn(TEXT("%.1f"), Stats.NumDrawDynamicElements / (float)Stats.NumComponents);
			ParticleTemplateViewer.AddColumn(TEXT("%u"), Stats.NumEmitters);
			ParticleTemplateViewer.AddColumn(TEXT("%u"), Stats.NumDraws);
			ParticleTemplateViewer.AddColumn(*It.Key());
			ParticleTemplateViewer.CycleRow();
		}

		ParticleTemplateViewer.AddColumn(TEXT("%.2f"), TotalRenderTime * 1000.0f);
		ParticleTemplateViewer.AddColumn(TEXT("%u"), TotalNumComponents);
		ParticleTemplateViewer.AddColumn(TEXT("%.1f"), TotalNumDrawDynamicElements / (float)TotalNumComponents);
		ParticleTemplateViewer.AddColumn(TEXT("%u"), TotalNumEmitters);
		ParticleTemplateViewer.AddColumn(TEXT("%u"), TotalNumDraws);
		ParticleTemplateViewer.AddColumn(TEXT("Totals"));
		ParticleTemplateViewer.CycleRow();

		UE_LOG(LogParticles, Warning, TEXT("One frame stats saved to %s"), *TemplateFileName);

		// Request a screenshot to be saved next frame
		FScreenshotRequest::RequestScreenshot( PathFromScreenshots / TEXT("Shot.bmp"), false );
		
		UE_LOG(LogParticles, Warning, TEXT("ScreenShotName %s"), *FScreenshotRequest::GetFilename() );

		GOneFrameTemplateRenderStats.Empty();
	}
}
#endif

#define LOG_DETAILED_PARTICLE_RENDER_STATS 0

#if LOG_DETAILED_PARTICLE_RENDER_STATS 
/** Global detailed update stats. */
static FDetailedTickStats GDetailedParticleRenderStats( 20, 10, 1, 4, TEXT("rendering") );
#define TRACK_DETAILED_PARTICLE_RENDER_STATS(Object) FScopedDetailTickStats DetailedTickStats(GDetailedParticleRenderStats,Object);
#else
#define TRACK_DETAILED_PARTICLE_RENDER_STATS(Object)
#endif


///////////////////////////////////////////////////////////////////////////////
FParticleOrderPool GParticleOrderPool;

///////////////////////////////////////////////////////////////////////////////
// Particle vertex factory pool
FParticleVertexFactoryPool GParticleVertexFactoryPool;

FParticleVertexFactoryBase* FParticleVertexFactoryPool::GetParticleVertexFactory(EParticleVertexFactoryType InType)
{
	SCOPE_CYCLE_COUNTER(STAT_ParticlePoolTime);
	check(InType < PVFT_MAX);
	FParticleVertexFactoryBase* VertexFactory = NULL;
	if (VertexFactoriesAvailable[InType].Num() == 0)
	{
		// If there are none in the pool, create a new one, add it to the in use list and return it
		VertexFactory = CreateParticleVertexFactory(InType);
		VertexFactories.Add(VertexFactory);
	}
	else
	{
		// Otherwise, pull one out of the available array
		VertexFactory = VertexFactoriesAvailable[InType][VertexFactoriesAvailable[InType].Num() - 1];
		VertexFactoriesAvailable[InType].RemoveAt(VertexFactoriesAvailable[InType].Num() - 1);
	}
	check(VertexFactory);
	// Set it to true to indicate it is in use
	VertexFactory->SetInUse(true);
	return VertexFactory;
}

bool FParticleVertexFactoryPool::ReturnParticleVertexFactory(FParticleVertexFactoryBase* InVertexFactory)
{
	SCOPE_CYCLE_COUNTER(STAT_ParticlePoolTime);
	// Set it to false to indicate it is not in use
	InVertexFactory->SetInUse(false);
	VertexFactoriesAvailable[InVertexFactory->GetParticleFactoryType()].Add(InVertexFactory);
	return true;
}

void FParticleVertexFactoryPool::ClearPool()
{
	SCOPE_CYCLE_COUNTER(STAT_ParticlePoolTime);
	ClearPoolInternal();
}

void FParticleVertexFactoryPool::ClearPoolInternal()
{
	for (int32 TestIndex=VertexFactories.Num()-1; TestIndex >= 0; --TestIndex)
	{
		FParticleVertexFactoryBase* VertexFactory = VertexFactories[TestIndex];
		if (!VertexFactory->GetInUse())
		{
			VertexFactories.RemoveAtSwap(TestIndex);
		}
	}

	// Release all the resources...
	// We can't safely touched the 'in-use' ones... 
	for (int32 PoolIdx = 0; PoolIdx < PVFT_MAX; PoolIdx++)
	{
		for (int32 RemoveIdx = VertexFactoriesAvailable[PoolIdx].Num() - 1; RemoveIdx >= 0; RemoveIdx--)
		{
			FParticleVertexFactoryBase* VertexFactory = VertexFactoriesAvailable[PoolIdx][RemoveIdx];
			if(VertexFactory != NULL)
			{
				VertexFactory->ReleaseResource();
				delete VertexFactory;
			}
			VertexFactoriesAvailable[PoolIdx].RemoveAt(RemoveIdx);
		}
	}
}

void FParticleVertexFactoryPool::FreePool()
{
	ClearPool();
	{
		SCOPE_CYCLE_COUNTER(STAT_ParticlePoolTime);
		for (int32 TestIndex=VertexFactories.Num()-1; TestIndex >= 0; --TestIndex)
		{
			FParticleVertexFactoryBase* VertexFactory = VertexFactories[TestIndex];
			check(VertexFactory);
			if (VertexFactory->GetInUse())
			{
				// Has already been released by the device cleanup...
				delete VertexFactory;
			}
		}
		VertexFactories.Empty();
	}
}

#if STATS
int32 FParticleVertexFactoryPool::GetTypeSize(EParticleVertexFactoryType InType)
{
	switch (InType)
	{
	case PVFT_Sprite:						return sizeof(FParticleSpriteVertexFactory);
	case PVFT_BeamTrail:					return sizeof(FParticleBeamTrailVertexFactory);
	case PVFT_Mesh:							return sizeof(FMeshParticleVertexFactory);
	default:								return 0;
	}
}

void FParticleVertexFactoryPool::DumpInfo(FOutputDevice& Ar)
{
	Ar.Logf(TEXT("ParticleVertexFactoryPool State"));
	Ar.Logf(TEXT("Type,Count,Mem(Bytes)"));
	int32 TotalMemory = 0;
	for (int32 PoolIdx = 0; PoolIdx < PVFT_MAX; PoolIdx++)
	{
		int32 LocalMemory = GetTypeSize((EParticleVertexFactoryType)PoolIdx) * VertexFactoriesAvailable[PoolIdx].Num();
		Ar.Logf(TEXT("%s,%d,%d"), 
			GetTypeString((EParticleVertexFactoryType)PoolIdx), 
			VertexFactoriesAvailable[PoolIdx].Num(),
			LocalMemory);
		TotalMemory += LocalMemory;
	}
	Ar.Logf(TEXT("TotalMemory Taken in Pool: %d"), TotalMemory);
	TotalMemory = 0;
	Ar.Logf(TEXT("ACTIVE,%d"), VertexFactories.Num());
	if (VertexFactories.Num() > 0)
	{
		int32 ActiveCounts[PVFT_MAX];
		FMemory::Memzero(&ActiveCounts[0], sizeof(int32) * PVFT_MAX);
		for (int32 InUseIndex = 0; InUseIndex < VertexFactories.Num(); ++InUseIndex)
		{
			FParticleVertexFactoryBase* VertexFactory = VertexFactories[InUseIndex];
			if (VertexFactory->GetInUse())
			{
				ActiveCounts[VertexFactory->GetParticleFactoryType()]++;
			}
		}
		for (int32 PoolIdx = 0; PoolIdx < PVFT_MAX; PoolIdx++)
		{
			int32 LocalMemory = GetTypeSize((EParticleVertexFactoryType)PoolIdx) * ActiveCounts[PoolIdx];
			Ar.Logf(TEXT("%s,%d,%d"), 
				GetTypeString((EParticleVertexFactoryType)PoolIdx), 
				ActiveCounts[PoolIdx],
				LocalMemory);
			TotalMemory += LocalMemory;
		}
	}
	Ar.Logf(TEXT("TotalMemory Taken by Actives: %d"), TotalMemory);
}
#endif

/** 
 *	Create a vertex factory for the given type.
 *
 *	@param	InType						The type of vertex factory to create.
 *
 *	@return	FParticleVertexFactoryBase*	The created VF; NULL if invalid InType
 */
FParticleVertexFactoryBase* FParticleVertexFactoryPool::CreateParticleVertexFactory(EParticleVertexFactoryType InType)
{
	FParticleVertexFactoryBase* NewVertexFactory = NULL;
	switch (InType)
	{
	case PVFT_Sprite:
		NewVertexFactory = new FParticleSpriteVertexFactory();
		break;
	case PVFT_BeamTrail:
		NewVertexFactory = new FParticleBeamTrailVertexFactory();
		break;
	case PVFT_Mesh:
		NewVertexFactory = new FMeshParticleVertexFactory();
		break;
	default:
		break;
	}
	check(NewVertexFactory);
	NewVertexFactory->InitResource();
	return NewVertexFactory;
}

void ParticleVertexFactoryPool_FreePool_RenderingThread()
{
	GParticleVertexFactoryPool.FreePool();
}

void ParticleVertexFactoryPool_FreePool()
{
	ENQUEUE_UNIQUE_RENDER_COMMAND(
		ParticleVertexFactoryFreePool,
	{
		ParticleVertexFactoryPool_FreePool_RenderingThread();
	}
	);		
}

void ParticleVertexFactoryPool_ClearPool_RenderingThread()
{
	GParticleVertexFactoryPool.ClearPool();
}

/** Globally accessible function for clearing the pool */
void ParticleVertexFactoryPool_ClearPool()
{
	ENQUEUE_UNIQUE_RENDER_COMMAND(
		ParticleVertexFactoryFreePool,
	{
		ParticleVertexFactoryPool_ClearPool_RenderingThread();
	}
	);		
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Retrieve the appropriate camera Up and Right vectors for LockAxis situations
 *
 * @param LockAxisFlag	The lock axis flag to compute camera vectors for.
 * @param LocalToWorld	The local-to-world transform for the emitter (identify unless the emitter is rendering in local space).
 * @param CameraUp		OUTPUT - the resulting camera Up vector
 * @param CameraRight	OUTPUT - the resulting camera Right vector
 */
void ComputeLockedAxes(EParticleAxisLock LockAxisFlag, const FMatrix& LocalToWorld, FVector& CameraUp, FVector& CameraRight)
{
	switch (LockAxisFlag)
	{
	case EPAL_X:
		CameraUp	= -LocalToWorld.GetUnitAxis( EAxis::Z );
		CameraRight	= -LocalToWorld.GetUnitAxis( EAxis::Y );
		break;
	case EPAL_Y:
		CameraUp	= -LocalToWorld.GetUnitAxis( EAxis::Z );
		CameraRight	=  LocalToWorld.GetUnitAxis( EAxis::X );
		break;
	case EPAL_Z:
		CameraUp	= -LocalToWorld.GetUnitAxis( EAxis::X );
		CameraRight	=  LocalToWorld.GetUnitAxis( EAxis::Y );
		break;
	case EPAL_NEGATIVE_X:
		CameraUp	= -LocalToWorld.GetUnitAxis( EAxis::Z );
		CameraRight	=  LocalToWorld.GetUnitAxis( EAxis::Y );
		break;
	case EPAL_NEGATIVE_Y:
		CameraUp	= -LocalToWorld.GetUnitAxis( EAxis::Z );
		CameraRight	= -LocalToWorld.GetUnitAxis( EAxis::X );
		break;
	case EPAL_NEGATIVE_Z:
		CameraUp	= -LocalToWorld.GetUnitAxis( EAxis::X );
		CameraRight	= -LocalToWorld.GetUnitAxis( EAxis::Y );
		break;
	case EPAL_ROTATE_X:
		CameraRight	= LocalToWorld.GetUnitAxis( EAxis::X );
		CameraUp	= FVector::ZeroVector;
		break;
	case EPAL_ROTATE_Y:
		CameraRight	= LocalToWorld.GetUnitAxis( EAxis::Y );
		CameraUp	= FVector::ZeroVector;
		break;
	case EPAL_ROTATE_Z:
		CameraRight	= LocalToWorld.GetUnitAxis( EAxis::Z );
		CameraUp	= FVector::ZeroVector;
		break;
	}
}

/**
 *	Helper function for retrieving the camera offset payload of a particle.
 *
 *	@param	InCameraPayloadOffset	The offset to the camera offset payload data.
 *	@param	InParticle				The particle being processed.
 *	@param	InPosition				The position of the particle being processed.
 *	@param	InCameraPosition		The position of the camera in local space.
 *
 *	@returns the offset to apply to the particle's position.
 */
FORCEINLINE FVector GetCameraOffsetFromPayload(
	int32 InCameraPayloadOffset,
	const FBaseParticle& InParticle, 
	const FVector& InPosition,
	const FVector& InCameraPosition )
{
 	checkSlow(InCameraPayloadOffset > 0);

	FVector DirToCamera = InCameraPosition - InPosition;
	float CheckSize = DirToCamera.SizeSquared();
	DirToCamera.Normalize();
	FCameraOffsetParticlePayload* CameraPayload = ((FCameraOffsetParticlePayload*)((uint8*)(&InParticle) + InCameraPayloadOffset));
	if (CheckSize > (CameraPayload->Offset * CameraPayload->Offset))
	{
		return DirToCamera * CameraPayload->Offset;
	}
	else
	{
		// If the offset will push the particle behind the camera, then push it 
		// WAY behind the camera. This is a hack... but in the case of 
		// PSA_Velocity, it is required to ensure that the particle doesn't 
		// 'spin' flat and come into view.
		return DirToCamera * CameraPayload->Offset * HALF_WORLD_MAX;
	}
}

/**  
 * Simple function to pass the async buffer fill task off to the owning emitter
*/
void FAsyncParticleFill::DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	// Test integrity of async particle fill: FPlatformProcess::Sleep(.01);
	SCOPE_CYCLE_COUNTER(STAT_ParticleAsyncTime);
	Parent->DoBufferFill();
}

void FDynamicSpriteEmitterDataBase::SortSpriteParticles(int32 SortMode, bool bLocalSpace, 
	int32 ParticleCount, const TArray<uint8>& ParticleData, int32 ParticleStride, const TArray<uint16>& ParticleIndices,
	const FSceneView* View, const FMatrix& LocalToWorld, FParticleOrder* ParticleOrder)
{
	SCOPE_CYCLE_COUNTER(STAT_SortingTime);

	struct FCompareParticleOrderZ
	{
		FORCEINLINE bool operator()( const FParticleOrder& A, const FParticleOrder& B ) const { return B.Z < A.Z; }
	};
	struct FCompareParticleOrderC
	{
		FORCEINLINE bool operator()( const FParticleOrder& A, const FParticleOrder& B ) const { return B.C < A.C; }
	};

	if (SortMode == PSORTMODE_ViewProjDepth)
	{
		for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
		{
			DECLARE_PARTICLE(Particle, ParticleData.GetData() + ParticleStride * ParticleIndices[ParticleIndex]);
			float InZ;
			if (bLocalSpace)
			{
				InZ = View->ViewProjectionMatrix.TransformPosition(LocalToWorld.TransformPosition(Particle.Location)).W;
			}
			else
			{
				InZ = View->ViewProjectionMatrix.TransformPosition(Particle.Location).W;
			}
			ParticleOrder[ParticleIndex].ParticleIndex = ParticleIndex;

			ParticleOrder[ParticleIndex].Z = InZ;
		}
		Sort( ParticleOrder, ParticleCount, FCompareParticleOrderZ() );
	}
	else if (SortMode == PSORTMODE_DistanceToView)
	{
		for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
		{
			DECLARE_PARTICLE(Particle, ParticleData.GetData() + ParticleStride * ParticleIndices[ParticleIndex]);
			float InZ;
			FVector Position;
			if (bLocalSpace)
			{
				Position = LocalToWorld.TransformPosition(Particle.Location);
			}
			else
			{
				Position = Particle.Location;
			}
			InZ = (View->ViewMatrices.ViewOrigin - Position).SizeSquared();
			ParticleOrder[ParticleIndex].ParticleIndex = ParticleIndex;
			ParticleOrder[ParticleIndex].Z = InZ;
		}
		Sort( ParticleOrder, ParticleCount, FCompareParticleOrderZ() );
	}
	else if (SortMode == PSORTMODE_Age_OldestFirst)
	{
		for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
		{
			DECLARE_PARTICLE(Particle, ParticleData.GetData() + ParticleStride * ParticleIndices[ParticleIndex]);
			ParticleOrder[ParticleIndex].ParticleIndex = ParticleIndex;
			ParticleOrder[ParticleIndex].C = Particle.Flags & STATE_CounterMask;
		}
		Sort( ParticleOrder, ParticleCount, FCompareParticleOrderC() );
	}
	else if (SortMode == PSORTMODE_Age_NewestFirst)
	{
		for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
		{
			DECLARE_PARTICLE(Particle, ParticleData.GetData() + ParticleStride * ParticleIndices[ParticleIndex]);
			ParticleOrder[ParticleIndex].ParticleIndex = ParticleIndex;
			ParticleOrder[ParticleIndex].C = (~Particle.Flags) & STATE_CounterMask;
		}
		Sort( ParticleOrder, ParticleCount, FCompareParticleOrderC() );
	}
}

void FDynamicSpriteEmitterDataBase::RenderDebug(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bCrosses)
{
	check(Proxy);

	const FDynamicSpriteEmitterReplayData& SpriteSource =
		static_cast< const FDynamicSpriteEmitterReplayData& >( GetSource() );

	const FMatrix& LocalToWorld = SpriteSource.bUseLocalSpace ? Proxy->GetLocalToWorld() : FMatrix::Identity;

	FMatrix CameraToWorld = View->ViewMatrices.ViewMatrix.Inverse();
	FVector CamX = CameraToWorld.TransformVector(FVector(1,0,0));
	FVector CamY = CameraToWorld.TransformVector(FVector(0,1,0));

	FLinearColor EmitterEditorColor = FLinearColor(1.0f,1.0f,0);

	for (int32 i = 0; i < SpriteSource.ActiveParticleCount; i++)
	{
		DECLARE_PARTICLE(Particle, SpriteSource.ParticleData.GetData() + SpriteSource.ParticleStride * SpriteSource.ParticleIndices[i]);

		FVector DrawLocation = LocalToWorld.TransformPosition(Particle.Location);
		if (bCrosses)
		{
			FVector Size = Particle.Size * SpriteSource.Scale;
			PDI->DrawLine(DrawLocation - (0.5f * Size.X * CamX), DrawLocation + (0.5f * Size.X * CamX), EmitterEditorColor, Proxy->GetDepthPriorityGroup(View));
			PDI->DrawLine(DrawLocation - (0.5f * Size.Y * CamY), DrawLocation + (0.5f * Size.Y * CamY), EmitterEditorColor, Proxy->GetDepthPriorityGroup(View));
		}
		else
		{
			PDI->DrawPoint(DrawLocation, EmitterEditorColor, 2, Proxy->GetDepthPriorityGroup(View));
		}
	}
}

/**
 *	Set up an buffer for async filling
 *
 *	@param	Proxy					The primitive scene proxy for the emitter.
 *	@param	InBufferIndex			Index of this buffer
 *	@param	InView					View for this buffer
 *	@param	InVertexCount			Count of verts for this buffer
 *	@param	InVertexSize			Stride of these verts, only used for verification
 *	@param	InDynamicParameterVertexStride	Stride of the dynamic parameter
 */
void FDynamicSpriteEmitterDataBase::BuildViewFillData(FParticleSystemSceneProxy* Proxy, int32 InBufferIndex,const FSceneView *InView,int32 InVertexCount,int32 InVertexSize,int32 InDynamicParameterVertexStride)
{
	bool bSuccess = true;

	FAsyncBufferFillData Data;

	Data.LocalToWorld = Proxy->GetLocalToWorld();
	Data.WorldToLocal = Proxy->GetWorldToLocal();
	Data.View = InView;
	check(Data.VertexSize == 0 || Data.VertexSize == InVertexSize);

	check(InBufferIndex == InstanceDataAllocations.Num())
	FGlobalDynamicVertexBuffer::FAllocation* DynamicVertexAllocation = new(InstanceDataAllocations) FGlobalDynamicVertexBuffer::FAllocation();
	*DynamicVertexAllocation = FGlobalDynamicVertexBuffer::Get().Allocate( InVertexCount * InVertexSize );
	Data.VertexData = DynamicVertexAllocation->Buffer;
	Data.VertexCount = InVertexCount;
	Data.VertexSize = InVertexSize;

	bSuccess &= DynamicVertexAllocation->IsValid();

	int32 NumIndices, IndexStride;
	GetIndexAllocInfo(NumIndices, IndexStride);
	check((uint32)NumIndices <= 65535);
	check(IndexStride > 0);

	FGlobalDynamicIndexBuffer::FAllocation* DynamicIndexAllocation = new(IndexDataAllocations) FGlobalDynamicIndexBuffer::FAllocation();
	*DynamicIndexAllocation = FGlobalDynamicIndexBuffer::Get().Allocate( NumIndices, IndexStride );
	Data.IndexData = DynamicIndexAllocation->Buffer;
	Data.IndexCount = NumIndices;

	bSuccess &= DynamicIndexAllocation->IsValid();

	Data.DynamicParameterData = NULL;
	if( bUsesDynamicParameter )
	{
		check( InDynamicParameterVertexStride > 0 );
		check(InBufferIndex == DynamicParameterDataAllocations.Num())
		FGlobalDynamicVertexBuffer::FAllocation* DynamicParameterAllocation = new(DynamicParameterDataAllocations) FGlobalDynamicVertexBuffer::FAllocation();
		*DynamicParameterAllocation = FGlobalDynamicVertexBuffer::Get().Allocate( InVertexCount * InDynamicParameterVertexStride );
		Data.DynamicParameterData = DynamicParameterAllocation->Buffer;

		bSuccess &= DynamicParameterAllocation->IsValid();
	}

	//Create the data but only fill it if all the allocations succeeded.
	if (InBufferIndex >= AsyncBufferFillTasks.Num())
	{
		new (AsyncBufferFillTasks) FAsyncBufferFillData();
	}
	check(InBufferIndex < AsyncBufferFillTasks.Num()); // please add the views in order

	if( bSuccess )
	{
		AsyncBufferFillTasks[InBufferIndex] = Data;
	}
}

/**
 *	Set up all buffers for async filling
 *
 *	@param	Proxy							The primitive scene proxy for the emitter.
 *	@param	ViewFamily						View family to process
 *	@param	VisibilityMap					Visibility map for the sub-views
 *	@param	bOnlyOneView					If true, then we don't need per-view buffers
 *	@param	InVertexCount					Count of verts for this buffer
 *	@param	InVertexSize					Stride of these verts, only used for verification
 *  @param  InDynamicParameterVertexSize	Stride of the dynamic parameter
 */
void FDynamicSpriteEmitterDataBase::BuildViewFillDataAndSubmit(FParticleSystemSceneProxy* Proxy, const FSceneViewFamily* ViewFamily,const uint32 VisibilityMap,bool bOnlyOneView,int32 InVertexCount,int32 InVertexSize,int32 InDynamicParameterVertexSize)
{
	EnsureAsyncTaskComplete();
	int32 NumUsedViews = 0;
	InstanceDataAllocations.Empty();
	IndexDataAllocations.Empty();
	DynamicParameterDataAllocations.Empty();
	for (int32 ViewIndex = 0; ViewIndex < ViewFamily->Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1<<ViewIndex))
		{
			const FSceneView* View = ViewFamily->Views[ViewIndex];
			BuildViewFillData(Proxy,NumUsedViews++,View,InVertexCount,InVertexSize, InDynamicParameterVertexSize);
			if (bOnlyOneView)
			{
				break;
			}
		}
	}
	if (AsyncBufferFillTasks.Num() > NumUsedViews)
	{
		AsyncBufferFillTasks.RemoveAt(NumUsedViews,AsyncBufferFillTasks.Num() - NumUsedViews);
	}
	if (NumUsedViews)
	{
#if 0 // async particle fill is broken in UE4
		if (!GIsEditor)
		{
			check(!AsyncTask.GetReference()); // this should not be still outstanding
			AsyncTask = TGraphTask<FAsyncParticleFill>::CreateTask(NULL, ENamedThreads::RenderThread).ConstructAndDispatchWhenReady(this);
		}
		else
#endif
		{
			DoBufferFill();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//	ParticleMeshEmitterInstance
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//	FDynamicSpriteEmitterData
///////////////////////////////////////////////////////////////////////////////

/** Initialize this emitter's dynamic rendering data, called after source data has been filled in */
void FDynamicSpriteEmitterData::Init( bool bInSelected )
{
	bSelected = bInSelected;

	bUsesDynamicParameter = GetSourceData()->DynamicParameterDataOffset > 0;

	UMaterialInterface const* MaterialInterface = const_cast<UMaterialInterface const*>(Source.MaterialInterface);
	MaterialResource[0] = MaterialInterface->GetRenderProxy(false);
	MaterialResource[1] = GIsEditor ? MaterialInterface->GetRenderProxy(true) : MaterialResource[0];

	// We won't need this on the render thread
	Source.MaterialInterface = NULL;
}

FVector2D GetParticleSize(const FBaseParticle& Particle, const FDynamicSpriteEmitterReplayDataBase& Source)
{
	FVector2D Size;
	Size.X = Particle.Size.X * Source.Scale.X;
	Size.Y = Particle.Size.Y * Source.Scale.Y;
	if (Source.ScreenAlignment == PSA_Square || Source.ScreenAlignment == PSA_FacingCameraPosition)
	{
		Size.Y = Size.X;
	}

	return Size;
}

void ApplyOrbitToPosition(
	const FBaseParticle& Particle, 
	const FDynamicSpriteEmitterReplayDataBase& Source, 
	const FMatrix& InLocalToWorld,
	FOrbitChainModuleInstancePayload*& LocalOrbitPayload,
	FVector& OrbitOffset,
	FVector& PrevOrbitOffset,
	FVector& ParticlePosition,
	FVector& ParticleOldPosition
	)
{
	if (Source.OrbitModuleOffset != 0)
	{
		int32 CurrentOffset = Source.OrbitModuleOffset;
		const uint8* ParticleBase = (const uint8*)&Particle;
		PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);
		OrbitOffset = OrbitPayload.Offset;

		if (Source.bUseLocalSpace == false)
		{
			OrbitOffset = InLocalToWorld.TransformVector(OrbitOffset);
		}
		PrevOrbitOffset = OrbitPayload.PreviousOffset;

		LocalOrbitPayload = &OrbitPayload;

		ParticlePosition += OrbitOffset;
		ParticleOldPosition += PrevOrbitOffset;
	}
}

bool FDynamicSpriteEmitterData::GetVertexAndIndexData(void* VertexData, void* DynamicParameterVertexData, void* FillIndexData, FParticleOrder* ParticleOrder, const FVector& InCameraPosition, const FMatrix& InLocalToWorld)
{
	SCOPE_CYCLE_COUNTER(STAT_ParticlePackingTime);
	int32 ParticleCount = Source.ActiveParticleCount;
	// 'clamp' the number of particles actually drawn
	//@todo.SAS. If sorted, we really want to render the front 'N' particles...
	// right now it renders the back ones. (Same for SubUV draws)
	if ((Source.MaxDrawCount >= 0) && (ParticleCount > Source.MaxDrawCount))
	{
		ParticleCount = Source.MaxDrawCount;
	}

	// Put the camera origin in the appropriate coordinate space.
	FVector CameraPosition = InCameraPosition;
	if (Source.bUseLocalSpace)
	{
		CameraPosition = InLocalToWorld.InverseTransformPosition(InCameraPosition);
	}

	// Pack the data
	int32	ParticleIndex;
	int32	ParticlePackingIndex = 0;
	int32	IndexPackingIndex = 0;

	int32 VertexStride = sizeof(FParticleSpriteVertex);
	int32 VertexDynamicParameterStride = sizeof(FParticleVertexDynamicParameter);
	uint8* TempVert = (uint8*)VertexData;
	uint8* TempDynamicParameterVert = (uint8*)DynamicParameterVertexData;
	FParticleSpriteVertex* FillVertex;
	FParticleVertexDynamicParameter* DynFillVertex;

	FVector OrbitOffset(0, 0, 0);
	FVector PrevOrbitOffset(0.0f, 0.0f, 0.0f);
	FVector4 DynamicParameterValue(1.0f,1.0f,1.0f,1.0f);
	FVector ParticlePosition;
	FVector ParticleOldPosition;
	float SubImageIndex = 0.0f;

	uint8* ParticleData = Source.ParticleData.GetData();
	uint16* ParticleIndices = Source.ParticleIndices.GetData();
	const FParticleOrder* OrderedIndices = ParticleOrder;

	for (int32 i = 0; i < ParticleCount; i++)
	{
		ParticleIndex = OrderedIndices ? OrderedIndices[i].ParticleIndex : i;
		DECLARE_PARTICLE(Particle, ParticleData + Source.ParticleStride * ParticleIndices[ParticleIndex]);
		if (i + 1 < ParticleCount)
		{
			int32 NextIndex = OrderedIndices ? OrderedIndices[i+1].ParticleIndex : (i + 1);
			DECLARE_PARTICLE(NextParticle, ParticleData + Source.ParticleStride * ParticleIndices[NextIndex]);
			FPlatformMisc::Prefetch(&NextParticle);
		}

		const FVector2D Size = GetParticleSize(Particle, Source);

		ParticlePosition = Particle.Location;
		ParticleOldPosition = Particle.OldLocation;

		FOrbitChainModuleInstancePayload* LocalOrbitPayload = NULL;

		ApplyOrbitToPosition(Particle, Source, InLocalToWorld, LocalOrbitPayload, OrbitOffset, PrevOrbitOffset, ParticlePosition, ParticleOldPosition);

		if (Source.CameraPayloadOffset != 0)
		{
			FVector CameraOffset = GetCameraOffsetFromPayload(Source.CameraPayloadOffset, Particle, ParticlePosition, CameraPosition);
			ParticlePosition += CameraOffset;
			ParticleOldPosition += CameraOffset;
		}

		if (Source.SubUVDataOffset > 0)
		{
			FFullSubUVPayload* SubUVPayload = (FFullSubUVPayload*)(((uint8*)&Particle) + Source.SubUVDataOffset);
			SubImageIndex = SubUVPayload->ImageIndex;
		}

		if (Source.DynamicParameterDataOffset > 0)
		{
			GetDynamicValueFromPayload(Source.DynamicParameterDataOffset, Particle, DynamicParameterValue);
		}

		FillVertex = (FParticleSpriteVertex*)TempVert;
		FillVertex->Position	= ParticlePosition;
		FillVertex->RelativeTime = Particle.RelativeTime;
		FillVertex->OldPosition	= ParticleOldPosition;
		// Create a floating point particle ID from the counter, map into approximately 0-1
		FillVertex->ParticleId = (Particle.Flags & STATE_CounterMask) / 10000.0f;
		FillVertex->Size		= Size;
		FillVertex->Rotation	= Particle.Rotation;
		FillVertex->SubImageIndex = SubImageIndex;
		FillVertex->Color		= Particle.Color;
		if (bUsesDynamicParameter)
		{
			DynFillVertex = (FParticleVertexDynamicParameter*)TempDynamicParameterVert;
			DynFillVertex->DynamicValue[0] = DynamicParameterValue.X;
			DynFillVertex->DynamicValue[1] = DynamicParameterValue.Y;
			DynFillVertex->DynamicValue[2] = DynamicParameterValue.Z;
			DynFillVertex->DynamicValue[3] = DynamicParameterValue.W;
			TempDynamicParameterVert += VertexDynamicParameterStride;
		}

		TempVert += VertexStride;

		if (LocalOrbitPayload)
		{
			LocalOrbitPayload->PreviousOffset = OrbitOffset;
		}
	}

	return true;
}

bool FDynamicSpriteEmitterData::GetVertexAndIndexDataNonInstanced(void* VertexData, void* DynamicParameterVertexData, void* FillIndexData, FParticleOrder* ParticleOrder, const FVector& InCameraPosition, const FMatrix& InLocalToWorld)
{
	SCOPE_CYCLE_COUNTER(STAT_ParticlePackingTime);

	int32 ParticleCount = Source.ActiveParticleCount;
	// 'clamp' the number of particles actually drawn
	//@todo.SAS. If sorted, we really want to render the front 'N' particles...
	// right now it renders the back ones. (Same for SubUV draws)
	if ((Source.MaxDrawCount >= 0) && (ParticleCount > Source.MaxDrawCount))
	{
		ParticleCount = Source.MaxDrawCount;
	}

	// Put the camera origin in the appropriate coordinate space.
	FVector CameraPosition = InCameraPosition;
	if (Source.bUseLocalSpace)
	{
		CameraPosition = InLocalToWorld.InverseTransformPosition(InCameraPosition);
	}

	// Pack the data
	int32	ParticleIndex;
	int32	ParticlePackingIndex = 0;
	int32	IndexPackingIndex = 0;

	int32 VertexStride = sizeof(FParticleSpriteVertexNonInstanced) * 4;
	int32 VertexDynamicParameterStride = sizeof(FParticleVertexDynamicParameter) * 4;

	uint8* TempVert = (uint8*)VertexData;
	uint8* TempDynamicParameterVert = (uint8*)DynamicParameterVertexData;
	FParticleSpriteVertexNonInstanced* FillVertex;
	FParticleVertexDynamicParameter* DynFillVertex;

	FVector OrbitOffset(0, 0, 0);
	FVector PrevOrbitOffset(0.0f, 0.0f, 0.0f);
	FVector4 DynamicParameterValue(1.0f,1.0f,1.0f,1.0f);
	FVector ParticlePosition;
	FVector ParticleOldPosition;
	float SubImageIndex = 0.0f;

	uint8* ParticleData = Source.ParticleData.GetData();
	uint16* ParticleIndices = Source.ParticleIndices.GetData();
	const FParticleOrder* OrderedIndices = ParticleOrder;

	for (int32 i = 0; i < ParticleCount; i++)
	{
		ParticleIndex = OrderedIndices ? OrderedIndices[i].ParticleIndex : i;
		DECLARE_PARTICLE(Particle, ParticleData + Source.ParticleStride * ParticleIndices[ParticleIndex]);
		if (i + 1 < ParticleCount)
		{
			int32 NextIndex = OrderedIndices ? OrderedIndices[i+1].ParticleIndex : (i + 1);
			DECLARE_PARTICLE(NextParticle, ParticleData + Source.ParticleStride * ParticleIndices[NextIndex]);
			FPlatformMisc::Prefetch(&NextParticle);
		}

		const FVector2D Size = GetParticleSize(Particle, Source);

		ParticlePosition = Particle.Location;
		ParticleOldPosition = Particle.OldLocation;

		FOrbitChainModuleInstancePayload* LocalOrbitPayload = NULL;

		ApplyOrbitToPosition(Particle, Source, InLocalToWorld, LocalOrbitPayload, OrbitOffset, PrevOrbitOffset, ParticlePosition, ParticleOldPosition);

		if (Source.CameraPayloadOffset != 0)
		{
			FVector CameraOffset = GetCameraOffsetFromPayload(Source.CameraPayloadOffset, Particle, ParticlePosition, CameraPosition);
			ParticlePosition += CameraOffset;
			ParticleOldPosition += CameraOffset;
		}

		if (Source.SubUVDataOffset > 0)
		{
			FFullSubUVPayload* SubUVPayload = (FFullSubUVPayload*)(((uint8*)&Particle) + Source.SubUVDataOffset);
			SubImageIndex = SubUVPayload->ImageIndex;
		}

		if (Source.DynamicParameterDataOffset > 0)
		{
			GetDynamicValueFromPayload(Source.DynamicParameterDataOffset, Particle, DynamicParameterValue);
		}

		FillVertex = (FParticleSpriteVertexNonInstanced*)TempVert;
		for(uint32 I = 0; I < 4; ++I)
		{
			if(I == 0)
			{
				FillVertex[I].UV = FVector2D(0.0f, 0.0f);
			}
			if(I == 1)
			{
				FillVertex[I].UV = FVector2D(0.0f, 1.0f);
			}
			if(I == 2)
			{
				FillVertex[I].UV = FVector2D(1.0f, 1.0f);
			}
			if(I == 3)
			{
				FillVertex[I].UV = FVector2D(1.0f, 0.0f);
			}

			FillVertex[I].Position	= ParticlePosition;
			FillVertex[I].RelativeTime = Particle.RelativeTime;
			FillVertex[I].OldPosition	= ParticleOldPosition;
			// Create a floating point particle ID from the counter, map into approximately 0-1
			FillVertex[I].ParticleId = (Particle.Flags & STATE_CounterMask) / 10000.0f;
			FillVertex[I].Size		= Size;
			FillVertex[I].Rotation	= Particle.Rotation;
			FillVertex[I].SubImageIndex = SubImageIndex;
			FillVertex[I].Color		= Particle.Color;
		}

		if (bUsesDynamicParameter)
		{
			DynFillVertex = (FParticleVertexDynamicParameter*)TempDynamicParameterVert;
			for(uint32 I = 0; I < 4; ++I)
			{
				DynFillVertex[I].DynamicValue[0] = DynamicParameterValue.X;
				DynFillVertex[I].DynamicValue[1] = DynamicParameterValue.Y;
				DynFillVertex[I].DynamicValue[2] = DynamicParameterValue.Z;
				DynFillVertex[I].DynamicValue[3] = DynamicParameterValue.W;
			}
			TempDynamicParameterVert += VertexDynamicParameterStride;
		}
		TempVert += VertexStride;

		if (LocalOrbitPayload)
		{
			LocalOrbitPayload->PreviousOffset = OrbitOffset;
		}
	}

	return true;
}



/**
 *	Called during InitViews for view processing on scene proxies before rendering them
 *  Only called for primitives that are visible and have bDynamicRelevance
 *
 *	@param	Proxy			The 'owner' particle system scene proxy
 *	@param	ViewFamily		The ViewFamily to pre-render for
 *	@param	VisibilityMap	A BitArray that indicates whether the primitive was visible in that view (index)
 *	@param	FrameNumber		The frame number of this pre-render
 */
void FDynamicSpriteEmitterData::PreRenderView(FParticleSystemSceneProxy* Proxy, const FSceneViewFamily* ViewFamily, const uint32 VisibilityMap, int32 FrameNumber)
{
	const bool bInstanced = GRHIFeatureLevel >= ERHIFeatureLevel::SM3;

	// Sort and generate particles for this view.
	const FDynamicSpriteEmitterReplayDataBase* SourceData = GetSourceData();
	if ( SourceData && SourceData->EmitterRenderMode == ERM_Normal )
	{
		// Determine how many vertices and indices are needed to render.
		const int32 ParticleCount = SourceData->ActiveParticleCount;
		const int32 VertexSize = GetDynamicVertexStride();
		const int32 DynamicParameterVertexSize = sizeof(FParticleVertexDynamicParameter);
		const int32 VertexPerParticle = bInstanced ? 1 : 4;
		const int32 ViewCount = ViewFamily->Views.Num();
		int32 ViewBit = 1;
		InstanceDataAllocations.Empty();
		DynamicParameterDataAllocations.Empty();
		PerViewUniformBuffers.Empty();

		if (SourceData->bUseLocalSpace == false)
		{
			Proxy->UpdateWorldSpacePrimitiveUniformBuffer();
		}

		// Iterate over views and generate vertices for each view.
		for ( int32 ViewIndex = 0; ViewIndex < ViewCount; ++ViewIndex, ViewBit <<= 1 )
		{
			FGlobalDynamicVertexBuffer::FAllocation* Allocation = new(InstanceDataAllocations) FGlobalDynamicVertexBuffer::FAllocation();
			FGlobalDynamicVertexBuffer::FAllocation* DynamicParameterAllocation = new(DynamicParameterDataAllocations) FGlobalDynamicVertexBuffer::FAllocation();
			
			FParticleSpriteViewUniformBufferRef* SpriteViewUniformBufferPtr = new(PerViewUniformBuffers) FParticleSpriteViewUniformBufferRef();
			if ( VisibilityMap & ViewBit )
			{
				const FSceneView* View = ViewFamily->Views[ViewIndex];

				// Allocate memory for render data.
				*Allocation = FGlobalDynamicVertexBuffer::Get().Allocate( ParticleCount * VertexSize * VertexPerParticle );

				if (bUsesDynamicParameter)
				{
					*DynamicParameterAllocation = FGlobalDynamicVertexBuffer::Get().Allocate( ParticleCount * DynamicParameterVertexSize * VertexPerParticle );
				}
				
				if ( Allocation->IsValid() && (!bUsesDynamicParameter || DynamicParameterAllocation->IsValid()) )
				{
					// Sort the particles if needed.
					FParticleOrder* ParticleOrder = NULL;
					if (SourceData->SortMode != PSORTMODE_None)
					{
						// If material is using unlit translucency and the blend mode is translucent then we need to sort (back to front)
						const FMaterial* Material = MaterialResource[bSelected]->GetMaterial(GRHIFeatureLevel);
						if (Material && 
							(Material->GetBlendMode() == BLEND_Translucent ||
							((SourceData->SortMode == PSORTMODE_Age_OldestFirst) || (SourceData->SortMode == PSORTMODE_Age_NewestFirst)))
							)
						{
							ParticleOrder = GParticleOrderPool.GetParticleOrderData(ParticleCount);
							SortSpriteParticles(SourceData->SortMode, SourceData->bUseLocalSpace, SourceData->ActiveParticleCount, 
								SourceData->ParticleData, SourceData->ParticleStride, SourceData->ParticleIndices,
								View, Proxy->GetLocalToWorld(), ParticleOrder);
						}
					}

					// Fill vertex buffers.
					if(bInstanced)
					{
						GetVertexAndIndexData(Allocation->Buffer, DynamicParameterAllocation->Buffer, NULL, ParticleOrder, View->ViewMatrices.ViewOrigin, Proxy->GetLocalToWorld());
					}
					else
					{
						GetVertexAndIndexDataNonInstanced(Allocation->Buffer, DynamicParameterAllocation->Buffer, NULL, ParticleOrder, View->ViewMatrices.ViewOrigin, Proxy->GetLocalToWorld());
					}
				}

				// Create per-view uniform buffer.
				FParticleSpriteViewUniformParameters UniformParameters;
				FVector2D ObjectNDCPosition;
				FVector2D ObjectMacroUVScales;
				Proxy->GetObjectPositionAndScale(*View,ObjectNDCPosition, ObjectMacroUVScales);
				UniformParameters.MacroUVParameters = FVector4(ObjectNDCPosition.X, ObjectNDCPosition.Y, ObjectMacroUVScales.X, ObjectMacroUVScales.Y);
				*SpriteViewUniformBufferPtr = FParticleSpriteViewUniformBufferRef::CreateUniformBufferImmediate(UniformParameters, UniformBuffer_SingleUse);
			}
		}
	}
}

void GatherParticleLightData(const FDynamicSpriteEmitterReplayDataBase& Source, const FMatrix& InLocalToWorld, TArray<FSimpleLightEntry, SceneRenderingAllocator>& OutParticleLights)
{
	if (Source.LightDataOffset != 0)
	{
		int32 ParticleCount = Source.ActiveParticleCount;
		// 'clamp' the number of particles actually drawn
		//@todo.SAS. If sorted, we really want to render the front 'N' particles...
		// right now it renders the back ones. (Same for SubUV draws)
		if ((Source.MaxDrawCount >= 0) && (ParticleCount > Source.MaxDrawCount))
		{
			ParticleCount = Source.MaxDrawCount;
		}

		OutParticleLights.Reserve(OutParticleLights.Num() + ParticleCount);

		const uint8* ParticleData = Source.ParticleData.GetData();
		const uint16* ParticleIndices = Source.ParticleIndices.GetData();

		for (int32 i = 0; i < ParticleCount; i++)
		{
			DECLARE_PARTICLE_CONST(Particle, ParticleData + Source.ParticleStride * ParticleIndices[i]);

			if (i + 1 < ParticleCount)
			{
				int32 NextIndex = (i + 1);
				DECLARE_PARTICLE_CONST(NextParticle, ParticleData + Source.ParticleStride * ParticleIndices[NextIndex]);
				FPlatformMisc::Prefetch(&NextParticle);
			}

			const FLightParticlePayload* LightPayload = (const FLightParticlePayload*)(((const uint8*)&Particle) + Source.LightDataOffset);

			if (LightPayload->bValid)
			{
				const FVector2D Size = GetParticleSize(Particle, Source);

				FVector ParticlePosition = Particle.Location;
				FOrbitChainModuleInstancePayload* LocalOrbitPayload = NULL;
				FVector Unused(0, 0, 0);
				FVector Unused2(0, 0, 0);
				FVector Unused3(0, 0, 0);

				ApplyOrbitToPosition(Particle, Source, InLocalToWorld, LocalOrbitPayload, Unused, Unused2, ParticlePosition, Unused3);

				FSimpleLightEntry ParticleLight;

				FVector LightPosition = Source.bUseLocalSpace ? FVector(InLocalToWorld.TransformPosition(ParticlePosition)) : ParticlePosition;
				ParticleLight.PositionAndRadius = FVector4(LightPosition, LightPayload->RadiusScale * (Size.X + Size.Y) / 2.0f);
				ParticleLight.Color = FVector(Particle.Color) * Particle.Color.A * LightPayload->ColorScale;
				ParticleLight.Exponent = LightPayload->LightExponent;
				ParticleLight.bAffectTranslucency = LightPayload->bAffectsTranslucency;

				// Only create a light if it will have visible contribution
				if (ParticleLight.PositionAndRadius.W > KINDA_SMALL_NUMBER
					&& ParticleLight.Color.GetMax() > KINDA_SMALL_NUMBER)
				{
					OutParticleLights.Add(ParticleLight);
				}
			}
		}
	}
}

void FDynamicSpriteEmitterData::GatherSimpleLights(const FParticleSystemSceneProxy* Proxy, TArray<FSimpleLightEntry, SceneRenderingAllocator>& OutParticleLights) const
{
	GatherParticleLightData(Source, Proxy->GetLocalToWorld(), OutParticleLights);
}

/**
 *	Render thread only draw call
 *
 *	@param	Proxy		The scene proxy for the particle system that owns this emitter
 *	@param	PDI			The primitive draw interface to render with
 *	@param	View		The scene view being rendered
 */
int32 FDynamicSpriteEmitterData::Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View)
{
	SCOPE_CYCLE_COUNTER(STAT_SpriteRenderingTime);

	const bool bInstanced = GRHIFeatureLevel >= ERHIFeatureLevel::SM3;

	if (bValid == false)
	{
		return 0;
	}

	const FDynamicSpriteEmitterReplayDataBase* SourceData = GetSourceData();
	check(SourceData);
	FParticleSpriteVertexFactory* SpriteVertexFactory = (FParticleSpriteVertexFactory*)VertexFactory;

	int32 NumDraws = 0;
	if (SourceData->EmitterRenderMode == ERM_Normal)
	{
		// Don't render if the material will be ignored
		const bool bIsWireframe = View->Family->EngineShowFlags.Wireframe;
		if (PDI->IsMaterialIgnored(MaterialResource[bSelected]) && !bIsWireframe)
		{
			return 0;
		}

		// Find the vertex allocation for this view.
		FGlobalDynamicVertexBuffer::FAllocation* Allocation = NULL;
		FGlobalDynamicVertexBuffer::FAllocation* DynamicParameterAllocation = NULL;
		
		const int32 ViewIndex = View->Family->Views.Find( View );
		if ( InstanceDataAllocations.IsValidIndex( ViewIndex ) )
		{
			Allocation = &InstanceDataAllocations[ ViewIndex ];
		}

		if ( DynamicParameterDataAllocations.IsValidIndex( ViewIndex ) )
		{
			DynamicParameterAllocation = &DynamicParameterDataAllocations[ ViewIndex ];
		}

		if ( Allocation && DynamicParameterAllocation && Allocation->IsValid() && (!bUsesDynamicParameter || DynamicParameterAllocation->IsValid()) && IsValidRef( PerViewUniformBuffers[ViewIndex] ) )
		{
			// Calculate the number of particles that must be drawn.
			int32 ParticleCount = Source.ActiveParticleCount;
			if ((Source.MaxDrawCount >= 0) && (ParticleCount > Source.MaxDrawCount))
			{
				ParticleCount = Source.MaxDrawCount;
			}

			// Set the sprite uniform buffer for this view.
			check( SpriteVertexFactory );
			SpriteVertexFactory->SetSpriteUniformBuffer( UniformBuffer );
			SpriteVertexFactory->SetSpriteViewUniformBuffer( PerViewUniformBuffers[ViewIndex] );
			SpriteVertexFactory->SetInstanceBuffer( Allocation->VertexBuffer, Allocation->VertexOffset, GetDynamicVertexStride(), bInstanced );
			SpriteVertexFactory->SetDynamicParameterBuffer( DynamicParameterAllocation ? DynamicParameterAllocation->VertexBuffer : NULL, DynamicParameterAllocation->VertexOffset, GetDynamicParameterVertexStride(), bInstanced );

			// Construct the mesh element to render.
			FMeshBatch Mesh;
			FMeshBatchElement& BatchElement = Mesh.Elements[0];
			BatchElement.IndexBuffer = &GParticleIndexBuffer;
			if(bInstanced)
			{
				BatchElement.NumPrimitives = 2;
				BatchElement.NumInstances = ParticleCount;
			}
			else
			{
				BatchElement.NumPrimitives = 2 * ParticleCount;
				BatchElement.NumInstances = 1;
			}
			BatchElement.FirstIndex = 0;
			Mesh.VertexFactory = SpriteVertexFactory;
			// if the particle rendering data is presupplied, use it directly
			Mesh.LCI = NULL;
			if (SourceData->bUseLocalSpace == true)
			{
				BatchElement.PrimitiveUniformBufferResource = &Proxy->GetUniformBuffer();
			}
			else
			{
				BatchElement.PrimitiveUniformBufferResource = &Proxy->GetWorldSpacePrimitiveUniformBuffer();
			}
			BatchElement.MinVertexIndex = 0;
			BatchElement.MaxVertexIndex = (ParticleCount * 4) - 1;
			Mesh.ReverseCulling = Proxy->IsLocalToWorldDeterminantNegative();
			Mesh.CastShadow = Proxy->GetCastShadow();
			Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)Proxy->GetDepthPriorityGroup(View);

			if ( bIsWireframe )
			{
				Mesh.MaterialRenderProxy = UMaterial::GetDefaultMaterial( MD_Surface )->GetRenderProxy( View->Family->EngineShowFlags.Selection ? bSelected : false );
			}
			else
			{
				Mesh.MaterialRenderProxy = MaterialResource[GIsEditor && (View->Family->EngineShowFlags.Selection) ? bSelected : 0];
			}
			Mesh.Type = PT_TriangleList;

			NumDraws += DrawRichMesh(
				PDI, 
				Mesh, 
				FLinearColor(1.0f, 0.0f, 0.0f),	//WireframeColor,
				FLinearColor(1.0f, 1.0f, 0.0f),	//LevelColor,
				FLinearColor(1.0f, 1.0f, 1.0f),	//PropertyColor,		
				Proxy,
				GIsEditor && (View->Family->EngineShowFlags.Selection) ? Proxy->IsSelected() : false,
				bIsWireframe
				);
		}
	}
	else if (SourceData->EmitterRenderMode == ERM_Point)
	{
		RenderDebug(Proxy, PDI, View, false);
	}
	else if (SourceData->EmitterRenderMode == ERM_Cross)
	{
		RenderDebug(Proxy, PDI, View, true);
	}

	return NumDraws;
}



/**
 *	Create the render thread resources for this emitter data
 *
 *	@param	InOwnerProxy	The proxy that owns this dynamic emitter data
 *
 *	@return	bool			true if successful, false if failed
 */
void FDynamicSpriteEmitterData::CreateRenderThreadResources(const FParticleSystemSceneProxy* InOwnerProxy)
{
	// Create the vertex factory...
	if (VertexFactory == NULL)
	{
		VertexFactory = GParticleVertexFactoryPool.GetParticleVertexFactory(PVFT_Sprite);
		check(VertexFactory);
	}

	// Generate the uniform buffer.
	const FDynamicSpriteEmitterReplayDataBase* SourceData = GetSourceData();
	if( SourceData )
	{
		FParticleSpriteUniformParameters UniformParameters;
		UniformParameters.AxisLockRight = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		UniformParameters.AxisLockUp = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		UniformParameters.RotationScale = 1.0f;
		UniformParameters.RotationBias = 0.0f;
		UniformParameters.TangentSelector = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		UniformParameters.InvDeltaSeconds = SourceData->InvDeltaSeconds;

		// Parameters for computing sprite tangents.
		const FMatrix& LocalToWorld = InOwnerProxy->GetLocalToWorld();
		const EParticleAxisLock LockAxisFlag = (EParticleAxisLock)SourceData->LockAxisFlag;
		const bool bRotationLock = (LockAxisFlag >= EPAL_ROTATE_X) && (LockAxisFlag <= EPAL_ROTATE_Z);
		if ( SourceData->ScreenAlignment == PSA_Velocity )
		{
			// No rotation for PSA_Velocity.
			UniformParameters.RotationScale = 0.0f;
			UniformParameters.TangentSelector.Y = 1.0f;
		}
		else if (LockAxisFlag == EPAL_NONE)
		{
			if ( SourceData->ScreenAlignment == PSA_FacingCameraPosition)
			{
				UniformParameters.TangentSelector.W = 1.0f;
			}
			else
			{						
				UniformParameters.TangentSelector.X = 1.0f;
			}
		}
		else
		{
			FVector AxisLockUp, AxisLockRight;
			const FMatrix& AxisLocalToWorld = SourceData->bUseLocalSpace ? LocalToWorld : FMatrix::Identity;
			ComputeLockedAxes( LockAxisFlag, AxisLocalToWorld, AxisLockUp, AxisLockRight );

			UniformParameters.AxisLockRight = AxisLockRight;
			UniformParameters.AxisLockRight.W = 1.0f;
			UniformParameters.AxisLockUp = AxisLockUp;
			UniformParameters.AxisLockUp.W = 1.0f;

			if ( bRotationLock )
			{
				UniformParameters.TangentSelector.Z = 1.0f;
			}
			else
			{
				UniformParameters.TangentSelector.X = 1.0f;
			}

			// For locked rotation about Z the particle should be rotated by 90 degrees.
			UniformParameters.RotationBias = (LockAxisFlag == EPAL_ROTATE_Z) ? (0.5f * PI) : 0.0f;
		}

		// SubUV information.
		UniformParameters.SubImageSize = FVector4(
			SourceData->SubImages_Horizontal,
			SourceData->SubImages_Vertical,
			1.0f / SourceData->SubImages_Horizontal,
			1.0f / SourceData->SubImages_Vertical );

		// Transform local space coordinates into worldspace
		const FVector WorldSpaceSphereCenter = LocalToWorld.TransformPosition(SourceData->NormalsSphereCenter);
		const FVector WorldSpaceCylinderDirection = LocalToWorld.TransformVector(SourceData->NormalsCylinderDirection);

		const EEmitterNormalsMode NormalsMode = (EEmitterNormalsMode)SourceData->EmitterNormalsMode;
		UniformParameters.NormalsType = NormalsMode;
		UniformParameters.NormalsSphereCenter = WorldSpaceSphereCenter;
		UniformParameters.NormalsCylinderUnitDirection = WorldSpaceCylinderDirection;

		UniformParameters.PivotOffset = SourceData->PivotOffset;

		UniformBuffer = FParticleSpriteUniformBufferRef::CreateUniformBufferImmediate( UniformParameters, UniformBuffer_MultiUse );
	}
}

/**
 *	Release the render thread resources for this emitter data
 *
 *	@param	InOwnerProxy	The proxy that owns this dynamic emitter data
 *
 *	@return	bool			true if successful, false if failed
 */
void FDynamicSpriteEmitterData::ReleaseRenderThreadResources(const FParticleSystemSceneProxy* InOwnerProxy)
{
	FDynamicSpriteEmitterDataBase::ReleaseRenderThreadResources( InOwnerProxy );
	UniformBuffer.SafeRelease();
	PerViewUniformBuffers.Empty();
}

///////////////////////////////////////////////////////////////////////////////
//	FDynamicMeshEmitterData
///////////////////////////////////////////////////////////////////////////////

FDynamicMeshEmitterData::FDynamicMeshEmitterData(const UParticleModuleRequired* RequiredModule)
	: FDynamicSpriteEmitterDataBase(RequiredModule)
	, LastFramePreRendered(-1)
	, StaticMesh( NULL )
	, MeshTypeDataOffset(0xFFFFFFFF)
	, bApplyPreRotation(false)
	, RollPitchYaw(0.0f, 0.0f, 0.0f)
	, bUseMeshLockedAxis(false)
	, bUseCameraFacing(false)
	, bApplyParticleRotationAsSpin(false)
	, CameraFacingOption(0)
{
	// only update motion blur transforms if we are not paused
	// bPlayersOnlyPending allows us to keep the particle transforms 
	// from the last ticked frame
}

FDynamicMeshEmitterData::~FDynamicMeshEmitterData()
{
}

/** Initialize this emitter's dynamic rendering data, called after source data has been filled in */
void FDynamicMeshEmitterData::Init( bool bInSelected,
									const FParticleMeshEmitterInstance* InEmitterInstance,
									UStaticMesh* InStaticMesh )
{
	bSelected = bInSelected;

	// @todo: For replays, currently we're assuming the original emitter instance is bound to the same mesh as
	//        when the replay was generated (safe), and various mesh/material indices are intact.  If
	//        we ever support swapping meshes/material on the fly, we'll need cache the mesh
	//        reference and mesh component/material indices in the actual replay data.

	StaticMesh = InStaticMesh;
	check(StaticMesh);

	check(Source.ActiveParticleCount < 16 * 1024);	// TTP #33375
	check(Source.ParticleStride < 2 * 1024);	// TTP #3375

	InEmitterInstance->GetMeshMaterials(
		MeshMaterials,
		InEmitterInstance->SpriteTemplate->LODLevels[InEmitterInstance->CurrentLODLevelIndex]
		);

	check(IsInGameThread());
	for (int32 i = 0; i < MeshMaterials.Num(); ++i)
	{
		UMaterialInterface* RenderMaterial = MeshMaterials[i];
		if (RenderMaterial == NULL  || (RenderMaterial->CheckMaterialUsage_Concurrent(MATUSAGE_MeshParticles) == false))
		{
			MeshMaterials[i] = UMaterial::GetDefaultMaterial(MD_Surface);
		}
	}

	bUsesDynamicParameter = GetSourceData()->DynamicParameterDataOffset > 0;

	// Find the offset to the mesh type data 
	if (InEmitterInstance->MeshTypeData != NULL)
	{
		UParticleModuleTypeDataMesh* MeshTD = InEmitterInstance->MeshTypeData;
		// offset to the mesh emitter type data
		MeshTypeDataOffset = InEmitterInstance->TypeDataOffset;

		// Setup pre-rotation values...
		if ((MeshTD->Pitch != 0.0f) || (MeshTD->Roll != 0.0f) || (MeshTD->Yaw != 0.0f))
		{
			bApplyPreRotation = true;
			RollPitchYaw = FVector(MeshTD->Roll, MeshTD->Pitch, MeshTD->Yaw);
		}
		else
		{
			bApplyPreRotation = false;
		}

		// Setup the camera facing options
		if (MeshTD->bCameraFacing == true)
		{
			bUseCameraFacing = true;
			CameraFacingOption = MeshTD->CameraFacingOption;
			bApplyParticleRotationAsSpin = MeshTD->bApplyParticleRotationAsSpin;
			bFaceCameraDirectionRatherThanPosition = MeshTD->bFaceCameraDirectionRatherThanPosition;
		}

		// Camera facing trumps locked axis... but can still use it.
		// Setup the locked axis option
		uint8 CheckAxisLockOption = MeshTD->AxisLockOption;
		if ((CheckAxisLockOption >= EPAL_X) && (CheckAxisLockOption <= EPAL_NEGATIVE_Z))
		{
			bUseMeshLockedAxis = true;
			Source.LockedAxis = FVector(
				(CheckAxisLockOption == EPAL_X) ? 1.0f : ((CheckAxisLockOption == EPAL_NEGATIVE_X) ? -1.0f :  0.0),
				(CheckAxisLockOption == EPAL_Y) ? 1.0f : ((CheckAxisLockOption == EPAL_NEGATIVE_Y) ? -1.0f :  0.0),
				(CheckAxisLockOption == EPAL_Z) ? 1.0f : ((CheckAxisLockOption == EPAL_NEGATIVE_Z) ? -1.0f :  0.0)
				);
		}
		else if ((CameraFacingOption >= LockedAxis_ZAxisFacing) && (CameraFacingOption <= LockedAxis_NegativeYAxisFacing))
		{
			// Catch the case where we NEED locked axis...
			bUseMeshLockedAxis = true;
			Source.LockedAxis = FVector(1.0f, 0.0f, 0.0f);
		}
	}

	// We won't need this on the render thread
	Source.MaterialInterface = NULL;
}

/**
 *	Create the render thread resources for this emitter data
 *
 *	@param	InOwnerProxy	The proxy that owns this dynamic emitter data
 *
 *	@return	bool			true if successful, false if failed
 */
void FDynamicMeshEmitterData::CreateRenderThreadResources(const FParticleSystemSceneProxy* InOwnerProxy)
{
	// Create the vertex factory
	if (VertexFactory == NULL)
	{
		VertexFactory = GParticleVertexFactoryPool.GetParticleVertexFactory(PVFT_Mesh);
		check(VertexFactory);
		SetupVertexFactory((FMeshParticleVertexFactory*)VertexFactory, StaticMesh->RenderData->LODResources[0]);
	}

	// Generate the uniform buffer.
	const FDynamicSpriteEmitterReplayDataBase* SourceData = GetSourceData();
	if( SourceData )
	{
		FMeshParticleUniformParameters UniformParameters;
		UniformParameters.SubImageSize = FVector4(
		1.0f / SourceData->SubImages_Horizontal,
		1.0f / SourceData->SubImages_Vertical,
		0,0);

		// A weight is used to determine whether the mesh texture coordinates or SubUVs are passed from the vertex shader to the pixel shader.
		const uint32 TexCoordWeight = (SourceData->SubUVDataOffset > 0) ? 1 : 0;
		UniformParameters.TexCoordWeightA = TexCoordWeight;
		UniformParameters.TexCoordWeightB = 1 - TexCoordWeight;
		UniformBuffer = FMeshParticleUniformBufferRef::CreateUniformBufferImmediate( UniformParameters, UniformBuffer_SingleUse );
	}
}

/**
 *	Release the render thread resources for this emitter data
 *
 *	@param	InOwnerProxy	The proxy that owns this dynamic emitter data
 *
 *	@return	bool			true if successful, false if failed
 */
void FDynamicMeshEmitterData::ReleaseRenderThreadResources(const FParticleSystemSceneProxy* InOwnerProxy)
{
	return FDynamicSpriteEmitterDataBase::ReleaseRenderThreadResources( InOwnerProxy );
	UniformBuffer.SafeRelease();
}

int32 FDynamicMeshEmitterData::Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View)
{
	SCOPE_CYCLE_COUNTER(STAT_MeshRenderingTime);

	if (bValid == false)
	{
		return 0;
	}

	const bool bInstanced = GRHIFeatureLevel >= ERHIFeatureLevel::SM3;

	int32 NumDraws = 0;
	if (Source.EmitterRenderMode == ERM_Normal)
	{
		const FStaticMeshLODResources& LODModel = StaticMesh->RenderData->LODResources[0];
		TArray<int32> ValidSectionIndices;

		bool bNoValidElements = true;

		for (int32 LODIndex = 0; LODIndex < 1; LODIndex++)
		{
			for (int32 ElementIndex = 0; ElementIndex < LODModel.Sections.Num(); ElementIndex++)
			{
				FMaterialRenderProxy* MaterialProxy = MeshMaterials[ElementIndex]->GetRenderProxy(bSelected);
				check(MaterialProxy);

				// If the material is ignored by the PDI (or not there at all...), 
				// do not add it to the list of valid elements.
				if ((MaterialProxy && !PDI->IsMaterialIgnored(MaterialProxy)) || View->Family->EngineShowFlags.Wireframe)
				{
					ValidSectionIndices.Add(ElementIndex);
					bNoValidElements = false;
				}
				else
				{
					ValidSectionIndices.Add(-1);
				}
			}
		}

		if (bNoValidElements == true)
		{
			// No valid materials... quick out
			return 0;
		}

		const bool bIsWireframe = AllowDebugViewmodes() && View->Family->EngineShowFlags.Wireframe;

		// Find the vertex allocation for this view.
		FGlobalDynamicVertexBuffer::FAllocation* Allocation = NULL;
		const int32 ViewIndex = View->Family->Views.Find( View );
		
		// Only unsorted instance data will exist for unsorted emitters and must be used for all views being rendered. 
		const int32 InstanceBufferIndex = ((Source.SortMode == PSORTMODE_None) ? 0 : ViewIndex);
		if(bInstanced)
		{
			check(InstanceDataAllocations.IsValidIndex( InstanceBufferIndex ) != false);
			check(InstanceDataAllocations[InstanceBufferIndex].IsValid());
			check(DynamicParameterDataAllocations.IsValidIndex( InstanceBufferIndex ) != false);
			check(!bUsesDynamicParameter || DynamicParameterDataAllocations[InstanceBufferIndex].IsValid());
		}

		// Calculate the number of particles that must be drawn.
		int32 ParticleCount = Source.ActiveParticleCount;
		if ((Source.MaxDrawCount >= 0) && (ParticleCount > Source.MaxDrawCount))
		{
			ParticleCount = Source.MaxDrawCount;
		}

		FMeshBatch Mesh;
		FMeshBatchElement& BatchElement = Mesh.Elements[0];
		Mesh.VertexFactory = VertexFactory;
		Mesh.DynamicVertexData = NULL;
		Mesh.LCI = NULL;
		Mesh.UseDynamicData = false;
		Mesh.ReverseCulling = Proxy->IsLocalToWorldDeterminantNegative();
		Mesh.CastShadow = Proxy->GetCastShadow();
		Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)Proxy->GetDepthPriorityGroup(View);

		//@todo. Handle LODs.
		for (int32 LODIndex = 0; LODIndex < 1; LODIndex++)
		{
			for (int32 ValidIndex = 0; ValidIndex < ValidSectionIndices.Num(); ValidIndex++)
			{
				int32 SectionIndex = ValidSectionIndices[ValidIndex];
				if (SectionIndex == -1)
				{
					continue;
				}

				FMaterialRenderProxy* MaterialProxy = MeshMaterials[SectionIndex]->GetRenderProxy(bSelected);
				const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
				if ((Section.NumTriangles == 0) || (MaterialProxy == NULL))
				{
					//@todo. This should never occur, but it does occasionally.
					continue;
				}

				// Draw the static mesh Sections.
				FMeshParticleVertexFactory* MeshVertexFactory = (FMeshParticleVertexFactory*)VertexFactory;
				MeshVertexFactory->SetUniformBuffer( UniformBuffer );

				if(bInstanced)
				{
					// Set the particle instance data. This buffer is generated in PreRenderView.
					MeshVertexFactory->SetInstanceBuffer(InstanceDataAllocations[InstanceBufferIndex].VertexBuffer, InstanceDataAllocations[InstanceBufferIndex].VertexOffset , GetDynamicVertexStride());
					MeshVertexFactory->SetDynamicParameterBuffer(DynamicParameterDataAllocations[InstanceBufferIndex].VertexBuffer, DynamicParameterDataAllocations[InstanceBufferIndex].VertexOffset , GetDynamicParameterVertexStride());
				}

				BatchElement.PrimitiveUniformBufferResource = &Proxy->GetWorldSpacePrimitiveUniformBuffer();
				BatchElement.FirstIndex = Section.FirstIndex;
				BatchElement.MinVertexIndex = Section.MinVertexIndex;
				BatchElement.MaxVertexIndex = Section.MaxVertexIndex;

				BatchElement.NumInstances = bInstanced ? ParticleCount : 1;

				if (bIsWireframe)
				{
					if( LODModel.WireframeIndexBuffer.IsInitialized()
						&& !(RHISupportsTessellation(GRHIShaderPlatform) && Mesh.VertexFactory->GetType()->SupportsTessellationShaders())
						)
					{
						Mesh.Type = PT_LineList;
						Mesh.MaterialRenderProxy = Proxy->GetDeselectedWireframeMatInst();
						BatchElement.FirstIndex = 0;
						BatchElement.IndexBuffer = &LODModel.WireframeIndexBuffer;
						BatchElement.NumPrimitives = LODModel.WireframeIndexBuffer.GetNumIndices() / 2;
						
					}
					else
					{
						Mesh.Type = PT_TriangleList;
						Mesh.MaterialRenderProxy = MeshMaterials[SectionIndex]->GetRenderProxy(bSelected);
						Mesh.bWireframe = true;	
						BatchElement.FirstIndex = 0;
						BatchElement.IndexBuffer = &LODModel.IndexBuffer;
						BatchElement.NumPrimitives = LODModel.IndexBuffer.GetNumIndices() / 3;
					}
				}
				else
				{
					Mesh.Type = PT_TriangleList;
					Mesh.MaterialRenderProxy = MeshMaterials[SectionIndex]->GetRenderProxy(bSelected);
					BatchElement.IndexBuffer = &LODModel.IndexBuffer;
					BatchElement.FirstIndex = Section.FirstIndex;
					BatchElement.NumPrimitives = Section.NumTriangles;
				}

				if(bInstanced)
				{
					NumDraws += DrawRichMesh(
						PDI, 
						Mesh, 
						FLinearColor(1.0f, 0.0f, 0.0f),	//WireframeColor,
						FLinearColor(1.0f, 1.0f, 0.0f),	//LevelColor,
						FLinearColor(1.0f, 1.0f, 1.0f),	//PropertyColor,		
						Proxy,
						GIsEditor && (View->Family->EngineShowFlags.Selection) ? bSelected : false,
						bIsWireframe
						);
				}
				else
				{
					for(int32 Particle = 0; Particle < ParticleCount; ++Particle)
					{
						MeshVertexFactory->SetInstanceDataCPU(InstanceDataAllocationsCPU.GetTypedData()+Particle);
						MeshVertexFactory->SetDynamicInstanceDataCPU(DynamicParameterDataAllocationsCPU.GetTypedData()+Particle);
						NumDraws += DrawRichMesh(
							PDI, 
							Mesh, 
							FLinearColor(1.0f, 0.0f, 0.0f),	//WireframeColor,
							FLinearColor(1.0f, 1.0f, 0.0f),	//LevelColor,
							FLinearColor(1.0f, 1.0f, 1.0f),	//PropertyColor,		
							Proxy,
							GIsEditor && (View->Family->EngineShowFlags.Selection) ? bSelected : false,
							bIsWireframe
							);
					}
				}
			}
		}
	}
	else if (Source.EmitterRenderMode == ERM_Point)
	{
		RenderDebug(Proxy, PDI, View, false);
	}
	else if (Source.EmitterRenderMode == ERM_Cross)
	{
		RenderDebug(Proxy, PDI, View, true);
	}

	return NumDraws;
}

/**
 *	Called during InitViews for view processing on scene proxies before rendering them
 *  Only called for primitives that are visible and have bDynamicRelevance
 *
 *	@param	Proxy			The 'owner' particle system scene proxy
 *	@param	ViewFamily		The ViewFamily to pre-render for
 *	@param	VisibilityMap	A BitArray that indicates whether the primitive was visible in that view (index)
 *	@param	FrameNumber		The frame number of this pre-render
 */
void FDynamicMeshEmitterData::PreRenderView(FParticleSystemSceneProxy* Proxy, const FSceneViewFamily* ViewFamily, const uint32 VisibilityMap, int32 FrameNumber)
{
	if (bValid == false)
	{
		return;
	}

	const bool bInstanced = GRHIFeatureLevel >= ERHIFeatureLevel::SM3;

	int32 ParticleCount = Source.ActiveParticleCount;
	if ((Source.MaxDrawCount >= 0) && (ParticleCount > Source.MaxDrawCount))
	{
		ParticleCount = Source.MaxDrawCount;
	}

	const int32 InstanceVertexStride  = GetDynamicVertexStride();
	const int32 DynamicParameterVertexStride  = GetDynamicParameterVertexStride();
	const int32 ViewCount = ViewFamily->Views.Num();
	if(bInstanced)
	{
		InstanceDataAllocations.Empty();
		DynamicParameterDataAllocations.Empty();
	}
	else
	{
		InstanceDataAllocationsCPU.Reset();
		DynamicParameterDataAllocationsCPU.Reset();
	}

	// Iterate over views and generate instance data for each view.
	for ( int32 ViewIndex = 0; ViewIndex < ViewCount; ++ViewIndex )
	{	
		if(bInstanced)
		{
			FGlobalDynamicVertexBuffer::FAllocation* Allocation = new(InstanceDataAllocations) FGlobalDynamicVertexBuffer::FAllocation();
			FGlobalDynamicVertexBuffer::FAllocation* DynamicParameterAllocation = new(DynamicParameterDataAllocations) FGlobalDynamicVertexBuffer::FAllocation();
			
			*Allocation = FGlobalDynamicVertexBuffer::Get().Allocate( ParticleCount * InstanceVertexStride );

			if (bUsesDynamicParameter)
			{
				*DynamicParameterAllocation = FGlobalDynamicVertexBuffer::Get().Allocate( ParticleCount * DynamicParameterVertexStride );
			}

			if(Allocation->IsValid() && (!bUsesDynamicParameter || DynamicParameterAllocation->IsValid()))
			{
				// Fill instance buffer.
				GetInstanceData(Allocation->Buffer, DynamicParameterAllocation->Buffer, Proxy, ViewFamily->Views[ViewIndex]);
			}
		}
		else
		{
			InstanceDataAllocationsCPU.Init( ParticleCount );
			DynamicParameterDataAllocationsCPU.Init( ParticleCount );

			// Fill instance buffer.
			GetInstanceData((void*) InstanceDataAllocationsCPU.GetTypedData(), (void*) DynamicParameterDataAllocationsCPU.GetTypedData(), Proxy, ViewFamily->Views[ViewIndex]);
		}

		// Instance data is not calculated per-view when the emitter is unsorted.
		if(Source.SortMode == PSORTMODE_None)
		{
			break;
		}
	}

	// Update the primitive uniform buffer
	if (LastFramePreRendered != FrameNumber)
	{
		Proxy->UpdateWorldSpacePrimitiveUniformBuffer();
		LastFramePreRendered = FrameNumber;
	}
}

void FDynamicMeshEmitterData::GatherSimpleLights(const FParticleSystemSceneProxy* Proxy, TArray<FSimpleLightEntry, SceneRenderingAllocator>& OutParticleLights) const
{
	GatherParticleLightData(Source, Proxy->GetLocalToWorld(), OutParticleLights);
}

void FDynamicMeshEmitterData::GetParticleTransform(FBaseParticle& InParticle, const FVector& CameraPosition, const FVector& CameraFacingOpVector, const FQuat& PointToLockedAxis, 
	FParticleSystemSceneProxy* Proxy, const FSceneView* View, FMatrix& OutTransformMat)
	
{
	const uint8* ParticleBase = (uint8*)&InParticle;
	OutTransformMat = FMatrix::Identity;
	
	FMatrix LocalToWorld;
	FTranslationMatrix kTransMat(FVector::ZeroVector);
	FScaleMatrix kScaleMat(FVector(1.0f));
	FRotator kLockedAxisRotator = FRotator::ZeroRotator;
	FVector Location;
	FVector ScaledSize;
	FVector	DirToCamera;
	FVector	LocalSpaceFacingAxis;
	FVector	LocalSpaceUpAxis;
	FQuat PointTo = PointToLockedAxis;

	// Initialize particle position and scale.
	FVector ParticlePosition(InParticle.Location);
	if (Source.CameraPayloadOffset != 0)
	{
		const FVector CameraOffset = GetCameraOffsetFromPayload(Source.CameraPayloadOffset, InParticle, ParticlePosition, CameraPosition);
		ParticlePosition += CameraOffset;
	}

	kTransMat.M[3][0] = ParticlePosition.X;
	kTransMat.M[3][1] = ParticlePosition.Y;
	kTransMat.M[3][2] = ParticlePosition.Z;
	ScaledSize = InParticle.Size * Source.Scale;
	kScaleMat.M[0][0] = ScaledSize.X;
	kScaleMat.M[1][1] = ScaledSize.Y;
	kScaleMat.M[2][2] = ScaledSize.Z;

	FRotator kRotator(0,0,0);
	LocalToWorld = Proxy->GetLocalToWorld();
	if (bUseCameraFacing == true)
	{
		Location = ParticlePosition;
		FVector	VelocityDirection = InParticle.Velocity;
		if (Source.bUseLocalSpace)
		{
			bool bClearLocal2World = false;
			// Transform the location to world space
			Location = LocalToWorld.TransformPosition(Location);
			if (CameraFacingOption <= XAxisFacing_NegativeYUp)
			{
				bClearLocal2World = true;
			}
			else if (CameraFacingOption >= VelocityAligned_ZAxisFacing)
			{
				bClearLocal2World = true;
				VelocityDirection = LocalToWorld.Inverse().GetTransposed().TransformVector(VelocityDirection);
			}

			if (bClearLocal2World)
			{
				// Set the translation matrix to the location
				kTransMat.SetOrigin(Location);
				// Set Local2World to identify to remove any rotational information
				LocalToWorld.SetIdentity();
			}
		}
		VelocityDirection.Normalize();

		if( bFaceCameraDirectionRatherThanPosition )
		{
			DirToCamera = -View->GetViewDirection();
		}
		else
		{
			DirToCamera	= View->ViewMatrices.ViewOrigin - Location;
		}
		
		DirToCamera.Normalize();
		if (DirToCamera.SizeSquared() <	0.5f)
		{
			// Assert possible if DirToCamera is not normalized
			DirToCamera	= FVector(1,0,0);
		}

		bool bFacingDirectionIsValid = true;
		if (CameraFacingOption != XAxisFacing_NoUp)
		{
			FVector FacingDir;
			FVector DesiredDir;

			if ((CameraFacingOption >= VelocityAligned_ZAxisFacing) &&
				(CameraFacingOption <= VelocityAligned_NegativeYAxisFacing))
			{
				if (VelocityDirection.IsNearlyZero())
				{
					// We have to fudge it
					bFacingDirectionIsValid = false;
				}
				// Velocity align the X-axis, and camera face the selected axis
				PointTo = FQuat::FindBetween(FVector(1.0f, 0.0f, 0.0f), VelocityDirection);
				FacingDir = VelocityDirection;
				DesiredDir = DirToCamera;
			}
			else if (CameraFacingOption <= XAxisFacing_NegativeYUp)
			{
				// Camera face the X-axis, and point the selected axis towards the world up
				PointTo = FQuat::FindBetween(FVector(1,0,0), DirToCamera);
				FacingDir = DirToCamera;
				DesiredDir = FVector(0,0,1);
			}
			else
			{
				// Align the X-axis with the selected LockAxis, and point the selected axis towards the camera
				// PointTo will contain quaternion for locked axis rotation.
				FacingDir = Source.LockedAxis;

				if(Source.bUseLocalSpace)
				{
					//Transform the direction vector into local space.
					DesiredDir = LocalToWorld.GetTransposed().TransformVector(DirToCamera);
				}	
				else
				{
					DesiredDir = DirToCamera;	
				}
			}

			FVector	DirToDesiredInRotationPlane = DesiredDir - ((DesiredDir | FacingDir) * FacingDir);
			DirToDesiredInRotationPlane.Normalize();
			FQuat FacingRotation = FQuat::FindBetween(PointTo.RotateVector(CameraFacingOpVector), DirToDesiredInRotationPlane);
			PointTo = FacingRotation * PointTo;

			// Add in additional rotation about either the directional or camera facing axis
			if (bApplyParticleRotationAsSpin)
			{
				if (bFacingDirectionIsValid)
				{
					FQuat AddedRotation = FQuat(FacingDir, InParticle.Rotation);
					kLockedAxisRotator = FRotator(AddedRotation * PointTo);
				}
			}
			else
			{
				FQuat AddedRotation = FQuat(DirToCamera, InParticle.Rotation);
				kLockedAxisRotator = FRotator(AddedRotation * PointTo);
			}
		}
		else
		{
			PointTo = FQuat::FindBetween(FVector(1,0,0), DirToCamera);
			// Add in additional rotation about facing axis
			FQuat AddedRotation = FQuat(DirToCamera, InParticle.Rotation);
			kLockedAxisRotator = FRotator(AddedRotation * PointTo);
		}
	}
	else if (bUseMeshLockedAxis == true)
	{
		// Add any 'sprite rotation' about the locked axis
		FQuat AddedRotation = FQuat(Source.LockedAxis, InParticle.Rotation);
		kLockedAxisRotator = FRotator(AddedRotation * PointTo);
	}
	else if (Source.ScreenAlignment == PSA_TypeSpecific)
	{
		Location = ParticlePosition;
		if (Source.bUseLocalSpace)
		{
			// Transform the location to world space
			Location = LocalToWorld.TransformPosition(Location);
			kTransMat.SetOrigin(Location);
			LocalToWorld.SetIdentity();
		}
		DirToCamera	= View->ViewMatrices.ViewOrigin - Location;
		DirToCamera.Normalize();
		if (DirToCamera.SizeSquared() <	0.5f)
		{
			// Assert possible if DirToCamera is not normalized
			DirToCamera	= FVector(1,0,0);
		}

		LocalSpaceFacingAxis = FVector(1,0,0); // facing axis is taken to be the local x axis.	
		LocalSpaceUpAxis = FVector(0,0,1); // up axis is taken to be the local z axis

		if (Source.MeshAlignment == PSMA_MeshFaceCameraWithLockedAxis)
		{
			// TODO: Allow an arbitrary	vector to serve	as the locked axis

			// For the locked axis behavior, only rotate to	face the camera	about the
			// locked direction, and maintain the up vector	pointing towards the locked	direction
			// Find	the	rotation that points the localupaxis towards the targetupaxis
			FQuat PointToUp	= FQuat::FindBetween(LocalSpaceUpAxis, Source.LockedAxis);

			// Add in rotation about the TargetUpAxis to point the facing vector towards the camera
			FVector	DirToCameraInRotationPlane = DirToCamera - ((DirToCamera | Source.LockedAxis)*Source.LockedAxis);
			DirToCameraInRotationPlane.Normalize();
			FQuat PointToCamera	= FQuat::FindBetween(PointToUp.RotateVector(LocalSpaceFacingAxis), DirToCameraInRotationPlane);

			// Set kRotator	to the composed	rotation
			FQuat MeshRotation = PointToCamera*PointToUp;
			kRotator = FRotator(MeshRotation);
		}
		else if (Source.MeshAlignment == PSMA_MeshFaceCameraWithSpin)
		{
				// Implement a tangent-rotation	version	of point-to-camera.	 The facing	direction points to	the	camera,
				// with	no roll, and has addtional sprite-particle rotation	about the tangential axis
				// (c.f. the roll rotation is about	the	radial axis)

				// Find	the	rotation that points the facing	axis towards the camera
				FRotator PointToRotation = FRotator(FQuat::FindBetween(LocalSpaceFacingAxis, DirToCamera));

				// When	constructing the rotation, we need to eliminate	roll around	the	dirtocamera	axis,
				// otherwise the particle appears to rotate	around the dircamera axis when it or the camera	moves
				PointToRotation.Roll = 0;

				// Add in the tangential rotation we do	want.
				FVector	vPositivePitch = FVector(0,0,1); //	this is	set	by the rotator's yaw/pitch/roll	reference frame
				FVector	vTangentAxis = vPositivePitch^DirToCamera;
				vTangentAxis.Normalize();
				if (vTangentAxis.SizeSquared() < 0.5f)
				{
					vTangentAxis = FVector(1,0,0); // assert is	possible if	FQuat axis/angle constructor is	passed zero-vector
				}

				FQuat AddedTangentialRotation =	FQuat(vTangentAxis,	InParticle.Rotation);

				// Set kRotator	to the composed	rotation
				FQuat MeshRotation = AddedTangentialRotation*PointToRotation.Quaternion();
				kRotator = FRotator(MeshRotation);
		}
		else 
		{
				// Implement a roll-rotation version of	point-to-camera.  The facing direction points to the camera,
				// with	no roll, and then rotates about	the	direction_to_camera	by the spriteparticle rotation.

				// Find	the	rotation that points the facing	axis towards the camera
				FRotator PointToRotation = FRotator(FQuat::FindBetween(LocalSpaceFacingAxis, DirToCamera));

				// When	constructing the rotation, we need to eliminate	roll around	the	dirtocamera	axis,
				// otherwise the particle appears to rotate	around the dircamera axis when it or the camera	moves
				PointToRotation.Roll = 0;

				// Add in the roll we do want.
				FQuat AddedRollRotation	= FQuat(DirToCamera, InParticle.Rotation);

				// Set kRotator	to the composed	rotation
				FQuat MeshRotation = AddedRollRotation*PointToRotation.Quaternion();
				kRotator = FRotator(MeshRotation);
		}
	}
	else if (Source.bMeshRotationActive)
	{
		FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((uint8*)&InParticle + Source.MeshRotationOffset);
		kRotator = FRotator::MakeFromEuler(PayloadData->Rotation);
	}
	else
	{
		float fRot = InParticle.Rotation * 180.0f / PI;
		FVector kRotVec = FVector(fRot, fRot, fRot);
		kRotator = FRotator::MakeFromEuler(kRotVec);
	}

	FRotationMatrix kRotMat(kRotator);
	if (bApplyPreRotation == true)
	{
		if ((bUseCameraFacing == true) || (bUseMeshLockedAxis == true))
		{
			OutTransformMat = FRotationMatrix(FRotator::MakeFromEuler(RollPitchYaw)) * kScaleMat * FRotationMatrix(kLockedAxisRotator) * kRotMat * kTransMat;
		}
		else
		{
			OutTransformMat = FRotationMatrix(FRotator::MakeFromEuler(RollPitchYaw)) * kScaleMat * kRotMat * kTransMat;
		}
	}
	else if ((bUseCameraFacing == true) || (bUseMeshLockedAxis == true))
	{
		OutTransformMat = kScaleMat * FRotationMatrix(kLockedAxisRotator) * kRotMat * kTransMat;
	}
	else
	{
		OutTransformMat = kScaleMat * kRotMat * kTransMat;
	}

	FVector OrbitOffset(0.0f, 0.0f, 0.0f);
	if (Source.OrbitModuleOffset != 0)
	{
		int32 CurrentOffset = Source.OrbitModuleOffset;
		PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);
		OrbitOffset = OrbitPayload.Offset;
		if (Source.bUseLocalSpace == false)
		{
			OrbitOffset = LocalToWorld.TransformVector(OrbitOffset);
		}

		FTranslationMatrix OrbitMatrix(OrbitOffset);
		OutTransformMat *= OrbitMatrix;
	}

	if (Source.bUseLocalSpace)
	{
		OutTransformMat *= LocalToWorld;
	}
}


void FDynamicMeshEmitterData::GetInstanceData(void* InstanceData, void* DynamicParameterData, FParticleSystemSceneProxy* Proxy, const FSceneView* View)
{
	SCOPE_CYCLE_COUNTER(STAT_ParticlePackingTime);

	FQuat PointToLockedAxis;
	if (bUseMeshLockedAxis == true)
	{
		// facing axis is taken to be the local x axis.	
		PointToLockedAxis = FQuat::FindBetween(FVector(1,0,0), Source.LockedAxis);
	}

	FVector CameraFacingOpVector = FVector::ZeroVector;
	if (CameraFacingOption != XAxisFacing_NoUp)
	{
		switch (CameraFacingOption)
		{
		case XAxisFacing_ZUp:
			CameraFacingOpVector = FVector( 0.0f, 0.0f, 1.0f);
			break;
		case XAxisFacing_NegativeZUp:
			CameraFacingOpVector = FVector( 0.0f, 0.0f,-1.0f);
			break;
		case XAxisFacing_YUp:
			CameraFacingOpVector = FVector( 0.0f, 1.0f, 0.0f);
			break;
		case XAxisFacing_NegativeYUp:
			CameraFacingOpVector = FVector( 0.0f,-1.0f, 0.0f);
			break;
		case LockedAxis_YAxisFacing:
		case VelocityAligned_YAxisFacing:
			CameraFacingOpVector = FVector(0.0f, 1.0f, 0.0f);
			break;
		case LockedAxis_NegativeYAxisFacing:
		case VelocityAligned_NegativeYAxisFacing:
			CameraFacingOpVector = FVector(0.0f,-1.0f, 0.0f);
			break;
		case LockedAxis_ZAxisFacing:
		case VelocityAligned_ZAxisFacing:
			CameraFacingOpVector = FVector(0.0f, 0.0f, 1.0f);
			break;
		case LockedAxis_NegativeZAxisFacing:
		case VelocityAligned_NegativeZAxisFacing:
			CameraFacingOpVector = FVector(0.0f, 0.0f,-1.0f);
			break;
		}
	}

	// Put the camera origin in the appropriate coordinate space.
	FVector CameraPosition = View->ViewMatrices.ViewOrigin;
	if (Source.bUseLocalSpace)
	{
		CameraPosition = Proxy->GetLocalToWorld().InverseTransformPosition(CameraPosition);
	}

	int32 SubImagesX = Source.SubImages_Horizontal;
	int32 SubImagesY = Source.SubImages_Vertical;
	float SubImageSizeX = 1.0f / SubImagesX;
	float SubImageSizeY = 1.0f / SubImagesY;

	int32 ParticleCount = Source.ActiveParticleCount;
	if ((Source.MaxDrawCount >= 0) && (ParticleCount > Source.MaxDrawCount))
	{
		ParticleCount = Source.MaxDrawCount;
	}

	int32 InstanceVertexStride = sizeof(FMeshParticleInstanceVertex);
	int32 DynamicParameterVertexStride = bUsesDynamicParameter ? sizeof(FMeshParticleInstanceVertexDynamicParameter) : 0;
	uint8* TempVert = (uint8*)InstanceData;
	uint8* TempDynamicParameterVert = (uint8*)DynamicParameterData;

	for (int32 i = ParticleCount - 1; i >= 0; i--)
	{
		const int32	CurrentIndex	= Source.ParticleIndices[i];
		const uint8* ParticleBase	= Source.ParticleData.GetData() + CurrentIndex * Source.ParticleStride;
		FBaseParticle& Particle		= *((FBaseParticle*) ParticleBase);
		FMeshParticleInstanceVertex* CurrentInstanceVertex = (FMeshParticleInstanceVertex*)TempVert;
		
		// Populate instance buffer;
		// The particle color.
		CurrentInstanceVertex->Color = Particle.Color;

		// Instance to world transformation. Translation (Instance world position) is packed into W
		FMatrix TransMat(FMatrix::Identity);
		GetParticleTransform(Particle, CameraPosition, CameraFacingOpVector, PointToLockedAxis, Proxy, View, TransMat);
		
		// Transpose on CPU to allow for simpler shader code to perform the transform. 
		const FMatrix Transpose = TransMat.GetTransposed();
		CurrentInstanceVertex->Transform[0] = FVector4(Transpose.M[0][0], Transpose.M[0][1], Transpose.M[0][2], Transpose.M[0][3]);
		CurrentInstanceVertex->Transform[1] = FVector4(Transpose.M[1][0], Transpose.M[1][1], Transpose.M[1][2], Transpose.M[1][3]);
		CurrentInstanceVertex->Transform[2] = FVector4(Transpose.M[2][0], Transpose.M[2][1], Transpose.M[2][2], Transpose.M[2][3]);

		// Particle velocity. Calculate on CPU to avoid computing in vertex shader.
		// Note: It would be preferred if we could check whether the material makes use of the 'Particle Direction' node to avoid this work.
		FVector DeltaPosition = Particle.Location - Particle.OldLocation;
		if(!DeltaPosition.IsZero())
		{
			if (Source.bUseLocalSpace)
			{
				DeltaPosition = Proxy->GetLocalToWorld().TransformVector(DeltaPosition);
			}
			FVector Direction;
			float Speed; 
			DeltaPosition.ToDirectionAndLength(Direction, Speed);

			// Pack direction and speed.
			CurrentInstanceVertex->Velocity = FVector4(Direction,Speed);
		}
		else
		{
			CurrentInstanceVertex->Velocity = FVector4();
		}

		// The particle dynamic value
		if (bUsesDynamicParameter)
		{
			if (Source.DynamicParameterDataOffset > 0)
			{
				FVector4 DynamicParameterValue;
				FMeshParticleInstanceVertexDynamicParameter* CurrentInstanceVertexDynParam = (FMeshParticleInstanceVertexDynamicParameter*)TempDynamicParameterVert;
				GetDynamicValueFromPayload(Source.DynamicParameterDataOffset, Particle, DynamicParameterValue );
				CurrentInstanceVertexDynParam->DynamicValue[0] = DynamicParameterValue.X;
				CurrentInstanceVertexDynParam->DynamicValue[1] = DynamicParameterValue.Y;
				CurrentInstanceVertexDynParam->DynamicValue[2] = DynamicParameterValue.Z;
				CurrentInstanceVertexDynParam->DynamicValue[3] = DynamicParameterValue.W;
				TempDynamicParameterVert += DynamicParameterVertexStride;
			}
		}
		
		// SubUVs 
		if (Source.SubUVInterpMethod != PSUVIM_None && Source.SubUVDataOffset > 0)
		{
			FFullSubUVPayload* SubUVPayload = (FFullSubUVPayload*)(((uint8*)&Particle) + Source.SubUVDataOffset);

			float SubImageIndex = SubUVPayload->ImageIndex;
			float SubImageLerp = FMath::Fractional(SubImageIndex);
			int32 SubImageA = FMath::Floor(SubImageIndex);
			int32 SubImageB = SubImageA + 1;
			int32 SubImageAH = SubImageA % SubImagesX;
			int32 SubImageBH = SubImageB % SubImagesX;
			int32 SubImageAV = SubImageA / SubImagesX;
			int32 SubImageBV = SubImageB / SubImagesX;

			// SubUV offsets and lerp value
			CurrentInstanceVertex->SubUVParams[0] = SubImageAH;
			CurrentInstanceVertex->SubUVParams[1] = SubImageAV;
			CurrentInstanceVertex->SubUVParams[2] = SubImageBH;
			CurrentInstanceVertex->SubUVParams[3] = SubImageBV;
			CurrentInstanceVertex->SubUVLerp = SubImageLerp;
		}

		// The particle's relative time
		CurrentInstanceVertex->RelativeTime = Particle.RelativeTime;

		TempVert += InstanceVertexStride; 
	}
}
void FDynamicMeshEmitterData::SetupVertexFactory( FMeshParticleVertexFactory* VertexFactory, FStaticMeshLODResources& LODResources)
{
		FMeshParticleVertexFactory::DataType Data;

		Data.PositionComponent = FVertexStreamComponent(
			&LODResources.PositionVertexBuffer,
			STRUCT_OFFSET(FPositionVertex,Position),
			LODResources.PositionVertexBuffer.GetStride(),
			VET_Float3
			);

		Data.TangentBasisComponents[0] = FVertexStreamComponent(
			&LODResources.VertexBuffer,
			STRUCT_OFFSET(FStaticMeshFullVertex,TangentX),
			LODResources.VertexBuffer.GetStride(),
			VET_PackedNormal
			);

		Data.TangentBasisComponents[1] = FVertexStreamComponent(
			&LODResources.VertexBuffer,
			STRUCT_OFFSET(FStaticMeshFullVertex,TangentZ),
			LODResources.VertexBuffer.GetStride(),
			VET_PackedNormal
			);

		Data.TextureCoordinates.Empty();
		if( !LODResources.VertexBuffer.GetUseFullPrecisionUVs() )
		{
			uint32 NumTexCoords = FMath::Min<uint32>(LODResources.VertexBuffer.GetNumTexCoords(),MAX_TEXCOORDS);
			for(uint32 UVIndex = 0;UVIndex < NumTexCoords;UVIndex++)
			{
				Data.TextureCoordinates.Add(FVertexStreamComponent(
					&LODResources.VertexBuffer,
					STRUCT_OFFSET(TStaticMeshFullVertexFloat16UVs<MAX_TEXCOORDS>,UVs) + sizeof(FVector2DHalf) * UVIndex,
					LODResources.VertexBuffer.GetStride(),
					VET_Half2
					));
			}
		}
		else
		{
			for(uint32 UVIndex = 0;UVIndex < LODResources.VertexBuffer.GetNumTexCoords();UVIndex++)
			{
				Data.TextureCoordinates.Add(FVertexStreamComponent(
					&LODResources.VertexBuffer,
					STRUCT_OFFSET(TStaticMeshFullVertexFloat32UVs<MAX_TEXCOORDS>,UVs) + sizeof(FVector2D) * UVIndex,
					LODResources.VertexBuffer.GetStride(),
					VET_Float2
					));
			}
		}	

		if(LODResources.ColorVertexBuffer.GetNumVertices() > 0)
		{
			Data.VertexColorComponent = FVertexStreamComponent(
				&LODResources.ColorVertexBuffer,
				0,
				LODResources.ColorVertexBuffer.GetStride(),
				VET_Color
				);
		}


		// Initialize instanced data. Vertex buffer and stride are set before render.
		// Particle color
		Data.ParticleColorComponent = FVertexStreamComponent(
			NULL,
			STRUCT_OFFSET(FMeshParticleInstanceVertex, Color),
			0,
			VET_Float4,
			true
			);

		// Particle transform matrix
		for (int32 MatrixRow = 0; MatrixRow < 3; MatrixRow++)
		{
			Data.TransformComponent[MatrixRow] = FVertexStreamComponent(
				NULL,
				STRUCT_OFFSET(FMeshParticleInstanceVertex, Transform) + sizeof(FVector4) * MatrixRow, 
				0,
				VET_Float4,
				true
				);
		}

		Data.VelocityComponent = FVertexStreamComponent(
			NULL,
			STRUCT_OFFSET(FMeshParticleInstanceVertex,Velocity),
			0,
			VET_Float4,
			true
			);

		// SubUVs.
		Data.SubUVs = FVertexStreamComponent(
			NULL,
			STRUCT_OFFSET(FMeshParticleInstanceVertex, SubUVParams), 
			0,
			VET_Short4,
			true
			);

		// Pack SubUV Lerp and the particle's relative time
		Data.SubUVLerpAndRelTime = FVertexStreamComponent(
			NULL,
			STRUCT_OFFSET(FMeshParticleInstanceVertex, SubUVLerp), 
			0,
			VET_Float2,
			true
			);

		Data.bInitialized = true;
		VertexFactory->SetData(Data);
}

///////////////////////////////////////////////////////////////////////////////
//	FDynamicBeam2EmitterData
///////////////////////////////////////////////////////////////////////////////


/** Initialize this emitter's dynamic rendering data, called after source data has been filled in */
void FDynamicBeam2EmitterData::Init( bool bInSelected )
{
	bSelected = bInSelected;

	check(Source.ActiveParticleCount < (MaxBeams));	// TTP #33330 - Max of 2048 beams from a single emitter
	check(Source.ParticleStride < 
		((MaxInterpolationPoints + 2) * (sizeof(FVector) + sizeof(float))) + 
		(MaxNoiseFrequency * (sizeof(FVector) + sizeof(FVector) + sizeof(float) + sizeof(float)))
		);	// TTP #33330 - Max of 10k per beam (includes interpolation points, noise, etc.)

	MaterialResource[0] = Source.MaterialInterface->GetRenderProxy(false);
	MaterialResource[1] = GIsEditor ? Source.MaterialInterface->GetRenderProxy(true) : MaterialResource[0];

	bUsesDynamicParameter = false;

	// We won't need this on the render thread
	Source.MaterialInterface = NULL;
}

/**
 *	Create the render thread resources for this emitter data
 *
 *	@param	InOwnerProxy	The proxy that owns this dynamic emitter data
 *
 *	@return	bool			true if successful, false if failed
 */
void FDynamicBeam2EmitterData::CreateRenderThreadResources(const FParticleSystemSceneProxy* InOwnerProxy)
{
	// Create the vertex factory...
	//@todo. Cache these??
	if (VertexFactory == NULL)
	{
		VertexFactory = (FParticleBeamTrailVertexFactory*)(GParticleVertexFactoryPool.GetParticleVertexFactory(PVFT_BeamTrail));
		check(VertexFactory);
	}
}

/** Perform the actual work of filling the buffer, often called from another thread 
* @param Me Fill data structure
*/
void FDynamicBeam2EmitterData::DoBufferFill(FAsyncBufferFillData& Me)
{
	if( Me.VertexCount <= 0 || Me.IndexCount <= 0 || Me.VertexData == NULL || Me.IndexData == NULL )
	{
		return;
	}

	FillIndexData(Me);
	if (Source.bLowFreqNoise_Enabled)
	{
		FillData_Noise(Me);
	}
	else
	{
		FillVertexData_NoNoise(Me);
	}
}

FParticleBeamTrailUniformBufferRef CreateBeamTrailUniformBuffer(
	FParticleSystemSceneProxy* Proxy,
	FDynamicSpriteEmitterReplayDataBase* SourceData,
	const FSceneView* View )
{
	FParticleBeamTrailUniformParameters UniformParameters;

	// Tangent vectors.
	FVector CameraUp(0.0f);
	FVector CameraRight(0.0f);
	const EParticleAxisLock LockAxisFlag = (EParticleAxisLock)SourceData->LockAxisFlag;
	if (LockAxisFlag == EPAL_NONE)
	{
		CameraUp	= -View->InvViewProjectionMatrix.TransformVector(FVector(1.0f,0.0f,0.0f)).SafeNormal();
		CameraRight	= -View->InvViewProjectionMatrix.TransformVector(FVector(0.0f,1.0f,0.0f)).SafeNormal();
	}
	else
	{
		const FMatrix& LocalToWorld = SourceData->bUseLocalSpace ? Proxy->GetLocalToWorld() : FMatrix::Identity;
		ComputeLockedAxes( LockAxisFlag, LocalToWorld, CameraUp, CameraRight );
	}
	UniformParameters.CameraUp = FVector4( CameraUp, 0.0f );
	UniformParameters.CameraRight = FVector4( CameraRight, 0.0f );

	// Screen alignment.
	UniformParameters.ScreenAlignment = FVector4( (float)SourceData->ScreenAlignment, 0.0f, 0.0f, 0.0f );

	return FParticleBeamTrailUniformBufferRef::CreateUniformBufferImmediate( UniformParameters, UniformBuffer_SingleUse );
}

int32 FDynamicBeam2EmitterData::Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View)
{
	SCOPE_CYCLE_COUNTER(STAT_BeamRenderingTime);
	INC_DWORD_STAT(STAT_BeamParticlesRenderCalls);

	if (bValid == false)
	{
		return 0;
	}

	if ((Source.VertexCount == 0) && (Source.IndexCount == 0))
	{
		return 0;
	}

	const bool bIsWireframe = View->Family->EngineShowFlags.Wireframe;
	bool bMaterialIgnored = PDI->IsMaterialIgnored(MaterialResource[bSelected]);
	if (bMaterialIgnored && !bIsWireframe)
	{
		return 0;
	}

	const FAsyncBufferFillData& Data = EnsureFillCompletion(View);

	int32 NumDraws = 0;
	if (Data.OutTriangleCount > 0)
	{
		if (!bMaterialIgnored || bIsWireframe)
		{
			FGlobalDynamicVertexBuffer::FAllocation* Allocation = NULL;
			FGlobalDynamicIndexBuffer::FAllocation* IndexAllocation = NULL;

			const int32 ViewIndex = View->Family->Views.Find( View );
			if ( InstanceDataAllocations.IsValidIndex( ViewIndex ) )
			{
				Allocation = &InstanceDataAllocations[ ViewIndex ];
			}	

			if ( IndexDataAllocations.IsValidIndex( ViewIndex ) )
			{
				IndexAllocation = &IndexDataAllocations[ ViewIndex ];
			}

			// Create and set the uniform buffer for this emitter.
			FParticleBeamTrailVertexFactory* BeamTrailVertexFactory = (FParticleBeamTrailVertexFactory*)VertexFactory;
			BeamTrailVertexFactory->SetBeamTrailUniformBuffer( CreateBeamTrailUniformBuffer( Proxy, &Source, View ) );			

			if( Allocation && IndexAllocation && Allocation->IsValid() && IndexAllocation->IsValid() )
			{
				BeamTrailVertexFactory->SetVertexBuffer( Allocation->VertexBuffer, Allocation->VertexOffset, GetDynamicVertexStride() );
				BeamTrailVertexFactory->SetDynamicParameterBuffer( NULL, 0, 0 );

				FMeshBatch Mesh;
				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer	= IndexAllocation->IndexBuffer;
				BatchElement.FirstIndex		= IndexAllocation->FirstIndex;
				Mesh.VertexFactory			= BeamTrailVertexFactory;
				Mesh.DynamicVertexData		= NULL;
				Mesh.DynamicVertexStride	= 0;
				BatchElement.DynamicIndexData		= NULL;
				BatchElement.DynamicIndexStride		= 0;
				Mesh.LCI					= NULL;
				if (Source.bUseLocalSpace == true)
				{
					BatchElement.PrimitiveUniformBufferResource = &Proxy->GetUniformBuffer();
				}
				else
				{
					BatchElement.PrimitiveUniformBufferResource = &Proxy->GetWorldSpacePrimitiveUniformBuffer();
				}
				int32 TrianglesToRender = Data.OutTriangleCount;
				if ((TrianglesToRender % 2) != 0)
				{
					TrianglesToRender--;
				}
				BatchElement.NumPrimitives			= TrianglesToRender;
				BatchElement.MinVertexIndex			= 0;
				BatchElement.MaxVertexIndex			= Source.VertexCount - 1;
				Mesh.UseDynamicData			= false;
				Mesh.ReverseCulling			= Proxy->IsLocalToWorldDeterminantNegative();
				Mesh.CastShadow				= Proxy->GetCastShadow();
				Mesh.DepthPriorityGroup		= (ESceneDepthPriorityGroup)Proxy->GetDepthPriorityGroup(View);

				if (AllowDebugViewmodes() && bIsWireframe && !View->Family->EngineShowFlags.Materials)
				{
					Mesh.MaterialRenderProxy	= Proxy->GetDeselectedWireframeMatInst();
				}
				else
				{
					Mesh.MaterialRenderProxy	= MaterialResource[GIsEditor && (View->Family->EngineShowFlags.Selection) ? bSelected : 0];
				}
				Mesh.Type = PT_TriangleStrip;

				NumDraws += DrawRichMesh(
					PDI,
					Mesh,
					FLinearColor(1.0f, 0.0f, 0.0f),
					FLinearColor(1.0f, 1.0f, 0.0f),
					FLinearColor(1.0f, 1.0f, 1.0f),
					Proxy,
					GIsEditor && (View->Family->EngineShowFlags.Selection) ? Proxy->IsSelected() : false,
					bIsWireframe
					);

				INC_DWORD_STAT_BY(STAT_BeamParticlesTrianglesRendered, Mesh.GetNumPrimitives());
			}
		}

		if (Source.bRenderDirectLine == true)
		{
			RenderDirectLine(Proxy, PDI, View);
		}

		if ((Source.bRenderLines == true) ||
			(Source.bRenderTessellation == true))
		{
			RenderLines(Proxy, PDI, View);
		}
	}
	return NumDraws;
}

/**
 *	Called during InitViews for view processing on scene proxies before rendering them
 *  Only called for primitives that are visible and have bDynamicRelevance
 *
 *	@param	Proxy			The 'owner' particle system scene proxy
 *	@param	ViewFamily		The ViewFamily to pre-render for
 *	@param	VisibilityMap	A BitArray that indicates whether the primitive was visible in that view (index)
 *	@param	FrameNumber		The frame number of this pre-render
 */
void FDynamicBeam2EmitterData::PreRenderView(FParticleSystemSceneProxy* Proxy, const FSceneViewFamily* ViewFamily, const uint32 VisibilityMap, int32 FrameNumber)
{
	if (bValid == false)
	{
		return;
	}

	// Only need to do this once per-view
	if (LastFramePreRendered < FrameNumber)
	{
		bool bOnlyOneView = !GIsEditor && ((GEngine && GEngine->GameViewport && (GEngine->GameViewport->GetCurrentSplitscreenConfiguration() == eSST_NONE)) ? true : false);

		BuildViewFillDataAndSubmit(Proxy,ViewFamily,VisibilityMap,bOnlyOneView,Source.VertexCount,sizeof(FParticleBeamTrailVertex),0);

		// Set the frame tracker
		LastFramePreRendered = FrameNumber;
	}

	if (Source.bUseLocalSpace == false)
	{
		Proxy->UpdateWorldSpacePrimitiveUniformBuffer();
	}
}

void FDynamicBeam2EmitterData::RenderDirectLine(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View)
{
	for (int32 Beam = 0; Beam < Source.ActiveParticleCount; Beam++)
	{
		DECLARE_PARTICLE_PTR(Particle, Source.ParticleData.GetData() + Source.ParticleStride * Beam);

		FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
		FVector*				InterpolatedPoints	= NULL;
		float*					NoiseRate			= NULL;
		float*					NoiseDelta			= NULL;
		FVector*				TargetNoisePoints	= NULL;
		FVector*				NextNoisePoints		= NULL;
		float*					TaperValues			= NULL;

		BeamPayloadData = (FBeam2TypeDataPayload*)((uint8*)Particle + Source.BeamDataOffset);
		if (BeamPayloadData->TriangleCount == 0)
		{
			continue;
		}

		DrawWireStar(PDI, BeamPayloadData->SourcePoint, 20.0f, FColor(0,255,0),Proxy->GetDepthPriorityGroup(View));
		DrawWireStar(PDI, BeamPayloadData->TargetPoint, 20.0f, FColor(255,0,0),Proxy->GetDepthPriorityGroup(View));
		PDI->DrawLine(BeamPayloadData->SourcePoint, BeamPayloadData->TargetPoint, FColor(255,255,0),Proxy->GetDepthPriorityGroup(View));
	}
}

void FDynamicBeam2EmitterData::RenderLines(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View)
{
	if (Source.bLowFreqNoise_Enabled)
	{
		int32	TrianglesToRender = 0;

		FMatrix WorldToLocal = Proxy->GetWorldToLocal();
		FMatrix LocalToWorld = Proxy->GetLocalToWorld();
		FMatrix CameraToWorld = View->ViewMatrices.ViewMatrix.Inverse();
		FVector	ViewOrigin = CameraToWorld.GetOrigin();

		Source.Sheets = (Source.Sheets > 0) ? Source.Sheets : 1;

		// Frequency is the number of noise points to generate, evenly distributed along the line.
		Source.Frequency = (Source.Frequency > 0) ? Source.Frequency : 1;

		// NoiseTessellation is the amount of tessellation that should occur between noise points.
		int32	TessFactor	= Source.NoiseTessellation ? Source.NoiseTessellation : 1;
		float	InvTessFactor	= 1.0f / TessFactor;
		int32		i;

		// The last position processed
		FVector	LastPosition, LastDrawPosition, LastTangent;
		// The current position
		FVector	CurrPosition, CurrDrawPosition;
		// The target
		FVector	TargetPosition, TargetDrawPosition;
		// The next target
		FVector	NextTargetPosition, NextTargetDrawPosition, TargetTangent;
		// The interperted draw position
		FVector InterpDrawPos;
		FVector	InterimDrawPosition;

		FVector	Size;

		FVector Location;
		FVector EndPoint;
		FVector Offset;
		FVector LastOffset;
		float	fStrength;
		float	fTargetStrength;

		int32	 VertexCount	= 0;

		// Tessellate the beam along the noise points
		for (i = 0; i < Source.ActiveParticleCount; i++)
		{
			DECLARE_PARTICLE_PTR(Particle, Source.ParticleData.GetData() + Source.ParticleStride * i);

			// Retrieve the beam data from the particle.
			FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
			float*					NoiseRate			= NULL;
			float*					NoiseDelta			= NULL;
			FVector*				TargetNoisePoints	= NULL;
			FVector*				NextNoisePoints		= NULL;
			float*					TaperValues			= NULL;
			float*					NoiseDistanceScale	= NULL;

			BeamPayloadData = (FBeam2TypeDataPayload*)((uint8*)Particle + Source.BeamDataOffset);
			if (BeamPayloadData->TriangleCount == 0)
			{
				continue;
			}
			if (Source.NoiseRateOffset != -1)
			{
				NoiseRate = (float*)((uint8*)Particle + Source.NoiseRateOffset);
			}
			if (Source.NoiseDeltaTimeOffset != -1)
			{
				NoiseDelta = (float*)((uint8*)Particle + Source.NoiseDeltaTimeOffset);
			}
			if (Source.TargetNoisePointsOffset != -1)
			{
				TargetNoisePoints = (FVector*)((uint8*)Particle + Source.TargetNoisePointsOffset);
			}
			if (Source.NextNoisePointsOffset != -1)
			{
				NextNoisePoints = (FVector*)((uint8*)Particle + Source.NextNoisePointsOffset);
			}
			if (Source.TaperValuesOffset != -1)
			{
				TaperValues = (float*)((uint8*)Particle + Source.TaperValuesOffset);
			}
			if (Source.NoiseDistanceScaleOffset != -1)
			{
				NoiseDistanceScale = (float*)((uint8*)Particle + Source.NoiseDistanceScaleOffset);
			}

			float NoiseDistScale = 1.0f;
			if (NoiseDistanceScale)
			{
				NoiseDistScale = *NoiseDistanceScale;
			}

			FVector* NoisePoints	= TargetNoisePoints;
			FVector* NextNoise		= NextNoisePoints;

			float NoiseRangeScaleFactor = Source.NoiseRangeScale;
			//@todo. How to handle no noise points?
			// If there are no noise points, why are we in here?
			if (NoisePoints == NULL)
			{
				continue;
			}

			// Pin the size to the X component
			Size = FVector(Particle->Size.X * Source.Scale.X);

			check(TessFactor > 0);

			// Setup the current position as the source point
			CurrPosition		= BeamPayloadData->SourcePoint;
			CurrDrawPosition	= CurrPosition;

			// Setup the source tangent & strength
			if (Source.bUseSource)
			{
				// The source module will have determined the proper source tangent.
				LastTangent	= BeamPayloadData->SourceTangent;
				fStrength	= BeamPayloadData->SourceStrength;
			}
			else
			{
				// We don't have a source module, so use the orientation of the emitter.
				LastTangent	= WorldToLocal.GetScaledAxis( EAxis::X );
				fStrength	= Source.NoiseTangentStrength;
			}
			LastTangent.Normalize();
			LastTangent *= fStrength;
			fTargetStrength	= Source.NoiseTangentStrength;

			// Set the last draw position to the source so we don't get 'under-hang'
			LastPosition		= CurrPosition;
			LastDrawPosition	= CurrDrawPosition;

			bool	bLocked	= BEAM2_TYPEDATA_LOCKED(BeamPayloadData->Lock_Max_NumNoisePoints);

			FVector	UseNoisePoint, CheckNoisePoint;
			FVector	NoiseDir;

			// Reset the texture coordinate
			LastPosition		= BeamPayloadData->SourcePoint;
			LastDrawPosition	= LastPosition;

			// Determine the current position by stepping the direct line and offsetting with the noise point. 
			CurrPosition		= LastPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;

			if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
			{
				NoiseDir		= NextNoise[0] - NoisePoints[0];
				NoiseDir.Normalize();
				CheckNoisePoint	= NoisePoints[0] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
				if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[0].X) < Source.NoiseLockRadius) &&
					(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[0].Y) < Source.NoiseLockRadius) &&
					(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[0].Z) < Source.NoiseLockRadius))
				{
					NoisePoints[0]	= NextNoise[0];
				}
				else
				{
					NoisePoints[0]	= CheckNoisePoint;
				}
			}

			CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[0] * NoiseDistScale);

			// Determine the offset for the leading edge
			Location	= LastDrawPosition;
			EndPoint	= CurrDrawPosition;

			// 'Lead' edge
			DrawWireStar(PDI, Location, 15.0f, FColor(0,255,0), Proxy->GetDepthPriorityGroup(View));

			for (int32 StepIndex = 0; StepIndex < BeamPayloadData->Steps; StepIndex++)
			{
				// Determine the current position by stepping the direct line and offsetting with the noise point. 
				CurrPosition		= LastPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;

				if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
				{
					NoiseDir		= NextNoise[StepIndex] - NoisePoints[StepIndex];
					NoiseDir.Normalize();
					CheckNoisePoint	= NoisePoints[StepIndex] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
					if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[StepIndex].X) < Source.NoiseLockRadius) &&
						(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[StepIndex].Y) < Source.NoiseLockRadius) &&
						(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[StepIndex].Z) < Source.NoiseLockRadius))
					{
						NoisePoints[StepIndex]	= NextNoise[StepIndex];
					}
					else
					{
						NoisePoints[StepIndex]	= CheckNoisePoint;
					}
				}

				CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[StepIndex] * NoiseDistScale);

				// Prep the next draw position to determine tangents
				bool bTarget = false;
				NextTargetPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
				if (bLocked && ((StepIndex + 1) == BeamPayloadData->Steps))
				{
					// If we are locked, and the next step is the target point, set the draw position as such.
					// (ie, we are on the last noise point...)
					NextTargetDrawPosition	= BeamPayloadData->TargetPoint;
					if (Source.bTargetNoise)
					{
						if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
						{
							NoiseDir		= NextNoise[Source.Frequency] - NoisePoints[Source.Frequency];
							NoiseDir.Normalize();
							CheckNoisePoint	= NoisePoints[Source.Frequency] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
							if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[Source.Frequency].X) < Source.NoiseLockRadius) &&
								(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[Source.Frequency].Y) < Source.NoiseLockRadius) &&
								(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[Source.Frequency].Z) < Source.NoiseLockRadius))
							{
								NoisePoints[Source.Frequency]	= NextNoise[Source.Frequency];
							}
							else
							{
								NoisePoints[Source.Frequency]	= CheckNoisePoint;
							}
						}

						NextTargetDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[Source.Frequency] * NoiseDistScale);
					}
					TargetTangent = BeamPayloadData->TargetTangent;
					fTargetStrength	= BeamPayloadData->TargetStrength;
				}
				else
				{
					// Just another noise point... offset the target to get the draw position.
					if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
					{
						NoiseDir		= NextNoise[StepIndex + 1] - NoisePoints[StepIndex + 1];
						NoiseDir.Normalize();
						CheckNoisePoint	= NoisePoints[StepIndex + 1] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
						if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[StepIndex + 1].X) < Source.NoiseLockRadius) &&
							(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[StepIndex + 1].Y) < Source.NoiseLockRadius) &&
							(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[StepIndex + 1].Z) < Source.NoiseLockRadius))
						{
							NoisePoints[StepIndex + 1]	= NextNoise[StepIndex + 1];
						}
						else
						{
							NoisePoints[StepIndex + 1]	= CheckNoisePoint;
						}
					}

					NextTargetDrawPosition	= NextTargetPosition + NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[StepIndex + 1] * NoiseDistScale);

					TargetTangent = ((1.0f - Source.NoiseTension) / 2.0f) * (NextTargetDrawPosition - LastDrawPosition);
				}
				TargetTangent.Normalize();
				TargetTangent *= fTargetStrength;

				InterimDrawPosition = LastDrawPosition;
				// Tessellate between the current position and the last position
				for (int32 TessIndex = 0; TessIndex < TessFactor; TessIndex++)
				{
					InterpDrawPos = FMath::CubicInterp(
						LastDrawPosition, LastTangent,
						CurrDrawPosition, TargetTangent,
						InvTessFactor * (TessIndex + 1));

					Location	= InterimDrawPosition;
					EndPoint	= InterpDrawPos;

					FColor StarColor(255,0,255);
					if (TessIndex == 0)
					{
						StarColor = FColor(0,0,255);
					}
					else
					if (TessIndex == (TessFactor - 1))
					{
						StarColor = FColor(255,255,0);
					}

					// Generate the vertex
					DrawWireStar(PDI, EndPoint, 15.0f, StarColor, Proxy->GetDepthPriorityGroup(View));
					PDI->DrawLine(Location, EndPoint, FLinearColor(1.0f,1.0f,0.0f), Proxy->GetDepthPriorityGroup(View));
					InterimDrawPosition	= InterpDrawPos;
				}
				LastPosition		= CurrPosition;
				LastDrawPosition	= CurrDrawPosition;
				LastTangent			= TargetTangent;
			}

			if (bLocked)
			{
				// Draw the line from the last point to the target
				CurrDrawPosition	= BeamPayloadData->TargetPoint;
				if (Source.bTargetNoise)
				{
					if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
					{
						NoiseDir		= NextNoise[Source.Frequency] - NoisePoints[Source.Frequency];
						NoiseDir.Normalize();
						CheckNoisePoint	= NoisePoints[Source.Frequency] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
						if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[Source.Frequency].X) < Source.NoiseLockRadius) &&
							(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[Source.Frequency].Y) < Source.NoiseLockRadius) &&
							(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[Source.Frequency].Z) < Source.NoiseLockRadius))
						{
							NoisePoints[Source.Frequency]	= NextNoise[Source.Frequency];
						}
						else
						{
							NoisePoints[Source.Frequency]	= CheckNoisePoint;
						}
					}

					CurrDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[Source.Frequency] * NoiseDistScale);
				}

				if (Source.bUseTarget)
				{
					TargetTangent = BeamPayloadData->TargetTangent;
				}
				else
				{
					NextTargetDrawPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
					TargetTangent = ((1.0f - Source.NoiseTension) / 2.0f) * 
						(NextTargetDrawPosition - LastDrawPosition);
				}
				TargetTangent.Normalize();
				TargetTangent *= fTargetStrength;

				// Tessellate this segment
				InterimDrawPosition = LastDrawPosition;
				for (int32 TessIndex = 0; TessIndex < TessFactor; TessIndex++)
				{
					InterpDrawPos = FMath::CubicInterp(
						LastDrawPosition, LastTangent,
						CurrDrawPosition, TargetTangent,
						InvTessFactor * (TessIndex + 1));

					Location	= InterimDrawPosition;
					EndPoint	= InterpDrawPos;

					FColor StarColor(255,0,255);
					if (TessIndex == 0)
					{
						StarColor = FColor(255,255,255);
					}
					else
					if (TessIndex == (TessFactor - 1))
					{
						StarColor = FColor(255,255,0);
					}

					// Generate the vertex
					DrawWireStar(PDI, EndPoint, 15.0f, StarColor, Proxy->GetDepthPriorityGroup(View));
					PDI->DrawLine(Location, EndPoint, FLinearColor(1.0f,1.0f,0.0f), Proxy->GetDepthPriorityGroup(View));
					VertexCount++;
					InterimDrawPosition	= InterpDrawPos;
				}
			}
		}
	}

	if (Source.InterpolationPoints > 1)
	{
		FMatrix CameraToWorld = View->ViewMatrices.ViewMatrix.Inverse();
		FVector	ViewOrigin = CameraToWorld.GetOrigin();
		int32 TessFactor = Source.InterpolationPoints ? Source.InterpolationPoints : 1;

		if (TessFactor <= 1)
		{
			for (int32 i = 0; i < Source.ActiveParticleCount; i++)
			{
				DECLARE_PARTICLE_PTR(Particle, Source.ParticleData.GetData() + Source.ParticleStride * i);
				FBeam2TypeDataPayload* BeamPayloadData = (FBeam2TypeDataPayload*)((uint8*)Particle + Source.BeamDataOffset);
				if (BeamPayloadData->TriangleCount == 0)
				{
					continue;
				}

				FVector EndPoint	= Particle->Location;
				FVector Location	= BeamPayloadData->SourcePoint;

				DrawWireStar(PDI, Location, 15.0f, FColor(255,0,0), Proxy->GetDepthPriorityGroup(View));
				DrawWireStar(PDI, EndPoint, 15.0f, FColor(255,0,0), Proxy->GetDepthPriorityGroup(View));
				PDI->DrawLine(Location, EndPoint, FColor(255,255,0), Proxy->GetDepthPriorityGroup(View));
			}
		}
		else
		{
			for (int32 i = 0; i < Source.ActiveParticleCount; i++)
			{
				DECLARE_PARTICLE_PTR(Particle, Source.ParticleData.GetData() + Source.ParticleStride * i);

				FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
				FVector*				InterpolatedPoints	= NULL;

				BeamPayloadData = (FBeam2TypeDataPayload*)((uint8*)Particle + Source.BeamDataOffset);
				if (BeamPayloadData->TriangleCount == 0)
				{
					continue;
				}
				if (Source.InterpolatedPointsOffset != -1)
				{
					InterpolatedPoints = (FVector*)((uint8*)Particle + Source.InterpolatedPointsOffset);
				}

				FVector Location;
				FVector EndPoint;

				check(InterpolatedPoints);	// TTP #33139

				Location	= BeamPayloadData->SourcePoint;
				EndPoint	= InterpolatedPoints[0];

				DrawWireStar(PDI, Location, 15.0f, FColor(255,0,0), Proxy->GetDepthPriorityGroup(View));
				for (int32 StepIndex = 0; StepIndex < BeamPayloadData->InterpolationSteps; StepIndex++)
				{
					EndPoint = InterpolatedPoints[StepIndex];
					DrawWireStar(PDI, EndPoint, 15.0f, FColor(255,0,0), Proxy->GetDepthPriorityGroup(View));
					PDI->DrawLine(Location, EndPoint, FColor(255,255,0), Proxy->GetDepthPriorityGroup(View));
					Location = EndPoint;
				}
			}
		}
	}
}

void FDynamicBeam2EmitterData::RenderDebug(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bCrosses)
{
}

void FDynamicBeam2EmitterData::GetIndexAllocInfo(int32& OutNumIndices, int32& OutStride ) const
{
	//	bool bWireframe = (View->Family->EngineShowFlags.Wireframe) && !View->Family->EngineShowFlags.Materials;
	bool bWireframe = false;

	int32 TempIndexCount = 0;
	for (int32 ii = 0; ii < Source.TrianglesPerSheet.Num(); ii++)
	{
		int32 Triangles = Source.TrianglesPerSheet[ii];
		if (bWireframe)
		{
			TempIndexCount += (8 * Triangles + 2) * Source.Sheets;
		}
		else
		{
			if (TempIndexCount == 0)
			{
				TempIndexCount = 2;
			}
			TempIndexCount += Triangles * Source.Sheets;
			TempIndexCount += 4 * (Source.Sheets - 1);	// Degenerate indices between sheets
			if ((ii + 1) < Source.TrianglesPerSheet.Num())
			{
				TempIndexCount += 4;	// Degenerate indices between beams
			}
		}
	}

	OutNumIndices = TempIndexCount;
	OutStride = Source.IndexStride;
}

int32 FDynamicBeam2EmitterData::FillIndexData(struct FAsyncBufferFillData& Data)
{
	SCOPE_CYCLE_COUNTER(STAT_BeamFillIndexTime);

	int32	TrianglesToRender = 0;

	//	bool bWireframe = (View->Family->EngineShowFlags.Wireframe) && !View->Family->EngineShowFlags.Materials;
	bool bWireframe = false;

	// Beam2 polygons are packed and joined as follows:
	//
	// 1--3--5--7--9-...
	// |\ |\ |\ |\ |\...
	// | \| \| \| \| ...
	// 0--2--4--6--8-...
	//
	// (ie, the 'leading' edge of polygon (n) is the trailing edge of polygon (n+1)
	//
	// NOTE: This is primed for moving to tri-strips...
	//
	int32 TessFactor	= Source.InterpolationPoints ? Source.InterpolationPoints : 1;
	if (Source.Sheets <= 0)
	{
		Source.Sheets = 1;
	}

	check( Data.IndexCount > 0 && Data.IndexData != NULL );

	if (Source.IndexStride == sizeof(uint16))
	{
		uint16*	Index				= (uint16*)Data.IndexData;
		uint16	VertexIndex			= 0;
		uint16	StartVertexIndex	= 0;

		for (int32 Beam = 0; Beam < Source.ActiveParticleCount; Beam++)
		{
			DECLARE_PARTICLE_PTR(Particle, Source.ParticleData.GetData() + Source.ParticleStride * Beam);

			FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
			FVector*				InterpolatedPoints	= NULL;
			float*					NoiseRate			= NULL;
			float*					NoiseDelta			= NULL;
			FVector*				TargetNoisePoints	= NULL;
			FVector*				NextNoisePoints		= NULL;
			float*					TaperValues			= NULL;

			BeamPayloadData = (FBeam2TypeDataPayload*)((uint8*)Particle + Source.BeamDataOffset);
			if (BeamPayloadData->TriangleCount == 0)
			{
				continue;
			}
			if ((Source.InterpolationPoints > 0) && (BeamPayloadData->Steps == 0))
			{
				continue;
			}

			if (bWireframe)
			{
				for (int32 SheetIndex = 0; SheetIndex < Source.Sheets; SheetIndex++)
				{
					VertexIndex = 0;

					// The 'starting' line
					TrianglesToRender += 1;
					*(Index++) = StartVertexIndex + 0;
					*(Index++) = StartVertexIndex + 1;

					// 4 lines per quad
					int32 TriCount = Source.TrianglesPerSheet[Beam];
					int32 QuadCount = TriCount / 2;
					TrianglesToRender += TriCount * 2;

					for (int32 i = 0; i < QuadCount; i++)
					{
						*(Index++) = StartVertexIndex + VertexIndex + 0;
						*(Index++) = StartVertexIndex + VertexIndex + 2;
						*(Index++) = StartVertexIndex + VertexIndex + 1;
						*(Index++) = StartVertexIndex + VertexIndex + 2;
						*(Index++) = StartVertexIndex + VertexIndex + 1;
						*(Index++) = StartVertexIndex + VertexIndex + 3;
						*(Index++) = StartVertexIndex + VertexIndex + 2;
						*(Index++) = StartVertexIndex + VertexIndex + 3;

						VertexIndex += 2;
					}

					StartVertexIndex += TriCount + 2;
				}
			}
			else
			{
				// 
				if (Beam == 0)
				{
					*(Index++) = VertexIndex++;	// SheetIndex + 0
					*(Index++) = VertexIndex++;	// SheetIndex + 1
				}

				for (int32 SheetIndex = 0; SheetIndex < Source.Sheets; SheetIndex++)
				{
					// 2 triangles per tessellation factor
					TrianglesToRender += BeamPayloadData->TriangleCount;

					// Sequentially step through each triangle - 1 vertex per triangle
					for (int32 i = 0; i < BeamPayloadData->TriangleCount; i++)
					{
						*(Index++) = VertexIndex++;
					}

					// Degenerate tris
					if ((SheetIndex + 1) < Source.Sheets)
					{
						*(Index++) = VertexIndex - 1;	// Last vertex of the previous sheet
						*(Index++) = VertexIndex;		// First vertex of the next sheet
						*(Index++) = VertexIndex++;		// First vertex of the next sheet
						*(Index++) = VertexIndex++;		// Second vertex of the next sheet

						TrianglesToRender += 4;
					}
				}
				if ((Beam + 1) < Source.ActiveParticleCount)
				{
					*(Index++) = VertexIndex - 1;	// Last vertex of the previous sheet
					*(Index++) = VertexIndex;		// First vertex of the next sheet
					*(Index++) = VertexIndex++;		// First vertex of the next sheet
					*(Index++) = VertexIndex++;		// Second vertex of the next sheet

					TrianglesToRender += 4;
				}
			}
		}
	}
	else
	{
		check(!TEXT("Rendering beam with > 5000 vertices!"));
		uint32*	Index		= (uint32*)Data.IndexData;
		uint32	VertexIndex	= 0;
		for (int32 Beam = 0; Beam < Source.ActiveParticleCount; Beam++)
		{
			DECLARE_PARTICLE_PTR(Particle, Source.ParticleData.GetData() + Source.ParticleStride * Beam);

			FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
			BeamPayloadData = (FBeam2TypeDataPayload*)((uint8*)Particle + Source.BeamDataOffset);
			if (BeamPayloadData->TriangleCount == 0)
			{
				continue;
			}

			// 
			if (Beam == 0)
			{
				*(Index++) = VertexIndex++;	// SheetIndex + 0
				*(Index++) = VertexIndex++;	// SheetIndex + 1
			}

			for (int32 SheetIndex = 0; SheetIndex < Source.Sheets; SheetIndex++)
			{
				// 2 triangles per tessellation factor
				TrianglesToRender += BeamPayloadData->TriangleCount;

				// Sequentially step through each triangle - 1 vertex per triangle
				for (int32 i = 0; i < BeamPayloadData->TriangleCount; i++)
				{
					*(Index++) = VertexIndex++;
				}

				// Degenerate tris
				if ((SheetIndex + 1) < Source.Sheets)
				{
					*(Index++) = VertexIndex - 1;	// Last vertex of the previous sheet
					*(Index++) = VertexIndex;		// First vertex of the next sheet
					*(Index++) = VertexIndex++;		// First vertex of the next sheet
					*(Index++) = VertexIndex++;		// Second vertex of the next sheet
					TrianglesToRender += 4;
				}
			}
			if ((Beam + 1) < Source.ActiveParticleCount)
			{
				*(Index++) = VertexIndex - 1;	// Last vertex of the previous sheet
				*(Index++) = VertexIndex;		// First vertex of the next sheet
				*(Index++) = VertexIndex++;		// First vertex of the next sheet
				*(Index++) = VertexIndex++;		// Second vertex of the next sheet
				TrianglesToRender += 4;
			}
		}
	}

	Data.OutTriangleCount = TrianglesToRender;
	return TrianglesToRender;
}

int32 FDynamicBeam2EmitterData::FillVertexData_NoNoise(FAsyncBufferFillData& Me)
{
	SCOPE_CYCLE_COUNTER(STAT_BeamFillVertexTime);

	int32	TrianglesToRender = 0;

	FParticleBeamTrailVertex* Vertex = (FParticleBeamTrailVertex*)Me.VertexData;
	FMatrix CameraToWorld = Me.View->ViewMatrices.ViewMatrix.InverseSafe();
	FVector	ViewOrigin = CameraToWorld.GetOrigin();
	int32 TessFactor = Source.InterpolationPoints ? Source.InterpolationPoints : 1;

	if (Source.Sheets <= 0)
	{
		Source.Sheets = 1;
	}

	FVector	Offset(0.0f), LastOffset(0.0f);

	int32 PackedCount = 0;

	if (TessFactor <= 1)
	{
		for (int32 i = 0; i < Source.ActiveParticleCount; i++)
		{
			DECLARE_PARTICLE_PTR(Particle, Source.ParticleData.GetData() + Source.ParticleStride * i);

			FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
			FVector*				InterpolatedPoints	= NULL;
			float*					NoiseRate			= NULL;
			float*					NoiseDelta			= NULL;
			FVector*				TargetNoisePoints	= NULL;
			FVector*				NextNoisePoints		= NULL;
			float*					TaperValues			= NULL;

			BeamPayloadData = (FBeam2TypeDataPayload*)((uint8*)Particle + Source.BeamDataOffset);
			if (BeamPayloadData->TriangleCount == 0)
			{
				continue;
			}
			if (Source.InterpolatedPointsOffset != -1)
			{
				InterpolatedPoints = (FVector*)((uint8*)Particle + Source.InterpolatedPointsOffset);
			}
			if (Source.NoiseRateOffset != -1)
			{
				NoiseRate = (float*)((uint8*)Particle + Source.NoiseRateOffset);
			}
			if (Source.NoiseDeltaTimeOffset != -1)
			{
				NoiseDelta = (float*)((uint8*)Particle + Source.NoiseDeltaTimeOffset);
			}
			if (Source.TargetNoisePointsOffset != -1)
			{
				TargetNoisePoints = (FVector*)((uint8*)Particle + Source.TargetNoisePointsOffset);
			}
			if (Source.NextNoisePointsOffset != -1)
			{
				NextNoisePoints = (FVector*)((uint8*)Particle + Source.NextNoisePointsOffset);
			}
			if (Source.TaperValuesOffset != -1)
			{
				TaperValues = (float*)((uint8*)Particle + Source.TaperValuesOffset);
			}

			// Pin the size to the X component
			FVector2D Size(Particle->Size.X * Source.Scale.X, Particle->Size.X * Source.Scale.X);

			FVector EndPoint	= Particle->Location;
			FVector Location	= BeamPayloadData->SourcePoint;
			FVector Right, Up;
			FVector WorkingUp;

			Right = Location - EndPoint;
			Right.Normalize();
			if (((Source.UpVectorStepSize == 1) && (i == 0)) || (Source.UpVectorStepSize == 0))
			{
				//Up = Right ^ ViewDirection;
				Up = Right ^ (Location - ViewOrigin);
				if (!Up.Normalize())
				{
					Up = CameraToWorld.GetScaledAxis( EAxis::Y );
				}
			}

			float	fUEnd;
			float	Tiles		= 1.0f;
			if (Source.TextureTileDistance > KINDA_SMALL_NUMBER)
			{
				FVector	Direction	= BeamPayloadData->TargetPoint - BeamPayloadData->SourcePoint;
				float	Distance	= Direction.Size();
				Tiles				= Distance / Source.TextureTileDistance;
			}
			fUEnd		= Tiles;

			if (BeamPayloadData->TravelRatio > KINDA_SMALL_NUMBER)
			{
				fUEnd	= Tiles * BeamPayloadData->TravelRatio;
			}

			// For the direct case, this isn't a big deal, as it will not require much work per sheet.
			for (int32 SheetIndex = 0; SheetIndex < Source.Sheets; SheetIndex++)
			{
				if (SheetIndex)
				{
					float	Angle		= ((float)PI / (float)Source.Sheets) * SheetIndex;
					FQuat	QuatRotator	= FQuat(Right, Angle);
					WorkingUp			= QuatRotator.RotateVector(Up);
				}
				else
				{
					WorkingUp	= Up;
				}

				float	Taper	= 1.0f;
				if (Source.TaperMethod != PEBTM_None)
				{
					check(TaperValues);
					Taper	= TaperValues[0];
				}

				// Size is locked to X, see initialization of Size.
				Offset.X		= WorkingUp.X * Size.X * Taper;
				Offset.Y		= WorkingUp.Y * Size.X * Taper;
				Offset.Z		= WorkingUp.Z * Size.X * Taper;

				// 'Lead' edge
				Vertex->Position	= Location + Offset;
				Vertex->OldPosition	= Location;
				Vertex->ParticleId	= 0;
				Vertex->Size		= Size;
				Vertex->Tex_U		= 0.0f;
				Vertex->Tex_V		= 0.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				PackedCount++;

				Vertex->Position	= Location - Offset;
				Vertex->OldPosition	= Location;
				Vertex->ParticleId	= 0;
				Vertex->Size		= Size;
				Vertex->Tex_U		= 0.0f;
				Vertex->Tex_V		= 1.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				PackedCount++;

				if (Source.TaperMethod != PEBTM_None)
				{
					check(TaperValues);
					Taper	= TaperValues[1];
				}

				// Size is locked to X, see initialization of Size.
				Offset.X		= WorkingUp.X * Size.X * Taper;
				Offset.Y		= WorkingUp.Y * Size.X * Taper;
				Offset.Z		= WorkingUp.Z * Size.X * Taper;

				//
				Vertex->Position	= EndPoint + Offset;
				Vertex->OldPosition	= Particle->OldLocation;
				Vertex->ParticleId	= 0;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fUEnd;
				Vertex->Tex_V		= 0.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				PackedCount++;

				Vertex->Position	= EndPoint - Offset;
				Vertex->OldPosition	= Particle->OldLocation;
				Vertex->ParticleId	= 0;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fUEnd;
				Vertex->Tex_V		= 1.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				PackedCount++;
			}
		}
	}
	else
	{
		float	fTextureIncrement	= 1.0f / Source.InterpolationPoints;;

		for (int32 i = 0; i < Source.ActiveParticleCount; i++)
		{
			DECLARE_PARTICLE_PTR(Particle, Source.ParticleData.GetData() + Source.ParticleStride * i);

			FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
			FVector*				InterpolatedPoints	= NULL;
			float*					NoiseRate			= NULL;
			float*					NoiseDelta			= NULL;
			FVector*				TargetNoisePoints	= NULL;
			FVector*				NextNoisePoints		= NULL;
			float*					TaperValues			= NULL;

			BeamPayloadData = (FBeam2TypeDataPayload*)((uint8*)Particle + Source.BeamDataOffset);
			if (BeamPayloadData->TriangleCount == 0)
			{
				continue;
			}
			if (Source.InterpolatedPointsOffset != -1)
			{
				InterpolatedPoints = (FVector*)((uint8*)Particle + Source.InterpolatedPointsOffset);
			}
			if (Source.NoiseRateOffset != -1)
			{
				NoiseRate = (float*)((uint8*)Particle + Source.NoiseRateOffset);
			}
			if (Source.NoiseDeltaTimeOffset != -1)
			{
				NoiseDelta = (float*)((uint8*)Particle + Source.NoiseDeltaTimeOffset);
			}
			if (Source.TargetNoisePointsOffset != -1)
			{
				TargetNoisePoints = (FVector*)((uint8*)Particle + Source.TargetNoisePointsOffset);
			}
			if (Source.NextNoisePointsOffset != -1)
			{
				NextNoisePoints = (FVector*)((uint8*)Particle + Source.NextNoisePointsOffset);
			}
			if (Source.TaperValuesOffset != -1)
			{
				TaperValues = (float*)((uint8*)Particle + Source.TaperValuesOffset);
			}

			if (Source.TextureTileDistance > KINDA_SMALL_NUMBER)
			{
				FVector	Direction	= BeamPayloadData->TargetPoint - BeamPayloadData->SourcePoint;
				float	Distance	= Direction.Size();
				float	Tiles		= Distance / Source.TextureTileDistance;
				fTextureIncrement	= Tiles / Source.InterpolationPoints;
			}

			// Pin the size to the X component
			FVector2D Size(Particle->Size.X * Source.Scale.X, Particle->Size.X * Source.Scale.X);

			float	Angle;
			FQuat	QuatRotator(0, 0, 0, 0);

			FVector Location;
			FVector EndPoint;
			FVector Right;
			FVector Up;
			FVector WorkingUp;
			float	fU;

			check(InterpolatedPoints);	// TTP #33139
			// For the direct case, this isn't a big deal, as it will not require much work per sheet.
			for (int32 SheetIndex = 0; SheetIndex < Source.Sheets; SheetIndex++)
			{
				fU			= 0.0f;
				Location	= BeamPayloadData->SourcePoint;
				EndPoint	= InterpolatedPoints[0];
				Right		= Location - EndPoint;
				Right.Normalize();
				if (Source.UpVectorStepSize == 0)
				{
					//Up = Right ^ ViewDirection;
					Up = Right ^ (Location - ViewOrigin);
					if (!Up.Normalize())
					{
						Up = CameraToWorld.GetScaledAxis( EAxis::Y );
					}
				}

				if (SheetIndex)
				{
					Angle		= ((float)PI / (float)Source.Sheets) * SheetIndex;
					QuatRotator	= FQuat(Right, Angle);
					WorkingUp	= QuatRotator.RotateVector(Up);
				}
				else
				{
					WorkingUp	= Up;
				}

				float	Taper	= 1.0f;

				if (Source.TaperMethod != PEBTM_None)
				{
					check(TaperValues);
					Taper	= TaperValues[0];
				}

				// Size is locked to X, see initialization of Size.
				Offset.X	= WorkingUp.X * Size.X * Taper;
				Offset.Y	= WorkingUp.Y * Size.X * Taper;
				Offset.Z	= WorkingUp.Z * Size.X * Taper;

				// 'Lead' edge
				Vertex->Position	= Location + Offset;
				Vertex->OldPosition	= Location;
				Vertex->ParticleId	= 0;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fU;
				Vertex->Tex_V		= 0.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				PackedCount++;

				Vertex->Position	= Location - Offset;
				Vertex->OldPosition	= Location;
				Vertex->ParticleId	= 0;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fU;
				Vertex->Tex_V		= 1.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				PackedCount++;

				for (int32 StepIndex = 0; StepIndex < BeamPayloadData->Steps; StepIndex++)
				{
					EndPoint	= InterpolatedPoints[StepIndex];
					if (Source.UpVectorStepSize == 0)
					{
						//Up = Right ^ ViewDirection;
						Up = Right ^ (Location - ViewOrigin);
						if (!Up.Normalize())
						{
							Up = CameraToWorld.GetScaledAxis( EAxis::Y );
						}
					}

					if (SheetIndex)
					{
						WorkingUp	= QuatRotator.RotateVector(Up);
					}
					else
					{
						WorkingUp	= Up;
					}

					if (Source.TaperMethod != PEBTM_None)
					{
						check(TaperValues);
						Taper	= TaperValues[StepIndex + 1];
					}

					// Size is locked to X, see initialization of Size.
					Offset.X		= WorkingUp.X * Size.X * Taper;
					Offset.Y		= WorkingUp.Y * Size.X * Taper;
					Offset.Z		= WorkingUp.Z * Size.X * Taper;

					//
					Vertex->Position	= EndPoint + Offset;
					Vertex->OldPosition	= EndPoint;
					Vertex->ParticleId	= 0;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU + fTextureIncrement;
					Vertex->Tex_V		= 0.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;
				PackedCount++;

					Vertex->Position	= EndPoint - Offset;
					Vertex->OldPosition	= EndPoint;
					Vertex->ParticleId	= 0;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU + fTextureIncrement;
					Vertex->Tex_V		= 1.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;
				PackedCount++;

					Location			 = EndPoint;
					fU					+= fTextureIncrement;
				}

				if (BeamPayloadData->TravelRatio > KINDA_SMALL_NUMBER)
				{
					//@todo.SAS. Re-implement partial-segment beams
				}
			}
		}
	}

	check(PackedCount <= Source.VertexCount);

	return TrianglesToRender;
}

int32 FDynamicBeam2EmitterData::FillData_Noise(FAsyncBufferFillData& Me)
{
	SCOPE_CYCLE_COUNTER(STAT_BeamFillVertexTime);

	int32	TrianglesToRender = 0;

	if (Source.InterpolationPoints > 0)
	{
		return FillData_InterpolatedNoise(Me);
	}

	FParticleBeamTrailVertex* Vertex = (FParticleBeamTrailVertex*)Me.VertexData;
	FMatrix CameraToWorld = Me.View->ViewMatrices.ViewMatrix.InverseSafe();

	if (Source.Sheets <= 0)
	{
		Source.Sheets = 1;
	}

	FVector	ViewOrigin	= CameraToWorld.GetOrigin();

	// Frequency is the number of noise points to generate, evenly distributed along the line.
	if (Source.Frequency <= 0)
	{
		Source.Frequency = 1;
	}

	// NoiseTessellation is the amount of tessellation that should occur between noise points.
	int32	TessFactor	= Source.NoiseTessellation ? Source.NoiseTessellation : 1;
	
	float	InvTessFactor	= 1.0f / TessFactor;
	int32		i;

	// The last position processed
	FVector	LastPosition, LastDrawPosition, LastTangent;
	// The current position
	FVector	CurrPosition, CurrDrawPosition;
	// The target
	FVector	TargetPosition, TargetDrawPosition;
	// The next target
	FVector	NextTargetPosition, NextTargetDrawPosition, TargetTangent;
	// The interperted draw position
	FVector InterpDrawPos;
	FVector	InterimDrawPosition;

	float	Angle;
	FQuat	QuatRotator;

	FVector Location;
	FVector EndPoint;
	FVector Right;
	FVector Up;
	FVector WorkingUp;
	FVector LastUp;
	FVector WorkingLastUp;
	FVector Offset;
	FVector LastOffset;
	float	fStrength;
	float	fTargetStrength;

	float	fU;
	float	TextureIncrement	= 1.0f / (((Source.Frequency > 0) ? Source.Frequency : 1) * TessFactor);	// TTP #33140/33159

	int32	 CheckVertexCount	= 0;

	FVector THE_Up(0.0f);

	FMatrix WorldToLocal = Me.WorldToLocal;
	FMatrix LocalToWorld = Me.LocalToWorld;

	// Tessellate the beam along the noise points
	for (i = 0; i < Source.ActiveParticleCount; i++)
	{
		DECLARE_PARTICLE_PTR(Particle, Source.ParticleData.GetData() + Source.ParticleStride * i);

		// Retrieve the beam data from the particle.
		FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
		FVector*				InterpolatedPoints	= NULL;
		float*					NoiseRate			= NULL;
		float*					NoiseDelta			= NULL;
		FVector*				TargetNoisePoints	= NULL;
		FVector*				NextNoisePoints		= NULL;
		float*					TaperValues			= NULL;
		float*					NoiseDistanceScale	= NULL;

		BeamPayloadData = (FBeam2TypeDataPayload*)((uint8*)Particle + Source.BeamDataOffset);
		if (BeamPayloadData->TriangleCount == 0)
		{
			continue;
		}
		if (Source.InterpolatedPointsOffset != -1)
		{
			InterpolatedPoints = (FVector*)((uint8*)Particle + Source.InterpolatedPointsOffset);
		}
		if (Source.NoiseRateOffset != -1)
		{
			NoiseRate = (float*)((uint8*)Particle + Source.NoiseRateOffset);
		}
		if (Source.NoiseDeltaTimeOffset != -1)
		{
			NoiseDelta = (float*)((uint8*)Particle + Source.NoiseDeltaTimeOffset);
		}
		if (Source.TargetNoisePointsOffset != -1)
		{
			TargetNoisePoints = (FVector*)((uint8*)Particle + Source.TargetNoisePointsOffset);
		}
		if (Source.NextNoisePointsOffset != -1)
		{
			NextNoisePoints = (FVector*)((uint8*)Particle + Source.NextNoisePointsOffset);
		}
		if (Source.TaperValuesOffset != -1)
		{
			TaperValues = (float*)((uint8*)Particle + Source.TaperValuesOffset);
		}
		if (Source.NoiseDistanceScaleOffset != -1)
		{
			NoiseDistanceScale = (float*)((uint8*)Particle + Source.NoiseDistanceScaleOffset);
		}

		float NoiseDistScale = 1.0f;
		if (NoiseDistanceScale)
		{
			NoiseDistScale = *NoiseDistanceScale;
		}

		FVector* NoisePoints	= TargetNoisePoints;
		FVector* NextNoise		= NextNoisePoints;

		float NoiseRangeScaleFactor = Source.NoiseRangeScale;
		//@todo. How to handle no noise points?
		// If there are no noise points, why are we in here?
		if (NoisePoints == NULL)
		{
			continue;
		}

		// Pin the size to the X component
		FVector2D Size(Particle->Size.X * Source.Scale.X, Particle->Size.X * Source.Scale.X);

		if (TessFactor <= 1)
		{
			// Setup the current position as the source point
			CurrPosition		= BeamPayloadData->SourcePoint;
			CurrDrawPosition	= CurrPosition;

			// Setup the source tangent & strength
			if (Source.bUseSource)
			{
				// The source module will have determined the proper source tangent.
				LastTangent	= BeamPayloadData->SourceTangent;
				fStrength	= BeamPayloadData->SourceStrength;
			}
			else
			{
				// We don't have a source module, so use the orientation of the emitter.
				LastTangent	= WorldToLocal.GetScaledAxis( EAxis::X );
				fStrength	= Source.NoiseTangentStrength;
			}
			LastTangent.Normalize();
			LastTangent *= fStrength;

			fTargetStrength	= Source.NoiseTangentStrength;

			// Set the last draw position to the source so we don't get 'under-hang'
			LastPosition		= CurrPosition;
			LastDrawPosition	= CurrDrawPosition;

			bool	bLocked	= BEAM2_TYPEDATA_LOCKED(BeamPayloadData->Lock_Max_NumNoisePoints);

			FVector	UseNoisePoint, CheckNoisePoint;
			FVector	NoiseDir;

			for (int32 SheetIndex = 0; SheetIndex < Source.Sheets; SheetIndex++)
			{
				// Reset the texture coordinate
				fU					= 0.0f;
				LastPosition		= BeamPayloadData->SourcePoint;
				LastDrawPosition	= LastPosition;

				// Determine the current position by stepping the direct line and offsetting with the noise point. 
				CurrPosition		= LastPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;

				if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
				{
					NoiseDir		= NextNoise[0] - NoisePoints[0];
					NoiseDir.Normalize();
					CheckNoisePoint	= NoisePoints[0] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
					if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[0].X) < Source.NoiseLockRadius) &&
						(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[0].Y) < Source.NoiseLockRadius) &&
						(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[0].Z) < Source.NoiseLockRadius))
					{
						NoisePoints[0]	= NextNoise[0];
					}
					else
					{
						NoisePoints[0]	= CheckNoisePoint;
					}
				}

				CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[0] * NoiseDistScale);

				// Determine the offset for the leading edge
				Location	= LastDrawPosition;
				EndPoint	= CurrDrawPosition;
				Right		= Location - EndPoint;
				Right.Normalize();
				if (((Source.UpVectorStepSize == 1) && (i == 0)) || (Source.UpVectorStepSize == 0))
				{
					//LastUp = Right ^ ViewDirection;
					LastUp = Right ^ (Location - ViewOrigin);
					if (!LastUp.Normalize())
					{
						LastUp = CameraToWorld.GetScaledAxis( EAxis::Y );
					}
					THE_Up = LastUp;
				}
				else
				{
					LastUp = THE_Up;
				}

				if (SheetIndex)
				{
					Angle			= ((float)PI / (float)Source.Sheets) * SheetIndex;
					QuatRotator		= FQuat(Right, Angle);
					WorkingLastUp	= QuatRotator.RotateVector(LastUp);
				}
				else
				{
					WorkingLastUp	= LastUp;
				}

				float	Taper	= 1.0f;

				if (Source.TaperMethod != PEBTM_None)
				{
					check(TaperValues);
					Taper	= TaperValues[0];
				}

				// Size is locked to X, see initialization of Size.
				LastOffset.X	= WorkingLastUp.X * Size.X * Taper;
				LastOffset.Y	= WorkingLastUp.Y * Size.X * Taper;
				LastOffset.Z	= WorkingLastUp.Z * Size.X * Taper;

				// 'Lead' edge
				Vertex->Position	= Location + LastOffset;
				Vertex->OldPosition	= Location;
				Vertex->ParticleId	= 0;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fU;
				Vertex->Tex_V		= 0.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				CheckVertexCount++;

				Vertex->Position	= Location - LastOffset;
				Vertex->OldPosition	= Location;
				Vertex->ParticleId	= 0;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fU;
				Vertex->Tex_V		= 1.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				CheckVertexCount++;

				fU	+= TextureIncrement;

				for (int32 StepIndex = 0; StepIndex < BeamPayloadData->Steps; StepIndex++)
				{
					// Determine the current position by stepping the direct line and offsetting with the noise point. 
					CurrPosition		= LastPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;

					if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
					{
						NoiseDir		= NextNoise[StepIndex] - NoisePoints[StepIndex];
						NoiseDir.Normalize();
						CheckNoisePoint	= NoisePoints[StepIndex] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
						if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[StepIndex].X) < Source.NoiseLockRadius) &&
							(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[StepIndex].Y) < Source.NoiseLockRadius) &&
							(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[StepIndex].Z) < Source.NoiseLockRadius))
						{
							NoisePoints[StepIndex]	= NextNoise[StepIndex];
						}
						else
						{
							NoisePoints[StepIndex]	= CheckNoisePoint;
						}
					}

					CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[StepIndex] * NoiseDistScale);

					// Prep the next draw position to determine tangents
					bool bTarget = false;
					NextTargetPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
					if (bLocked && ((StepIndex + 1) == BeamPayloadData->Steps))
					{
						// If we are locked, and the next step is the target point, set the draw position as such.
						// (ie, we are on the last noise point...)
						NextTargetDrawPosition	= BeamPayloadData->TargetPoint;
						if (Source.bTargetNoise)
						{
							if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
							{
								NoiseDir		= NextNoise[Source.Frequency] - NoisePoints[Source.Frequency];
								NoiseDir.Normalize();
								CheckNoisePoint	= NoisePoints[Source.Frequency] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
								if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[Source.Frequency].X) < Source.NoiseLockRadius) &&
									(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[Source.Frequency].Y) < Source.NoiseLockRadius) &&
									(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[Source.Frequency].Z) < Source.NoiseLockRadius))
								{
									NoisePoints[Source.Frequency]	= NextNoise[Source.Frequency];
								}
								else
								{
									NoisePoints[Source.Frequency]	= CheckNoisePoint;
								}
							}

							NextTargetDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[Source.Frequency] * NoiseDistScale);
						}
						TargetTangent = BeamPayloadData->TargetTangent;
						fTargetStrength	= BeamPayloadData->TargetStrength;
					}
					else
					{
						// Just another noise point... offset the target to get the draw position.
						if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
						{
							NoiseDir		= NextNoise[StepIndex + 1] - NoisePoints[StepIndex + 1];
							NoiseDir.Normalize();
							CheckNoisePoint	= NoisePoints[StepIndex + 1] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
							if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[StepIndex + 1].X) < Source.NoiseLockRadius) &&
								(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[StepIndex + 1].Y) < Source.NoiseLockRadius) &&
								(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[StepIndex + 1].Z) < Source.NoiseLockRadius))
							{
								NoisePoints[StepIndex + 1]	= NextNoise[StepIndex + 1];
							}
							else
							{
								NoisePoints[StepIndex + 1]	= CheckNoisePoint;
							}
						}

						NextTargetDrawPosition	= NextTargetPosition + NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[StepIndex + 1] * NoiseDistScale);

						TargetTangent = ((1.0f - Source.NoiseTension) / 2.0f) * (NextTargetDrawPosition - LastDrawPosition);
					}
					TargetTangent.Normalize();
					TargetTangent *= fTargetStrength;

					InterimDrawPosition = LastDrawPosition;
					// Tessellate between the current position and the last position
					for (int32 TessIndex = 0; TessIndex < TessFactor; TessIndex++)
					{
						InterpDrawPos = FMath::CubicInterp(
							LastDrawPosition, LastTangent,
							CurrDrawPosition, TargetTangent,
							InvTessFactor * (TessIndex + 1));

						Location	= InterimDrawPosition;
						EndPoint	= InterpDrawPos;
						Right		= Location - EndPoint;
						Right.Normalize();
						if (Source.UpVectorStepSize == 0)
						{
							//Up = Right ^  (Location - CameraToWorld.GetOrigin());
							Up = Right ^ (Location - ViewOrigin);
							if (!Up.Normalize())
							{
								Up = CameraToWorld.GetScaledAxis( EAxis::Y );
							}
						}
						else
						{
							Up = THE_Up;
						}

						if (SheetIndex)
						{
							Angle		= ((float)PI / (float)Source.Sheets) * SheetIndex;
							QuatRotator	= FQuat(Right, Angle);
							WorkingUp	= QuatRotator.RotateVector(Up);
						}
						else
						{
							WorkingUp	= Up;
						}

						if (Source.TaperMethod != PEBTM_None)
						{
							check(TaperValues);
							Taper	= TaperValues[StepIndex * TessFactor + TessIndex];
						}

						// Size is locked to X, see initialization of Size.
						Offset.X	= WorkingUp.X * Size.X * Taper;
						Offset.Y	= WorkingUp.Y * Size.X * Taper;
						Offset.Z	= WorkingUp.Z * Size.X * Taper;

						// Generate the vertex
						Vertex->Position	= InterpDrawPos + Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->ParticleId	= 0;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 0.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						CheckVertexCount++;

						Vertex->Position	= InterpDrawPos - Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->ParticleId	= 0;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 1.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						CheckVertexCount++;

						fU	+= TextureIncrement;
						InterimDrawPosition	= InterpDrawPos;
					}
					LastPosition		= CurrPosition;
					LastDrawPosition	= CurrDrawPosition;
					LastTangent			= TargetTangent;
				}

				if (bLocked)
				{
					// Draw the line from the last point to the target
					CurrDrawPosition	= BeamPayloadData->TargetPoint;
					if (Source.bTargetNoise)
					{
						if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
						{
							NoiseDir		= NextNoise[Source.Frequency] - NoisePoints[Source.Frequency];
							NoiseDir.Normalize();
							CheckNoisePoint	= NoisePoints[Source.Frequency] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
							if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[Source.Frequency].X) < Source.NoiseLockRadius) &&
								(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[Source.Frequency].Y) < Source.NoiseLockRadius) &&
								(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[Source.Frequency].Z) < Source.NoiseLockRadius))
							{
								NoisePoints[Source.Frequency]	= NextNoise[Source.Frequency];
							}
							else
							{
								NoisePoints[Source.Frequency]	= CheckNoisePoint;
							}
						}

						CurrDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[Source.Frequency] * NoiseDistScale);
					}

					if (Source.bUseTarget)
					{
						TargetTangent = BeamPayloadData->TargetTangent;
					}
					else
					{
						NextTargetDrawPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
						TargetTangent = ((1.0f - Source.NoiseTension) / 2.0f) * 
							(NextTargetDrawPosition - LastDrawPosition);
					}
					TargetTangent.Normalize();
					TargetTangent *= fTargetStrength;

					// Tessellate this segment
					InterimDrawPosition = LastDrawPosition;
					for (int32 TessIndex = 0; TessIndex < TessFactor; TessIndex++)
					{
						InterpDrawPos = FMath::CubicInterp(
							LastDrawPosition, LastTangent,
							CurrDrawPosition, TargetTangent,
							InvTessFactor * (TessIndex + 1));

						Location	= InterimDrawPosition;
						EndPoint	= InterpDrawPos;
						Right		= Location - EndPoint;
						Right.Normalize();
						if (Source.UpVectorStepSize == 0)
						{
							//Up = Right ^  (Location - CameraToWorld.GetOrigin());
							Up = Right ^ (Location - ViewOrigin);
							if (!Up.Normalize())
							{
								Up = CameraToWorld.GetScaledAxis( EAxis::Y );
							}
						}
						else
						{
							Up = THE_Up;
						}

						if (SheetIndex)
						{
							Angle		= ((float)PI / (float)Source.Sheets) * SheetIndex;
							QuatRotator	= FQuat(Right, Angle);
							WorkingUp	= QuatRotator.RotateVector(Up);
						}
						else
						{
							WorkingUp	= Up;
						}

						if (Source.TaperMethod != PEBTM_None)
						{
							check(TaperValues);
							Taper	= TaperValues[BeamPayloadData->Steps * TessFactor + TessIndex];
						}

						// Size is locked to X, see initialization of Size.
						Offset.X	= WorkingUp.X * Size.X * Taper;
						Offset.Y	= WorkingUp.Y * Size.X * Taper;
						Offset.Z	= WorkingUp.Z * Size.X * Taper;

						// Generate the vertex
						Vertex->Position	= InterpDrawPos + Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->ParticleId	= 0;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 0.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						CheckVertexCount++;

						Vertex->Position	= InterpDrawPos - Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->ParticleId	= 0;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 1.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						CheckVertexCount++;

						fU	+= TextureIncrement;
						InterimDrawPosition	= InterpDrawPos;
					}
				}
			}
		}
		else
		{
			// Setup the current position as the source point
			CurrPosition		= BeamPayloadData->SourcePoint;
			CurrDrawPosition	= CurrPosition;

			// Setup the source tangent & strength
			if (Source.bUseSource)
			{
				// The source module will have determined the proper source tangent.
				LastTangent	= BeamPayloadData->SourceTangent;
				fStrength	= BeamPayloadData->SourceStrength;
			}
			else
			{
				// We don't have a source module, so use the orientation of the emitter.
				LastTangent	= WorldToLocal.GetScaledAxis( EAxis::X );
				fStrength	= Source.NoiseTangentStrength;
			}
			LastTangent.Normalize();
			LastTangent *= fStrength;

			// Setup the target tangent strength
			fTargetStrength	= Source.NoiseTangentStrength;

			// Set the last draw position to the source so we don't get 'under-hang'
			LastPosition		= CurrPosition;
			LastDrawPosition	= CurrDrawPosition;

			bool	bLocked	= BEAM2_TYPEDATA_LOCKED(BeamPayloadData->Lock_Max_NumNoisePoints);

			FVector	UseNoisePoint, CheckNoisePoint;
			FVector	NoiseDir;

			for (int32 SheetIndex = 0; SheetIndex < Source.Sheets; SheetIndex++)
			{
				// Reset the texture coordinate
				fU					= 0.0f;
				LastPosition		= BeamPayloadData->SourcePoint;
				LastDrawPosition	= LastPosition;

				// Determine the current position by stepping the direct line and offsetting with the noise point. 
				CurrPosition		= LastPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;

				if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
				{
					NoiseDir		= NextNoise[0] - NoisePoints[0];
					NoiseDir.Normalize();
					CheckNoisePoint	= NoisePoints[0] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
					if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[0].X) < Source.NoiseLockRadius) &&
						(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[0].Y) < Source.NoiseLockRadius) &&
						(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[0].Z) < Source.NoiseLockRadius))
					{
						NoisePoints[0]	= NextNoise[0];
					}
					else
					{
						NoisePoints[0]	= CheckNoisePoint;
					}
				}

				CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[0] * NoiseDistScale);

				// Determine the offset for the leading edge
				Location	= LastDrawPosition;
				EndPoint	= CurrDrawPosition;
				Right		= Location - EndPoint;
				Right.Normalize();
				if (((Source.UpVectorStepSize == 1) && (i == 0)) || (Source.UpVectorStepSize == 0))
				{
					//LastUp = Right ^ ViewDirection;
					LastUp = Right ^ (Location - ViewOrigin);
					if (!LastUp.Normalize())
					{
						LastUp = CameraToWorld.GetScaledAxis( EAxis::Y );
					}
					THE_Up = LastUp;
				}
				else
				{
					LastUp = THE_Up;
				}

				if (SheetIndex)
				{
					Angle			= ((float)PI / (float)Source.Sheets) * SheetIndex;
					QuatRotator		= FQuat(Right, Angle);
					WorkingLastUp	= QuatRotator.RotateVector(LastUp);
				}
				else
				{
					WorkingLastUp	= LastUp;
				}

				float	Taper	= 1.0f;

				if (Source.TaperMethod != PEBTM_None)
				{
					check(TaperValues);
					Taper	= TaperValues[0];
				}

				// Size is locked to X, see initialization of Size.
				LastOffset.X	= WorkingLastUp.X * Size.X * Taper;
				LastOffset.Y	= WorkingLastUp.Y * Size.X * Taper;
				LastOffset.Z	= WorkingLastUp.Z * Size.X * Taper;

				// 'Lead' edge
				Vertex->Position	= Location + LastOffset;
				Vertex->OldPosition	= Location;
				Vertex->ParticleId	= 0;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fU;
				Vertex->Tex_V		= 0.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				CheckVertexCount++;

				Vertex->Position	= Location - LastOffset;
				Vertex->OldPosition	= Location;
				Vertex->ParticleId	= 0;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fU;
				Vertex->Tex_V		= 1.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				CheckVertexCount++;

				fU	+= TextureIncrement;

				for (int32 StepIndex = 0; StepIndex < BeamPayloadData->Steps; StepIndex++)
				{
					// Determine the current position by stepping the direct line and offsetting with the noise point. 
					CurrPosition		= LastPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;

					if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
					{
						NoiseDir		= NextNoise[StepIndex] - NoisePoints[StepIndex];
						NoiseDir.Normalize();
						CheckNoisePoint	= NoisePoints[StepIndex] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
						if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[StepIndex].X) < Source.NoiseLockRadius) &&
							(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[StepIndex].Y) < Source.NoiseLockRadius) &&
							(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[StepIndex].Z) < Source.NoiseLockRadius))
						{
							NoisePoints[StepIndex]	= NextNoise[StepIndex];
						}
						else
						{
							NoisePoints[StepIndex]	= CheckNoisePoint;
						}
					}

					CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[StepIndex] * NoiseDistScale);

					// Prep the next draw position to determine tangents
					bool bTarget = false;
					NextTargetPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
					if (bLocked && ((StepIndex + 1) == BeamPayloadData->Steps))
					{
						// If we are locked, and the next step is the target point, set the draw position as such.
						// (ie, we are on the last noise point...)
						NextTargetDrawPosition	= BeamPayloadData->TargetPoint;
						if (Source.bTargetNoise)
						{
							if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
							{
								NoiseDir		= NextNoise[Source.Frequency] - NoisePoints[Source.Frequency];
								NoiseDir.Normalize();
								CheckNoisePoint	= NoisePoints[Source.Frequency] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
								if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[Source.Frequency].X) < Source.NoiseLockRadius) &&
									(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[Source.Frequency].Y) < Source.NoiseLockRadius) &&
									(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[Source.Frequency].Z) < Source.NoiseLockRadius))
								{
									NoisePoints[Source.Frequency]	= NextNoise[Source.Frequency];
								}
								else
								{
									NoisePoints[Source.Frequency]	= CheckNoisePoint;
								}
							}

							NextTargetDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[Source.Frequency] * NoiseDistScale);
						}
						TargetTangent = BeamPayloadData->TargetTangent;
						fTargetStrength	= BeamPayloadData->TargetStrength;
					}
					else
					{
						// Just another noise point... offset the target to get the draw position.
						if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
						{
							NoiseDir		= NextNoise[StepIndex + 1] - NoisePoints[StepIndex + 1];
							NoiseDir.Normalize();
							CheckNoisePoint	= NoisePoints[StepIndex + 1] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
							if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[StepIndex + 1].X) < Source.NoiseLockRadius) &&
								(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[StepIndex + 1].Y) < Source.NoiseLockRadius) &&
								(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[StepIndex + 1].Z) < Source.NoiseLockRadius))
							{
								NoisePoints[StepIndex + 1]	= NextNoise[StepIndex + 1];
							}
							else
							{
								NoisePoints[StepIndex + 1]	= CheckNoisePoint;
							}
						}

						NextTargetDrawPosition	= NextTargetPosition + NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[StepIndex + 1] * NoiseDistScale);

						TargetTangent = ((1.0f - Source.NoiseTension) / 2.0f) * (NextTargetDrawPosition - LastDrawPosition);
					}
					TargetTangent.Normalize();
					TargetTangent *= fTargetStrength;

					InterimDrawPosition = LastDrawPosition;
					// Tessellate between the current position and the last position
					for (int32 TessIndex = 0; TessIndex < TessFactor; TessIndex++)
					{
						InterpDrawPos = FMath::CubicInterp(
							LastDrawPosition, LastTangent,
							CurrDrawPosition, TargetTangent,
							InvTessFactor * (TessIndex + 1));

						FPlatformMisc::Prefetch(Vertex+2);

						Location	= InterimDrawPosition;
						EndPoint	= InterpDrawPos;
						Right		= Location - EndPoint;
						Right.Normalize();
						if (Source.UpVectorStepSize == 0)
						{
							//Up = Right ^  (Location - CameraToWorld.GetOrigin());
							Up = Right ^ (Location - ViewOrigin);
							if (!Up.Normalize())
							{
								Up = CameraToWorld.GetScaledAxis( EAxis::Y );
							}
						}
						else
						{
							Up = THE_Up;
						}

						if (SheetIndex)
						{
							Angle		= ((float)PI / (float)Source.Sheets) * SheetIndex;
							QuatRotator	= FQuat(Right, Angle);
							WorkingUp	= QuatRotator.RotateVector(Up);
						}
						else
						{
							WorkingUp	= Up;
						}

						if (Source.TaperMethod != PEBTM_None)
						{
							check(TaperValues);
							Taper	= TaperValues[StepIndex * TessFactor + TessIndex];
						}

						// Size is locked to X, see initialization of Size.
						Offset.X	= WorkingUp.X * Size.X * Taper;
						Offset.Y	= WorkingUp.Y * Size.X * Taper;
						Offset.Z	= WorkingUp.Z * Size.X * Taper;

						// Generate the vertex
						Vertex->Position	= InterpDrawPos + Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->ParticleId	= 0;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 0.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						CheckVertexCount++;

						Vertex->Position	= InterpDrawPos - Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->ParticleId	= 0;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 1.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						CheckVertexCount++;

						fU	+= TextureIncrement;
						InterimDrawPosition	= InterpDrawPos;
					}
					LastPosition		= CurrPosition;
					LastDrawPosition	= CurrDrawPosition;
					LastTangent			= TargetTangent;
				}

				if (bLocked)
				{
					// Draw the line from the last point to the target
					CurrDrawPosition	= BeamPayloadData->TargetPoint;
					if (Source.bTargetNoise)
					{
						if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
						{
							NoiseDir		= NextNoise[Source.Frequency] - NoisePoints[Source.Frequency];
							NoiseDir.Normalize();
							CheckNoisePoint	= NoisePoints[Source.Frequency] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
							if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[Source.Frequency].X) < Source.NoiseLockRadius) &&
								(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[Source.Frequency].Y) < Source.NoiseLockRadius) &&
								(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[Source.Frequency].Z) < Source.NoiseLockRadius))
							{
								NoisePoints[Source.Frequency]	= NextNoise[Source.Frequency];
							}
							else
							{
								NoisePoints[Source.Frequency]	= CheckNoisePoint;
							}
						}

						CurrDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[Source.Frequency] * NoiseDistScale);
					}

					if (Source.bUseTarget)
					{
						TargetTangent = BeamPayloadData->TargetTangent;
					}
					else
					{
						NextTargetDrawPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
						TargetTangent = ((1.0f - Source.NoiseTension) / 2.0f) * 
							(NextTargetDrawPosition - LastDrawPosition);
					}
					TargetTangent.Normalize();
					TargetTangent *= fTargetStrength;

					// Tessellate this segment
					InterimDrawPosition = LastDrawPosition;
					for (int32 TessIndex = 0; TessIndex < TessFactor; TessIndex++)
					{
						InterpDrawPos = FMath::CubicInterp(
							LastDrawPosition, LastTangent,
							CurrDrawPosition, TargetTangent,
							InvTessFactor * (TessIndex + 1));

						Location	= InterimDrawPosition;
						EndPoint	= InterpDrawPos;
						Right		= Location - EndPoint;
						Right.Normalize();
						if (Source.UpVectorStepSize == 0)
						{
							//Up = Right ^  (Location - CameraToWorld.GetOrigin());
							Up = Right ^ (Location - ViewOrigin);
							if (!Up.Normalize())
							{
								Up = CameraToWorld.GetScaledAxis( EAxis::Y );
							}
						}
						else
						{
							Up = THE_Up;
						}

						if (SheetIndex)
						{
							Angle		= ((float)PI / (float)Source.Sheets) * SheetIndex;
							QuatRotator	= FQuat(Right, Angle);
							WorkingUp	= QuatRotator.RotateVector(Up);
						}
						else
						{
							WorkingUp	= Up;
						}

						if (Source.TaperMethod != PEBTM_None)
						{
							check(TaperValues);
							Taper	= TaperValues[BeamPayloadData->Steps * TessFactor + TessIndex];
						}

						// Size is locked to X, see initialization of Size.
						Offset.X	= WorkingUp.X * Size.X * Taper;
						Offset.Y	= WorkingUp.Y * Size.X * Taper;
						Offset.Z	= WorkingUp.Z * Size.X * Taper;

						// Generate the vertex
						Vertex->Position	= InterpDrawPos + Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->ParticleId	= 0;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 0.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						CheckVertexCount++;

						Vertex->Position	= InterpDrawPos - Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->ParticleId	= 0;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 1.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						CheckVertexCount++;

						fU	+= TextureIncrement;
						InterimDrawPosition	= InterpDrawPos;
					}
				}
				else
				if (BeamPayloadData->TravelRatio > KINDA_SMALL_NUMBER)
				{
					//@todo.SAS. Re-implement partial-segment beams
				}
			}
		}
	}

	check(CheckVertexCount <= Source.VertexCount);

	return TrianglesToRender;
}

int32 FDynamicBeam2EmitterData::FillData_InterpolatedNoise(FAsyncBufferFillData& Me)
{
	int32	TrianglesToRender = 0;

	check(Source.InterpolationPoints > 0);
	check(Source.Frequency > 0);

	FParticleBeamTrailVertex* Vertex = (FParticleBeamTrailVertex*)Me.VertexData;
	FMatrix CameraToWorld = Me.View->ViewMatrices.ViewMatrix.Inverse();
	
	if (Source.Sheets <= 0)
	{
		Source.Sheets = 1;
	}

	FVector	ViewOrigin	= CameraToWorld.GetOrigin();

	// Frequency is the number of noise points to generate, evenly distributed along the line.
	if (Source.Frequency <= 0)
	{
		Source.Frequency = 1;
	}

	// NoiseTessellation is the amount of tessellation that should occur between noise points.
	int32	TessFactor	= Source.NoiseTessellation ? Source.NoiseTessellation : 1;
	
	float	InvTessFactor	= 1.0f / TessFactor;
	int32		i;

	// The last position processed
	FVector	LastPosition, LastDrawPosition, LastTangent;
	// The current position
	FVector	CurrPosition, CurrDrawPosition;
	// The target
	FVector	TargetPosition, TargetDrawPosition;
	// The next target
	FVector	NextTargetPosition, NextTargetDrawPosition, TargetTangent;
	// The interperted draw position
	FVector InterpDrawPos;
	FVector	InterimDrawPosition;

	float	Angle;
	FQuat	QuatRotator;

	FVector Location;
	FVector EndPoint;
	FVector Right;
	FVector Up;
	FVector WorkingUp;
	FVector LastUp;
	FVector WorkingLastUp;
	FVector Offset;
	FVector LastOffset;
	float	fStrength;
	float	fTargetStrength;

	float	fU;
	float	TextureIncrement	= 1.0f / (((Source.Frequency > 0) ? Source.Frequency : 1) * TessFactor);	// TTP #33140/33159

	FVector THE_Up(0.0f);

	int32	 CheckVertexCount	= 0;

	FMatrix WorldToLocal = Me.WorldToLocal;
	FMatrix LocalToWorld = Me.LocalToWorld;

	// Tessellate the beam along the noise points
	for (i = 0; i < Source.ActiveParticleCount; i++)
	{
		DECLARE_PARTICLE_PTR(Particle, Source.ParticleData.GetData() + Source.ParticleStride * i);

		// Retrieve the beam data from the particle.
		FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
		FVector*				InterpolatedPoints	= NULL;
		float*					NoiseRate			= NULL;
		float*					NoiseDelta			= NULL;
		FVector*				TargetNoisePoints	= NULL;
		FVector*				NextNoisePoints		= NULL;
		float*					TaperValues			= NULL;
		float*					NoiseDistanceScale	= NULL;

		BeamPayloadData = (FBeam2TypeDataPayload*)((uint8*)Particle + Source.BeamDataOffset);
		if (BeamPayloadData->TriangleCount == 0)
		{
			continue;
		}
		if (BeamPayloadData->Steps == 0)
		{
			continue;
		}

		if (Source.InterpolatedPointsOffset != -1)
		{
			InterpolatedPoints = (FVector*)((uint8*)Particle + Source.InterpolatedPointsOffset);
		}
		if (Source.NoiseRateOffset != -1)
		{
			NoiseRate = (float*)((uint8*)Particle + Source.NoiseRateOffset);
		}
		if (Source.NoiseDeltaTimeOffset != -1)
		{
			NoiseDelta = (float*)((uint8*)Particle + Source.NoiseDeltaTimeOffset);
		}
		if (Source.TargetNoisePointsOffset != -1)
		{
			TargetNoisePoints = (FVector*)((uint8*)Particle + Source.TargetNoisePointsOffset);
		}
		if (Source.NextNoisePointsOffset != -1)
		{
			NextNoisePoints = (FVector*)((uint8*)Particle + Source.NextNoisePointsOffset);
		}
		if (Source.TaperValuesOffset != -1)
		{
			TaperValues = (float*)((uint8*)Particle + Source.TaperValuesOffset);
		}
		if (Source.NoiseDistanceScaleOffset != -1)
		{
			NoiseDistanceScale = (float*)((uint8*)Particle + Source.NoiseDistanceScaleOffset);
		}

		float NoiseDistScale = 1.0f;
		if (NoiseDistanceScale)
		{
			NoiseDistScale = *NoiseDistanceScale;
		}

		int32 Freq = BEAM2_TYPEDATA_FREQUENCY(BeamPayloadData->Lock_Max_NumNoisePoints);
		float InterpStepSize = (float)(BeamPayloadData->InterpolationSteps) / (float)(BeamPayloadData->Steps);
		float InterpFraction = FMath::Fractional(InterpStepSize);
		//bool bInterpFractionIsZero = (FMath::Abs(InterpFraction) < KINDA_SMALL_NUMBER) ? true : false;
		bool bInterpFractionIsZero = false;
		int32 InterpIndex = FMath::Trunc(InterpStepSize);

		FVector* NoisePoints	= TargetNoisePoints;
		FVector* NextNoise		= NextNoisePoints;

		// Appropriate checks are made before access access, no need to assert here
		CA_ASSUME(NoisePoints != NULL);
		CA_ASSUME(NextNoise != NULL);
		CA_ASSUME(InterpolatedPoints != NULL);

		float NoiseRangeScaleFactor = Source.NoiseRangeScale;
		//@todo. How to handle no noise points?
		// If there are no noise points, why are we in here?
		if (NoisePoints == NULL)
		{
			continue;
		}

		// Pin the size to the X component
		FVector2D Size(Particle->Size.X * Source.Scale.X, Particle->Size.X * Source.Scale.X);

		// Setup the current position as the source point
		CurrPosition		= BeamPayloadData->SourcePoint;
		CurrDrawPosition	= CurrPosition;

		// Setup the source tangent & strength
		if (Source.bUseSource)
		{
			// The source module will have determined the proper source tangent.
			LastTangent	= BeamPayloadData->SourceTangent;
			fStrength	= Source.NoiseTangentStrength;
		}
		else
		{
			// We don't have a source module, so use the orientation of the emitter.
			LastTangent	= WorldToLocal.GetScaledAxis( EAxis::X );
			fStrength	= Source.NoiseTangentStrength;
		}
		LastTangent *= fStrength;

		// Setup the target tangent strength
		fTargetStrength	= Source.NoiseTangentStrength;

		// Set the last draw position to the source so we don't get 'under-hang'
		LastPosition		= CurrPosition;
		LastDrawPosition	= CurrDrawPosition;

		bool	bLocked	= BEAM2_TYPEDATA_LOCKED(BeamPayloadData->Lock_Max_NumNoisePoints);

		FVector	UseNoisePoint, CheckNoisePoint;
		FVector	NoiseDir;

		for (int32 SheetIndex = 0; SheetIndex < Source.Sheets; SheetIndex++)
		{
			// Reset the texture coordinate
			fU					= 0.0f;
			LastPosition		= BeamPayloadData->SourcePoint;
			LastDrawPosition	= LastPosition;

			// Determine the current position by finding it along the interpolated path and 
			// offsetting with the noise point. 
			if (bInterpFractionIsZero)
			{
				CurrPosition = InterpolatedPoints[InterpIndex];
			}
			else
			{
				CurrPosition = 
					(InterpolatedPoints[InterpIndex + 0] * InterpFraction) + 
					(InterpolatedPoints[InterpIndex + 1] * (1.0f - InterpFraction));
			}

			if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
			{
				NoiseDir		= NextNoise[0] - NoisePoints[0];
				NoiseDir.Normalize();
				CheckNoisePoint	= NoisePoints[0] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
				if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[0].X) < Source.NoiseLockRadius) &&
					(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[0].Y) < Source.NoiseLockRadius) &&
					(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[0].Z) < Source.NoiseLockRadius))
				{
					NoisePoints[0]	= NextNoise[0];
				}
				else
				{
					NoisePoints[0]	= CheckNoisePoint;
				}
			}

			CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[0] * NoiseDistScale);

			// Determine the offset for the leading edge
			Location	= LastDrawPosition;
			EndPoint	= CurrDrawPosition;
			Right		= Location - EndPoint;
			Right.Normalize();
			if (((Source.UpVectorStepSize == 1) && (i == 0)) || (Source.UpVectorStepSize == 0))
			{
				//LastUp = Right ^ ViewDirection;
				LastUp = Right ^ (Location - ViewOrigin);
				if (!LastUp.Normalize())
				{
					LastUp = CameraToWorld.GetScaledAxis( EAxis::Y );
				}
				THE_Up = LastUp;
			}
			else
			{
				LastUp = THE_Up;
			}

			if (SheetIndex)
			{
				Angle			= ((float)PI / (float)Source.Sheets) * SheetIndex;
				QuatRotator		= FQuat(Right, Angle);
				WorkingLastUp	= QuatRotator.RotateVector(LastUp);
			}
			else
			{
				WorkingLastUp	= LastUp;
			}

			float	Taper	= 1.0f;

			if (Source.TaperMethod != PEBTM_None)
			{
				check(TaperValues);
				Taper	= TaperValues[0];
			}

			// Size is locked to X, see initialization of Size.
			LastOffset.X	= WorkingLastUp.X * Size.X * Taper;
			LastOffset.Y	= WorkingLastUp.Y * Size.X * Taper;
			LastOffset.Z	= WorkingLastUp.Z * Size.X * Taper;

			// 'Lead' edge
			Vertex->Position	= Location + LastOffset;
			Vertex->OldPosition	= Location;
			Vertex->ParticleId	= 0;
			Vertex->Size		= Size;
			Vertex->Tex_U		= fU;
			Vertex->Tex_V		= 0.0f;
			Vertex->Rotation	= Particle->Rotation;
			Vertex->Color		= Particle->Color;
			Vertex++;
			CheckVertexCount++;

			Vertex->Position	= Location - LastOffset;
			Vertex->OldPosition	= Location;
			Vertex->ParticleId	= 0;
			Vertex->Size		= Size;
			Vertex->Tex_U		= fU;
			Vertex->Tex_V		= 1.0f;
			Vertex->Rotation	= Particle->Rotation;
			Vertex->Color		= Particle->Color;
			Vertex++;
			CheckVertexCount++;

			fU	+= TextureIncrement;

			check(InterpolatedPoints);
			for (int32 StepIndex = 0; StepIndex < BeamPayloadData->Steps; StepIndex++)
			{
				// Determine the current position by finding it along the interpolated path and 
				// offsetting with the noise point. 
				if (bInterpFractionIsZero)
				{
					CurrPosition = InterpolatedPoints[StepIndex  * InterpIndex];
				}
				else
				{
					if (StepIndex == (BeamPayloadData->Steps - 1))
					{
						CurrPosition = 
							(InterpolatedPoints[StepIndex * InterpIndex] * (1.0f - InterpFraction)) + 
							(BeamPayloadData->TargetPoint * InterpFraction);
					}
					else
					{
						CurrPosition = 
							(InterpolatedPoints[StepIndex * InterpIndex + 0] * (1.0f - InterpFraction)) + 
							(InterpolatedPoints[StepIndex * InterpIndex + 1] * InterpFraction);
					}
				}


				if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
				{
					NoiseDir		= NextNoise[StepIndex] - NoisePoints[StepIndex];
					NoiseDir.Normalize();
					CheckNoisePoint	= NoisePoints[StepIndex] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
					if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[StepIndex].X) < Source.NoiseLockRadius) &&
						(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[StepIndex].Y) < Source.NoiseLockRadius) &&
						(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[StepIndex].Z) < Source.NoiseLockRadius))
					{
						NoisePoints[StepIndex]	= NextNoise[StepIndex];
					}
					else
					{
						NoisePoints[StepIndex]	= CheckNoisePoint;
					}
				}

				CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[StepIndex] * NoiseDistScale);

				// Prep the next draw position to determine tangents
				bool bTarget = false;
				NextTargetPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
				// Determine the current position by finding it along the interpolated path and 
				// offsetting with the noise point. 
				if (bInterpFractionIsZero)
				{
					if (StepIndex == (BeamPayloadData->Steps - 2))
					{
						NextTargetPosition = BeamPayloadData->TargetPoint;
					}
					else
					{
						NextTargetPosition = InterpolatedPoints[(StepIndex + 2) * InterpIndex + 0];
					}
				}
				else
				{
					if (StepIndex == (BeamPayloadData->Steps - 1))
					{
						NextTargetPosition = 
							(InterpolatedPoints[(StepIndex + 1) * InterpIndex + 0] * InterpFraction) + 
							(BeamPayloadData->TargetPoint * (1.0f - InterpFraction));
					}
					else
					{
						NextTargetPosition = 
							(InterpolatedPoints[(StepIndex + 1) * InterpIndex + 0] * InterpFraction) + 
							(InterpolatedPoints[(StepIndex + 1) * InterpIndex + 1] * (1.0f - InterpFraction));
					}
				}
				if (bLocked && ((StepIndex + 1) == BeamPayloadData->Steps))
				{
					// If we are locked, and the next step is the target point, set the draw position as such.
					// (ie, we are on the last noise point...)
					NextTargetDrawPosition	= BeamPayloadData->TargetPoint;
					if (Source.bTargetNoise)
					{
						if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
						{
							NoiseDir		= NextNoise[Source.Frequency] - NoisePoints[Source.Frequency];
							NoiseDir.Normalize();
							CheckNoisePoint	= NoisePoints[Source.Frequency] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
							if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[Source.Frequency].X) < Source.NoiseLockRadius) &&
								(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[Source.Frequency].Y) < Source.NoiseLockRadius) &&
								(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[Source.Frequency].Z) < Source.NoiseLockRadius))
							{
								NoisePoints[Source.Frequency]	= NextNoise[Source.Frequency];
							}
							else
							{
								NoisePoints[Source.Frequency]	= CheckNoisePoint;
							}
						}

						NextTargetDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[Source.Frequency] * NoiseDistScale);
					}
					TargetTangent = BeamPayloadData->TargetTangent;
					fTargetStrength	= Source.NoiseTangentStrength;
				}
				else
				{
					// Just another noise point... offset the target to get the draw position.
					if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
					{
						NoiseDir		= NextNoise[StepIndex + 1] - NoisePoints[StepIndex + 1];
						NoiseDir.Normalize();
						CheckNoisePoint	= NoisePoints[StepIndex + 1] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
						if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[StepIndex + 1].X) < Source.NoiseLockRadius) &&
							(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[StepIndex + 1].Y) < Source.NoiseLockRadius) &&
							(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[StepIndex + 1].Z) < Source.NoiseLockRadius))
						{
							NoisePoints[StepIndex + 1]	= NextNoise[StepIndex + 1];
						}
						else
						{
							NoisePoints[StepIndex + 1]	= CheckNoisePoint;
						}
					}

					NextTargetDrawPosition	= NextTargetPosition + NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[StepIndex + 1] * NoiseDistScale);

					TargetTangent = ((1.0f - Source.NoiseTension) / 2.0f) * (NextTargetDrawPosition - LastDrawPosition);
				}
				TargetTangent = ((1.0f - Source.NoiseTension) / 2.0f) * (NextTargetDrawPosition - LastDrawPosition);
				TargetTangent.Normalize();
				TargetTangent *= fTargetStrength;

				InterimDrawPosition = LastDrawPosition;
				// Tessellate between the current position and the last position
				for (int32 TessIndex = 0; TessIndex < TessFactor; TessIndex++)
				{
					InterpDrawPos = FMath::CubicInterp(
						LastDrawPosition, LastTangent,
						CurrDrawPosition, TargetTangent,
						InvTessFactor * (TessIndex + 1));

					Location	= InterimDrawPosition;
					EndPoint	= InterpDrawPos;
					Right		= Location - EndPoint;
					Right.Normalize();
					if (Source.UpVectorStepSize == 0)
					{
						//Up = Right ^  (Location - CameraToWorld.GetOrigin());
						Up = Right ^ (Location - ViewOrigin);
						if (!Up.Normalize())
						{
							Up = CameraToWorld.GetScaledAxis( EAxis::Y );
						}
					}
					else
					{
						Up = THE_Up;
					}

					if (SheetIndex)
					{
						Angle		= ((float)PI / (float)Source.Sheets) * SheetIndex;
						QuatRotator	= FQuat(Right, Angle);
						WorkingUp	= QuatRotator.RotateVector(Up);
					}
					else
					{
						WorkingUp	= Up;
					}

					if (Source.TaperMethod != PEBTM_None)
					{
						check(TaperValues);
						Taper	= TaperValues[StepIndex * TessFactor + TessIndex];
					}

					// Size is locked to X, see initialization of Size.
					Offset.X	= WorkingUp.X * Size.X * Taper;
					Offset.Y	= WorkingUp.Y * Size.X * Taper;
					Offset.Z	= WorkingUp.Z * Size.X * Taper;

					// Generate the vertex
					Vertex->Position	= InterpDrawPos + Offset;
					Vertex->OldPosition	= InterpDrawPos;
					Vertex->ParticleId	= 0;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU;
					Vertex->Tex_V		= 0.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;
					CheckVertexCount++;

					Vertex->Position	= InterpDrawPos - Offset;
					Vertex->OldPosition	= InterpDrawPos;
					Vertex->ParticleId	= 0;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU;
					Vertex->Tex_V		= 1.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;
					CheckVertexCount++;

					fU	+= TextureIncrement;
					InterimDrawPosition	= InterpDrawPos;
				}
				LastPosition		= CurrPosition;
				LastDrawPosition	= CurrDrawPosition;
				LastTangent			= TargetTangent;
			}

			if (bLocked)
			{
				// Draw the line from the last point to the target
				CurrDrawPosition	= BeamPayloadData->TargetPoint;
				if (Source.bTargetNoise)
				{
					if ((Source.NoiseLockTime >= 0.0f) && Source.bSmoothNoise_Enabled)
					{
						NoiseDir		= NextNoise[Source.Frequency] - NoisePoints[Source.Frequency];
						NoiseDir.Normalize();
						CheckNoisePoint	= NoisePoints[Source.Frequency] + NoiseDir * Source.NoiseSpeed * *NoiseRate;
						if ((FMath::Abs<float>(CheckNoisePoint.X - NextNoise[Source.Frequency].X) < Source.NoiseLockRadius) &&
							(FMath::Abs<float>(CheckNoisePoint.Y - NextNoise[Source.Frequency].Y) < Source.NoiseLockRadius) &&
							(FMath::Abs<float>(CheckNoisePoint.Z - NextNoise[Source.Frequency].Z) < Source.NoiseLockRadius))
						{
							NoisePoints[Source.Frequency]	= NextNoise[Source.Frequency];
						}
						else
						{
							NoisePoints[Source.Frequency]	= CheckNoisePoint;
						}
					}

					CurrDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformVector(NoisePoints[Source.Frequency] * NoiseDistScale);
				}

				NextTargetDrawPosition	= BeamPayloadData->TargetPoint;
				if (Source.bUseTarget)
				{
					TargetTangent = BeamPayloadData->TargetTangent;
				}
				else
				{
					TargetTangent = ((1.0f - Source.NoiseTension) / 2.0f) * 
						(NextTargetDrawPosition - LastDrawPosition);
					TargetTangent.Normalize();
				}
				TargetTangent *= fTargetStrength;

				// Tessellate this segment
				InterimDrawPosition = LastDrawPosition;
				for (int32 TessIndex = 0; TessIndex < TessFactor; TessIndex++)
				{
					InterpDrawPos = FMath::CubicInterp(
						LastDrawPosition, LastTangent,
						CurrDrawPosition, TargetTangent,
						InvTessFactor * (TessIndex + 1));

					Location	= InterimDrawPosition;
					EndPoint	= InterpDrawPos;
					Right		= Location - EndPoint;
					Right.Normalize();
					if (Source.UpVectorStepSize == 0)
					{
						//Up = Right ^  (Location - CameraToWorld.GetOrigin());
						Up = Right ^ (Location - ViewOrigin);
						if (!Up.Normalize())
						{
							Up = CameraToWorld.GetScaledAxis( EAxis::Y );
						}
					}
					else
					{
						Up = THE_Up;
					}

					if (SheetIndex)
					{
						Angle		= ((float)PI / (float)Source.Sheets) * SheetIndex;
						QuatRotator	= FQuat(Right, Angle);
						WorkingUp	= QuatRotator.RotateVector(Up);
					}
					else
					{
						WorkingUp	= Up;
					}

					if (Source.TaperMethod != PEBTM_None)
					{
						check(TaperValues);
						Taper	= TaperValues[BeamPayloadData->Steps * TessFactor + TessIndex];
					}

					// Size is locked to X, see initialization of Size.
					Offset.X	= WorkingUp.X * Size.X * Taper;
					Offset.Y	= WorkingUp.Y * Size.X * Taper;
					Offset.Z	= WorkingUp.Z * Size.X * Taper;

					// Generate the vertex
					Vertex->Position	= InterpDrawPos + Offset;
					Vertex->OldPosition	= InterpDrawPos;
					Vertex->ParticleId	= 0;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU;
					Vertex->Tex_V		= 0.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;
					CheckVertexCount++;

					Vertex->Position	= InterpDrawPos - Offset;
					Vertex->OldPosition	= InterpDrawPos;
					Vertex->ParticleId	= 0;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU;
					Vertex->Tex_V		= 1.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;
					CheckVertexCount++;

					fU	+= TextureIncrement;
					InterimDrawPosition	= InterpDrawPos;
				}
			}
			else
			if (BeamPayloadData->TravelRatio > KINDA_SMALL_NUMBER)
			{
				//@todo.SAS. Re-implement partial-segment beams
			}
		}
	}

	check(CheckVertexCount <= Source.VertexCount);

	return TrianglesToRender;
}

//
//	FDynamicTrailsEmitterData
//
/** Dynamic emitter data for Ribbon emitters */
/** Initialize this emitter's dynamic rendering data, called after source data has been filled in */
void FDynamicTrailsEmitterData::Init(bool bInSelected)
{
	bSelected = bInSelected;

	check(SourcePointer->ActiveParticleCount < (16 * 1024));	// TTP #33330
	check(SourcePointer->ParticleStride < (2 * 1024));			// TTP #33330

	MaterialResource[0] = SourcePointer->MaterialInterface->GetRenderProxy(false);
	MaterialResource[1] = GIsEditor ? SourcePointer->MaterialInterface->GetRenderProxy(true) : MaterialResource[0];
	bUsesDynamicParameter = false;
	// TODO: Should use dynamic parameter if needed!

	// We won't need this on the render thread
	SourcePointer->MaterialInterface = NULL;
}

/**
 *	Create the render thread resources for this emitter data
 *
 *	@param	InOwnerProxy	The proxy that owns this dynamic emitter data
 *
 *	@return	bool			true if successful, false if failed
 */
void FDynamicTrailsEmitterData::CreateRenderThreadResources(const FParticleSystemSceneProxy* InOwnerProxy)
{
	// Create the vertex factory...
	if (VertexFactory == NULL)
	{
		VertexFactory = (FParticleBeamTrailVertexFactory*)(GParticleVertexFactoryPool.GetParticleVertexFactory(PVFT_BeamTrail));
		check(VertexFactory);
	}
}

// Render thread only draw call
int32 FDynamicTrailsEmitterData::Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View)
{
	SCOPE_CYCLE_COUNTER(STAT_TrailRenderingTime);
	INC_DWORD_STAT(STAT_TrailParticlesRenderCalls);

	if (bValid == false)
	{
		return 0;
	}

	check(PDI);
	if ((SourcePointer->VertexCount <= 0) || (SourcePointer->ActiveParticleCount <= 0) || (SourcePointer->IndexCount < 3))
	{
		return 0;
	}

	bool const bIsWireframe = View->Family->EngineShowFlags.Wireframe;

	// Don't render if the material will be ignored
	if (PDI->IsMaterialIgnored(MaterialResource[bSelected]) && !bIsWireframe)
	{
		return 0;
	}

	int32 RenderedPrimitiveCount = 0;

	const FAsyncBufferFillData& Data = EnsureFillCompletion(View);

	if (Data.OutTriangleCount == 0)
	{
		return 0;
	}

	int32 NumDraws = 0;
	if (bRenderGeometry == true)
	{
		FGlobalDynamicVertexBuffer::FAllocation* Allocation = NULL;
		FGlobalDynamicIndexBuffer::FAllocation* IndexAllocation = NULL;
		FGlobalDynamicVertexBuffer::FAllocation* DynamicParameterAllocation = NULL;

		const int32 ViewIndex = View->Family->Views.Find( View );
		if ( InstanceDataAllocations.IsValidIndex( ViewIndex ) )
		{
			Allocation = &InstanceDataAllocations[ ViewIndex ];
		}	
		
		if ( IndexDataAllocations.IsValidIndex( ViewIndex ) )
		{
			IndexAllocation = &IndexDataAllocations[ ViewIndex ];
		}

		if ( DynamicParameterDataAllocations.IsValidIndex( ViewIndex ) )
		{
			DynamicParameterAllocation = &DynamicParameterDataAllocations[ ViewIndex ];
		}

		if ( Allocation && IndexAllocation && Allocation->IsValid() && IndexAllocation->IsValid() && (!bUsesDynamicParameter || (DynamicParameterAllocation && DynamicParameterAllocation->IsValid())) )
		{
			// Create and set the uniform buffer for this emitter.
			FParticleBeamTrailVertexFactory* BeamTrailVertexFactory = (FParticleBeamTrailVertexFactory*)VertexFactory;
			BeamTrailVertexFactory->SetBeamTrailUniformBuffer( CreateBeamTrailUniformBuffer( Proxy, SourcePointer, View ) );
			BeamTrailVertexFactory->SetVertexBuffer( Allocation->VertexBuffer, Allocation->VertexOffset, GetDynamicVertexStride() );
			BeamTrailVertexFactory->SetDynamicParameterBuffer( DynamicParameterAllocation ? DynamicParameterAllocation->VertexBuffer : NULL, DynamicParameterAllocation ? DynamicParameterAllocation->VertexOffset : 0, GetDynamicParameterVertexStride() );

			FMeshBatch Mesh;
			FMeshBatchElement& BatchElement = Mesh.Elements[0];
			BatchElement.IndexBuffer	= IndexAllocation->IndexBuffer;
			BatchElement.FirstIndex		= IndexAllocation->FirstIndex;
			Mesh.VertexFactory			= BeamTrailVertexFactory;
			Mesh.LCI					= NULL;

			BatchElement.PrimitiveUniformBufferResource = &Proxy->GetWorldSpacePrimitiveUniformBuffer();
			BatchElement.NumPrimitives			= Data.OutTriangleCount;
			BatchElement.MinVertexIndex			= 0;
			BatchElement.MaxVertexIndex			= SourcePointer->VertexCount - 1;
			Mesh.UseDynamicData			= false;
			Mesh.ReverseCulling			= Proxy->IsLocalToWorldDeterminantNegative();
			Mesh.CastShadow				= Proxy->GetCastShadow();
			Mesh.DepthPriorityGroup		= (ESceneDepthPriorityGroup)Proxy->GetDepthPriorityGroup(View);

			if (AllowDebugViewmodes() && bIsWireframe && !View->Family->EngineShowFlags.Materials)
			{
				Mesh.MaterialRenderProxy = Proxy->GetDeselectedWireframeMatInst();
			}
			else
			{
	#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (Data.OutTriangleCount != SourcePointer->PrimitiveCount)
				{
					UE_LOG(LogParticles, Log, TEXT("Data.OutTriangleCount = %4d vs. SourcePrimCount = %4d"), Data.OutTriangleCount, SourcePointer->PrimitiveCount);

					int32 CheckTrailCount = 0;
					int32 CheckTriangleCount = 0;
					for (int32 ParticleIdx = 0; ParticleIdx < SourcePointer->ActiveParticleCount; ParticleIdx++)
					{
						int32 CurrentIndex = SourcePointer->ParticleIndices[ParticleIdx];
						DECLARE_PARTICLE_PTR(CheckParticle, SourcePointer->ParticleData.GetData() + SourcePointer->ParticleStride * CurrentIndex);
						FTrailsBaseTypeDataPayload* TrailPayload = (FTrailsBaseTypeDataPayload*)((uint8*)CheckParticle + SourcePointer->TrailDataOffset);
						if (TRAIL_EMITTER_IS_HEAD(TrailPayload->Flags) == false)
						{
							continue;
						}

						UE_LOG(LogParticles, Log, TEXT("Trail %2d has %5d triangles"), TrailPayload->TrailIndex, TrailPayload->TriangleCount);
						CheckTriangleCount += TrailPayload->TriangleCount;
						CheckTrailCount++;
					}
					UE_LOG(LogParticles, Log, TEXT("Total 'live' trail count = %d"), CheckTrailCount);
					UE_LOG(LogParticles, Log, TEXT("\t%5d triangles total (not counting degens)"), CheckTriangleCount);
				}
	#endif
				checkf(Data.OutTriangleCount <= SourcePointer->PrimitiveCount, TEXT("Data.OutTriangleCount = %4d vs. SourcePrimCount = %4d"), Data.OutTriangleCount, SourcePointer->PrimitiveCount);
				Mesh.MaterialRenderProxy = MaterialResource[GIsEditor && (View->Family->EngineShowFlags.Selection) ? bSelected : 0];
			}
			Mesh.Type = PT_TriangleStrip;

			NumDraws += DrawRichMesh(
				PDI,
				Mesh,
				FLinearColor(1.0f, 0.0f, 0.0f),
				FLinearColor(1.0f, 1.0f, 0.0f),
				FLinearColor(1.0f, 1.0f, 1.0f),
				Proxy,
				GIsEditor && (View->Family->EngineShowFlags.Selection) ? Proxy->IsSelected() : false,
				bIsWireframe
				);

			RenderedPrimitiveCount = Mesh.GetNumPrimitives();
		}
	}

	RenderDebug(Proxy, PDI, View, false);

	INC_DWORD_STAT_BY(STAT_TrailParticlesTrianglesRendered, RenderedPrimitiveCount);
	return NumDraws;
}

/**
 *	Called during InitViews for view processing on scene proxies before rendering them
 *  Only called for primitives that are visible and have bDynamicRelevance
 *
 *	@param	Proxy			The 'owner' particle system scene proxy
 *	@param	ViewFamily		The ViewFamily to pre-render for
 *	@param	VisibilityMap	A BitArray that indicates whether the primitive was visible in that view (index)
 *	@param	FrameNumber		The frame number of this pre-render
 */
void FDynamicTrailsEmitterData::PreRenderView(FParticleSystemSceneProxy* Proxy, const FSceneViewFamily* ViewFamily, const uint32 VisibilityMap, int32 FrameNumber)
{
	if (bValid == false)
	{
		return;
	}

	// Only need to do this once per-view
	if (LastFramePreRendered < FrameNumber)
	{
		int32 VertexStride = GetDynamicVertexStride();
		int32 DynamicParameterVertexStride = 0;
		if (bUsesDynamicParameter == true)
		{
			DynamicParameterVertexStride = GetDynamicParameterVertexStride();
		}

		bool bOnlyOneView = ShouldUsePrerenderView() || ((GEngine && GEngine->GameViewport && (GEngine->GameViewport->GetCurrentSplitscreenConfiguration() == eSST_NONE)) ? true : false);

		BuildViewFillDataAndSubmit(Proxy,ViewFamily,VisibilityMap,bOnlyOneView,SourcePointer->VertexCount,VertexStride, DynamicParameterVertexStride);

		// Set the frame tracker
		LastFramePreRendered = FrameNumber;
	}

	if (SourcePointer->bUseLocalSpace == false)
	{
		Proxy->UpdateWorldSpacePrimitiveUniformBuffer();
	}
}

void FDynamicTrailsEmitterData::RenderDebug(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bCrosses)
{
	// Can't do anything in here...
}

void FDynamicTrailsEmitterData::GetIndexAllocInfo( int32& OutNumIndices, int32& OutStride ) const
{
	OutNumIndices = SourcePointer->IndexCount;
	OutStride = SourcePointer->IndexStride;
}

// Data fill functions
int32 FDynamicTrailsEmitterData::FillIndexData(struct FAsyncBufferFillData& Data)
{
	SCOPE_CYCLE_COUNTER(STAT_TrailFillIndexTime);

	int32	TrianglesToRender = 0;

	// Trails polygons are packed and joined as follows:
	//
	// 1--3--5--7--9-...
	// |\ |\ |\ |\ |\...
	// | \| \| \| \| ...
	// 0--2--4--6--8-...
	//
	// (ie, the 'leading' edge of polygon (n) is the trailing edge of polygon (n+1)
	//

	int32	Sheets = 1;
	int32	TessFactor = 1;//FMath::Max<int32>(Source.TessFactor, 1);

	bool bWireframe = (Data.View->Family->EngineShowFlags.Wireframe && !Data.View->Family->EngineShowFlags.Materials);

	int32	CheckCount	= 0;

	uint16*	Index		= (uint16*)Data.IndexData;
	uint16	VertexIndex	= 0;

	int32 CurrentTrail = 0;
	for (int32 ParticleIdx = 0; ParticleIdx < SourcePointer->ActiveParticleCount; ParticleIdx++)
	{
		int32 CurrentIndex = SourcePointer->ParticleIndices[ParticleIdx];
		DECLARE_PARTICLE_PTR(Particle, SourcePointer->ParticleData.GetData() + SourcePointer->ParticleStride * CurrentIndex);

		FTrailsBaseTypeDataPayload* TrailPayload = (FTrailsBaseTypeDataPayload*)((uint8*)Particle + SourcePointer->TrailDataOffset);
		if (TRAIL_EMITTER_IS_HEAD(TrailPayload->Flags) == false)
		{
			continue;
		}

		int32 LocalTrianglesToRender = TrailPayload->TriangleCount;
		if (LocalTrianglesToRender == 0)
		{
			continue;
		}

		//@todo. Support clip source segment

		// For the source particle itself
		if (CurrentTrail == 0)
		{
			*(Index++) = VertexIndex++;		// The first vertex..
			*(Index++) = VertexIndex++;		// The second index..
			CheckCount += 2;
		}
		else
		{
			// Add the verts to join this trail with the previous one
			*(Index++) = VertexIndex - 1;	// Last vertex of the previous sheet
			*(Index++) = VertexIndex;		// First vertex of the next sheet
			*(Index++) = VertexIndex++;		// First vertex of the next sheet
			*(Index++) = VertexIndex++;		// Second vertex of the next sheet
			TrianglesToRender += 4;
			CheckCount += 4;
		}

		for (int32 LocalIdx = 0; LocalIdx < LocalTrianglesToRender; LocalIdx++)
		{
			*(Index++) = VertexIndex++;
			CheckCount++;
			TrianglesToRender++;
		}

		//@todo. Support sheets!

		CurrentTrail++;
	}

	Data.OutTriangleCount = TrianglesToRender;
	return TrianglesToRender;
}

int32 FDynamicTrailsEmitterData::FillVertexData(struct FAsyncBufferFillData& Data)
{
	check(!TEXT("FillVertexData: Base implementation should NOT be called!"));
	return 0;
}

//
//	FDynamicRibbonEmitterData
//
/** Initialize this emitter's dynamic rendering data, called after source data has been filled in */
void FDynamicRibbonEmitterData::Init(bool bInSelected)
{
	SourcePointer = &Source;
	FDynamicTrailsEmitterData::Init(bInSelected);
	
	bUsesDynamicParameter = GetSourceData()->DynamicParameterDataOffset > 0;
}

bool FDynamicRibbonEmitterData::ShouldUsePrerenderView()
{
	return (RenderAxisOption != Trails_CameraUp);
}

void FDynamicRibbonEmitterData::RenderDebug(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bCrosses)
{
	if ((bRenderParticles == true) || (bRenderTangents == true))
	{
		// DEBUGGING
		// Draw all the points of the trail(s)
		FVector DrawPosition;
		float DrawSize;
		FColor DrawColor;
		FColor PrevDrawColor;
		FVector DrawTangentEnd;

		uint8* Address = Source.ParticleData.GetData();
		FRibbonTypeDataPayload* StartTrailPayload;
		FRibbonTypeDataPayload* EndTrailPayload = NULL;
		FBaseParticle* DebugParticle;
		FRibbonTypeDataPayload* TrailPayload;
		FBaseParticle* PrevParticle = NULL;
		FRibbonTypeDataPayload* PrevTrailPayload;
		for (int32 ParticleIdx = 0; ParticleIdx < Source.ActiveParticleCount; ParticleIdx++)
		{
			DECLARE_PARTICLE_PTR(Particle, Address + Source.ParticleStride * Source.ParticleIndices[ParticleIdx]);
			StartTrailPayload = (FRibbonTypeDataPayload*)((uint8*)Particle + Source.TrailDataOffset);
			if (TRAIL_EMITTER_IS_HEAD(StartTrailPayload->Flags) == 0)
			{
				continue;
			}

			// Pin the size to the X component
			float Increment = 1.0f / (StartTrailPayload->TriangleCount / 2);
			float ColorScale = 0.0f;

			DebugParticle = Particle;
			// Find the end particle in this chain...
			TrailPayload = StartTrailPayload;
			FBaseParticle* IteratorParticle = DebugParticle;
			while (TrailPayload)
			{
				int32	Next = TRAIL_EMITTER_GET_NEXT(TrailPayload->Flags);
				if (Next == TRAIL_EMITTER_NULL_NEXT)
				{
					DebugParticle = IteratorParticle;
					EndTrailPayload = TrailPayload;
					TrailPayload = NULL;
				}
				else
				{
					DECLARE_PARTICLE_PTR(TempParticle, Address + Source.ParticleStride * Next);
					IteratorParticle = TempParticle;
					TrailPayload = (FRibbonTypeDataPayload*)((uint8*)IteratorParticle + Source.TrailDataOffset);
				}
			}
			if (EndTrailPayload != StartTrailPayload)
			{
				FBaseParticle* CurrSpawnedParticle = NULL;
				FBaseParticle* NextSpawnedParticle = NULL;
				// We have more than one particle in the trail...
				TrailPayload = EndTrailPayload;

				if (TrailPayload->bInterpolatedSpawn == false)
				{
					CurrSpawnedParticle = DebugParticle;
				}
				while (TrailPayload)
				{
					int32	Prev = TRAIL_EMITTER_GET_PREV(TrailPayload->Flags);
					if (Prev == TRAIL_EMITTER_NULL_PREV)
					{
						PrevParticle = NULL;
						PrevTrailPayload = NULL;
					}
					else
					{
						DECLARE_PARTICLE_PTR(TempParticle, Address + Source.ParticleStride * Prev);
						PrevParticle = TempParticle;
						PrevTrailPayload = (FRibbonTypeDataPayload*)((uint8*)PrevParticle + Source.TrailDataOffset);
					}

					if (PrevTrailPayload && PrevTrailPayload->bInterpolatedSpawn == false)
					{
						if (CurrSpawnedParticle == NULL)
						{
							CurrSpawnedParticle = PrevParticle;
						}
						else
						{
							NextSpawnedParticle = PrevParticle;
						}
					}

					DrawPosition = DebugParticle->Location;
					DrawSize = DebugParticle->Size.X * Source.Scale.X;
					int32 Red   = FMath::Trunc(255.0f * (1.0f - ColorScale));
					int32 Green = FMath::Trunc(255.0f * ColorScale);
					ColorScale += Increment;
					DrawColor = FColor(Red,Green,0);
					Red   = FMath::Trunc(255.0f * (1.0f - ColorScale));
					Green = FMath::Trunc(255.0f * ColorScale);
					PrevDrawColor = FColor(Red,Green,0);

					if (bRenderParticles == true)
					{
						if (TrailPayload->bInterpolatedSpawn == false)
						{
							DrawWireStar(PDI, DrawPosition, DrawSize, FColor(255,0,0), Proxy->GetDepthPriorityGroup(View));
						}
						else
						{
							DrawWireStar(PDI, DrawPosition, DrawSize, FColor(0,255,0), Proxy->GetDepthPriorityGroup(View));
						}

						//
						if (bRenderTessellation == true)
						{
							if (PrevParticle != NULL)
							{
								// Draw a straight line between the particles
								// This will allow us to visualize the tessellation difference
								PDI->DrawLine(DrawPosition, PrevParticle->Location, FColor(0,0,255), Proxy->GetDepthPriorityGroup(View));
								int32 InterpCount = TrailPayload->RenderingInterpCount;
								// Interpolate between current and next...
								FVector LineStart = DrawPosition;
								float Diff = PrevTrailPayload->SpawnTime - TrailPayload->SpawnTime;
								FVector CurrUp = FVector(0.0f, 0.0f, 1.0f);
								float InvCount = 1.0f / InterpCount;
								FLinearColor StartColor = DrawColor;
								FLinearColor EndColor = PrevDrawColor;
								for (int32 SpawnIdx = 0; SpawnIdx < InterpCount; SpawnIdx++)
								{
									float TimeStep = InvCount * SpawnIdx;
									FVector LineEnd = FMath::CubicInterp<FVector>(
										DebugParticle->Location, TrailPayload->Tangent,
										PrevParticle->Location, PrevTrailPayload->Tangent,
										TimeStep);
									FLinearColor InterpColor = FMath::Lerp<FLinearColor>(StartColor, EndColor, TimeStep);
									PDI->DrawLine(LineStart, LineEnd, InterpColor, Proxy->GetDepthPriorityGroup(View));
									if (SpawnIdx > 0)
									{
										InterpColor.R = 1.0f - TimeStep;
										InterpColor.G = 1.0f - TimeStep;
										InterpColor.B = 1.0f - (1.0f - TimeStep);
									}
									DrawWireStar(PDI, LineEnd, DrawSize * 0.3f, InterpColor, Proxy->GetDepthPriorityGroup(View));
									LineStart = LineEnd;
								}
								PDI->DrawLine(LineStart, PrevParticle->Location, EndColor, Proxy->GetDepthPriorityGroup(View));
							}
						}
					}

					if (bRenderTangents == true)
					{
						DrawTangentEnd = DrawPosition + TrailPayload->Tangent;
						if (TrailPayload == StartTrailPayload)
						{
							PDI->DrawLine(DrawPosition, DrawTangentEnd, FLinearColor(0.0f, 1.0f, 0.0f), Proxy->GetDepthPriorityGroup(View));
						}
 						else if (TrailPayload == EndTrailPayload)
 						{
 							PDI->DrawLine(DrawPosition, DrawTangentEnd, FLinearColor(1.0f, 0.0f, 0.0f), Proxy->GetDepthPriorityGroup(View));
 						}
 						else
 						{
 							PDI->DrawLine(DrawPosition, DrawTangentEnd, FLinearColor(1.0f, 1.0f, 0.0f), Proxy->GetDepthPriorityGroup(View));
 						}
					}

					// The end will have Next set to the NULL flag...
					if (PrevParticle != NULL)
					{
						DebugParticle = PrevParticle;
						TrailPayload = PrevTrailPayload;
					}
					else
					{
						TrailPayload = NULL;
					}
				}
			}
		}
	}
}

// Data fill functions
int32 FDynamicRibbonEmitterData::FillVertexData(struct FAsyncBufferFillData& Data)
{
	SCOPE_CYCLE_COUNTER(STAT_TrailFillVertexTime);

	int32	TrianglesToRender = 0;

	uint8* TempVertexData = (uint8*)Data.VertexData;
	uint8* TempDynamicParamData = (uint8*)Data.DynamicParameterData;
	FParticleBeamTrailVertex* Vertex;
	FParticleBeamTrailVertexDynamicParameter* DynParamVertex;

	FMatrix CameraToWorld = Data.View->ViewMatrices.ViewMatrix.Inverse();
	FVector CameraUp = CameraToWorld.TransformVector(FVector(0,0,1));
	FVector	ViewOrigin	= CameraToWorld.GetOrigin();

	int32 MaxTessellationBetweenParticles = FMath::Max<int32>(Source.MaxTessellationBetweenParticles, 1);
	int32 Sheets = FMath::Max<int32>(Source.Sheets, 1);
	Sheets = 1;

	// The distance tracking for tiling the 2nd UV set
	float CurrDistance = 0.0f;

	FBaseParticle* PackingParticle;
	uint8* ParticleData = Source.ParticleData.GetData();
	for (int32 ParticleIdx = 0; ParticleIdx < Source.ActiveParticleCount; ParticleIdx++)
	{
		DECLARE_PARTICLE_PTR(Particle, ParticleData + Source.ParticleStride * Source.ParticleIndices[ParticleIdx]);
		FRibbonTypeDataPayload* TrailPayload = (FRibbonTypeDataPayload*)((uint8*)Particle + Source.TrailDataOffset);
		if (TRAIL_EMITTER_IS_HEAD(TrailPayload->Flags) == 0)
		{
			continue;
		}

		if (TRAIL_EMITTER_GET_NEXT(TrailPayload->Flags) == TRAIL_EMITTER_NULL_NEXT)
		{
			continue;
		}

		PackingParticle = Particle;
		// Pin the size to the X component
		FLinearColor CurrLinearColor = PackingParticle->Color;
		// The increment for going [0..1] along the complete trail
		float TextureIncrement = 1.0f / (TrailPayload->TriangleCount / 2);
		float Tex_U = 0.0f;
		FVector CurrTilePosition = PackingParticle->Location;
		FVector PrevTilePosition = PackingParticle->Location;
		FVector PrevWorkingUp(0,0,1);
		int32 VertexStride = sizeof(FParticleBeamTrailVertex);
		int32 DynamicParameterStride = 0;
		bool bFillDynamic = false;
		if (bUsesDynamicParameter == true && Data.DynamicParameterData != NULL)
		{
			DynamicParameterStride = sizeof(FParticleBeamTrailVertexDynamicParameter);
			if (Source.DynamicParameterDataOffset > 0)
			{
				bFillDynamic = true;
			}
		}
		float CurrTileU;
		FEmitterDynamicParameterPayload* CurrDynPayload = NULL;
		FEmitterDynamicParameterPayload* PrevDynPayload = NULL;
		FBaseParticle* PrevParticle = NULL;
		FRibbonTypeDataPayload* PrevTrailPayload = NULL;

		FVector WorkingUp = TrailPayload->Up;
		if (RenderAxisOption == Trails_CameraUp)
		{
			FVector DirToCamera = PackingParticle->Location - ViewOrigin;
			DirToCamera.Normalize();
			FVector NormailzedTangent = TrailPayload->Tangent;
			NormailzedTangent.Normalize();
			WorkingUp = NormailzedTangent ^ DirToCamera;
			if (WorkingUp.IsNearlyZero())
			{
				WorkingUp = CameraUp;
			}
		}

		while (TrailPayload)
		{
			float CurrSize = PackingParticle->Size.X * Source.Scale.X;

			int32 InterpCount = TrailPayload->RenderingInterpCount;
			if (InterpCount > 1)
			{
				check(PrevParticle);
				check(TRAIL_EMITTER_IS_HEAD(TrailPayload->Flags) == 0);

				// Interpolate between current and next...
				FVector CurrPosition = PackingParticle->Location;
				FVector CurrTangent = TrailPayload->Tangent;
				FVector CurrUp = WorkingUp;
				FLinearColor CurrColor = PackingParticle->Color;

				FVector PrevPosition = PrevParticle->Location;
				FVector PrevTangent = PrevTrailPayload->Tangent;
				FVector PrevUp = PrevWorkingUp;
				FLinearColor PrevColor = PrevParticle->Color;
				float PrevSize = PrevParticle->Size.X * Source.Scale.X;

				float InvCount = 1.0f / InterpCount;
				float Diff = PrevTrailPayload->SpawnTime - TrailPayload->SpawnTime;

				if (bFillDynamic == true)
				{
					CurrDynPayload = ((FEmitterDynamicParameterPayload*)((uint8*)(PackingParticle) + Source.DynamicParameterDataOffset));
					PrevDynPayload = ((FEmitterDynamicParameterPayload*)((uint8*)(PrevParticle) + Source.DynamicParameterDataOffset));
				}

				FVector4 InterpDynamic(1.0f, 1.0f, 1.0f, 1.0f);
				for (int32 SpawnIdx = InterpCount - 1; SpawnIdx >= 0; SpawnIdx--)
				{
					float TimeStep = InvCount * SpawnIdx;
					FVector InterpPos = FMath::CubicInterp<FVector>(CurrPosition, CurrTangent, PrevPosition, PrevTangent, TimeStep);
					FVector InterpUp = FMath::Lerp<FVector>(CurrUp, PrevUp, TimeStep);
					FLinearColor InterpColor = FMath::Lerp<FLinearColor>(CurrColor, PrevColor, TimeStep);
					float InterpSize = FMath::Lerp<float>(CurrSize, PrevSize, TimeStep);
					if (CurrDynPayload && PrevDynPayload)
					{
						InterpDynamic = FMath::Lerp<FVector4>(CurrDynPayload->DynamicParameterValue, PrevDynPayload->DynamicParameterValue, TimeStep);
					}

					if (bTextureTileDistance == true)	
					{
						CurrTileU = FMath::Lerp<float>(TrailPayload->TiledU, PrevTrailPayload->TiledU, TimeStep);
					}
					else
					{
						CurrTileU = Tex_U;
					}

					Vertex = (FParticleBeamTrailVertex*)(TempVertexData);
					Vertex->Position = InterpPos + InterpUp * InterpSize;
					Vertex->OldPosition = Vertex->Position;
					Vertex->ParticleId	= 0;
					Vertex->Size.X = InterpSize;
					Vertex->Size.Y = InterpSize;
					Vertex->Tex_U = Tex_U;
					Vertex->Tex_V = 0.0f;
					Vertex->Tex_U2 = CurrTileU;
					Vertex->Tex_V2 = 0.0f;
					Vertex->Rotation = PackingParticle->Rotation;
					Vertex->Color = InterpColor;
					if (bUsesDynamicParameter == true)
					{
						DynParamVertex = (FParticleBeamTrailVertexDynamicParameter*)(TempDynamicParamData);
						DynParamVertex->DynamicValue[0] = InterpDynamic.X;
						DynParamVertex->DynamicValue[1] = InterpDynamic.Y;
						DynParamVertex->DynamicValue[2] = InterpDynamic.Z;
						DynParamVertex->DynamicValue[3] = InterpDynamic.W;
						TempDynamicParamData += DynamicParameterStride;
					}
					TempVertexData += VertexStride;
					//PackedVertexCount++;

					Vertex = (FParticleBeamTrailVertex*)(TempVertexData);
					Vertex->Position = InterpPos - InterpUp * InterpSize;
					Vertex->OldPosition = Vertex->Position;
					Vertex->ParticleId	= 0;
					Vertex->Size.X = InterpSize;
					Vertex->Size.Y = InterpSize;
					Vertex->Tex_U = Tex_U;
					Vertex->Tex_V = 1.0f;
					Vertex->Tex_U2 = CurrTileU;
					Vertex->Tex_V2 = 1.0f;
					Vertex->Rotation = PackingParticle->Rotation;
					Vertex->Color = InterpColor;
					if (bUsesDynamicParameter == true)
					{
						DynParamVertex = (FParticleBeamTrailVertexDynamicParameter*)(TempDynamicParamData);
						DynParamVertex->DynamicValue[0] = InterpDynamic.X;
						DynParamVertex->DynamicValue[1] = InterpDynamic.Y;
						DynParamVertex->DynamicValue[2] = InterpDynamic.Z;
						DynParamVertex->DynamicValue[3] = InterpDynamic.W;
						TempDynamicParamData += DynamicParameterStride;
					}
					TempVertexData += VertexStride;
					//PackedVertexCount++;

					Tex_U += TextureIncrement;
				}
			}
			else
			{
				if (bFillDynamic == true)
				{
					CurrDynPayload = ((FEmitterDynamicParameterPayload*)((uint8*)(PackingParticle) + Source.DynamicParameterDataOffset));
				}

				if (bTextureTileDistance == true)
				{
					CurrTileU = TrailPayload->TiledU;
				}
				else
				{
					CurrTileU = Tex_U;
				}

				Vertex = (FParticleBeamTrailVertex*)(TempVertexData);
				Vertex->Position = PackingParticle->Location + WorkingUp * CurrSize;
				Vertex->OldPosition = PackingParticle->OldLocation;
				Vertex->ParticleId	= 0;
				Vertex->Size.X = CurrSize;
				Vertex->Size.Y = CurrSize;
				Vertex->Tex_U = Tex_U;
				Vertex->Tex_V = 0.0f;
				Vertex->Tex_U2 = CurrTileU;
				Vertex->Tex_V2 = 0.0f;
				Vertex->Rotation = PackingParticle->Rotation;
				Vertex->Color = PackingParticle->Color;
				if (bUsesDynamicParameter == true)
				{
					DynParamVertex = (FParticleBeamTrailVertexDynamicParameter*)(TempDynamicParamData);
					if (CurrDynPayload != NULL)
					{
						DynParamVertex->DynamicValue[0] = CurrDynPayload->DynamicParameterValue.X;
						DynParamVertex->DynamicValue[1] = CurrDynPayload->DynamicParameterValue.Y;
						DynParamVertex->DynamicValue[2] = CurrDynPayload->DynamicParameterValue.Z;
						DynParamVertex->DynamicValue[3] = CurrDynPayload->DynamicParameterValue.W;
					}
					else
					{
						DynParamVertex->DynamicValue[0] = 1.0f;
						DynParamVertex->DynamicValue[1] = 1.0f;
						DynParamVertex->DynamicValue[2] = 1.0f;
						DynParamVertex->DynamicValue[3] = 1.0f;
					}
					TempDynamicParamData += DynamicParameterStride;
				}
				TempVertexData += VertexStride;
				//PackedVertexCount++;

				Vertex = (FParticleBeamTrailVertex*)(TempVertexData);
				Vertex->Position = PackingParticle->Location - WorkingUp * CurrSize;
				Vertex->OldPosition = PackingParticle->OldLocation;
				Vertex->ParticleId	= 0;
				Vertex->Size.X = CurrSize;
				Vertex->Size.Y = CurrSize;
				Vertex->Tex_U = Tex_U;
				Vertex->Tex_V = 1.0f;
				Vertex->Tex_U2 = CurrTileU;
				Vertex->Tex_V2 = 1.0f;
				Vertex->Rotation = PackingParticle->Rotation;
				Vertex->Color = PackingParticle->Color;
				if (bUsesDynamicParameter == true)
				{
					DynParamVertex = (FParticleBeamTrailVertexDynamicParameter*)(TempDynamicParamData);
					if (CurrDynPayload != NULL)
					{
						DynParamVertex->DynamicValue[0] = CurrDynPayload->DynamicParameterValue.X;
						DynParamVertex->DynamicValue[1] = CurrDynPayload->DynamicParameterValue.Y;
						DynParamVertex->DynamicValue[2] = CurrDynPayload->DynamicParameterValue.Z;
						DynParamVertex->DynamicValue[3] = CurrDynPayload->DynamicParameterValue.W;
					}
					else
					{
						DynParamVertex->DynamicValue[0] = 1.0f;
						DynParamVertex->DynamicValue[1] = 1.0f;
						DynParamVertex->DynamicValue[2] = 1.0f;
						DynParamVertex->DynamicValue[3] = 1.0f;
					}
					TempDynamicParamData += DynamicParameterStride;
				}
				TempVertexData += VertexStride;
				//PackedVertexCount++;

				Tex_U += TextureIncrement;
			}

			PrevParticle = PackingParticle;
			PrevTrailPayload = TrailPayload;
			PrevWorkingUp = WorkingUp;

			int32	NextIdx = TRAIL_EMITTER_GET_NEXT(TrailPayload->Flags);
			if (NextIdx == TRAIL_EMITTER_NULL_NEXT)
			{
				TrailPayload = NULL;
				PackingParticle = NULL;
			}
			else
			{
				DECLARE_PARTICLE_PTR(TempParticle, ParticleData + Source.ParticleStride * NextIdx);
				PackingParticle = TempParticle;
				TrailPayload = (FRibbonTypeDataPayload*)((uint8*)TempParticle + Source.TrailDataOffset);
				WorkingUp = TrailPayload->Up;
				if (RenderAxisOption == Trails_CameraUp)
				{
					FVector DirToCamera = PackingParticle->Location - ViewOrigin;
					DirToCamera.Normalize();
					FVector NormailzedTangent = TrailPayload->Tangent;
					NormailzedTangent.Normalize();
					WorkingUp = NormailzedTangent ^ DirToCamera;
					if (WorkingUp.IsNearlyZero())
					{
						WorkingUp = CameraUp;
					}
				}
			}
		}
	}

	return TrianglesToRender;
}

///////////////////////////////////////////////////////////////////////////////
/** Dynamic emitter data for AnimTrail emitters */
/** Initialize this emitter's dynamic rendering data, called after source data has been filled in */
void FDynamicAnimTrailEmitterData::Init(bool bInSelected)
{
	SourcePointer = &Source;
	FDynamicTrailsEmitterData::Init(bInSelected);
}

void FDynamicAnimTrailEmitterData::RenderDebug(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bCrosses)
{
	if ((bRenderParticles == true) || (bRenderTangents == true))
	{
		// DEBUGGING
		// Draw all the points of the trail(s)
		FVector DrawPosition;
		FVector DrawFirstEdgePosition;
		FVector DrawSecondEdgePosition;
		float DrawSize;
		FColor DrawColor;
		FColor PrevDrawColor;
		FVector DrawTangentEnd;

		uint8* Address = Source.ParticleData.GetData();
		FAnimTrailTypeDataPayload* StartTrailPayload;
		FAnimTrailTypeDataPayload* EndTrailPayload = NULL;
		FBaseParticle* DebugParticle;
		FAnimTrailTypeDataPayload* TrailPayload;
		FBaseParticle* PrevParticle = NULL;
		FAnimTrailTypeDataPayload* PrevTrailPayload;
		for (int32 ParticleIdx = 0; ParticleIdx < Source.ActiveParticleCount; ParticleIdx++)
		{
			DECLARE_PARTICLE_PTR(Particle, Address + Source.ParticleStride * Source.ParticleIndices[ParticleIdx]);
			StartTrailPayload = (FAnimTrailTypeDataPayload*)((uint8*)Particle + Source.TrailDataOffset);
			if (TRAIL_EMITTER_IS_HEAD(StartTrailPayload->Flags) == 0)
			{
				continue;
			}

			// Pin the size to the X component
			float Increment = 1.0f / (StartTrailPayload->TriangleCount / 2);
			float ColorScale = 0.0f;

			DebugParticle = Particle;
			// Find the end particle in this chain...
			TrailPayload = StartTrailPayload;
			FBaseParticle* IteratorParticle = DebugParticle;
			while (TrailPayload)
			{
				int32	Next = TRAIL_EMITTER_GET_NEXT(TrailPayload->Flags);
				if (Next == TRAIL_EMITTER_NULL_NEXT)
				{
					DebugParticle = IteratorParticle;
					EndTrailPayload = TrailPayload;
					TrailPayload = NULL;
				}
				else
				{
					DECLARE_PARTICLE_PTR(TempParticle, Address + Source.ParticleStride * Next);
					IteratorParticle = TempParticle;
					TrailPayload = (FAnimTrailTypeDataPayload*)((uint8*)IteratorParticle + Source.TrailDataOffset);
				}
			}
			if (EndTrailPayload != StartTrailPayload)
			{
				FBaseParticle* CurrSpawnedParticle = NULL;
				FBaseParticle* NextSpawnedParticle = NULL;
				// We have more than one particle in the trail...
				TrailPayload = EndTrailPayload;

				if (TrailPayload->bInterpolatedSpawn == false)
				{
					CurrSpawnedParticle = DebugParticle;
				}
				while (TrailPayload)
				{
					int32	Prev = TRAIL_EMITTER_GET_PREV(TrailPayload->Flags);
					if (Prev == TRAIL_EMITTER_NULL_PREV)
					{
						PrevParticle = NULL;
						PrevTrailPayload = NULL;
					}
					else
					{
						DECLARE_PARTICLE_PTR(TempParticle, Address + Source.ParticleStride * Prev);
						PrevParticle = TempParticle;
						PrevTrailPayload = (FAnimTrailTypeDataPayload*)((uint8*)PrevParticle + Source.TrailDataOffset);
					}

					if (PrevTrailPayload && PrevTrailPayload->bInterpolatedSpawn == false)
					{
						if (CurrSpawnedParticle == NULL)
						{
							CurrSpawnedParticle = PrevParticle;
						}
						else
						{
							NextSpawnedParticle = PrevParticle;
						}
					}

					DrawPosition = DebugParticle->Location;
					DrawFirstEdgePosition = TrailPayload->FirstEdge;
					DrawSecondEdgePosition = TrailPayload->SecondEdge;
					DrawSize = DebugParticle->Size.X * Source.Scale.X;
					int32 Red   = FMath::Trunc(255.0f * (1.0f - ColorScale));
					int32 Green = FMath::Trunc(255.0f * ColorScale);
					ColorScale += Increment;
					DrawColor = FColor(Red,Green,0);
					Red   = FMath::Trunc(255.0f * (1.0f - ColorScale));
					Green = FMath::Trunc(255.0f * ColorScale);
					PrevDrawColor = FColor(Red,Green,0);

					if (bRenderParticles == true)
					{
						if (TrailPayload->bInterpolatedSpawn == false)
						{
							DrawWireStar(PDI, DrawPosition, DrawSize, FColor(255,0,0), Proxy->GetDepthPriorityGroup(View));
							DrawWireStar(PDI, DrawFirstEdgePosition, DrawSize, FColor(255,0,0), Proxy->GetDepthPriorityGroup(View));
							DrawWireStar(PDI, DrawSecondEdgePosition, DrawSize, FColor(255,0,0), Proxy->GetDepthPriorityGroup(View));
						}
						else
						{
							DrawWireStar(PDI, DrawPosition, DrawSize, FColor(0,255,0), Proxy->GetDepthPriorityGroup(View));
							DrawWireStar(PDI, DrawFirstEdgePosition, DrawSize, FColor(0,255,0), Proxy->GetDepthPriorityGroup(View));
							DrawWireStar(PDI, DrawSecondEdgePosition, DrawSize, FColor(0,255,0), Proxy->GetDepthPriorityGroup(View));
						}

						//
						if (bRenderTessellation == true)
						{
							if (PrevParticle != NULL)
							{
								// Draw a straight line between the particles
								// This will allow us to visualize the tessellation difference
								PDI->DrawLine(DrawPosition, PrevParticle->Location, FColor(0,0,255), Proxy->GetDepthPriorityGroup(View));
								PDI->DrawLine(DrawFirstEdgePosition, PrevTrailPayload->FirstEdge, FColor(0,0,255), Proxy->GetDepthPriorityGroup(View));
								PDI->DrawLine(DrawSecondEdgePosition, PrevTrailPayload->SecondEdge, FColor(0,0,255), Proxy->GetDepthPriorityGroup(View));

								int32 InterpCount = TrailPayload->RenderingInterpCount;
								// Interpolate between current and next...
								FVector LineStart = DrawPosition;
								FVector FirstStart = DrawFirstEdgePosition;
								FVector SecondStart = DrawSecondEdgePosition;
								float Diff = AnimSampleTimeStep;
								FVector CurrUp = FVector(0.0f, 0.0f, 1.0f);
								float InvCount = 1.0f / InterpCount;
								FLinearColor StartColor = DrawColor;
								FLinearColor EndColor = PrevDrawColor;
								for (int32 SpawnIdx = 0; SpawnIdx < InterpCount; SpawnIdx++)
								{
									float TimeStep = InvCount * SpawnIdx;
									FVector LineEnd = FMath::CubicInterp<FVector>(
										DebugParticle->Location, TrailPayload->ControlVelocity * AnimSampleTimeStep,
										PrevParticle->Location, PrevTrailPayload->ControlVelocity * AnimSampleTimeStep,
										TimeStep);
									FVector FirstEnd = FMath::CubicInterp<FVector>(
										TrailPayload->FirstEdge, TrailPayload->FirstVelocity * AnimSampleTimeStep,
										PrevTrailPayload->FirstEdge, PrevTrailPayload->FirstVelocity * AnimSampleTimeStep,
										TimeStep);
									FVector SecondEnd = FMath::CubicInterp<FVector>(
										TrailPayload->SecondEdge, TrailPayload->SecondVelocity * AnimSampleTimeStep,
										PrevTrailPayload->SecondEdge, PrevTrailPayload->SecondVelocity * AnimSampleTimeStep,
										TimeStep);
									FLinearColor InterpColor = FMath::Lerp<FLinearColor>(StartColor, EndColor, TimeStep);
									PDI->DrawLine(LineStart, LineEnd, InterpColor, Proxy->GetDepthPriorityGroup(View));
									PDI->DrawLine(FirstStart, FirstEnd, InterpColor, Proxy->GetDepthPriorityGroup(View));
									PDI->DrawLine(SecondStart, SecondEnd, InterpColor, Proxy->GetDepthPriorityGroup(View));
									if (SpawnIdx > 0)
									{
										InterpColor.R = 1.0f - TimeStep;
										InterpColor.G = 1.0f - TimeStep;
										InterpColor.B = 1.0f - (1.0f - TimeStep);
									}
									DrawWireStar(PDI, LineEnd, DrawSize * 0.3f, InterpColor, Proxy->GetDepthPriorityGroup(View));
									DrawWireStar(PDI, FirstEnd, DrawSize * 0.3f, InterpColor, Proxy->GetDepthPriorityGroup(View));
									DrawWireStar(PDI, SecondEnd, DrawSize * 0.3f, InterpColor, Proxy->GetDepthPriorityGroup(View));
									LineStart = LineEnd;
									FirstStart = FirstEnd;
									SecondStart = SecondEnd;
								}
								PDI->DrawLine(LineStart, PrevParticle->Location, EndColor, Proxy->GetDepthPriorityGroup(View));
								PDI->DrawLine(FirstStart, PrevTrailPayload->FirstEdge, EndColor, Proxy->GetDepthPriorityGroup(View));
								PDI->DrawLine(SecondStart, PrevTrailPayload->SecondEdge, EndColor, Proxy->GetDepthPriorityGroup(View));
							}
						}
					}

					if (bRenderTangents == true)
					{
						DrawTangentEnd = DrawPosition + TrailPayload->ControlVelocity * AnimSampleTimeStep;
						PDI->DrawLine(DrawPosition, DrawTangentEnd, FLinearColor(1.0f, 1.0f, 0.0f), Proxy->GetDepthPriorityGroup(View));
						DrawTangentEnd = DrawFirstEdgePosition + TrailPayload->FirstVelocity * AnimSampleTimeStep;
						PDI->DrawLine(DrawFirstEdgePosition, DrawTangentEnd, FLinearColor(1.0f, 1.0f, 0.0f), Proxy->GetDepthPriorityGroup(View));
						DrawTangentEnd = DrawSecondEdgePosition + TrailPayload->SecondVelocity * AnimSampleTimeStep;
						PDI->DrawLine(DrawSecondEdgePosition, DrawTangentEnd, FLinearColor(1.0f, 1.0f, 0.0f), Proxy->GetDepthPriorityGroup(View));
					}

					// The end will have Next set to the NULL flag...
					if (PrevParticle != NULL)
					{
						DebugParticle = PrevParticle;
						TrailPayload = PrevTrailPayload;
					}
					else
					{
						TrailPayload = NULL;
					}
				}
			}
		}
	}
}

// Data fill functions
int32 FDynamicAnimTrailEmitterData::FillVertexData(struct FAsyncBufferFillData& Data)
{
	SCOPE_CYCLE_COUNTER(STAT_TrailFillVertexTime);

	int32	TrianglesToRender = 0;

	uint8* TempVertexData = (uint8*)Data.VertexData;
	FParticleBeamTrailVertex* Vertex;
	FParticleBeamTrailVertexDynamicParameter* DynParamVertex;

	int32 Sheets = FMath::Max<int32>(Source.Sheets, 1);
	Sheets = 1;

	// The increment for going [0..1] along the complete trail
	float TextureIncrement = 1.0f / (Data.VertexCount / 2);
	// The distance tracking for tiling the 2nd UV set
	float CurrDistance = 0.0f;

	FBaseParticle* PackingParticle;
	uint8* ParticleData = Source.ParticleData.GetData();
	for (int32 ParticleIdx = 0; ParticleIdx < Source.ActiveParticleCount; ParticleIdx++)
	{
		DECLARE_PARTICLE_PTR(Particle, ParticleData + Source.ParticleStride * Source.ParticleIndices[ParticleIdx]);
		FAnimTrailTypeDataPayload* TrailPayload = (FAnimTrailTypeDataPayload*)((uint8*)Particle + Source.TrailDataOffset);
		if (TRAIL_EMITTER_IS_HEAD(TrailPayload->Flags) == 0)
		{
			continue;
		}

		if (TRAIL_EMITTER_GET_NEXT(TrailPayload->Flags) == TRAIL_EMITTER_NULL_NEXT)
		{
			continue;
		}

		PackingParticle = Particle;
		// Pin the size to the X component
		FLinearColor CurrLinearColor = PackingParticle->Color;
		float Tex_U = 0.0f;
		FVector CurrTilePosition = PackingParticle->Location;
		FVector PrevTilePosition = PackingParticle->Location;
		int32 VertexStride = sizeof(FParticleBeamTrailVertex);
		bool bFillDynamic = false;
		if (bUsesDynamicParameter == true)
		{
			VertexStride = sizeof(FParticleBeamTrailVertexDynamicParameter);
			if (Source.DynamicParameterDataOffset > 0)
			{
				bFillDynamic = true;
			}
		}
		float CurrTileU;
		FEmitterDynamicParameterPayload* CurrDynPayload = NULL;
		FEmitterDynamicParameterPayload* PrevDynPayload = NULL;
		FBaseParticle* PrevParticle = NULL;
		FAnimTrailTypeDataPayload* PrevTrailPayload = NULL;

		while (TrailPayload)
		{
			float CurrSize = PackingParticle->Size.X * Source.Scale.X;

			int32 InterpCount = TrailPayload->RenderingInterpCount;
			if (InterpCount > 1)
			{
				check(PrevParticle);
				check(TRAIL_EMITTER_IS_HEAD(TrailPayload->Flags) == 0);

				// Interpolate between current and next...
				FVector CurrPosition = PackingParticle->Location;
				FVector CurrTangent = TrailPayload->ControlVelocity;
				FVector CurrFirstEdge = TrailPayload->FirstEdge;
				FVector CurrFirstTangent = TrailPayload->FirstVelocity;
				FVector CurrSecondEdge = TrailPayload->SecondEdge;
				FVector CurrSecondTangent = TrailPayload->SecondVelocity;
				FLinearColor CurrColor = PackingParticle->Color;

				FVector PrevPosition = PrevParticle->Location;
				FVector PrevTangent = PrevTrailPayload->ControlVelocity;
				FVector PrevFirstEdge = PrevTrailPayload->FirstEdge;
				FVector PrevFirstTangent = PrevTrailPayload->FirstVelocity;
				FVector PrevSecondEdge = PrevTrailPayload->SecondEdge;
				FVector PrevSecondTangent = PrevTrailPayload->SecondVelocity;
				FLinearColor PrevColor = PrevParticle->Color;
				float PrevSize = PrevParticle->Size.X * Source.Scale.X;

				float InvCount = 1.0f / InterpCount;
				float Diff = PrevTrailPayload->SpawnTime - TrailPayload->SpawnTime;

				if (bFillDynamic == true)
				{
					CurrDynPayload = ((FEmitterDynamicParameterPayload*)((uint8*)(PackingParticle) + Source.DynamicParameterDataOffset));
					PrevDynPayload = ((FEmitterDynamicParameterPayload*)((uint8*)(PrevParticle) + Source.DynamicParameterDataOffset));
				}

				FVector4 InterpDynamic(1.0f, 1.0f, 1.0f, 1.0f);
				for (int32 SpawnIdx = InterpCount - 1; SpawnIdx >= 0; SpawnIdx--)
				{
					float TimeStep = InvCount * SpawnIdx;
					FVector InterpPos = FMath::CubicInterp<FVector>(
						CurrPosition, CurrTangent * AnimSampleTimeStep, 
						PrevPosition, PrevTangent * AnimSampleTimeStep, 
						TimeStep);
					FVector InterpFirst = FMath::CubicInterp<FVector>(
						CurrFirstEdge, CurrFirstTangent * AnimSampleTimeStep, 
						PrevFirstEdge, PrevFirstTangent * AnimSampleTimeStep, 
						TimeStep);
					FVector InterpSecond = FMath::CubicInterp<FVector>(
						CurrSecondEdge, CurrSecondTangent * AnimSampleTimeStep, 
						PrevSecondEdge, PrevSecondTangent * AnimSampleTimeStep, 
						TimeStep);
					FLinearColor InterpColor = FMath::Lerp<FLinearColor>(CurrColor, PrevColor, TimeStep);
					float InterpSize = FMath::Lerp<float>(CurrSize, PrevSize, TimeStep);
					if (CurrDynPayload && PrevDynPayload)
					{
						InterpDynamic = FMath::Lerp<FVector4>(CurrDynPayload->DynamicParameterValue, PrevDynPayload->DynamicParameterValue, TimeStep);
					}

					if (bTextureTileDistance == true)	
					{
						CurrTileU = FMath::Lerp<float>(TrailPayload->TiledU, PrevTrailPayload->TiledU, TimeStep);
					}
					else
					{
						CurrTileU = Tex_U;
					}

					Vertex = (FParticleBeamTrailVertex*)(TempVertexData);
					Vertex->Position = InterpFirst;//InterpPos + InterpFirst * InterpSize;
					Vertex->OldPosition = InterpFirst;
					Vertex->ParticleId	= 0;
					Vertex->Size.X = InterpSize;
					Vertex->Size.Y = InterpSize;
					Vertex->Tex_U = Tex_U;
					Vertex->Tex_V = 0.0f;
					Vertex->Tex_U2 = CurrTileU;
					Vertex->Tex_V2 = 0.0f;
					Vertex->Rotation = PackingParticle->Rotation;
					Vertex->Color = InterpColor;
					if (bUsesDynamicParameter == true)
					{
						DynParamVertex = (FParticleBeamTrailVertexDynamicParameter*)(TempVertexData);
						DynParamVertex->DynamicValue[0] = InterpDynamic.X;
						DynParamVertex->DynamicValue[1] = InterpDynamic.Y;
						DynParamVertex->DynamicValue[2] = InterpDynamic.Z;
						DynParamVertex->DynamicValue[3] = InterpDynamic.W;
					}
					TempVertexData += VertexStride;
					//PackedVertexCount++;

					Vertex = (FParticleBeamTrailVertex*)(TempVertexData);
					Vertex->Position = InterpSecond;//InterpPos - InterpSecond * InterpSize;
					Vertex->OldPosition = InterpSecond;
					Vertex->ParticleId	= 0;
					Vertex->Size.X = InterpSize;
					Vertex->Size.Y = InterpSize;
					Vertex->Tex_U = Tex_U;
					Vertex->Tex_V = 1.0f;
					Vertex->Tex_U2 = CurrTileU;
					Vertex->Tex_V2 = 1.0f;
					Vertex->Rotation = PackingParticle->Rotation;
					Vertex->Color = InterpColor;
					if (bUsesDynamicParameter == true)
					{
						DynParamVertex = (FParticleBeamTrailVertexDynamicParameter*)(TempVertexData);
						DynParamVertex->DynamicValue[0] = InterpDynamic.X;
						DynParamVertex->DynamicValue[1] = InterpDynamic.Y;
						DynParamVertex->DynamicValue[2] = InterpDynamic.Z;
						DynParamVertex->DynamicValue[3] = InterpDynamic.W;
					}
					TempVertexData += VertexStride;
					//PackedVertexCount++;

					Tex_U += TextureIncrement;
				}
			}
			else
			{
				if (bFillDynamic == true)
				{
					CurrDynPayload = ((FEmitterDynamicParameterPayload*)((uint8*)(PackingParticle) + Source.DynamicParameterDataOffset));
				}

				if (bTextureTileDistance == true)
				{
					CurrTileU = TrailPayload->TiledU;
				}
				else
				{
					CurrTileU = Tex_U;
				}

				Vertex = (FParticleBeamTrailVertex*)(TempVertexData);
				Vertex->Position = TrailPayload->FirstEdge;//PackingParticle->Location + TrailPayload->FirstEdge * CurrSize;
				Vertex->OldPosition = PackingParticle->OldLocation;
				Vertex->ParticleId	= 0;
				Vertex->Size.X = CurrSize;
				Vertex->Size.Y = CurrSize;
				Vertex->Tex_U = Tex_U;
				Vertex->Tex_V = 0.0f;
				Vertex->Tex_U2 = CurrTileU;
				Vertex->Tex_V2 = 0.0f;
				Vertex->Rotation = PackingParticle->Rotation;
				Vertex->Color = PackingParticle->Color;
				if (bUsesDynamicParameter == true)
				{
					DynParamVertex = (FParticleBeamTrailVertexDynamicParameter*)(TempVertexData);
					if (CurrDynPayload != NULL)
					{
						DynParamVertex->DynamicValue[0] = CurrDynPayload->DynamicParameterValue.X;
						DynParamVertex->DynamicValue[1] = CurrDynPayload->DynamicParameterValue.Y;
						DynParamVertex->DynamicValue[2] = CurrDynPayload->DynamicParameterValue.Z;
						DynParamVertex->DynamicValue[3] = CurrDynPayload->DynamicParameterValue.W;
					}
					else
					{
						DynParamVertex->DynamicValue[0] = 1.0f;
						DynParamVertex->DynamicValue[1] = 1.0f;
						DynParamVertex->DynamicValue[2] = 1.0f;
						DynParamVertex->DynamicValue[3] = 1.0f;
					}
				}
				TempVertexData += VertexStride;
				//PackedVertexCount++;

				Vertex = (FParticleBeamTrailVertex*)(TempVertexData);
				Vertex->Position = TrailPayload->SecondEdge;//PackingParticle->Location - TrailPayload->SecondEdge * CurrSize;
				Vertex->OldPosition = PackingParticle->OldLocation;
				Vertex->ParticleId	= 0;
				Vertex->Size.X = CurrSize;
				Vertex->Size.Y = CurrSize;
				Vertex->Tex_U = Tex_U;
				Vertex->Tex_V = 1.0f;
				Vertex->Tex_U2 = CurrTileU;
				Vertex->Tex_V2 = 1.0f;
				Vertex->Rotation = PackingParticle->Rotation;
				Vertex->Color = PackingParticle->Color;
				if (bUsesDynamicParameter == true)
				{
					DynParamVertex = (FParticleBeamTrailVertexDynamicParameter*)(TempVertexData);
					if (CurrDynPayload != NULL)
					{
						DynParamVertex->DynamicValue[0] = CurrDynPayload->DynamicParameterValue.X;
						DynParamVertex->DynamicValue[1] = CurrDynPayload->DynamicParameterValue.Y;
						DynParamVertex->DynamicValue[2] = CurrDynPayload->DynamicParameterValue.Z;
						DynParamVertex->DynamicValue[3] = CurrDynPayload->DynamicParameterValue.W;
					}
					else
					{
						DynParamVertex->DynamicValue[0] = 1.0f;
						DynParamVertex->DynamicValue[1] = 1.0f;
						DynParamVertex->DynamicValue[2] = 1.0f;
						DynParamVertex->DynamicValue[3] = 1.0f;
					}
				}
				TempVertexData += VertexStride;
				//PackedVertexCount++;

				Tex_U += TextureIncrement;
			}

			PrevParticle = PackingParticle;
			PrevTrailPayload = TrailPayload;

			int32	NextIdx = TRAIL_EMITTER_GET_NEXT(TrailPayload->Flags);
			if (NextIdx == TRAIL_EMITTER_NULL_NEXT)
			{
				TrailPayload = NULL;
				PackingParticle = NULL;
			}
			else
			{
				DECLARE_PARTICLE_PTR(TempParticle, ParticleData + Source.ParticleStride * NextIdx);
				PackingParticle = TempParticle;
				TrailPayload = (FAnimTrailTypeDataPayload*)((uint8*)TempParticle + Source.TrailDataOffset);
			}
		}
	}

	return TrianglesToRender;
}

///////////////////////////////////////////////////////////////////////////////
//	ParticleSystemSceneProxy
///////////////////////////////////////////////////////////////////////////////
/** Initialization constructor. */
FParticleSystemSceneProxy::FParticleSystemSceneProxy(const UParticleSystemComponent* Component, FParticleDynamicData* InDynamicData)
	: FPrimitiveSceneProxy(Component, Component->Template ? Component->Template->GetFName() : NAME_None)
	, Owner(Component->GetOwner())
	, CullDistance(Component->CachedMaxDrawDistance > 0 ? Component->CachedMaxDrawDistance : WORLD_MAX)
	, bCastShadow(Component->CastShadow)
	, MaterialRelevance(
		((Component->GetCurrentLODIndex() >= 0) && (Component->GetCurrentLODIndex() < Component->CachedViewRelevanceFlags.Num())) ?
			Component->CachedViewRelevanceFlags[Component->GetCurrentLODIndex()] :
		((Component->GetCurrentLODIndex() == -1) && (Component->CachedViewRelevanceFlags.Num() >= 1)) ?
			Component->CachedViewRelevanceFlags[0] :
			FMaterialRelevance()
		)
	, DynamicData(InDynamicData)
	, LastDynamicData(NULL)
	, SelectedWireframeMaterialInstance(
		GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(false) : NULL,
		GetSelectionColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f),true,false)
		)
	, DeselectedWireframeMaterialInstance(
		GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(false) : NULL,
		GetSelectionColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f),false,false)
		)
	, PendingLODDistance(0.0f)
	, LastFramePreRendered(-1)
{
#if STATS
	LastStatCaptureTime = GCurrentTime;
	bCountedThisFrame = false;
#endif
	LODMethod = Component->LODMethod;
}

FParticleSystemSceneProxy::~FParticleSystemSceneProxy()
{
	ReleaseRenderThreadResources();
	delete DynamicData;
	DynamicData = NULL;
}

// FPrimitiveSceneProxy interface.

/** 
* Draw the scene proxy as a dynamic element
*
* @param	PDI - draw interface to render to
* @param	View - current view
*/
void FParticleSystemSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View)
{
	QUICK_SCOPE_CYCLE_COUNTER( STAT_ParticleSystemSceneProxy_DrawDynamicElements );

	if (View->Family->EngineShowFlags.Particles)
	{
#if LOG_DETAILED_PARTICLE_RENDER_STATS
		static uint64 LastFrameCounter = 0;
		if( LastFrameCounter != GFrameCounter )
		{
			GDetailedParticleRenderStats.DumpStats();
			GDetailedParticleRenderStats.Reset();
			LastFrameCounter = GFrameCounter;
		}

		UParticleSystemComponent* ParticleSystemComponent = CastChecked<UParticleSystemComponent>(PrimitiveSceneInfo->Component);
		if (ParticleSystemComponent->Template == NULL)
		{
			return;
		}
#endif
		SCOPE_CYCLE_COUNTER(STAT_ParticleRenderingTime);
		TRACK_DETAILED_PARTICLE_RENDER_STATS(ParticleSystemComponent->Template);
		FScopeCycleCounter Context(GetStatId());

		const double StartTime = GTrackParticleRenderingStats || GTrackParticleRenderingStatsForOneFrame ? FPlatformTime::Seconds() : 0;

		int32 NumDraws = 0;
		int32 NumEmitters = 0;

			if (DynamicData != NULL)
			{
				for (int32 Index = 0; Index < DynamicData->DynamicEmitterDataArray.Num(); Index++)
				{
					FDynamicEmitterDataBase* Data =	DynamicData->DynamicEmitterDataArray[Index];
					if ((Data == NULL) || (Data->bValid != true))
					{
						continue;
					}
					//hold on to the emitter index in case we need to access any of its properties
					DynamicData->EmitterIndex = Index;

					const int32 DrawCalls = Data->Render(this, PDI, View);
					NumDraws += DrawCalls;
					if (DrawCalls > 0)
					{
						NumEmitters++;
					}
				}
			}

		INC_DWORD_STAT_BY(STAT_ParticleDrawCalls, NumDraws);

		if (View->Family->EngineShowFlags.Particles)
		{
			RenderBounds(PDI, View->Family->EngineShowFlags, GetBounds(), IsSelected());
			if (HasCustomOcclusionBounds())
			{
				RenderBounds(PDI, View->Family->EngineShowFlags, GetCustomOcclusionBounds(), IsSelected());
			}
		}

#if STATS

		// Capture one frame rendering stats if enabled and at least one draw call was submitted
		if (GTrackParticleRenderingStatsForOneFrame
			&& NumDraws > 0)
		{
			const double EndTime = FPlatformTime::Seconds();

			const float DeltaTime = EndTime - StartTime;
			const FString TemplateName = GetResourceName().ToString();
			FParticleTemplateRenderStats* TemplateStats = GOneFrameTemplateRenderStats.Find(TemplateName);

			if (TemplateStats)
			{
				// Update the existing record for this UParticleSystem
				TemplateStats->NumDraws += NumDraws;
				TemplateStats->MaxRenderTime += DeltaTime;
				TemplateStats->NumEmitters += NumEmitters;
				TemplateStats->NumDrawDynamicElements++;
				if (!bCountedThisFrame)
				{
					TemplateStats->NumComponents++;
				}
			}
			else
			{
				// Create a new record for this UParticleSystem
				FParticleTemplateRenderStats NewStats;
				NewStats.NumDraws = NumDraws;
				NewStats.MaxRenderTime = DeltaTime;
				NewStats.NumEmitters = NumEmitters;
				NewStats.NumComponents = 1;
				NewStats.NumDrawDynamicElements = 1;
				GOneFrameTemplateRenderStats.Add(TemplateName, NewStats);
			}
			bCountedThisFrame = true;
		}

		if (!GTrackParticleRenderingStatsForOneFrame)
		{
			// Mark the proxy has not having been processed, this will be used on the next frame dump to count components rendered accurately
			bCountedThisFrame = false;
		}

		// Capture render stats if enabled, and at least one draw call was submitted, and enough time has elapsed since the last capture
		// This misses spikes but allows efficient stat gathering over large periods of time
		if (GTrackParticleRenderingStats 
			// The main purpose of particle rendering stats are to optimize splitscreen, so only capture when we are in medium detail mode
			// This is needed to prevent capturing particle rendering time during cinematics where the engine switches back to high detail mode
			&& GetCachedScalabilityCVars().DetailMode != DM_High
			&& NumDraws > 0
			&& GCurrentTime - LastStatCaptureTime > GTimeBetweenParticleRenderStatCaptures)
		{
			LastStatCaptureTime = GCurrentTime;

			const double EndTime = FPlatformTime::Seconds();

			const float DeltaTime = EndTime - StartTime;
			// Only capture stats if the rendering time was large enough to be considered
			if (DeltaTime > GMinParticleDrawTimeToTrack)
			{
				const FString TemplateName = GetResourceName().ToString();
				FParticleTemplateRenderStats* TemplateStats = GTemplateRenderStats.Find(TemplateName);
				if (TemplateStats)
				{
					// Update the existing record for this UParticleSystem if the new stat had a longer render time
					if (DeltaTime > TemplateStats->MaxRenderTime)
					{
						TemplateStats->NumDraws = NumDraws;
						TemplateStats->MaxRenderTime = DeltaTime;
						TemplateStats->NumEmitters = NumEmitters;
					}
				}
				else
				{
					// Create a new record for this UParticleSystem
					FParticleTemplateRenderStats NewStats;
					NewStats.NumDraws = NumDraws;
					NewStats.MaxRenderTime = DeltaTime;
					NewStats.NumEmitters = NumEmitters;
					GTemplateRenderStats.Add(TemplateName, NewStats);
				}

				const FString ComponentName = GetOwnerName().ToString();
				if (ComponentName.Contains(TEXT("Emitter_")) )
				{
					FParticleComponentRenderStats* ComponentStats = GComponentRenderStats.Find(ComponentName);
					if (ComponentStats)
					{
						// Update the existing record for this component if the new stat had a longer render time
						if (DeltaTime > ComponentStats->MaxRenderTime)
						{
							ComponentStats->NumDraws = NumDraws;
							ComponentStats->MaxRenderTime = DeltaTime;
						}
					}
					else
					{
						// Create a new record for this component
						FParticleComponentRenderStats NewStats;
						NewStats.NumDraws = NumDraws;
						NewStats.MaxRenderTime = DeltaTime;
						GComponentRenderStats.Add(ComponentName, NewStats);
					}
				}
			}
		}
#endif
	}
}

void FParticleSystemSceneProxy::CreateRenderThreadResources()
{
	CreateRenderThreadResourcesForEmitterData();
}

void FParticleSystemSceneProxy::ReleaseRenderThreadResources()
{
	ReleaseRenderThreadResourcesForEmitterData();
	WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

void FParticleSystemSceneProxy::CreateRenderThreadResourcesForEmitterData()
{
	if (DynamicData)
	{
		for (int32 Index = 0; Index < DynamicData->DynamicEmitterDataArray.Num(); Index++)
		{
			FDynamicEmitterDataBase* Data =	DynamicData->DynamicEmitterDataArray[Index];
			if (Data != NULL)
			{
				Data->CreateRenderThreadResources(this);
			}
		}
	}
}

void FParticleSystemSceneProxy::ReleaseRenderThreadResourcesForEmitterData()
{
	if (DynamicData)
	{
		for (int32 Index = 0; Index < DynamicData->DynamicEmitterDataArray.Num(); Index++)
		{
			FDynamicEmitterDataBase* Data =	DynamicData->DynamicEmitterDataArray[Index];
			if (Data != NULL)
			{
				Data->ReleaseRenderThreadResources(this);
			}
		}
	}
}

void FParticleSystemSceneProxy::UpdateData(FParticleDynamicData* NewDynamicData)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		ParticleUpdateDataCommand,
		FParticleSystemSceneProxy*, Proxy, this,
		FParticleDynamicData*, NewDynamicData, NewDynamicData,
		{
			SCOPE_CYCLE_COUNTER(STAT_ParticleUpdateRTTime);
			STAT(FScopeCycleCounter Context(Proxy->GetStatId());)
			Proxy->UpdateData_RenderThread(NewDynamicData);
		}
		);
}

void FParticleSystemSceneProxy::UpdateData_RenderThread(FParticleDynamicData* NewDynamicData)
{

	ReleaseRenderThreadResourcesForEmitterData();
	if (DynamicData != NewDynamicData)
	{
		delete DynamicData;
	}
	DynamicData = NewDynamicData;
	CreateRenderThreadResourcesForEmitterData();
}

void FParticleSystemSceneProxy::DetermineLODDistance(const FSceneView* View, int32 FrameNumber)
{
	int32	LODIndex = -1;

	if (LODMethod == PARTICLESYSTEMLODMETHOD_Automatic)
	{
		// Default to the highest LOD level
		FVector	CameraPosition		= View->ViewMatrices.ViewOrigin;
		FVector	ComponentPosition	= GetLocalToWorld().GetOrigin();
		FVector	DistDiff			= ComponentPosition - CameraPosition;
		float	Distance			= DistDiff.Size() * View->LODDistanceFactor;

		if (FrameNumber != LastFramePreRendered)
		{
			// First time in the frame - then just set it...
			PendingLODDistance = Distance;
			LastFramePreRendered = FrameNumber;
		}
		else if (Distance < PendingLODDistance)
		{
			// Not first time in the frame, then we compare and set if closer
			PendingLODDistance = Distance;
		}
	}
}

/** Object position in post projection space. */
void FParticleSystemSceneProxy::GetObjectPositionAndScale(const FSceneView& View, FVector2D& ObjectNDCPosition, FVector2D& ObjectMacroUVScales) const
{
	const FVector4 ObjectPostProjectionPositionWithW = View.ViewProjectionMatrix.TransformPosition(DynamicData->SystemPositionForMacroUVs);
	ObjectNDCPosition = FVector2D(ObjectPostProjectionPositionWithW / FMath::Max(ObjectPostProjectionPositionWithW.W, 0.00001f));
	
	float MacroUVRadius = DynamicData->SystemRadiusForMacroUVs;
	FVector MacroUVPosition = DynamicData->SystemPositionForMacroUVs;
   
	uint32 Index = DynamicData->EmitterIndex;
	const FDynamicEmitterReplayDataBase& EmitterData = DynamicData->DynamicEmitterDataArray[Index]->GetSource();
	if (EmitterData.bOverrideSystemMacroUV)
	{
	    MacroUVRadius = EmitterData.MacroUVRadius;
		MacroUVPosition = GetLocalToWorld().TransformVector(EmitterData.MacroUVPosition);
	}

	if (MacroUVRadius > 0.0f)
	{
		// Need to determine the scales required to transform positions into UV's for the ParticleMacroUVs material node
		// Determine screenspace extents by transforming the object position + appropriate camera vector * radius
		const FVector4 RightPostProjectionPosition = View.ViewProjectionMatrix.TransformPosition(MacroUVPosition + MacroUVRadius * View.ViewMatrices.ViewMatrix.GetColumn(0));
		const FVector4 UpPostProjectionPosition = View.ViewProjectionMatrix.TransformPosition(MacroUVPosition + MacroUVRadius * View.ViewMatrices.ViewMatrix.GetColumn(1));
		//checkSlow(RightPostProjectionPosition.X - ObjectPostProjectionPositionWithW.X >= 0.0f && UpPostProjectionPosition.Y - ObjectPostProjectionPositionWithW.Y >= 0.0f);

        
		// Scales to transform the view space positions corresponding to SystemPositionForMacroUVs +- SystemRadiusForMacroUVs into [0, 1] in xy
		// Scales to transform the screen space positions corresponding to SystemPositionForMacroUVs +- SystemRadiusForMacroUVs into [0, 1] in zw
		ObjectMacroUVScales = FVector2D(
			1.0f / (RightPostProjectionPosition.X / RightPostProjectionPosition.W - ObjectNDCPosition.X), 
			-1.0f / (UpPostProjectionPosition.Y / UpPostProjectionPosition.W - ObjectNDCPosition.Y)
			);
	}
	else
	{
		ObjectMacroUVScales = FVector2D(0,0);
	}
}

/**
* @return Relevance for rendering the particle system primitive component in the given View
*/
FPrimitiveViewRelevance FParticleSystemSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View) && View->Family->EngineShowFlags.Particles;
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bDynamicRelevance = true;
	Result.bNeedsPreRenderView = Result.bDrawRelevance || Result.bShadowRelevance;
	Result.bHasSimpleLights = true;
	if (!View->Family->EngineShowFlags.Wireframe && View->Family->EngineShowFlags.Materials)
	{
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
	}
	if (View->Family->EngineShowFlags.Bounds || View->Family->EngineShowFlags.VectorFields)
	{
		Result.bOpaqueRelevance = true;
	}
	// see if any of the emitters use dynamic vertex data
	if (DynamicData == NULL)
	{
		// In order to get the LOD distances to update,
		// we need to force a call to DrawDynamicElements...
		Result.bOpaqueRelevance = true;
	}

	return Result;
}

void FParticleSystemSceneProxy::OnActorPositionChanged()
{
	WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

void FParticleSystemSceneProxy::OnTransformChanged()
{
	WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

void FParticleSystemSceneProxy::UpdateWorldSpacePrimitiveUniformBuffer()
{
	check(IsInRenderingThread());
	if (!WorldSpacePrimitiveUniformBuffer.IsInitialized())
	{
		FPrimitiveUniformShaderParameters PrimitiveUniformShaderParameters = GetPrimitiveUniformShaderParameters(
			FMatrix::Identity,
			GetActorPosition(),
			GetBounds(),
			GetLocalBounds(),
			ReceivesDecals(),
			1.0f			// LPV bias
			);
		WorldSpacePrimitiveUniformBuffer.SetContents(PrimitiveUniformShaderParameters);
		WorldSpacePrimitiveUniformBuffer.InitResource();
	}
}

/**
 *	Helper function for calculating the tessellation for a given view.
 *
 *	@param	View		The view of interest.
 *	@param	FrameNumber		The frame number being rendered.
 */
void FParticleSystemSceneProxy::ProcessPreRenderView(const FSceneView* View, int32 FrameNumber)
{
	const FSceneView* LocalView = View;

	if ((GIsEditor == true) || (GbEnableGameThreadLODCalculation == false))
	{
		DetermineLODDistance(LocalView, FrameNumber);
	}
}

/**
 *	Called during InitViews for view processing on scene proxies before rendering them
 *  Only called for primitives that are visible and have bDynamicRelevance
 *
 *	@param	ViewFamily		The ViewFamily to pre-render for
 *	@param	VisibilityMap	A BitArray that indicates whether the primitive was visible in that view (index)
 *	@param	FrameNumber		The frame number of this pre-render
 */
void FParticleSystemSceneProxy::PreRenderView(const FSceneViewFamily* ViewFamily, const uint32 VisibilityMap, int32 FrameNumber)
{
	for (int32 ViewIndex = 0; ViewIndex < ViewFamily->Views.Num(); ViewIndex++)
	{
		ProcessPreRenderView(ViewFamily->Views[ViewIndex], FrameNumber);
	}

	if (DynamicData != NULL)
	{
		for (int32 EmitterIndex = 0; EmitterIndex < DynamicData->DynamicEmitterDataArray.Num(); EmitterIndex++)
		{
			FDynamicEmitterDataBase* DynamicEmitterData = DynamicData->DynamicEmitterDataArray[EmitterIndex];
			if (DynamicEmitterData)
			{
				DynamicData->EmitterIndex = EmitterIndex;
				DynamicEmitterData->PreRenderView(this, ViewFamily, VisibilityMap, FrameNumber);
			}
		}
	}
}

void FParticleSystemSceneProxy::GatherSimpleLights(TArray<FSimpleLightEntry, SceneRenderingAllocator>& OutParticleLights) const
{
	if (DynamicData != NULL)
	{
		for (int32 EmitterIndex = 0; EmitterIndex < DynamicData->DynamicEmitterDataArray.Num(); EmitterIndex++)
		{
			const FDynamicEmitterDataBase* DynamicEmitterData = DynamicData->DynamicEmitterDataArray[EmitterIndex];
			if (DynamicEmitterData)
			{
				DynamicEmitterData->GatherSimpleLights(this, OutParticleLights);
			}
		}
	}
}

/**
 *	Occluding particle system scene proxy...
 */
/** Initialization constructor. */
FParticleSystemOcclusionSceneProxy::FParticleSystemOcclusionSceneProxy(const UParticleSystemComponent* Component, FParticleDynamicData* InDynamicData) :
	  FParticleSystemSceneProxy(Component,InDynamicData)
	, bHasCustomOcclusionBounds(false)
{
	if (Component->Template && (Component->Template->OcclusionBoundsMethod == EPSOBM_CustomBounds))
	{
		OcclusionBounds = FBoxSphereBounds(Component->Template->CustomOcclusionBounds);
		bHasCustomOcclusionBounds = true;
	}
}

FParticleSystemOcclusionSceneProxy::~FParticleSystemOcclusionSceneProxy()
{
}

/** 
 * Draw the scene proxy as a dynamic element
 *
 * @param	PDI - draw interface to render to
 * @param	View - current view
 */
void FParticleSystemOcclusionSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View)
{
	QUICK_SCOPE_CYCLE_COUNTER( STAT_ParticleSystemOcclusionSceneProxy_DrawDynamicElements );

	if (View->Family->EngineShowFlags.Particles)
	{
		if (DynamicData != NULL)
		{
			FBoxSphereBounds TempOcclusionBounds = OcclusionBounds;
			if (bHasCustomOcclusionBounds == true)
			{
				OcclusionBounds = OcclusionBounds.TransformBy(GetLocalToWorld());
			}

			// Update the occlusion bounds for next frame...
			if (bHasCustomOcclusionBounds == false)
			{
				OcclusionBounds = GetBounds();
			}
			else
			{
				OcclusionBounds = TempOcclusionBounds;
			}

			FParticleSystemSceneProxy::DrawDynamicElements(PDI, View);
			}
		}
	}

FPrimitiveSceneProxy* UParticleSystemComponent::CreateSceneProxy()
{
	FParticleSystemSceneProxy* NewProxy = NULL;

	//@fixme EmitterInstances.Num() check should be here to avoid proxies for dead emitters but there are some edge cases where it happens for emitters that have just activated...
	//@fixme Get non-instanced path working in ES2!
	if ((bIsActive == true)/** && (EmitterInstances.Num() > 0)*/ && Template)
	{
		UE_LOG(LogParticles,Verbose,
			TEXT("CreateSceneProxy @ %fs %s bIsActive=%d"), GetWorld()->TimeSeconds,
			Template != NULL ? *Template->GetName() : TEXT("NULL"), bIsActive);

		if (EmitterInstances.Num() > 0)
		{
			CacheViewRelevanceFlags(Template);
		}

		// Create the dynamic data for rendering this particle system.
		FParticleDynamicData* ParticleDynamicData = CreateDynamicData();

		if (Template->OcclusionBoundsMethod == EPSOBM_None)
		{
			NewProxy = ::new FParticleSystemSceneProxy(this,ParticleDynamicData);
		}
		else
		{
			Template->CustomOcclusionBounds.IsValid = true;
			NewProxy = ::new FParticleSystemOcclusionSceneProxy(this,ParticleDynamicData);
		}
		check (NewProxy);
	}
	
	// 
	return NewProxy;
}

#if WITH_EDITOR
void DrawParticleSystemHelpers(UParticleSystemComponent* InPSysComp, const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	if (InPSysComp != NULL)
	{
		for (int32 EmitterIdx = 0; EmitterIdx < InPSysComp->EmitterInstances.Num(); EmitterIdx++)
		{
			FParticleEmitterInstance* EmitterInst = InPSysComp->EmitterInstances[EmitterIdx];
			if (EmitterInst && EmitterInst->SpriteTemplate)
			{
				UParticleLODLevel* LODLevel = EmitterInst->SpriteTemplate->GetCurrentLODLevel(EmitterInst);
				for (int32 ModuleIdx = 0; ModuleIdx < LODLevel->Modules.Num(); ModuleIdx++)
				{
					UParticleModule* Module = LODLevel->Modules[ModuleIdx];
					if (Module && Module->bSupported3DDrawMode && Module->b3DDrawMode)
					{
						Module->Render3DPreview(EmitterInst, View, PDI);
					}
				}
			}
		}
	}
}

ENGINE_API void DrawParticleSystemHelpers(const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	for (TObjectIterator<AEmitter> It; It; ++It)
	{
		AEmitter* EmitterActor = *It;
		if (EmitterActor->ParticleSystemComponent.IsValid())
		{
			DrawParticleSystemHelpers(EmitterActor->ParticleSystemComponent, View, PDI);
		}
	}
}
#endif	//#if WITH_EDITOR

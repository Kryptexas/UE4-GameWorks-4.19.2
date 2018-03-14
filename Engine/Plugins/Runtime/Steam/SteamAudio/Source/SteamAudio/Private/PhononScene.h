//
// Copyright (C) Valve Corporation. All rights reserved.
//

#pragma once

#include "phonon.h"

#include "Async.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

class AActor;
class AStaticMeshActor;
class UWorld;

namespace SteamAudio
{
	struct FPhononSceneInfo
	{
		FPhononSceneInfo()
			: NumTriangles(0)
			, DataSize(0)
		{}

		uint32 NumTriangles;
		uint32 DataSize;
	};

	bool STEAMAUDIO_API LoadSceneFromDisk(UWorld* World, IPLhandle ComputeDevice, const IPLSimulationSettings& SimulationSettings,
		IPLhandle* PhononScene, FPhononSceneInfo& PhononSceneInfo);

	bool STEAMAUDIO_API LoadSceneInfoFromDisk(UWorld* World, FPhononSceneInfo& PhononSceneInfo);

#if WITH_EDITOR

	/** Creates and populates a Phonon scene. Gathers Phonon Geometry asynchronously on the game thread. */
	bool STEAMAUDIO_API CreateScene(UWorld* World, IPLhandle* PhononScene, TArray<IPLhandle>* PhononStaticMeshes, uint32& NumSceneTriangles);
	bool STEAMAUDIO_API SaveFinalizedSceneToDisk(UWorld* World, IPLhandle PhononScene, const FPhononSceneInfo& PhononSceneInfo);

	void STEAMAUDIO_API AddGeometryComponentsToStaticMeshes(UWorld* World);
	void STEAMAUDIO_API RemoveGeometryComponentsFromStaticMeshes(UWorld* World);

#endif // WITH_EDITOR

	uint32 GetNumTrianglesForStaticMesh(AStaticMeshActor* StaticMeshActor);
	uint32 GetNumTrianglesAtRoot(AActor* RootActor);
}

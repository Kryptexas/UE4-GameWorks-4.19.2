//
// Copyright (C) Valve Corporation. All rights reserved.
//

#pragma once

#include "GameFramework/Actor.h"
THIRD_PARTY_INCLUDES_START
#include "phonon.h"
THIRD_PARTY_INCLUDES_END

class AStaticMeshActor;

namespace SteamAudio
{
#if WITH_EDITOR
	void STEAMAUDIO_API LoadScene(class UWorld* World, IPLhandle* PhononScene, TArray<IPLhandle>* PhononStaticMeshes);
	uint32 GetNumTrianglesForStaticMesh(AStaticMeshActor* StaticMeshActor);
	uint32 GetNumTrianglesAtRoot(AActor* RootActor);
#endif // WITH_EDITOR
};

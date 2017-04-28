//
// Copyright (C) Valve Corporation. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "phonon.h"

class AStaticMeshActor;

namespace SteamAudio
{
	void STEAMAUDIO_API LoadScene(class UWorld* World, IPLhandle* PhononScene, TArray<IPLhandle>* PhononStaticMeshes);
	uint32 GetNumTrianglesForStaticMesh(AStaticMeshActor* StaticMeshActor);
	uint32 GetNumTrianglesAtRoot(AActor* RootActor);
}
//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "phonon.h"
#include "CoreMinimal.h"

namespace Phonon
{
	PHONON_API void LoadScene(class UWorld* World, IPLhandle* PhononScene, TArray<IPLhandle>* PhononStaticMeshes);
}
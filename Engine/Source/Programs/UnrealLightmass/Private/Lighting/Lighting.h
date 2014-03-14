// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Lighting.h: Private static lighting system includes.
=============================================================================*/

#pragma once

#include "LMCore.h"

/** 
 * Set to 1 to allow selecting lightmap texels by holding down T and left clicking in the editor,
 * And having debug information about that texel tracked during subsequent lighting rebuilds.
 * Be sure to set the define with the same name in Unreal!
 */
#define ALLOW_LIGHTMAP_SAMPLE_DEBUGGING	0

#include "LightmassScene.h"
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "LightingMesh.h"
#include "BuildOptions.h"
#include "LightmapData.h"
#include "Mappings.h"
#include "Collision.h"
#include "BSP.h"
#include "StaticMesh.h"
#include "FluidSurface.h"
#include "Landscape.h"

namespace Lightmass
{
	extern double GStartupTime;
}

#define WORLD_MAX			524288.0	/* Maximum size of the world */
#define HALF_WORLD_MAX		262144.0	/* Half the maximum size of the world */



// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ProxyLODMeshTypes.h"  // FVertexDataMesh
#include "ProxyLODGrid2d.h" // FTextureAtlasDesc

#include <vector>

/**
*  In mesh segmentation (charts and atlases) and parameterization (generating UVs ) we rely 
*  primarily on the open source DirectX Mesh code.  The methods here provide access to that 
*  functionality.
* 
*/

namespace ProxyLOD
{
	/**
	* Primary entry point:
	* Method that generates new UVs on the FVertexDataMesh according to the parameters specified in the FTextureAtlasDesc
	* The underlying code uses Isometeric approach (Iso-Charts) in UV generation.
	*
	* As a debugging option, the updated InOutMesh can have vertex colors that distinguish the various UV charts.
	*
	* NB: The mesh vertex count may change as vertices are split on UV seams.
	* 
	* @param InOutMesh            Mesh that will be updated by this function, adding UVs
	* @param InTextureAtlasDesc   Description of the texel resolution of the desired texture atlas.
	* @param bVertColorParts      Option to add vertex colors according the the chart in the texture atlas, used for debugging.
	*
	* @return 'true' if the UV generation succeeded,  'false' if it failed.
	*/
	bool GenerateUVs( FVertexDataMesh& InOutMesh, const FTextureAtlasDesc& InTextureAtlasDesc, const bool bVertexColorParts = false);

	/**
	* Generate adjacency data needed for the mesh, additionally this may alter the mesh in attempting to 
	* remove mesh degeneracy problems.  This method is primarily called within GenerateUVs
	*
	* @param InOutMesh          Mesh to process.
	* @param OutAdjacency       Adjacency data understood by the DirectX Mesh code.
	*
	* @return  'true' if the mesh was successfully cleaned of all bow-ties.
	*/
	bool GenerateAdjacenyAndCleanMesh(FVertexDataMesh& InOutMesh, std::vector<uint32>& OutAdjacency);
 
	/**
	* Generate mesh adjacency used by mesh clean code and uv generation code.
	* Various mesh types are supported.
	*
	* @param InOutMesh          Mesh to process.
	* @param OutAdjacency       Adjacency data understood by the DirectX Mesh code.
	*
	*/
	void GenerateAdjacency(const FVertexDataMesh& InMesh, std::vector<uint32>& OutAdjacencyArray);
	void GenerateAdjacency(const FAOSMesh& InMesh, std::vector<uint32>& OutAdjacencyArray);
	void GenerateAdjacency(const FRawMesh& InMesh, std::vector<uint32>& OutAdjacencyArray);
}
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "ProxyLODMeshTypes.h"
#include "ProxyLODMeshConvertUtils.h"

#include <openvdb/openvdb.h>



#include <openvdb/tools/VolumeToMesh.h> // for VolumeToMesh


namespace ProxyLOD
{

	/**
	* Generate a single sparse Signed Distance Field (SDF) Grid from the polys managed by MeshAdapter,  
	* Optionally store the index of the closest poly is recorded in a sparse PolyIndexGrid.  
	*
	* @param  InMeshAdapter     Source polygons to be voxelized.
	* @param  OutSDFGrid        Resulting sparse signed distance field.
	* @param  OutPolyIndexGrid  Optional resulting sparse grid. For each voxel with valid distance in the OutSDFGrid
	*                           the index of the closest poly will be recorded in the corresponding voxel 
	*                           of the OutPolyIndedGrid.
	*
	* @retun 'true' is success, 'false' will generally be an out of memory error.
	*/
	bool MeshArrayToSDFVolume( const FRawMeshArrayAdapter& InMeshAdapter,
	                           openvdb::FloatGrid::Ptr& OutSDFGrid,
		                       openvdb::Int32Grid* OutPolyIndexGrid = nullptr);

	/**
	* Extract the isosurface (=IsoValue) of the given SDF volume, in the form of triangle mesh.
	*
	* NB: This produces geometry only, and ignores Tangents, Texture Coordinates and Colors etc. 
	* 
	* @param  InSDFVolume       Sparse signed distance field: voxel based representation of geometry.  
	* @param  IsoValue          Indicates the surface defined by this distance value.
	* @param  AdaptivityValue   Internal adaptivity attempts to simplify regions of low curvature. 
	*                           Local face normals are compared using this value as a threshold.
	* @param  OutMesh           Resulting triangular mesh that is consistent with the surface defined by the iso-value.   
	*/
	template <typename DstMeshType>
	void SDFVolumeToMesh( const openvdb::FloatGrid::ConstPtr InSDFVolume, 
		                  const double IsoValue, 
						  const double AdaptivityValue, 
		                  DstMeshType& OutMesh);

	/**
	* Extract the isosurface (=IsoValue) of the given SDF volume, in the form of triangle mesh and adds
	* very simple vertex normals.
	*
	* NB: This produces geometry only, and ignores Tangents, Texture Coordinates and Colors etc.
	*
	* @param  InSDFVolume       Sparse signed distance field: voxel based representation of geometry.
	* @param  IsoValue          Indicates the surface defined by this distance value.
	* @param  AdaptivityValue   Internal adaptivity attempts to simplify regions of low curvature.
	*                           Local face normals are compared using this value as a threshold.
	* @param  OutMesh           Resulting triangular mesh that is consistent with the surface defined by the iso-value.
	*/
	template <typename AOSVertexType>
	void ExtractIsosurfaceWithNormals( const openvdb::FloatGrid::ConstPtr InSDFVolume, 
		                              const double IsoValue, 
		                              const double Adaptivity, 
		                              TAOSMesh<AOSVertexType>& OutMesh);
}




template <typename DstMeshType>
void ProxyLOD::SDFVolumeToMesh(const openvdb::FloatGrid::ConstPtr SDFVolume, const double IsoValue, const double Adaptivity, DstMeshType& OutMesh)
{

	// we should be generating a new FRawMesh.

	OutMesh.Empty();

	// Temporary Mesh that supports Quads and Triangles

	FMixedPolyMesh TempMixedPolyMesh;

	// Convert to Quad Mesh - extracting iso-surface defined by IsoValue

	openvdb::tools::volumeToMesh(*SDFVolume, TempMixedPolyMesh.Points, TempMixedPolyMesh.Triangles, TempMixedPolyMesh.Quads, IsoValue, Adaptivity);

	//TestUniqueVertexes(SimpleMesh);

	
	MixedPolyMeshToAOSMesh(TempMixedPolyMesh, OutMesh);
}

template <typename AOSVertexType>
void ProxyLOD::ExtractIsosurfaceWithNormals(const openvdb::FloatGrid::ConstPtr SDFVolume, const double IsoValue, const double Adaptivity, TAOSMesh<AOSVertexType>& OutMesh)
{
	SDFVolumeToMesh(SDFVolume, IsoValue, Adaptivity, OutMesh);

	AddNormals(OutMesh);
}


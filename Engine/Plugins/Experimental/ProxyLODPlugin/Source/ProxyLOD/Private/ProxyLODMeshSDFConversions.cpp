// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ProxyLODMeshSDFConversions.h"

#include "CoreMinimal.h"

#include <openvdb/tools/MeshToVolume.h> // for MeshToVolume

//#include <exception>


bool ProxyLOD::MeshArrayToSDFVolume(const FRawMeshArrayAdapter& MeshAdapter, openvdb::FloatGrid::Ptr& SDFGrid, openvdb::Int32Grid* PolyIndexGrid)
{

	bool success = true;
	try
	{
		const float HalfBandWidth = 2.f;
		int Flags = 0;
		SDFGrid = openvdb::tools::meshToVolume<openvdb::FloatGrid>(MeshAdapter, MeshAdapter.GetTransform(), HalfBandWidth /*exterior*/, HalfBandWidth/*interior*/, Flags, PolyIndexGrid);

		// reduce memory footprint, increase the spareness.

		openvdb::tools::pruneLevelSet(SDFGrid->tree(), HalfBandWidth, -HalfBandWidth);
	}
	catch (std::bad_alloc& )
	{
		success = false;
		if (SDFGrid)
		{
			SDFGrid->tree().clear(); // free any memory held in the smart pointers in the grid
		}
		if (PolyIndexGrid)
		{
			PolyIndexGrid->clear();
		}
	}

#if 0
	// Used in testing
	openvdb::tree::LeafManager<openvdb::FloatTree> leafManager(SDFGrid->tree());

	// The number of 8x8x8 voxel bricks.
	size_t leafCount = leafManager.leafCount();
	// number of voxels with real distance values
	size_t activeCount = SDFGrid->tree().activeLeafVoxelCount();
	// total amount of memory used.
	size_t numByets = SDFGrid->tree().memUsage();
#endif

	return success;

}

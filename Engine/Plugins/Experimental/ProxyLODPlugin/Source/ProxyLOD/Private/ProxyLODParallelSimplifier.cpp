// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ProxyLODParallelSimplifier.h"
#include "ProxyLODMeshPartition.h"

void ProxyLOD::PartitionAndSimplifyMesh(const FBBox& SrcBBox, const TArray<int32>& SrcTriNumByPartition, const float MinFractionToRetain, const float MaxFeatureCost, const int32 NumPartitions,
	FAOSMesh& InOutMesh)
{
	typedef FAOSMesh                           SimplifierMeshType;
	typedef ProxyLOD::FQuadricMeshSimplifier   FMeshSimplifier;

	check(NumPartitions > 0);
	check(SrcTriNumByPartition.Num() == NumPartitions);


	// Create individual meshes for each partition.

	TArray<SimplifierMeshType> PartitionedMeshArray;
	PartitionedMeshArray.Empty(NumPartitions);
	PartitionedMeshArray.AddZeroed(NumPartitions);

	// Partition the mesh along the major axis.  This copies the InOutMesh into the PartitionedMeshArray 

	PartitionOnMajorAxis(InOutMesh, SrcBBox, NumPartitions, PartitionedMeshArray);

	// Empty the source mesh.

	InOutMesh.Empty();

	// Allocate space for the seam vert ids.

	TArray<int32>*  SeamVertexIdArrayCollection = new TArray<int32>[NumPartitions];

	// Create tasks for each partition.

	ProxyLOD::FTaskGroup TaskGroup;
	for (int32 i = 0; i < NumPartitions; ++i)
	{

		SimplifierMeshType& LocalMesh = PartitionedMeshArray[i];
		TArray<int32>* SeamVertexIdArray = &SeamVertexIdArrayCollection[i];
		const int32 MaxTriNumToRetain = SrcTriNumByPartition[i]; // currently not used
		const int32 MinTriNumToRetain = FMath::CeilToInt(MaxTriNumToRetain * MinFractionToRetain);

		// Create a functor with the required interface for the task group

		auto PartitionedWork = [MinTriNumToRetain, MaxTriNumToRetain, MaxFeatureCost, &LocalMesh, SeamVertexIdArray]()
		{
			ProxyLOD::FSimplifierTerminatorBase Terminator(MinTriNumToRetain, MaxFeatureCost);
			SimplifyMesh(Terminator, LocalMesh, SeamVertexIdArray);
		};

		// Enqueue the task 

		TaskGroup.Run(PartitionedWork);
	}

	// This actually triggers the running of the jobs in the task group and then waits for completion.

	TaskGroup.Wait();

	// Merge the partitioned mesh back into a single mesh.

	MergeMeshArray(PartitionedMeshArray, SeamVertexIdArrayCollection, InOutMesh);

	// Delete the array of seam verts ids

	if (SeamVertexIdArrayCollection) delete[] SeamVertexIdArrayCollection;

}

void ProxyLOD::ParallelSimplifyMesh(const FClosestPolyField& SrcReference, const float MinFractionToRatain, const float MaxFeatureCost, FAOSMesh& InOutMesh)
{
	const FRawMeshArrayAdapter& SrcMeshAdapter = SrcReference.MeshAdapter();

	const size_t SrcTriNum = SrcMeshAdapter.polygonCount();

	FBBox SrcBBox = SrcMeshAdapter.GetBBox();

	TArray<int32> SrcTriNumByPartition;

	// Testing
	int32 NumPartitions;


	NumPartitions = 6;
	{
		// Find the number of Src Tris in each partition.

		FMajorAxisPartitionFunctor PartitionFunctor(SrcBBox, NumPartitions);

		SrcTriNumByPartition.Empty(NumPartitions);
		SrcTriNumByPartition.AddZeroed(NumPartitions);

		for (int32 i = 0; i < SrcTriNum; ++i)
		{
			auto PartitionIdx = PartitionFunctor(SrcMeshAdapter.GetRawPoly(i).VertexPositions);
			SrcTriNumByPartition[PartitionIdx] += 1;
		}



		float RetainFraction = 5.f;
		PartitionAndSimplifyMesh(SrcBBox, SrcTriNumByPartition, RetainFraction, MaxFeatureCost, NumPartitions, InOutMesh);

		//	ProjectVerticesOntoSrc(SrcReference, InOutMesh);
	}

	NumPartitions = 4;
	{
		// Find the number of Src Tris in each partition.

		FMajorAxisPartitionFunctor PartitionFunctor(SrcBBox, NumPartitions);

		SrcTriNumByPartition.Empty(NumPartitions);
		SrcTriNumByPartition.AddZeroed(NumPartitions);

		for (int32 i = 0; i < SrcTriNum; ++i)
		{
			auto PartitionIdx = PartitionFunctor(SrcMeshAdapter.GetRawPoly(i).VertexPositions);
			SrcTriNumByPartition[PartitionIdx] += 1;
		}



		float RetainFraction = 1.f;
		PartitionAndSimplifyMesh(SrcBBox, SrcTriNumByPartition, RetainFraction, MaxFeatureCost, NumPartitions, InOutMesh);

		//ProjectVerticesOntoSrc(SrcReference, InOutMesh);
	}

	NumPartitions = 2;
	if (1)
	{
		// Find the number of Src Tris in each partition.

		FMajorAxisPartitionFunctor PartitionFunctor(SrcBBox, NumPartitions);

		SrcTriNumByPartition.Empty(NumPartitions);
		SrcTriNumByPartition.AddZeroed(NumPartitions);

		for (int32 i = 0; i < SrcTriNum; ++i)
		{
			auto PartitionIdx = PartitionFunctor(SrcMeshAdapter.GetRawPoly(i).VertexPositions);
			SrcTriNumByPartition[PartitionIdx] += 1;
		}



		float RetainFraction = FMath::Max(0.5f, MinFractionToRatain);
		PartitionAndSimplifyMesh(SrcBBox, SrcTriNumByPartition, RetainFraction, MaxFeatureCost, NumPartitions, InOutMesh);

		//	ProjectVerticesOntoSrc(SrcReference, InOutMesh);
	}

	/* NumPartitions = 1; */
	{
		const int32 MaxTriNumToRetain = SrcTriNum;
		const int32 MinTriNumToRetain = FMath::CeilToInt(MaxTriNumToRetain * MinFractionToRatain);

		ProxyLOD::FSimplifierTerminator Terminator(MinTriNumToRetain, MaxTriNumToRetain, MaxFeatureCost);
		SimplifyMesh(Terminator, InOutMesh);

	}
}

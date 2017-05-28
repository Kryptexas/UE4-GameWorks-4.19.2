// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "DummyMeshReconstructorModule.h"
#include "BaseMeshReconstructorModule.h"
#include "Modules/ModuleManager.h"
#include "Runnable.h"
#include "PlatformProcess.h"
#include "RunnableThread.h"
#include "ThreadSafeBool.h"
#include "MRMeshComponent.h"
#include "DynamicMeshBuilder.h"
#include "Queue.h"

static const FVector BRICK_SIZE(256.0f, 256.0f, 256.0f);
static const int32 BRICK_COORD_RANDMAX = 8;

class FDummyMeshReconstructor : public FRunnable
{
public:
	FDummyMeshReconstructor( IMRMesh& MRMeshComponent )
	: bKeepRunning(true)
	, bResendAllData(false)
	, TargetMRMesh( MRMeshComponent )
	, ReconstructorThread(nullptr)
	{
	
		// Pre-allocate the reconstructed geometry data.
		// Re-allocating this array will cause dangling pointers from MRMeshComponent.
		ReconstructedGeometry.Reserve(BRICK_COORD_RANDMAX*BRICK_COORD_RANDMAX*BRICK_COORD_RANDMAX);
		
		// Start a new thread.
		ReconstructorThread = TUniquePtr<FRunnableThread>(FRunnableThread::Create(this, TEXT("Dummy Mesh Reconstructor")));
	}

	~FDummyMeshReconstructor()
	{
		// Stop the geometry generator thread.
		if (ReconstructorThread.IsValid())
		{
			ReconstructorThread->Kill(true);
		}
	}

	/**
	 * Get the MRMeshComponent that is currently presenting our data.
	 * Used for checking against re-connects to the same component.
	 */
	const IMRMesh& GetTargetMRMesh() const { return TargetMRMesh; }

	/** Re-send all the geometry data to the paired MRMeshComponent */
	void ResendAllData()
	{
		bResendAllData.AtomicSet(true);
	}

private:
	//~ FRunnable
	virtual uint32 Run() override
	{
		//
		// Main geometry generator loop
		//

		while (bKeepRunning)
		{
			if (bResendAllData)
			{
				// The component requested that we re-send all the data.
				for (FPayload& Payload : ReconstructedGeometry)
				{
					TargetMRMesh.SendBrickData(IMRMesh::FSendBrickDataArgs
					{
						Payload.BrickCoords,
						Payload.Vertices,
						Payload.Indices
					});
				}				
				bResendAllData.AtomicSet(false);
			}

			// Send newly-generated brick.
			{
				// Allocate a new brick. We own this memory.
				const int32 NewPayloadIndex = NewRandomPayload();
				TargetMRMesh.SendBrickData(IMRMesh::FSendBrickDataArgs
				{
					ReconstructedGeometry[NewPayloadIndex].BrickCoords,
					ReconstructedGeometry[NewPayloadIndex].Vertices,
					ReconstructedGeometry[NewPayloadIndex].Indices
				});
			}

			FPlatformProcess::Sleep(1.0f/5.0f);
		}

		return 0;
	}

	virtual void Stop() override
	{
		bKeepRunning.AtomicSet(false);
	}
	//~ FRunnable

	struct FPayload
	{
		FIntVector BrickCoords;
		TArray<FDynamicMeshVertex> Vertices;
		TArray<uint32> Indices;
	};
	
	int32 NewRandomPayload()
	{
		static const int32 MIN_BOXES = 2;
		static const int32 MAX_BOXES = 20;
		static const FBox RANDOM_SIZE_BOX = FBox(FVector::ZeroVector, 0.25f*BRICK_SIZE);

		const int32 NumBoxes = FMath::FloorToInt(FMath::RandRange(MIN_BOXES, MAX_BOXES));
		const int32 NumUniqueVerts = NumBoxes * 8;
		const int32 NumTris = NumBoxes * 6 * 2; // 2 tris per box face
		const int32 NumVertIndices = 3 * NumTris;

		
		const FIntVector BrickCoords(
			FMath::FloorToInt(FMath::RandRange(0, BRICK_COORD_RANDMAX)),
			FMath::FloorToInt(FMath::RandRange(0, BRICK_COORD_RANDMAX)),
			FMath::FloorToInt(FMath::RandRange(0, BRICK_COORD_RANDMAX))
		);

		const FVector BrickOrigin(BRICK_SIZE.X*BrickCoords.X, BRICK_SIZE.Y*BrickCoords.Y, BRICK_SIZE.Z*BrickCoords.Z);
		const FBox RandomLocationsBox = FBox(BrickOrigin, BrickOrigin + FVector(1024, 1024, 1024));

		//
		// !! Allocate a new Brick. We own this data.
		//
		ReconstructedGeometry.AddDefaulted(1);
		FPayload& NewPayload = ReconstructedGeometry.Last();
		NewPayload.BrickCoords = BrickCoords;
		NewPayload.Vertices.Reserve(NumUniqueVerts);
		NewPayload.Indices.Reserve(NumVertIndices);	

		auto AddBox = [](FVector Origin, FVector Extents, FPayload& NewPayloadIn)
		{
			const int32 IndexOffset = NewPayloadIn.Vertices.Num();
			NewPayloadIn.Vertices.Emplace<FDynamicMeshVertex>(FVector(Origin.X + Extents.X, Origin.Y - Extents.Y, Origin.Z + Extents.Z));  // IndexOffset+0
			NewPayloadIn.Vertices.Last().Color = FColor(255, 0, 255);
			NewPayloadIn.Vertices.Emplace<FDynamicMeshVertex>(FVector(Origin.X + Extents.X, Origin.Y + Extents.Y, Origin.Z + Extents.Z));  // IndexOffset+1
			NewPayloadIn.Vertices.Last().Color = FColor(255, 255, 255);
			NewPayloadIn.Vertices.Emplace<FDynamicMeshVertex>(FVector(Origin.X + Extents.X, Origin.Y + Extents.Y, Origin.Z - Extents.Z));  // IndexOffset+2
			NewPayloadIn.Vertices.Last().Color = FColor(255, 255, 0);
			NewPayloadIn.Vertices.Emplace<FDynamicMeshVertex>(FVector(Origin.X + Extents.X, Origin.Y - Extents.Y, Origin.Z - Extents.Z));  // IndexOffset+3
			NewPayloadIn.Vertices.Last().Color = FColor(255, 0, 0);
			NewPayloadIn.Vertices.Emplace<FDynamicMeshVertex>(FVector(Origin.X - Extents.X, Origin.Y - Extents.Y, Origin.Z + Extents.Z));  // IndexOffset+4
			NewPayloadIn.Vertices.Last().Color = FColor(0, 0, 255);
			NewPayloadIn.Vertices.Emplace<FDynamicMeshVertex>(FVector(Origin.X - Extents.X, Origin.Y + Extents.Y, Origin.Z + Extents.Z));  // IndexOffset+5
			NewPayloadIn.Vertices.Last().Color = FColor(0, 255, 255);
			NewPayloadIn.Vertices.Emplace<FDynamicMeshVertex>(FVector(Origin.X - Extents.X, Origin.Y + Extents.Y, Origin.Z - Extents.Z));  // IndexOffset+6
			NewPayloadIn.Vertices.Last().Color = FColor(0, 255, 0);
			NewPayloadIn.Vertices.Emplace<FDynamicMeshVertex>(FVector(Origin.X - Extents.X, Origin.Y - Extents.Y, Origin.Z - Extents.Z));  // IndexOffset+7
			NewPayloadIn.Vertices.Last().Color = FColor(0, 0, 0);

			NewPayloadIn.Indices.Add(IndexOffset + 0);
			NewPayloadIn.Indices.Add(IndexOffset + 1);
			NewPayloadIn.Indices.Add(IndexOffset + 2);

			NewPayloadIn.Indices.Add(IndexOffset + 0);
			NewPayloadIn.Indices.Add(IndexOffset + 2);
			NewPayloadIn.Indices.Add(IndexOffset + 3);

			NewPayloadIn.Indices.Add(IndexOffset + 0);
			NewPayloadIn.Indices.Add(IndexOffset + 4);
			NewPayloadIn.Indices.Add(IndexOffset + 1);

			NewPayloadIn.Indices.Add(IndexOffset + 1);
			NewPayloadIn.Indices.Add(IndexOffset + 4);
			NewPayloadIn.Indices.Add(IndexOffset + 5);

			NewPayloadIn.Indices.Add(IndexOffset + 7);
			NewPayloadIn.Indices.Add(IndexOffset + 5);
			NewPayloadIn.Indices.Add(IndexOffset + 4);

			NewPayloadIn.Indices.Add(IndexOffset + 6);
			NewPayloadIn.Indices.Add(IndexOffset + 5);
			NewPayloadIn.Indices.Add(IndexOffset + 7);

			NewPayloadIn.Indices.Add(IndexOffset + 7);
			NewPayloadIn.Indices.Add(IndexOffset + 3);
			NewPayloadIn.Indices.Add(IndexOffset + 2);

			NewPayloadIn.Indices.Add(IndexOffset + 7);
			NewPayloadIn.Indices.Add(IndexOffset + 2);
			NewPayloadIn.Indices.Add(IndexOffset + 6);

			NewPayloadIn.Indices.Add(IndexOffset + 7);
			NewPayloadIn.Indices.Add(IndexOffset + 4);
			NewPayloadIn.Indices.Add(IndexOffset + 0);

			NewPayloadIn.Indices.Add(IndexOffset + 7);
			NewPayloadIn.Indices.Add(IndexOffset + 0);
			NewPayloadIn.Indices.Add(IndexOffset + 3);

			NewPayloadIn.Indices.Add(IndexOffset + 1);
			NewPayloadIn.Indices.Add(IndexOffset + 5);
			NewPayloadIn.Indices.Add(IndexOffset + 6);

			NewPayloadIn.Indices.Add(IndexOffset + 2);
			NewPayloadIn.Indices.Add(IndexOffset + 1);
			NewPayloadIn.Indices.Add(IndexOffset + 6);
		};


		for (int i = 0; i < NumBoxes; ++i)
		{
			auto wat = FVector(FMath::RandRange(5.0f, 100.0f), FMath::RandRange(5.0f, 100.0f), FMath::RandRange(5.0f, 100.0f));

			AddBox(FMath::RandPointInBox(RandomLocationsBox), FMath::RandPointInBox(RANDOM_SIZE_BOX), NewPayload);
		}

		return ReconstructedGeometry.Num() - 1;
	}

	
	FThreadSafeBool bKeepRunning;
	FThreadSafeBool bResendAllData;
	IMRMesh& TargetMRMesh;	
	TArray<FPayload> ReconstructedGeometry;
	TUniquePtr<FRunnableThread> ReconstructorThread;
};

// Thin wrapper around the running thread that does all the reconstruction.
class FDummyMeshReconstructorModule : public FBaseMeshReconstructorModule
{
private:
	virtual void PairWithComponent(IMRMesh& MRMeshComponent) override
	{
		if (const bool bAlreadyPaired = Reconstructor.IsValid() && (&Reconstructor->GetTargetMRMesh() == &MRMeshComponent) )
		{
			// We are already paired to this component, but for some reason
			// we are re-connecting (probably render proxy re-create).
			// Resend all the data so that the component can process it.
			Reconstructor->ResendAllData();
		}
		else
		{
			// We are connecting to a new component. Flush all current data.
			Reconstructor = MakeUnique<FDummyMeshReconstructor>(MRMeshComponent);
		}
	}

	virtual void Stop() override
	{
		Reconstructor.Reset();
	}

	TUniquePtr<FDummyMeshReconstructor> Reconstructor;
};

IMPLEMENT_MODULE(FDummyMeshReconstructorModule, DummyMeshReconstructor);
// Copyright 2017 Google Inc.

#pragma once

#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"
#include "CoreDelegates.h"
#include "Templates/Function.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Tickable.h"
#include "MeshReconstructorBase.h"
#include "MRMeshComponent.h"

#if PLATFORM_ANDROID
#include "tango_client_api.h"
#include "tango_support_api.h"
#include "tango_3d_reconstruction_api.h"
#endif

#include "TangoMeshReconstructor.generated.h"

struct FSectionAddress
{
	int32 X;
	int32 Y;
	int32 Z;
	friend bool operator== (const FSectionAddress& A, const FSectionAddress& B)
	{
		return A.X == B.X && A.Y == B.Y && A.Z == B.Z;
	}

	friend uint32 GetTypeHash(const FSectionAddress& Other)
	{
		return FCrc::MemCrc32(&Other, sizeof(Other));
	}
};


class FTangoMeshSection
{
public:
	FSectionAddress Address;
	int32 SectionIndex;
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals; // @TODO: get rid of this
	TArray<FColor> Colors;
	void InitVertexAttrs();
	FThreadSafeBool bUpdatingMesh;
	FOnProcessingComplete OnProcessingComplete;
	void HandleProcessingComplete()
	{
		bUpdatingMesh.AtomicSet(false);
	}
};

UCLASS(ClassGroup = (Tango), Experimental)
class TANGO_API UTangoMeshReconstructor : public UMeshReconstructorBase, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UTangoMeshReconstructor();
	virtual void Tick(float DeltaTime) override;

	virtual void StartReconstruction() override;
	virtual void PauseReconstruction() override;
	virtual void StopReconstruction() override;
	virtual void ResumeReconstruction();
	virtual bool IsReconstructionPaused() const override;
	virtual bool IsReconstructionStarted() const override;
	virtual FMRMeshConfiguration ConnectMRMesh(UMRMeshComponent* Mesh) override;
	virtual void DisconnectMRMesh() override;

	/** Mesh resolution in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango|Mesh Reconstruction")
	int32 Resolution;
	/** Whether to remove empty space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango|Mesh Reconstruction")
	bool bUseSpaceClearing;
	bool bEnabled;
	// FTickableGameObject overrides
	bool IsTickable() const override;
	TStatId GetStatId() const override;

	void RunUpdater();
	void RunExtractor();

        TArray<TSharedPtr<FTangoMeshSection>> Sections;

	void RunOnGameThread(TFunction<void()> Func)
	{
		RunOnGameThreadQueue.Enqueue(Func);
	}
private:
	TQueue<TFunction<void()>> RunOnGameThreadQueue;
	virtual void CheckBeginReconstruction();
	bool bReconstructionRequested;
	bool bPauseRequested;
	FThreadSafeBool bReconstructing;
	FThreadSafeBool bPaused;
#if PLATFORM_ANDROID

	void DoStopReconstruction();
	void UpdateMesh(const TangoPointCloud* PointCloud);
	FTransform PointCloudTransform;

	Tango3DR_Config t3dr_config_;
	Tango3DR_ReconstructionContext t3dr_context_;
	Tango3DR_CameraCalibration t3dr_intrinsics_;
	FCriticalSection Tango3DR_StateLock; // protects above state

	FCriticalSection UpdatedIndicesMutex; // protects UpdatedIndices
	TSet<FSectionAddress> UpdatedIndices;
	TMap<FSectionAddress, int32> SectionAddressMap;
	FRunnableThread* Updater;
	FRunnableThread* Extractor;
	FRunnable* UpdaterRunnable;
	FRunnable* ExtractorRunnable;
	FEvent* IndicesAvailable;

	FCriticalSection ReconstructionLock;
	void OnEnterBackground();
	void OnEnterForeground();
	FDelegateHandle OnEnterForegroundHandle;
	FDelegateHandle OnEnterBackgroundHandle;


#endif
	IMRMesh* MRMeshPtr;
};

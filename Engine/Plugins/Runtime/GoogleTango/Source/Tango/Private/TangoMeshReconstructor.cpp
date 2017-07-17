// Copyright 2017 Google Inc.

#include "TangoMeshReconstructor.h"
#include "Misc/ScopeLock.h"
#include "TangoPluginPrivate.h"
#include "TangoLifecycle.h"
#include "TangoPointCloud.h"
#include "TangoBlueprintFunctionLibrary.h"
#include "HAL/Runnable.h"
#include "TangoPluginPrivate.h"
#if PLATFORM_ANDROID
#include "tango_client_api_dynamic.h"
#endif

namespace
{


	class FMeshUpdater : public FRunnable
	{
		TWeakObjectPtr<UTangoMeshReconstructor> TargetPtr;
	public:
		FMeshUpdater(UTangoMeshReconstructor* Target) : TargetPtr(Target) {}

		uint32 Run() override
		{
			UTangoMeshReconstructor* Target = TargetPtr.Get();
			if (Target)
			{
				Target->RunUpdater();
			}
			return 0;
		}
	};

	class FMeshExtractor : public FRunnable
	{
		TWeakObjectPtr<UTangoMeshReconstructor> TargetPtr;
	public:
		FMeshExtractor(UTangoMeshReconstructor* Target) : TargetPtr(Target) {}

		uint32 Run() override
		{
			UTangoMeshReconstructor* Target = TargetPtr.Get();
			if (Target)
			{
				Target->RunExtractor();
			}
			return 0;
		}
	};

}

void FTangoMeshSection::InitVertexAttrs()
{
#if PLATFORM_ANDROID
	const int kInitialVertexCount = 100;
	const int kInitialIndexCount = 99;
	Vertices.SetNumUninitialized(kInitialVertexCount);
	Normals.SetNumUninitialized(kInitialVertexCount);
	Triangles.SetNumUninitialized(kInitialIndexCount);
#endif
	OnProcessingComplete.BindRaw(this, &FTangoMeshSection::HandleProcessingComplete);
}


TStatId UTangoMeshReconstructor::GetStatId() const
{
	static TStatId StatId;
	return StatId;
}

bool UTangoMeshReconstructor::IsTickable() const
{
	return true;
}

// Sets default values for this component's properties
UTangoMeshReconstructor::UTangoMeshReconstructor() :
	Resolution(5),
	bUseSpaceClearing(true),
	bReconstructionRequested(false),
	bPauseRequested(false),
	MRMeshPtr(nullptr)
{

#if PLATFORM_ANDROID
	t3dr_config_ = nullptr;
	t3dr_context_ = nullptr;
	Updater = nullptr;
	Extractor = nullptr;
	UpdaterRunnable = nullptr;
	ExtractorRunnable = nullptr;
	IndicesAvailable = nullptr;
#endif
	// ...
}

#if PLATFORM_ANDROID
void UTangoMeshReconstructor::OnEnterForeground()
{
	if (bReconstructing && !bPauseRequested)
	{
		bPaused.AtomicSet(false);
	}
}

#endif

#if PLATFORM_ANDROID
void UTangoMeshReconstructor::OnEnterBackground()
{
	if (bReconstructing)
	{
		bPaused.AtomicSet(true);
	}
}

#endif

bool UTangoMeshReconstructor::IsReconstructionPaused() const
{
	return bPauseRequested;
}

bool UTangoMeshReconstructor::IsReconstructionStarted() const
{
	return bReconstructionRequested;
}

void UTangoMeshReconstructor::PauseReconstruction()
{
	bPauseRequested = true;
	bPaused.AtomicSet(true);
}

void UTangoMeshReconstructor::ResumeReconstruction()
{
	bPauseRequested = false;
	bPaused.AtomicSet(false);
}

FMRMeshConfiguration UTangoMeshReconstructor::ConnectMRMesh(UMRMeshComponent* InMRMeshPtr)
{
	if (MRMeshPtr != nullptr)
	{
		UE_LOG(LogTango, Warning, TEXT("TangoMeshReconstruction already connected"));
	}
	else
	{
		MRMeshPtr = InMRMeshPtr;
	}

	FMRMeshConfiguration OutConfig;
	OutConfig.bSendVertexColors = false;
	return OutConfig;
}

void UTangoMeshReconstructor::DisconnectMRMesh()
{
	if (MRMeshPtr == nullptr)
	{
		UE_LOG(LogTango, Warning, TEXT("TangoMeshReconstructor already disconnected"));
		return;
	}
	MRMeshPtr = nullptr;
}

void UTangoMeshReconstructor::StartReconstruction()
{
	if (bReconstructionRequested)
	{
		if (bPauseRequested)
		{
			ResumeReconstruction();
		}
		return;
	}
	bReconstructionRequested = true;
#if PLATFORM_ANDROID
	bReconstructing.AtomicSet(false);
	OnEnterBackgroundHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UTangoMeshReconstructor::OnEnterBackground);
	OnEnterForegroundHandle = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &UTangoMeshReconstructor::OnEnterForeground);
#endif
}


void UTangoMeshReconstructor::CheckBeginReconstruction()
{
#if PLATFORM_ANDROID
	if (!bReconstructionRequested)
	{
		return;
	}
	if (bReconstructing)
	{
		return;
	}
	if (!UTangoBlueprintFunctionLibrary::IsTangoRunning())
	{
		return;
	}
	FScopeLock ScopeLock(&ReconstructionLock);
	Tango3DR_Status t3dr_err;
	if (t3dr_config_ == nullptr)
	{
		t3dr_config_ =
			Tango3DR_Config_create(TANGO_3DR_CONFIG_RECONSTRUCTION);
	}
	t3dr_err = Tango3DR_Config_setDouble(t3dr_config_, "resolution", Resolution / 100.0f);
	if (t3dr_err != TANGO_3DR_SUCCESS) {
		UE_LOG(LogTango, Error, TEXT("Tango3DR_Config set resolution failed with error code: %d"), t3dr_err);
		return;
	}
	t3dr_err = Tango3DR_Config_setBool(t3dr_config_, "generate_color", false);
	if (t3dr_err != TANGO_3DR_SUCCESS) {
		UE_LOG(LogTango, Error, TEXT("Tango3DR_Config  generate_color failed with error code: %d"), t3dr_err);
		return;
	}
	t3dr_err = Tango3DR_Config_setBool(t3dr_config_, "use_space_clearing", bUseSpaceClearing);
	if (t3dr_err != TANGO_3DR_SUCCESS) {
		UE_LOG(LogTango, Warning, TEXT("Tango3DR_Config  use_space_clearing failed with error code: %d"), t3dr_err);
	}

	if (t3dr_context_ == nullptr)
	{
		t3dr_context_ = Tango3DR_ReconstructionContext_create(t3dr_config_);
		if (t3dr_context_ == nullptr) {
			UE_LOG(LogTango, Error, TEXT("Tango3DR_create failed with error code: %d"), t3dr_err);
			return;
		}
	}
	// Update the camera intrinsics too.
	TangoCameraIntrinsics intrinsics;
	int32 err = TangoService_getCameraIntrinsics_dynamic(TANGO_CAMERA_COLOR, &intrinsics);
	if (err != TANGO_SUCCESS) {
		UE_LOG(LogTango, Error, TEXT(
			"TangoMeshReconstruction: Failed to get camera intrinsics with error code: %d"),
			err);
		return;
	}
	t3dr_intrinsics_.calibration_type =
		static_cast<Tango3DR_TangoCalibrationType>(intrinsics.calibration_type);
	t3dr_intrinsics_.width = intrinsics.width;
	t3dr_intrinsics_.height = intrinsics.height;
	t3dr_intrinsics_.fx = intrinsics.fx;
	t3dr_intrinsics_.fy = intrinsics.fy;
	t3dr_intrinsics_.cx = intrinsics.cx;
	t3dr_intrinsics_.cy = intrinsics.cy;
	// Configure the color intrinsics to be used with updates to the mesh.
	t3dr_err = Tango3DR_ReconstructionContext_setColorCalibration(t3dr_context_, &t3dr_intrinsics_);
	if (t3dr_context_ == nullptr)
	{
		UE_LOG(LogTango, Error, TEXT("Tango3DR_setColorCalibration failed with error code: %d"), t3dr_err);
		return;
	}
	bReconstructing.AtomicSet(true);
	IndicesAvailable = FGenericPlatformProcess::GetSynchEventFromPool();
	Updater = FRunnableThread::Create(UpdaterRunnable = new FMeshUpdater(this), TEXT("MeshUpdater"));
	UE_LOG(LogTango, Log, TEXT("Created Mesh Updater Thread"));
	Extractor = FRunnableThread::Create(ExtractorRunnable = new FMeshExtractor(this), TEXT("MeshExtractor"));
	UE_LOG(LogTango, Log, TEXT("Created Mesh Extractor Thread"));
#endif
}
#if PLATFORM_ANDROID
void UTangoMeshReconstructor::DoStopReconstruction()
{
	if (!bReconstructing)
	{
		return;
	}
	bReconstructionRequested = false;
	bReconstructing.AtomicSet(false);
	bPaused.AtomicSet(false);
	FScopeLock ScopeLock(&ReconstructionLock);
	if (IndicesAvailable != nullptr)
	{
		IndicesAvailable->Trigger(); // force the Extractor to wake up
	}
	if (Updater != nullptr)
	{
		Updater->WaitForCompletion();
		delete Updater;
		delete UpdaterRunnable;
		Updater = nullptr;
		UpdaterRunnable = nullptr;
	}
	if (Extractor != nullptr)
	{
		Extractor->WaitForCompletion();
		delete Extractor;
		Extractor = nullptr;
		ExtractorRunnable = nullptr;
	}
	if (IndicesAvailable != nullptr)
	{
		FGenericPlatformProcess::ReturnSynchEventToPool(IndicesAvailable);
		IndicesAvailable = nullptr;
	}
	if (t3dr_context_ != nullptr)
	{
		if (Tango3DR_ReconstructionContext_destroy(t3dr_context_) != TANGO_3DR_SUCCESS)
		{
			UE_LOG(LogTango, Error, TEXT("Tango3DR_destroy failed"));
		}
		else
		{
			UE_LOG(LogTango, Log, TEXT("Destroyed 3DR context"));
		}
		t3dr_context_ = nullptr;
	}
	if (t3dr_config_ != nullptr)
	{
		if (Tango3DR_Config_destroy(t3dr_config_) != TANGO_3DR_SUCCESS)
		{
			UE_LOG(LogTango, Error, TEXT("Tango3DR_Config_destroy failed"));
		}
		t3dr_config_ = nullptr;
	}
	UpdatedIndices.Empty();
	SectionAddressMap.Empty();
	Sections.Empty();
}
#endif

void UTangoMeshReconstructor::StopReconstruction()
{
	if (!bReconstructionRequested)
	{
		return;
	}
	bReconstructionRequested = false;
#if PLATFORM_ANDROID
	DoStopReconstruction();
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(OnEnterBackgroundHandle);
	OnEnterBackgroundHandle.Reset();
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(OnEnterForegroundHandle);
	OnEnterForegroundHandle.Reset();
#endif
}


// Called every frame
void UTangoMeshReconstructor::Tick(float DeltaTime)
{
	if (!bReconstructionRequested)
	{
		return;
	}
#if PLATFORM_ANDROID
	CheckBeginReconstruction();
#endif
}

void UTangoMeshReconstructor::RunExtractor()
{
#if PLATFORM_ANDROID
	TArray<FSectionAddress> UpdatedIndicesLocal;
	double LastTimestamp = 0;
	const float UnrealUnitsPerMeter = FTangoDevice::GetInstance()->GetWorldToMetersScale();
	while (bReconstructing)
	{
		if (UpdatedIndicesLocal.Num() == 0)
		{
			IndicesAvailable->Wait();
			if (!bReconstructing)
			{
				break;
			}
		}
		{
			FScopeLock ScopeLock(&UpdatedIndicesMutex);
			TSet<FSectionAddress>::TConstIterator It(this->UpdatedIndices);
			for (; It; ++It)
			{
				UpdatedIndicesLocal.Add(*It);
			}
			this->UpdatedIndices.Reset();
		}
		for (int32 i = UpdatedIndicesLocal.Num() - 1; i >= 0 && bReconstructing; --i)
		{
			const FSectionAddress& SectionAddress = UpdatedIndicesLocal[i];
			int32* SectionIndexPtr = SectionAddressMap.Find(SectionAddress);
			int32 SectionIndex;
			if (SectionIndexPtr == nullptr)
			{
				SectionIndex = Sections.Num();
				TSharedPtr<FTangoMeshSection> NewSection(new FTangoMeshSection());
				Sections.Add(NewSection);
				NewSection->SectionIndex = SectionIndex;
				NewSection->InitVertexAttrs();
				SectionAddressMap.Add(SectionAddress, SectionIndex);
			}
			else
			{
				SectionIndex = *SectionIndexPtr;
			}
			TSharedPtr<FTangoMeshSection> SectionPtr = Sections[SectionIndex];
			if (SectionPtr->bUpdatingMesh)
			{
				continue;
			}
			FTangoMeshSection& Section = *SectionPtr;
			Tango3DR_Mesh tango_mesh =
			{
				/* timestamp */ 0.0,
				/* num_vertices */ 0u,
				/* num_faces */ 0u,
				/* num_textures */ 0u,
				/* max_num_vertices */ static_cast<uint32_t>(
					Section.Vertices.Max()),
				/* max_num_faces */ static_cast<uint32_t>(
					Section.Triangles.Max() / 3),
				/* max_num_textures */ 0u,
				/* vertices */ reinterpret_cast<Tango3DR_Vector3*>(
					Section.Vertices.GetData()),
				/* faces */ reinterpret_cast<Tango3DR_Face*>(
					Section.Triangles.GetData()),
				/* normals */ reinterpret_cast<Tango3DR_Vector3*>(
					Section.Normals.GetData()),
				/* colors */ nullptr,
				/* texture_coords */ nullptr,
				/*texture_ids */ nullptr,
				/* textures */ nullptr
			};
			Tango3DR_Status err = Tango3DR_extractPreallocatedMeshSegment(
				t3dr_context_, (int*)&SectionAddress, &tango_mesh);
			if (err == TANGO_3DR_ERROR)
			{
				UE_LOG(LogTango, Error, TEXT("extractPreallocatedMeshSegment failed with error code: %d"), err);
				continue;
			}
			else if (err == TANGO_3DR_INSUFFICIENT_SPACE)
			{
				UE_LOG(LogTango, Log, TEXT("MeshSection %d: %d, %d, %d: extractPreallocatedMeshSegment ran out of space with room for %d vertices, %d indices. (%d, %d)"),
					Section.SectionIndex,
					SectionAddress.X, SectionAddress.Y, SectionAddress.Z,
					Section.Vertices.Max(),
					Section.Triangles.Max(),
					Section.Vertices.Num(),
					Section.Triangles.Num());
				int32 Num = Section.Vertices.Max();
				Section.Vertices.SetNumUninitialized(Num * 2, false);
				Section.Normals.SetNumUninitialized(Num * 2, false);
				Section.Triangles.SetNumUninitialized(Num * 6, false);
				++i;
			}
			else
			{
				int32 num_vertices = tango_mesh.num_vertices;
				if (num_vertices > 0 || bUseSpaceClearing)
				{
					int32 num_triangles = tango_mesh.num_faces * 3;
					Section.Vertices.SetNumUninitialized(num_vertices, false);
					Section.Normals.SetNumUninitialized(num_vertices, false);
					Section.Triangles.SetNumUninitialized(num_triangles, false);
					//UE_LOG(LogTango, Log, TEXT("MeshSection id: %d, Verts: %d, Tris: %d"), Section.SectionIndex, Section.Vertices.Num(), Section.Triangles.Num());
					// convert from depth camera conventions to unreal
					for (int k = 0; k < num_vertices; k++)
					{
						FVector& V = Section.Vertices[k];
						V = FVector(V.Z, V.X, -V.Y) * UnrealUnitsPerMeter;
						FVector& N = Section.Normals[k];
						N = FVector(N.Z, N.X, -N.Y);
					}
					SectionPtr->bUpdatingMesh.AtomicSet(true);
					IMRMesh* Target = MRMeshPtr;
					if (Target)
					{
						Target->SendBrickData(
							IMRMesh::FSendBrickDataArgs
						{
							*(FIntVector*)&SectionAddress,
							SectionPtr->Vertices,
							SectionPtr->Colors,
							*(TArray<uint32>*)&SectionPtr->Triangles
						},
							SectionPtr->OnProcessingComplete
						);
					}
				}
				else
				{
					if (SectionIndexPtr == nullptr)
					{
						Sections.SetNum(Sections.Num() - 1);
						SectionAddressMap.Remove(SectionAddress);
					}
				}
				UpdatedIndicesLocal.RemoveAt(i);
			}
		}
	}
	UE_LOG(LogTango, Log, TEXT("Mesh Extractor Run Completed"));
#endif

}
#if PLATFORM_ANDROID

static void extract3DRPose(const TangoPoseData* data, Tango3DR_Pose* pose)
{
	pose->translation[0] = data->translation[0];
	pose->translation[1] = data->translation[1];
	pose->translation[2] = data->translation[2];
	pose->orientation[0] = data->orientation[0];
	pose->orientation[1] = data->orientation[1];
	pose->orientation[2] = data->orientation[2];
	pose->orientation[3] = data->orientation[3];
}

static void extract3DRPose(const FTransform& Transform, Tango3DR_Pose* pose)
{
	FQuat R = Transform.GetRotation();
	FVector T = Transform.GetTranslation();
	//must convert from unreal to depth camera conventions here
	const float UnrealUnitsPerMeter = FTangoDevice::GetInstance()->GetWorldToMetersScale();
	const float Scale = 1.0f / UnrealUnitsPerMeter;
	pose->translation[0] = T.Y *Scale;
	pose->translation[1] = -T.Z *Scale;
	pose->translation[2] = T.X *Scale;
	pose->orientation[0] = R.Y;
	pose->orientation[1] = -R.Z;
	pose->orientation[2] = R.X;
	pose->orientation[3] = -R.W;
}

void UTangoMeshReconstructor::RunUpdater()
{
	double LastTimestamp = 0;
	while (bReconstructing)
	{
		if (bPaused || !UTangoBlueprintFunctionLibrary::IsTangoRunning())
		{
			FGenericPlatformProcess::Sleep(1.0 / 5);
			continue;
		}
		bool bProcessedPointCloud = false;
		FTangoDevice::GetInstance()->TangoPointCloudManager.EvalPointCloud([&](const TangoPointCloud* PointCloud) {
			if (PointCloud != nullptr && PointCloud->timestamp != LastTimestamp)
			{
				UpdateMesh(PointCloud);
				bProcessedPointCloud = true;
				LastTimestamp = PointCloud->timestamp;
			}
		});
		if (!bProcessedPointCloud)
		{
			FGenericPlatformProcess::Sleep(1.0 / 60);
		}
	}
	UE_LOG(LogTango, Log, TEXT("Mesh Updater Run Completed"));
}

void UTangoMeshReconstructor::UpdateMesh(const TangoPointCloud *PointCloud)
{
	TangoPoseData Data;
	Tango3DR_PointCloud t3dr_depth;
	t3dr_depth.timestamp = PointCloud->timestamp;
	t3dr_depth.num_points = PointCloud->num_points;
	t3dr_depth.points = reinterpret_cast<Tango3DR_Vector4*>(PointCloud->points);
	// We must go through TangoMotion::GetPoseAtTime to get the depth to world transform
	// with correct ECEF handling. Unfortunately, the 3DR api only takes a pose so we
	// have to handle the scaling and coordinate space conversion ourselves.
	// If it took a matrix we could simply pass in the result of
	// TangoPointCloud::GetRawDepthToWorldMatrix().
	Tango3DR_Pose t3dr_depth_pose;
	FTangoPose DepthToWorldPose;
	if (!FTangoDevice::GetInstance()->TangoMotionManager.GetPoseAtTime(
		ETangoReferenceFrame::CAMERA_DEPTH, PointCloud->timestamp, DepthToWorldPose, true
	))
	{
		return;
	}
	extract3DRPose(DepthToWorldPose.Pose, &t3dr_depth_pose);
	Tango3DR_GridIndexArray t3dr_updated;
	Tango3DR_Status t3dr_err =
		Tango3DR_update(t3dr_context_, &t3dr_depth, &t3dr_depth_pose, nullptr,
			nullptr, &t3dr_updated);
	if (t3dr_err != TANGO_3DR_SUCCESS)
	{
		UE_LOG(LogTango, Error, TEXT("Tango3DR_update failed with error code: %d"), t3dr_err);
		return;
	}
	{
		FScopeLock ScopeLock(&UpdatedIndicesMutex);
		const FSectionAddress* Arr = (const FSectionAddress*)&t3dr_updated.indices[0][0];
		for (int32 i = 0; i < t3dr_updated.num_indices; i++)
		{
			UpdatedIndices.Add(Arr[i]);
		}
		IndicesAvailable->Trigger();
	}
	Tango3DR_GridIndexArray_destroy(&t3dr_updated);
}


#endif

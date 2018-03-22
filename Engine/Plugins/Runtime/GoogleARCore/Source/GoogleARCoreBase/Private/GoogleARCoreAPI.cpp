// Copyright 2017 Google Inc.

#include "GoogleARCoreAPI.h"

#include "Misc/EngineVersion.h"
#include "DrawDebugHelpers.h"
#include "Templates/Casts.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#endif

namespace 
{
#if PLATFORM_ANDROID
	static const FMatrix ARCoreToUnrealTransform = FMatrix(
		FPlane(0.0f, 0.0f, -1.0f, 0.0f),
		FPlane(1.0f, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, 1.0f, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, 0.0f, 1.0f));

	static const FMatrix ARCoreToUnrealTransformInverse = ARCoreToUnrealTransform.InverseFast();

	EGoogleARCoreAPIStatus ToARCoreAPIStatus(ArStatus Status)
	{
		return static_cast<EGoogleARCoreAPIStatus>(Status);
	}

	EARTrackingState ToARTrackingState(ArTrackingState State)
	{
		switch (State)
		{
		case AR_TRACKING_STATE_PAUSED:
			return EARTrackingState::NotTracking;
		case AR_TRACKING_STATE_STOPPED:
			return EARTrackingState::StoppedTracking;
		case AR_TRACKING_STATE_TRACKING:
			return EARTrackingState::Tracking;
		}
	}

	FTransform ARCorePoseToUnrealTransform(ArPose* ArPoseHandle, const ArSession* SessionHandle, float WorldToMeterScale)
	{
		FMatrix ARCorePoseMatrix;
		ArPose_getMatrix(SessionHandle, ArPoseHandle, ARCorePoseMatrix.M[0]);
		FTransform Result = FTransform(ARCoreToUnrealTransform * ARCorePoseMatrix * ARCoreToUnrealTransformInverse);
		Result.SetLocation(Result.GetLocation() * WorldToMeterScale);

		return Result;
	}

	void UnrealTransformToARCorePose(const FTransform& UnrealTransform, const ArSession* SessionHandle, ArPose** OutARPose, float WorldToMeterScale)
	{	
		check(OutARPose);

		FMatrix UnrealPoseMatrix = UnrealTransform.ToMatrixNoScale();
		UnrealPoseMatrix.SetOrigin(UnrealPoseMatrix.GetOrigin() / WorldToMeterScale);
		FMatrix ARCorePoseMatrix = ARCoreToUnrealTransformInverse * UnrealPoseMatrix * ARCoreToUnrealTransform;

		FVector ArPosePosition = ARCorePoseMatrix.GetOrigin();
		FQuat ArPoseRotation = ARCorePoseMatrix.ToQuat();
		float ArPoseData[7] = { ArPoseRotation.X, ArPoseRotation.Y, ArPoseRotation.Z, ArPoseRotation.W, ArPosePosition.X, ArPosePosition.Y, ArPosePosition.Z };
		ArPose_create(SessionHandle, ArPoseData, OutARPose);
	}
#endif
	inline bool CheckIsSessionValid(FString TypeName, const TWeakPtr<FGoogleARCoreSession>& SessionPtr)
	{
		if (!SessionPtr.IsValid())
		{
			return false;
		}
#if PLATFORM_ANDROID
		if (SessionPtr.Pin()->GetHandle() == nullptr)
		{
			return false;
		}
#endif
		return true;
	}
}

extern "C" 
{
#if PLATFORM_ANDROID
void ArSession_reportEngineType(ArSession* session, const char* engine_type, const char* engine_version);
#endif
}

/****************************************/
/*       FGoogleARCoreAPKManager        */
/****************************************/
EGoogleARCoreAvailability FGoogleARCoreAPKManager::CheckARCoreAPKAvailability()
{
#if PLATFORM_ANDROID
	static JNIEnv* Env = FAndroidApplication::GetJavaEnv();
	static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "getApplicationContext", "()Landroid/content/Context;", false);
	static jobject ApplicationContext = FJavaWrapper::CallObjectMethod(Env, FAndroidApplication::GetGameActivityThis(), Method);
	
	ArAvailability OutAvailability = AR_AVAILABILITY_UNKNOWN_ERROR;
	ArCoreApk_checkAvailability(Env, ApplicationContext, &OutAvailability);
	
	// Use static_cast here since we already make sure the enum has the same value.
	return static_cast<EGoogleARCoreAvailability>(OutAvailability);
#endif
	return EGoogleARCoreAvailability::UnsupportedDeviceNotCapable;
}

EGoogleARCoreAPIStatus FGoogleARCoreAPKManager::RequestInstall(bool bUserRequestedInstall, EGoogleARCoreInstallStatus& OutInstallStatus)
{
	EGoogleARCoreAPIStatus Status = EGoogleARCoreAPIStatus::AR_ERROR_FATAL;
#if PLATFORM_ANDROID
	static JNIEnv* Env = FAndroidApplication::GetJavaEnv();
	static jobject ApplicationActivity = FAndroidApplication::GetGameActivityThis();
	
	ArInstallStatus OutAvailability = AR_INSTALL_STATUS_INSTALLED;
	Status = ToARCoreAPIStatus(ArCoreApk_requestInstall(Env, ApplicationActivity, bUserRequestedInstall, &OutAvailability));
	OutInstallStatus = static_cast<EGoogleARCoreInstallStatus>(OutAvailability);
#endif
	return Status;
}

/****************************************/
/*         FGoogleARCoreSession         */
/****************************************/
FGoogleARCoreSession::FGoogleARCoreSession()
	: SessionCreateStatus(EGoogleARCoreAPIStatus::AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE)
	, SessionConfig(nullptr)
	, LatestFrame(nullptr)
	, UObjectManager(nullptr)
	, CameraTextureId(0)
	, CachedWorldToMeterScale(100.0f)
	, FrameNumber(0)

{
	// Create Android ARSession handle.
	LatestFrame = new FGoogleARCoreFrame(this);
#if PLATFORM_ANDROID
	JNIEnv* Env = FAndroidApplication::GetJavaEnv();
	jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "getApplicationContext", "()Landroid/content/Context;", false);
	jobject ApplicationContext = FJavaWrapper::CallObjectMethod(Env, FAndroidApplication::GetGameActivityThis(), Method);
	check(Env);
	check(ApplicationContext);

	SessionCreateStatus = ToARCoreAPIStatus(ArSession_create(Env, ApplicationContext, &SessionHandle));
	if (SessionCreateStatus != EGoogleARCoreAPIStatus::AR_SUCCESS)
	{
		UE_LOG(LogGoogleARCoreAPI, Error, TEXT("ArSession_create returns with error: %d"), static_cast<int>(SessionCreateStatus));
		return;
	}

	ArConfig_create(SessionHandle, &ConfigHandle);
	LatestFrame->Init();

	static bool ARCoreAnalyticsReported = false;
	if (!ARCoreAnalyticsReported)
	{
		ArSession_reportEngineType(SessionHandle, "Unreal Engine", TCHAR_TO_ANSI(*FEngineVersion::Current().ToString()));
		ARCoreAnalyticsReported = true;
	}
#endif
}

FGoogleARCoreSession::~FGoogleARCoreSession()
{
	for (UARPin* Anchor : UObjectManager->AllAnchors)
	{
		Anchor->OnTrackingStateChanged(EARTrackingState::StoppedTracking);
	}

	delete LatestFrame;

#if PLATFORM_ANDROID
	if (SessionHandle != nullptr)
	{
		ArSession_destroy(SessionHandle);
		ArConfig_destroy(ConfigHandle);
	}
#endif
}

// Properties
EGoogleARCoreAPIStatus FGoogleARCoreSession::GetSessionCreateStatus()
{
	return SessionCreateStatus;
}

UGoogleARCoreUObjectManager* FGoogleARCoreSession::GetUObjectManager()
{
	return UObjectManager;
}

float FGoogleARCoreSession::GetWorldToMeterScale()
{
	return CachedWorldToMeterScale;
}
#if PLATFORM_ANDROID
ArSession* FGoogleARCoreSession::GetHandle()
{
	return SessionHandle;
}
#endif

// Session lifecycle
bool FGoogleARCoreSession::IsConfigSupported(const UARSessionConfig& Config)
{
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return false;
	}

	ArConfig* NewConfigHandle = nullptr;
	ArConfig_create(SessionHandle, &NewConfigHandle);

	ArConfig_setLightEstimationMode(SessionHandle, NewConfigHandle, static_cast<ArLightEstimationMode>(Config.GetLightEstimationMode()));
	ArConfig_setPlaneFindingMode(SessionHandle, NewConfigHandle, static_cast<ArPlaneFindingMode>(Config.GetPlaneDetectionMode()));
	ArConfig_setUpdateMode(SessionHandle, NewConfigHandle, static_cast<ArUpdateMode>(Config.GetFrameSyncMode()));

	ArStatus Status = ArSession_checkSupported(SessionHandle, NewConfigHandle);
	ArConfig_destroy(NewConfigHandle);

	return Status == ArStatus::AR_SUCCESS;
#endif
	return false;
}

EGoogleARCoreAPIStatus FGoogleARCoreSession::ConfigSession(const UARSessionConfig& Config)
{
	SessionConfig = &Config;
	EGoogleARCoreAPIStatus ConfigStatus = EGoogleARCoreAPIStatus::AR_SUCCESS;

#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return EGoogleARCoreAPIStatus::AR_ERROR_FATAL;
	}
	ArConfig_setLightEstimationMode(SessionHandle, ConfigHandle, static_cast<ArLightEstimationMode>(Config.GetLightEstimationMode()));
	ArConfig_setPlaneFindingMode(SessionHandle, ConfigHandle, static_cast<ArPlaneFindingMode>(Config.GetPlaneDetectionMode()));
	ArConfig_setUpdateMode(SessionHandle, ConfigHandle, static_cast<ArUpdateMode>(Config.GetFrameSyncMode()));

	ConfigStatus = ToARCoreAPIStatus(ArSession_configure(SessionHandle, ConfigHandle));
#endif
	return ConfigStatus;
}

EGoogleARCoreAPIStatus FGoogleARCoreSession::Resume()
{
	EGoogleARCoreAPIStatus ResumeStatus = EGoogleARCoreAPIStatus::AR_SUCCESS;
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return EGoogleARCoreAPIStatus::AR_ERROR_FATAL;
	}

	ResumeStatus = ToARCoreAPIStatus(ArSession_resume(SessionHandle));
#endif
	return ResumeStatus;
}

EGoogleARCoreAPIStatus FGoogleARCoreSession::Pause()
{
	EGoogleARCoreAPIStatus PauseStatue = EGoogleARCoreAPIStatus::AR_SUCCESS;
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return EGoogleARCoreAPIStatus::AR_ERROR_FATAL;
	}

	PauseStatue = ToARCoreAPIStatus(ArSession_pause(SessionHandle));
#endif

	for (UARPin* Anchor : UObjectManager->AllAnchors)
	{
		Anchor->OnTrackingStateChanged(EARTrackingState::NotTracking);
	}

	return PauseStatue;
}

EGoogleARCoreAPIStatus FGoogleARCoreSession::Update(float WorldToMeterScale)
{
	EGoogleARCoreAPIStatus UpdateStatus = EGoogleARCoreAPIStatus::AR_SUCCESS;
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return EGoogleARCoreAPIStatus::AR_ERROR_FATAL;
	}
	UpdateStatus = ToARCoreAPIStatus(ArSession_update(SessionHandle, LatestFrame->FrameHandle));
#endif

	CachedWorldToMeterScale = WorldToMeterScale;
	int64 LastFrameTimestamp = LatestFrame->GetCameraTimestamp();
	LatestFrame->Update(WorldToMeterScale);
	if (LastFrameTimestamp != LatestFrame->GetCameraTimestamp())
	{
		FrameNumber++;
	}

	return UpdateStatus;
}

const FGoogleARCoreFrame* FGoogleARCoreSession::GetLatestFrame()
{
	return LatestFrame;
}

uint32 FGoogleARCoreSession::GetFrameNum()
{
	return FrameNumber;
}

void FGoogleARCoreSession::SetCameraTextureId(uint32_t TextureId)
{
	CameraTextureId = TextureId;
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return;
	}
	ArSession_setCameraTextureName(SessionHandle, TextureId);
#endif
}

void FGoogleARCoreSession::SetDisplayGeometry(int Rotation, int Width, int Height)
{
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return;
	}
	ArSession_setDisplayGeometry(SessionHandle, Rotation, Width, Height);
#endif
}

// Anchors and Planes
EGoogleARCoreAPIStatus FGoogleARCoreSession::CreateARAnchor(const FTransform& TransfromInTrackingSpace, UARTrackedGeometry* TrackedGeometry, USceneComponent* ComponentToPin, FName InDebugName, UARPin*& OutAnchor)
{
	EGoogleARCoreAPIStatus AnchorCreateStatus = EGoogleARCoreAPIStatus::AR_SUCCESS;
	OutAnchor = nullptr;

#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return EGoogleARCoreAPIStatus::AR_ERROR_SESSION_PAUSED;
	}

	ArAnchor* NewAnchorHandle = nullptr;
	ArPose *PoseHandle = nullptr;
	ArPose_create(SessionHandle, nullptr, &PoseHandle);
	UnrealTransformToARCorePose(TransfromInTrackingSpace, SessionHandle, &PoseHandle, CachedWorldToMeterScale);
	if (TrackedGeometry == nullptr)
	{
		AnchorCreateStatus = ToARCoreAPIStatus(ArSession_acquireNewAnchor(SessionHandle, PoseHandle, &NewAnchorHandle));
	}
	else
	{
		ensure(TrackedGeometry->GetNativeResource() != nullptr);
		ArTrackable* TrackableHandle = reinterpret_cast<FGoogleARCoreTrackableResource*>(TrackedGeometry->GetNativeResource())->GetNativeHandle();
		
		ensure(TrackableHandle != nullptr);
		AnchorCreateStatus = ToARCoreAPIStatus(ArTrackable_acquireNewAnchor(SessionHandle, TrackableHandle, PoseHandle, &NewAnchorHandle));
	}
	ArPose_destroy(PoseHandle);

	if (AnchorCreateStatus == EGoogleARCoreAPIStatus::AR_SUCCESS)
	{
		OutAnchor = NewObject<UARPin>();
		OutAnchor->InitARPin(GetARSystem(), ComponentToPin, TransfromInTrackingSpace, TrackedGeometry, InDebugName);

		UObjectManager->AllAnchors.Add(OutAnchor);
		UObjectManager->HandleToAnchorMap.Add(NewAnchorHandle, OutAnchor);
	}
#endif
	return AnchorCreateStatus;
}

void FGoogleARCoreSession::DetachAnchor(UARPin* Anchor)
{
	if (!UObjectManager->AllAnchors.Contains(Anchor))
	{
		return;
	}

#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return;
	}

	ArAnchor* AnchorHandle = *UObjectManager->HandleToAnchorMap.FindKey(Anchor);

	ArAnchor_detach(SessionHandle, AnchorHandle);
	ArAnchor_release(AnchorHandle);

	Anchor->OnTrackingStateChanged(EARTrackingState::StoppedTracking);

	UObjectManager->HandleToAnchorMap.Remove(AnchorHandle);
#endif
	UObjectManager->AllAnchors.Remove(Anchor);
}

void FGoogleARCoreSession::GetAllAnchors(TArray<UARPin*>& OutAnchors) const
{
	OutAnchors = UObjectManager->AllAnchors;
}

void FGoogleARCoreSession::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (SessionConfig)
	{
		Collector.AddReferencedObject(SessionConfig);
	}

	if (UObjectManager)
	{
		Collector.AddReferencedObject(UObjectManager);
	}
}

/****************************************/
/*         FGoogleARCoreFrame           */
/****************************************/
FGoogleARCoreFrame::FGoogleARCoreFrame(FGoogleARCoreSession* InSession)
	: Session(InSession)
	, LatestCameraPose(FTransform::Identity)
	, LatestCameraTimestamp(0)
	, LatestCameraTrackingState(EGoogleARCoreTrackingState::StoppedTracking)
	, LatestPointCloudStatus(EGoogleARCoreAPIStatus::AR_ERROR_SESSION_PAUSED)
	, LatestImageMetadataStatus(EGoogleARCoreAPIStatus::AR_ERROR_SESSION_PAUSED)
{
}

FGoogleARCoreFrame::~FGoogleARCoreFrame()
{
#if PLATFORM_ANDROID
	if (SessionHandle != nullptr)
	{
		ArFrame_destroy(FrameHandle);
		ArPose_destroy(SketchPoseHandle);
	}
#endif
}

void FGoogleARCoreFrame::Init()
{
#if PLATFORM_ANDROID
	if (Session->GetHandle())
	{
		SessionHandle = Session->GetHandle();
		ArFrame_create(SessionHandle, &FrameHandle);
		ArPose_create(SessionHandle, nullptr, &SketchPoseHandle);
	}
#endif
}



void FGoogleARCoreFrame::Update(float WorldToMeterScale)
{
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return;
	}

	ArCamera_release(CameraHandle);
	ArFrame_acquireCamera(SessionHandle, FrameHandle, &CameraHandle);

	ArCamera_getDisplayOrientedPose(SessionHandle, CameraHandle, SketchPoseHandle);

	ArTrackingState ARCoreTrackingState;
	ArCamera_getTrackingState(SessionHandle, CameraHandle, &ARCoreTrackingState);
	LatestCameraTrackingState = static_cast<EGoogleARCoreTrackingState>(ARCoreTrackingState);

	if (LatestCameraTrackingState == EGoogleARCoreTrackingState::Tracking)
	{
		int64_t FrameTimestamp = 0;
		ArFrame_getTimestamp(SessionHandle, FrameHandle, &FrameTimestamp);
		LatestCameraPose = ARCorePoseToUnrealTransform(SketchPoseHandle, SessionHandle, WorldToMeterScale);
		LatestCameraTimestamp = FrameTimestamp;
		// Update Point Cloud
		UGoogleARCorePointCloud* LatestPointCloud = Session->GetUObjectManager()->LatestPointCloud;
		LatestPointCloud->bIsUpdated = false;
		int64 PreviousTimeStamp = LatestPointCloud->GetUpdateTimestamp();
		ArPointCloud_release(LatestPointCloud->PointCloudHandle);
		LatestPointCloud->PointCloudHandle = nullptr;
		LatestPointCloudStatus = ToARCoreAPIStatus(ArFrame_acquirePointCloud(SessionHandle, FrameHandle, &LatestPointCloud->PointCloudHandle));

		if (PreviousTimeStamp != LatestPointCloud->GetUpdateTimestamp())
		{
			LatestPointCloud->bIsUpdated = true;
		}
	}

	// Update trackable that is cached in Unreal
	ArTrackableList* TrackableListHandle = nullptr;
	ArTrackableList_create(Session->GetHandle(), &TrackableListHandle);
	ArFrame_getUpdatedTrackables(Session->GetHandle(), FrameHandle, ArTrackableType::AR_TRACKABLE_BASE_TRACKABLE, TrackableListHandle);

	int TrackableListSize = 0;
	ArTrackableList_getSize(Session->GetHandle(), TrackableListHandle, &TrackableListSize);
	for (int i = 0; i < TrackableListSize; i++)
	{
		ArTrackable* TrackableHandle = nullptr;
		ArTrackableList_acquireItem(Session->GetHandle(), TrackableListHandle, i, &TrackableHandle);
		// Note that we only update trackables that is converted to Unreal type, this makes sure we only updated
		// the trackable that user has reference to, which avoid holding reference for all trackable so that ARCore may
		// have a chance to free it.
		if (Session->GetUObjectManager()->TrackableHandleMap.Contains(TrackableHandle))
		{
			TWeakObjectPtr<UARTrackedGeometry> UETrackableObject = Session->GetUObjectManager()->TrackableHandleMap[TrackableHandle];
			if (UETrackableObject.IsValid() && UETrackableObject->GetTrackingState() != EARTrackingState::StoppedTracking)
			{
				// Updated the cached tracked geometry when it is valid.
				FGoogleARCoreTrackableResource* TrackableResource = reinterpret_cast<FGoogleARCoreTrackableResource*>(UETrackableObject->GetNativeResource());
				TrackableResource->UpdateGeometryData();
			}
		}
		ArTrackable_release(TrackableHandle);
	}
	ArTrackableList_destroy(TrackableListHandle);

	// Update Image Metadata
	ArImageMetadata_release(LatestImageMetadata);
	LatestImageMetadata = nullptr;
	LatestImageMetadataStatus = ToARCoreAPIStatus(ArFrame_acquireImageMetadata(SessionHandle, FrameHandle, &LatestImageMetadata));

	// Update Anchors
	ArAnchorList* UpdatedAnchorListHandle = nullptr;
	ArAnchorList_create(SessionHandle, &UpdatedAnchorListHandle);
	ArFrame_getUpdatedAnchors(SessionHandle, FrameHandle, UpdatedAnchorListHandle);
	int AnchorListSize = 0;
	ArAnchorList_getSize(SessionHandle, UpdatedAnchorListHandle, &AnchorListSize);

	UpdatedAnchors.Empty();
	for (int i = 0; i < AnchorListSize; i++)
	{
		ArAnchor* AnchorHandle = nullptr;
		ArAnchorList_acquireItem(SessionHandle, UpdatedAnchorListHandle, i, &AnchorHandle);

		ArTrackingState AnchorTrackingState;
		ArAnchor_getTrackingState(SessionHandle, AnchorHandle, &AnchorTrackingState);
		if (!Session->GetUObjectManager()->HandleToAnchorMap.Contains(AnchorHandle))
		{
			continue;
		}
		UARPin* AnchorObject = Session->GetUObjectManager()->HandleToAnchorMap[AnchorHandle];
		if (AnchorObject->GetTrackingState() != EARTrackingState::StoppedTracking)
		{
			AnchorObject->OnTrackingStateChanged(ToARTrackingState(AnchorTrackingState));
		}

		if (AnchorObject->GetTrackingState() == EARTrackingState::Tracking)
		{
			ArAnchor_getPose(SessionHandle, AnchorHandle, SketchPoseHandle);
			FTransform AnchorPose = ARCorePoseToUnrealTransform(SketchPoseHandle, SessionHandle, WorldToMeterScale);
			AnchorObject->OnTransformUpdated(AnchorPose);
		}
		UpdatedAnchors.Add(AnchorObject);

		ArAnchor_release(AnchorHandle);
	}
	ArAnchorList_destroy(UpdatedAnchorListHandle);
#endif
}

FTransform FGoogleARCoreFrame::GetCameraPose() const
{
	return LatestCameraPose;
}

int64 FGoogleARCoreFrame::GetCameraTimestamp() const
{
	return LatestCameraTimestamp;
}

EGoogleARCoreTrackingState FGoogleARCoreFrame::GetCameraTrackingState() const
{
	return LatestCameraTrackingState;
}

void FGoogleARCoreFrame::GetUpdatedAnchors(TArray<UARPin*>& OutUpdatedAnchors) const
{
	OutUpdatedAnchors = UpdatedAnchors;
}

void FGoogleARCoreFrame::ARLineTrace(const FVector2D& ScreenPosition, EGoogleARCoreLineTraceChannel RequestedTraceChannels, TArray<FARTraceResult>& OutHitResults) const
{
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return;
	}

	ArHitResultList *HitResultList = nullptr;
	ArHitResult *HitResultHandle = nullptr;
	ArPose *HitResultPoseHandle = nullptr;
	int32_t HitResultCount = 0;

	ArHitResultList_create(SessionHandle, &HitResultList);
	ArPose_create(SessionHandle, nullptr, &HitResultPoseHandle);

	ArFrame_hitTest(SessionHandle, FrameHandle, ScreenPosition.X, ScreenPosition.Y, HitResultList);

	ArHitResultList_getSize(SessionHandle, HitResultList, &HitResultCount);
 
	ArHitResult_create(SessionHandle, &HitResultHandle);
	for(int32_t i = 0; i < HitResultCount; i++) 
	{
		ArHitResultList_getItem(SessionHandle, HitResultList, i, HitResultHandle);

		float Distance = 0;
		ArHitResult_getDistance(SessionHandle, HitResultHandle, &Distance);
		Distance *= Session->GetWorldToMeterScale();

		ArHitResult_getHitPose(SessionHandle, HitResultHandle, HitResultPoseHandle);
		FTransform HitTransform = ARCorePoseToUnrealTransform(HitResultPoseHandle, SessionHandle, Session->GetWorldToMeterScale());
		// Apply the alignment transform to the hit test result.
		HitTransform *= Session->GetARSystem()->GetAlignmentTransform();
		
		ArTrackable* TrackableHandle = nullptr;
		ArHitResult_acquireTrackable(SessionHandle, HitResultHandle, &TrackableHandle);

		ensure(TrackableHandle != nullptr);

		ArTrackableType TrackableType = ArTrackableType::AR_TRACKABLE_NOT_VALID;
		ArTrackable_getType(SessionHandle, TrackableHandle, &TrackableType);

		// Filter the HitResult based on the requested trace channel.
		if (TrackableType == AR_TRACKABLE_POINT)
		{
			ArPoint* ARPointHandle = reinterpret_cast<ArPoint*>(TrackableHandle);
			ArPointOrientationMode OrientationMode = AR_POINT_ORIENTATION_INITIALIZED_TO_IDENTITY;
			ArPoint_getOrientationMode(SessionHandle, ARPointHandle, &OrientationMode);
			if(OrientationMode == AR_POINT_ORIENTATION_ESTIMATED_SURFACE_NORMAL && !!(RequestedTraceChannels & EGoogleARCoreLineTraceChannel::FeaturePointWithSurfaceNormal))
			{
				UARTrackedGeometry* TrackedGeometry = Session->GetUObjectManager()->GetTrackableFromHandle<UARTrackedGeometry>(TrackableHandle, Session);
				FARTraceResult UEHitResult(Session->GetARSystem(), Distance, EARLineTraceChannels::FeaturePoint, HitTransform, TrackedGeometry);
				OutHitResults.Add(UEHitResult);
				continue;
			}
			if(!!(RequestedTraceChannels & EGoogleARCoreLineTraceChannel::FeaturePoint))
			{
				UARTrackedGeometry* TrackedGeometry = Session->GetUObjectManager()->GetTrackableFromHandle<UARTrackedGeometry>(TrackableHandle, Session);
				FARTraceResult UEHitResult(Session->GetARSystem(), Distance, EARLineTraceChannels::FeaturePoint, HitTransform, TrackedGeometry);
				OutHitResults.Add(UEHitResult);
				continue;
			}
		}
		if (TrackableType == AR_TRACKABLE_PLANE)
		{
			ArPlane* PlaneHandle = reinterpret_cast<ArPlane*>(TrackableHandle);
			if (!!(RequestedTraceChannels & EGoogleARCoreLineTraceChannel::PlaneUsingBoundaryPolygon))
			{
				int32 PointInsidePolygon = 0;
				ArPlane_isPoseInPolygon(SessionHandle, PlaneHandle, HitResultPoseHandle, &PointInsidePolygon);
				if (PointInsidePolygon)
				{
					UARTrackedGeometry* TrackedGeometry = Session->GetUObjectManager()->GetTrackableFromHandle<UARTrackedGeometry>(TrackableHandle, Session);
					FARTraceResult UEHitResult(Session->GetARSystem(), Distance, EARLineTraceChannels::PlaneUsingBoundaryPolygon, HitTransform, TrackedGeometry);
					OutHitResults.Add(UEHitResult);
					continue;
				}
			}
			if (!!(RequestedTraceChannels & EGoogleARCoreLineTraceChannel::PlaneUsingExtent))
			{
				int32 PointInsideExtents = 0;
				ArPlane_isPoseInExtents(SessionHandle, PlaneHandle, HitResultPoseHandle, &PointInsideExtents);
				if (PointInsideExtents)
				{
					UARTrackedGeometry* TrackedGeometry = Session->GetUObjectManager()->GetTrackableFromHandle<UARTrackedGeometry>(TrackableHandle, Session);
					FARTraceResult UEHitResult(Session->GetARSystem(), Distance, EARLineTraceChannels::PlaneUsingExtent, HitTransform, TrackedGeometry);
					OutHitResults.Add(UEHitResult);
					continue;
				}
			}
			if (!!(RequestedTraceChannels & EGoogleARCoreLineTraceChannel::InfinitePlane))
			{
				UARTrackedGeometry* TrackedGeometry = Session->GetUObjectManager()->GetTrackableFromHandle<UARTrackedGeometry>(TrackableHandle, Session);
				FARTraceResult UEHitResult(Session->GetARSystem(), Distance, EARLineTraceChannels::GroundPlane, HitTransform, TrackedGeometry);
				OutHitResults.Add(UEHitResult);
				continue;
			}
		}
	}

	ArHitResult_destroy(HitResultHandle);
	ArPose_destroy(HitResultPoseHandle);
	ArHitResultList_destroy(HitResultList);
#endif
}

bool FGoogleARCoreFrame::IsDisplayRotationChanged() const
{
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return false;
	}

	int Result = 0;
	ArFrame_getDisplayGeometryChanged(SessionHandle, FrameHandle, &Result);
	return Result == 0 ? false : true;
#endif
	return false;
}

FMatrix FGoogleARCoreFrame::GetProjectionMatrix() const
{
	FMatrix ProjectionMatrix;

#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return ProjectionMatrix;
	}

	ArCamera_getProjectionMatrix(SessionHandle, CameraHandle, GNearClippingPlane, 100.0f, ProjectionMatrix.M[0]);

	// Unreal uses the infinite far plane project matrix.
	ProjectionMatrix.M[2][2] = 0.0f;
	ProjectionMatrix.M[2][3] = 1.0f;
	ProjectionMatrix.M[3][2] = GNearClippingPlane;
#endif
	return ProjectionMatrix;
}

void FGoogleARCoreFrame::TransformDisplayUvCoords(const TArray<float>& UvCoords, TArray<float>& OutUvCoords) const
{
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return;
	}

	OutUvCoords.SetNumZeroed(8);
	ArFrame_transformDisplayUvCoords(SessionHandle, FrameHandle, 8, UvCoords.GetData(), OutUvCoords.GetData());
#endif
}

FGoogleARCoreLightEstimate FGoogleARCoreFrame::GetLightEstimate() const
{
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return FGoogleARCoreLightEstimate();
	}

	ArLightEstimate *LightEstimateHandle = nullptr;
	ArLightEstimate_create(SessionHandle, &LightEstimateHandle);
	ArFrame_getLightEstimate(SessionHandle, FrameHandle, LightEstimateHandle);

	ArLightEstimateState LightEstimateState;
	ArLightEstimate_getState(SessionHandle, LightEstimateHandle, &LightEstimateState);

	FGoogleARCoreLightEstimate LightEstimate;
	LightEstimate.bIsValid = (LightEstimateState == AR_LIGHT_ESTIMATE_STATE_VALID) ? true : false;

	if(LightEstimate.bIsValid)
	{
		ArLightEstimate_getPixelIntensity(SessionHandle, LightEstimateHandle, &LightEstimate.PixelIntensity);
	}
	else
	{
		LightEstimate.PixelIntensity = 0.0f;
	}

	ArLightEstimate_destroy(LightEstimateHandle);

	return LightEstimate;
#else
	return FGoogleARCoreLightEstimate();
#endif
}

EGoogleARCoreAPIStatus FGoogleARCoreFrame::GetPointCloud(UGoogleARCorePointCloud*& OutLatestPointCloud) const
{
	OutLatestPointCloud = Session->GetUObjectManager()->LatestPointCloud;
	return LatestPointCloudStatus;
}

EGoogleARCoreAPIStatus FGoogleARCoreFrame::AcquirePointCloud(UGoogleARCorePointCloud*& OutLatestPointCloud) const
{
	OutLatestPointCloud = nullptr;
	EGoogleARCoreAPIStatus AcquirePointCloudStatus = EGoogleARCoreAPIStatus::AR_SUCCESS;
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return EGoogleARCoreAPIStatus::AR_ERROR_SESSION_PAUSED;
	}

	ArPointCloud* PointCloudHandle = nullptr;
	AcquirePointCloudStatus = ToARCoreAPIStatus(ArFrame_acquirePointCloud(SessionHandle, FrameHandle, &PointCloudHandle));

	if (AcquirePointCloudStatus == EGoogleARCoreAPIStatus::AR_SUCCESS)
	{
		OutLatestPointCloud = NewObject<UGoogleARCorePointCloud>();
		OutLatestPointCloud->Session = Session->AsShared();
		OutLatestPointCloud->PointCloudHandle = PointCloudHandle;
		OutLatestPointCloud->bIsUpdated = true;
	}
	else
	{
		UE_LOG(LogGoogleARCoreAPI, Error, TEXT("AcquirePointCloud failed due to resource exhausted!"));
	}
#endif
	return AcquirePointCloudStatus;
}

#if PLATFORM_ANDROID
EGoogleARCoreAPIStatus FGoogleARCoreFrame::GetCameraMetadata(const ACameraMetadata*& OutCameraMetadata) const
{
	if (SessionHandle == nullptr)
	{
		return EGoogleARCoreAPIStatus::AR_ERROR_SESSION_PAUSED;
	}

	ArImageMetadata_getNdkCameraMetadata(SessionHandle, LatestImageMetadata, &OutCameraMetadata);
	
	return LatestImageMetadataStatus;
}
#endif

TSharedPtr<FGoogleARCoreSession> FGoogleARCoreSession::CreateARCoreSession()
{
	TSharedPtr<FGoogleARCoreSession> NewSession = MakeShared<FGoogleARCoreSession>();

	UGoogleARCoreUObjectManager* UObjectManager = NewObject<UGoogleARCoreUObjectManager>();
	UObjectManager->LatestPointCloud = NewObject<UGoogleARCorePointCloud>();
	UObjectManager->LatestPointCloud->Session = NewSession;
	UObjectManager->AddToRoot();

	NewSession->UObjectManager = UObjectManager;
	return NewSession;
}


/************************************************/
/*       UGoogleARCoreTrackableResource         */
/************************************************/
#if PLATFORM_ANDROID

EARTrackingState FGoogleARCoreTrackableResource::GetTrackingState()
{
	EARTrackingState TrackingState = EARTrackingState::StoppedTracking;
	if (CheckIsSessionValid("ARCoreTrackable", Session))
	{
		ArTrackingState ARTrackingState = ArTrackingState::AR_TRACKING_STATE_STOPPED;
		ArTrackable_getTrackingState(Session.Pin()->GetHandle(), TrackableHandle, &ARTrackingState);
		TrackingState = ToARTrackingState(ARTrackingState);
	}
	return TrackingState;
}

void FGoogleARCoreTrackableResource::UpdateGeometryData()
{
	TrackedGeometry->UpdateTrackingState(GetTrackingState());
}

void FGoogleARCoreTrackableResource::ResetNativeHandle(ArTrackable* InTrackableHandle)
{
	if (TrackableHandle != nullptr)
	{
		ArTrackable_release(TrackableHandle);
	}
	TrackableHandle = InTrackableHandle;

	UpdateGeometryData();
}

void FGoogleARCoreTrackedPlaneResource::UpdateGeometryData()
{
	FGoogleARCoreTrackableResource::UpdateGeometryData();

	UARPlaneGeometry* PlaneGeometry = CastChecked<UARPlaneGeometry>(TrackedGeometry);

	if (!CheckIsSessionValid("ARCorePlane", Session) || TrackedGeometry->GetTrackingState() == EARTrackingState::StoppedTracking)
	{
		return;
	}

	TSharedPtr<FGoogleARCoreSession> SessionPtr = Session.Pin();

	FTransform LocalToTrackingTransform;
	FVector Extent = FVector::ZeroVector;

	// Get Center Transform
	ArPose* ARPoseHandle = nullptr;
	ArPose_create(SessionPtr->GetHandle(), nullptr, &ARPoseHandle);
	ArPlane_getCenterPose(SessionPtr->GetHandle(), GetPlaneHandle(), ARPoseHandle);
	LocalToTrackingTransform = ARCorePoseToUnrealTransform(ARPoseHandle, SessionPtr->GetHandle(), SessionPtr->GetWorldToMeterScale());
	ArPose_destroy(ARPoseHandle);

	// Get Plane Extents
	float ARCorePlaneExtentX = 0; // X is right vector
	float ARCorePlaneExtentZ = 0; // Z is backward vector
	ArPlane_getExtentX(SessionPtr->GetHandle(), GetPlaneHandle(), &ARCorePlaneExtentX);
	ArPlane_getExtentZ(SessionPtr->GetHandle(), GetPlaneHandle(), &ARCorePlaneExtentZ);

	// Convert OpenGL axis to Unreal axis.
	Extent = FVector(-ARCorePlaneExtentZ, ARCorePlaneExtentX, 0) * SessionPtr->GetWorldToMeterScale();

	// Update Boundary Polygon
	int PolygonSize = 0;
	ArPlane_getPolygonSize(SessionPtr->GetHandle(), GetPlaneHandle(), &PolygonSize);

	TArray<FVector> BoundaryPolygon;
	BoundaryPolygon.Empty(PolygonSize / 2);

	if (PolygonSize != 0)
	{
		TArray<float> PolygonPointsXZ;
		PolygonPointsXZ.SetNumUninitialized(PolygonSize);
		ArPlane_getPolygon(SessionPtr->GetHandle(), GetPlaneHandle(), PolygonPointsXZ.GetData());

		for (int i = 0; i < PolygonSize / 2; i++)
		{
			FVector PointInLocalSpace(-PolygonPointsXZ[2 * i + 1] * SessionPtr->GetWorldToMeterScale(), PolygonPointsXZ[2 * i] * SessionPtr->GetWorldToMeterScale(), 0.0f);
			BoundaryPolygon.Add(PointInLocalSpace);
		}
	}

	ArPlane* SubsumedByPlaneHandle = nullptr;
	ArPlane_acquireSubsumedBy(SessionPtr->GetHandle(), GetPlaneHandle(), &SubsumedByPlaneHandle);
	ArTrackable* TrackableHandle = reinterpret_cast<ArTrackable*>(SubsumedByPlaneHandle);

	UARPlaneGeometry* SubsumedByPlane = SubsumedByPlaneHandle  == nullptr? nullptr : SessionPtr->GetUObjectManager()->GetTrackableFromHandle<UARPlaneGeometry>(TrackableHandle, SessionPtr.Get());

	uint32 FrameNum = SessionPtr->GetFrameNum();
	int64 TimeStamp = SessionPtr->GetLatestFrame()->GetCameraTimestamp();

	PlaneGeometry->UpdateTrackedGeometry(SessionPtr->GetARSystem(), FrameNum, static_cast<double>(TimeStamp), LocalToTrackingTransform, SessionPtr->GetARSystem()->GetAlignmentTransform(), FVector::ZeroVector, Extent, BoundaryPolygon, SubsumedByPlane);
	PlaneGeometry->SetDebugName(FName(TEXT("ARCorePlane")));
}

void FGoogleARCoreTrackedPointResource::UpdateGeometryData()
{
	FGoogleARCoreTrackableResource::UpdateGeometryData();

	UARTrackedPoint* TrackedPoint = CastChecked<UARTrackedPoint>(TrackedGeometry);

	if (!CheckIsSessionValid("ARCoreTrackablePoint", Session) || TrackedGeometry->GetTrackingState() == EARTrackingState::StoppedTracking)
	{
		return;
	}

	TSharedPtr<FGoogleARCoreSession> SessionPtr = Session.Pin();

	ArPose* ARPoseHandle = nullptr;
	ArPose_create(SessionPtr->GetHandle(), nullptr, &ARPoseHandle);
	ArPoint_getPose(SessionPtr->GetHandle(), GetPointHandle(), ARPoseHandle);
	FTransform PointPose = ARCorePoseToUnrealTransform(ARPoseHandle, SessionPtr->GetHandle(), SessionPtr->GetWorldToMeterScale());
	// TODO: hook up Orientation valid.
	bool bIsPoseOrientationValid = false;
	ArPose_destroy(ARPoseHandle);

	uint32 FrameNum = SessionPtr->GetFrameNum();
	int64 TimeStamp = SessionPtr->GetLatestFrame()->GetCameraTimestamp();
	TrackedPoint->UpdateTrackedGeometry(SessionPtr->GetARSystem(), FrameNum, static_cast<double>(TimeStamp), PointPose, SessionPtr->GetARSystem()->GetAlignmentTransform());
	TrackedPoint->SetDebugName(FName(TEXT("ARCoreTrackedPoint")));
}
#endif

/****************************************/
/*       UGoogleARCorePointCloud        */
/****************************************/
UGoogleARCorePointCloud::~UGoogleARCorePointCloud()
{
	ReleasePointCloud();
}

int64 UGoogleARCorePointCloud::GetUpdateTimestamp()
{
	if (CheckIsSessionValid("ARCorePointCloud", Session))
	{
#if PLATFORM_ANDROID
		int64_t TimeStamp = 0;
		ArPointCloud_getTimestamp(Session.Pin()->GetHandle(), PointCloudHandle, &TimeStamp);
		return TimeStamp;
#endif
	}
	return 0;
}

bool UGoogleARCorePointCloud::IsUpdated()
{
	return bIsUpdated;
}

int UGoogleARCorePointCloud::GetPointNum()
{
	int PointNum = 0;
	if (CheckIsSessionValid("ARCorePointCloud", Session))
	{
#if PLATFORM_ANDROID
		ArPointCloud_getNumberOfPoints(Session.Pin()->GetHandle(), PointCloudHandle, &PointNum);
#endif
	}
	return PointNum;
}

void UGoogleARCorePointCloud::GetPoint(int Index, FVector& OutWorldPosition, float& OutConfidence)
{
	FVector Point = FVector::ZeroVector;
	float Confidence = 0.0;
	if (CheckIsSessionValid("ARCorePointCloud", Session))
	{
#if PLATFORM_ANDROID
		const float* PointData = nullptr;
		ArPointCloud_getData(Session.Pin()->GetHandle(), PointCloudHandle, &PointData);

		Point.Y = PointData[Index * 4];
		Point.Z = PointData[Index * 4 + 1];
		Point.X = -PointData[Index * 4 + 2];

		Point = Point * Session.Pin()->GetWorldToMeterScale();
		FTransform PointLocalTransform(Point);
		TSharedRef<FARSystemBase, ESPMode::ThreadSafe> ARSystem = Session.Pin()->GetARSystem();
		FTransform PointWorldTransform = PointLocalTransform * ARSystem->GetAlignmentTransform() * ARSystem->GetTrackingToWorldTransform();
		Point = PointWorldTransform.GetTranslation();
		Confidence = PointData[Index * 4 + 3];
#endif
	}
	OutWorldPosition = Point;
	OutConfidence = Confidence;
}

void UGoogleARCorePointCloud::ReleasePointCloud()
{
#if PLATFORM_ANDROID
	ArPointCloud_release(PointCloudHandle);
	PointCloudHandle = nullptr;
#endif
}

#if PLATFORM_ANDROID
void UGoogleARCoreUObjectManager::DumpTrackableHandleMap(const ArSession* SessionHandle)
{
	for (auto KeyValuePair : TrackableHandleMap)
	{
		ArTrackable* TrackableHandle = KeyValuePair.Key;
		TWeakObjectPtr<UARTrackedGeometry> TrackedGeometry = KeyValuePair.Value;

		ArTrackableType TrackableType = ArTrackableType::AR_TRACKABLE_NOT_VALID;
		ArTrackable_getType(SessionHandle, TrackableHandle, &TrackableType);
		ArTrackingState ARTrackingState = ArTrackingState::AR_TRACKING_STATE_STOPPED;
		ArTrackable_getTrackingState(SessionHandle, TrackableHandle, &ARTrackingState);

		UE_LOG(LogGoogleARCoreAPI, Log, TEXT("TrackableHandle - address: %p, type: 0x%x, tracking state: %d"),
			TrackableHandle, (int)TrackableType, (int)ARTrackingState);
		if (TrackedGeometry.IsValid())
		{
			UARTrackedGeometry* TrackedGeometryObj = TrackedGeometry.Get();
			FGoogleARCoreTrackableResource* NativeResource = reinterpret_cast<FGoogleARCoreTrackableResource*>(TrackedGeometryObj->GetNativeResource());
			UE_LOG(LogGoogleARCoreAPI, Log, TEXT("TrackedGeometry - NativeResource:%p, type: %s, tracking state: %d"),
				NativeResource->GetNativeHandle(), *TrackedGeometryObj->GetClass()->GetFName().ToString(), (int)TrackedGeometryObj->GetTrackingState());
		}
		else
		{
			UE_LOG(LogGoogleARCoreAPI, Log, TEXT("TrackedGeometry - InValid or Pending Kill."))
		}
	}
}
#endif

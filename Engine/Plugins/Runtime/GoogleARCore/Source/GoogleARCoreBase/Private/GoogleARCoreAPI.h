// Copyright 2017 Google Inc.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "GoogleARCoreTypes.h"
#include "GoogleARCoreSessionConfig.h"
#include "GoogleARCoreCameraImageBlitter.h"
#include "ARSessionConfig.h"

#if PLATFORM_ANDROID
#include "arcore_c_api.h"
#endif

#include "GoogleARCoreAPI.generated.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoogleARCoreAPI, Log, All);

enum class EGoogleARCoreAPIStatus : int
{
	/// The operation was successful.
	AR_SUCCESS = 0,
	
	/// One of the arguments was invalid, either null or not appropriate for the
	/// operation requested.
	AR_ERROR_INVALID_ARGUMENT = -1,
	
	/// An internal error occurred that the application should not attempt to
	/// recover from.
	AR_ERROR_FATAL = -2,
	
	/// An operation was attempted that requires the session be running, but the
	/// session was paused.
	AR_ERROR_SESSION_PAUSED = -3,
	
	/// An operation was attempted that requires the session be paused, but the
	/// session was running.
	AR_ERROR_SESSION_NOT_PAUSED = -4,
	
	/// An operation was attempted that the session be in the TRACKING state,
	/// but the session was not.
	AR_ERROR_NOT_TRACKING = -5,
	
	/// A texture name was not set by calling ArSession_setCameraTextureName()
	/// before the first call to ArSession_update()
	AR_ERROR_TEXTURE_NOT_SET = -6,
	
	/// An operation required GL context but one was not available.
	AR_ERROR_MISSING_GL_CONTEXT = -7,
	
	/// The configuration supplied to ArSession_configure() was unsupported.
	/// To avoid this error, ensure that Session_checkSupported() returns true.
	AR_ERROR_UNSUPPORTED_CONFIGURATION = -8,
	
	/// The android camera permission has not been granted prior to calling
	/// ArSession_resume()
	AR_ERROR_CAMERA_PERMISSION_NOT_GRANTED = -9,
	
	/// Acquire failed because the object being acquired is already released.
	/// For example, this happens if the application holds an ::ArFrame beyond
	/// the next call to ArSession_update(), and then tries to acquire its point
	/// cloud.
	AR_ERROR_DEADLINE_EXCEEDED = -10,
	
	/// There are no available resources to complete the operation.  In cases of
	/// @c acquire methods returning this error, This can be avoided by
	/// releasing previously acquired objects before acquiring new ones.
	AR_ERROR_RESOURCE_EXHAUSTED = -11,
	
	/// Acquire failed because the data isn't available yet for the current
	/// frame. For example, acquire the image metadata may fail with this error
	/// because the camera hasn't fully started.
	AR_ERROR_NOT_YET_AVAILABLE = -12,
	
	/// The android camera has been reallocated to a higher priority app or is
	/// otherwise unavailable.
	AR_ERROR_CAMERA_NOT_AVAILABLE = -13,
	
	/// The ARCore APK is not installed on this device.
	AR_UNAVAILABLE_ARCORE_NOT_INSTALLED = -100,
	
	/// The device is not currently compatible with ARCore.
	AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE = -101,
	
	/// The ARCore APK currently installed on device is too old and needs to be
	/// updated.
	AR_UNAVAILABLE_APK_TOO_OLD = -103,
	
	/// The ARCore APK currently installed no longer supports the ARCore SDK
	/// that the application was built with.
	AR_UNAVAILABLE_SDK_TOO_OLD = -104,
	
	/// The user declined installation of the ARCore APK during this run of the
	/// application and the current request was not marked as user-initiated.
	AR_UNAVAILABLE_USER_DECLINED_INSTALLATION = -105
};

#if PLATFORM_ANDROID
static ArTrackableType GetTrackableType(UClass* ClassType)
{
	if (ClassType == UARTrackedGeometry::StaticClass())
	{
		return ArTrackableType::AR_TRACKABLE_BASE_TRACKABLE;
	}
	else if (ClassType == UARPlaneGeometry::StaticClass())
	{
		return ArTrackableType::AR_TRACKABLE_PLANE;
	}
	else if (ClassType == UARTrackedPoint::StaticClass())
	{
		return ArTrackableType::AR_TRACKABLE_POINT;
	}
	else
	{
		return ArTrackableType::AR_TRACKABLE_NOT_VALID;
	}
}
#endif

class FGoogleARCoreFrame;
class FGoogleARCoreSession;

UCLASS()
class UGoogleARCoreUObjectManager : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<UARPin*> AllAnchors;

	UPROPERTY()
	UGoogleARCorePointCloud* LatestPointCloud;

#if PLATFORM_ANDROID
	TMap<ArAnchor*, UARPin*> HandleToAnchorMap;
	TMap<ArTrackable*, TWeakObjectPtr<UARTrackedGeometry>> TrackableHandleMap;

	template< class T > T* GetTrackableFromHandle(ArTrackable* TrackableHandle, FGoogleARCoreSession* Session);

	void DumpTrackableHandleMap(const ArSession* SessionHandle);
#endif
};

class FGoogleARCoreAPKManager
{
public:
	static EGoogleARCoreAvailability CheckARCoreAPKAvailability();
	static EGoogleARCoreAPIStatus RequestInstall(bool bUserRequestedInstall, EGoogleARCoreInstallStatus& OutInstallStatus);
};


class FGoogleARCoreSession : public TSharedFromThis<FGoogleARCoreSession>, public FGCObject
{

public:
	static TSharedPtr<FGoogleARCoreSession> CreateARCoreSession();

	FGoogleARCoreSession();
	~FGoogleARCoreSession();

	// Properties
	EGoogleARCoreAPIStatus GetSessionCreateStatus();
	UGoogleARCoreUObjectManager* GetUObjectManager();
	float GetWorldToMeterScale();
	void SetARSystem(TSharedRef<FARSystemBase, ESPMode::ThreadSafe> InArSystem) { ARSystem = InArSystem; }
	TSharedRef<FARSystemBase, ESPMode::ThreadSafe> GetARSystem() { return ARSystem.ToSharedRef(); }
#if PLATFORM_ANDROID
	ArSession* GetHandle();
#endif

	// Lifecycle
	bool IsConfigSupported(const UARSessionConfig& Config);
	EGoogleARCoreAPIStatus ConfigSession(const UARSessionConfig& Config);
	EGoogleARCoreAPIStatus Resume();
	EGoogleARCoreAPIStatus Pause();
	EGoogleARCoreAPIStatus Update(float WorldToMeterScale);
	const FGoogleARCoreFrame* GetLatestFrame();
	uint32 GetFrameNum();

	void SetCameraTextureId(uint32_t TextureId);
	void SetDisplayGeometry(int Rotation, int Width, int Height);

	// Anchor API
	EGoogleARCoreAPIStatus CreateARAnchor(const FTransform& TransfromInTrackingSpace, UARTrackedGeometry* TrackedGeometry, USceneComponent* ComponentToPin, FName InDebugName, UARPin*& OutAnchor);
	void DetachAnchor(UARPin* Anchor);

	void GetAllAnchors(TArray<UARPin*>& OutAnchors) const;
	template< class T > void GetAllTrackables(TArray<T*>& OutARCoreTrackableList);
private:
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	EGoogleARCoreAPIStatus SessionCreateStatus;
	const UARSessionConfig* SessionConfig;
	FGoogleARCoreFrame* LatestFrame;
	UGoogleARCoreUObjectManager* UObjectManager;
	uint32_t CameraTextureId;
	float CachedWorldToMeterScale;
	uint32 FrameNumber;

	TSharedPtr<FARSystemBase, ESPMode::ThreadSafe> ARSystem;

#if PLATFORM_ANDROID
	ArSession* SessionHandle = nullptr;
	ArConfig* ConfigHandle = nullptr;
#endif
};

class FGoogleARCoreFrame
{
	friend class FGoogleARCoreSession;

public:
	FGoogleARCoreFrame(FGoogleARCoreSession* Session);
	~FGoogleARCoreFrame();
	
	void Init();

	void Update(float WorldToMeterScale);

	FTransform GetCameraPose() const;
	int64 GetCameraTimestamp() const;
	EGoogleARCoreTrackingState GetCameraTrackingState() const;

	void GetUpdatedAnchors(TArray<UARPin*>& OutUpdatedAnchors) const;
	template< class T > void GetUpdatedTrackables(TArray<T*>& OutARCoreTrackableList) const;
	
	void ARLineTrace(const FVector2D& ScreenPosition, EGoogleARCoreLineTraceChannel RequestedTraceChannels, TArray<FARTraceResult>& OutHitResults) const;

	bool IsDisplayRotationChanged() const;
	FMatrix GetProjectionMatrix() const;
	void TransformDisplayUvCoords(const TArray<float>& UvCoords, TArray<float>& OutUvCoords) const;

	FGoogleARCoreLightEstimate GetLightEstimate() const;
	EGoogleARCoreAPIStatus GetPointCloud(UGoogleARCorePointCloud*& OutLatestPointCloud) const;
	EGoogleARCoreAPIStatus AcquirePointCloud(UGoogleARCorePointCloud*& OutLatestPointCloud) const;
#if PLATFORM_ANDROID
	EGoogleARCoreAPIStatus GetCameraMetadata(const ACameraMetadata*& OutCameraMetadata) const;
#endif

private:
	FGoogleARCoreSession* Session;
	FTransform LatestCameraPose;
	int64 LatestCameraTimestamp;
	EGoogleARCoreTrackingState LatestCameraTrackingState;

	EGoogleARCoreAPIStatus LatestPointCloudStatus;
	EGoogleARCoreAPIStatus LatestImageMetadataStatus;

	TArray<UARPin*> UpdatedAnchors;

#if PLATFORM_ANDROID
	const ArSession* SessionHandle = nullptr;
	ArFrame* FrameHandle = nullptr;
	ArCamera* CameraHandle = nullptr;
	ArPose* SketchPoseHandle = nullptr;
	ArImageMetadata* LatestImageMetadata = nullptr;
#endif
};

class FGoogleARCoreTrackableResource : public IARRef
{
public:
	// IARRef interface
	virtual void AddRef() override { }

	virtual void RemoveRef() override
	{
#if PLATFORM_ANDROID
		ArTrackable_release(TrackableHandle);
		TrackableHandle = nullptr;
#endif
	}

#if PLATFORM_ANDROID
public:
	FGoogleARCoreTrackableResource(TSharedPtr<FGoogleARCoreSession> InSession, ArTrackable* InTrackableHandle, UARTrackedGeometry* InTrackedGeometry)
		: Session(InSession)
		, TrackableHandle(InTrackableHandle)
		, TrackedGeometry(InTrackedGeometry)
	{
		ensure(TrackableHandle != nullptr);
	}

	virtual ~FGoogleARCoreTrackableResource()
	{
		ArTrackable_release(TrackableHandle);
		TrackableHandle = nullptr;	
	}

	EARTrackingState GetTrackingState();

	virtual void UpdateGeometryData();

	TWeakPtr<FGoogleARCoreSession> GetSession() { return Session; }
	ArTrackable* GetNativeHandle() { return TrackableHandle; }
	
	void ResetNativeHandle(ArTrackable* InTrackableHandle);

protected:
	TWeakPtr<FGoogleARCoreSession> Session;
	ArTrackable* TrackableHandle;
	UARTrackedGeometry* TrackedGeometry;
#endif
};

class FGoogleARCoreTrackedPlaneResource : public FGoogleARCoreTrackableResource
{
public:
#if PLATFORM_ANDROID
	FGoogleARCoreTrackedPlaneResource(TSharedPtr<FGoogleARCoreSession> InSession, ArTrackable* InTrackableHandle, UARTrackedGeometry* InTrackedGeometry)
		: FGoogleARCoreTrackableResource(InSession, InTrackableHandle, InTrackedGeometry)
	{
		ensure(TrackableHandle != nullptr);
	}

	void UpdateGeometryData() override;

	ArPlane* GetPlaneHandle() { return reinterpret_cast<ArPlane*>(TrackableHandle); }
#endif
};

class FGoogleARCoreTrackedPointResource : public FGoogleARCoreTrackableResource
{
public:
#if PLATFORM_ANDROID
	FGoogleARCoreTrackedPointResource(TSharedPtr<FGoogleARCoreSession> InSession, ArTrackable* InTrackableHandle, UARTrackedGeometry* InTrackedGeometry)
		: FGoogleARCoreTrackableResource(InSession, InTrackableHandle, InTrackedGeometry)
	{
		ensure(TrackableHandle != nullptr);
	}

	void UpdateGeometryData() override;

	ArPoint* GetPointHandle() { return reinterpret_cast<ArPoint*>(TrackableHandle); }
#endif
};

#if PLATFORM_ANDROID
// Template function definition
template< class T >
T* UGoogleARCoreUObjectManager::GetTrackableFromHandle(ArTrackable* TrackableHandle, FGoogleARCoreSession* Session)
{
	if (!TrackableHandleMap.Contains(TrackableHandle) 
		|| !TrackableHandleMap[TrackableHandle].IsValid()
		|| TrackableHandleMap[TrackableHandle]->GetTrackingState() == EARTrackingState::StoppedTracking)
	{
		// Add the trackable to the cache.
		UARTrackedGeometry* NewTrackableObject = nullptr;
		ArTrackableType TrackableType = ArTrackableType::AR_TRACKABLE_NOT_VALID;
		ArTrackable_getType(Session->GetHandle(), TrackableHandle, &TrackableType);
		IARRef* NativeResource = nullptr;
		if (TrackableType == ArTrackableType::AR_TRACKABLE_PLANE)
		{
			UARPlaneGeometry* PlaneObject = NewObject<UARPlaneGeometry>();
			NewTrackableObject = static_cast<UARTrackedGeometry*>(PlaneObject);
			NativeResource = new FGoogleARCoreTrackedPlaneResource(Session->AsShared(), TrackableHandle, NewTrackableObject);
		}
		else if (TrackableType == ArTrackableType::AR_TRACKABLE_POINT)
		{
			UARTrackedPoint* PointObject = NewObject<UARTrackedPoint>();
			NewTrackableObject = static_cast<UARTrackedGeometry*>(PointObject);
			NativeResource = new FGoogleARCoreTrackedPointResource(Session->AsShared(), TrackableHandle, NewTrackableObject);
		}

		// We should have a valid trackable object now.
		checkf(NewTrackableObject, TEXT("Unknow ARCore Trackable Type: %d"), TrackableType);
		
		NewTrackableObject->InitializeNativeResource(NativeResource);
		NativeResource = nullptr;

		FGoogleARCoreTrackableResource* TrackableResource = reinterpret_cast<FGoogleARCoreTrackableResource*>(NewTrackableObject->GetNativeResource());
		
		// Update the tracked geometry data using the native resource
		TrackableResource->UpdateGeometryData();
		ensure(TrackableResource->GetTrackingState() != EARTrackingState::StoppedTracking);

		TrackableHandleMap.Add(TrackableHandle, TWeakObjectPtr<UARTrackedGeometry>(NewTrackableObject));
	}
	else
	{
		// If we are not create new trackable object, release the trackable handle.
		ArTrackable_release(TrackableHandle);
	}

	T* Result = Cast<T>(TrackableHandleMap[TrackableHandle].Get());
	checkf(Result, TEXT("UGoogleARCoreUObjectManager failed to get a valid trackable %p from the map."), TrackableHandle);
	return Result;
}
#endif

template< class T >
void FGoogleARCoreFrame::GetUpdatedTrackables(TArray<T*>& OutARCoreTrackableList) const
{
	OutARCoreTrackableList.Empty();
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return;
	}

	ArTrackableType TrackableType = GetTrackableType(T::StaticClass());
	if (TrackableType == ArTrackableType::AR_TRACKABLE_NOT_VALID)
	{
		UE_LOG(LogGoogleARCoreAPI, Error, TEXT("Invalid Trackable type: %s"), *T::StaticClass()->GetName());
		return;
	}

	ArTrackableList* TrackableListHandle = nullptr;
	ArTrackableList_create(Session->GetHandle(), &TrackableListHandle);
	ArFrame_getUpdatedTrackables(Session->GetHandle(), FrameHandle, TrackableType, TrackableListHandle);

	int TrackableListSize = 0;
	ArTrackableList_getSize(Session->GetHandle(), TrackableListHandle, &TrackableListSize);

	for (int i = 0; i < TrackableListSize; i++)
	{
		ArTrackable* TrackableHandle = nullptr;
		ArTrackableList_acquireItem(Session->GetHandle(), TrackableListHandle, i, &TrackableHandle);
		T* TrackableObject = Session->GetUObjectManager()->template GetTrackableFromHandle<T>(TrackableHandle, Session);

		OutARCoreTrackableList.Add(TrackableObject);
	}
	ArTrackableList_destroy(TrackableListHandle);
#endif
}

template< class T >
void FGoogleARCoreSession::GetAllTrackables(TArray<T*>& OutARCoreTrackableList)
{
	OutARCoreTrackableList.Empty();
#if PLATFORM_ANDROID
	if (SessionHandle == nullptr)
	{
		return;
	}

	ArTrackableType TrackableType = GetTrackableType(T::StaticClass());
	if (TrackableType == ArTrackableType::AR_TRACKABLE_NOT_VALID)
	{
		UE_LOG(LogGoogleARCoreAPI, Error, TEXT("Invalid Trackable type: %s"), *T::StaticClass()->GetName());
		return;
	}

	ArTrackableList* TrackableListHandle = nullptr;
	ArTrackableList_create(SessionHandle, &TrackableListHandle);
	ArSession_getAllTrackables(SessionHandle, TrackableType, TrackableListHandle);

	int TrackableListSize = 0;
	ArTrackableList_getSize(SessionHandle, TrackableListHandle, &TrackableListSize);

	for (int i = 0; i < TrackableListSize; i++)
	{
		ArTrackable* TrackableHandle = nullptr;
		ArTrackableList_acquireItem(SessionHandle, TrackableListHandle, i, &TrackableHandle);

		T* TrackableObject = UObjectManager->template GetTrackableFromHandle<T>(TrackableHandle, this);
		OutARCoreTrackableList.Add(TrackableObject);
	}
	ArTrackableList_destroy(TrackableListHandle);
#endif
}

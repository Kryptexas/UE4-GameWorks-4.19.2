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
	AR_SUCCESS = 0,
	// Invalid argument handling: null pointers and invalid enums for void
	// functions are handled by logging and returning best-effort value.
	// Non-void functions additionally return AR_ERROR_INVALID_ARGUMENT.
	AR_ERROR_INVALID_ARGUMENT = -1,
	AR_ERROR_FATAL = -2,
	AR_ERROR_SESSION_PAUSED = -3,
	AR_ERROR_SESSION_NOT_PAUSED = -4,
	AR_ERROR_NOT_TRACKING = -5,
	AR_ERROR_TEXTURE_NOT_SET = -6,
	AR_ERROR_MISSING_GL_CONTEXT = -7,
	AR_ERROR_UNSUPPORTED_CONFIGURATION = -8,
	AR_ERROR_CAMERA_PERMISSION_NOT_GRANTED = -9,

	// Acquire failed because the object being acquired is already released.
	// This happens e.g. if the developer holds an old frame for too long, and
	// then tries to acquire a point cloud from it.
	AR_ERROR_DEADLINE_EXCEEDED = -10,

	// Acquire failed because there are too many objects already acquired. For
	// example, the developer may acquire up to N point clouds.
	// N is determined by available resources, and is usually small, e.g. 8.
	// If the developer tries to acquire N+1 point clouds without releasing the
	// previously acquired ones, they will get this error.
	AR_ERROR_RESOURCE_EXHAUSTED = -11,

	// Acquire failed because the data isn't available yet for the current
	// frame. For example, acquire the image metadata may fail with this error
	// because the camera hasn't fully started.
	AR_ERROR_NOT_YET_AVAILABLE = -12,

	AR_UNAVAILABLE_ARCORE_NOT_INSTALLED = -100,
	AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE = -101,
	AR_UNAVAILABLE_ANDROID_VERSION_NOT_SUPPORTED = -102,
	// The ARCore APK currently installed on device is too old and needs to be
	// updated. For example, SDK v2.0.0 when APK is v1.0.0.
	AR_UNAVAILABLE_APK_TOO_OLD = -103,
	// The ARCore APK currently installed no longer supports the sdk that the
	// app was built with. For example, SDK v1.0.0 when APK includes support for
	// v2.0.0+.
	AR_UNAVAILABLE_SDK_TOO_OLD = -104,
};

UENUM(BlueprintType)
enum class EGoogleARCoreInstallStatusInternal : uint8
{
	/* The requested resource is already installed.*/
	Installed = 0,
	/* Installation of the resource was requested. The current activity will be paused. */
	Requrested = 1,
	/* Cannot start the install request because the platform is not supported. */
	NotAvailable = 200,
};

enum class EGoogleARCoreAvailabilityInternal : uint8
{
	/* An internal error occurred while determining ARCore availability. */
	UnkownError = 0,
	/* ARCore is not installed, and a query has been issued to check if ARCore is is supported. */
	UnkownChecking = 1,
	/*
	* ARCore is not installed, and the query to check if ARCore is supported
	* timed out. This may be due to the device being offline.
	*/
	UnkownTimedOut = 2,
	/* ARCore is not supported on this device.*/
	UnsupportedDeviceNotCapable = 100,
	/* The device and Android version are supported, but the ARCore APK is not installed.*/
	SupportedNotInstalled = 201,
	/*
	* The device and Android version are supported, and a version of the
	* ARCore APK is installed, but that ARCore APK version is too old.
	*/
	SupportedApkTooOld = 202,
	/* ARCore is supported, installed, and available to use. */
	SupportedInstalled = 203
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
	static EGoogleARCoreAvailabilityInternal CheckARCoreAPKAvailability();
	static EGoogleARCoreAPIStatus RequestInstall(bool bUserRequestedInstall, EGoogleARCoreInstallStatusInternal& OutInstallStatus);
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
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ARTypes.generated.h"

class FARSystemBase;
class USceneComponent;
class IXRTrackingSystem;
class UARPin;
class UARTrackedGeometry;
class UARLightEstimate;
struct FARTraceResult;

UENUM(BlueprintType, Category="AR AugmentedReality", meta=(Experimental))
enum class EARTrackingState : uint8
{
	/** Currently not tracking. */
	Tracking,
	
	/** Currently not tracking, but may resume tracking later. */
	NotTracking,
	
	/** Stopped tracking forever. */
	StoppedTracking,
	
};


/**
 * Channels that let users select which kind of tracked geometry to trace against.
 */
UENUM( BlueprintType, Category="AR AugmentedReality|Trace Result", meta=(Bitflags) )
enum class EARLineTraceChannels : uint8
{
	None = 0,
	
	/** Trace against points that the AR system considers significant . */
	FeaturePoint = 1,
	
	/** Trace against estimated plane that does not have an associated tracked geometry. */
	GroundPlane = 2,
	
	/** Trace against any plane tracked geometries using Center and Extent. */
	PlaneUsingExtent = 4,
	
	/** Trace against any plane tracked geometries using the boundary polygon. */
	PlaneUsingBoundaryPolygon = 8
};
ENUM_CLASS_FLAGS(EARLineTraceChannels);



UENUM(BlueprintType, Category="AR AugmentedReality", meta=(Experimental))
enum class EARTrackingQuality : uint8
{
	/** The tracking quality is not available. */
	NotTracking,
	
	/** The tracking quality is limited, relying only on the device's motion. */
	OrientationOnly,
	
	/** The tracking quality is good. */
	OrientationAndPosition
};

/**
 * Describes the current status of the AR session.
 */
UENUM(BlueprintType)
enum class EARSessionStatus : uint8
{
	/** Unreal AR session has not started yet.*/
	NotStarted,
	/** Unreal AR session is running. */
	Running,
	/** Unreal AR session failed to start due to the AR subsystem not being supported by the device. */
	NotSupported,
	/** The AR session encountered fatal error; the developer should call `StartARSession()` to re-start the AR subsystem. */
	FatalError,
	/** AR session failed to start because it lacks the necessary permission (likely access to the camera or the gyroscope). */
	PermissionNotGranted,
	/** AR session failed to start because the configuration isn't supported. */
	UnsupportedConfiguration,
	/** Session isn't running due to unknown reason; @see FARSessionStatus::AdditionalInfo for more information */
	Other,
};

/** The current state of the AR subsystem including an optional explanation string. */
USTRUCT(BlueprintType)
struct AUGMENTEDREALITY_API FARSessionStatus
{
public:
	
	GENERATED_BODY()

	FARSessionStatus()
	:FARSessionStatus(EARSessionStatus::Other)
	{}

	FARSessionStatus(EARSessionStatus InStatus, FString InExtraInfo = FString())
		: AdditionalInfo(InExtraInfo)
		, Status(InStatus)
	{

	}
	
	/** Optional information about the current status of the system. */
	UPROPERTY(BlueprintReadOnly, Category="AR AugmentedReality|Session")
	FString AdditionalInfo;

	/** The current status of the AR subsystem. */
	UPROPERTY(BlueprintReadOnly, Category = "AR AugmentedReality|Session")
	EARSessionStatus Status;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnARTrackingStateChanged, EARTrackingState, NewTrackingState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnARTransformUpdated, const FTransform&, OldToNewTransform );

UCLASS()
class UARTypesDummyClass : public UObject
{
	GENERATED_BODY()
};

/** A reference to a system-level AR object  */
class IARRef
{
public:
	virtual void AddRef() = 0;
	virtual void RemoveRef() = 0;
public:
	virtual ~IARRef() {}

};

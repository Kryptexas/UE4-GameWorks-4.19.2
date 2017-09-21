#pragma once

// AppleARKit
#include "AppleARKitSessionConfiguration.generated.h"

#if ARKIT_SUPPORT
#import <ARKit/ARKit.h>
#endif // ARKIT_SUPPORT

/**
 * Enum constants for indicating the world alignment.
 */
UENUM( BlueprintType, Category="AppleARKit" )
enum class EAppleARKitWorldAlignment : uint8
{
    /** Aligns the world with gravity that is defined by vector (0, -1, 0). */
    Gravity,
    
    /** 
     * Aligns the world with gravity that is defined by the vector (0, -1, 0)
     * and heading (w.r.t. True North) that is given by the vector (0, 0, -1). 
     */
    GravityAndHeading,
    
    /** Aligns the world with the camera's orientation. */
    Camera
};

/**
 Option set indicating the type of planes to detect.
 */
UENUM( BlueprintType, Category="AppleARKit", meta=(Bitflags) )
enum class EAppleARKitPlaneDetection : uint8
{
    /** No plane detection is run. */
    None        = 1,
    
    /** Plane detection determines horizontal planes in the scene. */
    Horizontal  = 2
};
ENUM_CLASS_FLAGS(EAppleARKitPlaneDetection);

/**
 * An object to describe and configure the Augmented Reality techniques to be used in a 
 * UAppleARKitSession.
 */
UCLASS( Config=Game, BlueprintType, Category="AppleARKit" )
class APPLEARKIT_API UAppleARKitSessionConfiguration : public UObject
{
    GENERATED_BODY()

public: 

    /**
     * The alignment that transforms will be with respect to.
     * @discussion The default is ARWorldAlignmentGravity.
     */
    UPROPERTY( Config, EditAnywhere, Category="AppleARKit|Session Configuration" )
    EAppleARKitWorldAlignment Alignment = EAppleARKitWorldAlignment::Gravity;

    /**
     * Enable or disable light estimation.
     * @discussion Enabled by default.
     */
    UPROPERTY( Config, EditAnywhere, Category="AppleARKit|Session Configuration" )
    bool bLightEstimationEnabled = true;
};

/**
 * A session configuration for world tracking.
 * 
 * World tracking provides 6 degrees of freedom tracking of the device.
 * By finding feature points in the scene, world tracking enables performing hit-tests against the frame.
 * Tracking can no longer be resumed once the session is paused.
 */
UCLASS( Config=Game, BlueprintType, Category="AppleARKit" )
class APPLEARKIT_API UAppleARKitWorldTrackingSessionConfiguration : public UAppleARKitSessionConfiguration
{
    GENERATED_BODY()

public:

    /**
     * Enable or disable light estimation.
     * @discussion Enabled by default.
     */
    UPROPERTY( Config, EditAnywhere, Category="AppleARKit|World Tracking Session Configuration", Meta = (Bitmask, BitmaskEnum = "EAppleARKitPlaneDetection") )
    int32 PlaneDetection = (int32)(EAppleARKitPlaneDetection::Horizontal);
};

#if ARKIT_SUPPORT

/** Conversion function to ARKit native ARWorldAlignment */
ARWorldAlignment ToARWorldAlignment( const EAppleARKitWorldAlignment& WorldAlignment );

/** Conversion function to ARKit native ARPlaneDetection */
ARPlaneDetection ToARPlaneDetection( const EAppleARKitPlaneDetection& PlaneDetection );

/** Conversion function to ARKit native ARSessionConfiguration */
ARConfiguration* ToARSessionConfiguration( const UAppleARKitSessionConfiguration* SessionConfiguration );

/** Conversion function to ARKit native ARWorldTrackingSessionConfiguration */
ARWorldTrackingConfiguration* ToARWorldTrackingSessionConfiguration( const UAppleARKitWorldTrackingSessionConfiguration* SessionConfiguration );

#endif // ARKIT_SUPPORT

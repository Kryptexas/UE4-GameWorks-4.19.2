#pragma once

// UE4
#include "Math/Transform.h"
#include "Math/UnrealMathUtility.h"
#include "UnrealEngine.h"
#include "Engine/GameViewportClient.h"

// ARKit
#if ARKIT_SUPPORT
#import <ARKit/ARKit.h>
#endif // ARKIT_SUPPORT


enum class EAppleARKitBackgroundFitMode : uint8
{
	/** The background image will be letterboxed to fit the screen */
	Fit,

	/** The background will be scaled & cropped to the screen */
	Fill,

	/** The background image will be stretched to fill the screen */
	Stretch,
};


enum class EAppleARKitTrackingQuality : uint8
{
    /** The tracking quality is not available. */
    NotAvailable,
    
    /** The tracking quality is limited, relying only on the device's motion. */
    Limited,
    
    /** The tracking quality is poor. */
    Poor,
    
    /** The tracking quality is good. */
    Good
};

/**
 * A model representing the camera and its properties at a single point in time.
 */
struct APPLEARKIT_API FAppleARKitCamera
{
	// Default constructor
	FAppleARKitCamera() {};

#if ARKIT_SUPPORT

	/** 
	 * This is a conversion copy-constructor that takes a raw ARCamera and fills this structs members
	 * with the UE4-ified versions of ARCamera's properties.
	 */ 
	FAppleARKitCamera( ARCamera* InARCamera );

#endif // #ARKIT_SUPPORT

	/**
	 * The tracking quality of the camera.
	 */
	UPROPERTY( BlueprintReadOnly, Category="AppleARKit|Camera" )
	EAppleARKitTrackingQuality TrackingQuality;

	/**
	 * The transformation matrix that defines the camera's rotation and translation in world coordinates.
	 */
	UPROPERTY( BlueprintReadOnly, Category="AppleARKit|Camera" )
	FTransform Transform;

	/* Raw orientation of the camera */
	UPROPERTY(BlueprintReadOnly, Category = "AppleARKit|Camera")
	FQuat Orientation;

	/* Raw position of the camera */
	UPROPERTY(BlueprintReadOnly, Category = "AppleARKit|Camera")
	FVector Translation;

	/**
	 * Camera image resolution in pixels
	 */
	UPROPERTY( BlueprintReadOnly, Category="AppleARKit|Camera" )
	FVector2D ImageResolution;

	/**
	 * Camera focal length in pixels
	 */
	UPROPERTY( BlueprintReadOnly, Category="AppleARKit|Camera" )
	FVector2D FocalLength;

	/**
	 * Camera principal point in pixels
	 */
	UPROPERTY( BlueprintReadOnly, Category="AppleARKit|Camera" )
	FVector2D PrincipalPoint;

	/** Returns the ImageResolution aspect ration (Width / Height) */
	float GetAspectRatio() const;

	/** Returns the horizonal FOV of the camera on this frame in degrees. */
	float GetHorizontalFieldOfView() const;

	/** Returns the vertical FOV of the camera on this frame in degrees. */
	float GetVerticalFieldOfView() const;

	/** Returns the effective horizontal field of view for the screen dimensions and fit mode in those dimensions */
	float GetHorizontalFieldOfViewForScreen( EAppleARKitBackgroundFitMode BackgroundFitMode ) const
	{
		// Use the global viewport size as the screen size
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize( ViewportSize );

		return GetHorizontalFieldOfViewForScreen( BackgroundFitMode, ViewportSize.X, ViewportSize.Y );
	}

	/** Returns the effective horizontal field of view for the screen dimensions and fit mode in those dimensions */
	float GetHorizontalFieldOfViewForScreen( EAppleARKitBackgroundFitMode BackgroundFitMode, float ScreenWidth, float ScreenHeight ) const;

	/** For the given screen position, returns the normalised capture image coordinates accounting for the fit mode of the image on screen */
	FVector2D GetImageCoordinateForScreenPosition( FVector2D ScreenPosition, EAppleARKitBackgroundFitMode BackgroundFitMode ) const
	{
		// Use the global viewport size as the screen size
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize( ViewportSize );

		return GetImageCoordinateForScreenPosition( ScreenPosition, BackgroundFitMode, ViewportSize.X, ViewportSize.Y );
	}

	/** For the given screen position, returns the normalised capture image coordinates accounting for the fit mode of the image on screen */
	FVector2D GetImageCoordinateForScreenPosition( FVector2D ScreenPosition, EAppleARKitBackgroundFitMode BackgroundFitMode, float ScreenWidth, float ScreenHeight ) const;
};

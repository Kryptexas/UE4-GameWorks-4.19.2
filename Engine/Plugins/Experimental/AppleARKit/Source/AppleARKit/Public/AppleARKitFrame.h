#pragma once

// AppleARKit
#include "AppleARKitCamera.h"
#include "AppleARKitLightEstimate.h"
#include "AppleARKitFrame.generated.h"

/**
 * An object representing a frame processed by UAppleARKitSession.
 * @discussion Each frame contains information about the current state of the scene.
 */
USTRUCT( BlueprintType, Category="AppleARKit" )
struct APPLEARKIT_API FAppleARKitFrame
{
	GENERATED_BODY()

	// Default constructor
	FAppleARKitFrame();

#if ARKIT_SUPPORT

	/** 
	 * This is a conversion copy-constructor that takes a raw ARFrame and fills this structs members
	 * with the UE4-ified versions of ARFrames properties.
	 *
	 * @param InARFrame 		- The frame to convert / copy
	 * @param MetalTextureCache - A CVMetalTextureCacheRef to use in the conversion of InARFrame's CVPixelBufferRef to CVMetalTextureRef for our CapturedImage property.
	 */ 
	FAppleARKitFrame( ARFrame* InARFrame, CVMetalTextureCacheRef MetalTextureCache );

	/** 
	 * Copy contructor. CapturedImage is skipped as we don't need / want to retain access to the 
	 * image buffer. */
	FAppleARKitFrame( const FAppleARKitFrame& Other );

	/** Destructor. Calls CFRelease on CapturedImage */
	virtual ~FAppleARKitFrame();

	/** 
	 * Assignment operator. CapturedImage is skipped as we don't need / want to retain access to the 
	 * image buffer. */
	FAppleARKitFrame& operator=( const FAppleARKitFrame& Other );

#endif // ARKIT_SUPPORT

	/** A timestamp identifying the frame. */
	double Timestamp;

#if ARKIT_SUPPORT

	/** The frame's captured images */
    CVMetalTextureRef CapturedYImage;
    CVMetalTextureRef CapturedCbCrImage;
    
#endif // ARKIT_SUPPORT

	/** The width and height in pixels of the frame's captured images. */
	uint32 CapturedYImageWidth;
	uint32 CapturedYImageHeight;
    
    uint32 CapturedCbCrImageWidth;
    uint32 CapturedCbCrImageHeight;
    
	/** The camera used to capture the frame's image. */
	UPROPERTY( BlueprintReadOnly, Category="AppleARKit|Frame" )
	FAppleARKitCamera Camera; 

	/**
	 * A light estimate representing the estimated light in the scene.
	 */
	UPROPERTY( BlueprintReadOnly, Category="AppleARKit|Frame" )
	FAppleARKitLightEstimate LightEstimate;

	/* 
	 * When adding new member variables, don't forget to handle them in the copy constructor and
	 * assignment operator above.
	 */
};

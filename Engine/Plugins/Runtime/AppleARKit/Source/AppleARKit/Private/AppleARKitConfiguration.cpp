// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

// AppleARKit
#include "AppleARKitConfiguration.h"
#include "AppleARKitModule.h"

#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000

ARWorldAlignment ToARWorldAlignment( const EAppleARKitWorldAlignment& InWorldAlignment )
{
	switch ( InWorldAlignment )
	{
		case EAppleARKitWorldAlignment::Gravity:
			return ARWorldAlignmentGravity;
			
    	case EAppleARKitWorldAlignment::GravityAndHeading:
    		return ARWorldAlignmentGravityAndHeading;

    	case EAppleARKitWorldAlignment::Camera:
    		return ARWorldAlignmentCamera;
    };
};

ARPlaneDetection ToARPlaneDetection( const EAppleARKitPlaneDetection& InPlaneDetection )
{
	ARPlaneDetection PlaneDetection = ARPlaneDetectionNone;
    
    if ( !!(InPlaneDetection & EAppleARKitPlaneDetection::Horizontal) )
    {
    	PlaneDetection |= ARPlaneDetectionHorizontal;
    }

    return PlaneDetection;
}

ARConfiguration* ToARConfiguration( UARSessionConfig* SessionConfig, FAppleARKitConfiguration& InConfiguration )
{
	EARSessionType SessionType = SessionConfig->GetSessionType();
	ARConfiguration* SessionConfiguration = nullptr;
	switch (SessionType)
	{
		case EARSessionType::Orientation:
		{
			if (AROrientationTrackingConfiguration.isSupported == FALSE)
			{
				return nullptr;
			}
			SessionConfiguration = [AROrientationTrackingConfiguration new];
			break;
		}
		case EARSessionType::World:
		{
			if (ARWorldTrackingConfiguration.isSupported == FALSE)
			{
				return nullptr;
			}
			ARWorldTrackingConfiguration* WorldTrackingConfiguration = [ARWorldTrackingConfiguration new];
			if (SessionConfig->GetPlaneDetectionMode() == EARPlaneDetectionMode::HorizontalPlaneDetection)
			{
				WorldTrackingConfiguration.planeDetection = ARPlaneDetectionHorizontal;
			}
			SessionConfiguration = WorldTrackingConfiguration;
			break;
		}
		case EARSessionType::Face:
		{
			if (ARFaceTrackingConfiguration.isSupported == FALSE)
			{
				return nullptr;
			}
			SessionConfiguration = [ARFaceTrackingConfiguration new];
			break;
		}
		default:
			return nullptr;
	}
	check(SessionConfiguration != nullptr);

	// Copy / convert properties
	SessionConfiguration.lightEstimationEnabled = InConfiguration.bLightEstimationEnabled;
	SessionConfiguration.providesAudioData = InConfiguration.bProvidesAudioData;
	SessionConfiguration.worldAlignment = ToARWorldAlignment(InConfiguration.Alignment);

    return SessionConfiguration;
}

#endif

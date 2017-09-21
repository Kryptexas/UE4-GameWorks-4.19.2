// AppleARKit
#include "AppleARKitSessionConfiguration.h"
#include "AppleARKit.h"

#if ARKIT_SUPPORT

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

ARConfiguration* ToARSessionConfiguration( const UAppleARKitSessionConfiguration* InSessionConfiguration )
{
	check( InSessionConfiguration );

	// World tracking?
	if ( const UAppleARKitWorldTrackingSessionConfiguration* WorldTrackingSessionConfiguration = Cast< UAppleARKitWorldTrackingSessionConfiguration >( InSessionConfiguration ) )
	{
		// Create an ARWorldTrackingConfiguration 
		ARWorldTrackingConfiguration *SessionConfiguration = [ARWorldTrackingConfiguration new];

		// Copy / convert properties
		SessionConfiguration.lightEstimationEnabled = WorldTrackingSessionConfiguration->bLightEstimationEnabled;
		SessionConfiguration.worldAlignment = ToARWorldAlignment( WorldTrackingSessionConfiguration->Alignment );
		// SessionConfiguration.planeDetection = ToARPlaneDetection( static_cast< EAppleARKitPlaneDetection >( WorldTrackingSessionConfiguration->PlaneDetection ) );
        SessionConfiguration.planeDetection = ARPlaneDetectionHorizontal;
      
		return SessionConfiguration;
	}
	else
	{
		// Create an ARConfiguration 
//		ARConfiguration *SessionConfiguration = [ARConfiguration new];

		// Copy / convert properties
//		SessionConfiguration.lightEstimationEnabled = InSessionConfiguration->bLightEstimationEnabled;
//		SessionConfiguration.worldAlignment = ToARWorldAlignment( InSessionConfiguration->Alignment );

//		return SessionConfiguration;
	}

	return nullptr;
}

ARWorldTrackingConfiguration* ToARWorldTrackingSessionConfiguration( const UAppleARKitWorldTrackingSessionConfiguration* InSessionConfiguration )
{
	return (ARWorldTrackingConfiguration*)ToARSessionConfiguration( InSessionConfiguration );
}

#endif

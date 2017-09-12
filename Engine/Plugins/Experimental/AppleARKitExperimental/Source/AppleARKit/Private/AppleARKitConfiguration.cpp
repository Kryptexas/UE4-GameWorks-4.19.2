// AppleARKit
#include "AppleARKitConfiguration.h"
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

ARConfiguration* ToARConfiguration( const UAppleARKitConfiguration* InConfiguration )
{
	check( InConfiguration );

	// World tracking?
	if ( const UAppleARKitWorldTrackingConfiguration* WorldTrackingConfiguration = Cast< UAppleARKitWorldTrackingConfiguration >( InConfiguration ) )
	{
		// Create an ARWorldTrackingConfiguration 
		ARWorldTrackingConfiguration* SessionConfiguration = [ARWorldTrackingConfiguration new];

		// Copy / convert properties
		SessionConfiguration.lightEstimationEnabled = WorldTrackingConfiguration->bLightEstimationEnabled;
		SessionConfiguration.worldAlignment = ToARWorldAlignment( WorldTrackingConfiguration->Alignment );
		// SessionConfiguration.planeDetection = ToARPlaneDetection( static_cast< EAppleARKitPlaneDetection >( WorldTrackingConfiguration->PlaneDetection ) );
        SessionConfiguration.planeDetection = ARPlaneDetectionHorizontal;
      
		return SessionConfiguration;
	}
	else
	{
		// Create an ARConfiguration
//		ARConfiguration* SessionConfiguration = [ARConfiguration new];

		// Copy / convert properties
//		SessionConfiguration.lightEstimationEnabled = InConfiguration->bLightEstimationEnabled;
//		SessionConfiguration.worldAlignment = ToARWorldAlignment( InConfiguration->Alignment );

//		return SessionConfiguration;
	}
    
    return nullptr;
}

ARWorldTrackingConfiguration* ToARWorldTrackingConfiguration( const UAppleARKitWorldTrackingConfiguration* InConfiguration )
{
	return (ARWorldTrackingConfiguration*)ToARConfiguration( InConfiguration );
}

#endif

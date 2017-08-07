#pragma once

// ARKit
#if ARKIT_SUPPORT
#import <ARKit/ARKit.h>
#endif // ARKIT_SUPPORT

// AppleARKit
#include "AppleARKitAnchor.h"
#include "AppleARKitPlaneAnchor.generated.h"

UCLASS( BlueprintType )
class UAppleARKitPlaneAnchor : public UAppleARKitAnchor
{
	GENERATED_BODY()

public: 

	/**
	 * The center of the plane in the anchor’s coordinate space.
	 */
	UFUNCTION( BlueprintPure, Category = "AppleARKitPlaneAnchor" )
	FVector GetCenter() const;

	/**
	 * The extent of the plane in the anchor’s coordinate space.
	 */
	UFUNCTION( BlueprintPure, Category = "AppleARKitPlaneAnchor")
	FVector GetExtent() const;

	UFUNCTION( BlueprintPure, Category = "AppleARKitPlaneAnchor")
	FTransform GetTransformToCenter() const;

#if ARKIT_SUPPORT

	virtual void Update_DelegateThread( ARAnchor* Anchor ) override;

#endif // ARKIT_SUPPORT

private:

	/**
	 * The center of the plane in the anchor’s coordinate space.
	 */
	FVector Center;

	/**
	 * The extent of the plane in the anchor’s coordinate space.
	 */
	FVector Extent;
};

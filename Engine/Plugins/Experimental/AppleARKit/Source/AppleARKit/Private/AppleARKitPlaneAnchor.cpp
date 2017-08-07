// AppleARKit
#include "AppleARKitPlaneAnchor.h"
#include "AppleARKit.h"

// UE4
#include "Misc/ScopeLock.h"
#include "Math/UnrealMathUtility.h"

FVector UAppleARKitPlaneAnchor::GetCenter() const
{
	FScopeLock ScopeLock( &UpdateLock );

	return Center;
}

FVector UAppleARKitPlaneAnchor::GetExtent() const
{
	FScopeLock ScopeLock( &UpdateLock );

	return Extent;
}

FTransform UAppleARKitPlaneAnchor::GetTransformToCenter() const
{
	FScopeLock ScopeLock( &UpdateLock );

	return FTransform( Center ) * Transform;
}

#if ARKIT_SUPPORT

void UAppleARKitPlaneAnchor::Update_DelegateThread( ARAnchor* Anchor )
{
	Super::Update_DelegateThread( Anchor );

	// Plane anchor?
	if ([Anchor isKindOfClass:[ARPlaneAnchor class]])
	{
		ARPlaneAnchor* PlaneAnchor = (ARPlaneAnchor*)Anchor;

		FScopeLock ScopeLock( &UpdateLock );
		
		// @todo use World Settings WorldToMetersScale
		Extent = FAppleARKitTransform::ToFVector( PlaneAnchor.extent ).GetAbs();
		Center = FAppleARKitTransform::ToFVector( PlaneAnchor.center );
	}
}

#endif // ARKIT_SUPPORT

// AppleARKit
#include "AppleARKitAnchor.h"
#include "AppleARKit.h"
#include "AppleARKitTransform.h"

// UE4
#include "Misc/ScopeLock.h"

FTransform UAppleARKitAnchor::GetTransform() const
{
	FScopeLock ScopeLock( &UpdateLock );

	return Transform;
}

#if ARKIT_SUPPORT

void UAppleARKitAnchor::Update_DelegateThread( ARAnchor* Anchor )
{
	FScopeLock ScopeLock( &UpdateLock );

	// @todo use World Settings WorldToMetersScale
	Transform = FAppleARKitTransform::ToFTransform( Anchor.transform );
}

#endif // ARKIT_SUPPORT

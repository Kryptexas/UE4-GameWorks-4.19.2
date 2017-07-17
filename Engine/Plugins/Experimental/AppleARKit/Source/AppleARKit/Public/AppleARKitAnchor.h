#pragma once

// ARKit
#if ARKIT_SUPPORT
#import <ARKit/ARKit.h>
#endif // ARKIT_SUPPORT

// UE4
#include "Misc/Guid.h"
#include "HAL/CriticalSection.h"

// AppleARKit
#include "AppleARKitAnchor.generated.h"

UCLASS( BlueprintType )
class UAppleARKitAnchor : public UObject
{
	GENERATED_BODY()

public: 

	/**
	 * Unique identifier of the anchor.
	 */
	UPROPERTY()
	FGuid Identifier;

	/**
	 * The transformation matrix that defines the anchor's rotation, translation and scale.
	 *
	 * NOTE: This does not have Session::BaseTransform applied due to thread safety issues. You'll
	 * need to apply this yourself in the game thread.
	 *
	 * @todo Fix this ^
	 */
	UFUNCTION( BlueprintPure, Category = "AppleARKitAnchor" )
	FTransform GetTransform() const;

#if ARKIT_SUPPORT

	virtual void Update_DelegateThread( ARAnchor* Anchor );

#endif // ARKIT_SUPPORT

protected:

	// Thread safe update lock
	mutable FCriticalSection UpdateLock;

	/**
	 * The transformation matrix that defines the anchor's rotation, translation and scale in world coordinates.
	 */
	FTransform Transform;
};

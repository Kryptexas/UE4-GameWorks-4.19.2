#pragma once

// UE4
#include "Camera/CameraActor.h"

// AppleARKit
#include "AppleARKitCameraActor.generated.h"

/** Specialisation of ACameraActor to use UAppleARKitCameraComponent as the camera component */
UCLASS( ClassGroup=AppleARKit, BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent) )
class AAppleARKitCameraActor : public ACameraActor
{
	GENERATED_BODY()

public:

	/** Default UObject constructor. */
	AAppleARKitCameraActor( const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get() );
};

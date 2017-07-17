// AppleARKitPlugin
#include "AppleARKitCameraActor.h"
#include "AppleARKit.h"
#include "AppleARKitCameraComponent.h"

AAppleARKitCameraActor::AAppleARKitCameraActor(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
  : Super(
  		ObjectInitializer.SetDefaultSubobjectClass<UAppleARKitCameraComponent>("CameraComponent")
  	)
{
}

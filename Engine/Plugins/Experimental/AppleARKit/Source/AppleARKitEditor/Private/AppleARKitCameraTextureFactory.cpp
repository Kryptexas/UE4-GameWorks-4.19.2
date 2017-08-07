#include "AppleARKitCameraTextureFactory.h"
#include "AppleARKitEditor.h"
#include "AppleARKitCameraTexture.h"

UAppleARKitCameraTextureFactory::UAppleARKitCameraTextureFactory()
{
	SupportedClass = UAppleARKitCameraTexture::StaticClass();

	bCreateNew = true;
}

FText UAppleARKitCameraTextureFactory::GetDisplayName() const
{
	return NSLOCTEXT( "AppleARKit", "CameraTextureName", "AppleARKit Camera Texture" );
}

UObject* UAppleARKitCameraTextureFactory::FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn )
{
	return NewObject< UAppleARKitCameraTexture >( InParent, InClass, InName, Flags );
}

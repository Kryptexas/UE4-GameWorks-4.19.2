// AppleARKit
#include "AppleARKitCameraTexture.h"
#include "AppleARKit.h"
#include "AppleARKitCameraTextureResource.h"

// UE4
#include "TextureResource.h"

FTextureResource* UAppleARKitCameraTexture::CreateResource()
{
	return new FAppleARKitCameraTextureResource( this );
}

float UAppleARKitCameraTexture::GetSurfaceWidth() const
{
	return (Resource != nullptr) ? Resource->GetSizeX() : 0.0f;
}

float UAppleARKitCameraTexture::GetSurfaceHeight() const
{
	return (Resource != nullptr) ? Resource->GetSizeY() : 0.0f;
}

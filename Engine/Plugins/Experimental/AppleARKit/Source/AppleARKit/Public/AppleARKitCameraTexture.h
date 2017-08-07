#pragma once

// UE4
#include "Engine/Texture.h"

// AppleARKit
#include "AppleARKitCameraTexture.generated.h"

UCLASS(hidecategories=Object, MinimalAPI)
class UAppleARKitCameraTexture : public UTexture
{
	GENERATED_BODY()

public:

	// UTexture Interface.
	APPLEARKIT_API virtual FTextureResource* CreateResource() override;
	APPLEARKIT_API virtual EMaterialValueType GetMaterialType() override { return MCT_Texture2D; }
	APPLEARKIT_API virtual float GetSurfaceWidth() const override;
	APPLEARKIT_API virtual float GetSurfaceHeight() const override;
};

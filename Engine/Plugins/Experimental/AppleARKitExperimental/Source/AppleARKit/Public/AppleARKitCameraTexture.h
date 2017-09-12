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
	APPLEARKITEXPERIMENTAL_API virtual FTextureResource* CreateResource() override;
	APPLEARKITEXPERIMENTAL_API virtual EMaterialValueType GetMaterialType() const override { return MCT_Texture2D; }
	APPLEARKITEXPERIMENTAL_API virtual float GetSurfaceWidth() const override;
	APPLEARKITEXPERIMENTAL_API virtual float GetSurfaceHeight() const override;
};

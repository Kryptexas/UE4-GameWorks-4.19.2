#pragma once

// UE4
#include "TextureResource.h"
#include "UObject/WeakObjectPtrTemplates.h"

// AppleARKit
#include "AppleARKitSession.h"

class FAppleARKitCameraTextureResource : public FTextureResource, public FDeferredUpdateResource
{
public:
	/** Initialization constructor. */
	APPLEARKIT_API FAppleARKitCameraTextureResource( class UAppleARKitCameraTexture* InOwner );

	// FTexture interface
	virtual uint32 GetSizeX() const override;
	virtual uint32 GetSizeY() const override;

	// FRenderResource interface
	virtual void InitDynamicRHI() override;
	virtual void ReleaseDynamicRHI() override;

	// FDeferredUpdateResource
	virtual void UpdateDeferredResource( FRHICommandListImmediate& RHICmdList, bool bClearRenderTarget=true ) override;

private:

	/** The owner of this resource. */
	class UAppleARKitCameraTexture* Owner;

	// Session
	TWeakObjectPtr< UAppleARKitSession > Session;

	// The last frame applied to this camera
	double LastUpdateTimestamp = -1.0;

	// Frame size
	uint32 SizeX = 1;
	uint32 SizeY = 1;
};

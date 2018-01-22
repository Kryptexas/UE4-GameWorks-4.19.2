// Copyright 2017 Google Inc.

#pragma once

#include "CoreDelegates.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/Texture.h"
#include "HAL/ThreadSafeBool.h"

class FGoogleARCoreDeviceCameraBlitter
{
public:

	FGoogleARCoreDeviceCameraBlitter();
	~FGoogleARCoreDeviceCameraBlitter();

	void DoBlit(uint32_t TextureId, FIntPoint ImageSize);
	UTexture *GetLastCameraImageTexture();

private:

	void LateInit(FIntPoint ImageSize);

	uint32 CurrentCameraCopy;
	uint32 BlitShaderProgram;
	uint32 BlitShaderProgram_Uniform_CameraTexture;
	uint32 BlitShaderProgram_Attribute_InPos;
	uint32 FrameBufferObject;
	uint32 VertexBufferObject;
	TArray<UTexture *> CameraCopies;
    TArray<uint32 *> CameraCopyIds;
};

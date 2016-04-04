// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanViewport.h: Vulkan viewport RHI definitions.
=============================================================================*/

#pragma once

#include "VulkanResources.h"

class FVulkanDynamicRHI;
class FVulkanSwapChain;
class FVulkanQueue;

class FVulkanViewport : public FRHIViewport
{
public:
	enum { NUM_BUFFERS = 3 };

	FVulkanViewport(FVulkanDynamicRHI* InRHI, void* WindowHandle, uint32 InSizeX,uint32 InSizeY,bool bInIsFullscreen, EPixelFormat InPreferredPixelFormat);
	~FVulkanViewport();

	FVulkanTexture2D* GetBackBuffer() const { return BackBuffers[CurrentBackBuffer]; }
	int32 GetBackBufferIndex() const { return CurrentBackBuffer; }

	//bool Present(VkImage& Image, VkQueue& Queue, bool bLockToVsync);

	//#todo-rco
	void WaitForFrameEventCompletion() {}

	//#todo-rco
	void IssueFrameEvent() {}

	FIntPoint GetSizeXY() const { return FIntPoint(SizeX, SizeY); }

#if VULKAN_USE_NEW_COMMAND_BUFFERS
	FVulkanSwapChain* GetSwapChain()
	{
		return SwapChain;
	}

	void AcquireBackBuffer(FVulkanCmdBuffer* CmdBuffer);
	void PrepareBackBufferForPresent(FVulkanCmdBuffer* CmdBuffer);
#else
#endif


protected:
	TRefCountPtr<FVulkanTexture2D> BackBuffers[NUM_BUFFERS];
	FVulkanDynamicRHI* RHI;
	uint32 SizeX;
	uint32 SizeY;
	bool bIsFullscreen;
	EPixelFormat PixelFormat;
	int32 CurrentBackBuffer;
	FVulkanSwapChain* SwapChain;

	friend class FVulkanDynamicRHI;
	friend class FVulkanCommandListContext;
};

template<>
struct TVulkanResourceTraits<FRHIViewport>
{
	typedef FVulkanViewport TConcreteType;
};

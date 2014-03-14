// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	D3D11Viewport.h: D3D viewport RHI definitions.
=============================================================================*/

#pragma once

/** A D3D event query resource. */
class FD3D11EventQuery : public FRenderResource
{
public:

	/** Initialization constructor. */
	FD3D11EventQuery(class FD3D11DynamicRHI* InD3DRHI):
		D3DRHI(InD3DRHI)
	{
	}

	/** Issues an event for the query to poll. */
	void IssueEvent();

	/** Waits for the event query to finish. */
	void WaitForCompletion();

	// FRenderResource interface.
	virtual void InitDynamicRHI();
	virtual void ReleaseDynamicRHI();

private:
	FD3D11DynamicRHI* D3DRHI;
	TRefCountPtr<ID3D11Query> Query;
};

class FD3D11Viewport : public FRHIViewport
{
public:

	FD3D11Viewport(class FD3D11DynamicRHI* InD3DRHI,HWND InWindowHandle,uint32 InSizeX,uint32 InSizeY,bool bInIsFullscreen);
	~FD3D11Viewport();

	static int32 GetBackBufferFormat();

	void Resize(uint32 InSizeX,uint32 InSizeY,bool bInIsFullscreen);

	/**
	 * If the swap chain has been invalidated by DXGI, resets the swap chain to the expected state; otherwise, does nothing.
	 * Called once/frame by the game thread on all viewports.
	 * @param bIgnoreFocus - Whether the reset should happen regardless of whether the window is focused.
     */
	void ConditionalResetSwapChain(bool bIgnoreFocus);

	/** Presents the swap chain. */
	void Present(bool bLockToVsync);

	// Accessors.
	FIntPoint GetSizeXY() const { return FIntPoint(SizeX, SizeY); }
	FD3D11Texture2D* GetBackBuffer() const { return BackBuffer; }

	void WaitForFrameEventCompletion()
	{
		FrameSyncEvent.WaitForCompletion();
	}

	void IssueFrameEvent()
	{
		FrameSyncEvent.IssueEvent();
	}

private:

	/** Presents the frame synchronizing with DWM. */
	void PresentWithVsyncDWM();

	/** Presents the swap chain checking the return result. */
	void PresentChecked(int32 SyncInterval);

	FD3D11DynamicRHI* D3DRHI;
	uint64 LastFlipTime;
	uint64 LastFrameComplete;
	uint64 LastCompleteTime;
	int32 SyncCounter;
	bool bSyncedLastFrame;
	HWND WindowHandle;
	uint32 SizeX;
	uint32 SizeY;
	bool bIsFullscreen;
	bool bIsValid;
	TRefCountPtr<IDXGISwapChain> SwapChain;
	TRefCountPtr<FD3D11Texture2D> BackBuffer;

	/** An event used to track the GPU's progress. */
	FD3D11EventQuery FrameSyncEvent;

	DXGI_MODE_DESC SetupDXGI_MODE_DESC() const;
};


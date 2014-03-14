// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RenderTargetPool.h: Scene render target pool manager.
=============================================================================*/

#pragma once



/** The reference to a pooled render target, use like this: TRefCountPtr<IPooledRenderTarget> */
struct FPooledRenderTarget : public IPooledRenderTarget
{
	/** constructor */
	FPooledRenderTarget(const FPooledRenderTargetDesc& InDesc) 
		: NumRefs(0)
		, Desc(InDesc)
		, UnusedForNFrames(0)
	{
	}

	virtual ~FPooledRenderTarget()
	{
		check(!NumRefs);
		RenderTargetItem.SafeRelease();
	}

	uint32 GetUnusedForNFrames() const { return UnusedForNFrames; }

	// interface IPooledRenderTarget --------------

	virtual uint32 AddRef() const;
	virtual uint32 Release() const;
	virtual uint32 GetRefCount() const;
	virtual bool IsFree() const;
	virtual void SetDebugName(const TCHAR *InName);
	virtual const FPooledRenderTargetDesc& GetDesc() const;
	virtual uint32 ComputeMemorySize() const;

private:

	/** For pool management (only if NumRef == 0 the element can be reused) */
	mutable int32 NumRefs;
	/** All necessary data to create the render target */
	FPooledRenderTargetDesc Desc;
	/** Allows to defer the release to save performance on some hardware (DirectX) */
	uint32 UnusedForNFrames;

	/** @return true:release this one, false otherwise */
	bool OnFrameStart();

	friend class FRenderTargetPool;
};

#include "VisualizeTexture.h"


/**
 * Encapsulates the render targets pools that allows easy sharing (mostly used on the render thread side)
 */
class FRenderTargetPool : public FRenderResource
{
public:
	FRenderTargetPool();

	/**
	 * @param DebugName must not be 0, we only store the pointer
	 * @param Out is not the return argument to avoid double allocation because of wrong reference counting
	 * call from RenderThread only
	 * @return true if the old element was still valid, false if a new one was assigned
	 */
	bool FindFreeElement(const FPooledRenderTargetDesc& Desc, TRefCountPtr<IPooledRenderTarget>& Out, const TCHAR* InDebugName);

	void CreateUntrackedElement(const FPooledRenderTargetDesc& Desc, TRefCountPtr<IPooledRenderTarget>& Out, const FSceneRenderTargetItem& Item);

	/** Only to get statistics on usage and free elements. Normally only called in renderthread or if FlushRenderingCommands was called() */
	void GetStats(uint32& OutWholeCount, uint32& OutWholePoolInKB, uint32& OutUsedInKB) const;
	/**
	 * Can release RT, should be called once per frame.
	 * call from RenderThread only
	 */
	void TickPoolElements();
	/** Free renderer resources */
	void ReleaseDynamicRHI();

	/** Allows to remove a resource so it cannot be shared and gets released immediately instead a/some frame[s] later. */
	void FreeUnusedResource(TRefCountPtr<IPooledRenderTarget>& In);

	/** Good to call between levels or before memory intense operations. */
	void FreeUnusedResources();

	// for debugging purpose, assumes you call FlushRenderingCommands() be
	// @return 0 if outside of the range
	FPooledRenderTarget* GetElementById(uint32 Id) const;
	
	// Render visualization
	void RenderVisualizeTexture(class FDeferredShadingSceneRenderer& Scene);

	void SetObserveTarget(const FString& InObservedDebugName, uint32 InObservedDebugNameReusedGoal = 0xffffffff);

	// Logs out usage information.
	void DumpMemoryUsage(FOutputDevice& OutputDevice);

	FVisualizeTexture VisualizeTexture;

private:

	/** Elements must not be 0 */
	TArray< TRefCountPtr<FPooledRenderTarget> > PooledRenderTargets;

	// redundant, can always be computed with GetStats(), to debug "out of memory" situations and used for r.RenderTargetPoolMin
	uint32 AllocationLevelInKB;

	// to avoid log spam
	bool bCurrentlyOverBudget;

	friend struct FPooledRenderTarget;

	// for debugging purpose
	void VerifyAllocationLevel() const;
};

/** The global render targets for easy shading. */
extern TGlobalResource<FRenderTargetPool> GRenderTargetPool;

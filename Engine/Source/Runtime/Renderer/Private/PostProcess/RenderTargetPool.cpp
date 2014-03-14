// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RenderTargetPool.cpp: Scene render target pool manager.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "RenderTargetPool.h"


/** The global render targets pool. */
TGlobalResource<FRenderTargetPool> GRenderTargetPool;

DEFINE_LOG_CATEGORY_STATIC(LogRenderTargetPool, Warning, All);

static uint32 ComputeSizeInKB(FPooledRenderTarget& Element)
{
	return (Element.ComputeMemorySize() + 1023) / 1024;
}

FRenderTargetPool::FRenderTargetPool()
	: AllocationLevelInKB(0)
	, bCurrentlyOverBudget(false)
{
}

bool FRenderTargetPool::FindFreeElement(const FPooledRenderTargetDesc& Desc, TRefCountPtr<IPooledRenderTarget> &Out, const TCHAR* InDebugName)
{
	check(IsInRenderingThread());

	if(!Desc.IsValid())
	{
		// no need to do anything
		return true;
	}

	// if we can keep the current one, do that
	if(Out)
	{
		FPooledRenderTarget* Current = (FPooledRenderTarget*)Out.GetReference();

		if(Out->GetDesc() == Desc)
		{
			// we can reuse the same, but the debug name might have changed
			Current->Desc.DebugName = InDebugName;
			RHIBindDebugLabelName(Current->GetRenderTargetItem().TargetableTexture, InDebugName);
			check(!Out->IsFree());
			return true;
		}
		else
		{
			// release old reference, it might free a RT we can use
			Out = 0;

			if(Current->IsFree())
			{
				// Adding this to the pool would be a waste, this seems to be a reallocation
				// where we want to change the size or format
				PooledRenderTargets.Remove(Current);
			}
		}
	}

	FPooledRenderTarget* Found = 0;

	// try to find a suitable element in the pool
	for(uint32 i = 0; i < (uint32)PooledRenderTargets.Num(); ++i)
	{
		FPooledRenderTarget* Element = PooledRenderTargets[i];

		if(Element->IsFree() && Element->GetDesc() == Desc)
		{
			Found = Element;
			break;
		}
	}

	if(!Found)
	{
		UE_LOG(LogRenderTargetPool, Display, TEXT("%d MB, NewRT %s %s"), (AllocationLevelInKB + 1023) / 1024, *Desc.GenerateInfoString(), InDebugName);

		// not found in the pool, create a new element
		Found = new FPooledRenderTarget(Desc);

		PooledRenderTargets.Add(Found);

		if(Desc.TargetableFlags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable | TexCreate_UAV))
		{
			if(Desc.Is2DTexture())
			{
				RHICreateTargetableShaderResource2D(
					Desc.Extent.X,
					Desc.Extent.Y,
					Desc.Format,
					Desc.NumMips,
					Desc.Flags,
					Desc.TargetableFlags,
					Desc.bForceSeparateTargetAndShaderResource,
					(FTexture2DRHIRef&)Found->RenderTargetItem.TargetableTexture,
					(FTexture2DRHIRef&)Found->RenderTargetItem.ShaderResourceTexture,
					Desc.NumSamples
					);
			}
			else if(Desc.Is3DTexture())
			{
				Found->RenderTargetItem.ShaderResourceTexture = RHICreateTexture3D(
					Desc.Extent.X,
					Desc.Extent.Y,
					Desc.Depth,
					Desc.Format,
					Desc.NumMips,
					Desc.TargetableFlags,
					NULL);

				// similar to RHICreateTargetableShaderResource2D
				Found->RenderTargetItem.TargetableTexture = Found->RenderTargetItem.ShaderResourceTexture;
			}
			else
			{
				check(Desc.IsCubemap());
				if(Desc.IsArray())
				{
					RHICreateTargetableShaderResourceCubeArray(
						Desc.Extent.X,
						Desc.ArraySize,
						Desc.Format,
						Desc.NumMips,
						Desc.Flags,
						Desc.TargetableFlags,
						false,
						(FTextureCubeRHIRef&)Found->RenderTargetItem.TargetableTexture,
						(FTextureCubeRHIRef&)Found->RenderTargetItem.ShaderResourceTexture
						);
				}
				else
				{
					RHICreateTargetableShaderResourceCube(
						Desc.Extent.X,
						Desc.Format,
						Desc.NumMips,
						Desc.Flags,
						Desc.TargetableFlags,
						false,
						(FTextureCubeRHIRef&)Found->RenderTargetItem.TargetableTexture,
						(FTextureCubeRHIRef&)Found->RenderTargetItem.ShaderResourceTexture
						);
				}

			}

			RHIBindDebugLabelName(Found->RenderTargetItem.TargetableTexture, InDebugName);
		}
		else
		{
			if(Desc.Is2DTexture())
			{
				// this is useful to get a CPU lockable texture through the same interface
				Found->RenderTargetItem.ShaderResourceTexture = RHICreateTexture2D(
					Desc.Extent.X,
					Desc.Extent.Y,
					Desc.Format,
					Desc.NumMips,
					Desc.NumSamples,
					Desc.Flags,
					NULL);
			}
			else if(Desc.Is3DTexture())
			{
				Found->RenderTargetItem.ShaderResourceTexture = RHICreateTexture3D(
					Desc.Extent.X,
					Desc.Extent.Y,
					Desc.Depth,
					Desc.Format,
					Desc.NumMips,
					Desc.Flags,
					NULL);
			}
			else 
			{
				check(Desc.IsCubemap());
				if(Desc.IsArray())
				{
					FTextureCubeRHIRef CubeTexture = RHICreateTextureCubeArray(Desc.Extent.X,Desc.ArraySize,Desc.Format,Desc.NumMips,Desc.Flags | Desc.TargetableFlags | TexCreate_ShaderResource,NULL);
					Found->RenderTargetItem.TargetableTexture = Found->RenderTargetItem.ShaderResourceTexture = CubeTexture;
				}
				else
				{
					FTextureCubeRHIRef CubeTexture = RHICreateTextureCube(Desc.Extent.X,Desc.Format,Desc.NumMips,Desc.Flags | Desc.TargetableFlags | TexCreate_ShaderResource,NULL);
					Found->RenderTargetItem.TargetableTexture = Found->RenderTargetItem.ShaderResourceTexture = CubeTexture;
				}
			}

			RHIBindDebugLabelName(Found->RenderTargetItem.ShaderResourceTexture, InDebugName);
		}

		if(Desc.TargetableFlags & TexCreate_UAV)
		{
			// The render target desc is invalid if a UAV is requested with an RHI that doesn't support the high-end feature level.
			check(GRHIFeatureLevel == ERHIFeatureLevel::SM5);
			Found->RenderTargetItem.UAV = RHICreateUnorderedAccessView(Found->RenderTargetItem.TargetableTexture);
		}

		AllocationLevelInKB += ComputeSizeInKB(*Found);
		VerifyAllocationLevel();
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	{
		static const auto ICVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.RenderTargetPoolTest"));
		
		if(ICVar->GetValueOnRenderThread())
		{
			if(Found->GetDesc().TargetableFlags & TexCreate_RenderTargetable)
			{
				RHISetRenderTarget(Found->RenderTargetItem.TargetableTexture, FTextureRHIRef());	
				RHIClear(true, FLinearColor(1000, 1000, 1000, 1000), false, 1.0f, false, 0, FIntRect());
			}
			else if(Found->GetDesc().TargetableFlags & TexCreate_UAV)
			{
				const uint32 ZeroClearValue[4] = { 1000, 1000, 1000, 1000 };
				RHIClearUAV(Found->RenderTargetItem.UAV, ZeroClearValue);
			}

			if(Desc.TargetableFlags & TexCreate_DepthStencilTargetable)
			{
				RHISetRenderTarget(FTextureRHIRef(), Found->RenderTargetItem.TargetableTexture);
				RHIClear(false, FLinearColor(0, 0, 0, 0), true, 0.0f, false, 0, FIntRect());
			}
		}
	}
#endif

	check(Found->IsFree());

	Found->Desc.DebugName = InDebugName;
	Found->UnusedForNFrames = 0;

	// assign to the reference counted variable
	Out = Found;

	check(!Found->IsFree());

	return false;
}

void FRenderTargetPool::CreateUntrackedElement(const FPooledRenderTargetDesc& Desc, TRefCountPtr<IPooledRenderTarget> &Out, const FSceneRenderTargetItem& Item)
{
	check(IsInRenderingThread());

	Out = 0;

	// not found in the pool, create a new element
	FPooledRenderTarget* Found = new FPooledRenderTarget(Desc);

	Found->RenderTargetItem = Item;

	// assign to the reference counted variable
	Out = Found;
}

void FRenderTargetPool::GetStats(uint32& OutWholeCount, uint32& OutWholePoolInKB, uint32& OutUsedInKB) const
{
	OutWholeCount = (uint32)PooledRenderTargets.Num();
	OutUsedInKB = 0;
	OutWholePoolInKB = 0;
		
	for(uint32 i = 0; i < (uint32)PooledRenderTargets.Num(); ++i)
	{
		FPooledRenderTarget* Element = PooledRenderTargets[i];

		uint32 SizeInKB = ComputeSizeInKB(*Element);
		
		OutWholePoolInKB += SizeInKB;

		if(!Element->IsFree())
		{
			OutUsedInKB += SizeInKB;
		}
	}

	check(AllocationLevelInKB == OutWholePoolInKB);
}

void FRenderTargetPool::TickPoolElements()
{
	check(IsInRenderingThread());

	uint32 MinimumPoolSizeInKB;
	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.RenderTargetPoolMin"));

		MinimumPoolSizeInKB = FMath::Clamp(CVar->GetValueOnRenderThread(), 0, 2000) * 1024;
	}

	for(uint32 i = 0; i < (uint32)PooledRenderTargets.Num(); ++i)
	{
		FPooledRenderTarget* Element = PooledRenderTargets[i];

		Element->OnFrameStart();
	}
			
	// we need to release something, take the oldest ones first
	while(AllocationLevelInKB > MinimumPoolSizeInKB)
	{
		// -1: not set
		int32 OldestElementIndex = -1;

		// find oldest element we can remove
		for(uint32 i = 0; i < (uint32)PooledRenderTargets.Num(); ++i)
		{
			FPooledRenderTarget* Element = PooledRenderTargets[i];

			if(Element->UnusedForNFrames > 2)
			{
				if(OldestElementIndex != -1)
				{
					if(PooledRenderTargets[OldestElementIndex]->UnusedForNFrames < Element->UnusedForNFrames)
					{
						OldestElementIndex = i;
					}
				}
				else
				{
					OldestElementIndex = i;
				}
			}
		}

		if(OldestElementIndex != -1)
		{
			AllocationLevelInKB -= ComputeSizeInKB(*PooledRenderTargets[OldestElementIndex]);

			// we assume because of reference counting the resource gets released when not needed any more
			PooledRenderTargets.RemoveAt(OldestElementIndex);

			VerifyAllocationLevel();
		}
		else
		{
			// There is no element we can remove but we are over budget, better we log that.
			// Options:
			//   * Increase the pool
			//   * Reduce rendering features or resolution
			//   * Investigate allocations, order or reusing other render targets can help
			//   * Ignore (editor case, might start using slow memory which can be ok)
			if(!bCurrentlyOverBudget)
			{
				UE_LOG(LogRenderTargetPool, Warning, TEXT("r.RenderTargetPoolMin exceeded %d/%d MB (ok in editor, bad on fixed memory platform)"), (AllocationLevelInKB + 1023) / 1024, MinimumPoolSizeInKB / 1024);
				bCurrentlyOverBudget = true;
			}
			// at this point we need to give up
			break;
		}

/*	
	// confused more than it helps (often a name is used on two elements in the pool and some pool elements are not rendered to this frame)
	else
	{
		// initial state of a render target (e.g. Velocity@0)
		GRenderTargetPool.VisualizeTexture.SetCheckPoint(Element);
*/	}

	if(AllocationLevelInKB <= MinimumPoolSizeInKB)
	{
		if(bCurrentlyOverBudget)
		{
			UE_LOG(LogRenderTargetPool, Display, TEXT("r.RenderTargetPoolMin resolved %d/%d MB"), (AllocationLevelInKB + 1023) / 1024, MinimumPoolSizeInKB / 1024);
			bCurrentlyOverBudget = false;
		}
	}
}

void FRenderTargetPool::FreeUnusedResource(TRefCountPtr<IPooledRenderTarget>& In)
{
	check(IsInRenderingThread());

	if(!IsValidRef(In))
	{
		// no work needed
		return;
	}

	for(uint32 i = 0; i < (uint32)PooledRenderTargets.Num(); ++i)
	{
		FPooledRenderTarget* Element = PooledRenderTargets[i];

		if(Element == In)
		{
			AllocationLevelInKB -= ComputeSizeInKB(*Element);
			// we assume because of reference counting the resource gets released when not needed any more
			PooledRenderTargets.RemoveAt(i);
			break;
		}
	}

	In.SafeRelease();

	VerifyAllocationLevel();
}

void FRenderTargetPool::FreeUnusedResources()
{
	check(IsInRenderingThread());

	for(uint32 i = 0; i < (uint32)PooledRenderTargets.Num();)
	{
		FPooledRenderTarget* Element = PooledRenderTargets[i];

		if(Element->IsFree())
		{
			AllocationLevelInKB -= ComputeSizeInKB(*Element);
			// we assume because of reference counting the resource gets released when not needed any more
			PooledRenderTargets.RemoveAt(i);
		}
		else
		{
			++i;
		}
	}

	VerifyAllocationLevel();
}


void FRenderTargetPool::DumpMemoryUsage(FOutputDevice& OutputDevice)
{
	OutputDevice.Logf(TEXT("Pooled Render Targets:"));
	for(int32 i = 0; i < PooledRenderTargets.Num(); ++i)
	{
		FPooledRenderTarget* Element = PooledRenderTargets[i];
		OutputDevice.Logf(
			TEXT("  %6.3fMB %4dx%4d%s%s %2dmip(s) %s (%s)"),
			ComputeSizeInKB(*Element) / 1024.0f,
			Element->Desc.Extent.X,
			Element->Desc.IsCubemap() ? Element->Desc.Extent.X : Element->Desc.Extent.Y,
			Element->Desc.Depth > 1 ? *FString::Printf(TEXT("x%3d"), Element->Desc.Depth) : (Element->Desc.IsCubemap() ? TEXT("cube") : TEXT("    ")),
			Element->Desc.bIsArray ? *FString::Printf(TEXT("[%3d]"), Element->Desc.ArraySize) : TEXT("     "),
			Element->Desc.NumMips,
			Element->Desc.DebugName,
			GPixelFormats[Element->Desc.Format].Name
			);
	}
	uint32 NumTargets=0;
	uint32 UsedKB=0;
	uint32 PoolKB=0;
	GetStats(NumTargets,PoolKB,UsedKB);
	OutputDevice.Logf(TEXT("%.3fMB total, %.3fMB used, %d render targets"), PoolKB / 1024.f, UsedKB / 1024.f, NumTargets);
}

static void DumpRenderTargetPoolMemory(FOutputDevice& OutputDevice)
{
	GRenderTargetPool.DumpMemoryUsage(OutputDevice);
}

static FAutoConsoleCommandWithOutputDevice GDumpRenderTargetPoolMemoryCmd(
	TEXT("r.DumpRenderTargetPoolMemory"),
	TEXT("Dump allocation information for the render target pool."),
	FConsoleCommandWithOutputDeviceDelegate::CreateStatic(DumpRenderTargetPoolMemory)
	);

uint32 FPooledRenderTarget::AddRef() const
{
	return uint32(++NumRefs);
}

uint32 FPooledRenderTarget::Release() const
{
	uint32 Refs = uint32(--NumRefs);
	if(Refs == 0)
	{
		// better we remove const from Release()
		FSceneRenderTargetItem& NonConstItem = (FSceneRenderTargetItem&)RenderTargetItem;

		NonConstItem.SafeRelease();
		delete this;
	}
	return Refs;
}

uint32 FPooledRenderTarget::GetRefCount() const
{
	return uint32(NumRefs);
}

void FPooledRenderTarget::SetDebugName(const TCHAR *InName)
{
	check(InName);

	Desc.DebugName = InName;
}

const FPooledRenderTargetDesc& FPooledRenderTarget::GetDesc() const
{
	return Desc;
}

void FRenderTargetPool::ReleaseDynamicRHI()
{
	check(IsInRenderingThread());
	PooledRenderTargets.Empty();
}

// for debugging purpose
FPooledRenderTarget* FRenderTargetPool::GetElementById(uint32 Id) const
{
	// is used in game and render thread

	if(Id >= (uint32)PooledRenderTargets.Num())
	{
		return 0;
	}

	return PooledRenderTargets[Id];
}

void FRenderTargetPool::VerifyAllocationLevel() const
{
/*
	// to verify internal consistency
	uint32 OutWholeCount;
	uint32 OutWholePoolInKB;
	uint32 OutUsedInKB;

	GetStats(OutWholeCount, OutWholePoolInKB, OutUsedInKB);
*/
}

bool FPooledRenderTarget::OnFrameStart()
{
	check(IsInRenderingThread());

	// If there are any references to the pooled render target other than the pool itself, then it may not be freed.
	if(!IsFree())
	{
		check(!UnusedForNFrames);
		return false;
	}

	++UnusedForNFrames;

	// this logic can be improved
	if(UnusedForNFrames > 10)
	{
		// release
		return true;
	}

	return false;
}

uint32 FPooledRenderTarget::ComputeMemorySize() const
{
	uint32 Size = 0;

	if(Desc.Is2DTexture())
	{
		Size += RHIComputeMemorySize((const FTexture2DRHIRef&)RenderTargetItem.TargetableTexture);
		if(RenderTargetItem.ShaderResourceTexture != RenderTargetItem.TargetableTexture)
		{
			Size += RHIComputeMemorySize((const FTexture2DRHIRef&)RenderTargetItem.ShaderResourceTexture);
		}
	}
	else if(Desc.Is3DTexture())
	{
		Size += RHIComputeMemorySize((const FTexture3DRHIRef&)RenderTargetItem.TargetableTexture);
		if(RenderTargetItem.ShaderResourceTexture != RenderTargetItem.TargetableTexture)
		{
			Size += RHIComputeMemorySize((const FTexture3DRHIRef&)RenderTargetItem.ShaderResourceTexture);
		}
	}
	else
	{
		Size += RHIComputeMemorySize((const FTextureCubeRHIRef&)RenderTargetItem.TargetableTexture);
		if(RenderTargetItem.ShaderResourceTexture != RenderTargetItem.TargetableTexture)
		{
			Size += RHIComputeMemorySize((const FTextureCubeRHIRef&)RenderTargetItem.ShaderResourceTexture);
		}
	}

	return Size;
}

bool FPooledRenderTarget::IsFree() const
{
	check(GetRefCount() >= 1);
	// If the only reference to the pooled render target is from the pool, then it's unused.
	return GetRefCount() == 1;
}

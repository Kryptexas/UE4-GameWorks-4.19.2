// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StaticBoundShaderState.cpp: Static bound shader state implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "StaticBoundShaderState.h"

TLinkedList<FGlobalBoundShaderStateResource*>*& FGlobalBoundShaderStateResource::GetGlobalBoundShaderStateList()
{
	static TLinkedList<FGlobalBoundShaderStateResource*>* List = NULL;
	return List;
}

FGlobalBoundShaderStateResource::FGlobalBoundShaderStateResource():
	GlobalListLink(this)
{
	// Add this resource to the global list in the rendering thread.
	if(IsInRenderingThread())
	{
		GlobalListLink.Link(GetGlobalBoundShaderStateList());
	}
	else
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			LinkGlobalBoundShaderStateResource,FGlobalBoundShaderStateResource*,Resource,this,
			{
				Resource->GlobalListLink.Link(GetGlobalBoundShaderStateList());
			});
	}
}

FGlobalBoundShaderStateResource::~FGlobalBoundShaderStateResource()
{
	// Remove this resource from the global list.
	GlobalListLink.Unlink();
}

/**
 * Initializes a global bound shader state with a vanilla bound shader state and required information.
 */
FBoundShaderStateRHIParamRef FGlobalBoundShaderStateResource::GetInitializedRHI(
	FVertexDeclarationRHIParamRef VertexDeclaration,
	FVertexShaderRHIParamRef VertexShader, 
	FPixelShaderRHIParamRef PixelShader,
	FGeometryShaderRHIParamRef GeometryShader
	)
{
	check(IsInitialized());

	// This should only be called in the rendering thread after the RHI has been initialized.
	check(GIsRHIInitialized);
	check(IsInRenderingThread());

	// Create the bound shader state if it hasn't been cached yet.
	if(!IsValidRef(BoundShaderState))
	{
		BoundShaderState = CreateBoundShaderState_Internal(
			VertexDeclaration,
			VertexShader,
			FHullShaderRHIRef(), 
			FDomainShaderRHIRef(),
			PixelShader,
			GeometryShader);
	}
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// Only people working on shaders (and therefore have LogShaders unsuppressed) will want to see these errors
	else if (!GUsingNullRHI && UE_LOG_ACTIVE(LogShaders, Warning))
	{
		FBoundShaderStateRHIRef TempBoundShaderState = CreateBoundShaderState_Internal(
			VertexDeclaration,
			VertexShader,
			FHullShaderRHIRef(),
			FDomainShaderRHIRef(),
			PixelShader,
			GeometryShader);
		// Verify that bound shader state caching is working and that the passed in shaders will actually be used
		// This will catch cases where one bound shader state is being used with more than one combination of shaders
		// Otherwise setting the shader will just silently fail once the bound shader state has been initialized with a different shader 
		// The catch is that this uses the caching mechanism to test for equality, instead of comparing the actual platform dependent shader references.
		check(TempBoundShaderState == BoundShaderState);
	}
#endif

	return BoundShaderState;
}

void FGlobalBoundShaderStateResource::ReleaseRHI()
{
	// Release the cached bound shader state.
	BoundShaderState.SafeRelease();
}

static void SetGlobalBoundShaderState_Internal(FGlobalBoundShaderState& GlobalBoundShaderState)
{
	// Check for unset uniform buffer parameters
	// Technically you can set uniform buffer parameters after calling RHISetBoundShaderState, but this is the most global place to check for unset parameters
	GlobalBoundShaderState.Get()->Args.VertexShader->VerifyBoundUniformBufferParameters();
	GlobalBoundShaderState.Get()->Args.PixelShader->VerifyBoundUniformBufferParameters();
	GlobalBoundShaderState.Get()->Args.GeometryShader->VerifyBoundUniformBufferParameters();

	FGlobalBoundShaderState_Internal* BSS = GlobalBoundShaderState.Get()->BSS;
	if (!BSS)
	{
		BSS = new FGlobalBoundShaderState_Internal(); // these are simply leaked and never freed
		GlobalBoundShaderState.Get()->BSS = BSS;
	}

	SetBoundShaderState_Internal(
		BSS->GetInitializedRHI(
			GlobalBoundShaderState.Get()->Args.VertexDeclarationRHI,
			GETSAFERHISHADER_VERTEX(GlobalBoundShaderState.Get()->Args.VertexShader),
			GETSAFERHISHADER_PIXEL(GlobalBoundShaderState.Get()->Args.PixelShader),
			(FGeometryShaderRHIParamRef)GETSAFERHISHADER_GEOMETRY(GlobalBoundShaderState.Get()->Args.GeometryShader))
		);
}


struct FRHICommandSetGlobalBoundShaderState : public FRHICommand<FRHICommandSetGlobalBoundShaderState>
{
	FGlobalBoundShaderState GlobalBoundShaderState;
	FORCEINLINE_DEBUGGABLE FRHICommandSetGlobalBoundShaderState(FGlobalBoundShaderState& InGlobalBoundShaderState)
		: GlobalBoundShaderState(InGlobalBoundShaderState)
	{
	}
	void Execute()
	{
		SetGlobalBoundShaderState_Internal(GlobalBoundShaderState);
	}
};


void SetGlobalBoundShaderState(
	FRHICommandList& RHICmdList,
	FGlobalBoundShaderState& GlobalBoundShaderState,
	FVertexDeclarationRHIParamRef VertexDeclarationRHI,
	FShader* VertexShader,
	FShader* PixelShader,
	FShader* GeometryShader
	)
{
	FGlobalBoundShaderStateWorkArea* ExistingGlobalBoundShaderState = GlobalBoundShaderState.Get();
	if (!ExistingGlobalBoundShaderState)
	{
		FGlobalBoundShaderStateWorkArea* NewGlobalBoundShaderState = new FGlobalBoundShaderStateWorkArea();
		NewGlobalBoundShaderState->Args.VertexDeclarationRHI = VertexDeclarationRHI;
		NewGlobalBoundShaderState->Args.VertexShader = VertexShader;
		NewGlobalBoundShaderState->Args.PixelShader = PixelShader;
		NewGlobalBoundShaderState->Args.GeometryShader = GeometryShader;
		FPlatformMisc::MemoryBarrier();

		FGlobalBoundShaderStateWorkArea* OldGlobalBoundShaderState = (FGlobalBoundShaderStateWorkArea*)FPlatformAtomics::InterlockedCompareExchangePointer((void**)GlobalBoundShaderState.GetPtr(), NewGlobalBoundShaderState, nullptr);

		if (OldGlobalBoundShaderState != nullptr)
		{
			//we lost
			delete NewGlobalBoundShaderState;
			check(OldGlobalBoundShaderState == ExistingGlobalBoundShaderState);
		}
	}
	else if (!(VertexDeclarationRHI == ExistingGlobalBoundShaderState->Args.VertexDeclarationRHI &&
		VertexShader == ExistingGlobalBoundShaderState->Args.VertexShader &&
		PixelShader == ExistingGlobalBoundShaderState->Args.PixelShader &&
		GeometryShader == ExistingGlobalBoundShaderState->Args.GeometryShader))
	{
		// this is sketchy from a parallel perspective, but assuming the writes are atomic, and probably even if they aren't, this should be ok
		ExistingGlobalBoundShaderState->Args.VertexDeclarationRHI = VertexDeclarationRHI;
		ExistingGlobalBoundShaderState->Args.VertexShader = VertexShader;
		ExistingGlobalBoundShaderState->Args.PixelShader = PixelShader;
		ExistingGlobalBoundShaderState->Args.GeometryShader = GeometryShader;
	}

	if (RHICmdList.Bypass())
	{
		SetGlobalBoundShaderState_Internal(GlobalBoundShaderState);
		return;
	}
	new (RHICmdList.AllocCommand<FRHICommandSetGlobalBoundShaderState>()) FRHICommandSetGlobalBoundShaderState(GlobalBoundShaderState);
}



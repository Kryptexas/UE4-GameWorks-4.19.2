// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalState.cpp: Metal state implementation.
=============================================================================*/

#include "MetalRHIPrivate.h"

/**
 * Translate from UE4 enums to Metal enums (leaving these in Metal just for help in setting them up)
 */
static MTLSamplerMipFilter TranslateMipFilterMode(ESamplerFilter Filter)
{
	switch (Filter)
	{
		case SF_Point:	return MTLSamplerMipFilterNearest;
		default:		return MTLSamplerMipFilterLinear;
	}
}

static MTLSamplerMinMagFilter TranslateFilterMode(ESamplerFilter Filter)
{
	switch (Filter)
	{
		case SF_Point:				return MTLSamplerMinMagFilterNearest;
		case SF_AnisotropicPoint:	return MTLSamplerMinMagFilterLinear;
		case SF_AnisotropicLinear:	return MTLSamplerMinMagFilterLinear;
		default:					return MTLSamplerMinMagFilterLinear;
	}
}

static uint32 GetMaxAnisotropy(ESamplerFilter Filter, uint32 MaxAniso)
{
	switch (Filter)
	{
		case SF_AnisotropicPoint:
		case SF_AnisotropicLinear:	return MaxAniso > 0 ? MaxAniso : GetCachedScalabilityCVars().MaxAnisotropy;
		default:					return 1;
	}
}

static MTLSamplerMinMagFilter TranslateZFilterMode(ESamplerFilter Filter)
{	switch (Filter)
	{
		case SF_Point:				return MTLSamplerMinMagFilterNearest;
		case SF_AnisotropicPoint:	return MTLSamplerMinMagFilterNearest;
		case SF_AnisotropicLinear:	return MTLSamplerMinMagFilterLinear;
		default:					return MTLSamplerMinMagFilterLinear;
	}
}

static MTLSamplerAddressMode TranslateWrapMode(ESamplerAddressMode AddressMode)
{
	switch (AddressMode)
	{
		case AM_Clamp:	return MTLSamplerAddressModeClampToEdge;
		case AM_Mirror: return MTLSamplerAddressModeMirrorRepeat;
		case AM_Border: return MTLSamplerAddressModeClampToEdge;
		default:		return MTLSamplerAddressModeRepeat;
	}
}

static MTLCompareFunction TranslateCompareFunction(ECompareFunction CompareFunction)
{
	switch(CompareFunction)
	{
		case CF_Less:			return MTLCompareFunctionLess;
		case CF_LessEqual:		return MTLCompareFunctionLessEqual;
		case CF_Greater:		return MTLCompareFunctionGreater;
		case CF_GreaterEqual:	return MTLCompareFunctionGreaterEqual;
		case CF_Equal:			return MTLCompareFunctionEqual;
		case CF_NotEqual:		return MTLCompareFunctionNotEqual;
		case CF_Never:			return MTLCompareFunctionNever;
		default:				return MTLCompareFunctionAlways;
	};
}

static int32 TranslateSamplerCompareFunction(ESamplerCompareFunction SamplerComparisonFunction)
{
	switch(SamplerComparisonFunction)
	{
		case SCF_Less:		return 0;
		case SCF_Never:		return 0;
		default:			return 0;
	};
}

static MTLStencilOperation TranslateStencilOp(EStencilOp StencilOp)
{
	switch(StencilOp)
	{
		case SO_Zero:				return MTLStencilOperationZero;
		case SO_Replace:			return MTLStencilOperationReplace;
		case SO_SaturatedIncrement:	return MTLStencilOperationIncrementClamp;
		case SO_SaturatedDecrement:	return MTLStencilOperationDecrementClamp;
		case SO_Invert:				return MTLStencilOperationInvert;
		case SO_Increment:			return MTLStencilOperationIncrementWrap;
		case SO_Decrement:			return MTLStencilOperationDecrementWrap;
		default:					return MTLStencilOperationKeep;
	};
}






FMetalSamplerState::FMetalSamplerState(const FSamplerStateInitializerRHI& Initializer)
{
	MTLSamplerDescriptor* Desc = [[MTLSamplerDescriptor alloc] init];
	
	Desc.minFilter = Desc.magFilter = TranslateFilterMode(Initializer.Filter);
	Desc.mipFilter = TranslateMipFilterMode(Initializer.Filter);
	Desc.maxAnisotropy = GetMaxAnisotropy(Initializer.Filter, Initializer.MaxAnisotropy);
	Desc.sAddressMode = TranslateWrapMode(Initializer.AddressU);
	Desc.tAddressMode = TranslateWrapMode(Initializer.AddressV);
	Desc.rAddressMode = TranslateWrapMode(Initializer.AddressW);
	Desc.lodMinClamp = Initializer.MinMipLevel;
	Desc.lodMaxClamp = Initializer.MaxMipLevel;
	
	State = [FMetalManager::GetDevice() newSamplerStateWithDescriptor:Desc];
	[Desc release];
	TRACK_OBJECT(State);
}

FMetalSamplerState::~FMetalSamplerState()
{
	UNTRACK_OBJECT(State);
	[State release];
}

FMetalRasterizerState::FMetalRasterizerState(const FRasterizerStateInitializerRHI& Initializer)
{
	State = Initializer;
}

FMetalRasterizerState::~FMetalRasterizerState()
{
	
}

void FMetalRasterizerState::Set()
{
	FMetalManager::Get()->SetRasterizerState(State);
}

FMetalDepthStencilState::FMetalDepthStencilState(const FDepthStencilStateInitializerRHI& Initializer)
{
	MTLDepthStencilDescriptor* Desc = [[MTLDepthStencilDescriptor alloc] init];
	Desc.depthCompareFunction = TranslateCompareFunction(Initializer.DepthTest);
	Desc.depthWriteEnabled = Initializer.bEnableDepthWrite;
	
	if (Initializer.bEnableFrontFaceStencil)
	{
		// set up front face stencil operations
		MTLStencilDescriptor* Stencil = [[MTLStencilDescriptor alloc] init];
		Stencil.stencilCompareFunction = TranslateCompareFunction(Initializer.FrontFaceStencilTest);
		Stencil.stencilFailureOperation = TranslateStencilOp(Initializer.FrontFaceStencilFailStencilOp);
		Stencil.depthFailureOperation = TranslateStencilOp(Initializer.FrontFaceStencilFailStencilOp);
		Stencil.depthStencilPassOperation = TranslateStencilOp(Initializer.FrontFacePassStencilOp);
		Stencil.readMask = Initializer.StencilReadMask;
		Stencil.writeMask = Initializer.StencilWriteMask;
		Desc.frontFaceStencil = Stencil;
		
		[Stencil release];
	}
	
	if (Initializer.bEnableFrontFaceStencil)
	{
		// set up back face stencil operations
		MTLStencilDescriptor* Stencil = [[MTLStencilDescriptor alloc] init];
		Stencil.stencilCompareFunction = TranslateCompareFunction(Initializer.BackFaceStencilTest);
		Stencil.stencilFailureOperation = TranslateStencilOp(Initializer.BackFaceStencilFailStencilOp);
		Stencil.depthFailureOperation = TranslateStencilOp(Initializer.BackFaceStencilFailStencilOp);
		Stencil.depthStencilPassOperation = TranslateStencilOp(Initializer.BackFacePassStencilOp);
		Stencil.readMask = Initializer.StencilReadMask;
		Stencil.writeMask = Initializer.StencilWriteMask;
		Desc.backFaceStencil = Stencil;
		
		[Stencil release];
	}
			
	// bake out the descriptor
	State = [FMetalManager::GetDevice() newDepthStencilStateWithDescriptor:Desc];
	TRACK_OBJECT(State);
	[Desc release];
	
	// cache some pipeline state info
	bIsDepthWriteEnabled = Initializer.bEnableDepthWrite;
	bIsStencilWriteEnabled = Initializer.bEnableFrontFaceStencil || Initializer.bEnableBackFaceStencil;
}

FMetalDepthStencilState::~FMetalDepthStencilState()
{
	UNTRACK_OBJECT(State);
	[State release];
}



void FMetalDepthStencilState::Set()
{
	//activate the state
	[FMetalManager::GetContext() setDepthStencilState:State];
	
	// update the pipeline state object
	FMetalManager::Get()->SetDepthStencilWriteEnabled(bIsDepthWriteEnabled, bIsStencilWriteEnabled);
}

FMetalBlendState::FMetalBlendState(const FBlendStateInitializerRHI& Initializer)
	: RenderTargetStates(Initializer)
{
}

FMetalBlendState::~FMetalBlendState()
{
// 	for(uint32 RenderTargetIndex = 0;RenderTargetIndex < MaxMetalRenderTargets; ++RenderTargetIndex)
// 	{
// 		UNTRACK_OBJECT(RenderTargetStates[RenderTargetIndex]);
// 		[RenderTargetStates[RenderTargetIndex] release];
// 	}
}

void FMetalBlendState::Set()
{
	// update the pipeline state, there's nothing else to do for blend state
	FMetalManager::Get()->SetBlendState(this);
}





FSamplerStateRHIRef FMetalDynamicRHI::RHICreateSamplerState(const FSamplerStateInitializerRHI& Initializer)
{
	return new FMetalSamplerState(Initializer);
}

FRasterizerStateRHIRef FMetalDynamicRHI::RHICreateRasterizerState(const FRasterizerStateInitializerRHI& Initializer)
{
    return new FMetalRasterizerState(Initializer);
}

FDepthStencilStateRHIRef FMetalDynamicRHI::RHICreateDepthStencilState(const FDepthStencilStateInitializerRHI& Initializer)
{
	return new FMetalDepthStencilState(Initializer);
}


FBlendStateRHIRef FMetalDynamicRHI::RHICreateBlendState(const FBlendStateInitializerRHI& Initializer)
{
	return new FMetalBlendState(Initializer);
}


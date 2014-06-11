// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalState.h: Metal state definitions.
=============================================================================*/

#pragma once

class FMetalSamplerState : public FRHISamplerState
{
public:
	
	/** 
	 * Constructor/destructor
	 */
	FMetalSamplerState(const FSamplerStateInitializerRHI& Initializer);
	~FMetalSamplerState();

	id <MTLSamplerState> State;
};

class FMetalRasterizerState : public FRHIRasterizerState
{
public:

	/**
	 * Constructor/destructor
	 */
	FMetalRasterizerState(const FRasterizerStateInitializerRHI& Initializer);
	~FMetalRasterizerState();

	/**
	 * Update the context with the rasterizer state
	 */
	void Set();
	
protected:
	FRasterizerStateInitializerRHI State;
};

class FMetalDepthStencilState : public FRHIDepthStencilState
{
public:

	/**
	 * Constructor/destructor
	 */
	FMetalDepthStencilState(const FDepthStencilStateInitializerRHI& Initializer);
	~FMetalDepthStencilState();
	
	/**
	 * Set as the active depth stencil state into the context
	 */
	void Set();
	
protected:
	id<MTLDepthStencilState> State;
	bool bIsDepthWriteEnabled;
	bool bIsStencilWriteEnabled;
	
};

class FMetalBlendState : public FRHIBlendState
{
public:

	/**
	 * Constructor/destructor
	 */
	FMetalBlendState(const FBlendStateInitializerRHI& Initializer);
	~FMetalBlendState();
	
	/**
	 * Set as the active blend states
	 */
	void Set();
	
	TStaticArray<MTLBlendDescriptor*, MaxSimultaneousRenderTargets> RenderTargetStates;
};

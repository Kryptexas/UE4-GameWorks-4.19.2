// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StaticBoundShaderState.h: Static bound shader state definitions.
=============================================================================*/

#ifndef __STATICBOUNDSHADERSTATE_H__
#define __STATICBOUNDSHADERSTATE_H__

/**
 * FGlobalBoundShaderState
 * 
 * Encapsulates a global bound shader state resource.
 */
class FGlobalBoundShaderStateResource : public FRenderResource
{
public:

	/** @return The list of global bound shader states. */
	static TLinkedList<FGlobalBoundShaderStateResource*>*& GetGlobalBoundShaderStateList();

	/** Initialization constructor. */
	ENGINE_API FGlobalBoundShaderStateResource();

	/** Destructor. */
	ENGINE_API virtual ~FGlobalBoundShaderStateResource();

	/**
	 * If this global bound shader state hasn't been initialized yet, initialize it.
	 * @return The bound shader state RHI.
	 */
	FBoundShaderStateRHIParamRef GetInitializedRHI(
		FVertexDeclarationRHIParamRef VertexDeclaration, 
		FVertexShaderRHIParamRef VertexShader, 
		FPixelShaderRHIParamRef PixelShader,
		FGeometryShaderRHIParamRef GeometryShader
		);

private:

	/** The cached bound shader state. */
	FBoundShaderStateRHIRef BoundShaderState;

	/** This resource's link in the list of global bound shader states. */
	TLinkedList<FGlobalBoundShaderStateResource*> GlobalListLink;

	// FRenderResource interface.
	ENGINE_API virtual void ReleaseRHI();
};

typedef TGlobalResource<FGlobalBoundShaderStateResource> FGlobalBoundShaderState;

/**
 * SetGlobalBoundShaderState - sets the global bound shader state, also creates and caches it if necessary
 *
 * @param BoundShaderState			- current bound shader state, will be updated if it wasn't a valid ref
 * @param VertexDeclaration			- the vertex declaration to use in creating the new bound shader state
 * @param VertexShader				- the vertex shader to use in creating the new bound shader state
 * @param PixelShader				- the pixel shader to use in creating the new bound shader state
 * @param GeometryShader			- the geometry shader to use in creating the new bound shader state (0 if not used)
 */
extern ENGINE_API void SetGlobalBoundShaderState(
	FGlobalBoundShaderState& BoundShaderState,
	FVertexDeclarationRHIParamRef VertexDeclaration,
	FShader* VertexShader,
	FShader* PixelShader,
	FShader* GeometryShader = 0
	);

#endif

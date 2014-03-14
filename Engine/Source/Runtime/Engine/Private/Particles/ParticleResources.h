// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ParticleResources.h: Declaration of global particle resources.
=============================================================================*/

#pragma once

#include "RenderResource.h"

/** The number of sprites to support per-instanced draw. */
enum { MAX_PARTICLES_PER_INSTANCE = 16 };
/** The size of the scratch vertex buffer. */
extern const int32 GParticleScratchVertexBufferSize;

/**
 * Vertex buffer containing texture coordinates for the four corners of a sprite.
 */
class FParticleTexCoordVertexBuffer : public FVertexBuffer
{
public:
	virtual void InitRHI() OVERRIDE;
};

/** Global particle texture coordinate vertex buffer. */
extern TGlobalResource<FParticleTexCoordVertexBuffer> GParticleTexCoordVertexBuffer;

/**
 * Index buffer for drawing an individual sprite.
 */
class FParticleIndexBuffer : public FIndexBuffer
{
public:
	virtual void InitRHI() OVERRIDE;
};

/** Global particle index buffer. */
extern TGlobalResource<FParticleIndexBuffer> GParticleIndexBuffer;

typedef FShaderResourceViewRHIParamRef FParticleShaderParamRef;
typedef FVertexBufferRHIParamRef FParticleBufferParamRef;

/**
 * Scratch vertex buffer available for dynamic draw calls.
 */
class FParticleScratchVertexBuffer : public FVertexBuffer
{
public:
	/** SRV in to the buffer as an array of FVector2D values. */
	FShaderResourceViewRHIRef VertexBufferSRV_G32R32F;
	
	FParticleShaderParamRef GetShaderParam();
	FParticleBufferParamRef GetBufferParam();

	virtual void InitRHI() OVERRIDE;
	virtual void ReleaseRHI() OVERRIDE;
};

/** The global scratch vertex buffer. */
extern TGlobalResource<FParticleScratchVertexBuffer> GParticleScratchVertexBuffer;

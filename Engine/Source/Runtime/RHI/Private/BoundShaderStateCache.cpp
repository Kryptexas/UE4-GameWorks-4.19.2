// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	BoundShaderStateCache.cpp: Bound shader state cache implementation.
=============================================================================*/

#include "RHI.h"
#include "BoundShaderStateCache.h"

typedef TMap<FBoundShaderStateKey,FCachedBoundShaderStateLink*> FBoundShaderStateCache;

/** Lazily initialized bound shader state cache singleton. */
static FBoundShaderStateCache& GetBoundShaderStateCache()
{
	static FBoundShaderStateCache BoundShaderStateCache;
	return BoundShaderStateCache;
}

FCachedBoundShaderStateLink::FCachedBoundShaderStateLink(
	FVertexDeclarationRHIParamRef VertexDeclaration,
	FVertexShaderRHIParamRef VertexShader,
	FPixelShaderRHIParamRef PixelShader,
	FHullShaderRHIParamRef HullShader,
	FDomainShaderRHIParamRef DomainShader,
	FGeometryShaderRHIParamRef GeometryShader,
	FBoundShaderStateRHIParamRef InBoundShaderState
	):
	BoundShaderState(InBoundShaderState),
	Key(VertexDeclaration,VertexShader,PixelShader,HullShader,DomainShader,GeometryShader)
{
	// Add this bound shader state to the cache.
	GetBoundShaderStateCache().Add(Key,this);
}

FCachedBoundShaderStateLink::FCachedBoundShaderStateLink(
	FVertexDeclarationRHIParamRef VertexDeclaration,
	FVertexShaderRHIParamRef VertexShader,
	FPixelShaderRHIParamRef PixelShader,
	FBoundShaderStateRHIParamRef InBoundShaderState
	):
	BoundShaderState(InBoundShaderState),
	Key(VertexDeclaration,VertexShader,PixelShader)
{
	// Add this bound shader state to the cache.
	GetBoundShaderStateCache().Add(Key,this);
}

FCachedBoundShaderStateLink::~FCachedBoundShaderStateLink()
{
	GetBoundShaderStateCache().Remove(Key);
}

FCachedBoundShaderStateLink* GetCachedBoundShaderState(
	FVertexDeclarationRHIParamRef VertexDeclaration,
	FVertexShaderRHIParamRef VertexShader,
	FPixelShaderRHIParamRef PixelShader,
	FHullShaderRHIParamRef HullShader,
	FDomainShaderRHIParamRef DomainShader,
	FGeometryShaderRHIParamRef GeometryShader
	)
{
	// Find the existing bound shader state in the cache.
	return GetBoundShaderStateCache().FindRef(
		FBoundShaderStateKey(VertexDeclaration,VertexShader,PixelShader,HullShader,DomainShader,GeometryShader)
		);
}

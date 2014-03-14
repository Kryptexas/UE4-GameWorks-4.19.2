// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MorphMesh.cpp: Unreal morph target mesh and blending implementation.
=============================================================================*/

#include "EnginePrivate.h"

UVertexAnimBase::UVertexAnimBase(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

//////////////////////////////////////////////////////////////////////////

UMorphTarget::UMorphTarget(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}


void UMorphTarget::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );
	FStripDataFlags StripFlags( Ar );
	if( !StripFlags.IsDataStrippedForServer() )
	{
		Ar << MorphLODModels;
	}
}


SIZE_T UMorphTarget::GetResourceSize(EResourceSizeMode::Type Mode)
{
	return 0;
}




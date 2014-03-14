// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DestructibleMeshEditorPrivatePCH.h"

UDestructibleChunkParamsProxy::UDestructibleChunkParamsProxy(const class FPostConstructInitializeProperties& PCIP)
	:Super(PCIP)
{

}

void UDestructibleChunkParamsProxy::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	check(DestructibleMesh != NULL && DestructibleMesh->FractureSettings != NULL);

	if (DestructibleMesh->FractureSettings->ChunkParameters.Num() > ChunkIndex)
	{
		DestructibleMesh->FractureSettings->ChunkParameters[ChunkIndex] = ChunkParams;
	}
}

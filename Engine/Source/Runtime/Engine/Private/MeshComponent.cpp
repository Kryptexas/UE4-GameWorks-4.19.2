// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"


UMeshComponent::UMeshComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	CastShadow = true;
	bUseAsOccluder = true;
	bCanEverAffectNavigation = true;
}


void UMeshComponent::BeginDestroy()
{
	// Notify the streaming system.
	GStreamingManager->NotifyPrimitiveDetached( this );

	Super::BeginDestroy();
}

UMaterialInterface* UMeshComponent::GetMaterial(int32 ElementIndex) const
{
	if(ElementIndex < Materials.Num() && Materials[ElementIndex])
	{
		return Materials[ElementIndex];
	}
	else
	{
		return NULL;
	}
}

void UMeshComponent::SetMaterial(int32 ElementIndex,UMaterialInterface* Material)
{
	if (ElementIndex >= 0 && (Materials.Num() <= ElementIndex || Materials[ElementIndex] != Material))
	{
		if (Materials.Num() <= ElementIndex)
		{
			Materials.AddZeroed(ElementIndex + 1 - Materials.Num());
		}

		Materials[ElementIndex] = Material;
		MarkRenderStateDirty();

		if(BodyInstance.IsValidBodyInstance())
		{
			BodyInstance.UpdatePhysicalMaterials();
		}
	}
}

FMaterialRelevance UMeshComponent::GetMaterialRelevance() const
{
	// Combine the material relevance for all materials.
	FMaterialRelevance Result;
	for(int32 ElementIndex = 0;ElementIndex < GetNumMaterials();ElementIndex++)
	{
		UMaterialInterface const* MaterialInterface = GetMaterial(ElementIndex);
		if(!MaterialInterface)
		{
			MaterialInterface = UMaterial::GetDefaultMaterial(MD_Surface);
		}
		Result |= MaterialInterface->GetRelevance_Concurrent();
	}
	return Result;
}

int32 UMeshComponent::GetNumMaterials() const
{
	return 0;
}

void UMeshComponent::PrestreamTextures( float Seconds, bool bPrioritizeCharacterTextures, int32 CinematicTextureGroups )
{
	// If requested, tell the streaming system to only process character textures for 30 frames.
	if ( bPrioritizeCharacterTextures )
	{
		GStreamingManager->SetDisregardWorldResourcesForFrames( 30 );
	}

	int32 NumElements = GetNumMaterials();
	for ( int32 ElementIndex=0; ElementIndex < NumElements; ++ElementIndex )
	{
		UMaterialInterface* Material = GetMaterial( ElementIndex );
		if ( Material )
		{
			Material->SetForceMipLevelsToBeResident( false, false, Seconds, CinematicTextureGroups );
		}
	}
}


void UMeshComponent::SetTextureForceResidentFlag( bool bForceMiplevelsToBeResident )
{
	const int32 CinematicTextureGroups = 0;

	int32 NumElements = GetNumMaterials();
	for ( int32 ElementIndex=0; ElementIndex < NumElements; ++ElementIndex )
	{
		UMaterialInterface* Material = GetMaterial( ElementIndex );
		if ( Material )
		{
			Material->SetForceMipLevelsToBeResident( true, bForceMiplevelsToBeResident, -1.0f, CinematicTextureGroups );
		}
	}
}

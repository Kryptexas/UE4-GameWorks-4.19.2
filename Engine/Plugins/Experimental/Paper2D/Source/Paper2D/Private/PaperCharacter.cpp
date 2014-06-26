// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PaperCharacter.cpp: APaperCharacter implementation
	A character that uses a paper sprite.
=============================================================================*/

#include "Paper2DPrivatePCH.h"


FName APaperCharacter::SpriteComponentName(TEXT("Sprite0"));

APaperCharacter::APaperCharacter(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Prevent creation of mesh
	PCIP.DoNotCreateDefaultSubobject(ACharacter::MeshComponentName);
	
	Sprite = PCIP.CreateOptionalDefaultSubobject<UPaperAnimatedRenderComponent>(this, APaperCharacter::SpriteComponentName);
	if (Sprite)
	{
		Sprite->AlwaysLoadOnClient = true;
		Sprite->AlwaysLoadOnServer = true;
		Sprite->bOwnerNoSee = false;
		//Mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPose;
		//Mesh->bCastDynamicShadow = true;
		Sprite->bAffectDynamicIndirectLighting = true;
		Sprite->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		// force tick after movement component updates
		if (CharacterMovement)
		{
			Sprite->PrimaryComponentTick.AddPrerequisite(this, CharacterMovement->PrimaryComponentTick);
		}
		Sprite->AttachParent = CapsuleComponent;
		static FName CollisionProfileName(TEXT("CharacterMesh"));
		Sprite->SetCollisionProfileName(CollisionProfileName);
		Sprite->bGenerateOverlapEvents = false;
	}
}



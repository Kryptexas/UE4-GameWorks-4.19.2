// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PrimitiveComponent2D.h"

//////////////////////////////////////////////////////////////////////////
// UPrimitiveComponent2D

UPrimitiveComponent2D::UPrimitiveComponent2D(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
}

void UPrimitiveComponent2D::CreatePhysicsState()
{
	Super::CreatePhysicsState();
	CreatePhysicsState2D();
}

void UPrimitiveComponent2D::DestroyPhysicsState()
{
	Super::DestroyPhysicsState();
	DestroyPhysicsState2D();
}

void UPrimitiveComponent2D::CreatePhysicsState2D()
{
	if (!BodyInstance2D.IsValidBodyInstance())
	{
		if (UBodySetup2D* BodySetup = GetBodySetup2D())
		{
			BodyInstance2D.InitBody(BodySetup, ComponentToWorld, this);
		}
	}
}

void UPrimitiveComponent2D::DestroyPhysicsState2D()
{
	// clean up physics engine representation
	if (BodyInstance2D.IsValidBodyInstance())
	{
		BodyInstance2D.TermBody();
	}
}

void UPrimitiveComponent2D::OnUpdateTransform(bool bSkipPhysicsMove)
{
	Super::OnUpdateTransform(bSkipPhysicsMove);

	// Always send new transform to physics
	if (bPhysicsStateCreated && !bSkipPhysicsMove)
	{
		// @todo UE4 rather than always pass false, this function should know if it is being teleported or not!
		SendPhysicsTransform(false);

		BodyInstance2D.SetBodyTransform(ComponentToWorld);
	}
}

bool UPrimitiveComponent2D::IsSimulatingPhysics(FName BoneName) const
{
	if (const FBodyInstance2D* BodyInst2D = GetBodyInstance2D(BoneName))
	{
		return BodyInst2D->IsInstanceSimulatingPhysics();
	}
	else
	{
		return Super::IsSimulatingPhysics(BoneName);
	}
}

void UPrimitiveComponent2D::SetSimulatePhysics(bool bSimulate)
{
	Super::SetSimulatePhysics(bSimulate);
	BodyInstance2D.SetInstanceSimulatePhysics(bSimulate);
}

class UBodySetup2D* UPrimitiveComponent2D::GetBodySetup2D() const
{
	return nullptr;
}

const FBodyInstance2D* UPrimitiveComponent2D::GetBodyInstance2D(FName BoneName) const
{
	return &BodyInstance2D;
}

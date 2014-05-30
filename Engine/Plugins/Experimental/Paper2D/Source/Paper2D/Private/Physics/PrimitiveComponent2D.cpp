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

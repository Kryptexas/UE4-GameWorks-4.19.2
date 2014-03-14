// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IntSerialization.cpp: UObject class for testing serialization int types
=============================================================================*/

#include "EnginePrivate.h"
#include "Runtime/Engine/Classes/Engine/IntSerialization.h"

UIntSerialization::UIntSerialization(const class FPostConstructInitializeProperties& PCIP)
	: UObject(PCIP)
{
}

/*void UIntSerilization::Serialize (FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << UnsignedInt16Variable << UnsignedInt32Variable << UnsignedInt64Variable << SignedInt8Variable << SignedInt16Variable << SignedInt64Variable << UnsignedInt8Variable << SignedInt32Variable;
}*/

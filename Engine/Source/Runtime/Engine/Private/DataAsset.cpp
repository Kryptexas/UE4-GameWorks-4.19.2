// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UDataAsset::UDataAsset(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

#if WITH_EDITORONLY_DATA
void UDataAsset::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading() && (Ar.UE4Ver() < VER_UE4_ADD_TRANSACTIONAL_TO_DATA_ASSETS))
	{
		SetFlags(RF_Transactional);
	}
}
#endif

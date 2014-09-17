// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayPrediction.h"

bool FPredictionKey::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	if (Ar.IsLoading())
	{
		Ar << Current;
		if (Current > 0)
		{
			Ar << Base;
		}
		PredictiveConnection = Map;
	}
	else
	{
		/**
		 *	Only serialize the payload if we have no owning connection (Client sending to server).
		 *	or if the owning connection is this connection (Server only sends the prediction key to the client who gave it to us).
		 */	
		if (PredictiveConnection == nullptr || (Map == PredictiveConnection))
		{
			Ar << Current;
			if (Current > 0)
			{
				Ar << Base;
			}
		}
		else
		{
			int16 Payload = 0;
			Ar << Payload;
		}
	}

	bOutSuccess = true;
	return true;
}

void FPredictionKey::GenerateNewPredictionKey()
{
	static int32 GKey = 1;
	Current = GKey++;
	bIsStale = false;
}

void FPredictionKey::GenerateDependantPredictionKey()
{
	if (Base == 0)
	{
		Base = Current;
	}
	GenerateNewPredictionKey();
}

FPredictionKey FPredictionKey::CreateNewPredictionKey()
{
	FPredictionKey NewKey;
	NewKey.GenerateNewPredictionKey();
	return NewKey;
}
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
	static KeyType GKey = 1;
	Current = GKey++;
	bIsStale = false;
}

void FPredictionKey::GenerateDependantPredictionKey()
{
	KeyType Previous = 0;
	if (Base == 0)
	{
		Base = Current;
	}
	else
	{
		Previous = Current;
	}

	GenerateNewPredictionKey();

	if (Previous > 0)
	{
		FPredictionKeyDelegates::AddDependancy(Current, Previous);
	}
}

FPredictionKey FPredictionKey::CreateNewPredictionKey()
{
	FPredictionKey NewKey;
	NewKey.GenerateNewPredictionKey();
	return NewKey;
}

FPredictionKeyEvent& FPredictionKey::NewRejectedDelegate()
{
	return FPredictionKeyDelegates::NewRejectedDelegate(Current);
}

FPredictionKeyEvent& FPredictionKey::NewCaughtUpDelegate()
{
	return FPredictionKeyDelegates::NewCaughtUpDelegate(Current);
}

// -------------------------------------

FPredictionKeyDelegates& FPredictionKeyDelegates::Get()
{
	static FPredictionKeyDelegates StaticMap;
	return StaticMap;
}

FPredictionKeyEvent& FPredictionKeyDelegates::NewRejectedDelegate(FPredictionKey::KeyType Key)
{
	TArray<FPredictionKeyEvent>& DelegateList = Get().DelegateMap.FindOrAdd(Key).RejectedDelegates;
	DelegateList.Add(FPredictionKeyEvent());
	return DelegateList.Top();
}

FPredictionKeyEvent& FPredictionKeyDelegates::NewCaughtUpDelegate(FPredictionKey::KeyType Key)
{
	TArray<FPredictionKeyEvent>& DelegateList = Get().DelegateMap.FindOrAdd(Key).CaughtUpDelegates;
	DelegateList.Add(FPredictionKeyEvent());
	return DelegateList.Top();

}

void FPredictionKeyDelegates::BroadcastRejectedDelegate(FPredictionKey::KeyType Key)
{
	TArray<FPredictionKeyEvent>& DelegateList = Get().DelegateMap.FindOrAdd(Key).RejectedDelegates;
	for (auto Delegate : DelegateList)
	{
		Delegate.ExecuteIfBound();
	}
}

void FPredictionKeyDelegates::BroadcastCaughtUpDelegate(FPredictionKey::KeyType Key)
{
	TArray<FPredictionKeyEvent>& DelegateList = Get().DelegateMap.FindOrAdd(Key).CaughtUpDelegates;
	for (auto Delegate : DelegateList)
	{
		Delegate.ExecuteIfBound();
	}
}

void FPredictionKeyDelegates::Reject(FPredictionKey::KeyType Key)
{
	FDelegates* DelPtr = Get().DelegateMap.Find(Key);
	if (DelPtr)
	{
		for (auto Delegate : DelPtr->RejectedDelegates)
		{
			Delegate.ExecuteIfBound();
		}

		DelPtr->RejectedDelegates.Empty();
	}
}

void FPredictionKeyDelegates::CatchUpTo(FPredictionKey::KeyType Key)
{
	for (auto MapIt = Get().DelegateMap.CreateIterator(); MapIt; ++MapIt)
	{
		if (MapIt.Key() <= Key)
		{		
			for (auto Delegate : MapIt.Value().CaughtUpDelegates)
			{
				Delegate.ExecuteIfBound();
			}
			
			MapIt.RemoveCurrent();
		}
	}
}

void FPredictionKeyDelegates::CaughtUp(FPredictionKey::KeyType Key)
{
	FDelegates* DelPtr = Get().DelegateMap.Find(Key);
	if (DelPtr)
	{
		for (auto Delegate : DelPtr->CaughtUpDelegates)
		{
			Delegate.ExecuteIfBound();
		}
		Get().DelegateMap.Remove(Key);
	}
}

void FPredictionKeyDelegates::AddDependancy(FPredictionKey::KeyType ThisKey, FPredictionKey::KeyType DependsOn)
{
	NewRejectedDelegate(DependsOn).BindStatic(&FPredictionKeyDelegates::Reject, ThisKey);
	NewCaughtUpDelegate(DependsOn).BindStatic(&FPredictionKeyDelegates::CaughtUp, ThisKey);
}
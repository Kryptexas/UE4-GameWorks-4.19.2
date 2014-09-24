// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayAbility.h"
#include "AbilitySystemComponent.h"
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

void FPredictionKey::NewRejectOrCaughtUpDelegate(FPredictionKeyEvent Event)
{
	FPredictionKeyDelegates::NewRejectOrCaughtUpDelegate(Current, Event);
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

void FPredictionKeyDelegates::NewRejectOrCaughtUpDelegate(FPredictionKey::KeyType Key, FPredictionKeyEvent NewEvent)
{
	FDelegates& Delegates = Get().DelegateMap.FindOrAdd(Key);
	Delegates.CaughtUpDelegates.Add(NewEvent);
	Delegates.RejectedDelegates.Add(NewEvent);
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

		Get().DelegateMap.Remove(Key);
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

// -------------------------------------

FScopedPredictionWindow::FScopedPredictionWindow(UAbilitySystemComponent* AbilitySystemComponent, FPredictionKey InPredictionKey)
{
	Owner = AbilitySystemComponent;
	Owner->ScopedPedictionKey = InPredictionKey;
	ClearScopedPredictionKey = true;

}

FScopedPredictionWindow::FScopedPredictionWindow(UGameplayAbility* GameplayAbilityInstance)
{
	Ability = GameplayAbilityInstance;
	check(Ability.IsValid());

	Owner = GameplayAbilityInstance->GetCurrentActorInfo()->AbilitySystemComponent.Get();
	check(Owner.IsValid());

	ClearScopedPredictionKey = false;

	FGameplayAbilityActivationInfo& ActivationInfo = Ability->GetCurrentActivationInfoRef();


	if (ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Authority)
	{
		ActivationInfo.SetPredictionKey(Owner->ScopedPedictionKey);
	}
	else
	{
		ClientPrevActivationMode = static_cast<int8>(ActivationInfo.ActivationMode);

		if (Owner->ScopedPedictionKey.IsValidKey())
		{
			ActivationInfo.SetPredictionKey(Owner->ScopedPedictionKey);
			ActivationInfo.ActivationMode = EGameplayAbilityActivationMode::Predicting;
		}
		else
		{
			ActivationInfo.GenerateNewPredictionKey();
			ScopedPredictionKey = ActivationInfo.GetPredictionKeyForNewAction();
		}
	}
}

FScopedPredictionWindow::~FScopedPredictionWindow()
{
	if (Ability.IsValid())
	{
		// Restore old activation info settings
		FGameplayAbilityActivationInfo& ActivationInfo = Ability->GetCurrentActivationInfoRef();
		if (ActivationInfo.ActivationMode != EGameplayAbilityActivationMode::Authority)
		{
			ActivationInfo.ActivationMode = static_cast< TEnumAsByte<EGameplayAbilityActivationMode::Type> > (ClientPrevActivationMode);
		}

		ActivationInfo.SetPredictionStale();
	}

	if (ClearScopedPredictionKey && Owner.IsValid())
	{
		Owner->ScopedPedictionKey = FPredictionKey();
	}
}

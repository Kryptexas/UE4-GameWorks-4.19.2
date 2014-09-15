// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayAbilityTargetTypes.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"


TArray<FActiveGameplayEffectHandle> FGameplayAbilityTargetData::ApplyGameplayEffect(UGameplayEffect* GameplayEffect, const FGameplayAbilityActorInfo InstigatorInfo, float Level)
{
	// TODO: Improve relationship between InstigatorContext and FGameplayAbilityTargetData/FHitResult (or use something different between HitResult)


	FGameplayEffectSpec	SpecToApply(GameplayEffect,					// The UGameplayEffect data asset
		InstigatorInfo.Actor.Get(),		// The actor who instigated this
		Level,							// FIXME: Leveling
		NULL							// FIXME: CurveData override... should we just remove this?
		);
	if (HasHitResult())
	{
		SpecToApply.InstigatorContext.AddHitResult(*GetHitResult());
	}

	if (HasOrigin())
	{
		SpecToApply.InstigatorContext.AddOrigin(GetOrigin().GetLocation());
	}

	TArray<TWeakObjectPtr<AActor> > Actors = GetActors();
	TArray<FActiveGameplayEffectHandle>	AppliedHandles;
	AppliedHandles.Reserve(Actors.Num());

	for (TWeakObjectPtr<AActor> TargetActor : Actors)
	{
		if (TargetActor.IsValid())
		{
			UAbilitySystemComponent* TargetComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor.Get());
			if (TargetComponent)
			{
				AppliedHandles.Add( InstigatorInfo.AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(SpecToApply, TargetComponent) );
			}
		}
	}

	return AppliedHandles;
}

FString FGameplayAbilityTargetData::ToString() const
{
	return TEXT("BASE CLASS");
}


FGameplayAbilityTargetDataHandle FGameplayAbilityTargetingLocationInfo::MakeTargetDataHandleFromHitResult(TWeakObjectPtr<UGameplayAbility> Ability, FHitResult HitResult) const
{
	if (LocationType == EGameplayAbilityTargetingLocationType::Type::SocketTransform)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability.IsValid() ? Ability.Get()->GetCurrentActorInfo() : NULL;
		AActor* AISourceActor = ActorInfo ? (ActorInfo->Actor.IsValid() ? ActorInfo->Actor.Get() : NULL) : NULL;
		UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->AnimInstance.Get() : NULL;
		USkeletalMeshComponent* AISourceComponent = AnimInstance ? AnimInstance->GetOwningComponent() : NULL;

		if (AISourceActor && AISourceComponent)
		{
			FGameplayAbilityTargetData_Mesh* ReturnData = new FGameplayAbilityTargetData_Mesh();
			ReturnData->SourceActor = AISourceActor;
			ReturnData->SourceComponent = AISourceComponent;
			ReturnData->SourceSocketName = SourceSocketName;
			ReturnData->TargetPoint = HitResult.Location;
			return FGameplayAbilityTargetDataHandle(ReturnData);
		}
	}

	/** Note: These are cleaned up by the FGameplayAbilityTargetDataHandle (via an internal TSharedPtr) */
	FGameplayAbilityTargetData_SingleTargetHit* ReturnData = new FGameplayAbilityTargetData_SingleTargetHit();
	ReturnData->HitResult = HitResult;
	return FGameplayAbilityTargetDataHandle(ReturnData);
}

FGameplayAbilityTargetDataHandle FGameplayAbilityTargetingLocationInfo::MakeTargetDataHandleFromActors(TArray<TWeakObjectPtr<AActor> > TargetActors, bool OneActorPerHandle) const
{
	/** Note: This is cleaned up by the FGameplayAbilityTargetDataHandle (via an internal TSharedPtr) */
	FGameplayAbilityTargetData_ActorArray* ReturnData = new FGameplayAbilityTargetData_ActorArray();
	FGameplayAbilityTargetDataHandle ReturnDataHandle = FGameplayAbilityTargetDataHandle(ReturnData);
	ReturnData->SourceLocation = *this;
	if (OneActorPerHandle)
	{
		if (TargetActors[0].IsValid())
		{
			ReturnData->TargetActorArray.Add(TargetActors[0].Get());
		}

		for (int32 i = 1; i < TargetActors.Num(); ++i)
		{
			if (TargetActors[i].IsValid())
			{
				FGameplayAbilityTargetData_ActorArray* CurrentData = new FGameplayAbilityTargetData_ActorArray();
				FGameplayAbilityTargetDataHandle CurrentDataHandle = FGameplayAbilityTargetDataHandle(CurrentData);
				CurrentData->SourceLocation = *this;
				CurrentData->TargetActorArray.Add(TargetActors[i].Get());
				ReturnDataHandle.Append(&CurrentDataHandle);
			}
		}
	}
	else
	{
		ReturnData->TargetActorArray = TargetActors;
	}
	return ReturnDataHandle;
}

bool FGameplayAbilityTargetDataHandle::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	int32 DataNum;
	if (Ar.IsSaving())
	{
		DataNum = Data.Num();
	}
	Ar << DataNum;
	if (Ar.IsLoading())
	{
		Data.SetNumZeroed(DataNum);
	}

	for (int32 i = 0; i < DataNum; ++i)
	{
		UScriptStruct* ScriptStruct = Data[i].IsValid() ? Data[i]->GetScriptStruct() : NULL;
		Ar << ScriptStruct;

		if (ScriptStruct)
		{
			if (Ar.IsLoading())
			{
				// For now, just always reset/reallocate the data when loading.
				// Longer term if we want to generalize this and use it for property replication, we should support
				// only reallocating when necessary
				check(!Data[i].IsValid());

				FGameplayAbilityTargetData * NewData = (FGameplayAbilityTargetData*)FMemory::Malloc(ScriptStruct->GetCppStructOps()->GetSize());
				ScriptStruct->InitializeScriptStruct(NewData);

				Data[i] = TSharedPtr<FGameplayAbilityTargetData>(NewData);
			}

			void* ContainerPtr = Data[i].Get();

			if (ScriptStruct->StructFlags & STRUCT_NetSerializeNative)
			{
				ScriptStruct->GetCppStructOps()->NetSerialize(Ar, Map, bOutSuccess, Data[i].Get());
			}
			else
			{
				// This won't work since UStructProperty::NetSerializeItem is deprecrated.
				//	1) we have to manually crawl through the topmost struct's fields since we don't have a UStructProperty for it (just the UScriptProperty)
				//	2) if there are any UStructProperties in the topmost struct's fields, we will assert in UStructProperty::NetSerializeItem.

				ABILITY_LOG(Fatal, TEXT("FGameplayAbilityTargetDataHandle::NetSerialize called on data struct %s without a native NetSerialize"), *ScriptStruct->GetName());

				for (TFieldIterator<UProperty> It(ScriptStruct); It; ++It)
				{
					if (It->PropertyFlags & CPF_RepSkip)
					{
						continue;
					}

					void * PropertyData = It->ContainerPtrToValuePtr<void*>(ContainerPtr);

					It->NetSerializeItem(Ar, Map, PropertyData);
				}
			}
		}
	}

	//ABILITY_LOG(Warning, TEXT("FGameplayAbilityTargetDataHandle Serialized: %s"), ScriptStruct ? *ScriptStruct->GetName() : TEXT("NULL") );

	bOutSuccess = true;
	return true;
}
bool FGameplayAbilityTargetingLocationInfo::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << LocationType;

	switch (LocationType)
	{
	case EGameplayAbilityTargetingLocationType::ActorTransform:
		Ar << SourceActor;
		break;
	case EGameplayAbilityTargetingLocationType::SocketTransform:
		Ar << SourceComponent;
		Ar << SourceSocketName;
		break;
	case EGameplayAbilityTargetingLocationType::LiteralTransform:
		Ar << LiteralTransform;
		break;
	default:
		check(false);		//This case should not happen
		break;
	}

	bOutSuccess = true;
	return true;
}

bool FGameplayAbilityTargetData_LocationInfo::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	SourceLocation.NetSerialize(Ar, Map, bOutSuccess);
	TargetLocation.NetSerialize(Ar, Map, bOutSuccess);

	bOutSuccess = true;
	return true;
}

bool FGameplayAbilityTargetData_ActorArray::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	SourceLocation.NetSerialize(Ar, Map, bOutSuccess);
	Ar << TargetActorArray;

	bOutSuccess = true;
	return true;
}

bool FGameplayAbilityTargetData_Mesh::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	//SourceActor can be used as a backup if the component isn't found.
	Ar << SourceActor;
	Ar << SourceComponent;
	Ar << SourceSocketName;
	TargetPoint.NetSerialize(Ar, Map, bOutSuccess);

	bOutSuccess = true;
	return true;
}

bool FGameplayAbilityTargetData_SingleTargetHit::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << HitResult.Actor;

	HitResult.TraceStart.NetSerialize(Ar, Map, bOutSuccess);
	HitResult.Location.NetSerialize(Ar, Map, bOutSuccess);
	HitResult.Normal.NetSerialize(Ar, Map, bOutSuccess);

	bOutSuccess = true;
	return true;
}

bool FGameplayAbilityTargetData_Radius::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Actors;	// Fixme: will this go through the package map properly?
	Ar << Origin;

	return true;
}

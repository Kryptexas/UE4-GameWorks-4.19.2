// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

#include "AnimNotifyQueue.generated.h"

class USkeletalMeshComponent;
struct FAnimInstanceProxy;
struct FAnimNotifyEvent;

USTRUCT()
struct FAnimNotifyEventReference
{
	GENERATED_BODY()

	FAnimNotifyEventReference()
		: Notify(nullptr)
		, NotifySource(nullptr)
	{}

	FAnimNotifyEventReference(const FAnimNotifyEventReference& rhs)
		: Notify(rhs.Notify)
		, NotifySource(rhs.NotifySource)
	{

	}

	FAnimNotifyEventReference(const FAnimNotifyEvent* InNotify, const UObject* InNotifySource)
		: Notify(InNotify)
		, NotifySource(InNotifySource)
	{}

	const FAnimNotifyEvent* GetNotify() const
	{
		return NotifySource ? Notify : nullptr;
	}

	friend bool operator==(const FAnimNotifyEventReference& Lhs, const FAnimNotifyEventReference& Rhs)
	{
		return Lhs.Notify == Rhs.Notify;
	}

	friend bool operator==(const FAnimNotifyEventReference& Lhs, const FAnimNotifyEvent& Rhs);

private:

	const FAnimNotifyEvent* Notify;

	UPROPERTY(transient)
	const UObject* NotifySource;
};

USTRUCT()
struct FAnimNotifyArray
{
	GENERATED_BODY()

	UPROPERTY(transient)
	TArray<FAnimNotifyEventReference> Notifies;
};

USTRUCT()
struct FAnimNotifyQueue
{
	GENERATED_BODY()

	FAnimNotifyQueue()
		: PredictedLODLevel(-1)
	{
		RandomStream.Initialize(0x05629063);
	}

	/** Should the notifies current filtering mode stop it from triggering */
	bool PassesFiltering(const FAnimNotifyEvent* Notify) const;

	/** Work out whether this notify should be triggered based on its chance of triggering value */
	bool PassesChanceOfTriggering(const FAnimNotifyEvent* Event) const;

	/** Add notify to queue*/
	void AddAnimNotify(const FAnimNotifyEvent* Notify, const UObject* NotifySource);

	/** Add anim notifies **/
	void AddAnimNotifies(bool bSrcIsLeader, const TArray<FAnimNotifyEventReference>& NewNotifies, const float InstanceWeight);

	/** Add anim notifies from montage**/
	void AddAnimNotifies(bool bSrcIsLeader, const TMap<FName, TArray<FAnimNotifyEventReference>>& NewNotifies, const float InstanceWeight);

	/** Wrapper functions for when we aren't coming from a sync group **/
	void AddAnimNotifies(const TArray<FAnimNotifyEventReference>& NewNotifies, const float InstanceWeight) { AddAnimNotifies(true, NewNotifies, InstanceWeight); }
	void AddAnimNotifies(const TMap<FName, TArray<FAnimNotifyEventReference>>& NewNotifies, const float InstanceWeight) { AddAnimNotifies(true, NewNotifies, InstanceWeight); }

	/** Reset queue & update LOD level */
	void Reset(USkeletalMeshComponent* Component);

	/** Append one queue to another */
	void Append(const FAnimNotifyQueue& Queue);
	
	/** 
	 *	Best LOD that was 'predicted' by UpdateSkelPose. Copied form USkeletalMeshComponent.
	 *	This is what bones were updated based on, so we do not allow rendering at a better LOD than this. 
	 */
	int32 PredictedLODLevel;

	/** Internal random stream */
	FRandomStream RandomStream;

	/** Animation Notifies that has been triggered in the latest tick **/
	UPROPERTY(transient)
	TArray<FAnimNotifyEventReference> AnimNotifies;

	/** Animation Notifies from montages that still need to be filtered by slot weight*/
	UPROPERTY(transient)
	TMap<FName, FAnimNotifyArray> UnfilteredMontageAnimNotifies;

	/** Takes the cached notifies from playing montages and adds them if they pass a slot weight check */
	void ApplyMontageNotifies(const FAnimInstanceProxy& Proxy);
private:
	/** Implementation for adding notifies*/
	void AddAnimNotifiesToDest(bool bSrcIsLeader, const TArray<FAnimNotifyEventReference>& NewNotifies, TArray<FAnimNotifyEventReference>& DestArray, const float InstanceWeight);

	/** Adds the contents of the NewNotifies array to the DestArray (maintaining uniqueness of notify states*/
	void AddAnimNotifiesToDestNoFiltering(const TArray<FAnimNotifyEventReference>& NewNotifies, TArray<FAnimNotifyEventReference>& DestArray) const;
};

// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Object.h"
#include "LiveLinkRefSkeleton.h"

#include "LiveLinkMessages.generated.h"

//Is this still needed?
UCLASS()
class UDummyObject : public UObject
{
	GENERATED_BODY()
	
	UPROPERTY()
	int32 test;
};

USTRUCT()
struct FLiveLinkSubjectData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FLiveLinkRefSkeleton RefSkeleton;

	UPROPERTY()
	FString SubjectName;

	UPROPERTY()
	int32 BoneID;
};

USTRUCT()
struct FLiveLinkSubjectFrame
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FTransform> Transforms;

	UPROPERTY()
	int32 BoneID;
};

USTRUCT()
struct FLiveLinkPing
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGuid PollRequest;

	// default constructor for the receiver
	FLiveLinkPing() = default;

	FLiveLinkPing(const FGuid& InPollRequest) : PollRequest(InPollRequest) {}
};

USTRUCT()
struct FLiveLinkPong
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString ProviderName;

	UPROPERTY()
	FString MachineName;

	UPROPERTY()
	FGuid PollRequest;

	// default constructor for the receiver
	FLiveLinkPong() = default;

	FLiveLinkPong(const FString& InProviderName, const FString& InMachineName, const FGuid& InPollRequest) : ProviderName(InProviderName), MachineName(InMachineName), PollRequest(InPollRequest) {}
};

USTRUCT()
struct FLiveLinkConnect
{
	GENERATED_USTRUCT_BODY()
};

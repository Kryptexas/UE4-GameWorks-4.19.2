// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TurnBasedMatchInterface.generated.h"

class IOnlineSubsystem;

UINTERFACE(Blueprintable)
class UTurnBasedMatchInterface : public UInterface
{
	GENERATED_BODY()
public:
	UTurnBasedMatchInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

class ITurnBasedMatchInterface
{
	GENERATED_BODY()
public:

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Online|TurnBased")
	void OnMatchReceivedTurn(const FString& Match, bool bDidBecomeActive);
};

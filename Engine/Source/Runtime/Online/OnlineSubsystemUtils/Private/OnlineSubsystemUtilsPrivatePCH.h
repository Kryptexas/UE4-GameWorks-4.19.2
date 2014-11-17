// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "Engine.h"
#include "Sockets.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemUtilsModule.h"
#include "SocketSubsystem.h"
#include "ModuleManager.h"
#include "OnlineSubsystemUtilsClasses.h"

// Helper class for various methods to reduce the call hierarchy
struct FOnlineSubsystemBPCallHelper
{
public:
	FOnlineSubsystemBPCallHelper(const TCHAR* CallFunctionContext, FName SystemName = NAME_None);

	void QueryIDFromPlayerController(APlayerController* PlayerController);

	bool IsValid() const
	{
		return UserID.IsValid() && (OnlineSub != nullptr);
	}

public:
	TSharedPtr<class FUniqueNetId> UserID;
	IOnlineSubsystem* const OnlineSub;
	const TCHAR* FunctionContext;
};

#define INVALID_INDEX -1




// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"
#include "LeaderboardBlueprintLibrary.h"
#include "Runtime/Online/OnlineSubsystem/Public/Interfaces/OnlineLeaderboardInterface.h"

//////////////////////////////////////////////////////////////////////////
// ULeaderboardBlueprintLibrary

ULeaderboardBlueprintLibrary::ULeaderboardBlueprintLibrary(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

bool ULeaderboardBlueprintLibrary::WriteLeaderboardObject(APlayerController* PlayerController, class FOnlineLeaderboardWrite& WriteObject)
{
	UE_LOG(LogOnline, Display, TEXT("WriteLeaderboardObject site 1"));

	if (APlayerState* PlayerState = (PlayerController != NULL) ? PlayerController->PlayerState : NULL)
	{
		UE_LOG(LogOnline, Display, TEXT("WriteLeaderboardObject site 2"));

		TSharedPtr<FUniqueNetId> UserId = PlayerState->UniqueId.GetUniqueNetId();
		if (UserId.IsValid())
		{
			UE_LOG(LogOnline, Display, TEXT("WriteLeaderboardObject site 3"));

			if (IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get())
			{
				UE_LOG(LogOnline, Display, TEXT("WriteLeaderboardObject site 4"));

				IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
				if (Leaderboards.IsValid())
				{
					UE_LOG(LogOnline, Display, TEXT("WriteLeaderboardObject site 5"));

					// the call will copy the user id and write object to its own memory
					bool bResult = Leaderboards->WriteLeaderboards(PlayerState->SessionName, *UserId, WriteObject);

					UE_LOG(LogOnline, Display, TEXT("WriteLeaderboardObject site 6"));

					// Flush the leaderboard immediately for now
					bool bFlushResult = Leaderboards->FlushLeaderboards(PlayerState->SessionName);

					UE_LOG(LogOnline, Display, TEXT("WriteLeaderboardObject site 7"));

					return bResult && bFlushResult;
				}
				else
				{
					FFrame::KismetExecutionMessage(TEXT("WriteLeaderboardObject - Leaderboards not supported by Online Subsystem"), ELogVerbosity::Warning);
				}
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("WriteLeaderboardObject - Invalid or uninitialized OnlineSubsystem"), ELogVerbosity::Warning);
			}
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("WriteLeaderboardObject - Cannot map local player to unique net ID"), ELogVerbosity::Warning);
		}
	}
	else
	{
		FFrame::KismetExecutionMessage(TEXT("WriteLeaderboardObject - Invalid player state"), ELogVerbosity::Warning);
	}

	UE_LOG(LogOnline, Display, TEXT("WriteLeaderboardObject site 8"));
	return false;
}

bool ULeaderboardBlueprintLibrary::WriteLeaderboardInteger(APlayerController* PlayerController, FName StatName, int32 StatValue)
{
	FOnlineLeaderboardWrite WriteObject;
	WriteObject.LeaderboardNames.Add(StatName);
	WriteObject.RatedStat = StatName;
	WriteObject.DisplayFormat = ELeaderboardFormat::Number;
	WriteObject.SortMethod = ELeaderboardSort::Descending;
	WriteObject.UpdateMethod = ELeaderboardUpdateMethod::KeepBest;
	WriteObject.SetIntStat(StatName, StatValue);

	return WriteLeaderboardObject(PlayerController, WriteObject);
}

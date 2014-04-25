// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemGooglePlayPrivatePCH.h"
#include "OnlineLeaderboardInterfaceGooglePlay.h"
#include "Online.h"

FOnlineLeaderboardsGooglePlay::FOnlineLeaderboardsGooglePlay(FOnlineSubsystemGooglePlay* InSubsystem)
{
}

bool FOnlineLeaderboardsGooglePlay::ReadLeaderboards(const TArray< TSharedRef<FUniqueNetId> >& Players, FOnlineLeaderboardReadRef& ReadObject)
{
	TriggerOnLeaderboardReadCompleteDelegates(false);
	return false;
}

bool FOnlineLeaderboardsGooglePlay::ReadLeaderboardsForFriends(int32 LocalUserNum, FOnlineLeaderboardReadRef& ReadObject)
{
	TriggerOnLeaderboardReadCompleteDelegates(false);
	return false;
}

void FOnlineLeaderboardsGooglePlay::FreeStats(FOnlineLeaderboardRead& ReadObject)
{
	// iOS doesn't support this, and there is no Google Play functionality for this either
}

bool FOnlineLeaderboardsGooglePlay::WriteLeaderboards(const FName& SessionName, const FUniqueNetId& Player, FOnlineLeaderboardWrite& WriteObject)
{
	UE_LOG_ONLINE(Display, TEXT("WriteLeaderboards"));

	bool bWroteAnyLeaderboard = false;

	for(int32 LeaderboardIdx = 0; LeaderboardIdx < WriteObject.LeaderboardNames.Num(); ++LeaderboardIdx)
	{
		FString LeaderboardName = WriteObject.LeaderboardNames[LeaderboardIdx].ToString();
		if(LeaderboardName.Equals(TEXT("TestLeaderboard")))
		{
			LeaderboardName = TEXT("leaderboard_00"); //hack to work around leaderboard name mismatch between test module and the format that java call expects
		}
		UE_LOG_ONLINE(Display, TEXT("Going through stats for leaderboard :  %s "), *LeaderboardName);
		
		for(FStatPropertyArray::TConstIterator It(WriteObject.Properties); It; ++It)
		{
			const FVariantData& Stat = It.Value();
			uint64 Score;
			FOnlinePendingLeaderboardWrite UnreportedScore;

			UE_LOG_ONLINE(Display, TEXT("Here's a stat"));

			//Google leaderboard stats are always a long/int64
			if(Stat.GetType() == EOnlineKeyValuePairDataType::Int64)
			{
				Stat.GetValue(Score);
				
				FOnlinePendingLeaderboardWrite* UnreportedScore = new (UnreportedScores) FOnlinePendingLeaderboardWrite();
				UnreportedScore->LeaderboardName = LeaderboardName;
				UnreportedScore->Score = Score;
				UE_LOG_ONLINE(Display, TEXT("FOnlineLeaderboardsAndroid::WriteLeaderboards() Int64 value Score: %d"), Score);

				bWroteAnyLeaderboard = true;
			}
			else if (Stat.GetType() == EOnlineKeyValuePairDataType::Int32)
			{
				// cast from 32 to 64
				int32 Score32 = 0;
				Stat.GetValue(Score32);
				Score = static_cast<uint64>(Score32);

				FOnlinePendingLeaderboardWrite* UnreportedScore = new (UnreportedScores) FOnlinePendingLeaderboardWrite();
				UnreportedScore->LeaderboardName = LeaderboardName;
				UnreportedScore->Score = Score;
				UE_LOG_ONLINE(Display, TEXT("FOnlineLeaderboardsAndroid::WriteLeaderboards() Int32 value Score: %d "), Score);

				bWroteAnyLeaderboard = true;
			}
		}
	}
	
	//Return whether any stat was cached
	return bWroteAnyLeaderboard;
}

bool FOnlineLeaderboardsGooglePlay::FlushLeaderboards(const FName& SessionName)
{
	UE_LOG_ONLINE(Display, TEXT("flush leaderboards session name :%s"), *SessionName.ToString());

	bool Success = true;
	//@todo android: make ansynchronous?
	for(int32 Index = 0; Index < UnreportedScores.Num(); ++Index)
	{
		UE_LOG_ONLINE(Display, TEXT("Submitting an unreported score to %s. Value: %d "), *UnreportedScores[Index].LeaderboardName);

		extern void AndroidThunkCpp_WriteLeaderboardValue(const FString&, int64_t);
		AndroidThunkCpp_WriteLeaderboardValue(GetLeaderboardID(UnreportedScores[Index].LeaderboardName), UnreportedScores[Index].Score);
	}

	UnreportedScores.Empty();

	TriggerOnLeaderboardFlushCompleteDelegates(SessionName, Success);

	return true;
}

bool FOnlineLeaderboardsGooglePlay::WriteOnlinePlayerRatings(const FName& SessionName, int32 LeaderboardId, const TArray<FOnlinePlayerScore>& PlayerScores)
{
	//iOS doesn't support this, and there is no Google Play functionality for this either
	return false;
}

FString FOnlineLeaderboardsGooglePlay::GetLeaderboardID(const FString& LeaderboardName)
{
	auto DefaultSettings = GetDefault<UAndroidRuntimeSettings>();
	for(const auto& Mapping : DefaultSettings->LeaderboardMap)
	{
		if(Mapping.Name == LeaderboardName)
		{
			return Mapping.LeaderboardID;
		}
	}

	UE_LOG(LogOnline, Warning, TEXT( "GetLeaderboardID: No mapping for leaderboard %s"), *LeaderboardName );
	return LeaderboardName;
}

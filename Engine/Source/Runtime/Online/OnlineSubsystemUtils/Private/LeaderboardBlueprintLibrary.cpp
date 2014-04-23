// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"
#include "LeaderboardBlueprintLibrary.h"
#include "Runtime/Online/OnlineSubsystem/Public/Interfaces/OnlineLeaderboardInterface.h"


#if PLATFORM_ANDROID
/**
 * Class to store the leaderboard config.
 * This really belongs in the proper OSS, move it there when we bring the whole OSS online.
 */
class FLeaderboardsConfig : public FOnlineJsonSerializable
{
public:
	FJsonSerializableKeyValueMap LeaderboardMap;

	BEGIN_ONLINE_JSON_SERIALIZER
	ONLINE_JSON_SERIALIZE_MAP( "LeaderboardMap", LeaderboardMap );
	END_ONLINE_JSON_SERIALIZER
};

/**
 * Helper function to get the platform- and game-specific leaderboard ID from the JSON config file.
 * This really belongs in the proper OSS class, move it there when we bring the whole OSS online.
 */
FString GetLeaderboardID(const FString& LeaderboardName)
{
	// Hacks to initialize the config one
	static bool bHasReadConfig = false;
	static FLeaderboardsConfig LeaderboardConfig;

	if(!bHasReadConfig)
	{
		const FString JSonConfigFilename = FPaths::GameDir() + TEXT( "Config/OSS/GooglePlay/Leaderboards.json" );

		FString JSonText;

		if ( !FFileHelper::LoadFileToString( JSonText, *JSonConfigFilename ) )
		{
			UE_LOG(LogOnline, Warning, TEXT( "GetLeaderboardID: Failed to find json OSS leaderboard config: %s"), *JSonConfigFilename );
			return LeaderboardName;
		}

		if ( !LeaderboardConfig.FromJson( JSonText ) )
		{
			UE_LOG(LogOnline, Warning, TEXT( "GetLeaderboardID: Failed to parse json OSS leaderboard config: %s"), *JSonConfigFilename );
			return LeaderboardName;
		}

		bHasReadConfig = true;
	}

	FString * LeaderboardID = LeaderboardConfig.LeaderboardMap.Find( LeaderboardName );

	if(LeaderboardID == NULL)
	{
		UE_LOG(LogOnline, Warning, TEXT( "GetLeaderboardID: No mapping for leaderboard %s"), *LeaderboardName );
		return LeaderboardName;
	}

	return *LeaderboardID;
}
#endif

//////////////////////////////////////////////////////////////////////////
// ULeaderboardBlueprintLibrary

ULeaderboardBlueprintLibrary::ULeaderboardBlueprintLibrary(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

bool ULeaderboardBlueprintLibrary::WriteLeaderboardObject(APlayerController* PlayerController, class FOnlineLeaderboardWrite& WriteObject)
{
#if PLATFORM_ANDROID
	// Hack in Android leaderboard write since we don't have the full OSS yet.
	UE_LOG(LogOnline, Log, TEXT("Leaderboard stat name is %s"), *WriteObject.RatedStat.ToString());

	for (FStatPropertyArray::TConstIterator It(WriteObject.Properties); It; ++It)
	{
		// Access the stat and the value.
		const FVariantData& Stat = It.Value();

		FString LeaderboardName(It.Key().ToString());
		
		int32 Value = 0;

		switch (Stat.GetType())
		{
		case EOnlineKeyValuePairDataType::Int32:
			{
				Stat.GetValue(Value);
				UE_LOG(LogOnline, Log, TEXT("Writing Int32 leaderboard stat %s with value %d."), *LeaderboardName, Value);
				extern void AndroidThunkCpp_WriteLeaderboardValue(const FString&, int64_t);
				AndroidThunkCpp_WriteLeaderboardValue(GetLeaderboardID(LeaderboardName), Value);
				return true;
			}

		default:
			{
				UE_LOG(LogOnline, Warning, TEXT("ULeaderboardBlueprintLibrary::WriteLeaderboardObject(Leaderboard: %s) Invalid data type (only Int32 is currently supported)"), *LeaderboardName);
				break;
			}
		}
	}

	return false;

#else
	if (APlayerState* PlayerState = (PlayerController != NULL) ? PlayerController->PlayerState : NULL)
	{
		TSharedPtr<FUniqueNetId> UserId = PlayerState->UniqueId.GetUniqueNetId();
		if (UserId.IsValid())
		{
			if (IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get())
			{
				IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
				if (Leaderboards.IsValid())
				{
					// the call will copy the user id and write object to its own memory
					bool bResult = Leaderboards->WriteLeaderboards(PlayerState->SessionName, *UserId, WriteObject);

					// Flush the leaderboard immediately for now
					bool bFlushResult = Leaderboards->FlushLeaderboards(PlayerState->SessionName);

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

	return false;
#endif
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

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPrivatePCH.h"
#include "OnlineSessionSettings.h"

/**
 * Logs session properties used from the session settings
 *
 * @param NamedSession the session to log the information for
 */
void DumpNamedSession(const FNamedOnlineSession* NamedSession)
{
	if (NamedSession != NULL)
	{
		UE_LOG(LogOnline, Verbose, TEXT("dumping NamedSession: "));
		UE_LOG(LogOnline, Verbose, TEXT("	SessionName: %s"), *NamedSession->SessionName.ToString());	
		UE_LOG(LogOnline, Verbose, TEXT("	HostingPlayerNum: %d"), NamedSession->HostingPlayerNum);
		UE_LOG(LogOnline, Verbose, TEXT("	SessionState: %s"), EOnlineSessionState::ToString(NamedSession->SessionState));
		UE_LOG(LogOnline, Verbose, TEXT("	RegisteredPlayers: "));
		if (NamedSession->RegisteredPlayers.Num())
		{
			for (int32 UserIdx=0; UserIdx < NamedSession->RegisteredPlayers.Num(); UserIdx++)
			{
				UE_LOG(LogOnline, Verbose, TEXT("	    %d: %s"), UserIdx, *NamedSession->RegisteredPlayers[UserIdx]->ToDebugString());
			}
		}
		else
		{
			UE_LOG(LogOnline, Verbose, TEXT("	    0 registered players"));
		}

		DumpSession(NamedSession);
	}
}

/**
 * Logs session properties used from the session settings
 *
 * @param Session the session to log the information for
 */
void DumpSession(const FOnlineSession* Session)
{
	if (Session != NULL)
	{
		UE_LOG(LogOnline, Verbose, TEXT("dumping Session: "));
		UE_LOG(LogOnline, Verbose, TEXT("	OwningPlayerName: %s"), *Session->OwningUserName);	
		UE_LOG(LogOnline, Verbose, TEXT("	OwningPlayerId: %s"), *Session->OwningUserId->ToDebugString());
		UE_LOG(LogOnline, Verbose, TEXT("	NumOpenPrivateConnections: %d"), Session->NumOpenPrivateConnections);	
		UE_LOG(LogOnline, Verbose, TEXT("	NumOpenPublicConnections: %d"), Session->NumOpenPublicConnections);	
		UE_LOG(LogOnline, Verbose, TEXT("	SessionInfo: %s"), Session->SessionInfo.IsValid() ? *Session->SessionInfo->ToDebugString() : TEXT("NULL"));
		DumpSessionSettings(&Session->SessionSettings);
	}
}

/**
 * Logs session properties used from the session settings
 *
 * @param SessionSettings the session to log the information for
 */
void DumpSessionSettings(const FOnlineSessionSettings* SessionSettings)
{
	if (SessionSettings != NULL)
	{
		UE_LOG(LogOnline, Verbose, TEXT("dumping SessionSettings: "));
		UE_LOG(LogOnline, Verbose, TEXT("\tNumPublicConnections: %d"), SessionSettings->NumPublicConnections);
		UE_LOG(LogOnline, Verbose, TEXT("\tNumPrivateConnections: %d"), SessionSettings->NumPrivateConnections);
		UE_LOG(LogOnline, Verbose, TEXT("\tbIsLanMatch: %s"), SessionSettings->bIsLANMatch ? TEXT("true") : TEXT("false"));
		UE_LOG(LogOnline, Verbose, TEXT("\tbIsDedicated: %s"), SessionSettings->bIsDedicated ? TEXT("true") : TEXT("false"));
		UE_LOG(LogOnline, Verbose, TEXT("\tbUsesStats: %s"), SessionSettings->bUsesStats ? TEXT("true") : TEXT("false"));
		UE_LOG(LogOnline, Verbose, TEXT("\tbShouldAdvertise: %s"), SessionSettings->bShouldAdvertise ? TEXT("true") : TEXT("false"));
		UE_LOG(LogOnline, Verbose, TEXT("\tbAllowJoinInProgress: %s"), SessionSettings->bAllowJoinInProgress ? TEXT("true") : TEXT("false"));
		UE_LOG(LogOnline, Verbose, TEXT("\tbAllowInvites: %s"), SessionSettings->bAllowInvites ? TEXT("true") : TEXT("false"));
		UE_LOG(LogOnline, Verbose, TEXT("\tbUsesPresence: %s"), SessionSettings->bUsesPresence ? TEXT("true") : TEXT("false"));
		UE_LOG(LogOnline, Verbose, TEXT("\tbAllowJoinViaPresence: %s"), SessionSettings->bAllowJoinViaPresence ? TEXT("true") : TEXT("false"));
		UE_LOG(LogOnline, Verbose, TEXT("\tbAllowJoinViaPresenceFriendsOnly: %s"), SessionSettings->bAllowJoinViaPresenceFriendsOnly ? TEXT("true") : TEXT("false"));
		UE_LOG(LogOnline, Verbose, TEXT("\tBuildUniqueId: 0x%08x"), SessionSettings->BuildUniqueId);
		UE_LOG(LogOnline, Verbose, TEXT("\tSettings:"));
		for (FSessionSettings::TConstIterator It(SessionSettings->Settings); It; ++It)
		{
			FName Key = It.Key();
			const FOnlineSessionSetting& Setting = It.Value();
			UE_LOG(LogOnline, Verbose, TEXT("\t\t%s=%s"), *Key.ToString(), *Setting.ToString());
		}
	}
}

/**
 *	Sets a key value pair combination that defines a session setting
 *
 * @param Key key for the setting
 * @param Value value of the setting
 * @param InType type of online advertisement
 */
template<typename ValueType> 
void FOnlineSessionSettings::Set(FName Key, const ValueType& Value, EOnlineDataAdvertisementType::Type InType)
{
	FOnlineSessionSetting* Setting = Settings.Find(Key);
	if (Setting)
	{
		Setting->Data.SetValue(Value);
		Setting->AdvertisementType = InType;
	}
	else
	{
		Settings.Add(Key, FOnlineSessionSetting(Value, InType));
	}
}

/** Explicit instantiation of supported types to Set template above */
#if !UE_BUILD_DOCS
template ONLINESUBSYSTEM_API void FOnlineSessionSettings::Set(FName Key, const int32& Value, EOnlineDataAdvertisementType::Type InType);
template ONLINESUBSYSTEM_API void FOnlineSessionSettings::Set(FName Key, const float& Value, EOnlineDataAdvertisementType::Type InType);
template ONLINESUBSYSTEM_API void FOnlineSessionSettings::Set(FName Key, const uint64& Value, EOnlineDataAdvertisementType::Type InType);
template ONLINESUBSYSTEM_API void FOnlineSessionSettings::Set(FName Key, const double& Value, EOnlineDataAdvertisementType::Type InType);
template ONLINESUBSYSTEM_API void FOnlineSessionSettings::Set(FName Key, const FString& Value, EOnlineDataAdvertisementType::Type InType);
template ONLINESUBSYSTEM_API void FOnlineSessionSettings::Set(FName Key, const bool& Value, EOnlineDataAdvertisementType::Type InType);
template ONLINESUBSYSTEM_API void FOnlineSessionSettings::Set(FName Key, const TArray<uint8>& Value, EOnlineDataAdvertisementType::Type InType);
#endif

/**
 *	Sets a key value pair combination that defines a session setting
 * from an existing session setting
 *
 * @param Key key for the setting
 * @param SrcSetting setting values
 */
void FOnlineSessionSettings::Set(FName Key, const FOnlineSessionSetting& SrcSetting)
{
	FOnlineSessionSetting* Setting = Settings.Find(Key);
	if (Setting)
	{
		Setting->Data = SrcSetting.Data;
		Setting->AdvertisementType = SrcSetting.AdvertisementType;
	}
	else
	{
		Settings.Add(Key, SrcSetting);
	}
}

/**
 *	Gets a key value pair combination that defines a session setting
 *
 * @param Key key for the setting
 * @param Value value of the setting
 * 
 * @return true if found, false otherwise
 */
template<typename ValueType> 
bool FOnlineSessionSettings::Get(FName Key, ValueType& Value) const
{
	const FOnlineSessionSetting* Setting = Settings.Find(Key);
	if (Setting)
	{
		Setting->Data.GetValue(Value);
		return true;
	}

	return false;
}

/** Explicit instantiation of supported types to Get template above */
template ONLINESUBSYSTEM_API bool FOnlineSessionSettings::Get(FName Key, int32& Value) const;
template ONLINESUBSYSTEM_API bool FOnlineSessionSettings::Get(FName Key, float& Value) const;
template ONLINESUBSYSTEM_API bool FOnlineSessionSettings::Get(FName Key, uint64& Value) const;
template ONLINESUBSYSTEM_API bool FOnlineSessionSettings::Get(FName Key, double& Value) const;
template ONLINESUBSYSTEM_API bool FOnlineSessionSettings::Get(FName Key, bool& Value) const;
template ONLINESUBSYSTEM_API bool FOnlineSessionSettings::Get(FName Key, FString& Value) const;
template ONLINESUBSYSTEM_API bool FOnlineSessionSettings::Get(FName Key, TArray<uint8>& Value) const;

/**
 *  Removes a key value pair combination
 *
 * @param Key key to remove
 * 
 * @return true if found and removed, false otherwise
 */
bool FOnlineSessionSettings::Remove(FName Key)
{
	return Settings.Remove(Key);
}

/** 
 * Retrieve a setting's advertisement type
 *
 * @param Key key of the setting
 * 
 * @return the advertisement type for the setting
 */
EOnlineDataAdvertisementType::Type FOnlineSessionSettings::GetAdvertisementType(FName Key) const
{
	const FOnlineSessionSetting* Setting = Settings.Find(Key);
	if (Setting)
	{
		return Setting->AdvertisementType;
	}

	UE_LOG(LogOnline, Warning, TEXT("Unable to find key for advertisement type request: %s"), *Key.ToString());
	return EOnlineDataAdvertisementType::DontAdvertise;
}

/**
 *	Sets a key value pair combination that defines a search parameter
 *
 * @param Key key for the setting
 * @param Value value of the setting
 * @param InType type of comparison
 */
template<typename ValueType> 
void FOnlineSearchSettings::Set(FName Key, const ValueType& Value, EOnlineComparisonOp::Type InType)
{
	FOnlineSessionSearchParam* SearchParam = SearchParams.Find(Key);
	if (SearchParam)
	{
		SearchParam->Data.SetValue(Value);
		SearchParam->ComparisonOp = InType;
	}
	else
	{
		SearchParams.Add(Key, FOnlineSessionSearchParam(Value, InType));
	}
}

/** Explicit instantiation of supported types to Set template above */
template ONLINESUBSYSTEM_API void FOnlineSearchSettings::Set<int32>(FName Key, const int32& Value, EOnlineComparisonOp::Type InType);
template ONLINESUBSYSTEM_API void FOnlineSearchSettings::Set<float>(FName Key, const float& Value, EOnlineComparisonOp::Type InType);
template ONLINESUBSYSTEM_API void FOnlineSearchSettings::Set<uint64>(FName Key, const uint64& Value, EOnlineComparisonOp::Type InType);
template ONLINESUBSYSTEM_API void FOnlineSearchSettings::Set<double>(FName Key, const double& Value, EOnlineComparisonOp::Type InType);
template ONLINESUBSYSTEM_API void FOnlineSearchSettings::Set<FString>(FName Key, const FString& Value, EOnlineComparisonOp::Type InType);
template ONLINESUBSYSTEM_API void FOnlineSearchSettings::Set< TArray<uint8> >(FName Key, const TArray<uint8>& Value, EOnlineComparisonOp::Type InType);
template ONLINESUBSYSTEM_API void FOnlineSearchSettings::Set<bool>(FName Key, const bool& Value, EOnlineComparisonOp::Type InType);

/**
 *	Gets a key value pair combination that defines a search parameter
 *
 * @param Key key for the setting
 * @param Value value of the setting
 * 
 * @return true if found, false otherwise
 */
template<typename ValueType> 
bool FOnlineSearchSettings::Get(FName Key, ValueType& Value) const
{
	const FOnlineSessionSearchParam* SearchParam = SearchParams.Find(Key);
	if (SearchParam)
	{
		SearchParam->Data.GetValue(Value);
		return true;
	}

	return false;
}

/** Explicit instantiation of supported types to Get template above */
template ONLINESUBSYSTEM_API bool FOnlineSearchSettings::Get<int32>(FName Key, int32& Value) const;
template ONLINESUBSYSTEM_API bool FOnlineSearchSettings::Get<float>(FName Key, float& Value) const;
template ONLINESUBSYSTEM_API bool FOnlineSearchSettings::Get<uint64>(FName Key, uint64& Value) const;
template ONLINESUBSYSTEM_API bool FOnlineSearchSettings::Get<double>(FName Key, double& Value) const;
template ONLINESUBSYSTEM_API bool FOnlineSearchSettings::Get<FString>(FName Key, FString& Value) const;
template ONLINESUBSYSTEM_API bool FOnlineSearchSettings::Get< TArray<uint8> >(FName Key, TArray<uint8>& Value) const;
template ONLINESUBSYSTEM_API bool FOnlineSearchSettings::Get<bool>(FName Key, bool& Value) const;

/** 
 * Retrieve a setting's advertisement type
 *
 * @param Key key of the setting
 * 
 * @return the advertisement type for the setting
 */
EOnlineComparisonOp::Type FOnlineSearchSettings::GetComparisonOp(FName Key) const
{
	const FOnlineSessionSearchParam* SearchParam = SearchParams.Find(Key);
	if (SearchParam)
	{
		return SearchParam->ComparisonOp;
	}

	UE_LOG(LogOnline, Warning, TEXT("Unable to find key for comparison op request: %s"), *Key.ToString());
	return EOnlineComparisonOp::Equals;
}


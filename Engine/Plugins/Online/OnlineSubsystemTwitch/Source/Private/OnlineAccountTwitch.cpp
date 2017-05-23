// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemTwitchPrivate.h"
#include "OnlineAccountTwitch.h"

TSharedRef<const FUniqueNetId> FUserOnlineAccountTwitch::GetUserId() const
{
	return UserId;
}

FString FUserOnlineAccountTwitch::GetRealName() const
{
	FString Result;
	GetAccountData(TEXT("name"), Result);
	return Result;
}

FString FUserOnlineAccountTwitch::GetDisplayName(const FString& Platform) const
{
	FString Result;
	GetAccountData(TEXT("displayName"), Result);
	return Result;
}

bool FUserOnlineAccountTwitch::GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	return GetAccountData(AttrName, OutAttrValue);
}

bool FUserOnlineAccountTwitch::SetUserAttribute(const FString& AttrName, const FString& AttrValue)
{
	return SetAccountData(AttrName, AttrValue);
}

FString FUserOnlineAccountTwitch::GetAccessToken() const
{
	return AuthTicket;
}

bool FUserOnlineAccountTwitch::GetAuthAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	return false;
}

/** 
 * Authorization JSON block from Twitch token validation
 */
struct FTwitchTokenValidationResponseAuthorizationJson : public FJsonSerializable
{
	BEGIN_JSON_SERIALIZER
		JSON_SERIALIZE_ARRAY("scopes", Scopes);
	END_JSON_SERIALIZER

	/** List of scope fields the user has given permissions to in the token */
	TArray<FString> Scopes;
};

/** 
 * Token JSON block from Twitch token validation
 */
struct FTwitchTokenValidationResponseJson : public FJsonSerializable
{
	BEGIN_JSON_SERIALIZER
		JSON_SERIALIZE("valid", bValid);
		JSON_SERIALIZE_OBJECT_SERIALIZABLE("authorization", Authorization);
		JSON_SERIALIZE("user_name", UserName);
		JSON_SERIALIZE("user_id", UserId);
		JSON_SERIALIZE("client_id", ClientId);
	END_JSON_SERIALIZER

	/** Whether or not the token is still valid */
	bool bValid;
	/** Json block containing scope fields, if the token is valid */
	FTwitchTokenValidationResponseAuthorizationJson Authorization;
	/** Twitch user name, if the token is valid*/
	FString UserName;
	/** Twitch user Id, if the token is valid */
	FString UserId;
	/** Client Id the token was granted for, if the token is valid */
	FString ClientId;
};

bool FUserOnlineAccountTwitch::Parse(const FString& InAuthTicket, const FString& JsonStr)
{
	bool bResult = false;

	if (!InAuthTicket.IsEmpty())
	{
		if (!JsonStr.IsEmpty())
		{
			TSharedPtr<FJsonObject> JsonUser;
			TSharedRef< TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(JsonStr);

			if (FJsonSerializer::Deserialize(JsonReader, JsonUser) &&
				JsonUser.IsValid() &&
				JsonUser->HasTypedField<EJson::Object>(TEXT("token")))
			{
				FTwitchTokenValidationResponseJson TwitchToken;
				if (TwitchToken.FromJson(JsonUser->GetObjectField(TEXT("token"))))
				{
					if (TwitchToken.bValid)
					{
						UserId = MakeShared<FUniqueNetIdString>(TwitchToken.UserId);
						if (!TwitchToken.UserName.IsEmpty())
						{
							SetAccountData(TEXT("displayName"), TwitchToken.UserName);
						}
						AuthTicket = InAuthTicket;
						ScopePermissions = MoveTemp(TwitchToken.Authorization.Scopes);
						bResult = true;
					}
					else
					{
						UE_LOG_ONLINE(Log, TEXT("FUserOnlineAccountTwitch::Parse: Twitch token is not valid"));
					}
				}
				else
				{
					UE_LOG_ONLINE(Warning, TEXT("FUserOnlineAccountTwitch::Parse: JSON response missing field 'token': payload=%s"), *JsonStr);
				}
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("FUserOnlineAccountTwitch::Parse: Failed to parse JSON response: payload=%s"), *JsonStr);
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("FUserOnlineAccountTwitch::Parse: Empty JSON"));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("FUserOnlineAccountTwitch::Parse: Empty Auth Ticket"));
	}
	return bResult;
}

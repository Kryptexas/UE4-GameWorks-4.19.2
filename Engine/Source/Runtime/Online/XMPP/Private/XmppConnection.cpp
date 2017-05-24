// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "XmppConnection.h"
#include "Misc/Guid.h"

#define XMPP_RESOURCE_VERSION 2

bool FXmppUserJid::ParseResource(const FString& InResource, FString& OutAppId, FString& OutPlatform)
{
	OutAppId.Empty();
	OutPlatform.Empty();

	TArray<FString> ParsedResource;
	if (InResource.ParseIntoArray(ParsedResource, TEXT(":"), false) > 1)
	{
		if (ParsedResource[0].StartsWith(TEXT("V")))
		{
			const int32 Version = FCString::Atoi((*ParsedResource[0]) + 1);
			if (Version == XMPP_RESOURCE_VERSION)
			{
				if (ParsedResource.Num() >= 3)
				{
					OutAppId = ParsedResource[1];
					OutPlatform = ParsedResource[2];
					return true;
				}
			}
		}
	}
	else
	{
		FString ClientId, NotUsed;
		if (InResource.Split(TEXT("-"), &ClientId, &NotUsed) &&
			!ClientId.IsEmpty())
		{
			OutAppId = ClientId;
			return true;
		}
	}
	return false;
}

FString FXmppUserJid::CreateResource(const FString& AppId, const FString& Platform)
{
	return FString::Printf(TEXT("V%d:%s:%s:%s"),
		XMPP_RESOURCE_VERSION, *AppId, *Platform, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
}


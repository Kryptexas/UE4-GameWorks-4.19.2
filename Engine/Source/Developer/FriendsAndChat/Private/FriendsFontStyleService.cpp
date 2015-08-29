// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendsFontStyleService.h"

class FFriendsFontStyleServiceImpl
	: public FFriendsFontStyleService
{
public:

	virtual void SetFontStyles(const FFriendsFontStyle& InFriendsNormalFontStyle) override
	{
		FriendsNormalFontStyle = InFriendsNormalFontStyle;
	}

	virtual void SetUserFontSize(EChatFontType::Type InFontSize) override
	{
		FontSize = InFontSize;
	}

	virtual FSlateFontInfo GetNormalFont() override
	{
		if (FontSize == EChatFontType::Large)
		{
			return FriendsNormalFontStyle.FriendsFontLarge;
		}
		return FriendsNormalFontStyle.FriendsFontNormal;
	}

	virtual FSlateFontInfo GetNormalBoldFont() override
	{
		if (FontSize == EChatFontType::Large)
		{
			return FriendsNormalFontStyle.FriendsFontLargeBold;
		}
		return FriendsNormalFontStyle.FriendsFontNormalBold;
	}

private:
	FFriendsFontStyleServiceImpl()
	{
		FontSize = EChatFontType::Standard;
	}

	FFriendsFontStyle FriendsNormalFontStyle;
	EChatFontType::Type FontSize;

	friend FFriendsFontStyleServiceFactory;
};

TSharedRef< FFriendsFontStyleService > FFriendsFontStyleServiceFactory::Create()
{
	TSharedRef< FFriendsFontStyleServiceImpl > StyleService(new FFriendsFontStyleServiceImpl());
	return StyleService;
}
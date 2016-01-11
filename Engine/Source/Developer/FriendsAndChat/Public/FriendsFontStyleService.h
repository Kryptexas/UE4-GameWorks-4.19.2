// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class FFriendsFontStyleService
	: public TSharedFromThis<FFriendsFontStyleService>
{
public:
	virtual ~FFriendsFontStyleService() {}


	virtual void SetFontStyles(const FFriendsFontStyle& FriendsNormalFontStyle) = 0;

	virtual void SetUserFontSize(EChatFontType::Type FontSize) = 0;

	virtual FSlateFontInfo GetNormalFont() = 0;
	virtual FSlateFontInfo GetNormalBoldFont() = 0;
};

/**
* Creates the implementation for an FFriendsFontStyleService.
*
* @return the newly created FFriendsFontStyleService implementation.
*/
FACTORY(TSharedRef< FFriendsFontStyleService >, FFriendsFontStyleService);
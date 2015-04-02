// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SClanItem.h"
#include "ClanInfo.h"
#include "SFriendsAndChatCombo.h"

#define LOCTEXT_NAMESPACE "SClassItem"

class SClanItemImpl : public SClanItem
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<class IClanInfo>& ClanInfo)
	{
		FriendStyle = *InArgs._FriendStyle;
		this->ClanInfo = ClanInfo;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(STextBlock)
			.Font(FriendStyle.FriendsFontStyleBold)
			.ColorAndOpacity(FriendStyle.DefaultFontColor)
			.Text(ClanInfo->GetTitle())
		]);
	}

private:

	virtual bool SupportsKeyboardFocus() const override
	{
		return true;
	}

private:

	TSharedPtr<IClanInfo> ClanInfo;

	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;

	TSharedPtr<SWidget> MenuContent;

	EPopupMethod MenuMethod;
};

TSharedRef<SClanItem> SClanItem::New()
{
	return MakeShareable(new SClanItemImpl());
}

#undef LOCTEXT_NAMESPACE

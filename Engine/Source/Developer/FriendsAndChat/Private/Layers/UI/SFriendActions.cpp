// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SFriendActions.h"
#include "FriendViewModel.h"

#define LOCTEXT_NAMESPACE "SFriendItem"

class SFriendActionsImpl : public SFriendActions
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FFriendViewModel>& InFriendViewModel)
	{
		FriendStyle = *InArgs._FriendStyle;
		FriendViewModel = InFriendViewModel;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SAssignNew(ActionContainer, SVerticalBox)
		]);


		TArray<EFriendActionType::Type> Actions;
		FriendViewModel->EnumerateActions(Actions);
		for (EFriendActionType::Type Action : Actions)
		{
			ActionContainer->AddSlot()
			[
				SNew(SButton)
				.OnClicked(this, &SFriendActionsImpl::HandleActionClicked, Action)
				.ButtonStyle(&FriendStyle.ActionButtonStyle)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Cursor(EMouseCursor::Hand)
				.ContentPadding(FMargin(5.0f))
				[
					SNew(STextBlock)
					.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
					.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
					.Text(EFriendActionType::ToText(Action))
				]
			];
		}
	}

private:

	FReply HandleActionClicked(EFriendActionType::Type ActionType)
	{
		FriendViewModel->PerformAction(ActionType);
		FSlateApplication::Get().DismissAllMenus();
		return FReply::Handled();
	}

private:

	TSharedPtr<FFriendViewModel> FriendViewModel;
	FFriendsAndChatStyle FriendStyle;
	TSharedPtr<SVerticalBox> ActionContainer;
};

TSharedRef<SFriendActions> SFriendActions::New()
{
	return MakeShareable(new SFriendActionsImpl());
}

#undef LOCTEXT_NAMESPACE

// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatTipViewModel.h"
#include "FriendsChatMarkupService.h"
#include "SChatMarkupTips.h"

#define LOCTEXT_NAMESPACE ""

/**
 * Declares the chat markup tips widget
*/
class SChatMarkupTipsImpl : public SChatMarkupTips
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FChatTipViewModel>& InViewModel)
	{
		ViewModel = InViewModel;
		ViewModel->OnChatTipAvailable().AddSP(this, &SChatMarkupTipsImpl::HandleTipAvailable);

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SAssignNew(ChatTipBox, SVerticalBox)
		]);
	}

private:

	void HandleTipAvailable()
	{
		ChatTipBox->ClearChildren();
		TArray<TSharedRef<IChatTip> >& ChatTips = ViewModel->GetChatTips();
		if(ChatTips.Num())
		{
			for(const auto& ChatTip : ViewModel->GetChatTips())
			{
				ChatTipBox->AddSlot()
				.AutoHeight()
				[
					SNew(SButton)
					.ButtonColorAndOpacity(this, &SChatMarkupTipsImpl::GetActionBackgroundColor, TWeakPtr<IChatTip>(ChatTip))
					.Text(ChatTip->GetTipText())
					.OnClicked(this, &SChatMarkupTipsImpl::HandleActionClicked, ChatTip)
				];
			}
		}
	}

	FReply HandleActionClicked(TSharedRef<IChatTip> ChatTip)
	{
		return ChatTip->ExecuteTip();
	}

	FSlateColor GetActionBackgroundColor(TWeakPtr<IChatTip> TabPtr) const
	{
		TSharedPtr<IChatTip> Tab = TabPtr.Pin();
		if (Tab.IsValid())
		{
			if (Tab == ViewModel->GetActiveTip())
			{
				return FLinearColor::White;
			}
		}
		return FLinearColor::Gray;
	}

private:
	TSharedPtr<FChatTipViewModel> ViewModel;
	TSharedPtr<SVerticalBox> ChatTipBox;
};

TSharedRef<SChatMarkupTips> SChatMarkupTips::New()
{
	return MakeShareable(new SChatMarkupTipsImpl());
}

#undef LOCTEXT_NAMESPACE

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
		MarkupStyle = *InArgs._MarkupStyle;

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
					.ButtonStyle(&MarkupStyle.MarkupButtonStyle)
					.ButtonColorAndOpacity(this, &SChatMarkupTipsImpl::GetActionBackgroundColor, TWeakPtr<IChatTip>(ChatTip))
					.OnClicked(this, &SChatMarkupTipsImpl::HandleActionClicked, ChatTip)
					[
						SNew(STextBlock)
						.Text(ChatTip->GetTipText())
						.TextStyle(&MarkupStyle.MarkupTextStyle)
						.ColorAndOpacity(FLinearColor::White)
					]
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
		if (Tab.IsValid() && Tab == ViewModel->GetActiveTip())
		{
			return FLinearColor::Black;
		}
		return FLinearColor::Gray;
	}

private:
	TSharedPtr<FChatTipViewModel> ViewModel;
	TSharedPtr<SVerticalBox> ChatTipBox;
	/** Holds the style to use when making the widget. */
	FFriendsMarkupStyle MarkupStyle;
};

TSharedRef<SChatMarkupTips> SChatMarkupTips::New()
{
	return MakeShareable(new SChatMarkupTipsImpl());
}

#undef LOCTEXT_NAMESPACE

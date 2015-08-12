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
		ViewModel->OnChatTipSelected().AddSP(this, &SChatMarkupTipsImpl::OnChatTipSelected);
		MarkupStyle = *InArgs._MarkupStyle;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SAssignNew(ChatTipList, SListView<TSharedRef<IChatTip > >)
			.SelectionMode(ESelectionMode::Single)
			.ListItemsSource(&ChatTips)
			.OnGenerateRow(this, &SChatMarkupTipsImpl::OnGenerateChatTip)
		]);
	}

private:

	void HandleTipAvailable()
	{
		ChatTips = ViewModel->GetChatTips();
		ChatTipList->RequestListRefresh();
	}

	TSharedRef<ITableRow> OnGenerateChatTip(TSharedRef<IChatTip > ChatTip, const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(STableRow<TSharedPtr<IChatTip> >, OwnerTable)
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

	void OnChatTipSelected(TSharedRef<IChatTip> NewChatTip)
	{
		ChatTipList->RequestScrollIntoView(NewChatTip);
		ChatTipList->RequestListRefresh();
	}

private:
	TSharedPtr<FChatTipViewModel> ViewModel;
	TSharedPtr<SVerticalBox> ChatTipBox;
	TArray<TSharedRef<IChatTip> > ChatTips;
	TSharedPtr< SListView<TSharedRef<IChatTip > > > ChatTipList;

	/** Holds the style to use when making the widget. */
	FFriendsMarkupStyle MarkupStyle;
};

TSharedRef<SChatMarkupTips> SChatMarkupTips::New()
{
	return MakeShareable(new SChatMarkupTipsImpl());
}

#undef LOCTEXT_NAMESPACE

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatItemViewModel.h"
#include "SChatItem.h"

#define LOCTEXT_NAMESPACE "SChatWindow"

class SChatItemImpl : public SChatItem
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FChatItemViewModel>& InViewModel)
	{
		FriendStyle = *InArgs._FriendStyle;
		MenuMethod = InArgs._Method;
		this->ViewModel = InViewModel;

		FSlateColor DisplayColor;
		switch(ViewModel->GetMessageType())
		{
			case EChatMessageType::Global: DisplayColor = FriendStyle.DefaultChatColor; break;
			case EChatMessageType::Whisper: DisplayColor =  FriendStyle.WhisplerChatColor; break;
			case EChatMessageType::Party: DisplayColor =  FriendStyle.PartyChatColor; break;
			case EChatMessageType::Network: DisplayColor =  FriendStyle.NetworkChatColor; break;
			default:
			DisplayColor = FLinearColor::Gray;
		}
		
		const EVisibility FromSelfVisibility = ViewModel->IsFromSelf() ? EVisibility::Visible : EVisibility::Collapsed;
		const EVisibility FriendNameVisibility = !ViewModel->IsFromSelf() || ViewModel->GetMessageType() == EChatMessageType::Whisper ? EVisibility::Visible : EVisibility::Collapsed;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(5))
			[
				SNew(SImage)
				.Image(this, &SChatItemImpl::GetChatIcon)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(5,1))
			[
				SAssignNew(FriendItemBorder, SBorder)
				.BorderImage(&FriendStyle.TitleBarBrush)
				.BorderBackgroundColor(this, &SChatItemImpl::GetFriendBorderColor)
				[
					SNew(STextBlock)
					.Visibility(FriendNameVisibility)
					.Text(ViewModel->GetFriendID())
					.ColorAndOpacity(DisplayColor)
					.Font(FriendStyle.FriendsFontStyle)
				]
			]
			+SHorizontalBox::Slot()
			.Padding(FMargin(5,1))
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(ViewModel->GetMessage())
				.ColorAndOpacity(DisplayColor)
				.Font(FriendStyle.FriendsFontStyleSmall)
				.AutoWrapText(true)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.Padding(FMargin(5,1))
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(ViewModel->GetMessageTime())
				.ColorAndOpacity(this, &SChatItemImpl::GetTimeDisplayColor)
				.Font(FriendStyle.FriendsFontStyleSmall)
				.AutoWrapText(true)
			]
		]);
	}
private:

	FSlateColor GetTimeDisplayColor() const
	{
		switch(ViewModel->GetMessageType())
		{
			case EChatMessageType::Global: return FriendStyle.DefaultChatColor.CopyWithNewOpacity(ViewModel->GetFadeAmountColor()); break;
			case EChatMessageType::Whisper: return FriendStyle.WhisplerChatColor.CopyWithNewOpacity(ViewModel->GetFadeAmountColor()); break;
			case EChatMessageType::Party: return FriendStyle.PartyChatColor.CopyWithNewOpacity(ViewModel->GetFadeAmountColor()); break;
			case EChatMessageType::Network: return FriendStyle.NetworkChatColor.CopyWithNewOpacity(ViewModel->GetFadeAmountColor()); break;
			default:
			return FLinearColor::Gray;
		}
	}

	const FSlateBrush* GetChatIcon() const
	{
		switch(ViewModel->GetMessageType())
		{
			case EChatMessageType::Global: return &FriendStyle.ChatGlobalBrush; break;
			case EChatMessageType::Whisper: return &FriendStyle.ChatWhisperBrush; break;
			case EChatMessageType::Party: return &FriendStyle.ChatPartyBrush; break;
			case EChatMessageType::Network: return &FriendStyle.ChatPartyBrush; break;
			default:
			return nullptr;
		}
	}

	FSlateColor GetFriendBorderColor() const
	{
		return FriendItemBorder->IsHovered() ? FLinearColor::Gray : FLinearColor::Transparent;
	}

private:

	// Holds the Friends List view model
	TSharedPtr<FChatItemViewModel> ViewModel;

	TSharedPtr<SBorder> FriendItemBorder;

	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;

	// Holds the menu method - Full screen requires use owning window or crashes.
	SMenuAnchor::EMethod MenuMethod;
};

TSharedRef<SChatItem> SChatItem::New()
{
	return MakeShareable(new SChatItemImpl());
}

#undef LOCTEXT_NAMESPACE

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

		FText DisplayNameText = ViewModel->GetFriendNameDisplayText();
		if(ViewModel->IsFromSelf())
		{ 
			if (ViewModel->GetMessageType() == EChatMessageType::Whisper)
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("Username"), ViewModel->GetFriendNameDisplayText());
				DisplayNameText = FText::Format(LOCTEXT("SChatItem_To", "To {Username}"), Args);
			}
		}

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(5,1))
			[
				SNew(SImage)
				.ColorAndOpacity(this, &SChatItemImpl::GetChannelColor)
				.Image(this, &SChatItemImpl::GetChatIcon)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(5,1))
			.MaxWidth(150)
			[
				SNew(STextBlock)
				.Visibility(ViewModel->IsFromSelf() && ViewModel->GetMessageType() != EChatMessageType::Party ? EVisibility::Visible : EVisibility::Collapsed)
				.Text(DisplayNameText)
				.ColorAndOpacity(this, &SChatItemImpl::GetChannelColor)
				.Font(FriendStyle.FriendsFontStyleSmallBold)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(5, 1, 2, 1))
			.MaxWidth(150)
			[
				SNew(SButton)
				.Visibility(ViewModel->IsFromSelf() || ViewModel->GetMessageType() == EChatMessageType::Party ? EVisibility::Collapsed : EVisibility::Visible)
				.ButtonStyle(&FriendStyle.FriendListItemButtonStyle)
				.OnClicked(this, &SChatItemImpl::HandleNameClicked)
				.ContentPadding(0)
				[
					SNew(STextBlock)
					.Text(DisplayNameText)
					.ColorAndOpacity(this, &SChatItemImpl::GetChannelColor)
					.Font(FriendStyle.FriendsFontStyleSmallBold)
				]
			]
			+SHorizontalBox::Slot()
			.Padding(FMargin(2, 1, 5, 1))
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(ViewModel->GetMessage())
				.ColorAndOpacity(this, &SChatItemImpl::GetChannelColor)
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
		if(ViewModel->UseOverrideColor())
		{
			return ViewModel->GetOverrideColor();
		}
		else
		{
			switch(ViewModel->GetMessageType())
			{
				case EChatMessageType::Global: return FriendStyle.DefaultChatColor.CopyWithNewOpacity(ViewModel->GetFadeAmountColor()); break;
				case EChatMessageType::Whisper: return FriendStyle.WhisplerChatColor.CopyWithNewOpacity(ViewModel->GetFadeAmountColor()); break;
				case EChatMessageType::Party: return FriendStyle.PartyChatColor.CopyWithNewOpacity(ViewModel->GetFadeAmountColor()); break;
				default: return FLinearColor::Gray;
			}
		}
	}

	FSlateColor GetChannelColor () const
	{
		if(ViewModel->UseOverrideColor())
		{
			return ViewModel->GetOverrideColor();
		}
		else
		{
			FSlateColor DisplayColor;
			switch(ViewModel->GetMessageType())
			{
				case EChatMessageType::Global: DisplayColor = FriendStyle.DefaultChatColor; break;
				case EChatMessageType::Whisper: DisplayColor =  FriendStyle.WhisplerChatColor; break;
				case EChatMessageType::Party: DisplayColor =  FriendStyle.PartyChatColor; break;
				default: DisplayColor = FLinearColor::Gray;
			}
			return DisplayColor;
		}
	}

	const FSlateBrush* GetChatIcon() const
	{
		if(ViewModel->UseOverrideColor())
		{
			return nullptr;
		}
		else
		{
			switch(ViewModel->GetMessageType())
			{
				case EChatMessageType::Global: return &FriendStyle.ChatGlobalBrush; break;
				case EChatMessageType::Whisper: return &FriendStyle.ChatWhisperBrush; break;
				case EChatMessageType::Party: return &FriendStyle.ChatPartyBrush; break;
				default:
				return nullptr;
			}
		}
	}

	FReply HandleNameClicked()
	{
		ViewModel->FriendNameSelected();
		return FReply::Handled();
	}

private:

	// Holds the Friends List view model
	TSharedPtr<FChatItemViewModel> ViewModel;

	TSharedPtr<SBorder> FriendItemBorder;

	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;

	// Holds the menu method - Full screen requires use owning window or crashes.
	EPopupMethod MenuMethod;
};

TSharedRef<SChatItem> SChatItem::New()
{
	return MakeShareable(new SChatItemImpl());
}

#undef LOCTEXT_NAMESPACE

// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendsFontStyleService.h"
#include "SChatWindow.h"
#include "ChatViewModel.h"
#include "ChatItemViewModel.h"
#include "SScrollBox.h"
#include "SFriendsList.h"
#include "SChatMarkupTips.h"
#include "SFriendActions.h"
#include "SChatEntryWidget.h"
#include "ChatTextLayoutMarshaller.h"
#include "TextDecorators.h"

#define LOCTEXT_NAMESPACE "SChatWindow"

class SChatWindowImpl : public SChatWindow
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FChatViewModel>& InViewModel)
	{
		LastMessageIndex = 0;
		FriendStyle = *InArgs._FriendStyle;
		MenuMethod = InArgs._Method;
		ViewModel = InViewModel;
		FChatViewModel* ViewModelPtr = ViewModel.Get();
		ViewModel->OnChatListUpdated().AddSP(this, &SChatWindowImpl::RefreshChatList);
		ViewModel->OnSettingsUpdated().AddSP(this, &SChatWindowImpl::SettingsChanged);

		FFriendsAndChatModuleStyle::Initialize(FriendStyle);

		OnHyperlinkClicked = FSlateHyperlinkRun::FOnClick::CreateSP(SharedThis(this), &SChatWindowImpl::HandleNameClicked);
		OnChannelHyperlinkClicked = FSlateHyperlinkRun::FOnClick::CreateSP(SharedThis(this), &SChatWindowImpl::HandleChannelClicked);

		RichTextMarshaller = FChatTextLayoutMarshallerFactory::Create(&FriendStyle, ViewModel->MultiChat());
		RichTextMarshaller->AddChannelNameHyperlinkDecorator(FHyperlinkDecorator::Create(TEXT("Channel"), OnChannelHyperlinkClicked));
		RichTextMarshaller->AddUserNameHyperlinkDecorator(FHyperlinkDecorator::Create(TEXT("UserName"), OnHyperlinkClicked));

		TimeTransparency = 0.0f;

		FadeAnimation = FCurveSequence();
		FadeCurve = FadeAnimation.AddCurve(0.f, 0.5f, ECurveEaseFunction::Linear);
		if (ViewModel.IsValid())
		{
			ViewModel->SetCurve(FadeCurve);
		}
		
		ExternalScrollbar = SNew(SScrollBar)
		.Thickness(FVector2D(4, 4))
		.Style(&FriendStyle.ScrollBarStyle)
		.AlwaysShowScrollbar(true);

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				SNew(SBorder)
				.BorderImage(&FriendStyle.FriendsChatStyle.ChatBackgroundBrush)
				.BorderBackgroundColor(this, &SChatWindowImpl::GetTimedFadeSlateColor)
				.Visibility(this, &SChatWindowImpl::GetChatBackgroundVisibility)
				.Padding(0)
			]
			+SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.VAlign(VAlign_Bottom)
				[
					SNew(SOverlay)
					.Visibility(this, &SChatWindowImpl::GetChatListVisibility)
					+ SOverlay::Slot()
					.Padding(FMargin(10, 0))
					.VAlign(VAlign_Bottom)
					[
						SAssignNew(ChatScrollBox, SScrollBox)
						.ExternalScrollbar(ExternalScrollbar)
						.Style(&FriendStyle.FriendsChatStyle.ScrollBorderStyle)
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Right)
					.Padding(FMargin(10, 0))
					[
						SNew( SBorder )
						.BorderBackgroundColor(FLinearColor::Transparent)
						.ColorAndOpacity(this, &SChatWindowImpl::GetTimedFadeColor)
						.Padding(FMargin(0))
						[
							ExternalScrollbar.ToSharedRef()
						]
					]
					+SOverlay::Slot()
					.Padding(FMargin(10, 0))
					.VAlign(VAlign_Bottom)
					[
						SNew(SBorder)
						.Visibility(ViewModelPtr, &FChatViewModel::GetTipVisibility)
						[
							SNew(SChatMarkupTips, ViewModel->GetChatTipViewModel())
							.MarkupStyle(&FriendStyle.FriendsMarkupStyle)
						]
					]
				]
				+SVerticalBox::Slot()
				.VAlign(VAlign_Bottom)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					.Visibility(this, &SChatWindowImpl::GetChatEntryVisibility)
					+SHorizontalBox::Slot()
					[
						SNew(SBox)
						.MinDesiredHeight(74)
						[
							SNew(SBorder)
							.BorderImage(&FriendStyle.FriendsChatStyle.ChatFooterBrush)
							.BorderBackgroundColor(this, &SChatWindowImpl::GetTimedFadeSlateColor)
							.Padding(FMargin(10, 10, 10, 10))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.Expose(ChannelTextSlot)
								.AutoHeight()
								+ SVerticalBox::Slot()
								.VAlign(VAlign_Center)
								[
									SAssignNew(ChatTextBox, SChatEntryWidget, ViewModel.ToSharedRef())
								 	.FriendStyle(&FriendStyle)
									.HintText(InArgs._ActivationHintText)
								]
								+SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SBorder)
									.Padding(FMargin(10, 10))
									.VAlign(VAlign_Center)
									.Visibility(this, &SChatWindowImpl::GetLostConnectionVisibility)
									.BorderImage(&FriendStyle.FriendsChatStyle.ChatContainerBackground)
									.BorderBackgroundColor(FLinearColor(FColor(255, 255, 255, 128)))
									[
										SNew(STextBlock)
										.ColorAndOpacity(FLinearColor::Red)
										.AutoWrapText(true)
										.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
										.Text(ViewModelPtr, &FChatViewModel::GetChatDisconnectText)
									]
								]
							]
						]
					]
				]
			]
		]);

		if(ChannelTextSlot && ViewModel->MultiChat())
		{
			ChannelTextSlot->AttachWidget(
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(ViewModelPtr, &FChatViewModel::GetOutgoingChannelText)
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.ButtonStyle(&FriendStyle.FriendsChatStyle.FriendsMaximizeButtonStyle)
					.Visibility(this, &SChatWindowImpl::GetChatMaximizeVisibility)
					.OnClicked(this, &SChatWindowImpl::HandleMaximizeClicked)
				]
			);
		}

		RefreshChatList();
	}

	virtual void HandleWindowActivated() override
	{
		ViewModel->SetFocus();
		ViewModel->SetIsActive(true);
	}

	virtual void HandleWindowDeactivated() override
	{
		ViewModel->SetIsActive(false);
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		SUserWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		int32 NewWindowWidth = FMath::FloorToInt(AllottedGeometry.GetLocalSize().X);
		if(NewWindowWidth != WindowWidth)
		{
			WindowWidth = NewWindowWidth;
		}

		if(ViewModel.IsValid())
		{
			if (IsHovered() || ChatTextBox->HasKeyboardFocus())
			{
				if (FadeCurve.GetLerp() == 0.0f)
				{
					FadeAnimation.Play(this->AsShared());
				}
			}
			else
			{
				if (FadeCurve.GetLerp() == 1.0f)
				{
					FadeAnimation.PlayReverse(this->AsShared());
				}
			}
		}

		if (!bUserHasScrolled && ExternalScrollbar.IsValid() && ExternalScrollbar->IsScrolling())
		{
			bUserHasScrolled = true;
		}
		if (bUserHasScrolled && ExternalScrollbar.IsValid() && !ExternalScrollbar->IsScrolling())
		{
			bUserHasScrolled = false;
			if (ExternalScrollbar->DistanceFromBottom() == 0)
			{
				ViewModel->SetMessageShown(true);
			}
		}
	}

	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override
	{
		bUserHasScrolled = true;
		return FReply::Handled();
	}

private:

	FReply HandleFriendActionDropDownClicked() const
	{
		ChatItemActionMenu->IsOpen() ? ChatItemActionMenu->SetIsOpen(false) : ChatItemActionMenu->SetIsOpen(true);
		return FReply::Handled();
	}

	bool HasMenuOptions() const
	{
		return true;
	}

	void SettingsChanged()
	{
		CreateChatWidgets();
		RefreshChatList();
	}

	void CreateChatWidgets()
	{
		if(RichText.IsValid())
		{
			RichText.Reset();
			ChatScrollBox->ClearChildren();
		}
		if (!RichText.IsValid())
		{
			SAssignNew(RichText, SMultiLineEditableTextBox)
			.Style(&FriendStyle.FriendsChatStyle.ChatDisplayTextStyle)
			.Font(FFriendsAndChatModuleStyle::GetStyleService()->GetNormalFont())
			.WrapTextAt(this, &SChatWindowImpl::GetChatWrapWidth)
			.Marshaller(RichTextMarshaller.ToSharedRef())
			.IsReadOnly(true);

			ChatScrollBox->AddSlot()
			[
				RichText.ToSharedRef()
			];

			ChatTextBox->RebuildTextEntry();
		}
	}

	void RefreshChatList()
	{
		if (!RichText.IsValid())
		{
			CreateChatWidgets();
		}

		// ToDo - Move the following logic into the view model
		TArray<TSharedRef<FChatItemViewModel>> Messages = ViewModel->GetMessages();
		for (;LastMessageIndex < Messages.Num(); ++LastMessageIndex)
		{
			TSharedRef<FChatItemViewModel> ChatItem = Messages[LastMessageIndex];

			if (LastMessage.IsValid() &&
				LastMessage->GetMessageType() == ChatItem->GetMessageType() &&
				LastMessage->IsFromSelf() == ChatItem->IsFromSelf() &&
				LastMessage->GetSenderID().IsValid() && ChatItem->GetSenderID().IsValid() &&
				*LastMessage->GetSenderID() == *ChatItem->GetSenderID()  &&
				LastMessage->GetRecipientID().IsValid() && ChatItem->GetRecipientID().IsValid() &&
				*LastMessage->GetRecipientID() == *ChatItem->GetRecipientID() &&
				(ChatItem->GetMessageTime() - LastMessage->GetMessageTime()).GetDuration() <= FTimespan::FromMinutes(1.0))
			{
				RichTextMarshaller->AppendMessage(ChatItem, true);
			}
			else
			{
				RichTextMarshaller->AppendLineBreak();
				RichTextMarshaller->AppendMessage(ChatItem, false);
			}
			LastMessage = ChatItem;
		}

		if (RichText.IsValid() && ChatScrollBox.IsValid())
		{
			if(ExternalScrollbar.IsValid() && ExternalScrollbar->DistanceFromBottom() == 0)
			{
				ChatScrollBox->ScrollToEnd();
				ViewModel->SetMessageShown(true);
			}
			else
			{
				ViewModel->SetMessageShown(false);
			}
		}
	}

	void HandleNameClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
	{
		const FString* UsernameString = Metadata.Find(TEXT("Username"));
		const FString* UniqueIDString = Metadata.Find(TEXT("uid"));

		if (UsernameString && UniqueIDString)
		{
			FText Username = FText::FromString(*UsernameString);
			const TSharedRef<FFriendViewModel> FriendViewModel = ViewModel->GetFriendViewModel(*UniqueIDString, Username).ToSharedRef();

			bool DisplayChatOption = ViewModel->DisplayChatOption(FriendViewModel);

			TSharedRef<SWidget> Widget = SNew(SFriendActions, FriendViewModel).FriendStyle(&FriendStyle).FromChat(true).DisplayChatOption(DisplayChatOption);

			FSlateApplication::Get().PushMenu(
				SharedThis(this),
				FWidgetPath(),
				Widget,
				FSlateApplication::Get().GetCursorPos(),
				FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
				);
		}
	}

	void HandleChannelClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
	{
		const FString* ChannelString = Metadata.Find(TEXT("Command"));

		if (ChannelString)
		{
			ViewModel->SetOutgoingMessageChannel(EChatMessageType::EnumFromString(*ChannelString));
			ViewModel->SetFocus();
		}
	}

	EVisibility GetChatChannelVisibility() const
	{
		return EVisibility::Visible;
	}

	EVisibility GetChatEntryVisibility() const
	{
		return ViewModel->GetTextEntryVisibility();
	}

	EVisibility GetChatBackgroundVisibility() const
	{
		return ViewModel->GetBackgroundVisibility();
	}

	EVisibility GetLostConnectionVisibility() const
	{
		return ViewModel->IsChatConnected() ? EVisibility::Collapsed : EVisibility::Visible;
	}

	FSlateColor GetTimedFadeSlateColor() const
	{
		return GetTimedFadeColor();
	}

	FLinearColor GetTimedFadeColor() const
	{
		return FLinearColor(1, 1, 1, ViewModel->GetWindowOpacity() * FadeCurve.GetLerp());
	}

	EVisibility GetChatListVisibility() const
	{
		return ViewModel->GetChatListVisibility();
	}

	float GetChatWrapWidth() const
	{
		return FMath::Max(WindowWidth - 40, 0.0f);
	}
	
	FReply HandleMaximizeClicked()
	{
		ViewModel->ToggleChatMinimized();
		return FReply::Handled();
	}

	EVisibility GetChatMaximizeVisibility() const
	{
		return ViewModel->GetChatMaximizeVisibility();
	}

private:

	TSharedPtr<SScrollBox> ChatScrollBox;

	TSharedPtr<FChatTextLayoutMarshaller> RichTextMarshaller;

	TSharedPtr<SMultiLineEditableTextBox> RichText;

	SVerticalBox::FSlot* ChannelTextSlot;

	int32 LastMessageIndex;

	TSharedPtr<FChatItemViewModel> LastMessage;

	// Holds the scroll bar
	TSharedPtr<SScrollBar> ExternalScrollbar;

	// Holds the chat list display
	TSharedPtr<SChatEntryWidget> ChatTextBox;

	// Holds the Friend action button
	TSharedPtr<SMenuAnchor> ChatItemActionMenu;

	// Holds the Friends List view model
	TSharedPtr<FChatViewModel> ViewModel;

	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;

	// Holds the menu method - Full screen requires use owning window or crashes.
	EPopupMethod MenuMethod;

	// Should AutoScroll
	bool bAutoScroll;

	// Has the user scrolled
	bool bUserHasScrolled;

	// Holds the time transparency.
	float TimeTransparency;

	// Holds the window width
	float WindowWidth;

	// Holds the fade in animation
	FCurveSequence FadeAnimation;
	// Holds the fade in curve
	FCurveHandle FadeCurve;

	FSlateHyperlinkRun::FOnClick OnHyperlinkClicked;
	FSlateHyperlinkRun::FOnClick OnChannelHyperlinkClicked;
	FWidgetDecorator::FCreateWidget OnTimeStampCreated;

	static const float CHAT_HINT_UPDATE_THROTTLE;
};

const float SChatWindowImpl::CHAT_HINT_UPDATE_THROTTLE = 1.0f;

TSharedRef<SChatWindow> SChatWindow::New()
{
	return MakeShareable(new SChatWindowImpl());
}

#undef LOCTEXT_NAMESPACE

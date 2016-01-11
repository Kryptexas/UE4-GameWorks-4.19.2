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
#include "RichTextLayoutMarshaller.h"
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

		RichTextMarshaller = FRichTextLayoutMarshaller::Create(
			TArray<TSharedRef<ITextDecorator>>(), 
			&FFriendsAndChatModuleStyle::Get()
			);

		OnHyperlinkClicked = FSlateHyperlinkRun::FOnClick::CreateSP(SharedThis(this), &SChatWindowImpl::HandleNameClicked);
		OnChannelHyperlinkClicked = FSlateHyperlinkRun::FOnClick::CreateSP(SharedThis(this), &SChatWindowImpl::HandleChannelClicked);
		OnTimeStampCreated = FWidgetDecorator::FCreateWidget::CreateSP(SharedThis(this), &SChatWindowImpl::HandleTimeStampDisplay);

		RichTextMarshaller->AppendInlineDecorator(FHyperlinkDecorator::Create(TEXT("UserName"), OnHyperlinkClicked));
		RichTextMarshaller->AppendInlineDecorator(FHyperlinkDecorator::Create(TEXT("Channel"), OnChannelHyperlinkClicked));
		RichTextMarshaller->AppendInlineDecorator(FImageDecorator::Create(TEXT("img"), &FFriendsAndChatModuleStyle::Get()));
		RichTextMarshaller->AppendInlineDecorator(FWidgetDecorator::Create(TEXT("TimeStamp"), OnTimeStampCreated));

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
			SNew(SBorder)
			.BorderImage(&FriendStyle.FriendsChatStyle.ChatBackgroundBrush)
			.BorderBackgroundColor(this, &SChatWindowImpl::GetTimedFadeSlateColor)
			.Visibility(this, &SChatWindowImpl::GetChatBackgroundVisibility)
			.Padding(0)
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
					.Padding(FMargin(40, 0))
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
								[
									SAssignNew(ChatTextBox, SChatEntryWidget, ViewModel.ToSharedRef())
								 	.FriendStyle(&FriendStyle)
									.Marshaller(RichTextMarshaller)
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
				SNew(STextBlock)
				.Text(ViewModelPtr, &FChatViewModel::GetOutgoingChannelText)
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
	}

	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override
	{
		bUserHasScrolled = true;
		return FReply::Unhandled();
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
			.Text(this, &SChatWindowImpl::GetChatText)
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
				ChatText.AppendLine(FormatListMessageText(ChatItem, true));
			}
			else
			{
				static FString MessageBreak(TEXT("<MessageBreak></>"));
				ChatText.AppendLine(MessageBreak);
				ChatText.AppendLine(FormatListMessageText(ChatItem, false));
			}
			LastMessage = ChatItem;
		}

		if (RichText.IsValid() && ChatScrollBox.IsValid())
		{
			RichText->Refresh();
			if(ExternalScrollbar.IsValid() && ExternalScrollbar->DistanceFromBottom() == 0)
			{
				ChatScrollBox->ScrollToEnd();
			}
		}
	}

	FText GetChatText() const
	{
		return ChatText.ToText();
	}

	void HandleNameClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
	{
		const FString* UsernameString = Metadata.Find(TEXT("Username"));
		const FString* UniqueIDString = Metadata.Find(TEXT("uid"));

		if (UsernameString && UniqueIDString)
		{
			FText Username = FText::FromString(*UsernameString);
			const TSharedRef<FFriendViewModel> FriendViewModel = ViewModel->GetFriendViewModel(*UniqueIDString, Username).ToSharedRef();
			TSharedRef<SWidget> Widget = SNew(SFriendActions, FriendViewModel).FriendStyle(&FriendStyle);

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

	FSlateWidgetRun::FWidgetRunInfo HandleTimeStampDisplay( const FTextRunInfo& RunInfo, const ISlateStyle* Style ) const
	{
		// Internal class for timestamps
		class SChatTimeStamp : public SCompoundWidget
		{
			SLATE_BEGIN_ARGS(SChatTimeStamp)
				: _DateTime(FDateTime::Now())
			{ }
				SLATE_ARGUMENT(FDateTime, DateTime)
			SLATE_END_ARGS()

			virtual ~SChatTimeStamp()
			{
				FTicker::GetCoreTicker().RemoveTicker(TickerHandle);
			}

			void Construct(const FArguments& InArgs)
			{
				TimeStamp = InArgs._DateTime;
				ChildSlot
				[
					SNew(STextBlock)
					.Text(this, &SChatTimeStamp::GetMessageTimeText)
					.Font(FFriendsAndChatModuleStyle::GetStyleService()->GetNormalFont())
				];

				if (UpdateTimeStamp(60)) //this will broadcast a changed event
				{
					TickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(this, &SChatTimeStamp::UpdateTimeStamp), 60);
				}
			}

			private:

			bool UpdateTimeStamp(float Delay)
			{
				bool ContinueTicking = true;

				FDateTime Now = FDateTime::Now();

				if ((Now - TimeStamp).GetDuration() <= FTimespan::FromMinutes(1.0))
				{
					static FText NowTimeStamp = NSLOCTEXT("SChatWindow", "Now_TimeStamp", "Now");
					MessageTimeAsText = NowTimeStamp;
				}
				else if ((Now - TimeStamp).GetDuration() <= FTimespan::FromMinutes(2.0))
				{
					static FText OneMinuteTimeStamp = NSLOCTEXT("SChatWindow", "1_Minute_TimeStamp", "1 min");
					MessageTimeAsText = OneMinuteTimeStamp;
				}
				else if ((Now - TimeStamp).GetDuration() <= FTimespan::FromMinutes(3.0))
				{
					static FText TwoMinuteTimeStamp = NSLOCTEXT("SChatWindow", "2_Minute_TimeStamp", "2 min");
					MessageTimeAsText = TwoMinuteTimeStamp;
				}
				else if ((Now - TimeStamp).GetDuration() <= FTimespan::FromMinutes(4.0))
				{
					static FText ThreeMinuteTimeStamp = NSLOCTEXT("SChatWindow", "3_Minute_TimeStamp", "3 min");
					MessageTimeAsText = ThreeMinuteTimeStamp;
				}
				else if ((Now - TimeStamp).GetDuration() <= FTimespan::FromMinutes(5.0))
				{
					static FText FourMinuteTimeStamp = NSLOCTEXT("SChatWindow", "4_Minute_TimeStamp", "4 min");
					MessageTimeAsText = FourMinuteTimeStamp;
				}
				else if ((Now - TimeStamp).GetDuration() <= FTimespan::FromMinutes(6.0))
				{
					static FText FiveMinuteTimeStamp = NSLOCTEXT("SChatWindow", "5_Minute_TimeStamp", "5 min");
					MessageTimeAsText = FiveMinuteTimeStamp;
				}
				else
				{
					MessageTimeAsText = FText::AsTime(TimeStamp, EDateTimeStyle::Short, "GMT");
					ContinueTicking = false;
				}

				return ContinueTicking;
			}

			FText GetMessageTimeText() const
			{
				return MessageTimeAsText;
			}

			private:
				FText MessageTimeAsText;
				FDelegateHandle TickerHandle;
				FDateTime TimeStamp;
		};

		TSharedRef< SWidget > Widget = SNullWidget::NullWidget;

		FDateTime TimeStamp;
		FString TimeStampSting = RunInfo.Content.ToString();
		if (!TimeStampSting.IsEmpty() && FDateTime::Parse(TimeStampSting, TimeStamp))
		{
			Widget = SNew( SChatTimeStamp )
			.DateTime(TimeStamp);
		}


		TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		int16 Baseline = FontMeasure->GetBaseline(FFriendsAndChatModuleStyle::GetStyleService()->GetNormalFont());

		return FSlateWidgetRun::FWidgetRunInfo(Widget, Baseline);
	}

	FText FormatListMessageText(TSharedRef<FChatItemViewModel> ChatItem, bool bGrouped)
	{
		const FText& TextToSet = ChatItem->GetMessage();
		FFormatNamedArguments Args;
		FString HyperlinkStyle = TEXT("UserNameTextStyle.DefaultHyperlink");
		FSlateBrush* Brush = nullptr;

		switch (ChatItem->GetMessageType())
		{
		case EChatMessageType::Global: 
			HyperlinkStyle = TEXT("UserNameTextStyle.GlobalHyperlink"); 
			Args.Add(TEXT("RoomName"), FText::FromString(TEXT("[g]")));
			break;
		case EChatMessageType::Whisper: 
			HyperlinkStyle = TEXT("UserNameTextStyle.Whisperlink"); 
			Args.Add(TEXT("RoomName"), FText::FromString(TEXT("[w]")));
			break;
		case EChatMessageType::Game: 
			HyperlinkStyle = TEXT("UserNameTextStyle.GameHyperlink"); 
			Args.Add(TEXT("RoomName"), FText::FromString(TEXT("[p]")));
			break;
		}

		Args.Add(TEXT("Channel"), FText::FromString(EChatMessageType::ShortcutString(ChatItem->GetMessageType())));
		Args.Add(TEXT("RecipientName"), ChatItem->GetRecipientName());
		if(ChatItem->GetRecipientID().IsValid())
		{
			Args.Add(TEXT("RecipientID"), FText::FromString(ChatItem->GetRecipientID()->ToString()));
		}
		Args.Add(TEXT("SenderName"), ChatItem->GetSenderName());
		if(ChatItem->GetSenderID().IsValid())
		{
			Args.Add(TEXT("SenderID"), FText::FromString(ChatItem->GetSenderID()->ToString()));
		}

		FString MessageTime = ChatItem->GetMessageTime().ToString();
		FString TimeStampMarkup = "<TimeStamp>" + MessageTime + "</>";
		Args.Add(TEXT("TimeStampMarkup"), FText::FromString(TimeStampMarkup));

		Args.Add(TEXT("NameStyle"), FText::FromString(HyperlinkStyle));
		Args.Add(TEXT("Message"), TextToSet);

		FText ListMessage;

		if (bGrouped)
		{
			ListMessage = FText::Format(NSLOCTEXT("SChatWindow", "DisplayMessage", "{Message}"), TextToSet);
		}
		else
		{
			if (!ViewModel->MultiChat())
			{
				if (ChatItem->IsFromSelf())
				{
					ListMessage = FText::Format(NSLOCTEXT("FChatItemViewModel", "DisplayNameAndMessage", "{TimeStampMarkup} {SenderName} : {Message}"), Args);
				}
				else
				{
					ListMessage = FText::Format(NSLOCTEXT("SChatWindow", "DisplayNameAndMessageWithMarkup", "{TimeStampMarkup} <a id=\"UserName\" uid=\"{SenderID}\" Username=\"{SenderName}\" style=\"{NameStyle}\">{SenderName}</> : {Message}"), Args);
				}
			}
			else
			{
				if (ChatItem->GetMessageType() == EChatMessageType::Whisper)
				{
					if (ChatItem->IsFromSelf())
					{
						ListMessage = FText::Format(NSLOCTEXT("SChatWindow", "DisplayNameAndMessageWhisperFromSelf", "{TimeStampMarkup} <a id=\"Channel\" Command=\"{Channel}\" style=\"{NameStyle}\">{RoomName}</> to <a id=\"UserName\" uid=\"{RecipientID}\" Username=\"{RecipientName}\" style=\"{NameStyle}\">{RecipientName}</> : {Message}"), Args);
					}
					else
					{
						ListMessage = FText::Format(NSLOCTEXT("SChatWindow", "DisplayNameAndMessageWhisper", "{TimeStampMarkup} <a id=\"Channel\" Command=\"{Channel}\" style=\"{NameStyle}\">{RoomName}</> from <a id=\"UserName\" uid=\"{SenderID}\" Username=\"{SenderName}\" style=\"{NameStyle}\">{SenderName}</> : {Message}"), Args);
					}
				}
				else
				{
					if (ChatItem->IsFromSelf())
					{
						ListMessage = FText::Format(NSLOCTEXT("SChatWindow", "DisplayNameAndMessageOtherFromSelf", "{TimeStampMarkup} <a id=\"Channel\" Command=\"{Channel}\" style=\"{NameStyle}\">{RoomName}</> {SenderName} : {Message}"), Args);
					}
					else
					{
						ListMessage = FText::Format(NSLOCTEXT("SChatWindow", "DisplayNameAndMessageOther", "{TimeStampMarkup} <a id=\"Channel\" Command=\"{Channel}\" style=\"{NameStyle}\">{RoomName}</> <a id=\"UserName\" uid=\"{SenderID}\" Username=\"{SenderName}\" style=\"{NameStyle}\">{SenderName}</> : {Message}"), Args);
					}
				}
			}
		}
		return ListMessage;
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
	
private:

	TSharedPtr<SScrollBox> ChatScrollBox;

	TSharedPtr<FRichTextLayoutMarshaller> RichTextMarshaller;

	TSharedPtr<SMultiLineEditableTextBox> RichText;

	SVerticalBox::FSlot* ChannelTextSlot;

	FTextBuilder ChatText;

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

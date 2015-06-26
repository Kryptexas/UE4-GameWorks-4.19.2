// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendsFontStyleService.h"
#include "SChatEntryWidget.h"
#include "RichTextLayoutMarshaller.h"
#include "ChatViewModel.h"
#include "ChatEntryCommands.h"

#define LOCTEXT_NAMESPACE ""

class SChatEntryWidgetImpl : public SChatEntryWidget
{
public:

	/**
	 * Construct the chat entry widget.
	 * @param InArgs		Widget args
	 * @param InViewModel	The chat view model - used for accessing chat markup etc.
	 */
	void Construct(const FArguments& InArgs, const TSharedRef<FChatViewModel>& InViewModel)
	{
		ViewModel = InViewModel;
		ViewModel->OnTextValidated().AddSP(this, &SChatEntryWidgetImpl::HandleValidatedText);
		ViewModel->OnChatListSetFocus().AddSP(this, &SChatEntryWidgetImpl::SetFocus);
		FriendStyle = *InArgs._FriendStyle;
		float HightOverride = FriendStyle.FriendsChatStyle.ChatEntryHeight;
		MaxChatLength = InArgs._MaxChatLength;
		HintText = InArgs._HintText;
		Marshaller = InArgs._Marshaller;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
			 	SNew(SBox)
				.Visibility(ViewModel->AllowMarkup() ? EVisibility::Visible : EVisibility::Collapsed)
			 	.WidthOverride(HightOverride)
			 	[
					SNew(SButton)
					.OnClicked(this, &SChatEntryWidgetImpl::HandleActionDropDownClicked)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.Cursor(EMouseCursor::Hand)
					[
						SNew(STextBlock)
						.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontLargeBold)
						.Text(FText::FromString("/"))
					]
				 ]
			]
			+SHorizontalBox::Slot()
			[
				SAssignNew(ChatBox, SBox)
			]
		]);

		RebuildTextEntry();
	}

	void RebuildTextEntry() override
	{
		FText EnteredText;
		if (ChatTextBox.IsValid())
		{
			EnteredText = ChatTextBox->GetText();
			ChatTextBox.Reset();
		}
		if (!ChatTextBox.IsValid())
		{
			SAssignNew(ChatTextBox, SMultiLineEditableTextBox)
				.Style(&FriendStyle.FriendsChatStyle.ChatEntryTextStyle)
				.Font(FFriendsAndChatModuleStyle::GetStyleService()->GetNormalFont())
				.ClearKeyboardFocusOnCommit(false)
				.OnTextCommitted(this, &SChatEntryWidgetImpl::HandleChatEntered)
				.OnKeyDownHandler(this, &SChatEntryWidgetImpl::HandleChatKeydown)
				.HintText(HintText)
				.OnTextChanged(this, &SChatEntryWidgetImpl::HandleChatTextChanged)
				.IsEnabled(this, &SChatEntryWidgetImpl::IsChatEntryEnabled)
				.ModiferKeyForNewLine(EModifierKey::Shift)
				.Marshaller(Marshaller)
				.WrapTextAt(this, &SChatEntryWidgetImpl::GetMessageBoxWrapWidth);

			if (!EnteredText.IsEmpty())
			{
				ChatTextBox->SetText(EnteredText);
			}
		}

		ChatBox->SetContent(ChatTextBox.ToSharedRef());
	}

	// Begin SUserWidget

	virtual bool SupportsKeyboardFocus() const override
	{
		return true;
	}

	/**
	 * Set the focus to the chat entry bar when this widget is given focus.
	 */
	virtual FReply OnFocusReceived( const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent ) override
	{
		if(ViewModel->ShouldCaptureFocus())
		{
			return FReply::Handled().ReleaseMouseCapture();
		}
	
		SetFocus();
		return FReply::Handled();
	}

	/**
	 * Hack to find the wrap text width of the current widget. Rich text should support this in the future
	 */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		SUserWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		int32 NewWindowWidth = FMath::FloorToInt(AllottedGeometry.GetLocalSize().X);
		if(NewWindowWidth != WindowWidth)
		{
			WindowWidth = NewWindowWidth;
		}
	}

	// End SUserWidget
private:

	FReply HandleActionDropDownClicked()
	{
		ViewModel->ToggleChatTipVisibility();
		return FReply::Handled();
	}

	/**
	 * Handle chat commited - send the message to be broadcast from the message service
	 * @param CommentText	The committed text
	 * @param CommitInfo	The commit type.
	 */
	void HandleChatEntered(const FText& CommittedText, ETextCommit::Type CommitInfo)
	{
		if (CommitInfo == ETextCommit::OnEnter)
		{
			SendChatMessage();
		}
	}

	/**
	 * Handle key event from the rich text box
	 * @param Geometry	The geometry
	 * @param KeyEvent	The key event.
	 */
	FReply HandleChatKeydown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
	{
		return ViewModel->HandleChatKeyEntry(KeyEvent);
	}

	/**
	 * Handle chat entered - Check for markup after each character is added
	 * @param CurrentText	The current text in the chat box
	 */
	void HandleChatTextChanged(const FText& CurrentText)
	{
		FText PlainText = ChatTextBox->GetPlainText();
		CheckLimit(PlainText);
		ViewModel->ValidateChatInput(CurrentText, PlainText);
	}

	/**
	 * Handle event for text being validated my the markup system.
	 * Set focus to the entry box, and set the validated text
	 */
	void HandleValidatedText()
	{
		SetFocus();
		FText InputText = ViewModel->GetValidatedInput();
		ChatTextBox->SetText(InputText);
		ChatTextBox->GoTo(ETextLocation::EndOfDocument);
	}

	/**
	 * Send the entered message out through the message system.
	 */
	void SendChatMessage()
	{
		const FText& CurrentText = ChatTextBox->GetPlainText();
		if (CheckLimit(CurrentText))
		{
			if(ViewModel->SendMessage(ChatTextBox->GetText(), CurrentText))
			{
				ChatTextBox->SetText(FText::GetEmpty());
			}
			else if(!CurrentText.IsEmpty())
			{
				ChatTextBox->SetError(LOCTEXT("CouldNotSend", "Unable to send chat message"));
			}
		}
	}

	/**
	 * Set focus to the chat entry box.
	 */
	void SetFocus()
	{
		FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
		if (ChatTextBox.IsValid())
		{
			FWidgetPath WidgetToFocusPath;

			bool bFoundPath = FSlateApplication::Get().FindPathToWidget(ChatTextBox.ToSharedRef(), WidgetToFocusPath);
			if (bFoundPath && WidgetToFocusPath.IsValid())
			{
				FSlateApplication::Get().SetKeyboardFocus(WidgetToFocusPath, EKeyboardFocusCause::SetDirectly);
			}
		}
	}

	/**
	 * Check the length of chat entry, and warn if too long
	 * @param CurrentText	The current text in the chat box
	 */
	bool CheckLimit(const FText& CurrentText)
	{
		int32 OverLimit = CurrentText.ToString().Len() - MaxChatLength;
		if (OverLimit > 0)
		{
			ChatTextBox->SetError(FText::Format(LOCTEXT("TooManyCharactersFmt", "{0} Characters Over Limit"), FText::AsNumber(OverLimit)));
			return false;
		}

		// no error
		ChatTextBox->SetError(FText());
		return true;
	}

	/**
	 * Disable chat entry if we are not connected to chat
	 */
	bool IsChatEntryEnabled() const
	{
		return ViewModel->IsChatConnected();
	}

	/**
	 * Hack to find the wrap text width of the current widget. Rich text should support this in the future.
	 * Return the window width minus a boarder
	 */
	float GetMessageBoxWrapWidth() const
	{
		return FMath::Max(WindowWidth - 60, 0.0f);
	}

private:
	/* Holds the max chat length */
	int32 MaxChatLength;
	/* Holds the current window width (Hack until Rich Text can figure this out) */
	mutable float WindowWidth;
	/* Holds the Chat Text */
	TSharedPtr<SBox> ChatBox;
	/* Holds the Chat entry box */
	TSharedPtr<SMultiLineEditableTextBox> ChatTextBox;
	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;
	/* Holds the Chat view model */
	TSharedPtr<FChatViewModel> ViewModel;
	/* Holds the hint text for the chat box*/
	TAttribute< FText > HintText;
	/* Holds the marshaller for the chat box*/
	TSharedPtr<ITextLayoutMarshaller> Marshaller;
};

TSharedRef<SChatEntryWidget> SChatEntryWidget::New()
{
	return MakeShareable(new SChatEntryWidgetImpl());
}

#undef LOCTEXT_NAMESPACE

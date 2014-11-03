// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatViewModel.h"
#include "ChatItemViewModel.h"


#define LOCTEXT_NAMESPACE "SChatWindow"

class SChatWindowImpl : public SChatWindow
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FChatViewModel>& InViewModel)
	{
		// Dragable bar for the Friends list
		class SDraggableBar : public SBox
		{
			SLATE_BEGIN_ARGS(SDraggableBar)
			{}
			SLATE_DEFAULT_SLOT(FArguments, Content)
			SLATE_END_ARGS()
			void Construct( const FArguments& InArgs )
			{
				SBox::Construct(
					SBox::FArguments()
					.Padding( 12.f )
					[
						InArgs._Content.Widget
					]
				);
				this->Cursor = EMouseCursor::CardinalCross;
			}

			virtual EWindowZone::Type GetWindowZoneOverride() const override
			{
				return EWindowZone::TitleBar;
			}
		};

		OnCloseClicked = InArgs._OnCloseClicked;
		OnMinimizeClicked = InArgs._OnMinimizeClicked;
		FriendStyle = *InArgs._FriendStyle;
		MenuMethod = InArgs._Method;
		this->ViewModel = InViewModel;
		FChatViewModel* ViewModelPtr = ViewModel.Get();
		ViewModel->OnChatListUpdated().AddSP(this, &SChatWindowImpl::RefreshChatList);


		EVisibility MinimizeButtonVisibility = OnMinimizeClicked.IsBound() ? EVisibility::Visible : EVisibility::Collapsed;
		EVisibility CloseButtonVisibility = OnCloseClicked.IsBound() ? EVisibility::Visible : EVisibility::Collapsed;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Top)
			[
				SNew(SOverlay)
				.Visibility(MinimizeButtonVisibility)
				+SOverlay::Slot()
				.VAlign( VAlign_Top )
				.HAlign( HAlign_Fill )
				[
					SNew( SBorder )
					.BorderImage(&FriendStyle.TitleBarBrush)
					.BorderBackgroundColor(FLinearColor::White)
					[
						SNew(SDraggableBar)
					]
				]
				+SOverlay::Slot()
				.VAlign( VAlign_Top )
				.HAlign( HAlign_Right )
				[
					SNew( SHorizontalBox )
					+SHorizontalBox::Slot()
					.HAlign( HAlign_Right )
					.AutoWidth()
					.Padding( 5.0f )
					[
						SNew( SButton )
						.Visibility( MinimizeButtonVisibility )
						.IsFocusable( false )
						.ContentPadding( 0 )
						.Cursor( EMouseCursor::Default )
						.ButtonStyle( &FriendStyle.MinimizeButtonStyle )
						.OnClicked(this, &SChatWindowImpl::MinimizeButton_OnClicked)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding( 5.0f )
					[
						SNew( SButton )
						.Visibility( CloseButtonVisibility )
						.IsFocusable( false )
						.ContentPadding( 0 )
						.Cursor( EMouseCursor::Default )
						.ButtonStyle(  &FriendStyle.CloseButtonStyle  )
						.OnClicked( this, &SChatWindowImpl::CloseButton_OnClicked )
					]
				]
			]
			+SVerticalBox::Slot()
			.Padding(FMargin(0,5))
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Fill)
			[
				SAssignNew(ChatList, SListView<TSharedRef<FChatItemViewModel>>)
				.ListItemsSource(&ViewModel->GetFilteredChatList())
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SChatWindowImpl::MakeChatWidget)
				.OnSelectionChanged(ViewModelPtr, &FChatViewModel::HandleSelectionChanged)
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0,5))
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Left)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew(ActionMenu, SMenuAnchor)
					.Placement(EMenuPlacement::MenuPlacement_AboveAnchor)
					.Method(SMenuAnchor::UseCurrentWindow)
					.MenuContent(GetMenuContent())
					[
						SNew(SButton)
						.ButtonStyle(FCoreStyle::Get(), "NoBorder")
						.ContentPadding(FMargin(5, 0))
						.OnClicked(this, &SChatWindowImpl::HandleActionDropDownClicked)
						.Cursor(EMouseCursor::Hand)
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.VAlign(VAlign_Top)
							.AutoHeight()
							[
								SNew(SImage)
								.ColorAndOpacity(FLinearColor::White)
								.Image(&FriendStyle.FriendsComboDropdownImageBrush)
							]
							+SVerticalBox::Slot()
							.VAlign(VAlign_Center)
							.AutoHeight()
							[
								SNew(STextBlock)
								.ColorAndOpacity(FLinearColor::White)
								.Text(ViewModelPtr, &FChatViewModel::GetViewGroupText)
							]
						]
					]
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.Padding(FMargin(5))
				[
					SNew(STextBlock)
					.Text(ViewModelPtr, &FChatViewModel::GetChatGroupText)
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0,5))
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Fill)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SAssignNew(ChatBox, SEditableTextBox)
					.OnTextCommitted(this, &SChatWindowImpl::HandleChatEntered)
					.HintText(LOCTEXT("FriendsListSearch", "Enter Chat"))
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.OnClicked(this, &SChatWindowImpl::HandleSendClicked)
					.Text(FText::FromString("Send"))
				]
			]
		]);

		RefreshChatList();
	}

private:

	TSharedRef<ITableRow> MakeChatWidget(TSharedRef<FChatItemViewModel> ChatMessage, const TSharedRef<STableViewBase>& OwnerTable)
	{
		FSlateColor DisplayColor = ChatMessage->GetDisplayColor();
		const EVisibility FromSelfVisibility = ChatMessage->IsFromSelf() ? EVisibility::Visible : EVisibility::Collapsed;
		const EVisibility FriendNameVisibility = !ChatMessage->IsFromSelf() || ChatMessage->GetMessageType() == EChatMessageType::Whisper ? EVisibility::Visible : EVisibility::Collapsed;

		return SNew(STableRow< TSharedPtr<SWidget> >, OwnerTable)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.Padding(FMargin(5,1))
			.AutoWidth()
			[
				// Temporary indication that this is from the user
				SNew(STextBlock)
				.Visibility(FromSelfVisibility)
				.Text(FText::FromString(">"))
				.ColorAndOpacity(FLinearColor::White)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5,1))
			[
				SNew(STextBlock)
				.Text(ChatMessage->GetMessageTypeText())
				.ColorAndOpacity(DisplayColor)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5,1))
			[
				SNew(STextBlock)
				.Visibility(FriendNameVisibility)
				.Text(ChatMessage->GetFriendID())
				.ColorAndOpacity(DisplayColor)
			]
			+SHorizontalBox::Slot()
			.Padding(FMargin(5,1))
			[
				SNew(STextBlock)
				.Text(ChatMessage->GetMessage())
				.ColorAndOpacity(DisplayColor)
				.AutoWrapText(true)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.Padding(FMargin(5,1))
			[
				SNew(STextBlock)
				.Text(ChatMessage->GetMessageTime())
				.ColorAndOpacity(DisplayColor)
				.AutoWrapText(true)
			]
		];
	}

	FReply CloseButton_OnClicked()
	{
		if ( OnCloseClicked.IsBound() )
		{
			OnCloseClicked.Execute();
		}
		return FReply::Handled();
	}

	FReply MinimizeButton_OnClicked()
	{
		if (OnMinimizeClicked.IsBound())
		{
			OnMinimizeClicked.Execute();
		}
		return FReply::Handled();
	}

	FReply HandleActionDropDownClicked() const
	{
		ActionMenu->SetIsOpen(true);
		return FReply::Handled();
	}

	/**
	 * Generate the action menu.
	 * @return the action menu widget
	 */
	TSharedRef<SWidget> GetMenuContent()
	{
		TSharedPtr<SVerticalBox> ChannelOptions;
		SAssignNew(ChannelOptions, SVerticalBox);

		TArray<EChatMessageType::Type> ChatMessageOptions;

		ViewModel->EnumerateChatChannelOptionsList(ChatMessageOptions);

		for( const auto& Option : ChatMessageOptions)
		{
			ChannelOptions->AddSlot()
			[
				SNew(SButton)
				.OnClicked(this, &SChatWindowImpl::HandleChannelChanged, Option)
				.Text(EChatMessageType::ToText(Option))
			];
		};

		return SNew(SBorder)
			.Padding(FMargin(1, 5))
			[
				ChannelOptions.ToSharedRef()
			];
	}

	FReply HandleChannelChanged(const EChatMessageType::Type NewOption)
	{
		ViewModel->SetViewChannel(NewOption);
		ActionMenu->SetIsOpen(false);
		return FReply::Handled();
	}

	void CreateChatList()
	{
		if(ViewModel->GetFilteredChatList().Num())
		{
			ChatList->RequestListRefresh();
			ChatList->RequestScrollIntoView(ViewModel->GetFilteredChatList().Last());
		}
	}

	void HandleChatEntered(const FText& CommentText, ETextCommit::Type CommitInfo)
	{
		if (CommitInfo == ETextCommit::OnEnter)
		{
			SendChatMessage();
		}
	}

	FReply HandleSendClicked()
	{
		SendChatMessage();
		return FReply::Handled();
	}

	void SendChatMessage()
	{
		ViewModel->SendMessage(ChatBox->GetText());
		ChatBox->SetText(FText::GetEmpty());
		SetFocus();
	}

	void RefreshChatList()
	{
		CreateChatList();
	}

	void SetFocus()
	{
		if (ChatBox.IsValid())
		{
			FWidgetPath WidgetToFocusPath;

			bool bFoundPath = FSlateApplication::Get().FindPathToWidget(ChatBox.ToSharedRef(), WidgetToFocusPath);
			if (bFoundPath && WidgetToFocusPath.IsValid())
			{
				FSlateApplication::Get().SetKeyboardFocus(WidgetToFocusPath, EKeyboardFocusCause::SetDirectly);
			}
		}
	}
private:

	// Holds the chat list
	TSharedPtr<SListView<TSharedRef<FChatItemViewModel> > > ChatList;

	// Holds the chat list display
	TSharedPtr<SEditableTextBox> ChatBox;

	// Holds the menu anchor
	TSharedPtr<SMenuAnchor> ActionMenu;

	// Holds the Friends List view model
	TSharedPtr<FChatViewModel> ViewModel;

	/** Holds the delegate for when the close button is clicked. */
	FOnClicked OnCloseClicked;

	/** Holds the delegate for when the minimize button is clicked. */
	FOnClicked OnMinimizeClicked;

	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;

	// Holds the menu method - Full screen requires use owning window or crashes.
	SMenuAnchor::EMethod MenuMethod;
};

TSharedRef<SChatWindow> SChatWindow::New()
{
	return MakeShareable(new SChatWindowImpl());
}

#undef LOCTEXT_NAMESPACE

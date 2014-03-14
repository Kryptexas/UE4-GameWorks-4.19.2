// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MessageLogPrivatePCH.h"
#include "SMessageLogListing.h"
#include "IMessageLogListing.h"
#include "UObjectToken.h"

#if WITH_EDITOR
#include "IDocumentation.h"
#endif

#define LOCTEXT_NAMESPACE "Developer.MessageLog"

class SMessageLogListingItem : public STableRow< TSharedPtr< FTokenizedMessage > >
{
public:
	DECLARE_DELEGATE_TwoParams( FOnTokenClicked, TSharedPtr<FTokenizedMessage>, const TSharedRef<class IMessageToken>& );

public:
	SLATE_BEGIN_ARGS(SMessageLogListingItem)
		: _Message()
		, _OnTokenClicked()
		{}
		SLATE_ATTRIBUTE(TSharedPtr<FTokenizedMessage>, Message)
		SLATE_EVENT( FOnTokenClicked, OnTokenClicked )
	SLATE_END_ARGS()

	/**
	 * Construct child widgets that comprise this widget.
	 *
	 * @param InArgs  Declaration from which to construct this widget
	 */
	void Construct( const FArguments& InArgs, const TSharedRef< STableViewBase >& InOwnerTableView )
	{
		this->OnTokenClicked = InArgs._OnTokenClicked;

		Message = InArgs._Message.Get();

		STableRow< TSharedPtr< FTokenizedMessage > >::Construct(
			STableRow< TSharedPtr< FTokenizedMessage > >::FArguments()
			.Content()
			[
				GenerateWidget()
			],
			InOwnerTableView );
	}

	/** @return Widget for this log listing item*/
	virtual TSharedRef<SWidget> GenerateWidget()
	{
		// See if we have any valid tokens which match the column name
		const TArray< TSharedRef<IMessageToken> >& MessageTokens = Message->GetMessageTokens();

		// Collect entire message for tooltip
		FText FullMessage = Message->ToText();

		// Create the horizontal box and add the icon
		TSharedPtr<SHorizontalBox> HorzBox;
		TSharedRef<SWidget> CellContent =
			SNew(SBorder)
			.OnMouseButtonUp(this, &SMessageLogListingItem::Message_OnMouseButtonUp)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			.Padding( FMargin(2.f, 2.f, 2.f, 1.f) )
			[
				SAssignNew(HorzBox, SHorizontalBox)
				.ToolTipText(FullMessage)
			];
		check(HorzBox.IsValid());

		// Iterate over parts of the message and create widgets for them
		for (auto TokenIt = MessageTokens.CreateConstIterator(); TokenIt; ++TokenIt)
		{
			const TSharedRef<IMessageToken>& Token = *TokenIt;
			CreateMessage( HorzBox, Token );
		}

		return CellContent;
	}

protected:
	TSharedPtr<SWidget> CreateHyperlink(const TSharedRef<IMessageToken>& InMessageToken, const FText& InToolTip = FText() )
	{
		return SNew(SHyperlink)
			.Text(InMessageToken->ToText())
			.ToolTipText(InToolTip)
			.TextStyle(FEditorStyle::Get(), "MessageLog")
			.OnNavigate(this, &SMessageLogListingItem::Hyperlink_OnNavigate, InMessageToken);
	}

	void CreateMessage( TSharedPtr<SHorizontalBox> InHorzBox, const TSharedRef<IMessageToken>& InMessageToken )
	{
		if ( InHorzBox.IsValid() )
		{
			TSharedPtr<SWidget> Content;

			switch(InMessageToken->GetType())
			{
			case EMessageToken::Text:
			case EMessageToken::String:
				{
					if(InMessageToken->GetOnMessageTokenActivated().IsBound())
					{
						Content = CreateHyperlink(InMessageToken);
					}
					else
					{
						SAssignNew(Content, STextBlock)
							.Text( InMessageToken->ToText() )
							.TextStyle( FEditorStyle::Get(), TEXT("MessageLog") );
					}
				}
				break;
			case EMessageToken::Image:
				{
					const TSharedRef<FImageToken> ImageToken = StaticCastSharedRef<FImageToken>(InMessageToken);
					if(ImageToken->GetImageName() != NAME_None)
					{
						if(InMessageToken->GetOnMessageTokenActivated().IsBound())
						{
							SAssignNew(Content, SButton)
								.OnClicked(this, &SMessageLogListingItem::Button_OnClicked, InMessageToken)
								.Content()
								[
									SNew(SImage)
									.Image( FEditorStyle::GetBrush( ImageToken->GetImageName() ) )
								];
						}
						else
						{
							SAssignNew(Content, SImage)
								.Image( FEditorStyle::GetBrush( ImageToken->GetImageName() ) );
						}
					}
				}
				break;
			case EMessageToken::Severity:
				{
					const TSharedRef<FSeverityToken> SeverityToken = StaticCastSharedRef<FSeverityToken>(InMessageToken);
					FName SeverityBrush = FTokenizedMessage::GetSeverityIconName(SeverityToken->GetSeverity());
					if(SeverityBrush != NAME_None)
					{
						if(InMessageToken->GetOnMessageTokenActivated().IsBound())
						{
							SAssignNew(Content, SButton)
								.OnClicked(this, &SMessageLogListingItem::Button_OnClicked, InMessageToken)
								.Content()
								[
									SNew(SImage)
									.Image( FEditorStyle::GetBrush( SeverityBrush ) )
								];
						}
						else
						{
							SAssignNew(Content, SImage)
								.Image( FEditorStyle::GetBrush( SeverityBrush ) );
						}
					}
				}
				break;
			case EMessageToken::Object:
				{
					const TSharedRef<FUObjectToken> UObjectToken = StaticCastSharedRef<FUObjectToken>(InMessageToken);
					Content = CreateHyperlink(InMessageToken, FUObjectToken::DefaultOnGetObjectDisplayName().IsBound() ? FUObjectToken::DefaultOnGetObjectDisplayName().Execute(UObjectToken->GetObject().Get(), true) : UObjectToken->ToText());
				}
				break;
			case EMessageToken::URL:
				{
					const TSharedRef<FURLToken> URLToken = StaticCastSharedRef<FURLToken>(InMessageToken);
					Content = CreateHyperlink(InMessageToken, FText::FromString( URLToken->GetURL() ) );
				}
				break;
			case EMessageToken::AssetName:
				{
					const TSharedRef<FAssetNameToken> AssetNameToken = StaticCastSharedRef<FAssetNameToken>(InMessageToken);
					Content = CreateHyperlink(InMessageToken, AssetNameToken->ToText());
				}
				break;
			case EMessageToken::Documentation:
				{
#if WITH_EDITOR
					const TSharedRef<FDocumentationToken> DocumentationToken = StaticCastSharedRef<FDocumentationToken>(InMessageToken);
					Content = IDocumentation::Get()->CreateAnchor( DocumentationToken->GetDocumentationLink(), DocumentationToken->GetPreviewExcerptLink(), DocumentationToken->GetPreviewExcerptName() );
#endif
				}
				break;
			}

			AddContentToBox( InHorzBox, Content );
		}
	}
	
	void AddContentToBox( TSharedPtr<SHorizontalBox> InHorzBox, TSharedPtr<SWidget> InContent ) const
	{
		if ( InHorzBox.IsValid() && InContent.IsValid() )
		{
			InHorzBox->AddSlot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				.Padding(0.f, 0.f, 2.f, 0.f)
				[
					InContent.ToSharedRef()
				];
		}
	}

	void Hyperlink_OnNavigate( TSharedRef<IMessageToken> InMessageToken )
	{
		InMessageToken->GetOnMessageTokenActivated().ExecuteIfBound( InMessageToken );
		OnTokenClicked.ExecuteIfBound(Message, InMessageToken);
	}

	FReply Button_OnClicked( TSharedRef<IMessageToken> InMessageToken )
	{
		InMessageToken->GetOnMessageTokenActivated().ExecuteIfBound( InMessageToken );
		OnTokenClicked.ExecuteIfBound(Message, InMessageToken);
		return FReply::Handled();
	}

	FReply Message_OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			Message->GetRightClickedMethod().ExecuteIfBound(Message);
		}

		return FReply::Handled();
	}

protected:

	/** The message used to create this widget */
	TSharedPtr<FTokenizedMessage> Message;

	/** Delegate to execute when the token is clicked */
	FOnTokenClicked OnTokenClicked;
};


SMessageLogListing::SMessageLogListing()
	: bUpdatingSelection( false )
	, UICommandList( MakeShareable( new FUICommandList ) )
{

}


SMessageLogListing::~SMessageLogListing()
{
	MessageLogListingViewModel->OnDataChanged().RemoveAll( this );
	MessageLogListingViewModel->OnSelectionChanged().RemoveAll( this );
}

void SMessageLogListing::Construct( const FArguments& InArgs, const TSharedRef< IMessageLogListing >& InModelView )
{
	MessageLogListingViewModel = StaticCastSharedRef<FMessageLogListingViewModel>(InModelView);

	TSharedPtr<SHorizontalBox> HorizontalBox = NULL;

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.FillHeight(1)
		[
			SAssignNew(MessageListView, SListView< TSharedRef<FTokenizedMessage> >)
				.ListItemsSource(&MessageLogListingViewModel->GetFilteredMessages())
				.OnGenerateRow(this, &SMessageLogListing::MakeMessageLogListItemWidget)
				.OnSelectionChanged(this, &SMessageLogListing::OnLineSelectionChanged)
				.ItemHeight(24)
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 1)
		[
			SNew(SSeparator)
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(HorizontalBox, SHorizontalBox)	
		]
	];

	if(MessageLogListingViewModel->GetShowFilters())
	{
		HorizontalBox->AddSlot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Left)
			[
				SNew(SComboButton)
				.ButtonStyle( FEditorStyle::Get(), "HoverHintOnly" )
				.ForegroundColor(FSlateColor::UseForeground())
				.ContentPadding(2)
				.OnGetMenuContent(this, &SMessageLogListing::OnGetFilterMenuContent)
				.ButtonContent()
				[
					SNew(STextBlock) 
					.Text(LOCTEXT("Show", "Show"))
					.ToolTipText(LOCTEXT("ShowToolTip", "Only show messages of the selected types"))
				]
			];
	}

	HorizontalBox->AddSlot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Right)
		[
			SNew(SComboButton)
			.IsEnabled(this, &SMessageLogListing::IsPageWidgetEnabled)
			.Visibility(this, &SMessageLogListing::GetPageWidgetVisibility)
			.ButtonStyle( FEditorStyle::Get(), "HoverHintOnly" )
			.ForegroundColor(FSlateColor::UseForeground())
			.ContentPadding(2)
			.OnGetMenuContent(this, &SMessageLogListing::OnGetPageMenuContent)
			.ButtonContent()
			[
				SNew(STextBlock) 
				.Text(this, &SMessageLogListing::OnGetPageMenuLabel)
				.ToolTipText(LOCTEXT("PageToolTip", "Choose the log page to view"))
			]
		];

	// if we aren't using pages, we allow the user to manually clear the log
	HorizontalBox->AddSlot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Right)
		[
			SNew(SButton)
			.OnClicked(this, &SMessageLogListing::OnClear)
			.IsEnabled(this, &SMessageLogListing::IsClearWidgetEnabled)
			.Visibility(this, &SMessageLogListing::GetClearWidgetVisibility)
			.ButtonStyle( FEditorStyle::Get(), "HoverHintOnly" )
			.ForegroundColor(FSlateColor::UseForeground())
			.ContentPadding(2)
			[
				SNew(STextBlock) 
				.Text(LOCTEXT("ClearMessageLog", "Clear"))
				.ToolTipText(LOCTEXT("ClearMessageLog_ToolTip", "Clear the messages in this log"))
			]
		];

	// Register with the the view object so that it will notify if any data changes
	MessageLogListingViewModel->OnDataChanged().AddSP( this, &SMessageLogListing::OnChanged );
	MessageLogListingViewModel->OnSelectionChanged().AddSP( this, &SMessageLogListing::OnSelectionChanged );
	UICommandList->MapAction( FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP( this, &SMessageLogListing::CopySelectedToClipboard ),
		FCanExecuteAction() );
}


void SMessageLogListing::OnChanged()
{
	ClearSelectedMessages();
	RefreshVisibility();
}

void SMessageLogListing::OnSelectionChanged()
{
	if( bUpdatingSelection )
	{
		return;
	}

	bUpdatingSelection = true;
	const auto& SelectedMessages = MessageLogListingViewModel->GetSelectedMessages();
	MessageListView->ClearSelection();
	for( auto It = SelectedMessages.CreateConstIterator(); It; ++It )
	{
		MessageListView->SetItemSelection( *It, true );
	}

	if( SelectedMessages.Num() > 0 )
	{
		ScrollToMessage( SelectedMessages[0] );
	}
	bUpdatingSelection = false;
}

void SMessageLogListing::RefreshVisibility()
{
	const TArray< TSharedRef<FTokenizedMessage> >& Messages = MessageLogListingViewModel->GetFilteredMessages();
	if(Messages.Num() > 0)
	{
		ScrollToMessage( Messages[0] );
	}
	
	MessageListView->RequestListRefresh();
}

void SMessageLogListing::BroadcastMessageTokenClicked( TSharedPtr<FTokenizedMessage> Message, const TSharedRef<IMessageToken>& Token )
{
	ClearSelectedMessages();
	SelectMessage( Message.ToSharedRef(), true );
	MessageLogListingViewModel->ExecuteToken(Token);
}

const TArray< TSharedRef<FTokenizedMessage> > SMessageLogListing::GetSelectedMessages() const
{
	return MessageLogListingViewModel->GetSelectedMessages();
}

void SMessageLogListing::SelectMessage( const TSharedRef<class FTokenizedMessage>& Message, bool bSelected ) const
{
	MessageLogListingViewModel->SelectMessage( Message, bSelected );
}

bool SMessageLogListing::IsMessageSelected( const TSharedRef<class FTokenizedMessage>& Message ) const
{
	return MessageLogListingViewModel->IsMessageSelected( Message );
}

void SMessageLogListing::ScrollToMessage( const TSharedRef<class FTokenizedMessage>& Message ) const
{
	if(!MessageListView->IsItemVisible(Message))
	{
		MessageListView->RequestScrollIntoView( Message );
	}
}

void SMessageLogListing::ClearSelectedMessages() const
{
	MessageLogListingViewModel->ClearSelectedMessages();
}

void SMessageLogListing::InvertSelectedMessages() const
{
	MessageLogListingViewModel->InvertSelectedMessages();
}

FText SMessageLogListing::GetSelectedMessagesAsText() const
{
	return MessageLogListingViewModel->GetSelectedMessagesAsText();
}


FText SMessageLogListing::GetAllMessagesAsText() const
{
	return MessageLogListingViewModel->GetAllMessagesAsText();
}

TSharedRef<ITableRow> SMessageLogListing::MakeMessageLogListItemWidget( TSharedRef<FTokenizedMessage> Message, const TSharedRef<STableViewBase>& OwnerTable )
{
	return
		SNew(SMessageLogListingItem, OwnerTable)
		.Message(Message)
		.OnTokenClicked( this, &SMessageLogListing::BroadcastMessageTokenClicked );
}

void SMessageLogListing::OnLineSelectionChanged( TSharedPtr< FTokenizedMessage > Selection, ESelectInfo::Type /*SelectInfo*/ )
{
	if( bUpdatingSelection )
	{
		return;
	}

	bUpdatingSelection = true;
	TArray< TSharedRef< FTokenizedMessage > > SelectedItems = MessageListView->GetSelectedItems();
	
	MessageLogListingViewModel->SelectMessages( SelectedItems );
	
	if ( Selection.IsValid() )
	{
		Selection->GetSelectionChangedMethod().ExecuteIfBound( SelectedItems );
	}

	bUpdatingSelection = false;
}

void SMessageLogListing::CopySelectedToClipboard() const
{
	CopyText( true, true );
}

FText SMessageLogListing::CopyText( bool bSelected, bool bClipboard ) const
{
	FText CombinedString;

	if( bSelected )
	{
		// Get the selected item and then get the selected messages as a string.
		CombinedString = GetSelectedMessagesAsText();
	}
	else
	{
		// Get the selected item and then get all the messages as a string.
		CombinedString = GetAllMessagesAsText();
	}
	if( bClipboard )
	{
		// Pass that to the clipboard.
		FPlatformMisc::ClipboardCopy( *CombinedString.ToString() );
	}

	return CombinedString;
}

const TSharedRef< const FUICommandList > SMessageLogListing::GetCommandList() const 
{ 
	return UICommandList;
}

FReply SMessageLogListing::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	return UICommandList->ProcessCommandBindings( InKeyboardEvent ) ? FReply::Handled() : FReply::Unhandled();
}

EVisibility SMessageLogListing::GetFilterMenuVisibility()
{
	if( MessageLogListingViewModel->GetShowFilters() )
	{
		return EVisibility::Visible;
	}

	return EVisibility::Hidden;
}

TSharedRef<ITableRow> SMessageLogListing::MakeShowWidget(TSharedRef<FMessageFilter> Selection, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew(STableRow< TSharedRef<FMessageFilter> >, OwnerTable)
		[
			SNew(SCheckBox)
			.IsChecked(Selection, &FMessageFilter::OnGetDisplayCheckState)
			.OnCheckStateChanged(Selection, &FMessageFilter::OnDisplayCheckStateChanged)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SImage)
					.Image(Selection->GetIcon())
				]
				+SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
					.Text(Selection->GetName().ToString())
				]
			]
		];
}

TSharedRef<SWidget> SMessageLogListing::OnGetFilterMenuContent()
{
	return 
		SNew( SListView< TSharedRef<FMessageFilter> >)
		.ListItemsSource(&MessageLogListingViewModel->GetMessageFilters())
		.OnGenerateRow(this, &SMessageLogListing::MakeShowWidget)
		.ItemHeight(24);
}

FText SMessageLogListing::OnGetPageMenuLabel() const
{
	if(MessageLogListingViewModel->GetPageCount() > 1)
	{
		return MessageLogListingViewModel->GetPageTitle(MessageLogListingViewModel->GetCurrentPageIndex());
	}
	else
	{
		return LOCTEXT("PageMenuLabel", "Page");
	}
}

TSharedRef<SWidget> SMessageLogListing::OnGetPageMenuContent() const
{
	if(MessageLogListingViewModel->GetPageCount() > 1)
	{
		FMenuBuilder MenuBuilder(true, NULL);
		for(uint32 PageIndex = 0; PageIndex < MessageLogListingViewModel->GetPageCount(); PageIndex++)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("PageName"), MessageLogListingViewModel->GetPageTitle(PageIndex));
			MenuBuilder.AddMenuEntry( 
				MessageLogListingViewModel->GetPageTitle(PageIndex), 
				FText::Format(LOCTEXT("PageMenuEntry_Tooltip", "View page: {PageName}"), Arguments), 
				FSlateIcon(),
				FExecuteAction::CreateSP(this, &SMessageLogListing::OnPageSelected, PageIndex));
		}

		return MenuBuilder.MakeWidget();
	}

	return SNullWidget::NullWidget;
}

void SMessageLogListing::OnPageSelected(uint32 PageIndex)
{
	MessageLogListingViewModel->SetCurrentPageIndex(PageIndex);
}

bool SMessageLogListing::IsPageWidgetEnabled() const
{
	return MessageLogListingViewModel->GetPageCount() > 1;
}

EVisibility SMessageLogListing::GetPageWidgetVisibility() const
{
	if(MessageLogListingViewModel->GetShowPages())
	{
		return EVisibility::Visible;
	}
	else
	{
		return EVisibility::Collapsed;
	}
}

bool SMessageLogListing::IsClearWidgetEnabled() const
{
	return MessageLogListingViewModel->NumMessages() > 0;
}

EVisibility SMessageLogListing::GetClearWidgetVisibility() const
{
	if(MessageLogListingViewModel->GetShowPages())
	{
		return EVisibility::Collapsed;
	}
	else
	{
		return EVisibility::Visible;
	}
}

FReply SMessageLogListing::OnClear()
{
	MessageLogListingViewModel->ClearMessages();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

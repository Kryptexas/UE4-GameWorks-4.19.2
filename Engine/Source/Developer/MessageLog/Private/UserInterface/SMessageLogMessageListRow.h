// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObjectToken.h"

#if WITH_EDITOR
	#include "IDocumentation.h"
#endif

#define LOCTEXT_NAMESPACE "SMessageLogMessageListRow"


class SMessageLogMessageListRow
	: public STableRow<TSharedPtr<FTokenizedMessage>>
{
public:

	DECLARE_DELEGATE_TwoParams( FOnTokenClicked, TSharedPtr<FTokenizedMessage>, const TSharedRef<class IMessageToken>& );

public:

	SLATE_BEGIN_ARGS(SMessageLogMessageListRow)
		: _Message()
		, _OnTokenClicked()
	{ }
		SLATE_ATTRIBUTE(TSharedPtr<FTokenizedMessage>, Message)
		SLATE_EVENT(FOnTokenClicked, OnTokenClicked)
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

		STableRow<TSharedPtr<FTokenizedMessage>>::Construct(
			STableRow<TSharedPtr<FTokenizedMessage>>::FArguments()
			.Content()
			[
				GenerateWidget()
			],
			InOwnerTableView
		);
	}

	/** @return Widget for this log listing item*/
	virtual TSharedRef<SWidget> GenerateWidget()
	{
		// See if we have any valid tokens which match the column name
		const TArray<TSharedRef<IMessageToken>>& MessageTokens = Message->GetMessageTokens();

		// Create the horizontal box and add the icon
		TSharedRef<SVerticalBox> DetailBox = SNew(SVerticalBox);
		TSharedRef<SHorizontalBox> TokenBox = SNew(SHorizontalBox);
		FName SeverityImageName = NAME_None;
		bool HasTokens = false;
		FText TitleText;

		// Iterate over parts of the message and create widgets for them
		for (auto TokenIt = MessageTokens.CreateConstIterator(); TokenIt; ++TokenIt)
		{
			const TSharedRef<IMessageToken>& Token = *TokenIt;

			if (Token->GetType() == EMessageToken::Severity)
			{
				if (SeverityImageName == NAME_None)
				{
					const TSharedRef<FSeverityToken> SeverityToken = StaticCastSharedRef<FSeverityToken>(Token);
					SeverityImageName = FTokenizedMessage::GetSeverityIconName(SeverityToken->GetSeverity());
				}
			}
			else if (Token->GetType() == EMessageToken::Text)
			{
				if (TitleText.IsEmpty())
				{
					TitleText = Token->ToText();
				}
				else
				{
					DetailBox->AddSlot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 6.0f)
						[
							SNew(STextBlock)
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								.Text(Token->ToText())
						];
				}
			}
			else
			{
				CreateMessage(TokenBox, Token);
				HasTokens = true;
			}		
		}

		return SNew(SHorizontalBox)
			.ToolTipText(Message->ToText())

		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
					.Padding(2.0f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					[
						(SeverityImageName == NAME_None)
							? SNullWidget::NullWidget
							: SNew(SImage)
								.Image(FEditorStyle::GetBrush(SeverityImageName))
					]
			]

		+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(2.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
					.Padding(2.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
							.AutoHeight()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 9))
									.Text(TitleText)
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 4.0f, 0.0f, 0.0f)
							[
								DetailBox
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								!HasTokens
									? SNullWidget::NullWidget
									: SNew(SSeparator)
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Right)
							[
								!HasTokens
									? SNullWidget::NullWidget
									: TokenBox
							]
					]
			];
	}

protected:

	TSharedRef<SWidget> CreateHyperlink( const TSharedRef<IMessageToken>& InMessageToken, const FText& InToolTip = FText() )
	{
		return SNew(SHyperlink)
			.Text(InMessageToken->ToText())
			.ToolTipText(InToolTip)
			.TextStyle(FEditorStyle::Get(), "MessageLog")
			.OnNavigate(this, &SMessageLogMessageListRow::HandleHyperlinkNavigate, InMessageToken);
	}

	void CreateMessage( const TSharedRef<SHorizontalBox>& InHorzBox, const TSharedRef<IMessageToken>& InMessageToken )
	{
		TSharedPtr<SWidget> Content;
		FName IconBrushName;

		switch(InMessageToken->GetType())
		{/*
		case EMessageToken::Image:
			{
				const TSharedRef<FImageToken> ImageToken = StaticCastSharedRef<FImageToken>(InMessageToken);
				
				if (ImageToken->GetImageName() != NAME_None)
				{
					if (InMessageToken->GetOnMessageTokenActivated().IsBound())
					{
						Content = SNew(SButton)
							.OnClicked(this, &SMessageLogMessageListRow::HandleTokenButtonClicked, InMessageToken)
							.Content()
							[
								SNew(SImage)
									.Image(FEditorStyle::GetBrush(ImageToken->GetImageName()))
							];
					}
					else
					{
						Content = SNew(SImage)
							.Image(FEditorStyle::GetBrush(ImageToken->GetImageName()));
					}
				}
			}
			break;
*/
		case EMessageToken::Object:
			{
				const TSharedRef<FUObjectToken> UObjectToken = StaticCastSharedRef<FUObjectToken>(InMessageToken);
				
				IconBrushName = FName("PropertyWindow.Button_Browse");
				Content = CreateHyperlink(InMessageToken, FUObjectToken::DefaultOnGetObjectDisplayName().IsBound()
					? FUObjectToken::DefaultOnGetObjectDisplayName().Execute(UObjectToken->GetObject().Get(), true)
					: UObjectToken->ToText());
			}
			break;

		case EMessageToken::URL:
			{
				const TSharedRef<FURLToken> URLToken = StaticCastSharedRef<FURLToken>(InMessageToken);

				IconBrushName = FName("MessageLog.Url");
				Content = CreateHyperlink(InMessageToken, FText::FromString(URLToken->GetURL()));
			}
			break;

		case EMessageToken::Action:
			{
				const TSharedRef<FActionToken> ActionToken = StaticCastSharedRef<FActionToken>(InMessageToken);

				IconBrushName = FName("MessageLog.Action");
				Content = SNew(SHyperlink)
					.Text(InMessageToken->ToText())
					.ToolTipText(ActionToken->GetActionDescription())
					.TextStyle(FEditorStyle::Get(), "MessageLog")
					.OnNavigate(this, &SMessageLogMessageListRow::HandleActionHyperlinkNavigate, ActionToken);
			}
			break;

		case EMessageToken::AssetName:
			{
				const TSharedRef<FAssetNameToken> AssetNameToken = StaticCastSharedRef<FAssetNameToken>(InMessageToken);

				IconBrushName = FName("PropertyWindow.Button_Browse");
				Content = CreateHyperlink(InMessageToken, AssetNameToken->ToText());
			}
			break;

		case EMessageToken::Documentation:
			{
#if WITH_EDITOR
				const TSharedRef<FDocumentationToken> DocumentationToken = StaticCastSharedRef<FDocumentationToken>(InMessageToken);

				IconBrushName = FName("MessageLog.Docs");
				Content = SNew(SHyperlink)
					.Text(LOCTEXT("DocsLabel", "Docs"))
					.ToolTip(IDocumentation::Get()->CreateToolTip(
						LOCTEXT("DocumentationTokenToolTip", "Click to open documentation"),
						NULL,
						DocumentationToken->GetPreviewExcerptLink(),
						DocumentationToken->GetPreviewExcerptName())
					)
					.TextStyle(FEditorStyle::Get(), "MessageLog")
					.OnNavigate(this, &SMessageLogMessageListRow::HandleDocsHyperlinkNavigate, DocumentationToken->GetDocumentationLink());
#endif
			}
			break;
		}

		if (Content.IsValid() && (IconBrushName != NAME_None))
		{
			InHorzBox->AddSlot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(16.f, 4.0f, 0.0f, 12.0f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
								.ColorAndOpacity(FSlateColor::UseForeground())
								.Image(FEditorStyle::GetBrush(IconBrushName))
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(2.0f, 0.0f, 0.0f, 0.0f)
						[
							Content.ToSharedRef()
						]
				];
		}
	}

private:

	void HandleActionHyperlinkNavigate( TSharedRef<FActionToken> ActionToken )
	{
		ActionToken->ExecuteAction();
	}

	void HandleDocsHyperlinkNavigate( FString DocumentationLink )
	{
		IDocumentation::Get()->Open(DocumentationLink);
	}

	void HandleHyperlinkNavigate( TSharedRef<IMessageToken> InMessageToken )
	{
		InMessageToken->GetOnMessageTokenActivated().ExecuteIfBound(InMessageToken);
		OnTokenClicked.ExecuteIfBound(Message, InMessageToken);
	}

	FReply HandleTokenButtonClicked( TSharedRef<IMessageToken> InMessageToken )
	{
		InMessageToken->GetOnMessageTokenActivated().ExecuteIfBound(InMessageToken);
		OnTokenClicked.ExecuteIfBound(Message, InMessageToken);
		return FReply::Handled();
	}

protected:

	/** The message used to create this widget. */
	TSharedPtr<FTokenizedMessage> Message;

	/** Delegate to execute when the token is clicked. */
	FOnTokenClicked OnTokenClicked;
};


#undef LOCTEXT_NAMESPACE

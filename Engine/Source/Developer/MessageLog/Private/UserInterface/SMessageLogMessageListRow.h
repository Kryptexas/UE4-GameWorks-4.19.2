// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObjectToken.h"

#if WITH_EDITOR
	#include "IDocumentation.h"
	#include "IIntroTutorials.h"
#endif

#define LOCTEXT_NAMESPACE "SMessageLogMessageListRow"


class MESSAGELOG_API SMessageLogMessageListRow
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
	void Construct( const FArguments& InArgs, const TSharedRef< STableViewBase >& InOwnerTableView );

public:

	/** @return Widget for this log listing item*/
	virtual TSharedRef<SWidget> GenerateWidget();

protected:

	TSharedRef<SWidget> CreateHyperlink( const TSharedRef<IMessageToken>& InMessageToken, const FText& InToolTip = FText() );

	void CreateMessage( const TSharedRef<SHorizontalBox>& InHorzBox, const TSharedRef<IMessageToken>& InMessageToken, float Padding );

private:

	void HandleActionHyperlinkNavigate( TSharedRef<FActionToken> ActionToken )
	{
		ActionToken->ExecuteAction();
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

#if WITH_EDITOR
	void HandleDocsHyperlinkNavigate( FString DocumentationLink )
	{
		IDocumentation::Get()->Open(DocumentationLink);
	}

	void HandleTutorialHyperlinkNavigate( FString TutorialAssetName )
	{
		IIntroTutorials::Get().LaunchTutorial(TutorialAssetName);
	}
#endif

protected:

	/** The message used to create this widget. */
	TSharedPtr<FTokenizedMessage> Message;

	/** Delegate to execute when the token is clicked. */
	FOnTokenClicked OnTokenClicked;
};


#undef LOCTEXT_NAMESPACE

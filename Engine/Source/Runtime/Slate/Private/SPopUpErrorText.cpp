// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"


void SPopupErrorText::Construct( const FArguments& InArgs )
{
	SComboButton::Construct( SComboButton::FArguments()
		.ButtonStyle( FCoreStyle::Get(), "NoBorder" )
		.Method( InArgs._ShowInNewWindow ? SMenuAnchor::CreateNewWindow : SMenuAnchor::UseCurrentWindow )
		.HasDownArrow(false)
		.ContentPadding(0)
		.ButtonContent()
		[
			SAssignNew( HasErrorSymbol, SErrorText )
			.ErrorText(NSLOCTEXT("UnrealEd", "Error", "!"))
		]
		.MenuBorderBrush(FCoreStyle::Get().GetBrush("NoBorder"))
		.MenuBorderPadding(FMargin(0))
		.MenuPlacement(MenuPlacement_BelowAnchor)
		.MenuContent()
		[
			SAssignNew( ErrorText, SErrorText )
		]
	);
}

void SPopupErrorText::SetError( const FText& InErrorText )
{
	SetError( InErrorText.ToString() );
}

void SPopupErrorText::SetError( const FString& InErrorText )
{
	const bool bHasError = !InErrorText.IsEmpty();

	ErrorText->SetError(InErrorText);
	HasErrorSymbol->SetError( bHasError ? NSLOCTEXT("UnrealEd", "Error", "!") : FText::GetEmpty() );

	this->SetIsOpen( bHasError, false );
}
		
bool SPopupErrorText::HasError() const
{
	return ErrorText->HasError();
}

TSharedRef<SWidget> SPopupErrorText::AsWidget()
{
	return SharedThis(this);
}



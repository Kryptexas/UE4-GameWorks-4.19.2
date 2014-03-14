// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DocumentationModulePrivatePCH.h"
#include "SDocumentationAnchor.h"
#include "DocumentationLink.h"
#include "IDocumentation.h"

void SDocumentationAnchor::Construct(const FArguments& InArgs, const FString& InLink )
{
	Link = InLink;
	
	struct StringToText_Local
	{
		static FText PassThroughAttribute( TAttribute<FString> StringAttribute )
		{
			return FText::FromString( StringAttribute.Get( FString() ) );
		}
	};

	TAttribute<FText> ToolTipText;
	if ( InArgs._ToolTipText.IsBound() )
	{
		ToolTipText = TAttribute< FText >::Create( TAttribute< FText >::FGetter::CreateStatic( &StringToText_Local::PassThroughAttribute, InArgs._ToolTipText ) );
	}
	else
	{
		ToolTipText = FText::FromString( InArgs._ToolTipText.Get( FString() ) );
	}

	if ( !ToolTipText.IsBound() && ToolTipText.Get().IsEmpty() )
	{
		ToolTipText = NSLOCTEXT("DocumentationAnchor", "DefaultToolTip", "Click to open documentation");
	}

	Default = FEditorStyle::GetBrush( "HelpIcon" );
	Hovered = FEditorStyle::GetBrush( "HelpIcon.Hovered" );
	Pressed = FEditorStyle::GetBrush( "HelpIcon.Pressed" );

	FString PreviewLink = InArgs._PreviewLink;
	if (!PreviewLink.IsEmpty())
	{
		// All in-editor udn documents must live under the Shared/ folder
		ensure(PreviewLink.StartsWith(TEXT("Shared/")));
	}

	ChildSlot
	[
		SAssignNew( Button, SButton )
		.ContentPadding( 0 )
		.ButtonStyle( FEditorStyle::Get(), "HelpButton" )
		.OnClicked( this, &SDocumentationAnchor::OnClicked )
		.HAlign( HAlign_Center )
		.VAlign( VAlign_Center )
		.ToolTip( IDocumentation::Get()->CreateToolTip( ToolTipText, NULL, PreviewLink, InArgs._PreviewExcerptName ) )
		[
			SAssignNew(ButtonImage, SImage)
			.Image( this, &SDocumentationAnchor::GetButtonImage )
		]
	];
}

const FSlateBrush* SDocumentationAnchor::GetButtonImage() const
{
	if ( Button->IsPressed() )
	{
		return Pressed;
	}

	if ( ButtonImage->IsHovered() )
	{
		return Hovered;
	}

	return Default;
}

FReply SDocumentationAnchor::OnClicked() const
{
	IDocumentation::Get()->Open( Link );
	return FReply::Handled();
}

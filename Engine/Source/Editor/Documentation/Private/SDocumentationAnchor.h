// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class SDocumentationAnchor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SDocumentationAnchor )
	{}
		SLATE_ARGUMENT( FString, PreviewLink )
		SLATE_ARGUMENT( FString, PreviewExcerptName )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const FString& InLink );

private:
	
	const FSlateBrush* GetButtonImage() const;

	FReply OnClicked() const;

private:

	FString Link;
	TSharedPtr< SButton > Button;
	TSharedPtr< SImage > ButtonImage;
	const FSlateBrush* Default; 
	const FSlateBrush* Hovered; 
	const FSlateBrush* Pressed; 

	TSharedPtr< IDocumentationPage > DocumentationPage;
};
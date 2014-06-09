// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 
#pragma once

class SLATE_API IErrorReportingWidget
{
public:
	virtual void SetError( const FString& InErrorText ) = 0;
	virtual void SetError( const FText& InErrorText ) = 0;
	virtual bool HasError() const = 0;
	virtual TSharedRef<SWidget> AsWidget() = 0;
};

class SLATE_API SErrorText : public SBorder, public IErrorReportingWidget
{
public:
	SLATE_BEGIN_ARGS( SErrorText )
		: _ErrorText()
		, _BackgroundColor(FCoreStyle::Get().GetColor("ErrorReporting.BackgroundColor"))
		{}

		SLATE_TEXT_ARGUMENT(ErrorText)
		SLATE_ATTRIBUTE(FSlateColor, BackgroundColor)
		
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// IErrorReportingWidget interface

	virtual void SetError( const FText& InErrorText ) override;
	virtual void SetError( const FString& InErrorText ) override;

	virtual bool HasError() const override;

	virtual TSharedRef<SWidget> AsWidget() override;

	// IErrorReportingWidget interface

private:
	TAttribute<EVisibility> CustomVisibility;
	EVisibility MyVisibility() const;

	TSharedPtr<class STextBlock> TextBlock;

	FVector2D GetDesiredSizeScale() const;
	FCurveSequence ExpandAnimation;
};



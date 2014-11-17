// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 
#pragma once

class SLATE_API SErrorHint : public SCompoundWidget, public IErrorReportingWidget
{
public:
	SLATE_BEGIN_ARGS( SErrorHint )
		: _ErrorText()
		{}
		SLATE_TEXT_ARGUMENT(ErrorText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// IErrorReportingWidget interface

	virtual void SetError( const FText& InErrorText ) OVERRIDE;
	virtual void SetError( const FString& InErrorText ) OVERRIDE;

	virtual bool HasError() const OVERRIDE;

	virtual TSharedRef<SWidget> AsWidget() OVERRIDE;

	// IErrorReportingWidget interface

private:
	TAttribute<EVisibility> CustomVisibility;
	EVisibility MyVisibility() const;

	FVector2D GetDesiredSizeScale() const;
	FCurveSequence ExpandAnimation;

	TSharedPtr<SWidget> ImageWidget;
	FText ErrorText;
	FText GetErrorText() const;
};



// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class GRAPHEDITOR_API SGraphNodeK2Event : public SGraphNodeK2Default
{
public:
	SGraphNodeK2Event() : SGraphNodeK2Default(), bHasDelegateOutputPin(false) {}

protected:
	virtual TSharedRef<SWidget> CreateTitleWidget(TSharedPtr<SNodeTitle> NodeTitle) OVERRIDE;
	virtual bool UseLowDetailNodeTitles() const OVERRIDE;
	virtual void AddPin( const TSharedRef<SGraphPin>& PinToAdd ) OVERRIDE;


	virtual void SetDefaultTitleAreaWidget(TSharedRef<SOverlay> DefaultTitleAreaWidget) OVERRIDE
	{
		TitleAreaWidget = DefaultTitleAreaWidget;
	}

private:
	bool ParentUseLowDetailNodeTitles() const
	{
		return SGraphNodeK2Default::UseLowDetailNodeTitles();
	}

	EVisibility GetTitleVisibility() const;

	TSharedPtr<SOverlay> TitleAreaWidget;
	bool bHasDelegateOutputPin;
};

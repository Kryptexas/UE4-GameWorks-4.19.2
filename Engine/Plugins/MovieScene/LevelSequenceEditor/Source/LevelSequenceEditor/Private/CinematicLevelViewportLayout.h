// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LevelViewportLayout.h"


/** A viewport layout that comprises a normal editor viewport, and a cinematic preview viewport */
class FCinematicLevelViewportLayout_OnePane : public FLevelViewportLayout
{
public:

	virtual void SaveLayoutString(const FString& LayoutString) const override;
	virtual const FName& GetLayoutTypeName() const override;

protected:

	virtual TSharedRef<SWidget> MakeViewportLayout(const FString& LayoutString) override;
	virtual void ReplaceWidget( TSharedRef< SWidget > Source, TSharedRef< SWidget > Replacement ) override;


private:
	/** The widget */
	TSharedPtr<SBox> WidgetSlot;
};


/** A viewport layout that comprises a normal editor viewport, and a cinematic preview viewport */
class FCinematicLevelViewportLayout_TwoPane : public FLevelViewportLayout
{
public:

	virtual void SaveLayoutString(const FString& LayoutString) const override;
	virtual const FName& GetLayoutTypeName() const override;

protected:

	virtual TSharedRef<SWidget> MakeViewportLayout(const FString& LayoutString) override;
	virtual void ReplaceWidget( TSharedRef< SWidget > Source, TSharedRef< SWidget > Replacement ) override;


private:
	/** The splitter widget */
	TSharedPtr<class SSplitter> SplitterWidget;
};

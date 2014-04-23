// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __LevelViewportLayoutOnePane_h__
#define __LevelViewportLayoutOnePane_h__

#pragma once

#include "LevelViewportLayout.h"

class FLevelViewportLayoutOnePane : public FLevelViewportLayout
{
public:
	/**
	* Saves viewport layout information between editor sessions
	*/
	virtual void SaveLayoutString(const FString& LayoutString) const OVERRIDE;

	virtual const FName& GetLayoutTypeName() const OVERRIDE{ return LevelViewportConfigurationNames::OnePane; }
protected:
	/**
	* Creates the viewport for the single pane
	*/
	virtual TSharedRef<SWidget> MakeViewportLayout(const FString& LayoutString) OVERRIDE;

	/** Overridden from FLevelViewportLayout */
	virtual void ReplaceWidget(TSharedRef< SWidget > Source, TSharedRef< SWidget > Replacement) OVERRIDE;

protected:
	/** The viewport widget parent box */
	TSharedPtr< SHorizontalBox > ViewportBox;
};

#endif
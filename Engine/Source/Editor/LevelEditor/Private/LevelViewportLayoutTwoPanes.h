// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __LevelViewportLayoutTwoPanes_h__
#define __LevelViewportLayoutTwoPanes_h__

#pragma once

#include "LevelViewportLayout.h"

template <EOrientation TOrientation>
class TLevelViewportLayoutTwoPanes : public FLevelViewportLayout
{
public:
	/**
	 * Saves viewport layout information between editor sessions
	 */
	virtual void SaveLayoutString(const FString& LayoutString) const OVERRIDE;
protected:
	/**
	 * Creates the viewports and splitter for the two panes vertical layout                   
	 */
	virtual TSharedRef<SWidget> MakeViewportLayout(const FString& LayoutString) OVERRIDE;

	/** Overridden from FLevelViewportLayout */
	virtual void ReplaceWidget( TSharedRef< SWidget > Source, TSharedRef< SWidget > Replacement ) OVERRIDE;


private:
	/** The splitter widget */
	TSharedPtr< class SSplitter > SplitterWidget;
};


// FLevelViewportLayoutTwoPanesVert /////////////////////////////

class FLevelViewportLayoutTwoPanesVert : public TLevelViewportLayoutTwoPanes<EOrientation::Orient_Vertical>
{
public:
	virtual const FName& GetLayoutTypeName() const OVERRIDE { return LevelViewportConfigurationNames::TwoPanesVert; }
};


// FLevelViewportLayoutTwoPanesHoriz /////////////////////////////

class FLevelViewportLayoutTwoPanesHoriz : public TLevelViewportLayoutTwoPanes<EOrientation::Orient_Horizontal>
{
public:
	virtual const FName& GetLayoutTypeName() const OVERRIDE { return LevelViewportConfigurationNames::TwoPanesHoriz; }
};

#endif

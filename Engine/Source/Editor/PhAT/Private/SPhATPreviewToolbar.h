// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/*-----------------------------------------------------------------------------
   SPhATPreviewViewportToolBar
-----------------------------------------------------------------------------*/

class SPhATPreviewViewportToolBar : public SViewportToolBar
{
public:
	SLATE_BEGIN_ARGS(SPhATPreviewViewportToolBar){}
		SLATE_ARGUMENT(TWeakPtr<FPhAT>, PhATPtr)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/** Generates the toolbar view menu content */
	TSharedRef<SWidget> GenerateViewMenu() const;

	/** Generates the toolbar modes menu content */
	TSharedRef<SWidget> GenerateModesMenu() const;

private:
	/** The viewport that we are in */
	TWeakPtr<FPhAT> PhATPtr;
};

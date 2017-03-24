// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetEditorToolkit.h"

class CLOTHPAINTER_API SClothPaintTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SClothPaintTab)
	{}	
	SLATE_ARGUMENT(TWeakPtr<class FAssetEditorToolkit>, InHostingApp)
	SLATE_END_ARGS()

	SClothPaintTab();
	~SClothPaintTab();

	/** SWidget functions */
	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

protected:
	void UpdatePaintTools();
protected:
	TWeakPtr<class FAssetEditorToolkit> HostingApp;
	
	TSharedPtr<SWidget> ModeWidget;
	TSharedPtr<SVerticalBox> ContentBox;

	bool bModeApplied;
	bool bPaintModeEnabled;
};
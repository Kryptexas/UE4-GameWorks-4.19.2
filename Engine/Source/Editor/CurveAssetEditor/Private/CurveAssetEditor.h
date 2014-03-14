// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/AssetEditorManager.h"

class FCurveAssetEditor :  public ICurveAssetEditor
{
public:

	FCurveAssetEditor()
		: ViewMinInput( 0.f )
		, ViewMaxInput( 0.f )
		, TrackWidget( NULL )
	{}

	virtual ~FCurveAssetEditor() {}

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;

	/**
	 * Edits the specified table
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	CurveToEdit				The curve to edit
	 */
	void InitCurveAssetEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UCurveBase* CurveToEdit );

	/** IToolkit interface */
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;

private:

	float GetViewMinInput() const { return ViewMinInput; }
	float GetViewMaxInput() const { return ViewMaxInput; }
	/** Sets InViewMinInput and InViewMaxInput */
	void SetInputViewRange(float InViewMinInput, float InViewMaxInput);
	/** Return length of timeline */
	float GetTimelineLength() const;
	/**	Spawns the tab with the curve asset inside */
	TSharedRef<SDockTab> SpawnTab_CurveAsset( const FSpawnTabArgs& Args );

	TSharedPtr<class SCurveEditor> TrackWidget;

	/**	The tab id for the curve asset tab */
	static const FName CurveTabId;

	float ViewMinInput;
	float ViewMaxInput;
};

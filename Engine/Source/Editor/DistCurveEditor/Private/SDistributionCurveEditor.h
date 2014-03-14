// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SCurveEditorViewport;
class FCurveEditorSharedData;

DECLARE_LOG_CATEGORY_EXTERN(LogCurveEd, Log, All);


/*-----------------------------------------------------------------------------
   SDistributionCurveEditor
-----------------------------------------------------------------------------*/

class SDistributionCurveEditor : public IDistributionCurveEditor
{
public:
	SLATE_BEGIN_ARGS(SDistributionCurveEditor) 
		: _EdSetup(NULL)
		, _NotifyObject(NULL)
		, _CurveEdOptions(FCurveEdOptions())
		{}

		SLATE_ARGUMENT(UInterpCurveEdSetup*, EdSetup)
		SLATE_ARGUMENT(FCurveEdNotifyInterface*, NotifyObject)
		SLATE_ARGUMENT(FCurveEdOptions, CurveEdOptions)
	SLATE_END_ARGS()

	/** Constructor/Destructor */
	SDistributionCurveEditor();
	virtual ~SDistributionCurveEditor();

	/** SCompoundWidget functions */
	void Construct(const FArguments& InArgs);

	/** IDistributionCurveEditor interface */
	virtual void RefreshViewport() OVERRIDE;
	virtual void CurveChanged() OVERRIDE;
	virtual void SetCurveVisible(const UObject* InCurve, bool bShow) OVERRIDE;
	virtual void ClearAllVisibleCurves() OVERRIDE;
	virtual void SetCurveSelected(const UObject* InCurve, bool bSelected) OVERRIDE;
	virtual void ClearAllSelectedCurves() OVERRIDE;
	virtual void ScrollToFirstSelected() OVERRIDE;
	virtual void SetActiveTabToFirstSelected() OVERRIDE;
	virtual UInterpCurveEdSetup* GetEdSetup() OVERRIDE;
	virtual float GetStartIn() OVERRIDE;
	virtual float GetEndIn() OVERRIDE;
	virtual void SetPositionMarker(bool bEnabled, float InPosition, const FColor& InMarkerColor) OVERRIDE;
	virtual void SetEndMarker(bool bEnabled, float InEndPosition) OVERRIDE;
	virtual void SetRegionMarker(bool bEnabled, float InRegionStart, float InRegionEnd, const FColor& InRegionFillColor) OVERRIDE;
	virtual void SetInSnap(bool bEnabled, float SnapAmount, bool bInSnapToFrames) OVERRIDE;
	virtual void SetViewInterval(float StartIn, float EndIn) OVERRIDE;

	/** Accessors */
	TSharedPtr<FCurveEditorSharedData> GetSharedData();

	/** Toolbar/menu command methods */
	void OnDeleteKeys();
	void OnFitToAll();
	void OnFitToSelected();
	void OnFitHorizontally();
	void OnFitVertically();
	void OnSetTangentType(int32 NewType);

	/** Methods for opening context menus */
	void OpenLabelMenu();
	void OpenKeyMenu();
	void OpenGeneralMenu();

	void CloseEntryPopup();
private:
	/** Creates the geometry mode controls */
	void CreateLayout(FCurveEdOptions CurveEdOptions);

	/** Query whether or not we're in small icon mode */
	EVisibility GetLargeIconVisibility() const;

	/** Builds the toolbar widget for the ParticleSystem editor */
	TSharedRef<SHorizontalBox> BuildToolBar();

	/**	Binds our UI commands to delegates */
	void BindCommands();

	/** Toolbar/menu command methods */
	void OnRemoveCurve();
	void OnRemoveAllCurves();
	void OnSetTime();
	void OnSetValue();
	void OnSetColor();
	void OnScaleTimes();
	void OnScaleValues();
	void OnSetMode(int32 NewMode);
	bool IsModeChecked(int32 Mode) const;
	bool IsTangentTypeChecked(int32 Type) const;
	void OnFlattenTangents();
	void OnStraightenTangents();
	void OnShowAllTangents();
	bool IsShowAllTangentsChecked() const;
	void OnCreateTab();
	void OnDeleteTab();

	/** Methods for building context menus */
	TSharedRef<SWidget> BuildMenuWidgetLabel();
	TSharedRef<SWidget> BuildMenuWidgetKey();
	TSharedRef<SWidget> BuildMenuWidgetGeneral();

	/** Methods related to the tab combobox */
	void TabSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void SetTabSelection(TSharedPtr<FString> NewSelection, bool bUpdateWidget);

	/** On commit callbacks for various user input dialogs */
	void KeyTimeCommitted(const FText& CommentText, ETextCommit::Type CommitInfo);
	void KeyValueCommitted(const FText& CommentText, ETextCommit::Type CommitInfo);
	void ScaleTimeCommitted(const FText& CommentText, ETextCommit::Type CommitInfo);
	void ScaleValueCommitted(const FText& CommentText, ETextCommit::Type CommitInfo);
	void TabNameCommitted(const FText& CommentText, ETextCommit::Type CommitInfo);

	/** Helper function to handle undo/redo */
	bool NotifyPendingCurveChange(bool bSelectedOnly);

	/** Fits the curve editor view horizontally to the curve data */
	void FitViewHorizontally();

	/** Fits the curve editor view vertically to the curve data */
	void FitViewVertically();

	/** Straightens or flattens all curve tangents */
	void ModifyTangents(bool bDoStraighten);

	/** Helper method to set selected tab */
	TSharedPtr<FString> GetSelectedTab() const;

private:
	/** A list commands to execute if a user presses the corresponding keybinding in the text box */
	TSharedRef<FUICommandList> UICommandList;

	/** Viewport */
	TSharedPtr<SCurveEditorViewport> Viewport;

	/** Toolbar */
	TSharedPtr<SHorizontalBox> Toolbar;

	/** Data and methods shared across multiple classes */
	TSharedPtr<FCurveEditorSharedData> SharedData;

	/** Reference to owner of the current popup */
	TWeakPtr<SWindow> EntryPopupWindow;

	/** Tabs dropdown */
	TSharedPtr<STextComboBox> TabNamesComboBox;

	/** Names of the curve tabs */
	TArray< TSharedPtr<FString> > TabNames;

	/** Buffer amount used when fitting the viewport to the curve */
	const float FitMargin;

	/** Selected Tab to use */
	TSharedPtr<FString> SelectedTab;
};

// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"

#include "MergeProxyUtils/Utils.h"

class FMeshMergingTool;
class IDetailsView;
class UMeshMergingSettingsObject;

/*-----------------------------------------------------------------------------
   SMeshMergingDialog
-----------------------------------------------------------------------------*/
class SMeshMergingDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMeshMergingDialog)
	{
	}

	SLATE_END_ARGS()

public:
	/** **/
	SMeshMergingDialog();
	~SMeshMergingDialog();

	/** SWidget functions */
	void Construct(const FArguments& InArgs, FMeshMergingTool* InTool);	

	/** Getter functionality */
	const TArray<TSharedPtr<FMergeComponentData>>& GetSelectedComponents() const { return ComponentSelectionControl.SelectedComponents; }
	/** Get number of selected meshes */
	const int32 GetNumSelectedMeshComponents() const { return ComponentSelectionControl.NumSelectedMeshComponents; }

	/** Resets the state of the UI and flags it for refreshing */
	void Reset();
private:	
	/** Begin override SCompoundWidget */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	/** End override SCompoundWidget */

	/** Creates and sets up the settings view element*/
	void CreateSettingsView();	

	/** Delegate for the creation of the list view item's widget */
	TSharedRef<ITableRow> MakeComponentListItemWidget(TSharedPtr<FMergeComponentData> ComponentData, const TSharedRef<STableViewBase>& OwnerTable);

	/** Delegate to determine whether or not the UI elements should be enabled (determined by number of selected actors / mesh components) */
	bool GetContentEnabledState() const;
		
	/** Delegates for replace actors checkbox UI element*/
	ECheckBoxState GetReplaceSourceActors() const;
	void SetReplaceSourceActors(ECheckBoxState NewValue);
	
	/** Editor delgates for map and selection changes */
	void OnLevelSelectionChanged(UObject* Obj);
	void OnMapChange(uint32 MapFlags);
	void OnNewCurrentLevel();

	/** Updates SelectedMeshComponent array according to retrieved mesh components from editor selection*/
	void UpdateSelectedStaticMeshComponents();	
	/** Stores the individual check box states for the currently selected mesh components */
	void StoreCheckBoxState();
private:
	/** Owning mesh merging tool */
	FMeshMergingTool* Tool;



	FComponentSelectionControl ComponentSelectionControl;

	/** Settings view ui element ptr */
	TSharedPtr<IDetailsView> SettingsView;
	/** Cached pointer to mesh merging setting singleton object */
	UMeshMergingSettingsObject* MergeSettings;
	/** List view state tracking data */
	bool bRefreshListView;

};

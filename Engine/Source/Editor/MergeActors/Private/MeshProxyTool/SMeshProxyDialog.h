// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableViewBase.h"

#include "MergeProxyUtils/Utils.h"

class FMeshProxyTool;
class IDetailsView;
class UMeshProxySettingsObject;
class UObject;

/*-----------------------------------------------------------------------------
SMeshProxyDialog  
-----------------------------------------------------------------------------*/

class SMeshProxyDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMeshProxyDialog)
	{
	}
	SLATE_END_ARGS()

public:
	/** **/
	SMeshProxyDialog();
	~SMeshProxyDialog();

	/** SWidget functions */
	void Construct(const FArguments& InArgs, FMeshProxyTool* InTool);

	/** Resets the state of the UI and flags it for refreshing */
	void Reset();

	/** Getter functionality */
	const TArray<TSharedPtr<FMergeComponentData>>& GetSelectedComponents() const { return ComponentSelectionControl.SelectedComponents; }
	/** Get number of selected meshes */
	const int32 GetNumSelectedMeshComponents() const { return ComponentSelectionControl.NumSelectedMeshComponents; }

	/** Begin override SCompoundWidget */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	/** End override SCompoundWidget */

	/** Creates and sets up the settings view element*/
	void CreateSettingsView();

	/** Delegate for the creation of the list view item's widget */
	TSharedRef<ITableRow> MakeComponentListItemWidget(TSharedPtr<FMergeComponentData> ComponentData, const TSharedRef<STableViewBase>& OwnerTable);

	/** Delegate to determine whether or not the UI elements should be enabled (determined by number of selected actors / mesh components) */
	bool GetContentEnabledState() const;


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
	FMeshProxyTool* Tool;

	FComponentSelectionControl ComponentSelectionControl;

	/** Settings view ui element ptr */
	TSharedPtr<IDetailsView> SettingsView;

	/** Cached pointer to mesh merging setting singleton object */
	UMeshProxySettingsObject* ProxySettings;

	/** List view state tracking data */
	bool bRefreshListView;
};



class FThirdPartyMeshProxyTool;
/*-----------------------------------------------------------------------------
	SThirdPartyMeshProxyDialog  -- Used for Simplygon (third party tool) integration.
-----------------------------------------------------------------------------*/
class SThirdPartyMeshProxyDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SThirdPartyMeshProxyDialog)
	{
	}
	SLATE_END_ARGS()

public:
	/** SWidget functions */
	void Construct(const FArguments& InArgs, FThirdPartyMeshProxyTool* InTool);

protected:
	/** ScreenSize accessors */
	TOptional<int32> GetScreenSize() const;
	void ScreenSizeChanged(int32 NewValue);		//used with editable text block (Simplygon)

	/** Recalculate Normals accessors */
	ECheckBoxState GetRecalculateNormals() const;
	void SetRecalculateNormals(ECheckBoxState NewValue);

	/** Hard Angle Threshold accessors */
	TOptional<float> GetHardAngleThreshold() const;
	bool HardAngleThresholdEnabled() const;
	void HardAngleThresholdChanged(float NewValue);

	/** Hole filling accessors */
	TOptional<int32> GetMergeDistance() const;
	void MergeDistanceChanged(int32 NewValue);

	/** TextureResolution accessors */
	void SetTextureResolution(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void SetLightMapResolution(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	/** Export material properties acessors **/
	ECheckBoxState GetExportNormalMap() const;
	void SetExportNormalMap(ECheckBoxState NewValue);
	ECheckBoxState GetExportMetallicMap() const;
	void SetExportMetallicMap(ECheckBoxState NewValue);
	ECheckBoxState GetExportRoughnessMap() const;
	void SetExportRoughnessMap(ECheckBoxState NewValue);
	ECheckBoxState GetExportSpecularMap() const;
	void SetExportSpecularMap(ECheckBoxState NewValue);

private:
	/** Creates the geometry mode controls */
	void CreateLayout();

	int32 FindTextureResolutionEntryIndex(int32 InResolution) const;
	FText GetPropertyToolTipText(const FName& PropertyName) const;

private:
	FThirdPartyMeshProxyTool* Tool;

	TArray< TSharedPtr<FString> >	CuttingPlaneOptions;
	TArray< TSharedPtr<FString> >	TextureResolutionOptions;
};


// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetThumbnail.h"
#include "PropertyEditing.h"

enum ECreationModeChoice
{
	CreateNew,
	UseChannel0,
};

enum ELimitModeChoice
{
	Stretching,
	Charts
};

class FLevelOfDetailSettingsLayout;

class FStaticMeshDetails : public IDetailCustomization
{
public:
	FStaticMeshDetails( class FStaticMeshEditor& InStaticMeshEditor );
	~FStaticMeshDetails();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( class IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;

	/** @return true if settings have been changed and need to be applied to the static mesh */
	bool IsApplyNeeded() const;

	/** Applies level of detail changes to the static mesh */
	void ApplyChanges();
private:
	/** Level of detail settings for the details panel */
	TSharedPtr<FLevelOfDetailSettingsLayout> LevelOfDetailSettings;
	/** Static mesh editor */
	class FStaticMeshEditor& StaticMeshEditor;
};


/** 
 * Window handles convex decomposition, settings and controls.
 */
class SConvexDecomposition : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SConvexDecomposition ) :
		_StaticMeshEditorPtr()
		{
		}
		/** The Static Mesh Editor this tool is associated with. */
		SLATE_ARGUMENT( TWeakPtr< IStaticMeshEditor >, StaticMeshEditorPtr )

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual ~SConvexDecomposition();

private:
	/** Callback when the "Apply" button is clicked. */
	FReply OnApplyDecomp();

	/** Callback when the "Defaults" button is clicked. */
	FReply OnDefaults();

	/** Assigns the max number of hulls based on the spinbox's value. */
	void OnHullCountCommitted(int32 InNewValue, ETextCommit::Type CommitInfo);

	/** Assigns the max number of hulls based on the spinbox's value. */
	void OnHullCountChanged(int32 InNewValue);


	/** 
	 *	Retrieves the max number of hulls allowed.
	 *
	 *	@return			The max number of hulls selected.
	 */
	int32 GetHullCount() const;

	/** Assigns the max number of hulls based on the spinbox's value. */
	void OnVertsPerHullCountCommitted(int32 InNewValue, ETextCommit::Type CommitInfo);

	/** Assigns the max number of hulls based on the spinbox's value. */
	void OnVertsPerHullCountChanged(int32 InNewValue);

	/** 
	 *	Retrieves the max number of verts per hull allowed.
	 *
	 *	@return			The max number of verts per hull selected.
	 */
	int32 GetVertsPerHullCount() const;

private:
	/** The Static Mesh Editor this tool is associated with. */
	TWeakPtr<IStaticMeshEditor> StaticMeshEditorPtr;

	/** Spinbox for the max number of hulls allowed. */
	TSharedPtr< SSpinBox<int32> > MaxHull;

	/** The current number of max number of hulls selected. */
	int32 CurrentMaxHullCount;

	/** Spinbox for the max number of verts per hulls allowed. */
	TSharedPtr< SSpinBox<int32> > MaxVertsPerHull;

	/** The current number of max verts per hull selected. */
	int32 CurrentMaxVertsPerHullCount;

};


class FMeshBuildSettingsLayout : public IDetailCustomNodeBuilder, public TSharedFromThis<FMeshBuildSettingsLayout>
{
public:
	FMeshBuildSettingsLayout( TSharedRef<FLevelOfDetailSettingsLayout> InParentLODSettings );
	virtual ~FMeshBuildSettingsLayout();

	const FMeshBuildSettings& GetSettings() const { return BuildSettings; }
	void UpdateSettings(const FMeshBuildSettings& InSettings);

private:
	/** IDetailCustomNodeBuilder Interface*/
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRegenerateChildren ) OVERRIDE {}
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) OVERRIDE;
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) OVERRIDE;
	virtual void Tick( float DeltaTime ) OVERRIDE{}
	virtual bool RequiresTick() const OVERRIDE { return false; }
	virtual FName GetName() const OVERRIDE { static FName MeshBuildSettings("MeshBuildSettings"); return MeshBuildSettings; }
	virtual bool InitiallyCollapsed() const OVERRIDE { return true; }

	FReply OnApplyChanges();
	ESlateCheckBoxState::Type ShouldRecomputeNormals() const;
	ESlateCheckBoxState::Type ShouldRecomputeTangents() const;
	ESlateCheckBoxState::Type ShouldRemoveDegenerates() const;
	ESlateCheckBoxState::Type ShouldUseFullPrecisionUVs() const;
	TOptional<float> GetBuildScale() const;

	void OnRecomputeNormalsChanged(ESlateCheckBoxState::Type NewState);
	void OnRecomputeTangentsChanged(ESlateCheckBoxState::Type NewState);
	void OnRemoveDegeneratesChanged(ESlateCheckBoxState::Type NewState);
	void OnUseFullPrecisionUVsChanged(ESlateCheckBoxState::Type NewState);
	void OnBuildScaleChanged( float NewScale );

private:
	TWeakPtr<FLevelOfDetailSettingsLayout> ParentLODSettings;
	FMeshBuildSettings BuildSettings;
};

class FMeshReductionSettingsLayout : public IDetailCustomNodeBuilder, public TSharedFromThis<FMeshReductionSettingsLayout>
{
public:
	FMeshReductionSettingsLayout( TSharedRef<FLevelOfDetailSettingsLayout> InParentLODSettings );
	virtual ~FMeshReductionSettingsLayout();

	const FMeshReductionSettings& GetSettings() const;
	void UpdateSettings(const FMeshReductionSettings& InSettings);
private:
	/** IDetailCustomNodeBuilder Interface*/
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRegenerateChildren ) OVERRIDE {}
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) OVERRIDE;
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) OVERRIDE;
	virtual void Tick( float DeltaTime ) OVERRIDE{}
	virtual bool RequiresTick() const OVERRIDE { return false; }
	virtual FName GetName() const OVERRIDE { static FName MeshReductionSettings("MeshReductionSettings"); return MeshReductionSettings; }
	virtual bool InitiallyCollapsed() const OVERRIDE { return true; }

	FReply OnApplyChanges();
	float GetPercentTriangles() const;
	float GetMaxDeviation() const;
	float GetWeldingThreshold() const;
	ESlateCheckBoxState::Type ShouldRecalculateNormals() const;
	float GetHardAngleThreshold() const;

	void OnPercentTrianglesChanged(float NewValue);
	void OnMaxDeviationChanged(float NewValue);
	void OnReductionAmountChanged(float NewValue);
	void OnRecalculateNormalsChanged(ESlateCheckBoxState::Type NewValue);
	void OnWeldingThresholdChanged(float NewValue);
	
	void OnHardAngleThresholdChanged(float NewValue);

	void OnSilhouetteImportanceChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	void OnTextureImportanceChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	void OnShadingImportanceChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);

private:
	TWeakPtr<FLevelOfDetailSettingsLayout> ParentLODSettings;
	FMeshReductionSettings ReductionSettings;
	TArray<TSharedPtr<FString> > ImportanceOptions;
	TSharedPtr<STextComboBox> SilhouetteCombo;
	TSharedPtr<STextComboBox> TextureCombo;
	TSharedPtr<STextComboBox> ShadingCombo;
};

class FMeshSectionSettingsLayout : public TSharedFromThis<FMeshSectionSettingsLayout>
{
public:
	FMeshSectionSettingsLayout( IStaticMeshEditor& InStaticMeshEditor, int32 InLODIndex )
		: StaticMeshEditor( InStaticMeshEditor )
		, LODIndex( InLODIndex )
	{}

	virtual ~FMeshSectionSettingsLayout();

	void AddToCategory( IDetailCategoryBuilder& CategoryBuilder );

private:
	UStaticMesh& GetStaticMesh() const;
	void GetMaterials(class IMaterialListBuilder& ListBuilder);
	void OnMaterialChanged(UMaterialInterface* NewMaterial, UMaterialInterface* PrevMaterial, int32 SlotIndex, bool bReplaceAll);
	TSharedRef<SWidget> OnGenerateNameWidgetsForMaterial(UMaterialInterface* Material, int32 SlotIndex);
	TSharedRef<SWidget> OnGenerateWidgetsForMaterial(UMaterialInterface* Material, int32 SlotIndex);
	void OnResetMaterialToDefaultClicked(UMaterialInterface* Material, int32 SlotIndex);
	ESlateCheckBoxState::Type DoesSectionCastShadow(int32 SectionIndex) const;
	void OnSectionCastShadowChanged(ESlateCheckBoxState::Type NewState, int32 SectionIndex);
	ESlateCheckBoxState::Type DoesSectionCollide(int32 SectionIndex) const;
	void OnSectionCollisionChanged(ESlateCheckBoxState::Type NewState, int32 SectionIndex);
	ESlateCheckBoxState::Type IsSectionSelected(int32 SectionIndex) const;
	void OnSectionSelectedChanged(ESlateCheckBoxState::Type NewState, int32 SectionIndex);
	void CallPostEditChange();
	
	IStaticMeshEditor& StaticMeshEditor;
	int32 LODIndex;
};

/** 
 * Window for adding and removing LOD.
 */
class FLevelOfDetailSettingsLayout : public TSharedFromThis<FLevelOfDetailSettingsLayout>
{
public:
	FLevelOfDetailSettingsLayout( FStaticMeshEditor& StaticMeshEditor );
	virtual ~FLevelOfDetailSettingsLayout();
	
	void AddToDetailsPanel( IDetailLayoutBuilder& DetailBuilder );

	/** Returns true if settings have been changed and an Apply is needed to update the asset. */
	bool IsApplyNeeded() const;

	/** Apply current LOD settings to the mesh. */
	void ApplyChanges();

private:

	/** Creates the UI for Current LOD panel */
	void AddLODLevelCategories( class IDetailLayoutBuilder& DetailBuilder );

	/** Callbacks. */
	FReply OnApply();
	void OnLODCountChanged(int32 NewValue);
	void OnLODCountCommitted(int32 InValue, ETextCommit::Type CommitInfo);
	int32 GetLODCount() const;
	float GetLODDistance(int32 LODIndex) const;
	FText GetLODDistanceTitle(int32 LODIndex) const;
	bool CanChangeLODDistance() const;
	void OnLODDistanceChanged(float NewValue, int32 LODIndex);
	void OnLODDistanceCommitted(float NewValue, ETextCommit::Type CommitType, int32 LODIndex);
	void OnBuildSettingsExpanded(bool bIsExpanded, int32 LODIndex);
	void OnReductionSettingsExpanded(bool bIsExpanded, int32 LODIndex);
	void OnSectionSettingsExpanded(bool bIsExpanded, int32 LODIndex);
	void OnLODGroupChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	bool IsAutoLODEnabled() const;
	ESlateCheckBoxState::Type IsAutoLODChecked() const;
	void OnAutoLODChanged(ESlateCheckBoxState::Type NewState);
	float GetPixelError() const;
	void OnPixelErrorChanged(float NewValue);
	void OnImportLOD(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	void UpdateLODNames();
	FText GetLODCountTooltip() const;

private:

	/** The Static Mesh Editor this tool is associated with. */
	FStaticMeshEditor& StaticMeshEditor;

	/** Pool for material thumbnails. */
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;

	/** LOD group options. */
	TArray<FName> LODGroupNames;
	TArray<TSharedPtr<FString> > LODGroupOptions;

	/** LOD import options */
	TArray<TSharedPtr<FString> > LODNames;

	/** Simplification options for each LOD level (in the LOD Chain tool). */
	TSharedPtr<FMeshReductionSettingsLayout> ReductionSettingsWidgets[MAX_STATIC_MESH_LODS];
	TSharedPtr<FMeshBuildSettingsLayout> BuildSettingsWidgets[MAX_STATIC_MESH_LODS];
	TSharedPtr<FMeshSectionSettingsLayout> SectionSettingsWidgets[MAX_STATIC_MESH_LODS];

	/** The distances at which each LOD swaps in. */
	float LODDistances[MAX_STATIC_MESH_LODS];

	/** Helper value that corresponds to the 'Number of LODs' spinbox.*/
	int32 LODCount;

	/** Whether certain parts of the UI are expanded so changes persist across
	    recreating the UI. */
	bool bBuildSettingsExpanded[MAX_STATIC_MESH_LODS];
	bool bReductionSettingsExpanded[MAX_STATIC_MESH_LODS];
	bool bSectionSettingsExpanded[MAX_STATIC_MESH_LODS];
};

/** 
 * Window for generating unique UVs for the static mesh.
 */
class SGenerateUniqueUVs : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SGenerateUniqueUVs ) :
	  _StaticMeshEditorPtr()
	  {
	  }
	  /** The Static Mesh Editor this tool is associated with. */
	  SLATE_ARGUMENT( TWeakPtr< IStaticMeshEditor >, StaticMeshEditorPtr )

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual ~SGenerateUniqueUVs();

	/** Refreshes the tool's LOD and UV channel options. */
	void RefreshTool();

private:
	/** Decides if the radio button should be checked. */
	ESlateCheckBoxState::Type IsCreationModeChecked( enum ECreationModeChoice ButtonId ) const;

	/** Handles the Creation mode being changed and manages the radio buttons. */
	void OnCreationModeChanged( ESlateCheckBoxState::Type NewRadioState, enum ECreationModeChoice RadioThatChanged );

	/** Decides if the radio button should be checked. */
	ESlateCheckBoxState::Type IsLimitModeChecked( enum ELimitModeChoice ButtonId ) const;

	/** Handles the Limit mode being changed and manages the radio buttons. */
	void OnLimitModeChanged( ESlateCheckBoxState::Type NewRadioState, enum ELimitModeChoice RadioThatChanged );

	/** Checks the state of the Creation Mode, TRUE if Create New is selected. */
	bool IsCreateNew() const;

	/** Checks the state of the limit mode, TRUE if charts is selected. */
	bool IsChartLimit() const;

	/** Checks the state of the limit mode, TRUE if stretching is selected. */
	bool IsStretchingLimit() const;

	/** Callback for the Apply button. */
	FReply OnApply();

	/** Callback for spinbox to set the MaxStretching variable. */
	void OnMaxStretchingChanged(float InValue);

	/** Callback for spinbox to set the MaxStretching variable. */
	void OnMaxStretchingCommitted(float InValue, ETextCommit::Type CommitInfo);

	/** Callback for spinbox to set the MaxCharts variable. Reduces the value to an int32 */
	void OnMaxChartsChanged(float InValue);

	/** Callback for spinbox to set the MaxCharts variable. Reduces the value to an int32 */
	void OnMaxChartsCommitted(float InValue, ETextCommit::Type CommitInfo);
	
	/** Retrieves the current value for MaxCharts for a spinbox, important because the value is limited to an int32. */
	float GetMaxCharts() const;

	/** Callback for spinbox to set the MinSpacing variable. */
	void OnMinSpacingChanged(float InValue);

	/** Callback for spinbox to set the MinSpacing variable. */
	void OnMinSpacingCommitted(float InValue, ETextCommit::Type CommitInfo);

	/** Refreshes the UV Channel list. */
	void RefreshUVChannelList();

private:
	/** The Static Mesh Editor this tool is associated with. */
	TWeakPtr< IStaticMeshEditor > StaticMeshEditorPtr;

	/** Drop down menu for selecting the UV Channel to save to. */
	TSharedPtr<class STextComboBox> UVChannelCombo;

	/** Options list for the UV Channel options. */
	TArray< TSharedPtr< FString > > UVChannels;

	/** The current Creation mode selected by the radio buttons. */
	ECreationModeChoice CurrentCreationModeChoice;

	/** The current Limit mode selected by the radio buttons. */
	ELimitModeChoice CurrentLimitModeChoice;

	/** Modified by a spinbox, user setting for max stretching. */
	float MaxStretching;

	/** Modified by a spinbox, user setting for max charts, clamped to be an int32. */
	float MaxCharts;

	/** Modified by a spinbox, user setting for the min spacing between charts. */
	float MinSpacingBetweenCharts;
};
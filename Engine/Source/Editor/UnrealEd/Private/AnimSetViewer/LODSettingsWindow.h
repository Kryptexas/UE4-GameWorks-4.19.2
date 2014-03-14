// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** 
* FDlgSkeletalMeshLODSettings
* 
* Wrapper class for SDlgSkeletalMeshLODSettings. This class creates and launches the dialog then awaits the
* Result to return to the user. 
*/
class FDlgSkeletalMeshLODSettings
{
public:
	FDlgSkeletalMeshLODSettings( struct FSkeletalMeshUpdateContext& UpdateContext );

	/**  Shows the dialog box and waits for the user to respond. */
	void ShowModal();

private:

	/** Cached pointer to the modal window */
	TSharedPtr<SWindow> DialogWindow;

	/** Cached pointer to the LOD Chain widget */
	TSharedPtr<class SDlgSkeletalMeshLODSettings> DialogWidget;
};

/** 
 * Slate Widget for Simplygon simplification settings
 */
class SSkeletalSimplificationOptions : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSkeletalSimplificationOptions)
		: _DefaultQuality( 100 )
		{
		}
		/** Default value for the Quality Slider. */
		SLATE_ARGUMENT( float , DefaultQuality )
		/** Skeleton this Mesh belong to **/
		SLATE_ARGUMENT( USkeleton*, Skeleton )
		/** Desired current LOD. Starts from 0-(N-1) when 0 is based LOD */
		SLATE_ARGUMENT( int32 , DesiredLOD )
	SLATE_END_ARGS()	

	/**
	 * Constructs this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );

	virtual ~SSkeletalSimplificationOptions();

public:

	/** Returns the skeletal simplification settings for this LOD */
	FSkeletalMeshOptimizationSettings GetLODSettings() const;

private:

	/** Callback function whenever the quality spinbox is changed. */
	void OnQualityChanged(float InValue);

	/** Callback function whenever the quality spinbox value is committed. */
	void OnQualityCommitted(float InValue, ETextCommit::Type CommitInfo);

	/** Returns the quality level, used by the spinbox as a callback. */
	float GetQualityValue() const;

	/** Callback function whenever the welding threshold spinbox is changed. */
	void OnWeldingThresholdChanged(float InValue);

	/** Callback function whenever the welding threshold spinbox value is committed. */
	void OnWeldingThresholdCommitted(float InValue, ETextCommit::Type CommitInfo);

	/** Callback function whenever the hard edge angle threshold spinbox is changed. */
	void OnHardEdgeAngleThresholdChanged(float InValue);

	/** Callback function whenever the hard edge angle threshold spinbox value is committed. */
	void OnHardEdgeAngleThresholdCommitted(float InValue, ETextCommit::Type CommitInfo);

	/** Callback function whenever the 'number of bones' spinbox is changed. */
	void OnNumberOfBonesChanged(float InValue);

	/** Callback function whenever the 'number of bones' spinbox value is committed. */
	void OnNumberOfBonesCommitted( float InValue, ETextCommit::Type CommitInfo);

	/** Callback function whenever the 'max bones per vertex' spinbox is changed. */
	void OnMaxBonesPerVertexChanged(int32 InValue);

	/** Callback function whenever the 'max bones per vertex' spinbox value is committed. */
	void OnMaxBonesPerVertexCommitted( int32 InValue, ETextCommit::Type CommitInfo);

	/** Callback function whenever the simplification type option is changed. */
	void OnSimplificationTypeSelectionChanged(TSharedPtr<FString> InValue, ESelectInfo::Type SelectInfo);

	/** Returns percentage of bones, used by the 'number of bones' spinbox */
	float GetNumberOfBones() const;

	/** Determines the visibility of the max deviation spinbox. */
	EVisibility IsMaxDeviationVisible() const;

	/** Determines the visibility of the triangle visible spinbox. */
	EVisibility IsNumTrianglesVisible() const;

	/** Determines the active state of the hard edge angle threshold spinbox. */
	bool IsHardEdgeAndleThresholdEnabled() const;

	/** Called when '+' button is pressed for Bones To Remove. */
	FReply OnAddBonesToRemove();

	/** Handle the committed text */
	void OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo);

	/** All possible suggestions for the search text */
	TArray<FString> PossibleSuggestions;

	/** Get the search suggestions */
	TArray<FString> GetSearchSuggestions() const;

	/** Refresh Bones To Remove Widget **/
	void RefreshBonesToRemove();
	/** Remove from the Bones List when 'X' button is clicked */
	FReply OnRemoveFromBonesList(const FName Name);

private:

	/** Drop down menu for selecting the simplification type. */
	TSharedPtr< STextComboBox > SimplificationTypeCombo;

	/** Label for the Quality slider. */
	TSharedPtr< STextBlock > QualityLabel;

	/** Drop down menu for selecting the silhouette setting. */
	TSharedPtr< STextComboBox > SilhouetteCombo;
	
	/** Drop down menu for selecting the texture setting. */
	TSharedPtr< STextComboBox > TextureCombo;

	/** Drop down menu for selecting the normals setting. */
	TSharedPtr< STextComboBox > ShadingCombo;

	/** Drop down menu for selecting the animation setting. */
	TSharedPtr< STextComboBox > SkinningCombo;
	
	/** Options array for the quality settings. */
	TArray< TSharedPtr<FString> > QualityOptions;

	/** Options array for simplification types. */
	TArray< TSharedPtr<FString> > SimplificationTypeOptions;

	/** Widget for marking to recompute normals. */
	TSharedPtr< SCheckBox > RecomputeNormalsCheckbox;

	/** Bones To remove widgets - user selected bones **/
	TSharedPtr< SVerticalBox > BonesToRemoveListWidget;
	
	/** Current Skeleton **/
	USkeleton * Skeleton;
	/** This is the index to BoneReductionSettingsForLODs of Skeleton. 
	 * 	That array only contains [1-(N-1)] LOD structure, so when DesiredLOD is 1, the LODSettingIndex will be 0*/
	int32 LODSettingIndex;
	/** Current Bone Name that being typed **/
	FText CurrentBoneName;

	/** Helper value, modified when the quality spinbox is changed. */
	float Quality;

	/** Helper value, modified when the welding threshold spinbox is changed. */
	float WeldingThreshold;

	/** Helper value, modified when the hard edge angle threshold spinbox is changed. */
	float HardEdgeAngleThreshold;

	/** Helper value, modified when the bones per vertex spinbox is changed. */
	int32 MaxBonesPerVertex;

	/** Populate Possible Suggestions**/
	void PopulatePossibleSuggestions(const USkeleton * Skeleton);
	/** Callback for When Text is modified **/
	void OnTextChanged(const FText& CurrentText);
	/** Add Bones To Remove **/
	void AddBonesToRemove(const FString & NewBonesToRemove);
};
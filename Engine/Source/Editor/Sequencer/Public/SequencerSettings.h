// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SequencerSettings.generated.h"

UENUM()
enum ESequencerSpawnPosition
{
	/** Origin. */
	SSP_Origin UMETA(DisplayName="Origin"),

	/** Place in Front of Camera. */
	SSP_PlaceInFrontOfCamera UMETA(DisplayName="Place in Front of Camera"),
};

UENUM()
enum ESequencerZoomPosition
{
	/** Current Time. */
	SZP_CurrentTime UMETA(DisplayName="Current Time"),

	/** Mouse Position. */
	SZP_MousePosition UMETA(DisplayName="Mouse Position"),
};

/** Empty class used to house multiple named USequencerSettings */
UCLASS()
class USequencerSettingsContainer
	: public UObject
{
public:
	GENERATED_BODY()

	/** Get or create a settings object for the specified name */
	template<class T> 
	static T* GetOrCreate(const TCHAR* InName)
	{
		static const TCHAR* SettingsContainerName = TEXT("SequencerSettingsContainer");

		auto* Outer = FindObject<USequencerSettingsContainer>(GetTransientPackage(), SettingsContainerName);
		if (!Outer)
		{
			Outer = NewObject<USequencerSettingsContainer>(GetTransientPackage(), USequencerSettingsContainer::StaticClass(), SettingsContainerName);
			Outer->AddToRoot();
		}
	
		T* Inst = FindObject<T>( Outer, InName );
		if (!Inst)
		{
			Inst = NewObject<T>( Outer, T::StaticClass(), InName );
			Inst->LoadConfig();
		}

		return Inst;
	}
};


/** Serializable options for sequencer. */
UCLASS(config=EditorPerProjectUserSettings, PerObjectConfig)
class SEQUENCER_API USequencerSettings
	: public UObject
{
public:
	GENERATED_UCLASS_BODY()

	DECLARE_MULTICAST_DELEGATE( FOnShowCurveEditorChanged );
	DECLARE_MULTICAST_DELEGATE( FOnTimeSnapIntervalChanged );

	/** Gets the current auto-key mode. */
	EAutoKeyMode GetAutoKeyMode() const;
	/** Sets the current auto-key mode. */
	void SetAutoKeyMode(EAutoKeyMode AutoKeyMode);

	/** Gets whether or not key all is enabled. */
	bool GetKeyAllEnabled() const;
	/** Sets whether or not key all is enabled. */
	void SetKeyAllEnabled(bool InbKeyAllEnabled);

	/** Gets whether or not to key interp properties only. */
	bool GetKeyInterpPropertiesOnly() const;
	/** Sets whether or not to key interp properties only. */
	void SetKeyInterpPropertiesOnly(bool InbKeyInterpPropertiesOnly); 

	/** Gets default key interpolation. */
	EMovieSceneKeyInterpolation GetKeyInterpolation() const;
	/** Sets default key interpolation */
	void SetKeyInterpolation(EMovieSceneKeyInterpolation InKeyInterpolation);

	/** Get initial spawn position. */
	ESequencerSpawnPosition GetSpawnPosition() const;
	/** Set initial spawn position. */
	void SetSpawnPosition(ESequencerSpawnPosition InSpawnPosition);

	/** Get whether to create spawnable cameras. */
	bool GetCreateSpawnableCameras() const;
	/** Set whether to create spawnable cameras. */
	void SetCreateSpawnableCameras(bool bInCreateSpawnableCameras);

	/** Gets whether or not to show frame numbers. */
	bool GetShowFrameNumbers() const;
	/** Sets whether or not to show frame numbers. */
	void SetShowFrameNumbers(bool InbShowFrameNumbers);

	/** Gets whether or not to show the time range slider. */
	bool GetShowRangeSlider() const;
	/** Sets whether or not to show frame numbers. */
	void SetShowRangeSlider(bool InbShowRangeSlider);

	/** Gets whether or not snapping is enabled. */
	bool GetIsSnapEnabled() const;
	/** Sets whether or not snapping is enabled. */
	void SetIsSnapEnabled(bool InbIsSnapEnabled);

	/** Gets the time in seconds used for interval snapping. */
	float GetTimeSnapInterval() const;
	/** Sets the time in seconds used for interval snapping. */
	void SetTimeSnapInterval(float InTimeSnapInterval);

	/** Gets whether or not to snap key times to the interval. */
	bool GetSnapKeyTimesToInterval() const;
	/** Sets whether or not to snap keys to the interval. */
	void SetSnapKeyTimesToInterval(bool InbSnapKeyTimesToInterval);

	/** Gets whether or not to snap keys to other keys. */
	bool GetSnapKeyTimesToKeys() const;
	/** Sets whether or not to snap keys to other keys. */
	void SetSnapKeyTimesToKeys(bool InbSnapKeyTimesToKeys);

	/** Gets whether or not to snap sections to the interval. */
	bool GetSnapSectionTimesToInterval() const;
	/** Sets whether or not to snap sections to the interval. */
	void SetSnapSectionTimesToInterval(bool InbSnapSectionTimesToInterval);

	/** Gets whether or not to snap sections to other sections. */
	bool GetSnapSectionTimesToSections() const;
	/** sets whether or not to snap sections to other sections. */
	void SetSnapSectionTimesToSections( bool InbSnapSectionTimesToSections );

	/** Gets whether or not to snap the play time to keys while scrubbing. */
	bool GetSnapPlayTimeToKeys() const;
	/** Sets whether or not to snap the play time to keys while scrubbing. */
	void SetSnapPlayTimeToKeys(bool InbSnapPlayTimeToKeys);

	/** Gets whether or not to snap the play time to the interval while scrubbing. */
	bool GetSnapPlayTimeToInterval() const;
	/** Sets whether or not to snap the play time to the interval while scrubbing. */
	void SetSnapPlayTimeToInterval(bool InbSnapPlayTimeToInterval);

	/** Gets whether or not to snap the play time to the dragged key. */
	bool GetSnapPlayTimeToDraggedKey() const;
	/** Sets whether or not to snap the play time to the dragged key. */
	void SetSnapPlayTimeToDraggedKey(bool InbSnapPlayTimeToDraggedKey);

	/** Gets the snapping interval for curve values. */
	float GetCurveValueSnapInterval() const;
	/** Sets the snapping interval for curve values. */
	void SetCurveValueSnapInterval(float InCurveValueSnapInterval);

	/** Gets whether or not to snap curve values to the interval. */
	bool GetSnapCurveValueToInterval() const;
	/** Sets whether or not to snap curve values to the interval. */
	void SetSnapCurveValueToInterval(bool InbSnapCurveValueToInterval);

	/** Gets whether or not the label browser is visible. */
	bool GetLabelBrowserVisible() const;
	/** Sets whether or not the label browser is visible. */
	void SetLabelBrowserVisible(bool Visible);

	/** Get zoom in/out position (mouse position or current time). */
	ESequencerZoomPosition GetZoomPosition() const;
	/** Set zoom in/out position (mouse position or current time). */
	void SetZoomPosition(ESequencerZoomPosition InZoomPosition);

	/** Gets whether or not auto-scroll is enabled. */
	bool GetAutoScrollEnabled() const;
	/** Sets whether or not auto-scroll is enabled. */
	void SetAutoScrollEnabled(bool bInAutoScrollEnabled);

	/** Gets whether or not the curve editor should be shown. */
	bool GetShowCurveEditor() const;
	/** Sets whether or not the curve editor should be shown. */
	void SetShowCurveEditor(bool InbShowCurveEditor);

	/** Gets whether or not to show curve tool tips in the curve editor. */
	bool GetShowCurveEditorCurveToolTips() const;
	/** Sets whether or not to show curve tool tips in the curve editor. */
	void SetShowCurveEditorCurveToolTips(bool InbShowCurveEditorCurveToolTips);
	
	/** Gets whether or not to link the curve editor time range. */
	bool GetLinkCurveEditorTimeRange() const;
	/** Sets whether or not to link the curve editor time range. */
	void SetLinkCurveEditorTimeRange(bool InbLinkCurveEditorTimeRange);

	/** Gets the loop mode. */
	bool IsLooping() const;
	/** Sets the loop mode. */
	void SetLooping(bool bInLooping);

	/** @return true if the cursor should be kept within the playback range during playback in sequencer, false otherwise */
	bool ShouldKeepCursorInPlayRange() const;
	/** Set whether or not the cursor should be kept within the playback range during playback in sequencer */
	void SetKeepCursorInPlayRange(bool bInKeepCursorInPlayRange);

	/** @return true if the playback range should be synced to the section bounds, false otherwise */
	bool ShouldKeepPlayRangeInSectionBounds() const;
	/** Set whether or not the playback range should be synced to the section bounds */
	void SetKeepPlayRangeInSectionBounds(bool bInKeepPlayRangeInSectionBounds);

	/** Get the number of digits we should zero-pad to when showing frame numbers in sequencer */
	uint8 GetZeroPadFrames() const;
	/** Set the number of digits we should zero-pad to when showing frame numbers in sequencer */
	void SetZeroPadFrames(uint8 InZeroPadFrames);

	/** @return true if showing combined keyframes at the top node */
	bool GetShowCombinedKeyframes() const;
	/** Set whether to show combined keyframes at the top node */ 
	void SetShowCombinedKeyframes(bool bInShowCombinedKeyframes);

	/** @return true if key areas are infinite */
	bool GetInfiniteKeyAreas() const;
	/** Set whether to show channel colors */
	void SetInfiniteKeyAreas(bool bInInfiniteKeyAreas);

	/** @return true if showing channel colors */
	bool GetShowChannelColors() const;
	/** Set whether to show channel colors */
	void SetShowChannelColors(bool bInShowChannelColors);

	/** @return true if showing transport controls in level editor viewports */
	bool GetShowViewportTransportControls() const;
	/** Toggle whether to show transport controls in level editor viewports */
	void SetShowViewportTransportControls(bool bVisible);

	/** Snaps a time value in seconds to the currently selected interval. */
	float SnapTimeToInterval(float InTimeValue) const;

	/** Gets the multicast delegate which is run whenever the curve editor is shown/hidden. */
	FOnShowCurveEditorChanged& GetOnShowCurveEditorChanged();

	/** Gets the multicast delegate which is run whenever the time snap interval is changed. */
	FOnTimeSnapIntervalChanged& GetOnTimeSnapIntervalChanged();

protected:

	UPROPERTY( config, EditAnywhere, Category=Keyframing )
	TEnumAsByte<EAutoKeyMode> AutoKeyMode;

	UPROPERTY( config, EditAnywhere, Category=Keyframing )
	bool bKeyAllEnabled;

	UPROPERTY( config, EditAnywhere, Category=Keyframing )
	bool bKeyInterpPropertiesOnly;

	UPROPERTY( config, EditAnywhere, Category=Keyframing )
	TEnumAsByte<EMovieSceneKeyInterpolation> KeyInterpolation;

	UPROPERTY( config, EditAnywhere, Category=General )
	TEnumAsByte<ESequencerSpawnPosition> SpawnPosition;

	UPROPERTY( config, EditAnywhere, Category=General )
	bool bCreateSpawnableCameras;

	UPROPERTY( config, EditAnywhere, Category=Timeline )
	bool bShowFrameNumbers;

	UPROPERTY( config, EditAnywhere, Category=Timeline )
	bool bShowRangeSlider;

	UPROPERTY( config, EditAnywhere, Category=Snapping )
	bool bIsSnapEnabled;

	UPROPERTY( config, EditAnywhere, Category=Snapping )
	float TimeSnapInterval;

	UPROPERTY( config, EditAnywhere, Category=Snapping )
	bool bSnapKeyTimesToInterval;

	UPROPERTY( config, EditAnywhere, Category=Snapping )
	bool bSnapKeyTimesToKeys;

	UPROPERTY( config, EditAnywhere, Category=Snapping )
	bool bSnapSectionTimesToInterval;

	UPROPERTY( config, EditAnywhere, Category=Snapping )
	bool bSnapSectionTimesToSections;

	UPROPERTY( config, EditAnywhere, Category=Snapping )
	bool bSnapPlayTimeToKeys;

	UPROPERTY( config, EditAnywhere, Category=Snapping )
	bool bSnapPlayTimeToInterval;

	UPROPERTY( config, EditAnywhere, Category=Snapping )
	bool bSnapPlayTimeToDraggedKey;

	UPROPERTY( config, EditAnywhere, Category=Snapping )
	float CurveValueSnapInterval;

	UPROPERTY( config, EditAnywhere, Category=Snapping )
	bool bSnapCurveValueToInterval;

	UPROPERTY( config, EditAnywhere, Category=General )
	bool bLabelBrowserVisible;

	UPROPERTY( config, EditAnywhere, Category=Timeline )
	TEnumAsByte<ESequencerZoomPosition> ZoomPosition;

	UPROPERTY( config, EditAnywhere, Category=Timeline )
	bool bAutoScrollEnabled;

	UPROPERTY( config, EditAnywhere, Category=CurveEditor )
	bool bShowCurveEditor;

	UPROPERTY( config, EditAnywhere, Category=CurveEditor )
	bool bShowCurveEditorCurveToolTips;

	UPROPERTY( config, EditAnywhere, Category=CurveEditor )
	bool bLinkCurveEditorTimeRange;

	UPROPERTY( config )
	bool bLooping;

	UPROPERTY( config, EditAnywhere, Category=Timeline )
	bool bKeepCursorInPlayRange;

	UPROPERTY( config, EditAnywhere, Category=Timeline )
	bool bKeepPlayRangeInSectionBounds;

	UPROPERTY( config, EditAnywhere, Category=Timeline )
	uint8 ZeroPadFrames;

	UPROPERTY( config, EditAnywhere, Category=Timeline )
	bool bShowCombinedKeyframes;

	UPROPERTY( config, EditAnywhere, Category=Timeline )
	bool bInfiniteKeyAreas;

	UPROPERTY( config, EditAnywhere, Category=Timeline )
	bool bShowChannelColors;

	UPROPERTY( config )
	bool bShowViewportTransportControls;

	FOnShowCurveEditorChanged OnShowCurveEditorChanged;

	FOnTimeSnapIntervalChanged OnTimeSnapIntervalChanged;
};

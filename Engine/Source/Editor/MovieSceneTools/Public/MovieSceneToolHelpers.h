// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "Misc/Attribute.h"
#include "Widgets/SWidget.h"
#include "Curves/RichCurve.h"
#include "Math/InterpCurvePoint.h"
#include "MovieSceneObjectBindingID.h"
#include "MovieSceneObjectBindingIDPicker.h"
#include "ISequencer.h"

class ISequencer;
class UMovieScene;
class UMovieSceneSection;
class UInterpTrackMoveAxis;
struct FMovieSceneObjectBindingID;
class UMovieSceneTrack;

DECLARE_DELEGATE_TwoParams(FOnEnumSelectionChanged, int32 /*Selection*/, ESelectInfo::Type /*SelectionType*/);

class MOVIESCENETOOLS_API MovieSceneToolHelpers
{
public:

	/**
	 * Trim section at the given time
	 *
	 * @param Sections The sections to trim
	 * @param Time	The time at which to trim
	 * @param bTrimLeft Trim left or trim right
	 */
	static void TrimSection(const TSet<TWeakObjectPtr<UMovieSceneSection>>& Sections, float Time, bool bTrimLeft);

	/**
	 * Splits sections at the given time
	 *
	 * @param Sections The sections to split
	 * @param Time	The time at which to split
	 */
	static void SplitSection(const TSet<TWeakObjectPtr<UMovieSceneSection>>& Sections, float Time);

	/**
	 * Parse a shot name into its components.
	 *
	 * @param ShotName The shot name to parse
	 * @param ShotPrefix The parsed shot prefix
	 * @param ShotNumber The parsed shot number
	 * @param TakeNumber The parsed take number
	 * @return Whether the shot name was parsed successfully
	 */
	static bool ParseShotName(const FString& ShotName, FString& ShotPrefix, uint32& ShotNumber, uint32& TakeNumber);

	/**
	 * Compose a shot name given its components.
	 *
	 * @param ShotPrefix The shot prefix to use
	 * @param ShotNumber The shot number to use
	 * @param TakeNumber The take number to use
	 * @return The composed shot name
	 */
	static FString ComposeShotName(const FString& ShotPrefix, uint32 ShotNumber, uint32 TakeNumber);

	/**
	 * Generate a new shot package
	 *
	 * @param SequenceMovieScene The sequence movie scene for the new shot
	 * @param NewShotName The new shot name
	 * @return The new shot path
	 */
	static FString GenerateNewShotPath(UMovieScene* SequenceMovieScene, FString& NewShotName);

	/**
	 * Generate a new shot name
	 *
	 * @param AllSections All the sections in the given shot track
	 * @param Time The time to generate the new shot name at
	 * @return The new shot name
	 */
	static FString GenerateNewShotName(const TArray<UMovieSceneSection*>& AllSections, float Time);

	/**
	 * Gather takes - level sequence assets that have the same shot prefix and shot number in the same asset path (directory)
	 * 
	 * @param Section The section to gather takes from
	 * @param TakeNumbers The gathered take numbers
	 * @param CurrentTakeNumber The current take number of the section
	 */
	static void GatherTakes(const UMovieSceneSection* Section, TArray<uint32>& TakeNumbers, uint32& CurrentTakeNumber);


	/**
	 * Get the asset associated with the take number
	 *
	 * @param Section The section to gather the take from
	 * @param TakeNumber The take number to get
	 * @return The asset
	 */
	static UObject* GetTake(const UMovieSceneSection* Section, uint32 TakeNumber);

	/**
	 * Get the next available row index for the section so that it doesn't overlap any other sections in time.
	 *
	 * @param InTrack The track to find the next available row on
	 * @param InSection The section
	 * @return The next available row index
	 */
	static int32 FindAvailableRowIndex(UMovieSceneTrack* InTrack, UMovieSceneSection* InSection);

	/**
	 * Generate a combobox for editing enum values
	 *
	 * @param Enum The enum to make the combobox from
	 * @param CurrentValue The current value to display
	 * @param OnSelectionChanged Delegate fired when selection is changed
	 * @return The new widget
	 */
	static TSharedRef<SWidget> MakeEnumComboBox(const UEnum* Enum, TAttribute<int32> CurrentValue, FOnEnumSelectionChanged OnSelectionChanged);


	/**
	 * Show Import EDL Dialog
	 *
	 * @param InMovieScene The movie scene to import the edl into
	 * @param InFrameRate The frame rate to import the EDL at
	 * @param InOpenDirectory Optional directory path to open from. If none given, a dialog will pop up to prompt the user
	 * @return Whether the import was successful
	 */
	static bool ShowImportEDLDialog(UMovieScene* InMovieScene, float InFrameRate, FString InOpenDirectory = TEXT(""));

	/**
	 * Show Export EDL Dialog
	 *
	 * @param InMovieScene The movie scene with the cinematic shot track and audio tracks to export
	 * @param InFrameRate The frame rate to export the EDL at
	 * @param InSaveDirectory Optional directory path to save to. If none given, a dialog will pop up to prompt the user
	 * @param InHandleFrames The number of handle frames to include for each shot.
	 * @return Whether the export was successful
	 */
	static bool ShowExportEDLDialog(const UMovieScene* InMovieScene, float InFrameRate, FString InSaveDirectory = TEXT(""), int32 InHandleFrames = 8);

	/**
	 * Import FBX
	 *
	 * @param InMovieScene The movie scene to import the fbx into
	 * @param InObjectBindingNameMap The object binding to name map to map import fbx animation onto
	 * @return Whether the import was successful
	 */
	static bool ImportFBX(UMovieScene* InMovieScene, ISequencer& InSequencer, const TMap<FGuid, FString>& InObjectBindingNameMap);

	/*
	 * Rich curve interpolation to matinee interpolation
	 *
	 * @param InterpMode The rich curve interpolation to convert
	 * @return The converted matinee interpolation
	 */
	static EInterpCurveMode RichCurveInterpolationToMatineeInterpolation( ERichCurveInterpMode InterpMode );

	/*
	 * Copy rich curve to move axis
	 *
	 * @param RichCurve The rich curve to copy from
	 * @param MoveAxis The move axis to copy to
	 */
	static void CopyRichCurveToMoveAxis(const FRichCurve& RichCurve, UInterpTrackMoveAxis* MoveAxis);

	/*
	 * Export the object binding to a camera anim
	 *
	 * @param InMovieScene The movie scene to export the object binding from
	 * @param InObjectBinding The object binding to export
	 * @return The exported camera anim asset
	 */
	static UObject* ExportToCameraAnim(UMovieScene* InMovieScene, FGuid& InObjectBinding);
};

class FTrackEditorBindingIDPicker : public FMovieSceneObjectBindingIDPicker
{
public:
	FTrackEditorBindingIDPicker(FMovieSceneSequenceID InLocalSequenceID, TWeakPtr<ISequencer> InSequencer)
		: FMovieSceneObjectBindingIDPicker(InLocalSequenceID, InSequencer)
	{
		Initialize();
	}

	DECLARE_EVENT_OneParam(FTrackEditorBindingIDPicker, FOnBindingPicked, FMovieSceneObjectBindingID)
	FOnBindingPicked& OnBindingPicked()
	{
		return OnBindingPickedEvent;
	}

	using FMovieSceneObjectBindingIDPicker::GetPickerMenu;

private:

	virtual UMovieSceneSequence* GetSequence() const override { return WeakSequencer.Pin()->GetFocusedMovieSceneSequence(); }
	virtual void SetCurrentValue(const FMovieSceneObjectBindingID& InBindingId) override { OnBindingPickedEvent.Broadcast(InBindingId); }
	virtual FMovieSceneObjectBindingID GetCurrentValue() const override { return FMovieSceneObjectBindingID(); }

	FOnBindingPicked OnBindingPickedEvent;
};


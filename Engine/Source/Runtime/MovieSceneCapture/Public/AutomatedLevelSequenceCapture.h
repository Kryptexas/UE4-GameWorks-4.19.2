// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneCapture.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "AutomatedLevelSequenceCapture.generated.h"

UCLASS(config=EditorSettings)
class MOVIESCENECAPTURE_API UAutomatedLevelSequenceCapture : public UMovieSceneCapture
{
public:
	UAutomatedLevelSequenceCapture(const FObjectInitializer&);

	GENERATED_BODY()

	/** The level sequence to use during capture */
	UPROPERTY(EditAnywhere, Category=General, meta=(AllowedClasses="LevelSequence"))
	FStringAssetReference LevelSequence;

	/** The level to use for the capture */
	UPROPERTY(EditAnywhere, Category=General, meta=(AllowedClasses="World"))
	FStringAssetReference Level;

	/** Specific playback settings */
	UPROPERTY(config, EditAnywhere, Category="Playback Settings", meta=(ShowOnlyInnerProperties))
	FLevelSequencePlaybackSettings PlaybackSettings;

	/** The amount of time to wait before playback and capture start. Useful for allowing Post Processing effects to settle down before capturing the animation. */
	UPROPERTY(config, EditAnywhere, Category=CaptureSettings, AdvancedDisplay, meta=(Units=Seconds, ClampMin=0))
	float PrerollAmount;

public:
	// UMovieSceneCapture interface
	virtual FString GetPackageName() const override { return Level.ToString(); }
	virtual void Initialize(FViewport* InViewport) override;
	virtual void CaptureFrame(float DeltaSeconds) override;

private:

	/** Animation instance created at runtime before we start capturing */
	UPROPERTY(transient)
	ULevelSequenceInstance* AnimationInstance;

	/** Animation player used to playback the animation at runtime */
	UPROPERTY(transient)
	ULevelSequencePlayer* AnimationPlayback;

	/** Transient amount of time that has passed since the level was started. When >= Settings.Preroll, playback will start  */
	TOptional<float> PrerollTime;
};
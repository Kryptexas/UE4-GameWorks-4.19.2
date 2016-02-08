// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimationRecordingSettings.h"
#include "Components/SkinnedMeshComponent.h"

#include "ActorRecording.generated.h"

USTRUCT()
struct FActorRecordingSettings
{
	GENERATED_BODY()

	FActorRecordingSettings()
		: bRecordTransforms(false)
		, bRecordVisibility(true)
		, bRecordNearbySpawnedActors(false)
		, NearbyActorRecordingProximity(1000.0f)
	{}

public:
	/** 
	 * Whether to record actor transforms. This can be useful if you want the actors to end up in specific locations after the sequence. 
	 * By default we rely on animations to provide transforms, but this can be changed using the "Record In World Space" animation setting.
	 */
	UPROPERTY(EditAnywhere, Category = "Actor Recording")
	bool bRecordTransforms;

	/** Whether to record actor visibility. */
	UPROPERTY(EditAnywhere, Category = "Actor Recording")
	bool bRecordVisibility;

	/** Whether to record nearby spawned actors. */
	UPROPERTY(EditAnywhere, Category = "Actor Recording")
	bool bRecordNearbySpawnedActors;

	/** Proximity to currently recorded actors to record newly spawned actors. */
	UPROPERTY(EditAnywhere, Category = "Actor Recording")
	float NearbyActorRecordingProximity;
};

UCLASS(MinimalAPI, Transient)
class UActorRecording : public UObject
{
	GENERATED_BODY()

public:
	/** Start this queued recording */
	bool StartRecording();

	/** Stop this recording. Has no effect if we are not currently recording */
	bool StopRecording();

	/** Tick this recording */
	void Tick(float DeltaSeconds);

	/** Whether we are currently recording */
	bool IsRecording() const;

public:
	/** The actor we want to record */
	UPROPERTY(EditAnywhere, Category = "Actor Recording")
	TLazyObjectPtr<AActor> ActorToRecord;

	UPROPERTY(EditAnywhere, Category = "Actor Recording")
	FActorRecordingSettings ActorSettings;

	/** Whether we should specify the target animation or auto-create it */
	UPROPERTY(EditAnywhere, Category = "Animation Recording")
	bool bSpecifyTargetAnimation;

	/** The target animation we want to record to */
	UPROPERTY(EditAnywhere, Category = "Animation Recording", meta=(EditCondition = "bSpecifyTargetAnimation"))
	TWeakObjectPtr<UAnimSequence> TargetAnimation;

	/** The settings to apply to this actor's animation */
	UPROPERTY(EditAnywhere, Category = "Animation Recording")
	FAnimationRecordingSettings AnimationSettings;

	/** Our last recorded animation */
	UPROPERTY(Transient)
	TWeakObjectPtr<UAnimSequence> LastRecordedAnimation;

	/** Guid that identifies our spawnable in a recorded sequence */
	FGuid Guid;

private:
	/** Used to store/restore update flag when recording */
	EMeshComponentUpdateFlag::Type MeshComponentUpdateFlag;

	/** Used to store/restore URO when recording */
	bool bEnableUpdateRateOptimizations;
};
// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FSequenceRecorder
{
public:
	/** Singleton accessor */
	static FSequenceRecorder& Get();

	/** Starts recording a sequence. */
	bool StartRecording();

	/** Starts recording a sequence for the specified world. */
	bool StartRecordingForReplay(UWorld* World, const struct FSequenceRecorderActorFilter& ActorFilter);

	/** Stops any currently recording sequence */
	bool StopRecording();

	/** Tick the sequence recorder */
	void Tick(float DeltaSeconds);

	/** Get whether we are currently delaying a recording */
	bool IsDelaying() const;

	/** Get the current delay we are waiting for */
	float GetCurrentDelay() const;

	TWeakObjectPtr<class ULevelSequence> GetCurrentSequence() { return CurrentSequence; }

	bool IsRecordingQueued(AActor* Actor) const;

	class UActorRecording* FindRecording(AActor* Actor) const;

	void StartAllQueuedRecordings();

	void StopAllQueuedRecordings();

	void StopRecordingDeadAnimations();

	class UActorRecording* AddNewQueuedRecording(AActor* Actor = nullptr, UAnimSequence* AnimSequence = nullptr, float Length = 0.0f);

	void RemoveQueuedRecording(AActor* Actor);

	void RemoveQueuedRecording(class UActorRecording* Recording);

	bool HasQueuedRecordings() const;

	const TArray<class UActorRecording*>& GetQueuedRecordings() { return QueuedRecordings; }

	bool AreQueuedRecordingsDirty() const { return bQueuedRecordingsDirty; }

	void ResetQueuedRecordingsDirty() { bQueuedRecordingsDirty = false; }

	bool IsRecording() const { return CurrentSequence.IsValid(); }

private:
	/** Starts recording a sequence, possibly delayed */
	bool StartRecordingInternal(UWorld* World);

	/** Check if an actor is valid for recording */
	bool IsActorValidForRecording(AActor* Actor);

	/** Handle actors being spawned */
	static void HandleActorSpawned(AActor* Actor);

private:
	/** Constructor, private - use Get() function */
	FSequenceRecorder();

	/** Currently recording level sequence, if any */
	TWeakObjectPtr<class ULevelSequence> CurrentSequence;

	/** World we are recording a replay for, if any */
	TLazyObjectPtr<class UWorld> CurrentReplayWorld;

	TArray<class UActorRecording*> QueuedRecordings;

	TArray<class UActorRecording*> DeadRecordings;

	TArray<TWeakObjectPtr<AActor>> QueuedSpawnedActors;

	bool bQueuedRecordingsDirty;

	/** The delay we are currently waiting for */
	float CurrentDelay;

	/** Current recording time */
	float CurrentTime;

	/** Delegate handles for FOnActorSpawned events */
	TMap<TWeakObjectPtr<UWorld>, FDelegateHandle> ActorSpawningDelegateHandles;
};

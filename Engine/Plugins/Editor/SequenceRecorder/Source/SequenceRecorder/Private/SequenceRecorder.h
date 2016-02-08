// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FSequenceRecorder
{
public:
	/** Singleton accessor */
	static FSequenceRecorder& Get();

	/** Starts recording a sequence. */
	bool StartRecording();

	/** Stops any currently recording sequence */
	bool StopRecording();

	/** Tick the sequence recorder */
	void Tick(float DeltaSeconds);

	/** Get whether we are currently delaying a recording */
	bool IsDelaying() const;

	/** Get the current delay we are waiting for */
	float GetCurrentDelay() const;

	bool IsRecordingQueued(AActor* Actor) const;

	void StartAllQueuedRecordings();

	void StopAllQueuedRecordings();

	void StopRecordingDeadAnimations();

	void AddNewQueuedRecording(AActor* Actor = nullptr, UAnimSequence* AnimSequence = nullptr, float Length = 0.0f);

	void RemoveQueuedRecording(AActor* Actor);

	void RemoveQueuedRecording(class UActorRecording* Recording);

	bool HasQueuedRecordings() const;

	const TArray<class UActorRecording*>& GetQueuedRecordings() { return QueuedRecordings; }

	bool AreQueuedRecordingsDirty() const { return bQueuedRecordingsDirty; }

	void ResetQueuedRecordingsDirty() { bQueuedRecordingsDirty = false; }

private:
	/** Starts recording a sequence, possibly delayed */
	bool StartRecordingInternal();

private:
	/** Constructor, private - use Get() function */
	FSequenceRecorder();

	/** Currently recording level sequence, if any */
	TWeakObjectPtr<class ULevelSequence> CurrentSequence;

	/** The current set of property recorders */
	TArray<TSharedPtr<class IMovieScenePropertyRecorder>> PropertyRecorders;

	TArray<class UActorRecording*> QueuedRecordings;

	bool bQueuedRecordingsDirty;

	/** The delay we are currently waiting for */
	float CurrentDelay;

	/** Current recording time */
	float CurrentTime;
};

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FAnimationRecorder

// records the mesh pose to animation input
struct FAnimationRecorder
{
private:
	int32 SampleRate;
	float Duration;
	int32 LastFrame;
	float TimePassed;
	UAnimSequence * AnimationObject;
	TArray<FTransform> PreviousSpacesBases;
	
public:
	FAnimationRecorder();
	~FAnimationRecorder();

	void StartRecord(USkeletalMeshComponent * Component, UAnimSequence * InAnimationObject, float InDuration);
	void StopRecord();
	// return false if nothing to update
	// return true if it has properly updated
	bool UpdateRecord(USkeletalMeshComponent * Component, float DeltaTime);
	const UAnimSequence * GetAnimationObject() const { return AnimationObject; }

private:
	void Record( USkeletalMeshComponent * Component, TArray<FTransform> SpacesBases, int32 FrameToAdd );
};


// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/IQueuedWork.h"

class FImgMediaLoader;
class IImgMediaReader;

struct FImgMediaFrame;


/**
 * Loads a single image frame from disk.
 */
class FImgMediaLoaderWork
	: public IQueuedWork
{
public:

	/**
	 * Create and initialize a new instance.
	 *
	 * @param InOwner The loader that created this work item.
	 * @param InReader The image reader to use.
	 */
	FImgMediaLoaderWork(const TSharedRef<FImgMediaLoader, ESPMode::ThreadSafe>& InOwner, const TSharedRef<IImgMediaReader, ESPMode::ThreadSafe>& InReader);

public:

	/**
	 * Initialize this work item.
	 *
	 * @param InFrameNumber The number of the image frame.
	 * @param InImagePath The file path to the image frame to read.
	 * @see Shutdown
	 */
	void Initialize(int32 InFrameNumber, const FString& InImagePath);

public:

	//~ IQueuedWork interface

	virtual void Abandon() override;
	virtual void DoThreadedWork() override;

protected:

	/**
	 * Notify the owner of this work item, or self destruct.
	 *
	 * @param Frame The frame that was loaded by this work item.
	 */
	void Finalize(FImgMediaFrame* Frame);

private:

	/** The number of the image frame. */
	int32 FrameNumber;

	/** The file path to the image frame to read. */
	FString ImagePath;

	/** The loader that created this reader task. */
	TWeakPtr<FImgMediaLoader, ESPMode::ThreadSafe> OwnerPtr;

	/** The image sequence reader to use. */
	TSharedPtr<IImgMediaReader, ESPMode::ThreadSafe> Reader;
};

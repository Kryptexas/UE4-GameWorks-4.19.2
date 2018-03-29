// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ImgMediaLoaderWork.h"
#include "ImgMediaPrivate.h"

#include "Misc/ScopeLock.h"

#include "IImgMediaReader.h"
#include "ImgMediaLoader.h"


/** Time spent abandoning worker threads. */
DECLARE_CYCLE_STAT(TEXT("ImgMedia Loader Abadon Work"), STAT_ImgMedia_LoaderAbandonWork, STATGROUP_Media);

/** Time spent finalizing worker threads. */
DECLARE_CYCLE_STAT(TEXT("ImgMedia Loader Finalize Work"), STAT_ImgMedia_LoaderFinalizeWork, STATGROUP_Media);

/** Time spent reading image frames in worker threads. */
DECLARE_CYCLE_STAT(TEXT("ImgMedia Loader Read Frame"), STAT_ImgMedia_LoaderReadFrame, STATGROUP_Media);


/* FImgMediaLoaderWork structors
 *****************************************************************************/

FImgMediaLoaderWork::FImgMediaLoaderWork(const TSharedRef<FImgMediaLoader, ESPMode::ThreadSafe>& InOwner, const TSharedRef<IImgMediaReader, ESPMode::ThreadSafe>& InReader)
	: FrameNumber(INDEX_NONE)
	, OwnerPtr(InOwner)
	, Reader(InReader)
{ }


/* FImgMediaLoaderWork interface
 *****************************************************************************/

void FImgMediaLoaderWork::Initialize(int32 InFrameNumber, const FString& InImagePath)
{
	FrameNumber = InFrameNumber;
	ImagePath = InImagePath;
}


/* IQueuedWork interface
 *****************************************************************************/

void FImgMediaLoaderWork::Abandon()
{
	SCOPE_CYCLE_COUNTER(STAT_ImgMedia_LoaderAbandonWork);

	Finalize(nullptr);
}


void FImgMediaLoaderWork::DoThreadedWork()
{
	FImgMediaFrame* Frame;

	if ((FrameNumber == INDEX_NONE) || ImagePath.IsEmpty())
	{
		Frame = nullptr;
	}
	else
	{
//		SCOPE_CYCLE_COUNTER(STAT_ImgMedia_LoaderReadFrame);

		// read the image frame
		Frame = new FImgMediaFrame();

		if (!Reader->ReadFrame(ImagePath, *Frame))
		{
			delete Frame;
			Frame = nullptr;
		}
	}

//	SCOPE_CYCLE_COUNTER(STAT_ImgMedia_LoaderFinalizeWork);

	Finalize(Frame);
}


/* FImgMediaLoaderWork implementation
 *****************************************************************************/

void FImgMediaLoaderWork::Finalize(FImgMediaFrame* Frame)
{
	TSharedPtr<FImgMediaLoader, ESPMode::ThreadSafe> Owner = OwnerPtr.Pin();

	if (Owner.IsValid())
	{
		Owner->NotifyWorkComplete(*this, FrameNumber, MakeShareable(Frame));
	}
	else
	{
		delete Frame;
		delete this; // owner is gone, self destruct!
	}
}

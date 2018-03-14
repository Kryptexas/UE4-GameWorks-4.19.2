// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneMediaTemplate.h"

#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "UObject/Package.h"
#include "UObject/GCObject.h"

#include "MovieSceneMediaData.h"
#include "MovieSceneMediaSection.h"
#include "MovieSceneMediaTrack.h"


/* Local helpers
 *****************************************************************************/

struct FMediaSectionPreRollExecutionToken
	: IMovieSceneExecutionToken
{
	FMediaSectionPreRollExecutionToken(UMediaSource* InMediaSource, FTimespan InStartTimeSeconds)
		: MediaSource(InMediaSource)
		, StartTime(InStartTimeSeconds)
	{ }

	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	{
		using namespace PropertyTemplate;

		FMovieSceneMediaData& SectionData = PersistentData.GetSectionData<FMovieSceneMediaData>();
		UMediaPlayer* MediaPlayer = SectionData.GetMediaPlayer();

		if (!ensure(MediaPlayer != nullptr))
		{
			return;
		}

		// open the media source if necessary
		if (MediaPlayer->GetUrl().IsEmpty())
		{
			SectionData.SeekOnOpen(StartTime);
			MediaPlayer->OpenSource(MediaSource);
		}
	}

private:

	UMediaSource* MediaSource;
	FTimespan StartTime;
};


struct FMediaSectionExecutionToken
	: IMovieSceneExecutionToken
{
	FMediaSectionExecutionToken(UMediaSource* InMediaSource, FTimespan InCurrentTime)
		: CurrentTime(InCurrentTime)
		, MediaSource(InMediaSource)
		, PlaybackRate(1.0f)
	{ }

	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	{
		FMovieSceneMediaData& SectionData = PersistentData.GetSectionData<FMovieSceneMediaData>();
		UMediaPlayer* MediaPlayer = SectionData.GetMediaPlayer();

		if (!ensure(MediaPlayer != nullptr))
		{
			return;
		}

		// open the media source if necessary
		if (MediaPlayer->GetUrl().IsEmpty())
		{
			SectionData.SeekOnOpen(CurrentTime);
			MediaPlayer->OpenSource(MediaSource);

			return;
		}

		if (MediaPlayer->IsPreparing())
		{
			SectionData.SeekOnOpen(CurrentTime);

			return;
		}

		// update media player
		const FTimespan MediaTime = CurrentTime % MediaPlayer->GetDuration();

		if (Context.GetStatus() == EMovieScenePlayerStatus::Playing)
		{
			if (!MediaPlayer->IsPlaying())
			{
				MediaPlayer->Seek(MediaTime);
				MediaPlayer->SetRate(1.0f);
			}
			else if (Context.HasJumped())
			{
				MediaPlayer->Seek(MediaTime);
			}
		}
		else
		{
			if (MediaPlayer->IsPlaying())
			{
				MediaPlayer->SetRate(0.0f);
			}

			MediaPlayer->Seek(MediaTime);
		}
	}

private:

	FTimespan CurrentTime;
	UMediaSource* MediaSource;
	float PlaybackRate;
};


/* FMovieSceneMediaSectionTemplate structors
 *****************************************************************************/

FMovieSceneMediaSectionTemplate::FMovieSceneMediaSectionTemplate(const UMovieSceneMediaSection& InSection, const UMovieSceneMediaTrack& InTrack)
{
	Params.MediaSoundComponent = InSection.MediaSoundComponent;
	Params.MediaSource = InSection.GetMediaSource();
	Params.MediaTexture = InSection.MediaTexture;
	Params.SectionEndTime = InSection.GetEndTime();
	Params.SectionStartTime = InSection.GetStartTime();
}


/* FMovieSceneEvalTemplate interface
 *****************************************************************************/

void FMovieSceneMediaSectionTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	if ((Params.MediaSource == nullptr) || Context.IsPostRoll())
	{
		return;
	}

	const int64 RoundingErrorHack = 100;

	// @todo: account for start offset and video time dilation if/when these are added

	if (Context.IsPreRoll())
	{
		if (Context.GetDirection() == EPlayDirection::Forwards)
		{
			float StartTimeSeconds = Context.HasPreRollEndTime() ? (Context.GetPreRollEndTime() - Params.SectionStartTime) : 0.f;
			ExecutionTokens.Add(FMediaSectionPreRollExecutionToken(Params.MediaSource, FTimespan(int64(StartTimeSeconds * ETimespan::TicksPerSecond + RoundingErrorHack))));
		}
	}
	else if (!Context.IsPostRoll() && (Context.GetTime() < Params.SectionEndTime))
	{
		float CurrentTimeSeconds = Context.GetTime() - Params.SectionStartTime;
		ExecutionTokens.Add(FMediaSectionExecutionToken(Params.MediaSource, int64(CurrentTimeSeconds * ETimespan::TicksPerSecond + RoundingErrorHack)));
	}
}


UScriptStruct& FMovieSceneMediaSectionTemplate::GetScriptStructImpl() const
{
	return *StaticStruct();
}


void FMovieSceneMediaSectionTemplate::Initialize(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
{
	FMovieSceneMediaData* SectionData = PersistentData.FindSectionData<FMovieSceneMediaData>();

	if (!ensure(SectionData != nullptr))
	{
		return;
	}

	UMediaPlayer* MediaPlayer = SectionData->GetMediaPlayer();

	if (!ensure(MediaPlayer != nullptr))
	{
		return;
	}

	const bool IsEvaluating = !(Context.IsPreRoll() || Context.IsPostRoll() || (Context.GetTime() >= Params.SectionEndTime));

	if (Params.MediaSoundComponent != nullptr)
	{
		if (IsEvaluating)
		{
			Params.MediaSoundComponent->SetMediaPlayer(MediaPlayer);
		}
		else if (Params.MediaSoundComponent->GetMediaPlayer() == MediaPlayer)
		{
			Params.MediaSoundComponent->SetMediaPlayer(nullptr);
		}
	}

	if (Params.MediaTexture != nullptr)
	{
		if (IsEvaluating)
		{
			Params.MediaTexture->SetMediaPlayer(MediaPlayer);
		}
		else if (Params.MediaTexture->GetMediaPlayer() == MediaPlayer)
		{
			Params.MediaTexture->SetMediaPlayer(nullptr);
		}
	}
}


void FMovieSceneMediaSectionTemplate::Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
{
	PersistentData.AddSectionData<FMovieSceneMediaData>().Setup();
}


void FMovieSceneMediaSectionTemplate::SetupOverrides()
{
	EnableOverrides(RequiresInitializeFlag | RequiresSetupFlag | RequiresTearDownFlag);
}


void FMovieSceneMediaSectionTemplate::TearDown(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
{
	FMovieSceneMediaData* SectionData = PersistentData.FindSectionData<FMovieSceneMediaData>();

	if (!ensure(SectionData != nullptr))
	{
		return;
	}

	UMediaPlayer* MediaPlayer = SectionData->GetMediaPlayer();

	if (!ensure(MediaPlayer != nullptr))
	{
		return;
	}

	if ((Params.MediaSoundComponent != nullptr) && (Params.MediaSoundComponent->GetMediaPlayer() == MediaPlayer))
	{
		Params.MediaSoundComponent->SetMediaPlayer(nullptr);
	}

	if ((Params.MediaTexture != nullptr) && (Params.MediaTexture->GetMediaPlayer() == MediaPlayer))
	{
		Params.MediaTexture->SetMediaPlayer(nullptr);
	}
}

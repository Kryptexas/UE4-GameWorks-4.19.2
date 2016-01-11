// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Editor/Sequencer/Public/ISequencerModule.h"
#include "Editor/Sequencer/Public/MovieSceneTrackEditor.h"
#include "Editor/Sequencer/Public/ISequencerSection.h"
#include "Runtime/MovieScene/Public/MovieScene.h"
#include "Runtime/MovieScene/Public/MovieSceneTrack.h"
#include "Runtime/MovieScene/Public/IMovieSceneTrackInstance.h"
#include "Runtime/MovieScene/Public/MovieSceneSection.h"

#include "NiagaraSequencer.generated.h"

/** 
 *	Niagara editor movie scene section; represents one emitter in the timeline
 */
UCLASS()
class UNiagaraMovieSceneSection : public UMovieSceneSection
{
	GENERATED_UCLASS_BODY()
public:
	FText GetEmitterName()	{ return EmitterName; }
	void SetEmitterName(FText InName)	{ EmitterName = InName; }
	void SetEmitterProps(TWeakObjectPtr<const UNiagaraEmitterProperties> Props)	{ EmitterProps = Props; }

	TWeakObjectPtr<const UNiagaraEmitterProperties> GetEmitterProps() const	{ return EmitterProps; }

private:
	FText EmitterName;
	TWeakObjectPtr<const UNiagaraEmitterProperties> EmitterProps;
};

/**
*	Visual representation of UNiagaraMovieSceneSection
*/
class INiagaraSequencerSection : public ISequencerSection
{
public:
	INiagaraSequencerSection(UMovieSceneSection &SectionObject)
	{
		UNiagaraMovieSceneSection *NiagaraSectionObject = Cast<UNiagaraMovieSceneSection>(&SectionObject);
		check(NiagaraSectionObject);

		EmitterSection = NiagaraSectionObject;
	}

	UMovieSceneSection *GetSectionObject(void)
	{
		return EmitterSection;
	}

	int32 OnPaintSection(const FGeometry &AllottedGeometry, const FSlateRect &SectionClippingRect, FSlateWindowElementList &OutDrawElements, int32 LayerId, bool  bParentEnabled) const
	{
		// draw the first run of the emitter
		FSlateDrawElement::MakeBox
			(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FEditorStyle::GetBrush("CurveEd.TimelineArea"),
			SectionClippingRect,
			ESlateDrawEffect::None,
			FLinearColor(0.3f, 0.3f, 0.6f)
			);


		// draw all loops of the emitter as 'ghosts' of the original section
		float X = AllottedGeometry.AbsolutePosition.X;
		float GeomW = AllottedGeometry.GetDrawSize().X;
		float ClipW = SectionClippingRect.Right-SectionClippingRect.Left;
		FSlateRect NewClipRect = SectionClippingRect;
		NewClipRect.Left += 5;
		NewClipRect.Right -= 10;
		for (int32 Loop = 0; Loop < EmitterSection->GetEmitterProps()->NumLoops - 1; Loop++)
		{
			NewClipRect.Left += (GeomW) - FMath::Max(0.0f, GeomW - ClipW);
			NewClipRect.Right += GeomW;
			FSlateDrawElement::MakeBox
				(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2D(GeomW*(Loop+1), 0.0f), AllottedGeometry.GetDrawSize(), 1.0f),
				FEditorStyle::GetBrush("CurveEd.TimelineArea"),
				NewClipRect,
				ESlateDrawEffect::None,
				FLinearColor(0.3f, 0.3f, 0.6f, 0.25f)
				);
		}

		return LayerId;
	}

	FText GetDisplayName(void) const	{ return EmitterSection->GetEmitterName(); }
	FText GetSectionTitle(void) const	{ return EmitterSection->GetEmitterName(); }
	float GetSectionHeight() const		{ return 32.0f; }

	void GenerateSectionLayout(ISectionLayoutBuilder &) const {}

private:
	UNiagaraMovieSceneSection *EmitterSection;
};





/**
*	One instance of a UEmitterMovieSceneTrack
*/
class INiagaraTrackInstance : public IMovieSceneTrackInstance
{
public:
	INiagaraTrackInstance(class UEmitterMovieSceneTrack *InTrack)
		: Track(InTrack)
	{
	}

	virtual void Update(float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player)
	{
	}

	virtual void RefreshInstance(const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player);

	virtual void ClearInstance( IMovieScenePlayer& Player ) override {}
	virtual void SaveState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player) override {}
	virtual void RestoreState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player) override {}

private:
	TSharedPtr<FNiagaraSimulation> Emitter;
	class UEmitterMovieSceneTrack *Track;
};





/**
*	Single track containing exactly one UNiagaraMovieSceneSection, representing one emitter
*/
UCLASS(MinimalAPI)
class UEmitterMovieSceneTrack : public UMovieSceneTrack
{
	GENERATED_UCLASS_BODY()
public:

	/** UMovieSceneTrack interface */
	virtual FName GetTrackName() const override { return *Emitter->GetProperties()->EmitterName; }
	void SetEmitter(TSharedPtr<FNiagaraSimulation> InEmitter)
	{
		if (InEmitter.IsValid())
		{
			Emitter = InEmitter;
			if (const UNiagaraEmitterProperties* EmitterProps = InEmitter->GetProperties().Get())
			{
				UNiagaraMovieSceneSection *Section = NewObject<UNiagaraMovieSceneSection>(this, *Emitter->GetProperties()->EmitterName);
				Section->SetEmitterProps(EmitterProps);

				Section->SetStartTime(EmitterProps->StartTime);
				Section->SetEndTime(EmitterProps->EndTime);
				Section->SetEmitterName(FText::FromString(EmitterProps->EmitterName));

				Sections.Add(Section);
			}
		}
	}

	virtual void RemoveAllAnimationData() override	{};
	virtual bool HasSection(UMovieSceneSection* Section) const override { return false; }
	virtual bool IsEmpty() const override { return false; }
	virtual TSharedPtr<class IMovieSceneTrackInstance> CreateInstance()
	{
		return MakeShareable(new INiagaraTrackInstance(this));
	}
	virtual const TArray<UMovieSceneSection*>& GetAllSections() const override
	{
		return Sections;
	}

	virtual TRange<float> GetSectionBoundaries() const override
	{
		return TRange<float>(0, FLT_MAX);
	}

	TSharedPtr<FNiagaraSimulation> GetEmitter() { return Emitter; }

private:
	TSharedPtr<FNiagaraSimulation> Emitter;
	TArray<UMovieSceneSection*> Sections;
};



/**
*	Track editor for Niagara emitter tracks
*/
class FNiagaraTrackEditor : public FMovieSceneTrackEditor
{
public:
	FNiagaraTrackEditor(TSharedPtr<ISequencer> Sequencer) : FMovieSceneTrackEditor(Sequencer.ToSharedRef())
	{

	}

	virtual bool SupportsType(TSubclassOf<class UMovieSceneTrack> TrackClass) const
	{
		if (TrackClass == UEmitterMovieSceneTrack::StaticClass())
		{
			return true;
		}
		return false;
	}

	virtual TSharedRef<ISequencerSection> MakeSectionInterface(class UMovieSceneSection& SectionObject, UMovieSceneTrack& Track)
	{
		INiagaraSequencerSection *Sec = new INiagaraSequencerSection(SectionObject);
		return MakeShareable(Sec);
	}

};

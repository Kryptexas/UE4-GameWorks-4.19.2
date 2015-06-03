// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "Editor/Sequencer/Public/ISequencerModule.h"
#include "Editor/Sequencer/Public/ISequencerObjectBindingManager.h"
#include "Editor/Sequencer/Public/MovieSceneTrackEditor.h"
#include "Editor/Sequencer/Public/ISequencerSection.h"
#include "Runtime/MovieSceneCore/Classes/MovieScene.h"
#include "Runtime/MovieSceneCore/Classes/MovieSceneTrack.h"
#include "Runtime/MovieSceneCore/Public/IMovieSceneTrackInstance.h"
#include "Runtime/MovieSceneCore/Classes/MovieSceneSection.h"

#include "NiagaraSequencer.generated.h"

UCLASS()
class UNiagaraMovieSceneSection : public UMovieSceneSection
{
	GENERATED_UCLASS_BODY()
public:
	FText GetEmitterName()	{ return EmitterName; }
	void SetEmitterName(FText InName)	{ EmitterName = InName; }

private:
	FText EmitterName;
};

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


		return 0;
	}

	FText GetDisplayName(void) const	{ return EmitterSection->GetEmitterName(); }
	FText GetSectionTitle(void) const	{ return EmitterSection->GetEmitterName(); }
	float GetSectionHeight() const		{ return 32.0f; }

	void ISequencerSection::GenerateSectionLayout(ISectionLayoutBuilder &) const {}

private:
	UNiagaraMovieSceneSection *EmitterSection;
};



class INiagaraTrackInstance : public IMovieSceneTrackInstance
{
public:
	INiagaraTrackInstance(TSharedPtr<FNiagaraSimulation> InEmitter)
	{
		Emitter = InEmitter;
	}

	virtual void Update(float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player)
	{}
	virtual void RefreshInstance(const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player)
	{}

private:
	TSharedPtr<FNiagaraSimulation> Emitter;
};







UCLASS(MinimalAPI)
class UEmitterMovieSceneTrack : public UMovieSceneTrack
{
	GENERATED_UCLASS_BODY()
public:

	/** UMovieSceneTrack interface */
	virtual FName GetTrackName() const override { return *Emitter->GetProperties()->Name; }
	void SetEmitter(TSharedPtr<FNiagaraSimulation> InEmitter)
	{
		Emitter = InEmitter;
		const FNiagaraEmitterProperties *EmitterProps = InEmitter->GetProperties();

		UNiagaraMovieSceneSection *Section = NewObject<UNiagaraMovieSceneSection>(this, *Emitter->GetProperties()->Name);
		Section->SetStartTime(EmitterProps->StartTime);
		Section->SetEndTime(EmitterProps->EndTime);
		Section->SetEmitterName(FText::FromString(EmitterProps->Name));

		Sections.Add(Section);
	}

	virtual void RemoveAllAnimationData() override	{};
	virtual bool HasSection(UMovieSceneSection* Section) const override { return false; }
	virtual bool IsEmpty() const override { return false; }
	virtual TSharedPtr<class IMovieSceneTrackInstance> CreateInstance()
	{
		return MakeShareable(new INiagaraTrackInstance(Emitter));
	}
	virtual TArray<UMovieSceneSection*> GetAllSections() const
	{
		return Sections;
	}
private:
	TSharedPtr<FNiagaraSimulation> Emitter;
	TArray<UMovieSceneSection*> Sections;
};



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

	virtual TSharedRef<ISequencerSection> MakeSectionInterface(class UMovieSceneSection& SectionObject, UMovieSceneTrack* Track)
	{
		INiagaraSequencerSection *Sec = new INiagaraSequencerSection(SectionObject);
		return MakeShareable(Sec);
	}

};




/** Sequencer object binding manager for Niagara effects;
*/
class FNiagaraSequencerObjectBindingManager : public FGCObject, public ISequencerObjectBindingManager
{
public:
	FNiagaraSequencerObjectBindingManager()
	{}

	~FNiagaraSequencerObjectBindingManager()
	{}

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override {	}

	/** ISequencerObjectBindingManager interface */
	virtual bool AllowsSpawnableObjects() const override { return false; }
	virtual FGuid FindGuidForObject(const UMovieScene& MovieScene, UObject& Object) const override { return FGuid(); }
	virtual void SpawnOrDestroyObjectsForInstance(TSharedRef<FMovieSceneInstance> MovieSceneInstance, bool bDestroyAll) override { }
	virtual void DestroyAllSpawnedObjects() override { }
	virtual bool CanPossessObject(UObject& Object) const override 	{ return false; }
	virtual void BindPossessableObject(const FGuid& PossessableGuid, UObject& PossessedObject) override	{};
	virtual void UnbindPossessableObjects(const FGuid& PossessableGuid) override	{};
	virtual void GetRuntimeObjects(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, TArray<UObject*>& OutRuntimeObjects) const override	{}

	virtual bool TryGetObjectBindingDisplayName(const FGuid& ObjectGuid, FText& DisplayName) const override	{ return false; }
private:
};
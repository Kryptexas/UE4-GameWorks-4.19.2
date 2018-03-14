// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneNameableTrack.h"
#include "MovieSceneNiagaraEmitterTrack.generated.h"


class FNiagaraEmitterHandleViewModel;

/**
*	Single track containing exactly one UNiagaraMovieSceneSection, representing one emitter
*/
UCLASS(MinimalAPI)
class UMovieSceneNiagaraEmitterTrack
	: public UMovieSceneNameableTrack
{
	GENERATED_UCLASS_BODY()
public:

	TSharedPtr<FNiagaraEmitterHandleViewModel> GetEmitterHandle();

	void SetEmitterHandle(TSharedRef<FNiagaraEmitterHandleViewModel> InEmitterHandleViewModel);

public:

	//~ UMovieSceneTrack interface
	virtual void RemoveAllAnimationData() override { }
	virtual bool HasSection(const UMovieSceneSection& Section) const override;
	virtual void RemoveSection(UMovieSceneSection& Section) override;
	virtual bool IsEmpty() const override;
	virtual const TArray<UMovieSceneSection*>& GetAllSections() const override;
	virtual TRange<float> GetSectionBoundaries() const override;
	virtual FName GetTrackName() const override;
	/** Gets the unique id for the emitter handle that was associated with this track; used for copy/paste detection */
	FGuid GetEmitterHandleId() const;

private:
	void EmitterPropertyChanged();
	void EmitterHandlePropertyChanged();
	void SynchronizeWithEmitterHandle();

private:
	TWeakPtr<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel;

	UPROPERTY()
	TArray<UMovieSceneSection*> Sections;

	// Used for detecting copy/paste 
	UPROPERTY()
	FGuid EmitterHandleId;
};
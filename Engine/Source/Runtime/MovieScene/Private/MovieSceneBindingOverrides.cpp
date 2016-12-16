// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneBindingOverrides.h"

UMovieSceneBindingOverrides::UMovieSceneBindingOverrides(const FObjectInitializer& Init)
	: Super(Init)
	, bLookupDirty(true)
{
}

bool UMovieSceneBindingOverrides::LocateBoundObjects(const FGuid& InBindingId, TArray<UObject*, TInlineAllocator<1>>& OutObjects) const
{
	if (bLookupDirty)
	{
		RebuildLookupMap();
	}

	bool bAllowDefault = true;

	for (auto It = LookupMap.CreateConstKeyIterator(InBindingId);It;++It)
	{
		const FMovieSceneBindingOverrideData& Data = BindingData[It.Value()];
		UObject* Object = Data.Object.Get();

		if (Data.bOverridesDefault)
		{
			bAllowDefault = false;
		}

		if (Object)
		{
			OutObjects.Add(Object);
		}
	}

	return bAllowDefault;
}

void UMovieSceneBindingOverrides::SetBinding(FMovieSceneObjectBindingPtr Binding, const TArray<UObject*>& Objects, bool bAllowBindingsFromAsset)
{
	ResetBinding(Binding);

	for (UObject* Object : Objects)
	{
		if (!Object)
		{
			continue;
		}

		LookupMap.Add(Binding.Guid, BindingData.Num());

		FMovieSceneBindingOverrideData NewBinding;
		NewBinding.ObjectBindingId = Binding;
		NewBinding.Object = Object;
		NewBinding.bOverridesDefault = !bAllowBindingsFromAsset;
		BindingData.Add(NewBinding);
	}
}

void UMovieSceneBindingOverrides::AddBinding(FMovieSceneObjectBindingPtr Binding, UObject* Object, bool bAllowBindingsFromAsset)
{
	if (Object)
	{
		LookupMap.Add(Binding.Guid, BindingData.Num());

		FMovieSceneBindingOverrideData NewBinding;
		NewBinding.ObjectBindingId = Binding;
		NewBinding.Object = Object;
		NewBinding.bOverridesDefault = !bAllowBindingsFromAsset;
		BindingData.Add(NewBinding);
	}
}

void UMovieSceneBindingOverrides::RemoveBinding(FMovieSceneObjectBindingPtr Binding, UObject* Object)
{
	int32 NumRemoved = BindingData.RemoveAll([=](const FMovieSceneBindingOverrideData& InBindingData){
		return InBindingData.Object == Object && Binding.Guid == InBindingData.ObjectBindingId.Guid;
	});

	if (NumRemoved)
	{
		bLookupDirty = true;
	}
}

void UMovieSceneBindingOverrides::ResetBinding(FMovieSceneObjectBindingPtr Binding)
{
	int32 NumRemoved = BindingData.RemoveAll([=](const FMovieSceneBindingOverrideData& InBindingData){
		return Binding.Guid == InBindingData.ObjectBindingId.Guid;
	});

	if (NumRemoved)
	{
		bLookupDirty = true;
	}
}

void UMovieSceneBindingOverrides::ResetBindings()
{
	if (BindingData.Num())
	{
		BindingData.Reset();
		LookupMap.Reset();
		bLookupDirty = false;
	}
}

void UMovieSceneBindingOverrides::RebuildLookupMap() const
{
	LookupMap.Reset();

	for (int32 Index = 0; Index < BindingData.Num(); ++Index)
	{
		LookupMap.Add(BindingData[Index].ObjectBindingId.Guid, Index);
	}

	bLookupDirty = false;
}

#if WITH_EDITOR

void UMovieSceneBindingOverrides::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildLookupMap();
}

#endif
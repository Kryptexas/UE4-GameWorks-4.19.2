// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "LevelSequenceEditorPCH.h"
#include "ISequencer.h"
#include "ISequencerObjectChangeListener.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "LevelSequenceEditorSpawnRegister.h"



/* FLevelSequenceEditorSpawnRegister structors
 *****************************************************************************/

FLevelSequenceEditorSpawnRegister::FLevelSequenceEditorSpawnRegister()
{
	bShouldClearSelectionCache = true;

	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	OnActorSelectionChangedHandle = LevelEditor.OnActorSelectionChanged().AddRaw(this, &FLevelSequenceEditorSpawnRegister::HandleActorSelectionChanged);

	OnActorMovedHandle = GEditor->OnActorMoved().AddLambda([=](AActor* Actor){
		HandleAnyPropertyChanged(*Actor);
	});
}


FLevelSequenceEditorSpawnRegister::~FLevelSequenceEditorSpawnRegister()
{
	GEditor->OnActorMoved().Remove(OnActorMovedHandle);
	if (FLevelEditorModule* LevelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor"))
	{
		LevelEditor->OnActorSelectionChanged().Remove(OnActorSelectionChangedHandle);
	}
}


/* FLevelSequenceSpawnRegister interface
 *****************************************************************************/

UObject* FLevelSequenceEditorSpawnRegister::SpawnObject(const FGuid& BindingId, FMovieSceneSequenceInstance& SequenceInstance, IMovieScenePlayer& Player)
{
	TGuardValue<bool> Guard(bShouldClearSelectionCache, false);
	UObject* NewObject = FLevelSequenceSpawnRegister::SpawnObject(BindingId, SequenceInstance, Player);

	// Add an object listener for the spawned object to propagate changes back onto the spawnable default
	TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();

	if (Sequencer.IsValid() && NewObject)
	{
		Sequencer->GetObjectChangeListener().GetOnAnyPropertyChanged(*NewObject).AddSP(this, &FLevelSequenceEditorSpawnRegister::HandleAnyPropertyChanged);

		// Select the actor if we think it should be selected
		AActor* Actor = Cast<AActor>(NewObject);
		if (Actor && SelectedSpawnedObjects.Contains(FMovieSceneSpawnRegisterKey(BindingId, SequenceInstance)))
		{
			GEditor->SelectActor(Actor, true /*bSelected*/, true /*bNotify*/);
		}
	}

	return NewObject;
}


void FLevelSequenceEditorSpawnRegister::PreDestroyObject(UObject& Object, const FGuid& BindingId, FMovieSceneSequenceInstance& SequenceInstance)
{
	TGuardValue<bool> Guard(bShouldClearSelectionCache, false);

	// Cache its selection state
	AActor* Actor = Cast<AActor>(&Object);
	if (Actor && GEditor->GetSelectedActors()->IsSelected(Actor))
	{
		SelectedSpawnedObjects.Add(FMovieSceneSpawnRegisterKey(BindingId, SequenceInstance));
		GEditor->SelectActor(Actor, false /*bSelected*/, true /*bNotify*/);
	}

	// Remove our object listener
	TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
	if (Sequencer.IsValid())
	{
		Sequencer->GetObjectChangeListener().ReportObjectDestroyed(Object);
	}

	FLevelSequenceSpawnRegister::PreDestroyObject(Object, BindingId, SequenceInstance);
}


/* FLevelSequenceEditorSpawnRegister implementation
 *****************************************************************************/

void FLevelSequenceEditorSpawnRegister::PopulateKeyedPropertyMap(AActor& SpawnedObject, TMap<UObject*, TSet<UProperty*>>& OutKeyedPropertyMap)
{
	TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
	Sequencer->GetAllKeyedProperties(SpawnedObject, OutKeyedPropertyMap.FindOrAdd(&SpawnedObject));

	for (UActorComponent* Component : SpawnedObject.GetComponents())
	{
		Sequencer->GetAllKeyedProperties(*Component, OutKeyedPropertyMap.FindOrAdd(Component));
	}
}


/* FLevelSequenceEditorSpawnRegister callbacks
 *****************************************************************************/

void FLevelSequenceEditorSpawnRegister::HandleActorSelectionChanged(const TArray<UObject*>& NewSelection, bool bForceRefresh)
{
	if (bShouldClearSelectionCache)
	{
		SelectedSpawnedObjects.Reset();
	}
}


void FLevelSequenceEditorSpawnRegister::HandleAnyPropertyChanged(UObject& SpawnedObject)
{
	//@todo: AndrewR to filter this to just spawned objects only
	return;

	using namespace EditorUtilities;

	AActor* Actor = CastChecked<AActor>(&SpawnedObject);
	if (!Actor)
	{
		return;
	}

	TMap<UObject*, TSet<UProperty*>> ObjectToKeyedProperties;
	PopulateKeyedPropertyMap(*Actor, ObjectToKeyedProperties);

	// Copy any changed actor properties onto the default actor, provided they are not keyed
	FCopyOptions Options(ECopyOptions::PropagateChangesToArchetypeInstances);

	// Set up a property filter so only stuff that is not keyed gets copied onto the default
	Options.PropertyFilter = [&](const UProperty& Property, const UObject& Object) -> bool {
		const TSet<UProperty*>* ExcludedProperties = ObjectToKeyedProperties.Find(const_cast<UObject*>(&Object));

		return !ExcludedProperties || !ExcludedProperties->Contains(const_cast<UProperty*>(&Property));
	};

	// Now copy the actor properties
	AActor* DefaultActor = Actor->GetClass()->GetDefaultObject<AActor>();
	EditorUtilities::CopyActorProperties(Actor, DefaultActor, Options);

	// The above function call explicitly doesn't copy the root component transform (so the default actor is always at 0,0,0)
	// But in sequencer, we want the object to have a default transform if it doesn't have a transform track
	static FName RelativeLocation = GET_MEMBER_NAME_CHECKED(USceneComponent, RelativeLocation);
	static FName RelativeRotation = GET_MEMBER_NAME_CHECKED(USceneComponent, RelativeRotation);
	static FName RelativeScale3D = GET_MEMBER_NAME_CHECKED(USceneComponent, RelativeScale3D);

	bool bHasKeyedTransform = false;
	for (UProperty* Property : *ObjectToKeyedProperties.Find(Actor))
	{
		FName PropertyName = Property->GetFName();
		bHasKeyedTransform = PropertyName == RelativeLocation || PropertyName == RelativeRotation || PropertyName == RelativeScale3D;
		if (bHasKeyedTransform)
		{
			break;
		}
	}

	// Set the default transform if it's not keyed
	USceneComponent* RootComponent = Actor->GetRootComponent();
	USceneComponent* DefaultRootComponent = DefaultActor->GetRootComponent();

	if (!bHasKeyedTransform && RootComponent && DefaultRootComponent)
	{
		DefaultRootComponent->RelativeLocation = RootComponent->RelativeLocation;
		DefaultRootComponent->RelativeRotation = RootComponent->RelativeRotation;
		DefaultRootComponent->RelativeScale3D = RootComponent->RelativeScale3D;
	}
}

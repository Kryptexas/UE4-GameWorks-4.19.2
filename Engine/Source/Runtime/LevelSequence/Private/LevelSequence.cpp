// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "LevelSequencePCH.h"
#include "LevelSequence.h"
#include "LevelSequenceObject.h"
#include "MovieScene.h"
#include "MovieSceneCommonHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogLevelSequence, Log, All);

ULevelSequence::ULevelSequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MovieScene(nullptr)
{
}

void ULevelSequence::Initialize()
{
	// @todo sequencer: gmp: fix me
	MovieScene = NewObject<UMovieScene>(this, NAME_None, RF_Transactional);
	MovieScene->SetForceFixedFrameIntervalPlayback( true );
	MovieScene->SetFixedFrameInterval( 1 / 30.0f );
}

void ULevelSequence::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	TSet<FGuid> InvalidSpawnables;

	for (int32 Index = 0; Index < MovieScene->GetSpawnableCount(); ++Index)
	{
		FMovieSceneSpawnable& Spawnable = MovieScene->GetSpawnable(Index);
		if (!Spawnable.GetObjectTemplate())
		{
			if (Spawnable.GeneratedClass_DEPRECATED && Spawnable.GeneratedClass_DEPRECATED->ClassGeneratedBy)
			{
				const FName TemplateName = MakeUniqueObjectName(MovieScene, UObject::StaticClass(), Spawnable.GeneratedClass_DEPRECATED->ClassGeneratedBy->GetFName());

				UObject* NewTemplate = NewObject<UObject>(MovieScene, Spawnable.GeneratedClass_DEPRECATED, TemplateName);
				if (NewTemplate)
				{
					Spawnable.CopyObjectTemplate(*NewTemplate, *MovieScene);
				}
			}
		}

		if (!Spawnable.GetObjectTemplate())
		{
			InvalidSpawnables.Add(Spawnable.GetGuid());
			UE_LOG(LogLevelSequence, Warning, TEXT("Discarding spawnable with ID '%s' since its generated class could not produce to a template actor"), *Spawnable.GetGuid().ToString());
		}
	}

	for (FGuid& ID : InvalidSpawnables)
	{
		MovieScene->RemoveSpawnable(ID);
	}
#endif

	if ( MovieScene->GetFixedFrameInterval() == 0 )
	{
		MovieScene->SetFixedFrameInterval( 1 / 30.0f );
	}
}

void ULevelSequence::ConvertPersistentBindingsToDefault(UObject* FixupContext)
{
	if (PossessedObjects_DEPRECATED.Num() == 0)
	{
		return;
	}

	MarkPackageDirty();
	for (auto& Pair : PossessedObjects_DEPRECATED)
	{
		UObject* Object = Pair.Value.GetObject();
		if (Object)
		{
			FGuid ObjectId;
			FGuid::Parse(Pair.Key, ObjectId);
			ObjectReferences.CreateBinding(ObjectId, Object, FixupContext);
		}
	}
	PossessedObjects_DEPRECATED.Empty();
}

void ULevelSequence::BindPossessableObject(const FGuid& ObjectId, UObject& PossessedObject, UObject* Context)
{
	if (Context)
	{
		ObjectReferences.CreateBinding(ObjectId, &PossessedObject, Context);
	}
}

void ULevelSequence::BindPossessableObject(const FGuid& ObjectId, const FLevelSequenceObjectReference& ObjectReference)
{
	ObjectReferences.CreateBinding(ObjectId, ObjectReference);
}

bool ULevelSequence::CanPossessObject(UObject& Object) const
{
	return Object.IsA<AActor>() || Object.IsA<UActorComponent>();
}

UObject* ULevelSequence::FindPossessableObject(const FGuid& ObjectId, UObject* Context) const
{
	return Context ? ObjectReferences.ResolveBinding(ObjectId, Context) : nullptr;
}

FGuid ULevelSequence::FindPossessableObjectId(UObject& Object) const
{
	return ObjectReferences.FindBindingId(&Object, Object.GetWorld());
}

UMovieScene* ULevelSequence::GetMovieScene() const
{
	return MovieScene;
}

UObject* ULevelSequence::GetParentObject(UObject* Object) const
{
	UActorComponent* Component = Cast<UActorComponent>(Object);

	if (Component != nullptr)
	{
		return Component->GetOwner();
	}

	return nullptr;
}

bool ULevelSequence::AllowsSpawnableObjects() const
{
	return true;
}

void ULevelSequence::UnbindPossessableObjects(const FGuid& ObjectId)
{
	ObjectReferences.RemoveBinding(ObjectId);
}

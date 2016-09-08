// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneVectorTrackInstance.h"
#include "MovieSceneVectorTrack.h"


FMovieSceneVectorTrackInstance::FMovieSceneVectorTrackInstance( UMovieSceneVectorTrack& InVectorTrack )
{
	VectorTrack = &InVectorTrack;
	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( VectorTrack->GetPropertyName(), VectorTrack->GetPropertyPath() ) );
}


void FMovieSceneVectorTrackInstance::SaveState(const TArray<TWeakObjectPtr<UObject>>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	int32 NumChannelsUsed = VectorTrack->GetNumChannelsUsed();
	
	for (auto ObjectPtr : RuntimeObjects)
	{
		UObject* Object = ObjectPtr.Get();

		switch( NumChannelsUsed )
		{
			case 2:
			{
				if (InitVector2DMap.Find(Object) == nullptr)
				{
					FVector2D VectorValue = PropertyBindings->GetCurrentValue<FVector2D>(Object);
					InitVector2DMap.Add(Object, VectorValue);
				}
				break;
			}
			case 3:
			{
				if (InitVector3DMap.Find(Object) == nullptr)
				{
					FVector VectorValue = PropertyBindings->GetCurrentValue<FVector>(Object);
					InitVector3DMap.Add(Object, VectorValue);
				}
				break;
			}
			case 4:
			{
				if (InitVector4DMap.Find(Object) == nullptr)
				{
					FVector4 VectorValue = PropertyBindings->GetCurrentValue<FVector4>(Object);
					InitVector4DMap.Add(Object, VectorValue);
				}
				break;
			}
		}
	}
}


void FMovieSceneVectorTrackInstance::RestoreState(const TArray<TWeakObjectPtr<UObject>>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	int32 NumChannelsUsed = VectorTrack->GetNumChannelsUsed();

	for (auto ObjectPtr : RuntimeObjects)
	{
		UObject* Object = ObjectPtr.Get();

		if (!IsValid(Object))
		{
			continue;
		}

		switch( NumChannelsUsed )
		{
			case 2:
			{
				FVector2D *VectorValue = InitVector2DMap.Find(Object);
				if (VectorValue != nullptr)
				{
					PropertyBindings->CallFunction<FVector2D>(Object, VectorValue);
				}
				break;
			}
			case 3:
			{
				FVector *VectorValue = InitVector3DMap.Find(Object);
				if (VectorValue != nullptr)
				{
					PropertyBindings->CallFunction<FVector2D>(Object, VectorValue);
				}
				break;
			}
			case 4:
			{
				FVector4 *VectorValue = InitVector4DMap.Find(Object);
				if (VectorValue != nullptr)
				{
					PropertyBindings->CallFunction<FVector2D>(Object, VectorValue);
				}
				break;
			}
		}
	}

	PropertyBindings->UpdateBindings( RuntimeObjects );
}


void FMovieSceneVectorTrackInstance::Update(EMovieSceneUpdateData& UpdateData, const TArray<TWeakObjectPtr<UObject>>& RuntimeObjects, class IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	int32 NumChannelsUsed = VectorTrack->GetNumChannelsUsed();
	switch (NumChannelsUsed)
	{
	case 2:
	{
		for (auto ObjectPtr : RuntimeObjects)
		{
			UObject* Object = ObjectPtr.Get();
			if(Object != nullptr)
			{
				FVector2D Vector2D = PropertyBindings->GetCurrentValue<FVector2D>(Object);
				FVector4 Vector4(Vector2D.X, Vector2D.Y);
				if (VectorTrack->Eval(UpdateData.Position, UpdateData.LastPosition, Vector4))
				{
					Vector2D.X = Vector4.X;
					Vector2D.Y = Vector4.Y;
					PropertyBindings->CallFunction<FVector2D>(Object, &Vector2D);
				}
			}
		}
		break;
	}
	case 3:
	{
		for (auto ObjectPtr : RuntimeObjects)
		{
			UObject* Object = ObjectPtr.Get();
			if (Object != nullptr)
			{
				FVector Vector = PropertyBindings->GetCurrentValue<FVector>(Object);
				FVector4 Vector4(Vector.X, Vector.Y, Vector.Z);
				if (VectorTrack->Eval(UpdateData.Position, UpdateData.LastPosition, Vector4))
				{
					Vector.X = Vector4.X;
					Vector.Y = Vector4.Y;
					Vector.Z = Vector4.Z;
					PropertyBindings->CallFunction<FVector>(Object, &Vector);
				}
			}
		}
		break;
	}
	case 4:
	{
		for (auto ObjectPtr : RuntimeObjects)
		{
			UObject* Object = ObjectPtr.Get();
			if (Object != nullptr)
			{
				FVector4 Vector4 = PropertyBindings->GetCurrentValue<FVector4>(Object);
				if (VectorTrack->Eval(UpdateData.Position, UpdateData.LastPosition, Vector4))
				{
					PropertyBindings->CallFunction<FVector4>(Object, &Vector4);
				}
			}
		}
		break;
	}
	default:
		UE_LOG(LogMovieScene, Warning, TEXT("Invalid number of channels(%d) for vector track"), NumChannelsUsed);
		break;
	}
}


void FMovieSceneVectorTrackInstance::RefreshInstance(const TArray<TWeakObjectPtr<UObject>>& RuntimeObjects, class IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	PropertyBindings->UpdateBindings(RuntimeObjects);
}

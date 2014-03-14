// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneVectorTrackInstance.h"
#include "MatineeUtils.h"


FMovieSceneVectorTrackInstance::FMovieSceneVectorTrackInstance( UMovieSceneVectorTrack& InVectorTrack )
{
	VectorTrack = &InVectorTrack;
}

void FMovieSceneVectorTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	FVector4 Vector;
	if( VectorTrack->Eval( Position, LastPosition, Vector ) )
	{
		for( int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex )
		{
			UObject* Object = RuntimeObjects[ObjIndex];

			UObject* PropertyOwner = NULL;
			UProperty* Property = NULL;
			//@todo Sequencer - Major performance problems here.  This needs to be initialized and stored (not serialized) somewhere
			void* Address = FMatineeUtils::GetPropertyAddress<void>( Object, VectorTrack->GetPropertyName(), Property, PropertyOwner );
		
			if (Address)
			{
				bool bIsVector2D = false, bIsVector = false, bIsVector4 = false;
				const UStructProperty* StructProp = Cast<const UStructProperty>(Property);
				check(StructProp && StructProp->Struct);
				FName StructName = StructProp->Struct->GetFName();

				bIsVector2D = StructName == NAME_Vector2D;
				bIsVector = StructName == NAME_Vector;
				bIsVector4 = StructName == NAME_Vector4;
				check(bIsVector2D || bIsVector || bIsVector4);

				if (bIsVector2D)
				{
					*(FVector2D*)Address = FVector2D(Vector.X, Vector.Y);
				}
				else if (bIsVector)
				{
					*(FVector*)Address = FVector(Vector.X, Vector.Y, Vector.Z);
				}
				else if (bIsVector4)
				{
					*(FVector4*)Address = Vector;
				}
				// Let the property owner know that we changed one of its properties
				PropertyOwner->PostInterpChange( Property );
			}
		}
	}
}


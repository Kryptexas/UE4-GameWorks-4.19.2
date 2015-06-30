// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieScenePrivatePCH.h"
#include "MovieSceneBindings.h"

FMovieSceneBoundObject::FMovieSceneBoundObject(const FGuid& InitPosessableGuid, const TArray< FMovieSceneBoundObjectInfo >& InitObjectInfos)
	: PossessableGuid( InitPosessableGuid ),
	  ObjectInfos( InitObjectInfos )
{
}



UMovieSceneBindings::UMovieSceneBindings( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{

	RootMovieScene = NULL;
}



UMovieScene* UMovieSceneBindings::GetRootMovieScene()
{
	return RootMovieScene;
}


void UMovieSceneBindings::SetRootMovieScene( class UMovieScene* NewMovieScene )
{
	if( NewMovieScene != RootMovieScene )
	{
		Modify();
		RootMovieScene = NewMovieScene;
	}
}


FMovieSceneBoundObject& UMovieSceneBindings::AddBinding(const FGuid& PosessableGuid, const TArray< FMovieSceneBoundObjectInfo >& ObjectInfos)
{
	Modify();
	BoundObjects.Add( FMovieSceneBoundObject( PosessableGuid, ObjectInfos ) );
	return BoundObjects.Last();
}


TArray< FMovieSceneBoundObjectInfo > UMovieSceneBindings::FindBoundObjects( const FGuid& Guid ) const
{
	for( TArray< FMovieSceneBoundObject >::TConstIterator BoundObjectIter( BoundObjects.CreateConstIterator() ); BoundObjectIter; ++BoundObjectIter )
	{
		const FMovieSceneBoundObject& BoundObject = *BoundObjectIter;

		if( BoundObject.GetPossessableGuid() == Guid )
		{
			return BoundObject.GetObjectInfos();
		}
	}

	return TArray< FMovieSceneBoundObjectInfo >();
}


TArray< FMovieSceneBoundObject >& UMovieSceneBindings::GetBoundObjects()
{
	return BoundObjects;
}

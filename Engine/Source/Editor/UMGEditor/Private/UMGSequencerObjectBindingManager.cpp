// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "UMGSequencerObjectBindingManager.h"

FUMGSequencerObjectBindingManager::FUMGSequencerObjectBindingManager( UWidgetBlueprint& InWidgetBlueprint )
	: WidgetBlueprint( &InWidgetBlueprint )
{
	
}

FGuid FUMGSequencerObjectBindingManager::FindGuidForObject( const UMovieScene& MovieScene, UObject& Object ) const
{
	return PreviewObjectToGuidMap.FindRef( &Object );
}


bool FUMGSequencerObjectBindingManager::CanPossessObject( UObject& Object ) const
{
	return !Object.IsIn( WidgetBlueprint.Get() );
}

void FUMGSequencerObjectBindingManager::BindPossessableObject( const FGuid& PossessableGuid, UObject& PossessedObject )
{
	PreviewObjectToGuidMap.Add( &PossessedObject, PossessableGuid );
}

void FUMGSequencerObjectBindingManager::UnbindPossessableObjects( const FGuid& PossessableGuid )
{
	for( auto It = PreviewObjectToGuidMap.CreateIterator(); It; ++It )
	{
		if( It.Value() == PossessableGuid )
		{
			It.RemoveCurrent();
		}
	}
}

void FUMGSequencerObjectBindingManager::GetRuntimeObjects( const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, TArray<UObject*>& OutRuntimeObjects ) const
{
	for( auto It = PreviewObjectToGuidMap.CreateConstIterator(); It; ++It )
	{
		UObject* Object = It.Key().Get();
		if( Object )
		{
			OutRuntimeObjects.Add( Object );
		}
	}
}

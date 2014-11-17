// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "UMGSequencerObjectBindingManager.h"
#include "WidgetBlueprintEditor.h"
#include "ISequencer.h"

FUMGSequencerObjectBindingManager::FUMGSequencerObjectBindingManager( FWidgetBlueprintEditor& InWidgetBlueprintEditor )
	: WidgetBlueprintEditor( InWidgetBlueprintEditor )
{
	WidgetBlueprintEditor.GetOnWidgetPreviewUpdated().AddRaw( this, &FUMGSequencerObjectBindingManager::OnWidgetPreviewUpdated );
}

FUMGSequencerObjectBindingManager::~FUMGSequencerObjectBindingManager()
{
}

FGuid FUMGSequencerObjectBindingManager::FindGuidForObject( const UMovieScene& MovieScene, UObject& Object ) const
{
	return PreviewObjectToGuidMap.FindRef( &Object );
}

bool FUMGSequencerObjectBindingManager::CanPossessObject( UObject& Object ) const
{
	UWidgetBlueprint* WidgetBlueprint = WidgetBlueprintEditor.GetWidgetBlueprintObj();

	return !Object.IsIn( WidgetBlueprint );
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

void FUMGSequencerObjectBindingManager::OnWidgetPreviewUpdated()
{
	//
	UUserWidget* PreviewWidget = WidgetBlueprintEditor.GetPreview();

	UWidgetTree* WidgetTree = PreviewWidget->WidgetTree;

	TMap< TWeakObjectPtr<UObject>, FGuid> OldPreviewObjectToGuidMap = PreviewObjectToGuidMap;
	PreviewObjectToGuidMap.Empty();

	for( auto It = OldPreviewObjectToGuidMap.CreateConstIterator(); It; ++It )
	{
		const TWeakObjectPtr<UObject> Object = It.Key();
		if(Object.IsValid())
		{
			FName ObjectName = Object->GetFName();
			UObject* NewObject = FindObject<UObject>( WidgetTree, *ObjectName.ToString() );
			if( NewObject )
			{
				check( NewObject->GetClass() == Object->GetClass() );
				PreviewObjectToGuidMap.Add( NewObject, It.Value() );
			}
		}
	}

	WidgetBlueprintEditor.GetSequencer()->UpdateRuntimeInstances();
}
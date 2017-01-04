// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "EditorWorldExtension.h"
#include "EngineGlobals.h"
#include "Editor.h"

/************************************************************************/
/* UEditorExtension		                                                */
/************************************************************************/
UEditorWorldExtension::UEditorWorldExtension() :
	Super(),
	OwningWorld( nullptr )
{

}

UEditorWorldExtension::~UEditorWorldExtension()
{
	OwningWorld = nullptr;
}

bool UEditorWorldExtension::InputKey( FEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event )
{
	return false;
}

bool UEditorWorldExtension::InputAxis( FEditorViewportClient* InViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime )
{
	return false;
}

UWorld* UEditorWorldExtension::GetWorld() const
{
	return OwningWorld;
}

void UEditorWorldExtension::SetWorld( UWorld* InitWorld )
{
	OwningWorld = InitWorld;
}

/************************************************************************/
/* FEditorWorldExtensionCollection                                      */
/************************************************************************/
FEditorWorldExtensionCollection::FEditorWorldExtensionCollection( FWorldContext& InWorldContext ) :
	WorldContext ( InWorldContext )
{

}

FEditorWorldExtensionCollection::~FEditorWorldExtensionCollection()
{
	EditorExtensions.Empty();
}

UWorld* FEditorWorldExtensionCollection::GetWorld() const
{
	return WorldContext.World();
}

FWorldContext& FEditorWorldExtensionCollection::GetWorldContext() const
{
	return WorldContext;
}

UEditorWorldExtension* FEditorWorldExtensionCollection::AddExtension( TSubclassOf<UEditorWorldExtension> EditorExtensionClass )
{
	UEditorWorldExtension* CreatedExtension = EditorExtensionClass.GetDefaultObject();
	AddExtension( CreatedExtension );
	return CreatedExtension;
}

void FEditorWorldExtensionCollection::AddExtension( UEditorWorldExtension* EditorExtension )
{
	check( EditorExtension != nullptr );

	const int32 ExistingExtensionIndex = EditorExtensions.IndexOfByPredicate(
		[ &EditorExtension ]( const FEditorExtensionTuple& Element ) -> bool
		{ 
			return Element.Get<0>() == EditorExtension;
		} 
	);

	if( ExistingExtensionIndex != INDEX_NONE )
	{
		FEditorExtensionTuple& EditorExtensionTuple = EditorExtensions[ ExistingExtensionIndex ];
		int32& RefCount = EditorExtensionTuple.Get<1>();
		++RefCount;
	}
	else
	{
		const int32 InitialRefCount = 1;
		EditorExtensions.Add( FEditorExtensionTuple( EditorExtension, InitialRefCount ) );

		EditorExtension->SetWorld( GetWorld() );
		EditorExtension->Init();
	}
}

void FEditorWorldExtensionCollection::RemoveExtension( UEditorWorldExtension* EditorExtension )
{
	check( EditorExtension != nullptr );

	const int32 ExistingExtensionIndex = EditorExtensions.IndexOfByPredicate(
		[ &EditorExtension ]( const FEditorExtensionTuple& Element ) -> bool 
		{ 
			return Element.Get<0>() == EditorExtension;
		} 
	);
	if( ensure( ExistingExtensionIndex != INDEX_NONE ) )
	{
		FEditorExtensionTuple& EditorExtensionTuple = EditorExtensions[ ExistingExtensionIndex ];
		int32& RefCount = EditorExtensionTuple.Get<1>();
		--RefCount;

		if( RefCount == 0 )
		{
			EditorExtensions.RemoveAt( ExistingExtensionIndex );
			EditorExtension->Shutdown();
		}
	}
}

UEditorWorldExtension* FEditorWorldExtensionCollection::FindExtension( TSubclassOf<UEditorWorldExtension> EditorExtensionClass )
{
	UEditorWorldExtension* ResultExtension = nullptr;
	for( FEditorExtensionTuple& EditorExtensionTuple : EditorExtensions )
	{
		UEditorWorldExtension* EditorExtension = EditorExtensionTuple.Get<0>();
		if( EditorExtension->GetClass() == EditorExtensionClass )
		{
			ResultExtension = EditorExtension;
			break;
		}
	}

	return ResultExtension;
}

void FEditorWorldExtensionCollection::Tick( float DeltaSeconds )
{
	for( FEditorExtensionTuple& EditorExtensionTuple : EditorExtensions )
	{
		UEditorWorldExtension* EditorExtension = EditorExtensionTuple.Get<0>();
		EditorExtension->Tick( DeltaSeconds );
	}
}

bool FEditorWorldExtensionCollection::InputKey( FEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event )
{
	bool bHandled = false;

	for( FEditorExtensionTuple& EditorExtensionTuple : EditorExtensions )
	{
		UEditorWorldExtension* EditorExtension = EditorExtensionTuple.Get<0>();
		bHandled |= EditorExtension->InputKey( InViewportClient, Viewport, Key, Event );
	}

	return bHandled;
}

bool FEditorWorldExtensionCollection::InputAxis( FEditorViewportClient* InViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime )
{
	bool bHandled = false;

	for( FEditorExtensionTuple& EditorExtensionTuple : EditorExtensions )
	{
		UEditorWorldExtension* EditorExtension = EditorExtensionTuple.Get<0>();
		bHandled |= EditorExtension->InputAxis( InViewportClient, Viewport, ControllerId, Key, Delta, DeltaTime );
	}

	return bHandled;
}

void FEditorWorldExtensionCollection::AddReferencedObjects( FReferenceCollector& Collector )
{
	for( FEditorExtensionTuple& EditorExtensionTuple : EditorExtensions )
	{
		UEditorWorldExtension* EditorExtension = EditorExtensionTuple.Get<0>();
		Collector.AddReferencedObject( EditorExtension );
	}
}

/************************************************************************/
/* FEditorWorldExtensionManager                                         */
/************************************************************************/
FEditorWorldExtensionManager::FEditorWorldExtensionManager()
{
	if( GEngine )
	{
		GEngine->OnWorldContextDestroyed().AddRaw( this, &FEditorWorldExtensionManager::OnWorldContextRemove );
	}
}

FEditorWorldExtensionManager::~FEditorWorldExtensionManager()
{
	if( GEngine )
	{
		GEngine->OnWorldContextDestroyed().RemoveAll( this );
	}

	EditorWorldExtensionCollection.Empty();
}

TSharedPtr<FEditorWorldExtensionCollection> FEditorWorldExtensionManager::GetEditorWorldExtensions( const UWorld* InWorld )
{
	// Try to find this world in the map and return it or create and add one if nothing found
	TSharedPtr<FEditorWorldExtensionCollection> Result;
	if( InWorld )
	{
		TSharedPtr<FEditorWorldExtensionCollection>* FoundWorld = EditorWorldExtensionCollection.Find( InWorld->GetUniqueID() );
		if( FoundWorld != nullptr )
		{
			Result = *FoundWorld;
		}
		else
		{
			FWorldContext* WorldContext = GEditor->GetWorldContextFromWorld( InWorld );
			Result = OnWorldContextAdd( *WorldContext );
		}
	}
	return Result;
}

TSharedPtr<FEditorWorldExtensionCollection> FEditorWorldExtensionManager::OnWorldContextAdd( FWorldContext& InWorldContext )
{
	//Only add editor type world to the map
	UWorld* World = InWorldContext.World();
	TSharedPtr<FEditorWorldExtensionCollection> Result;
	if( World != nullptr )
	{
		TSharedPtr<FEditorWorldExtensionCollection> ExtensionCollection( new FEditorWorldExtensionCollection( InWorldContext ) );
		Result = ExtensionCollection;
		EditorWorldExtensionCollection.Add( World->GetUniqueID(), Result );
	}
	return Result;
}

void FEditorWorldExtensionManager::OnWorldContextRemove( FWorldContext& InWorldContext )
{
	UWorld* World = InWorldContext.World();
	if(World)
	{
		EditorWorldExtensionCollection.Remove( World->GetUniqueID() );
	}
}

void FEditorWorldExtensionManager::Tick( float DeltaSeconds )
{
	// Tick all the collections
	for( auto& Collection : EditorWorldExtensionCollection )
	{
		FEditorWorldExtensionCollection& ExtensionCollection = *Collection.Value;
		ExtensionCollection.Tick( DeltaSeconds );
	}
}
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LayersPrivatePCH.h"

#define LOCTEXT_NAMESPACE "Layer"

FLayerViewModel::FLayerViewModel( const TWeakObjectPtr< ULayer >& InLayer, const TSharedRef< ILayers >& InWorldLayers, const TWeakObjectPtr< UEditorEngine >& InEditor )
	: WorldLayers( InWorldLayers )
	, Layer( InLayer )
	, Editor( InEditor )
{

}


void FLayerViewModel::Initialize()
{
	WorldLayers->OnLayersChanged().AddSP( this, &FLayerViewModel::OnLayersChanged );

	if ( Editor.IsValid() )
	{
		Editor->RegisterForUndo(this);
	}

	RefreshActorStats();
}


FLayerViewModel::~FLayerViewModel()
{
	WorldLayers->OnLayersChanged().RemoveAll( this );

	if ( Editor.IsValid() )
	{
		Editor->UnregisterForUndo(this);
	}
}


FName FLayerViewModel::GetFName() const
{
	if( !Layer.IsValid() )
	{
		return NAME_None;
	}

	return Layer->LayerName;
}


FString FLayerViewModel::GetName() const
{
	FString name;
	if( Layer.IsValid() )
	{
		name = Layer->LayerName.ToString();
	}
	return name;
}

FText FLayerViewModel::GetNameAsText() const
{
	if( !Layer.IsValid() )
	{
		return FText::GetEmpty();
	}

	return FText::FromName(Layer->LayerName);
}


bool FLayerViewModel::IsVisible() const
{
	if( !Layer.IsValid() )
	{
		return false;
	}

	return Layer->bIsVisible;
}


void FLayerViewModel::ToggleVisibility()
{
	if( !Layer.IsValid() )
	{
		return;
	}

	const FScopedTransaction Transaction( LOCTEXT("ToggleVisibility", "Toggle Layer Visibility") );
	WorldLayers->ToggleLayerVisibility( Layer->LayerName );
}


void FLayerViewModel::RenameTo( const FString& NewLayerName )
{
	if( !Layer.IsValid() )
	{
		return;
	}

	const FName NewLayerFName = FName( *NewLayerName );

	if( Layer->LayerName == NewLayerFName)
	{
		return;
	}

	int32 LayerIndex = 0;
	FName UniqueNewLayerName = NewLayerFName;
	TWeakObjectPtr< ULayer > FoundLayer;
	while( WorldLayers->TryGetLayer( UniqueNewLayerName, FoundLayer ) )
	{
		UniqueNewLayerName = FName( *FString::Printf( TEXT( "%s_%d" ), *NewLayerFName.ToString(), ++LayerIndex ) );
	}

	const FScopedTransaction Transaction( LOCTEXT("RenameTo", "Rename Layer") );

	WorldLayers->RenameLayer( Layer->LayerName, UniqueNewLayerName );
}


bool FLayerViewModel::CanAssignActors( const TArray< TWeakObjectPtr<AActor> > Actors, FString& OutMessage ) const
{
	if( !Layer.IsValid() )
	{
		OutMessage = LOCTEXT("InvalidLayer", "Invalid Layer").ToString();
		return false;
	}

	bool bHasValidActorToAssign = false;
	int32 bAlreadyAssignedActors = 0;
	for( auto ActorIt = Actors.CreateConstIterator(); ActorIt; ++ActorIt )
	{
		TWeakObjectPtr< AActor > Actor = *ActorIt;
		if( !Actor.IsValid() )
		{
			OutMessage = FString::Printf( *LOCTEXT("InvalidActors", "Cannot add invalid Actors to %s").ToString(), *Layer->LayerName.ToString() );
			return false;
		}

		if( !WorldLayers->IsActorValidForLayer( Actor ) )
		{
			OutMessage = FString::Printf( *LOCTEXT("InvalidLayers", "Actor '%s' cannot be associated with Layers").ToString(), *Actor->GetFName().ToString() );
			return false;
		}

		if( Actor->Layers.Contains( Layer->LayerName ) )
		{
			bAlreadyAssignedActors++;
		}
		else
		{
			bHasValidActorToAssign = true;
		}
	}

	if( bAlreadyAssignedActors == Actors.Num() )
	{
		OutMessage = FString::Printf( *LOCTEXT("AlreadyAssignedActors", "All Actors already assigned to %s").ToString(), *Layer->LayerName.ToString() );
		return false;
	}

	if( bHasValidActorToAssign )
	{
		OutMessage = FString::Printf( *LOCTEXT("AssignActors", "Assign Actors to %s").ToString(), *Layer->LayerName.ToString() );
		return true;
	}

	OutMessage.Empty();
	return false;
}


bool FLayerViewModel::CanAssignActor( const TWeakObjectPtr<AActor> Actor, FString& OutMessage ) const
{
	if( !Layer.IsValid() )
	{
		OutMessage = LOCTEXT("InvalidLayer", "Invalid Layer").ToString();
		return false;
	}

	if( !Actor.IsValid() )
	{
		OutMessage = FString::Printf( *LOCTEXT("InvalidActor", "Cannot add invalid Actors to %s").ToString(), *Layer->LayerName.ToString() );
		return false;
	}

	if( !WorldLayers->IsActorValidForLayer( Actor ) )
	{
		OutMessage = FString::Printf(* LOCTEXT("InvalidLayers", "Actor '%s' cannot be associated with Layers").ToString(), *Actor->GetFName().ToString() );
		return false;
	}

	if( Actor->Layers.Contains( Layer->LayerName )  )
	{
		OutMessage = FString::Printf( *LOCTEXT("AlreadyAssignedActor", "Already assigned to %s").ToString(), *Layer->LayerName.ToString() );
		return false;
	}

	OutMessage = FString::Printf( *LOCTEXT("AssignActor", "Assign to %s").ToString(), *Layer->LayerName.ToString() );
	return true;
}


void FLayerViewModel::AppendActors( TArray< TWeakObjectPtr< AActor > >& InActors ) const
{
	if( !Layer.IsValid() )
	{
		return;
	}

	WorldLayers->AppendActorsForLayer( Layer->LayerName, InActors );
}


void FLayerViewModel::AppendActorsOfSpecificType( TArray< TWeakObjectPtr< AActor > >& InActors, const TWeakObjectPtr< UClass >& Class )
{
	if( !Layer.IsValid() )
	{
		return;
	}

	struct Local
	{
		static bool ActorIsOfClass( const TWeakObjectPtr< AActor >& Actor, const TWeakObjectPtr< UClass > Class )
		{
			return Actor->GetClass() == Class.Get();
		}
	};

	TSharedRef< TDelegateFilter< const TWeakObjectPtr< AActor >& > > Filter = MakeShareable( new TDelegateFilter< const TWeakObjectPtr< AActor >& >( TDelegateFilter< const TWeakObjectPtr< AActor >& >::FPredicate::CreateStatic( &Local::ActorIsOfClass, Class ) ) );
	WorldLayers->AppendActorsForLayer( Layer->LayerName, InActors, Filter );
}


void FLayerViewModel::AddActor( const TWeakObjectPtr< AActor >& Actor )
{
	if( !Layer.IsValid() )
	{
		return;
	}

	const FScopedTransaction Transaction( LOCTEXT("AddActor", "Add Actor to Layer") );
	WorldLayers->AddActorToLayer( Actor, Layer->LayerName );
}


void FLayerViewModel::AddActors( const TArray< TWeakObjectPtr< AActor > >& Actors )
{
	if( !Layer.IsValid() )
	{
		return;
	}

	const FScopedTransaction Transaction( LOCTEXT("AddActors", "Add Actors to Layer") );
	WorldLayers->AddActorsToLayer( Actors, Layer->LayerName );
}


void FLayerViewModel::RemoveActors( const TArray< TWeakObjectPtr< AActor > >& Actors )
{
	if( !Layer.IsValid() )
	{
		return;
	}

	const FScopedTransaction Transaction( LOCTEXT("RemoveActors", "Remove Actors from Layer") );
	WorldLayers->RemoveActorsFromLayer( Actors, Layer->LayerName );
}


void FLayerViewModel::RemoveActor( const TWeakObjectPtr< AActor >& Actor )
{
	if( !Layer.IsValid() )
	{
		return;
	}

	const FScopedTransaction Transaction( LOCTEXT("RemoveActor", "Remove Actor from Layer") );
	WorldLayers->RemoveActorFromLayer( Actor, Layer->LayerName );
}


void FLayerViewModel::SelectActors( bool bSelect, bool bNotify, bool bSelectEvenIfHidden, const TSharedPtr< IFilter< const TWeakObjectPtr< AActor >& > >& Filter )
{
	if( !Layer.IsValid() )
	{
		return;
	}

	const FScopedTransaction Transaction( LOCTEXT("SelectActors", "Select Actors in Layer") );

	const bool bNotifySelectNone = false;
	const bool bDeselectBSPSurfs = true;
	Editor->SelectNone( bNotifySelectNone, bDeselectBSPSurfs );

	WorldLayers->SelectActorsInLayer( Layer->LayerName, bSelect, bNotify, bSelectEvenIfHidden, Filter );
}


FString FLayerViewModel::GetActorStatTotal( int32 StatsIndex  ) const
{
	if( !Layer.IsValid() )
	{
		return TEXT( "0" );
	}

	if( ActorStats.Num() <= StatsIndex )
	{
		return LOCTEXT("InvalidActorStatTotal", "Invalid").ToString();
	}

	return FString::Printf( TEXT( "%d" ), ActorStats[ StatsIndex ].Total );
}


void FLayerViewModel::SelectActorsOfSpecificType( const TWeakObjectPtr< UClass >& Class )
{
	if( !Layer.IsValid() )
	{
		return;
	}

	struct Local
	{
		static bool ActorIsOfClass( const TWeakObjectPtr< AActor >& Actor, const TWeakObjectPtr< UClass > Class )
		{
			return Actor->GetClass() == Class.Get();
		}
	};

	const bool bSelect = true;
	const bool bNotify = true; 
	const bool bSelectEvenIfHidden = true;
	TSharedRef< TDelegateFilter< const TWeakObjectPtr< AActor >& > > Filter = MakeShareable( new TDelegateFilter< const TWeakObjectPtr< AActor >& >( TDelegateFilter< const TWeakObjectPtr< AActor >& >::FPredicate::CreateStatic( &Local::ActorIsOfClass, Class ) ) );
	SelectActors( bSelect, bNotify, bSelectEvenIfHidden, Filter );
}


const TArray< FLayerActorStats >& FLayerViewModel::GetActorStats() const
{
	return ActorStats;
}


void FLayerViewModel::SetDataSource( const TWeakObjectPtr< ULayer >& InLayer )
{
	if( Layer == InLayer )
	{
		return;
	}

	Layer = InLayer;
	Refresh();
}


const TWeakObjectPtr< ULayer > FLayerViewModel::GetDataSource()
{
	return Layer;
}


void FLayerViewModel::OnLayersChanged( const ELayersAction::Type Action, const TWeakObjectPtr< ULayer >& ChangedLayer, const FName& ChangedProperty )
{
	if( Action != ELayersAction::Modify && Action != ELayersAction::Reset )
	{
		return;
	}

	if( ChangedLayer.IsValid() && ChangedLayer != Layer )
	{
		return;
	}

	if( Action == ELayersAction::Reset || ChangedProperty == TEXT( "ActorStats" ) )
	{
		RefreshActorStats();
	}

	ChangedEvent.Broadcast();
}


void FLayerViewModel::RefreshActorStats()
{
	ActorStats.Empty();

	if( !Layer.IsValid() )
	{
		return;
	}

	ActorStats.Append( Layer->ActorStats );

	struct FCompareLayerActorStats
	{
		FORCEINLINE bool operator()( const FLayerActorStats& Lhs, const FLayerActorStats& Rhs ) const 
		{ 
			int32 Result = Lhs.Type->GetFName().Compare( Rhs.Type->GetFName() );
			return Result > 0; 
		}
	};

	ActorStats.Sort( FCompareLayerActorStats() );
}


void FLayerViewModel::Refresh()
{
	OnLayersChanged( ELayersAction::Reset, NULL, NAME_None );
}


#undef LOCTEXT_NAMESPACE

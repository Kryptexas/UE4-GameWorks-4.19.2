// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Layers/ILayers.h"

class FLayers : public ILayers
{

public:

	/**
	 *	Creates a new FLayers
	 */
	static TSharedRef< FLayers > Create( const TWeakObjectPtr< UEditorEngine >& InEditor )
	{
		TSharedRef< FLayers > Layers = MakeShareable( new FLayers( InEditor ) );
		Layers->Initialize();
		return Layers;
	}

	/**
	 *	Destructor
	 */
	~FLayers();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Begin ILayer Implementation -- See ILayers for documentation

	DECLARE_DERIVED_EVENT( FLayers, ILayers::FOnLayersChanged, FOnLayersChanged );
	virtual FOnLayersChanged& OnLayersChanged() OVERRIDE { return LayersChanged; }

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Operations on Levels

	virtual void AddLevelLayerInformation( const TWeakObjectPtr< ULevel >& Level ) OVERRIDE;
	virtual void RemoveLevelLayerInformation( const TWeakObjectPtr< ULevel >& Level ) OVERRIDE;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Operations on an individual actor.

	virtual bool IsActorValidForLayer( const TWeakObjectPtr< AActor >& Actor ) OVERRIDE;
	
	virtual bool InitializeNewActorLayers( const TWeakObjectPtr< AActor >& Actor ) OVERRIDE;
	virtual bool DisassociateActorFromLayers( const TWeakObjectPtr< AActor >& Actor ) OVERRIDE;

	virtual bool AddActorToLayer( const TWeakObjectPtr< AActor >& Actor, const FName& LayerName ) OVERRIDE;
	virtual bool AddActorToLayers( const TWeakObjectPtr< AActor >& Actor, const TArray< FName >& LayerNames ) OVERRIDE;
	
	virtual bool RemoveActorFromLayer( const TWeakObjectPtr< AActor >& Actor, const FName& LayerToRemove, const bool bUpdateStats = true ) OVERRIDE;
	virtual bool RemoveActorFromLayers( const TWeakObjectPtr< AActor >& Actor, const TArray< FName >& LayerNames, const bool bUpdateStats = true  ) OVERRIDE;


	/////////////////////////////////////////////////
	// Operations on multiple actors.
	
	virtual bool AddActorsToLayer( const TArray< TWeakObjectPtr< AActor > >& Actors, const FName& LayerName ) OVERRIDE;
	virtual bool AddActorsToLayers( const TArray< TWeakObjectPtr< AActor > >& Actors, const TArray< FName >& LayerNames ) OVERRIDE;
	
	virtual bool RemoveActorsFromLayer( const TArray< TWeakObjectPtr< AActor > >& Actors, const FName& LayerName, const bool bUpdateStats = true ) OVERRIDE;
	virtual bool RemoveActorsFromLayers( const TArray< TWeakObjectPtr< AActor > >& Actors, const TArray< FName >& LayerNames, const bool bUpdateStats = true ) OVERRIDE;


	/////////////////////////////////////////////////
	// Operations on selected actors.
	TArray< TWeakObjectPtr< AActor > > GetSelectedActors() const;

	virtual bool AddSelectedActorsToLayer( const FName& LayerName ) OVERRIDE;
	virtual bool AddSelectedActorsToLayers( const TArray< FName >& LayerNames ) OVERRIDE;
	
	virtual bool RemoveSelectedActorsFromLayer( const FName& LayerName ) OVERRIDE;
	virtual bool RemoveSelectedActorsFromLayers( const TArray< FName >& LayerNames ) OVERRIDE;


	/////////////////////////////////////////////////
	// Operations on actors in layers

	virtual bool SelectActorsInLayers( const TArray< FName >& LayerNames, bool bSelect, bool bNotify, bool bSelectEvenIfHidden = false, const TSharedPtr< ActorFilter >& Filter = TSharedPtr< ActorFilter >( NULL ) ) OVERRIDE;
	virtual bool SelectActorsInLayer( const FName& LayerName, bool bSelect, bool bNotify, bool bSelectEvenIfHidden = false, const TSharedPtr< ActorFilter >& Filter = TSharedPtr< ActorFilter >( NULL ) ) OVERRIDE;


	/////////////////////////////////////////////////
	// Operations on actor viewport visibility regarding layers

	virtual void UpdateAllViewVisibility( const FName& LayerThatChanged ) OVERRIDE;
	virtual void UpdatePerViewVisibility( FLevelEditorViewportClient* ViewportClient, const FName& LayerThatChanged=NAME_Skip ) OVERRIDE;
	
	virtual void UpdateActorViewVisibility( FLevelEditorViewportClient* ViewportClient, const TWeakObjectPtr< AActor >& Actor, bool bReregisterIfDirty=true ) OVERRIDE;
	virtual void UpdateActorAllViewsVisibility( const TWeakObjectPtr< AActor >& Actor ) OVERRIDE;
	
	virtual void RemoveViewFromActorViewVisibility( FLevelEditorViewportClient* ViewportClient ) OVERRIDE;
	
	virtual bool UpdateActorVisibility( const TWeakObjectPtr< AActor >& Actor, bool& bOutSelectionChanged, bool& bOutActorModified, bool bNotifySelectionChange, bool bRedrawViewports ) OVERRIDE;
	virtual bool UpdateAllActorsVisibility( bool bNotifySelectionChange, bool bRedrawViewports ) OVERRIDE;


	/////////////////////////////////////////////////
	// Operations on layers

	virtual void AppendActorsForLayer( const FName& LayerName, TArray< TWeakObjectPtr< AActor > >& InActors, const TSharedPtr< ActorFilter >& Filter = TSharedPtr< ActorFilter >( NULL )  ) const OVERRIDE;
	virtual void AppendActorsForLayers( const TArray< FName >& LayerNames, TArray< TWeakObjectPtr< AActor > >& OutActors, const TSharedPtr< ActorFilter >& Filter = TSharedPtr< ActorFilter >( NULL )  ) const OVERRIDE;

	virtual void SetLayerVisibility( const FName& LayerName, bool bIsVisible ) OVERRIDE;
	virtual void SetLayersVisibility( const TArray< FName >& LayerNames, bool bIsVisible ) OVERRIDE;

	virtual void ToggleLayerVisibility( const FName& LayerName ) OVERRIDE;
	virtual void ToggleLayersVisibility( const TArray< FName >& LayerNames ) OVERRIDE;

	virtual void MakeAllLayersVisible() OVERRIDE;

	virtual TWeakObjectPtr< ULayer > GetLayer( const FName& LayerName ) const OVERRIDE;
	virtual bool TryGetLayer( const FName& LayerName, TWeakObjectPtr< ULayer >& OutLayer ) OVERRIDE;

	virtual void AddAllLayerNamesTo( TArray< FName >& OutLayerNames ) const OVERRIDE;
	virtual void AddAllLayersTo( TArray< TWeakObjectPtr< ULayer > >& OutLayers ) const OVERRIDE;

	virtual TWeakObjectPtr< ULayer > CreateLayer( const FName& LayerName ) OVERRIDE;

	virtual void DeleteLayers( const TArray< FName >& LayersToDelete ) OVERRIDE;
	virtual void DeleteLayer( const FName& LayerToDelete ) OVERRIDE;

	virtual bool RenameLayer( const FName OriginalLayerName, const FName& NewLayerName ) OVERRIDE;


	UWorld* GetWorld() const { return GWorld;} // Fallback to GWorld
protected:

	void AddActorToStats( const TWeakObjectPtr< ULayer >& Layer, const TWeakObjectPtr< AActor >& Actor );
	void RemoveActorFromStats( const TWeakObjectPtr< ULayer >& Layer, const TWeakObjectPtr< AActor >& Actor );

	//	End ILayer Implementation
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:

	/** 
	 * Delegate handlers
	 **/ 
	void OnEditorMapChange(uint32 MapChangeFlags);

	/**
	 *	FLayers Constructor
	 *
	 *	@param	InEditor	The UEditorEngine to affect
	 */
	FLayers( const TWeakObjectPtr< UEditorEngine >& InEditor );

	/**
	 *	Prepares for use
	 */
	void Initialize();

	/** 
	 *	Checks to see if the named layer exists, and if it doesn't creates it.
	 *
	 * @param	LayerName	A valid layer name
	 * @return				The ULayer Object of the named layer
	 */
	TWeakObjectPtr< ULayer > EnsureLayerExists( const FName& LayerName );


private:

	/** The associated UEditorEngine */
	TWeakObjectPtr< UEditorEngine > Editor;

	/**	Fires whenever one or more layer changes */
	FOnLayersChanged LayersChanged;
};

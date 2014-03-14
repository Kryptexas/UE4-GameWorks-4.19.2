// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class IPropertyHandle;

/**
 * Listens for changes objects and calls delegates when those objects change
 */
class FSequencerObjectChangeListener : public ISequencerObjectChangeListener
{
public:
	FSequencerObjectChangeListener( TSharedRef<ISequencer> InSequencer );
	~FSequencerObjectChangeListener();

	/** ISequencerObjectChangeListener interface */
	virtual FOnAnimatablePropertyChanged& GetOnAnimatablePropertyChanged( TSubclassOf<UProperty> PropertyClass ) OVERRIDE;
	virtual FOnPropagateObjectChanges& GetOnPropagateObjectChanges() OVERRIDE;
	virtual void TriggerAllPropertiesChanged(UObject* Object) OVERRIDE;

private:
	/**
	 * Called when PreEditChange is called on an object
	 *
	 * @param Object	The object that PreEditChange was called on
	 */
	void OnObjectPreEditChange( UObject* Object );

	/**
	 * Called when PostEditChange is called on an object
	 *
	 * @param Object	The object that PostEditChange was called on
	 */
	void OnObjectPostEditChange( UObject* Object );

	/**
	 * Called after an actor is moved (PostEditMove)
	 *
	 * @param Actor The actor that PostEditMove was called on
	 */
	void OnActorPostEditMove( AActor* Actor );

	/**
	 * Called when a property changes on an object that is being observed
	 *
	 * @param Object	The object that PostEditChange was called on
	 */
	void OnPropertyChanged( const TArray<UObject*>& ChangedObjects, const TSharedRef< const IPropertyHandle> PropertyValue, bool bRequireAutoKey );

private:
	/** Mapping of object to a listener used to check for property changes */
	TMap< TWeakObjectPtr<UObject>, TSharedPtr<class IPropertyChangeListener> > ActivePropertyChangeListeners;

	/** A mapping of property classes to multi-cast delegate that is broadcast when properties of that type change */
	TMap< TSubclassOf<UProperty>, FOnAnimatablePropertyChanged > ClassToPropertyChangedMap;

	/** Delegate to call when object changes should be propagated */
	FOnPropagateObjectChanges OnPropagateObjectChanges;

	/** The owning sequencer */
	TWeakPtr< ISequencer > Sequencer;
};
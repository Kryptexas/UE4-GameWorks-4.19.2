// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class IPropertyHandle;

DECLARE_MULTICAST_DELEGATE_ThreeParams( FOnAnimatablePropertyChanged, const TArray<UObject*>&, const IPropertyHandle&, bool );

DECLARE_MULTICAST_DELEGATE_OneParam ( FOnPropagateObjectChanges, UObject* );

/**
 * Listens for changes objects and calls delegates when those objects change
 */
class ISequencerObjectChangeListener
{
public:
	/**
	 * A delegate for when a property of a specific UProperty class is changed.  
	 */
	virtual FOnAnimatablePropertyChanged& GetOnAnimatablePropertyChanged( TSubclassOf<UProperty> PropertyClass ) = 0;

	/**
	 * A delegate for when object changes should be propagated to/from puppet actors
	 */
	virtual FOnPropagateObjectChanges& GetOnPropagateObjectChanges() = 0;
	
	/**
	 * Triggers all properties as changed for the passed in object.
	 */
	virtual void TriggerAllPropertiesChanged(UObject* Object) = 0;
};
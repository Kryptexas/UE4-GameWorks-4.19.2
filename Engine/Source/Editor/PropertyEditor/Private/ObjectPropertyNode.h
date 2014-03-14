// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//-----------------------------------------------------------------------------
//	FObjectPropertyNode - Used for the root and various sub-nodes
//-----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// Object iteration
typedef TArray< TWeakObjectPtr<UObject> >::TIterator TPropObjectIterator;
typedef TArray< TWeakObjectPtr<UObject> >::TConstIterator TPropObjectConstIterator;

class FObjectPropertyNode : public FPropertyNode
{
public:
	FObjectPropertyNode();
	virtual ~FObjectPropertyNode();

	/** FPropertyNode Interface */
	virtual FObjectPropertyNode* AsObjectNode() OVERRIDE { return this;}
	virtual const FObjectPropertyNode* AsObjectNode() const OVERRIDE { return this; }
	virtual bool GetReadAddressUncached(FPropertyNode& InNode, bool InRequiresSingleSelection, FReadAddressListData& OutAddresses, bool bComparePropertyContents = true, bool bObjectForceCompare = false, bool bArrayPropertiesCanDifferInSize = false) const OVERRIDE;
	virtual bool GetReadAddressUncached(FPropertyNode& InNode, FReadAddressListData& OutAddresses) const OVERRIDE;

	virtual uint8* GetValueBaseAddress( uint8* Base ) OVERRIDE;

	/**
	 * Returns the UObject at index "n" of the Objects Array
	 * @param InIndex - index to read out of the array
	 */
	UObject* GetUObject(int32 InIndex);
	const UObject* GetUObject(int32 InIndex) const;

	/** Returns the number of objects for which properties are currently being edited. */
	int32 GetNumObjects() const	{ return Objects.Num(); }

	/**
	 * Adds a new object to the list.
	 */
	void AddObject( UObject* InObject );
	/**
	 * Removes an object from the list.
	 */
	void RemoveObject(UObject* InObject);
	/**
	 * Removes all objects from the list.
	 */
	void RemoveAllObjects();

	/**
	 * Purges any objects marked pending kill from the object list
	 */
	void PurgeKilledObjects();

	// Called when the object list is finalized, Finalize() finishes the property window setup.
	void Finalize();

	/** @return		The base-est baseclass for objects in this list. */
	UClass*			GetObjectBaseClass()       { return BaseClass.IsValid() ? BaseClass.Get() : NULL; }
	/** @return		The base-est baseclass for objects in this list. */
	const UClass*	GetObjectBaseClass() const { return BaseClass.IsValid() ? BaseClass.Get() : NULL; }

	//////////////////////////////////////////////////////////////////////////
	/** @return		The property stored at this node, to be passed to Pre/PostEditChange. */
	virtual UProperty*		GetStoredProperty()		{ return StoredProperty.IsValid() ? StoredProperty.Get() : NULL; }

	TPropObjectIterator			ObjectIterator()			{ return TPropObjectIterator( Objects ); }
	TPropObjectConstIterator	ObjectConstIterator() const	{ return TPropObjectConstIterator( Objects ); }


	/** Generates a single child from the provided property name.  Any existing children are destroyed */
	TSharedPtr<FPropertyNode> GenerateSingleChild( FName ChildPropertyName );

	/**
	 * @return The hidden categories 
	 */
	const TSet<FName>& GetHiddenCategories() const { return HiddenCategories; }

protected:
	/** FPropertyNode interface */
	virtual void InitBeforeNodeFlags() OVERRIDE;
	virtual void InitChildNodes() OVERRIDE;
	virtual void GetQualifiedName( FString& PathPlusIndex, const bool bWithArrayIndex ) const OVERRIDE;

	/**
	 * Looks at the Objects array and creates the best base class.  Called by
	 * Finalize(); that is, when the list of selected objects is being finalized.
	 */
	void SetBestBaseClass();

private:
	/**
	 * Creates child nodes
	 * 
	 * @param SingleChildName	The property name of a single child to create instead of all childen
	 */
	void InternalInitChildNodes( FName SingleChildName = NAME_None );
private:
	/** The list of objects we are editing properties for. */
	TArray< TWeakObjectPtr<UObject> >		Objects;

	/** The lowest level base class for objects in this list. */
	TWeakObjectPtr<UClass>					BaseClass;

	/**
	 * The property passed to Pre/PostEditChange calls.  
	 */
	TWeakObjectPtr<UProperty>				StoredProperty;

	/**
	 * Set of all category names hidden by the objects in this node
	 */
	TSet<FName> HiddenCategories;
};


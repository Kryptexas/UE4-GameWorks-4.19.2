// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FObjectBaseAddress
{
public:
	FObjectBaseAddress()
		:	Object( NULL )
		,	BaseAddress( NULL )
	{}
	FObjectBaseAddress(UObject* InObject, uint8* InBaseAddress)
		:	Object( InObject )
		,	BaseAddress( InBaseAddress )
	{}

	UObject*	Object;
	uint8*		BaseAddress;
};


/**
 * Encapsulates a property node (and property) and provides functionality to read and write to that node     
 */
class FPropertyValueImpl
{
public:
	/**
	 * Constructor
	 * 
	 * @param InPropertyNode	The property node containing information about the property to be edited
	 * @param NotifyHook		Optional notify hook for pre/post edit change messages sent outside of objects being modified
	 */
	FPropertyValueImpl( TSharedPtr<FPropertyNode> InPropertyNode, FNotifyHook* InNotifyHook, TSharedPtr<IPropertyUtilities> InPropertyUtilities );

	virtual ~FPropertyValueImpl();

	/**
	 * Sets an object property to point to the new object
	 * 
	 * @param NewObject	The new object value
	 */
	bool SetObject( const UObject* NewObject, EPropertyValueSetFlags::Type Flags);

	/**
	 * Sets the value of an object property to the selected object in the content browser
	 * @return Whether or not the call was successful
	 */
	virtual FPropertyAccess::Result OnUseSelected();

	/**
	 * Recurse up to the next object node, adding all array indices into a map according to their property name
	 * @param ArrayIndexMap - for the current object, what properties use which array offsets
	 * @param InNode - node to start adding array offsets for.  This function will move upward until it gets to an object node
	 */
	static void GenerateArrayIndexMapToObjectNode( TMap<FString,int32>& OutArrayIndexMap, FPropertyNode* PropertyNode );
	
	/**
	 * Gets the value as a string formatted for multiple values in an array                 
	 */
	FString GetPropertyValueArray() const;

	/**
	 * Gets the objects that need to be modified from the passed in property node
	 * 
	 * @param ObjectsToModify	The addresses of the objects to modify
	 * @param InPropertyNode	The property node to get objects from
	 */
	void GetObjectsToModify( TArray<FObjectBaseAddress>& ObjectsToModify, FPropertyNode* InPropertyNode ) const ;

	/**
	 * Gets the union of values with the appropriate type for the property set
	 *
	 * @param OutAddress	The location of the property value
	 * @return The result of the query 
	 */
	FPropertyAccess::Result GetValueData( void*& OutAddress ) const;

	/**
	 * Given an address and a property type, get the actual value out
	 *
	 * @param Address	The location of the property value
	 * @return The property value
	 */
	template<class TProperty>
	typename TProperty::TCppType GetPropertyValue(void const* Address) const
	{
		TSharedPtr<FPropertyNode> PropertyNodePin = PropertyNode.Pin();
		check(PropertyNodePin.IsValid());
		return CastChecked<TProperty>(PropertyNodePin->GetProperty())->GetPropertyValue(Address);
	}

	/**
	 * Given an address, get the actual UObject* value out
	 *
	 * @param Address	The location of the property value
	 * @return The object property value
	 */
	UObject* GetObjectPropertyValue(void const* Address) const
	{
		TSharedPtr<FPropertyNode> PropertyNodePin = PropertyNode.Pin();
		check(PropertyNodePin.IsValid());
		return CastChecked<UObjectPropertyBase>(PropertyNodePin->GetProperty())->GetObjectPropertyValue(Address);
	}


	/**
	 * The core functionality for setting values on a property
	 *
	 * @param	The value formatted as a string
	 * @return	The result of attempting to set the value
	 */
	FPropertyAccess::Result ImportText( const FString& InValue, EPropertyValueSetFlags::Type Flags );
	FPropertyAccess::Result ImportText( const FString& InValue, FPropertyNode* PropertyNode, EPropertyValueSetFlags::Type Flags );
	FPropertyAccess::Result ImportText( const TArray<FObjectBaseAddress>& InObjects, const TArray<FString>& InValues, FPropertyNode* PropertyNode, EPropertyValueSetFlags::Type Flags );

	/**
	 * Accesses the raw data of this property. 
	 *
	 * @param RawData	An array of raw data. 
	 */ 
	void AccessRawData( TArray<void*>& RawData );

	/**
	 * Sets a delegate to call when the property value changes
	 */
	void SetOnPropertyValueChanged( const FSimpleDelegate& InOnPropertyValueChanged );

	/**
	 * Sets a delegate to call when children of the property node must be rebuilt
	 */
	void SetOnRebuildChildren( const FSimpleDelegate& InOnRebuildChildren );

	/**
	 * Sends a formatted string to an object property if safe to do so
	 *
	 * @param Text	The text to send
	 * @return true if the text could be set
	 */
	bool SendTextToObjectProperty( const FString& Text, EPropertyValueSetFlags::Type Flags ); 

	/**
	 * Get the value of a property as a formatted string.
	 * Each UProperty has a specific string format that it sets
	 *
	 * @param OutValue The formatted string value to set
	 * @return The result of attempting to get the value
	 */
	FPropertyAccess::Result GetValueAsString( FString& OutString ) const;

/**
	 * Get the value of a property as FText.
	 *
	 * @param OutValue The formatted text value to set
	 * @return The result of attempting to get the value
	 */
	FPropertyAccess::Result GetValueAsText( FText& OutText ) const;

	/**
	 * Sets the value of a property formatted from a string.
	 * Each UProperty has a specific string format it requires
	 *
	 * @param InValue The formatted string value to set
	 * @return The result of attempting to set the value
	 */
	FPropertyAccess::Result SetValueAsString( const FString& InValue, EPropertyValueSetFlags::Type Flags );

	/**
	 * Returns whether or not a property is of a specific subclass of UProperty
	 *
	 * @param ClassType	The class type to check
	 * @param true if the property is a ClassType
	 */
	bool IsPropertyTypeOf( UClass* ClassType ) const ;


	/**
	 * @return Clamps the value as a float from meta data stored on the property
	 */
	float ClampFloatValueFromMetaData( float InValue ) const ;

	/**
	 * @return Clamps the value as an integer from meta data stored on the property
	 */
	int32 ClampIntValueFromMetaData( int32 InValue ) const ;
	

	/**
	 * @return The property node used by this value
	 */
	TSharedPtr<FPropertyNode> GetPropertyNode() const;

	/**
	 * @return The number of children the property node has
	 */
	int32 GetNumChildren() const;

	/**
	 * Gets a child node of the property node
	 * 
	 * @param ChildName	The name of the child to get
	 * @return The child property node or NULL if it doesnt exist
	 */
	TSharedPtr<FPropertyNode> GetChildNode( FName ChildName ) const;

	/**
	 * Gets a child node of the property node
	 * 
	 * @param ChildIndex	The child index where the child is stored
	 * @return The child property node or NULL if it doesnt exist
	 */
	TSharedPtr<FPropertyNode> GetChildNode( int32 ChildIndex ) const ;

	/**
	 * Rests the value to its default value
	 */
	void ResetToDefault() ;

	/**
	 * @return true if the property value differs from its default value
	 */
	bool DiffersFromDefault() const ;

	/**
	 * @return true if the property  is edit const and cannot be changed
	 */
	bool IsEditConst() const;

	/**
	 * @return The label to use for displaying reset to default values
	 */
	FText GetResetToDefaultLabel() const;

	/**
	 * Adds a child to the property node (arrays only)
	 */
	void AddChild();

	/**
	 * Removes all children from the property node (arrays only)
	 */
	void ClearChildren();

	/** 
	 * Inserts a child at Index
	 */ 
	void InsertChild( int32 Index );

	/**
	 * Inserts a child at the index provided by the child node                                                            
	 */
	void InsertChild( TSharedPtr<FPropertyNode> ChildNodeToInsertAfter );

	/** 
	 * Duplicates the child at Index (arrays only)
	 */ 
	void DuplicateChild( int32 Index );

	/**
	 * Duplicates the provided child (arrays only)                                                             
	 */
	void DuplicateChild( TSharedPtr<FPropertyNode> ChildNodeToDuplicate );

	/** 
	 * Deletes the child at Index
	 */ 
	void DeleteChild( int32 Index );

	/** 
	 * Deletes the provided child
	 */ 
	void DeleteChild( TSharedPtr<FPropertyNode> ChildNodeToDelete );


	/**
	 * @return true if the property node is valid
	 */
	bool HasValidProperty() const;

	/**
	 * @return The display name of the property
	 */
	FString GetDisplayName() const;

	/** @return The notify hook being used */
	FNotifyHook* GetNotifyHook() const { return NotifyHook; }

	TSharedPtr<IPropertyUtilities> GetPropertyUtilities() const { return PropertyUtilities.Pin(); }
protected:
	/**
	 * 
	 * @param InPropertyNode	The property to get value from
	 * @param OutText			The property formatted in a string
	 * @return true if the value was retrieved successfully
	 */
	FPropertyAccess::Result GetPropertyValueString( FString& OutString, FPropertyNode* InPropertyNode ) const;

	/**
	 * @param InPropertyNode	The property to get value from
	 * @param OutText			The property formatted in text
	 * @return true if the value was retrieved successfully
	 */
	FPropertyAccess::Result GetPropertyValueText( FText& OutText, FPropertyNode* InPropertyNode ) const;

protected:
	/** Property node used to access UProperty and address of object to change */
	TWeakPtr<FPropertyNode> PropertyNode;
	TWeakPtr<IPropertyUtilities> PropertyUtilities;
	/** This delegate is executed when the property value changed.  The property node is registed with this callback and we store it so we can remove it later */
	FSimpleDelegate PropertyValueChangedDelegate;
	/** Notify hook to call when properties change */
	FNotifyHook* NotifyHook;
	/** Set true if a change was made with bFinished=false */
	bool bInteractiveChangeInProgress;
};

#define DECLARE_PROPERTY_ACCESSOR( ValueType ) \
	virtual FPropertyAccess::Result SetValue( const ValueType& InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE; \
	virtual FPropertyAccess::Result GetValue( ValueType& OutValue ) const OVERRIDE; 

class FDetailCategoryImpl;

/**
 * The base implementation of a property handle
 */
class FPropertyHandleBase : public IPropertyHandle
{
public:
	FPropertyHandleBase( TSharedPtr<FPropertyNode> PropertyNode, FNotifyHook* NotifyHook, TSharedPtr<IPropertyUtilities> PropertyUtilities );

	/** IPropertyHandle interface */
	DECLARE_PROPERTY_ACCESSOR( bool )
	DECLARE_PROPERTY_ACCESSOR( int32 )
	DECLARE_PROPERTY_ACCESSOR( float )
	DECLARE_PROPERTY_ACCESSOR( FString )
	DECLARE_PROPERTY_ACCESSOR( FName )
	DECLARE_PROPERTY_ACCESSOR( uint8 )
	DECLARE_PROPERTY_ACCESSOR( FVector )
	DECLARE_PROPERTY_ACCESSOR( FVector2D )
	DECLARE_PROPERTY_ACCESSOR( FVector4 )
	DECLARE_PROPERTY_ACCESSOR( FQuat )
	DECLARE_PROPERTY_ACCESSOR( FRotator )
	DECLARE_PROPERTY_ACCESSOR( UObject* )

	/** IPropertyHandle interface */
	virtual bool IsValidHandle() const OVERRIDE;
	virtual FString GetPropertyDisplayName() const OVERRIDE;
	virtual void ResetToDefault() OVERRIDE;
	virtual bool DiffersFromDefault() const OVERRIDE;
	virtual FText GetResetToDefaultLabel() const OVERRIDE;
	virtual void MarkHiddenByCustomization() OVERRIDE;
	virtual bool IsCustomized() const OVERRIDE;
	virtual TSharedRef<SWidget> CreatePropertyNameWidget( const FString& NameOverride = TEXT(""), bool bDisplayResetToDefault = false, bool bDisplayText = true, bool bDisplayThumbnail = true ) const OVERRIDE;
	virtual TSharedRef<SWidget> CreatePropertyValueWidget() const OVERRIDE;
	virtual bool IsEditConst() const OVERRIDE;
	virtual void SetOnPropertyValueChanged( const FSimpleDelegate& InOnPropertyValueChanged ) OVERRIDE;
	virtual int32 GetIndexInArray() const OVERRIDE;
	virtual FPropertyAccess::Result GetValueAsFormattedString( FString& OutValue ) const OVERRIDE;
	virtual FPropertyAccess::Result SetValueFromFormattedString( const FString& InValue,  EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;
	virtual TSharedPtr<IPropertyHandle> GetChildHandle( uint32 ChildIndex ) const OVERRIDE;
	virtual TSharedPtr<IPropertyHandle> GetChildHandle( FName ChildName ) const OVERRIDE;
	virtual TSharedPtr<IPropertyHandle> GetParentHandle() const OVERRIDE;
	virtual void AccessRawData( TArray<void*>& RawData ) OVERRIDE;
	virtual uint32 GetNumOuterObjects() const OVERRIDE;
	virtual void GetOuterObjects( TArray<UObject*>& OuterObjects ) OVERRIDE;
	virtual FPropertyAccess::Result GetNumChildren( uint32& OutNumChildren ) const OVERRIDE;
	virtual TSharedPtr<IPropertyHandleArray> AsArray() OVERRIDE { return NULL; }
	virtual const UClass* GetPropertyClass() const OVERRIDE;
	virtual UProperty* GetProperty() const OVERRIDE;
	virtual FString GetToolTipText() const OVERRIDE;
	virtual void SetToolTipText(const FString& ToolTip) OVERRIDE;
	virtual FPropertyAccess::Result SetPerObjectValues( const TArray<FString>& InPerObjectValues,  EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;
	virtual FPropertyAccess::Result GetPerObjectValues( TArray<FString>& OutPerObjectValues ) OVERRIDE;
	virtual bool GeneratePossibleValues(TArray< TSharedPtr<FString> >& OutOptionStrings, TArray< TSharedPtr<FString> >& OutToolTips, TArray<bool>& OutRestrictedItems) OVERRIDE;
	virtual FPropertyAccess::Result SetObjectValueFromSelection() OVERRIDE;
	virtual void NotifyPreChange() OVERRIDE;
	virtual void NotifyPostChange() OVERRIDE;
	virtual void AddRestriction( TSharedRef<const FPropertyRestriction> Restriction )OVERRIDE;
	virtual bool IsRestricted(const FString& Value) const OVERRIDE;
	virtual bool IsRestricted(const FString& Value, TArray<FText>& OutReasons) const OVERRIDE;
	virtual bool GenerateRestrictionToolTip(const FString& Value, FText& OutTooltip) const OVERRIDE;

	TSharedPtr<FPropertyNode> GetPropertyNode();
protected:
	TSharedPtr<FPropertyValueImpl> Implementation;
};

class FPropertyHandleInt : public FPropertyHandleBase
{	
public:
	FPropertyHandleInt( TSharedRef<FPropertyNode> PropertyNode, FNotifyHook* NotifyHook, TSharedPtr<IPropertyUtilities> PropertyUtilities );
	static bool Supports( TSharedRef<FPropertyNode> PropertyNode );
	virtual FPropertyAccess::Result GetValue( int32& OutValue ) const OVERRIDE;
	virtual FPropertyAccess::Result SetValue( const int32& InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;
};

class FPropertyHandleFloat : public FPropertyHandleBase
{
public:
	FPropertyHandleFloat( TSharedRef<FPropertyNode> PropertyNode, FNotifyHook* NotifyHook, TSharedPtr<IPropertyUtilities> PropertyUtilities );
	static bool Supports( TSharedRef<FPropertyNode> PropertyNode );
	virtual FPropertyAccess::Result GetValue( float& OutValue ) const OVERRIDE;
	virtual FPropertyAccess::Result SetValue( const float& InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;
};

class FPropertyHandleBool : public FPropertyHandleBase
{
public:
	FPropertyHandleBool( TSharedRef<FPropertyNode> PropertyNode, FNotifyHook* NotifyHook, TSharedPtr<IPropertyUtilities> PropertyUtilities );
	static bool Supports( TSharedRef<FPropertyNode> PropertyNode );
	virtual FPropertyAccess::Result GetValue( bool& OutValue ) const OVERRIDE;
	virtual FPropertyAccess::Result SetValue( const bool& InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;
};

class FPropertyHandleByte : public FPropertyHandleBase
{	
public:
	FPropertyHandleByte( TSharedRef<FPropertyNode> PropertyNode, FNotifyHook* NotifyHook, TSharedPtr<IPropertyUtilities> PropertyUtilities );
	static bool Supports( TSharedRef<FPropertyNode> PropertyNode );
	virtual FPropertyAccess::Result GetValue( uint8& OutValue ) const OVERRIDE;
	virtual FPropertyAccess::Result SetValue( const uint8& InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;
};

class FPropertyHandleString : public FPropertyHandleBase
{
public:
	FPropertyHandleString( TSharedRef<FPropertyNode> PropertyNode, FNotifyHook* NotifyHook,TSharedPtr<IPropertyUtilities> PropertyUtilities );
	static bool Supports( TSharedRef<FPropertyNode> PropertyNode );
	virtual FPropertyAccess::Result GetValue( FString& OutValue ) const OVERRIDE;
	virtual FPropertyAccess::Result SetValue( const FString& InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;

	virtual FPropertyAccess::Result GetValue( FName& OutValue ) const OVERRIDE;
	virtual FPropertyAccess::Result SetValue( const FName& InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;
};

class FPropertyHandleObject : public FPropertyHandleBase
{
public:
	FPropertyHandleObject( TSharedRef<FPropertyNode> PropertyNode, FNotifyHook* NotifyHook, TSharedPtr<IPropertyUtilities> PropertyUtilities );
	static bool Supports( TSharedRef<FPropertyNode> PropertyNode );
	virtual FPropertyAccess::Result GetValue( UObject*& OutValue ) const OVERRIDE;
	virtual FPropertyAccess::Result SetValue( const UObject*& InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;

};

class FPropertyHandleVector : public FPropertyHandleBase
{
public:
	FPropertyHandleVector( TSharedRef<FPropertyNode> PropertyNode, FNotifyHook* NotifyHook, TSharedPtr<IPropertyUtilities> PropertyUtilities );
	static bool Supports( TSharedRef<FPropertyNode> PropertyNode );
	virtual FPropertyAccess::Result GetValue( FVector& OutValue ) const OVERRIDE;
	virtual FPropertyAccess::Result SetValue( const FVector& InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;

	virtual FPropertyAccess::Result GetValue( FVector2D& OutValue ) const OVERRIDE;
	virtual FPropertyAccess::Result SetValue( const FVector2D& InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;

	virtual FPropertyAccess::Result GetValue( FVector4& OutValue ) const OVERRIDE;
	virtual FPropertyAccess::Result SetValue( const FVector4& InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;

	virtual FPropertyAccess::Result GetValue( FQuat& OutValue ) const OVERRIDE;
	virtual FPropertyAccess::Result SetValue( const FQuat& InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;

	virtual FPropertyAccess::Result SetX( float InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags );
	virtual FPropertyAccess::Result SetY( float InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags );
	virtual FPropertyAccess::Result SetZ( float InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags );
	virtual FPropertyAccess::Result SetW( float InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags );
private:
	TArray< TSharedPtr<FPropertyHandleFloat> > VectorComponents;
};

class FPropertyHandleRotator : public FPropertyHandleBase
{
public:
	FPropertyHandleRotator( TSharedRef<FPropertyNode> PropertyNode, FNotifyHook* NotifyHook, TSharedPtr<IPropertyUtilities> PropertyUtilities  );
	static bool Supports( TSharedRef<FPropertyNode> PropertyNode );
	virtual FPropertyAccess::Result GetValue( FRotator& OutValue ) const OVERRIDE;
	virtual FPropertyAccess::Result SetValue( const FRotator& InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags ) OVERRIDE;

	virtual FPropertyAccess::Result SetRoll( float InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags );
	virtual FPropertyAccess::Result SetPitch( float InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags );
	virtual FPropertyAccess::Result SetYaw( float InValue, EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::DefaultFlags );
private:
	TSharedPtr<FPropertyHandleFloat> RollValue;
	TSharedPtr<FPropertyHandleFloat> PitchValue;
	TSharedPtr<FPropertyHandleFloat> YawValue;
};


class FPropertyHandleArray : public FPropertyHandleBase, public IPropertyHandleArray, public TSharedFromThis<FPropertyHandleArray>
{
public:
	FPropertyHandleArray( TSharedRef<FPropertyNode> PropertyNode, FNotifyHook* NotifyHook, TSharedPtr<IPropertyUtilities> PropertyUtilities );
	static bool Supports( TSharedRef<FPropertyNode> PropertyNode );
	/** IPropertyHandleArray interface */
	virtual FPropertyAccess::Result AddItem() OVERRIDE;
	virtual FPropertyAccess::Result EmptyArray() OVERRIDE;
	virtual FPropertyAccess::Result Insert( int32 Index ) OVERRIDE;
	virtual FPropertyAccess::Result DuplicateItem( int32 Index ) OVERRIDE;
	virtual FPropertyAccess::Result DeleteItem( int32 Index ) OVERRIDE;
	virtual FPropertyAccess::Result GetNumElements( uint32& OutNumItems ) const OVERRIDE;
	virtual void SetOnNumElementsChanged( FSimpleDelegate& InOnNumElementsChanged ) OVERRIDE;
	virtual TSharedPtr<IPropertyHandleArray> AsArray() OVERRIDE;
	virtual TSharedRef<IPropertyHandle> GetElement( int32 Index ) const OVERRIDE;
private:
	/**
	 * @return Whether or not the array can be modified
	 */
	bool IsEditable() const;
};

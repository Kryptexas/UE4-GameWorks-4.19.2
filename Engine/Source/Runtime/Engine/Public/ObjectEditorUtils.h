// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_EDITOR
class UProperty;

namespace FObjectEditorUtils
{

	/**
	 * Gets the category this property belongs to.
	 *
	 * @param	InProperty	Property we want the category name of.
	 * @return	Category name of the given property.
	 */
	ENGINE_API FString GetCategory( const class UProperty* InProperty );	

	/**
	 * Gets the category this property belongs to.
	 *
	 * @param	InProperty	Property we want the category name of.
	 * @return	Category name of the given property.
	 */
	ENGINE_API FName GetCategoryFName( const class UProperty* InProperty );


	/**
	 * Query if a function is flagged as hidden from the given class either by category or by function name
	 *
	 * @param	InFunction	Function to check
	 * @param	InClass		Class to check hidden if the function is hidden in
	 *
	 * @return	true if the function is hidden in the specified class.
	 */
	ENGINE_API bool IsFunctionHiddenFromClass( const UFunction* InFunction, const UClass* Class );

	/**
	 * Query if a the category a variable belongs to is flagged as hidden from the given class.
	 *
	 * @param	InVariable	Property to check
	 * @param	InClass		Class to check hidden if the function is hidden in
	 *
	 * @return	true if the functions category is hidden in the specified class.
	 */
	ENGINE_API bool IsVariableCategoryHiddenFromClass( const UProperty* InVariable, const UClass* Class );

	/**
	 * Copy the value of a property from source object to a destination object.
	 *
	 * @param	SourceObject		The object to copy the property value from.
	 * @param	SourceProperty		The property on the SourceObject to copy the value from.
	 * @param	DestinationObject	The object to copy the property value to.
	 * @param	DestinationProperty	The property on the DestinationObject to copy the value to.
	 *
	 * @return true if the value was successfully migrated.
	 */
	ENGINE_API bool MigratePropertyValue(UObject* SourceObject, UProperty* SourceProperty, UObject* DestinationObject, UProperty* DestinationProperty);

	/**
	 * Copy the value of a property from source object to a destination object.
	 *
	 * @param	SourceObject			The object to copy the property value from.
	 * @param	SourcePropertyName		The name of the property on the SourceObject to copy the value from.
	 * @param	DestinationObject		The object to copy the property value to.
	 * @param	DestinationPropertyName	The name of the property on the DestinationObject to copy the value to.
	 *
	 * @return true if the value was successfully migrated.
	 */
	template <typename SourceType, typename DestinationType>
	bool MigratePropertyValue(SourceType* SourceObject, FName SourcePropertyName, DestinationType* DestinationObject, FName DestinationPropertyName)
	{
		UProperty* SourceProperty = FindFieldChecked<UProperty>(SourceType::StaticClass(), SourcePropertyName);
		UProperty* DestinationProperty = FindFieldChecked<UProperty>(DestinationType::StaticClass(), DestinationPropertyName);

		return FObjectEditorUtils::MigratePropertyValue(SourceObject, SourceProperty, DestinationObject, DestinationProperty);
	}

	/**
	 * Set the value on an UObject using reflection.
	 * @param	Object			The object to copy the value into.
	 * @param	PropertyName	The name of the property to set.
	 * @param	Value			The value to assign to the property.
	 *
	 * @return true if the value was set correctly
	 */
	template <typename ObjectType, typename ValueType>
	bool SetPropertyValue(ObjectType* Object, FName PropertyName, ValueType Value)
	{
		// Get the property addresses for the source and destination objects.
		UProperty* Property = FindFieldChecked<UProperty>(ObjectType::StaticClass(), PropertyName);

		// Get the property addresses for the object
		ValueType* SourceAddr = Property->ContainerPtrToValuePtr<ValueType>(Object);

		if ( SourceAddr == NULL )
		{
			return false;
		}

		if ( !Object->HasAnyFlags(RF_ClassDefaultObject) )
		{
			Object->PreEditChange(Property);
		}

		// Set the value on the destination object.
		*SourceAddr = Value;

		if ( !Object->HasAnyFlags(RF_ClassDefaultObject) )
		{
			FPropertyChangedEvent PropertyEvent(Property);
			Object->PostEditChangeProperty(PropertyEvent);
		}

		return true;
	}
};

#endif // WITH_EDITOR

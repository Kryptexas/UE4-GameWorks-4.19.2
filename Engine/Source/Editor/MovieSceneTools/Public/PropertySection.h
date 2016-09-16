// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneTrack.h"
#include "MovieSceneSection.h"
#include "MovieSceneCommonHelpers.h"
#include "ISequencerSection.h"

class FTrackInstancePropertyBindings;

/**
 * A generic implementation for displaying simple property sections
 */
class MOVIESCENETOOLS_API FPropertySection
	: public ISequencerSection
{
public:

	/**
	* Creates a new property section for editing non-property sections.
	*
	* @param InSectionObject The section object which is being displayed and edited.
	* @param InDisplayName A display name for the section being displayed and edited.
	* @TODO Make another section which is a good base class for non-property sections.
	*/
	FPropertySection(UMovieSceneSection& InSectionObject, const FText& InDisplayName);

	/**
	* Creates a new property section.
	*
	* @param InSequencer The sequencer which is controlling this property section.
	* @param InObjectBinding The object binding for the object which owns the property that this section is animating.
	* @param InPropertyName The name of the property which is animated by this section.
	* @param InPropertyPath A string representing the path to the property which is animated by this section.
	* @param InSectionObject The section object which is being displayed and edited.
	* @param InDisplayName A display name for the section being displayed and edited.
	*/
	FPropertySection(ISequencer* InSequencer, FGuid InObjectBinding, FName InPropertyName, const FString& InPropertyPath, UMovieSceneSection& InSectionObject, const FText& InDisplayName);

public:

	// ISequencerSection interface

	virtual UMovieSceneSection* GetSectionObject() override;
	virtual FText GetDisplayName() const override;
	virtual FText GetSectionTitle() const override;
	virtual int32 OnPaintSection(FSequencerSectionPainter& Painter) const override;

protected:

	/** Gets the sequencer which is controlling this section .*/
	ISequencer* GetSequencer() const { return Sequencer; }

	/** Gets the property being animated by this section.  Returns nullptr if this section was not constructed with the necessary
		data to get the property value, or if the runtime object or property can not be found. */
	UProperty* GetProperty() const;

	/** Gets the current value of the property which is being animated.  The optional return value will be unset if
		this section was not constructed with the necessary data, or is the runtime object or property can not be found. */
	template<typename ValueType>
	TOptional<ValueType> GetPropertyValue() const
	{
		UObject* RuntimeObject = GetRuntimeObjectAndUpdatePropertyBindings();

		// Bool property values are stored in a bit field so using a straight cast of the pointer to get their value does not
		// work.  Instead use the actual property to get the correct value.
		const UBoolProperty* BoolProperty = Cast<const UBoolProperty>(GetProperty());
		if (BoolProperty)
		{
			uint32* ValuePtr = BoolProperty->ContainerPtrToValuePtr<uint32>(RuntimeObject);
			bool BoolPropertyValue = BoolProperty->GetPropertyValue(ValuePtr);
			void *PropertyValue = &BoolPropertyValue;
			return TOptional<ValueType>(*((ValueType*)PropertyValue));
		}

		return RuntimeObject != nullptr
			? TOptional<ValueType>(PropertyBindings->GetCurrentValue<ValueType>(RuntimeObject))
			: TOptional<ValueType>();
	}

	/** Returns true when this section was constructed with the data necessary to query for the current property value. */
	bool CanGetPropertyValue() const;

	/** Gets the current runtime object for the object binding which was used to construct this section.  Returns nullptr
		if no valid object can be found, or if an object binding was not supplied. */
	UObject* GetRuntimeObjectAndUpdatePropertyBindings() const;

protected:

	/** Display name of the section */
	FText DisplayName;

	/** The section we are visualizing */
	UMovieSceneSection& SectionObject;

private:

	/** The sequencer which is controlling this section. */
	ISequencer* Sequencer;

	/** An object binding for the object which owns the property being animated by this section. */
	FGuid ObjectBinding;

	/** An object which is used to retrieve the value of a property based on it's name and path. */
	mutable TSharedPtr<FTrackInstancePropertyBindings> PropertyBindings;

	/** Caches a weak pointer to the most recent runtime object retrieved by this section.  This cache
		is maintained so that the property bindings can be updated any time this object changes. */
	mutable TWeakObjectPtr<UObject> RuntimeObjectCache;
};


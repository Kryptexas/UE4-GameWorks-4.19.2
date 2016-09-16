// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "BoolKeyArea.h"
#include "VisibilityPropertySection.h"
#include "MovieSceneVisibilitySection.h"


void FVisibilityPropertySection::GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder) const
{
	UMovieSceneVisibilitySection* VisibilitySection = Cast<UMovieSceneVisibilitySection>(&SectionObject);
	TAttribute<TOptional<bool>> ExternalValue;
	ExternalValue.Bind(TAttribute<TOptional<bool>>::FGetter::CreateLambda([&]
	{
		UObject* RuntimeObject = GetRuntimeObjectAndUpdatePropertyBindings();

		// Bool property values are stored in a bit field so using a straight cast of the pointer to get their value does not
		// work.  Instead use the actual property to get the correct value.
		const UBoolProperty* BoolProperty = Cast<const UBoolProperty>( GetProperty());
		uint32* ValuePtr = BoolProperty->ContainerPtrToValuePtr<uint32>(RuntimeObject);
		bool BoolPropertyValue = !BoolProperty->GetPropertyValue(ValuePtr);
		void *PropertyValue = &BoolPropertyValue;
		return TOptional<bool>(*((bool*)PropertyValue));
	}));
	TSharedRef<FBoolKeyArea> KeyArea = MakeShareable(new FBoolKeyArea(VisibilitySection->GetCurve(), ExternalValue, VisibilitySection));
	LayoutBuilder.SetSectionAsKeyArea(KeyArea);
}
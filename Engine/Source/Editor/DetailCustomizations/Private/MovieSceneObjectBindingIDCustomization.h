// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IPropertyTypeCustomization.h"
#include "MovieSceneObjectBindingIDPicker.h"

class FReply;
class IPropertyHandle;
class UMovieSceneSequence;
class FDragDropOperation;
struct FMovieSceneObjectBindingID;


class FMovieSceneObjectBindingIDCustomization
	: public IPropertyTypeCustomization
	, FMovieSceneObjectBindingIDPicker
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FMovieSceneObjectBindingIDCustomization);
	}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override {}

private:

	virtual UMovieSceneSequence* GetSequence() const override;

	virtual void SetCurrentValue(const FMovieSceneObjectBindingID& InBindingId) override;

	virtual FMovieSceneObjectBindingID GetCurrentValue() const override;

	FReply OnDrop(TSharedPtr<FDragDropOperation> InOperation);

	TSharedPtr<IPropertyHandle> StructProperty;
};
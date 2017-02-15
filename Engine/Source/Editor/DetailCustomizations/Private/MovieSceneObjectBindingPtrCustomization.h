// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IPropertyTypeCustomization.h"


class IDetailsView;
class IMovieSceneBindingOwnerInterface;
class IPropertyHandle;
class SWidget;
class UMovieSceneSequence;

struct FMovieScenePossessable;


class FMovieSceneObjectBindingPtrCustomization
	: public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FMovieSceneObjectBindingPtrCustomization);
	}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override {}

private:

	FText ResolveBindingNameInSequence() const;

	IMovieSceneBindingOwnerInterface* GetInterface() const;
	UMovieSceneSequence* GetSequence() const;

	void SetBindingId(FGuid InGuid);
	FGuid GetGuid() const;

	TSharedRef<SWidget> GetBindingMenu();

private:

	TSharedPtr<IPropertyHandle> StructProperty;
	TSharedPtr<IPropertyHandle> GuidProperty;
};
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LandscapeUIDetails.h"
//#include "Misc/Attribute.h"
//#include "Misc/Guid.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"
#include "Runtime/Landscape/Classes/Landscape.h"
//#include "Editor.h"

FLandscapeUIDetails::FLandscapeUIDetails()
{
}

FLandscapeUIDetails::~FLandscapeUIDetails()
{
}

TSharedRef<IDetailCustomization> FLandscapeUIDetails::MakeInstance()
{
	return MakeShareable( new FLandscapeUIDetails);
}

void FLandscapeUIDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	TArray<TWeakObjectPtr<UObject>> EditingObjects;
	DetailBuilder.GetObjectsBeingCustomized(EditingObjects);

	if (EditingObjects.Num() == 1)
	{
		ALandscape* Landscape = Cast<ALandscape>(EditingObjects[0].Get());

		if (Landscape != nullptr && Landscape->NumSubsections == 1)
		{
			TSharedRef<IPropertyHandle> ComponentScreenSizeToUseSubSectionsProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ALandscapeProxy, ComponentScreenSizeToUseSubSections));
			DetailBuilder.HideProperty(ComponentScreenSizeToUseSubSectionsProp);
		}
	}
}
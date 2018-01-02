// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "BodyInstanceCustomization.h"

class IDetailLayoutBuilder;
class IPropertyHandle;

class FBodySetupDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override;

private:
	void AddPhysicalAnimation(IDetailLayoutBuilder& DetailBuilder);

private:
	TSharedPtr<class FBodyInstanceCustomizationHelper> BodyInstanceCustomizationHelper;
	TArray< TWeakObjectPtr<UObject> > ObjectsCustomized;
	TSharedPtr<IPropertyHandle> CollisionReponseHandle;
};



class FSkeletalBodySetupDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	TArray< TWeakObjectPtr<UObject> > ObjectsCustomized;
};


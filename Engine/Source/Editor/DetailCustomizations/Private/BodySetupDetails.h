// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FBodySetupDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;

	FText OnGetBodyMass() const;
	bool IsBodyMassReadOnly() const { return true; }

private:
	TArray< TWeakObjectPtr<UObject> > ObjectsCustomized;
};


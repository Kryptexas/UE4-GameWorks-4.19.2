// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FPrimitiveComponentDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;

private:
	void AddAdvancedSubCategory(IDetailLayoutBuilder& DetailBuilder, FName MainCategory, FName SubCategory);
	
	/** Returns whether to enable editing the 'Simulate Physics' checkbox based on the selected objects physics geometry */
	bool IsSimulatePhysicsEditable() const;

	FText OnGetBodyMass() const;
	bool IsBodyMassReadOnly() const { return true; }

private:
	/** Objects being customized so we can update the 'Simulate Physics' state if physics geometry is added/removed */
	TArray< TWeakObjectPtr<UObject> > ObjectsCustomized;
};


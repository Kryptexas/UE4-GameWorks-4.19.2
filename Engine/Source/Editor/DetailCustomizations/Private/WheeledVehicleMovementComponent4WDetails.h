// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FWheeledVehicleMovementComponent4WDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
private:
	void SetGearShiftRatio(float NewValue, ETextCommit::Type CommitInfo, bool bShiftUp, int32 GearIdx);
	void FillAutoBoxWidget(UWheeledVehicleMovementComponent4W * VehicleMovement, FDetailWidgetRow & AutoBoxSetup);

	TArray<TWeakObjectPtr<UObject> > SelectedObjects;	//the objects we're showing details for
};


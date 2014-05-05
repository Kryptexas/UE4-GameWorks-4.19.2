// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCurveEditor.h"

class FWheeledVehicleMovementComponent4WDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
	
private:

	struct FSteeringCurveEditor : public FCurveOwnerInterface
	{
		FSteeringCurveEditor(UWheeledVehicleMovementComponent4W * InVehicle = NULL);
		/** FCurveOwnerInterface interface */
		virtual TArray<FRichCurveEditInfoConst> GetCurves() const OVERRIDE;
		virtual TArray<FRichCurveEditInfo> GetCurves() OVERRIDE;
		virtual UObject* GetOwner() OVERRIDE;
		virtual void ModifyOwner() OVERRIDE;
		virtual void MakeTransactional() OVERRIDE;

	private:
		UWheeledVehicleMovementComponent4W * VehicleComponent;
		UObject * Owner;

	} SteeringCurveEditor;

	TArray<TWeakObjectPtr<UObject> > SelectedObjects;	//the objects we're showing details for
	
	/** Steering curve widget */
	TSharedPtr<class SCurveEditor> SteeringCurveWidget;
};


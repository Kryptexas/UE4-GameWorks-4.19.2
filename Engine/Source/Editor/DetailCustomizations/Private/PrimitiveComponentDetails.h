// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FPrimitiveComponentDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override;

private:
	ESlateCheckBoxState::Type IsMobilityActive(TSharedRef<IPropertyHandle> MobilityHandle, EComponentMobility::Type InMobility) const;

	void OnMobilityChanged(ESlateCheckBoxState::Type InCheckedState, TSharedRef<IPropertyHandle> MobilityHandle, EComponentMobility::Type InMobility);

	void AddAdvancedSubCategory(IDetailLayoutBuilder& DetailBuilder, FName MainCategory, FName SubCategory);

	FReply OnMobilityResetClicked(TSharedRef<IPropertyHandle> MobilityHandle);

	EVisibility GetMobilityResetVisibility(TSharedRef<IPropertyHandle> MobilityHandle) const;
	
	/** Returns whether to enable editing the 'Simulate Physics' checkbox based on the selected objects physics geometry */
	bool IsSimulatePhysicsEditable() const;
	/** Returns whether to enable editing the 'Use Async Scene' checkbox based on the selected objects' mobility and if the project uses an AsyncScene */
	bool IsUseAsyncEditable() const;

	FText OnGetBodyMass() const;
	bool IsBodyMassReadOnly() const { return true; }

private:
	/** Objects being customized so we can update the 'Simulate Physics' state if physics geometry is added/removed */
	TArray< TWeakObjectPtr<UObject> > ObjectsCustomized;
};


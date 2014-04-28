// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "WheeledVehicleMovementComponent4WDetails.h"
#include "ScopedTransaction.h"
#include "ObjectEditorUtils.h"
#include "Vehicles/WheeledVehicleMovementComponent4W.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "WheeledVehicleMovementComponent4WDetails"

//////////////////////////////////////////////////////////////
// This class customizes various settings in WheeledVehicleMovementComponent4W
//////////////////////////////////////////////////////////////

TSharedRef<IDetailCustomization> FWheeledVehicleMovementComponent4WDetails::MakeInstance()
{
	return MakeShareable(new FWheeledVehicleMovementComponent4WDetails);
}

void FWheeledVehicleMovementComponent4WDetails::FillAutoBoxWidget(UWheeledVehicleMovementComponent4W * VehicleMovement, FDetailWidgetRow & AutoBoxSetup)
{
	/*FSlateFontInfo DetailFont = IDetailLayoutBuilder::GetDetailFont();

	AutoBoxSetup.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("AutoBoxSetupHeader", "Auto Box Setup"))
		.ToolTipText(LOCTEXT("AutoBoxSetupHeaderTip", "Describes the automatic transition of the vehicle - how it shifts gears etc."))
		.Font(DetailFont)
	];

	TSharedRef<SVerticalBox> AutoBoxWidget = SNew(SVerticalBox);

	AutoBoxWidget
	[
		SNew(STextBlock)
		.Text(LOCTEXT("ASDS", "Testing"))
	];

	AutoBoxSetup.ValueContent()
	[
		AutoBoxWidget
	];*/
	
	//We want to inline the gear ratios with Neutral and Latency.
	//We only want to show as many ratios as there are gears
	/*FVehicleAutoBoxData AutoBox = VehicleMovement->AutoBoxSetup;
	FVehicleGearData GearData = VehicleMovement->GearSetup;

	uint32 NumChildren = 0;
	AutoBoxSetup->GetNumChildren(NumChildren);
	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedPtr<IPropertyHandle> ChildProperty = AutoBoxSetup->GetChildHandle(ChildIndex);
		// Only permit modifying bSimulatePhysics when the body has some geometry.
		if (ChildProperty->GetProperty()->GetName() == TEXT("ForwardGearAutoBox"))
		{
			for (int32 GearIdx = 0; GearIdx != GearData.ForwardGearRatios.Num(); ++GearIdx)
			{
				const FGearUpDownRatio & GearUpDownRatio = AutoBox.ForwardGearAutoBox[GearIdx + FVehicleGearData::eFIRST];
				FText Label = FText::Format(LOCTEXT("AutoBoxGear", "Gear {0}"), FText::AsNumber(GearIdx + 1));

				VehicleSetupCategory.AddCustomRow(Label.ToString(), false)
				.NameContent()
					[
						SNew(STextBlock)
						.Text(Label)
						.ToolTipText(LOCTEXT("MassInKG", "Mass of the body in KG"))
						.Font(DetailFont)
					]
				.ValueContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(0.5f)
						[
							SNew(SNumericEntryBox<float>)
							.Value(GearUpDownRatio.DownRatio)
							.MinValue(0.f)
							.MaxValue(1.f)
							.Font(DetailFont)
							.OnValueCommitted(this, &FWheeledVehicleMovementComponent4WDetails::SetGearShiftRatio, false, GearIdx)
							.LabelVAlign(VAlign_Center)
							.Label()
								[
									SNew(STextBlock)
									.Font(DetailFont)
									.Text(LOCTEXT("AutoBoxShiftMin", "Min"))
								]
							.ToolTipText(LOCTEXT("AutoBoxShiftMinTip", "Minimum currentRPM/maxRPM ratio required to stay in this gear. Going lower will cause autobox to gear down"))
						]	
						+ SHorizontalBox::Slot()
						.FillWidth(0.5f)
						.Padding(4, 0)
						[
							SNew(SNumericEntryBox<float>)
							.Value(GearUpDownRatio.UpRatio)
							.MinValue(0.f)
							.MaxValue(1.f)
							.Font(DetailFont)
							.OnValueCommitted(this, &FWheeledVehicleMovementComponent4WDetails::SetGearShiftRatio, true, GearIdx)
							.LabelVAlign(VAlign_Center)
							.Label()
							[
								SNew(STextBlock)
								.Font(DetailFont)
								.Text(LOCTEXT("AutoBoxShiftMax", "Max"))
							]
							.ToolTipText(LOCTEXT("AutoBoxShiftMinTip", "Maximum currentRPM/maxRPM ratio required to stay in this gear. Going higher will cause autobox to gear up"))
						]
					];

			}
		}
		else
		{
			VehicleSetupCategory.AddProperty(ChildProperty);
		}
	}*/
}

void FWheeledVehicleMovementComponent4WDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);
	if (SelectedObjects.Num() != 1)	//only do customization if we have one object selected
	{
		return;
	}

	UWheeledVehicleMovementComponent4W * VehicleMovement = SelectedObjects[0].IsValid() ? Cast<UWheeledVehicleMovementComponent4W>(SelectedObjects[0].Get()) : NULL;

	// See which categories are hidden
	TArray<FString> HideCategories;
	DetailBuilder.GetDetailsView().GetBaseClass()->GetHideCategories(HideCategories);
	

	//Vehicle Setup
	if (!HideCategories.Contains(TEXT("VehicleSetup")))
	{
		IDetailCategoryBuilder& VehicleSetupCategory = DetailBuilder.EditCategory("VehicleSetup");
		//Autobox
		//FDetailWidgetRow & AutoBoxRow = VehicleSetupCategory.AddProperty(DetailBuilder.GetProperty("AutoBoxSetup")).CustomWidget();
		//FillAutoBoxWidget(VehicleMovement, AutoBoxRow);
	}
}

void FWheeledVehicleMovementComponent4WDetails::SetGearShiftRatio(float NewValue, ETextCommit::Type, bool bShiftUp, int32 GearIdx)
{
	/*if (SelectedObjects.Num() != 1)	//only do customization if we have one object selected
	{
		return;
	}

	if (UWheeledVehicleMovementComponent4W * VehicleMovement = SelectedObjects[0].IsValid() ? Cast<UWheeledVehicleMovementComponent4W>(SelectedObjects[0].Get()) : NULL)
	{
		FVehicleAutoBoxData & AutoBox = VehicleMovement->AutoBoxSetup;
		FGearUpDownRatio & GearUpDownRatio = AutoBox.ForwardGearAutoBox[GearIdx + FVehicleGearData::eFIRST];
		
		if (bShiftUp)
		{
			GearUpDownRatio.UpRatio = NewValue;
		}
		else
		{
			GearUpDownRatio.DownRatio = NewValue;
		}

	}*/

}

#undef LOCTEXT_NAMESPACE


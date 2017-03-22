//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#include "PhononProbeVolumeDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailsView.h"
#include "IDetailCustomization.h"
#include "PhononProbeVolume.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

namespace Phonon
{
	TSharedRef<IDetailCustomization> FPhononProbeVolumeDetails::MakeInstance()
	{
		return MakeShareable(new FPhononProbeVolumeDetails);
	}

	void FPhononProbeVolumeDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
	{
		const TArray<TWeakObjectPtr<UObject>>& SelectedObjects = DetailLayout.GetDetailsView().GetSelectedObjects();

		for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
		{
			const TWeakObjectPtr<UObject>& CurrentObject = SelectedObjects[ObjectIndex];
			if (CurrentObject.IsValid())
			{
				APhononProbeVolume* CurrentCaptureActor = Cast<APhononProbeVolume>(CurrentObject.Get());
				if (CurrentCaptureActor)
				{
					PhononProbeVolume = CurrentCaptureActor;
					break;
				}
			}
		}

		DetailLayout.HideCategory("BrushSettings");

		DetailLayout.EditCategory("ProbeGeneration")
			.AddCustomRow(NSLOCTEXT("PhononProbeVolumeDetails", "Generate Probes", "Generate Probes"))
			.NameContent()
			[
				SNullWidget::NullWidget
			]
			.ValueContent()
			[
				SNew(SBox)
				.WidthOverride(125)
				[
					SNew(SButton)
					.ContentPadding(3)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.OnClicked(this, &FPhononProbeVolumeDetails::OnGenerateProbes)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("PhononProbeVolumeDetails", "Generate Probes", "Generate Probes"))
						.Font(IDetailLayoutBuilder::GetDetailFont())
					]
				]
			];
	}

	FReply FPhononProbeVolumeDetails::OnGenerateProbes()
	{
		PhononProbeVolume->GenerateProbes();
		return FReply::Handled();
	}
}
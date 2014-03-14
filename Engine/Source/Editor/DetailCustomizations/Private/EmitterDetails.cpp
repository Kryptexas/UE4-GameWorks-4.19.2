// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "EmitterDetails.h"


TSharedRef<IDetailCustomization> FEmitterDetails::MakeInstance()
{
	return MakeShareable(new FEmitterDetails);
}

void FEmitterDetails::CustomizeDetails(IDetailLayoutBuilder& InDetailLayout)
{
	this->DetailLayout = &InDetailLayout;

	InDetailLayout.EditCategory("Particles", TEXT(""),  ECategoryPriority::Important );

	IDetailCategoryBuilder& CustomCategory = InDetailLayout.EditCategory("EmitterActions", NSLOCTEXT("EmitterDetails", "EmitterActionCategoryName", "Emitter Actions").ToString(), ECategoryPriority::Important );
	
	CustomCategory.AddCustomRow( TEXT("") )
	[
		SNew(SUniformGridPanel)
		.SlotPadding(2.0f)
		+SUniformGridPanel::Slot(0,0)
		[
			SNew(SButton)
			.OnClicked(this, &FEmitterDetails::OnAutoPopulateClicked)
			.ToolTipText(NSLOCTEXT("EmitterDetails", "AutoPopulateButtonTooltip", "Copies properties from the source particle system into the instanced parameters of this emitter").ToString())
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("EmitterDetails", "AutoPopulateButton", "Expose Parameter").ToString())
			]
		]
		+SUniformGridPanel::Slot(1,0)
		[

			SNew(SButton)
			.OnClicked(this, &FEmitterDetails::OnResetEmitter)
			.ToolTipText(NSLOCTEXT("EmitterDetails", "ResetEmitterButtonTooltip", "Resets the selected emitters.").ToString())
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
					.Text(NSLOCTEXT("EmitterDetails", "ResetEmitterButton", "Reset Emitter").ToString())
			]
		]
	];
}

FReply FEmitterDetails::OnAutoPopulateClicked()
{
	const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailLayout->GetDetailsView().GetSelectedObjects();

	for (int32 Idx = 0; Idx < SelectedObjects.Num(); ++Idx)
	{
		if (SelectedObjects[Idx].IsValid())
		{
			AEmitter* Emitter = Cast<AEmitter>(SelectedObjects[Idx].Get());
			if (Emitter)
			{
				Emitter->AutoPopulateInstanceProperties();
			}
		}
	}

	return FReply::Handled();
}

FReply FEmitterDetails::OnResetEmitter()
{
	const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailLayout->GetDetailsView().GetSelectedObjects();
	// Iterate over selected Actors.
	for (int32 Idx = 0; Idx < SelectedObjects.Num(); ++Idx)
	{
		if (SelectedObjects[Idx].IsValid())
		{
			AEmitter* Emitter = Cast<AEmitter>(SelectedObjects[Idx].Get());
			if (Emitter)
			{
				UParticleSystemComponent* PSysComp = Emitter->ParticleSystemComponent;
				if (PSysComp)
				{
					PSysComp->ResetParticles();
					PSysComp->ActivateSystem();
				}
			}
		}
	}
	return FReply::Handled();
}


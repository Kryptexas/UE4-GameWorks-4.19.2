// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "BodySetupDetails.h"
#include "ScopedTransaction.h"
#include "ObjectEditorUtils.h"

#define LOCTEXT_NAMESPACE "BodySetupDetails"

TSharedRef<IDetailCustomization> FBodySetupDetails::MakeInstance()
{
	return MakeShareable( new FBodySetupDetails );
}

void FBodySetupDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	// Customize collision section
	{
		if ( DetailBuilder.GetProperty("DefaultInstance")->IsValidHandle() )
		{
			IDetailCategoryBuilder& PhysicsCategory = DetailBuilder.EditCategory("Physics");
			IDetailCategoryBuilder& CollisionCategory = DetailBuilder.EditCategory("Collision");

			TSharedPtr<IPropertyHandle> BodyInstanceHandler = DetailBuilder.GetProperty("DefaultInstance");
			DetailBuilder.HideProperty(BodyInstanceHandler);

			TSharedPtr<IPropertyHandle> CollisionTraceHandler = DetailBuilder.GetProperty("CollisionTraceFlag");
			DetailBuilder.HideProperty(CollisionTraceHandler);

			// add physics properties to physics category
			uint32 NumChildren = 0;
			BodyInstanceHandler->GetNumChildren(NumChildren);

			// Get the objects being customized so we can enable/disable editing of 'Simulate Physics'
			DetailBuilder.GetObjectsBeingCustomized(ObjectsCustomized);

			PhysicsCategory.AddCustomRow(TEXT("Mass"), false)
				.NameContent()
				[
					SNew (STextBlock)
					.Text(NSLOCTEXT("MassInKG", "MassInKG_Name", "Mass in KG"))
					.ToolTipText(NSLOCTEXT("MassInKG", "MassInKG_ToolTip", "Mass of the body in KG"))
					.Font( IDetailLayoutBuilder::GetDetailFont() )
				]
			.ValueContent()
				[
					SNew(SEditableTextBox)
					.Text(this, &FBodySetupDetails::OnGetBodyMass)
					.IsReadOnly(this, &FBodySetupDetails::IsBodyMassReadOnly)
					.Font(IDetailLayoutBuilder::GetDetailFont())
				];

			// add all properties of this now - after adding 
			for (uint32 ChildIndex=0; ChildIndex < NumChildren; ++ChildIndex)
			{
				TSharedPtr<IPropertyHandle> ChildProperty = BodyInstanceHandler->GetChildHandle(ChildIndex);
				FString Category = FObjectEditorUtils::GetCategory(ChildProperty->GetProperty());
				if (ChildProperty->GetProperty()->GetName() == TEXT("bSimulatePhysics"))
				{
					// skip bSimulatePhysics
					// this is because we don't want bSimulatePhysics to show up 
					// phat editor 
					// staitc mesh already hides everything else not interested in
					// so phat editor just should not show this option
					continue;
				}
				if (Category == TEXT("Physics"))
				{
					PhysicsCategory.AddProperty(ChildProperty);
				}
				else if (Category == TEXT("Collision"))
				{
					CollisionCategory.AddProperty(ChildProperty);
				}
			}
		}
	}
}

FText FBodySetupDetails::OnGetBodyMass() const
{
	UBodySetup* BS = NULL;

	float Mass = 0.0f;
	bool bMultipleValue = false;

	for (auto ObjectIt = ObjectsCustomized.CreateConstIterator(); ObjectIt; ++ObjectIt)
	{
		if(ObjectIt->IsValid() && (*ObjectIt)->IsA(UBodySetup::StaticClass()))
		{
			BS = Cast<UBodySetup>(ObjectIt->Get());
			
			float BSMass = BS->CalculateMass();
			if (Mass == 0.0f || FMath::Abs(Mass - BSMass) < 0.1f)
			{
				Mass = BSMass;
			}
			else
			{
				bMultipleValue = true;
				break;
			}
		}
	}

	if (bMultipleValue)
	{
		return LOCTEXT("MultipleValues", "Multiple Values");
	}

	return FText::AsNumber(Mass);
}

#undef LOCTEXT_NAMESPACE


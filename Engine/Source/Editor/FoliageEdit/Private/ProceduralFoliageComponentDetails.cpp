// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "PropertyEditing.h"
#include "ProceduralFoliageComponentDetails.h"
#include "ProceduralFoliageSpawner.h"
#include "ProceduralFoliageComponent.h"
#include "InstancedFoliage.h"
#include "FoliageEdMode.h"
#include "ScopedTransaction.h"

TSharedRef<IDetailCustomization> FProceduralFoliageComponentDetails::MakeInstance()
{
	return MakeShareable( new FProceduralFoliageComponentDetails() );

}


void FProceduralFoliageComponentDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	const FName ProceduralFoliageCategoryName("ProceduralFoliage");
	IDetailCategoryBuilder& ProceduralFoliageCategory = DetailBuilder.EditCategory(ProceduralFoliageCategoryName);

	const FText ResimulateText = NSLOCTEXT("ProceduralFoliageComponentDetails","ResimulateButtonText", "Resimulate" );

	TArray< TWeakObjectPtr<UObject> > ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized( ObjectsBeingCustomized );

	for( TWeakObjectPtr<UObject>& Object : ObjectsBeingCustomized )
	{
		UProceduralFoliageComponent* Component = Cast<UProceduralFoliageComponent>( Object.Get() );
		if( ensure( Component ) )
		{
			SelectedComponents.Add( Component );
		}
	}

	TArray<TSharedRef<IPropertyHandle>> AllProperties;
	bool bSimpleProperties = true;
	bool bAdvancedProperties = false;
	// Add all properties in the category in order
	ProceduralFoliageCategory.GetDefaultProperties(AllProperties, bSimpleProperties, bAdvancedProperties);
	for( auto& Property : AllProperties )
	{
		ProceduralFoliageCategory.AddProperty(Property);
	}

	FDetailWidgetRow& NewRow =  ProceduralFoliageCategory.AddCustomRow( ResimulateText );

	NewRow.ValueContent()
	.MaxDesiredWidth(120.f)
	[
		SNew(SButton)
		.OnClicked( this, &FProceduralFoliageComponentDetails::OnResimulateClicked )
		.ToolTipText( NSLOCTEXT("ProceduralFoliageComponentDetails","ResimulateButton_Tooltip", "Runs the procedural foliage spawner simulation. Replaces any existing instances spawned by a previous simulation." ) )
		.IsEnabled( this, &FProceduralFoliageComponentDetails::IsResimulateEnabled )
		[
			SNew( STextBlock )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text( ResimulateText )
		]
	];
}

FReply FProceduralFoliageComponentDetails::OnResimulateClicked()
{
	TSet<UProceduralFoliageSpawner*> UniqueFoliageSpawners;
	for( TWeakObjectPtr<UProceduralFoliageComponent>& Component : SelectedComponents )
	{
		if( Component.IsValid() && Component->FoliageSpawner )
		{
			if( !UniqueFoliageSpawners.Contains( Component->FoliageSpawner ) )
			{
				UniqueFoliageSpawners.Add(Component->FoliageSpawner);
			}

			FScopedTransaction Transaction(NSLOCTEXT("ProceduralFoliageComponentDetails", "Resimulate_Transaction", "Procedural Foliage Simulation"));
			TArray <FDesiredFoliageInstance> DesiredFoliageInstances;
			if (Component->SpawnProceduralContent(DesiredFoliageInstances))
			{
				FEdModeFoliage::AddInstances(Component->GetWorld(), DesiredFoliageInstances);
			}
		}
	}
	return FReply::Handled();
}

bool FProceduralFoliageComponentDetails::IsResimulateEnabled() const
{
	for(const TWeakObjectPtr<UProceduralFoliageComponent>& Component : SelectedComponents)
	{
		if(Component.IsValid() && Component->FoliageSpawner)
		{
			return true;
		}
	}

	return false;
}
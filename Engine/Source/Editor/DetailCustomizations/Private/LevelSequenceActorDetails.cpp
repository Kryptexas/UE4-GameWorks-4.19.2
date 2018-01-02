// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LevelSequenceActorDetails.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Editor.h"
#include "Toolkits/AssetEditorManager.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "IDetailsView.h"
#include "LevelSequenceActor.h"
#include "Algo/Transform.h"

#define LOCTEXT_NAMESPACE "LevelSequenceActorDetails"


TSharedRef<IDetailCustomization> FLevelSequenceActorDetails::MakeInstance()
{
	return MakeShareable( new FLevelSequenceActorDetails );
}

void AddAllSubObjectProperties(TArray<UObject*>& SubObjects, IDetailCategoryBuilder& Category, TAttribute<EVisibility> Visibility = TAttribute<EVisibility>(EVisibility::Visible))
{
	SubObjects.Remove(nullptr);
	if (!SubObjects.Num())
	{
		return;
	}

	for (const UProperty* TestProperty : TFieldRange<UProperty>(SubObjects[0]->GetClass()))
	{
		if (TestProperty->HasAnyPropertyFlags(CPF_Edit))
		{
			const bool bAdvancedDisplay = TestProperty->HasAnyPropertyFlags(CPF_AdvancedDisplay);
			const EPropertyLocation::Type PropertyLocation = bAdvancedDisplay ? EPropertyLocation::Advanced : EPropertyLocation::Common;

			IDetailPropertyRow* NewRow = Category.AddExternalObjectProperty(SubObjects, TestProperty->GetFName(), PropertyLocation);
			if (NewRow)
			{
				NewRow->Visibility(Visibility);
			}
		}
	}
}

void FLevelSequenceActorDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailLayout.GetSelectedObjects();

	for( int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex )
	{
		const TWeakObjectPtr<UObject>& CurrentObject = SelectedObjects[ObjectIndex];
		if ( CurrentObject.IsValid() )
		{
			ALevelSequenceActor* CurrentLevelSequenceActor = Cast<ALevelSequenceActor>(CurrentObject.Get());
			if (CurrentLevelSequenceActor != NULL)
			{
				LevelSequenceActor = CurrentLevelSequenceActor;
				break;
			}
		}
	}

	TArray<ALevelSequenceActor*> LevelSequenceActors;
	{
		TArray<TWeakObjectPtr<>> ObjectPtrs;
		DetailLayout.GetObjectsBeingCustomized(ObjectPtrs);

		for (TWeakObjectPtr<> WeakObj : ObjectPtrs)
		{
			if (auto* Actor = Cast<ALevelSequenceActor>(WeakObj.Get()))
			{
				LevelSequenceActors.Add(Actor);
			}
		}
	}
	

	IDetailCategoryBuilder& GeneralCategory = DetailLayout.EditCategory( "General", NSLOCTEXT("GeneralDetails", "General", "General"), ECategoryPriority::Important );

	GeneralCategory.AddCustomRow( NSLOCTEXT("LevelSequenceActorDetails", "OpenLevelSequence", "Open Level Sequence") )
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(1.f)
		.Padding(0, 5, 10, 5)
		[
			SNew(SButton)
			.ContentPadding(3)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.IsEnabled( this, &FLevelSequenceActorDetails::CanOpenLevelSequenceForActor )
			.OnClicked( this, &FLevelSequenceActorDetails::OnOpenLevelSequenceForActor )
			.Text( NSLOCTEXT("LevelSequenceActorDetails", "OpenLevelSequence", "Open Level Sequence") )
		]
	];

	TArray<UObject*> SubObjects;

	IDetailCategoryBuilder& BurnInOptionsCategory = DetailLayout.EditCategory( "BurnInOptions", LOCTEXT("BurnInOptions", "Burn In Options") ).InitiallyCollapsed(false);
	{
		SubObjects.Reset();
		Algo::Transform(LevelSequenceActors, SubObjects, &ALevelSequenceActor::BurnInOptions);

		AddAllSubObjectProperties(SubObjects, BurnInOptionsCategory);
	}

	IDetailCategoryBuilder& BindingOverridesCategory = DetailLayout.EditCategory( "BindingOverrides", LOCTEXT("BindingOverrides", "Binding Overrides") ).InitiallyCollapsed(false);
	{
		SubObjects.Reset();
		Algo::Transform(LevelSequenceActors, SubObjects, &ALevelSequenceActor::BindingOverrides);
		AddAllSubObjectProperties(SubObjects, BindingOverridesCategory);
	}

	IDetailCategoryBuilder& InstanceDataCategory = DetailLayout.EditCategory( "InstanceData", LOCTEXT("InstanceData", "Instance Data") ).InitiallyCollapsed(false);
	{
		TSharedRef<IPropertyHandle> UseInstanceData = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(ALevelSequenceActor, bOverrideInstanceData));

		DetailLayout.HideProperty(UseInstanceData);
		InstanceDataCategory.AddProperty(UseInstanceData);

		SubObjects.Reset();
		Algo::Transform(LevelSequenceActors, SubObjects, &ALevelSequenceActor::DefaultInstanceData);

		auto IsVisible = [UseInstanceData]() -> EVisibility
		{
			bool bValue = false;
			return UseInstanceData->GetValue(bValue) == FPropertyAccess::Success && bValue ? EVisibility::Visible : EVisibility::Collapsed;
		};

		TAttribute<EVisibility> Visibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda(IsVisible));
		AddAllSubObjectProperties(SubObjects, InstanceDataCategory, Visibility);
	}
}


bool FLevelSequenceActorDetails::CanOpenLevelSequenceForActor() const
{
	if( LevelSequenceActor.IsValid() )
	{
		return LevelSequenceActor.Get()->LevelSequence.IsValid();
	}
	return false;
}

FReply FLevelSequenceActorDetails::OnOpenLevelSequenceForActor()
{
	if( LevelSequenceActor.IsValid() )
	{
		UObject* LoadedObject = LevelSequenceActor.Get()->LevelSequence.TryLoad();
		if (LoadedObject != nullptr)
		{
			FAssetEditorManager::Get().OpenEditorForAsset(LoadedObject);
		}
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

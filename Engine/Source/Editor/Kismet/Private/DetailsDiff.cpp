// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "DetailsDiff.h"
#include "Modules/ModuleManager.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Algo/Copy.h"
#include "Algo/Transform.h"

FDetailsDiff::FDetailsDiff(const UObject* InObject, FOnDisplayedPropertiesChanged InOnDisplayedPropertiesChanged )
	: OnDisplayedPropertiesChanged( InOnDisplayedPropertiesChanged )
	, DisplayedObject( InObject )
	, DetailsView()
{
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bShowDifferingPropertiesOption = true;

	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	DetailsView = EditModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateStatic([]{return false; }));
	// Forcing all advanced properties to be displayed for now, the logic to show changes made to advance properties
	// conditionally is fragile and low priority for now:
	DetailsView->ShowAllAdvancedProperties();
	// This is a read only details view (see the property editing delegate), but const correctness at the details view
	// level would stress the type system a little:
	DetailsView->SetObject(const_cast<UObject*>(InObject));
	DetailsView->SetOnDisplayedPropertiesChanged( ::FOnDisplayedPropertiesChanged::CreateRaw(this, &FDetailsDiff::HandlePropertiesChanged) );

	DifferingProperties = DetailsView->GetPropertiesInOrderDisplayed();
}

FDetailsDiff::~FDetailsDiff()
{
	DetailsView->SetOnDisplayedPropertiesChanged(::FOnDisplayedPropertiesChanged());
}

void FDetailsDiff::HighlightProperty(const FPropertySoftPath& PropertyName)
{
	// resolve the property soft path:
	const FPropertyPath ResolvedProperty = PropertyName.ResolvePath(DisplayedObject);
	DetailsView->HighlightProperty(ResolvedProperty);
}

TSharedRef< SWidget > FDetailsDiff::DetailsWidget()
{
	return DetailsView.ToSharedRef();
}

void FDetailsDiff::HandlePropertiesChanged()
{
	if( OnDisplayedPropertiesChanged.IsBound() )
	{
		DifferingProperties = DetailsView->GetPropertiesInOrderDisplayed();
		OnDisplayedPropertiesChanged.Execute();
	}
}

TArray<FPropertySoftPath> FDetailsDiff::GetDisplayedProperties() const
{
	TArray<FPropertySoftPath> Ret;
	Algo::Copy(DifferingProperties, Ret);
	return Ret;
}

void FDetailsDiff::DiffAgainst(const FDetailsDiff& Newer, TArray< FSingleObjectDiffEntry > &OutDifferences) const
{
	TSharedPtr< class IDetailsView > OldDetailsView = DetailsView;
	TSharedPtr< class IDetailsView > NewDetailsView = Newer.DetailsView;

	const TArray<TWeakObjectPtr<UObject>>& OldSelectedObjects = OldDetailsView->GetSelectedObjects();
	const TArray<TWeakObjectPtr<UObject>>& NewSelectedObjects = NewDetailsView->GetSelectedObjects();

	const TArray<FPropertyPath> OldProperties = OldDetailsView->GetPropertiesInOrderDisplayed();
	const TArray<FPropertyPath> NewProperties = NewDetailsView->GetPropertiesInOrderDisplayed();

	TSet<FPropertySoftPath> OldPropertiesSet;
	TSet<FPropertySoftPath> NewPropertiesSet;

	Algo::Transform(OldProperties, OldPropertiesSet, [](const FPropertyPath& Entry) { return FPropertySoftPath(Entry); });
	Algo::Transform(NewProperties, NewPropertiesSet, [](const FPropertyPath& Entry) { return FPropertySoftPath(Entry); });

	// detect removed properties:
	const TSet<FPropertySoftPath> RemovedProperties = OldPropertiesSet.Difference(NewPropertiesSet);
	for (const FPropertySoftPath& RemovedProperty : RemovedProperties)
	{
		// @todo: (doc) label these as removed, rather than added to a
		FSingleObjectDiffEntry Entry(RemovedProperty, EPropertyDiffType::PropertyAddedToA);
		OutDifferences.Push(Entry);
	}

	// detect added properties:
	const TSet<FPropertySoftPath> AddedProperties = NewPropertiesSet.Difference(OldPropertiesSet);
	for (const FPropertySoftPath& AddedProperty : AddedProperties)
	{
		FSingleObjectDiffEntry Entry(AddedProperty, EPropertyDiffType::PropertyAddedToB);
		OutDifferences.Push(Entry);
	}

	// check for changed properties
	const TSet<FPropertySoftPath> CommonProperties = NewPropertiesSet.Intersect(OldPropertiesSet);
	for (const FPropertySoftPath& CommonProperty : CommonProperties)
	{
		// get value, diff:
		check(NewSelectedObjects.Num() == 1);
		FResolvedProperty OldProperty = CommonProperty.Resolve(OldSelectedObjects[0].Get());
		FResolvedProperty NewProperty = CommonProperty.Resolve(NewSelectedObjects[0].Get());

		TArray<FPropertySoftPath> DifferingSubProperties;

		if (!DiffUtils::Identical(OldProperty, NewProperty, CommonProperty, DifferingSubProperties))
		{
			for (const FPropertySoftPath& DifferingSubProperty : DifferingSubProperties)
			{
				OutDifferences.Push(FSingleObjectDiffEntry(DifferingSubProperty, EPropertyDiffType::PropertyValueChanged));
			}
		}
	}
}

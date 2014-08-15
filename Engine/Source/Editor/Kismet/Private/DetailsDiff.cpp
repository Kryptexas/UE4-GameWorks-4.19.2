// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "DetailsDiff.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"

FDetailsDiff::FDetailsDiff(const UObject* InObject, const TMap< FName, const UProperty* >& InPropertyMap, const TSet< FName >& InIdenticalProperties)
	: PropertyMap( InPropertyMap )
	, DetailsView()
{
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bShowDifferingPropertiesOption = true;

	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	DetailsView = EditModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateStatic([]{return false; }));
	// This is a read only details view (see the property editing delegate), but it is not const correct:
	DetailsView->SetObject(const_cast<UObject*>(InObject));
	DetailsView->UpdateIdenticalProperties(InIdenticalProperties);
}

void FDetailsDiff::HighlightProperty(FName PropertyName)
{
	const UProperty** Property = PropertyMap.Find(PropertyName);
	DetailsView->HighlightProperty( Property ? *Property : NULL );
}

TSharedRef< SWidget > FDetailsDiff::DetailsWidget()
{
	return DetailsView.ToSharedRef();
}

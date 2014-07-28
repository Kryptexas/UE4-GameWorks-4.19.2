// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "DocumentationActorDetails.h"
#include "Engine/DocumentationActor.h"
#include "ObjectEditorUtils.h"


#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyEditing.h"
#include "EditorCategoryUtils.h"


#define LOCTEXT_NAMESPACE "DocumentationActorDetails"


TSharedRef<IDetailCustomization> FDocumentationActorDetails::MakeInstance()
{
	return MakeShareable(new FDocumentationActorDetails);
}

void FDocumentationActorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	SelectedDocActor = nullptr; 
	
	const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailBuilder.GetDetailsView().GetSelectedObjects();
	
	// Find the first documentation actor -we should only open one documentation instance at a time.
	for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
	{
		if (ADocumentationActor* CurrentDocActor = Cast<ADocumentationActor>(SelectedObjects[ObjectIndex].Get()))
		{
			if (CurrentDocActor != nullptr)
			{
				if (CurrentDocActor->HasValidDocumentLink() == true)
				{
					SelectedDocActor = CurrentDocActor;
					break;
				}
			}
		}
	}

	// Add a button we can click on to open the documentation
	IDetailCategoryBuilder& HelpCategory = DetailBuilder.EditCategory("Help Data");
	HelpCategory.AddCustomRow("Help Documentation")
		[
			SNew(SButton)
			.Text(LOCTEXT("HelpDocumentation", "Help Documentation"))
			.HAlign(HAlign_Center)
			.OnClicked(this, &FDocumentationActorDetails::OnHelpButtonClicked)
		];
	
}

FReply FDocumentationActorDetails::OnHelpButtonClicked()
{
	FReply Handled = FReply::Unhandled();
	
	if (SelectedDocActor.IsValid() == true )
	{
		Handled = SelectedDocActor->OpenDocumentLink() == true ? FReply::Handled() : FReply::Unhandled();
	}
	return Handled;
}


#undef LOCTEXT_NAMESPACE

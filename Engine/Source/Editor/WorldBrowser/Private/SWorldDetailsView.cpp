// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "WorldBrowserPrivatePCH.h"

#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "SWorldDetailsView.h"

#define LOCTEXT_NAMESPACE "WorldBrowser"

void SWorldDetailsView::Construct(const FArguments& InArgs)
{
	WorldModel = InArgs._InWorldModel;
	WorldModel->SelectionChanged.AddSP(this, &SWorldDetailsView::OnSelectionChanged);
	
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	// Create properties editor pane
	FDetailsViewArgs Args;
	Args.bObjectsUseNameArea = true;

	DetailsView = PropertyModule.CreateDetailView(Args);
	ChildSlot
	[
		DetailsView.ToSharedRef()
	];

	WorldModel->RegisterDetailsCustomization(PropertyModule, DetailsView);
}

SWorldDetailsView::SWorldDetailsView()
{
}

SWorldDetailsView::~SWorldDetailsView()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	WorldModel->UnregisterDetailsCustomization(PropertyModule, DetailsView);
	WorldModel->SelectionChanged.RemoveAll(this);
}

void SWorldDetailsView::OnSelectionChanged()
{
	auto SelectedLevels = WorldModel->GetSelectedLevels();
	TArray<UObject*> TileProperties;
	
	for (auto It = SelectedLevels.CreateIterator(); It; ++It)
	{
		UObject* PropertiesObject = (*It)->GetNodeObject();
		if (PropertiesObject)
		{
			TileProperties.Add(PropertiesObject);
		}
	}

	DetailsView->SetObjects(TileProperties, true);
}

#undef LOCTEXT_NAMESPACE
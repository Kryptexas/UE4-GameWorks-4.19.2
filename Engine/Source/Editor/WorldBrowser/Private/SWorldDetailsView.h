// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"

class FLevelCollectionModel;
//----------------------------------------------------------------
//
//
//----------------------------------------------------------------
class SWorldDetailsView 
	: public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWorldDetailsView) {}
		SLATE_ARGUMENT(TSharedPtr<FLevelCollectionModel>, InWorldModel)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	SWorldDetailsView();
	~SWorldDetailsView();

private:
	/** Handles selection changes in data source */
	void OnSelectionChanged();
	
private:
	TSharedPtr<FLevelCollectionModel>	WorldModel;
	TSharedPtr<IDetailsView>			DetailsView;
};

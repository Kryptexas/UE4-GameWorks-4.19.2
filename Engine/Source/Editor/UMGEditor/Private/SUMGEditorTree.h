// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "IUMGEditor.h"
#include "BlueprintEditor.h"

//TODO rename SUMGEditorHierarchy
class SUMGEditorTree : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SUMGEditorTree ){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor, USimpleConstructionScript* InSCS);
	~SUMGEditorTree();

	FReply CreateTestUI();
	
private:

	TWeakPtr<class FBlueprintEditor> BlueprintEditor;

	TSharedPtr< STreeView< TSharedPtr< FString > > > PageTreeView;
};

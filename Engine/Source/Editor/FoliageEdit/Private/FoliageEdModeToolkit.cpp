// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "FoliageEdModeToolkit.h"
#include "SFoliageEdit.h"
#include "SFoliageEdit2.h"

#define LOCTEXT_NAMESPACE "FoliageEditMode"

static TAutoConsoleVariable<int32> CVarFoliageNewUI(
	TEXT("foliage.NewUI"),
	0,
	TEXT("Enables new foliage UI"));


void FFoliageEdModeToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{

}

void FFoliageEdModeToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{

}

void FFoliageEdModeToolkit::Init(const TSharedPtr< class IToolkitHost >& InitToolkitHost)
{
	bool NewUI = CVarFoliageNewUI.GetValueOnAnyThread() != 0;
	
	if (NewUI)
	{
		FoliageEdWidget2 = SNew(SFoliageEdit2);
	}
	else
	{
		FoliageEdWidget = SNew(SFoliageEdit);
	}
	

	FModeToolkit::Init(InitToolkitHost);
}

FName FFoliageEdModeToolkit::GetToolkitFName() const
{
	return FName("FoliageEditMode");
}

FText FFoliageEdModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT( "ToolkitName", "Foliage Edit Mode" );
}

class FEdMode* FFoliageEdModeToolkit::GetEditorMode() const
{
	return GLevelEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_Foliage);
}

TSharedPtr<SWidget> FFoliageEdModeToolkit::GetInlineContent() const
{
	if (FoliageEdWidget2.IsValid())
	{
		return FoliageEdWidget2;
	}
	else
	{
		return FoliageEdWidget;
	}
}

void FFoliageEdModeToolkit::RefreshFullList()
{
	if (FoliageEdWidget2.IsValid())
	{
		FoliageEdWidget2->RefreshFullList();
	}
	else
	{
		FoliageEdWidget->RefreshFullList();
	}
}

#undef LOCTEXT_NAMESPACE

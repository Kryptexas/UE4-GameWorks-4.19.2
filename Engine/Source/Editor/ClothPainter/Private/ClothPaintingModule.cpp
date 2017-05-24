// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ClothPaintingModule.h"

#include "SClothPaintTab.h"

#include "ModuleManager.h"
#include "PropertyEditorModule.h" 
#include "WorkflowOrientedApp/WorkflowTabFactory.h"
#include "WorkflowOrientedApp/ApplicationMode.h"

#include "EditorModeRegistry.h"
#include "ClothingPaintEditMode.h"

#include "PropertyEditorModule.h"
#include "ClothPaintSettingsCustomization.h"
#include "Settings/EditorExperimentalSettings.h"
#include "ClothPaintToolCommands.h"

#define LOCTEXT_NAMESPACE "ClothPaintingModule"

IMPLEMENT_MODULE(FClothPaintingModule, ClothPainter);

struct FClothPaintTabSummoner : public FWorkflowTabFactory
{
public:
	/** Tab ID name */
	static const FName TabName;

	FClothPaintTabSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(TabName, InHostingApp)
	{
		TabLabel = LOCTEXT("ClothPaintTabLabel", "ClothPaint");
		TabIcon = FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.SkeletalMesh");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override
	{
		return SNew(SClothPaintTab).InHostingApp(HostingApp);
	}

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override
	{
		return LOCTEXT("ClothPaintTabToolTip", "Tab for Painting Cloth properties");
	}

	static TSharedPtr<FWorkflowTabFactory> CreateFactory(TSharedPtr<FAssetEditorToolkit> InAssetEditor)
	{
		return MakeShareable(new FClothPaintTabSummoner(InAssetEditor));
	}
};
const FName FClothPaintTabSummoner::TabName = TEXT("ClothPainting");

void FClothPaintingModule::StartupModule()
{	
	ExperimentalSettings = GetMutableDefault<UEditorExperimentalSettings>();
	ExperimentalSettingsEventHandle = ExperimentalSettings->OnSettingChanged().AddRaw(this, &FClothPaintingModule::OnExperimentalSettingsChanged);

	if(ExperimentalSettings->bClothingTools)
	{
		SetupMode();
	}

	// Register any commands for the cloth painter
	ClothPaintToolCommands::RegisterClothPaintToolCommands();
}

void FClothPaintingModule::SetupMode()
{
	// Add application mode extender
	Extender = FWorkflowApplicationModeExtender::CreateRaw(this, &FClothPaintingModule::ExtendApplicationMode);
	FWorkflowCentricApplication::GetModeExtenderList().Add(Extender);

	FEditorModeRegistry::Get().RegisterMode<FClothingPaintEditMode>(PaintModeID, LOCTEXT("ClothPaintEditMode", "Cloth Painting"), FSlateIcon(), false);
}

TSharedRef<FApplicationMode> FClothPaintingModule::ExtendApplicationMode(const FName ModeName, TSharedRef<FApplicationMode> InMode)
{
	// For skeleton and animation editor modes add our custom tab factory to it
	if (ModeName == TEXT("SkeletalMeshEditorMode"))
	{
		InMode->AddTabFactory(FCreateWorkflowTabFactory::CreateStatic(&FClothPaintTabSummoner::CreateFactory));
		RegisteredApplicationModes.Add(InMode);
	}
	
	return InMode;
}

void FClothPaintingModule::OnExperimentalSettingsChanged(FName PropertyName)
{
	if(UEditorExperimentalSettings* ActualSettings = ExperimentalSettings.Get())
	{
		if(PropertyName == GET_MEMBER_NAME_CHECKED(UEditorExperimentalSettings, bClothingTools))
		{
			if(ActualSettings->bClothingTools)
			{
				SetupMode();
			}
			else
			{
				ShutdownMode();
			}
		}
	}
}

void FClothPaintingModule::ShutdownModule()
{
	if(UEditorExperimentalSettings* ActualSettings = ExperimentalSettings.Get())
	{
		if(ExperimentalSettings->bClothingTools)
		{
			ShutdownMode();
		}

		ExperimentalSettings->OnSettingChanged().Remove(ExperimentalSettingsEventHandle);
	}
	else
	{
		ShutdownMode();
	}
}

void FClothPaintingModule::ShutdownMode()
{
	// Remove extender delegate
	FWorkflowCentricApplication::GetModeExtenderList().RemoveAll([this](FWorkflowApplicationModeExtender& StoredExtender)
	{
		return StoredExtender.GetHandle() == Extender.GetHandle();
	});

	// During shutdown clean up all factories from any modes which are still active/alive
	for(TWeakPtr<FApplicationMode> WeakMode : RegisteredApplicationModes)
	{
		if(WeakMode.IsValid())
		{
			TSharedPtr<FApplicationMode> Mode = WeakMode.Pin();
			Mode->RemoveTabFactory(FClothPaintTabSummoner::TabName);
		}
	}

	RegisteredApplicationModes.Empty();

	FEditorModeRegistry::Get().UnregisterMode(PaintModeID);
}

#undef LOCTEXT_NAMESPACE // "AnimationModifiersModule"
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LandscapeEditorPrivatePCH.h"
#include "LandscapeEdMode.h"
#include "SLandscapeEditor.h"
#include "LandscapeEditorCommands.h"
#include "AssetThumbnail.h"
#include "LandscapeEditorObject.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "LandscapeEditor"

void SLandscapeAssetThumbnail::Construct(const FArguments& InArgs, UObject* Asset, TSharedRef<FAssetThumbnailPool> ThumbnailPool)
{
	FIntPoint ThumbnailSize = InArgs._ThumbnailSize;

	AssetThumbnail = MakeShareable( new FAssetThumbnail( Asset, ThumbnailSize.X, ThumbnailSize.Y, ThumbnailPool ) );

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(ThumbnailSize.X)
		.HeightOverride(ThumbnailSize.Y)
		[
			AssetThumbnail->MakeThumbnailWidget()
		]
	];

	UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>(Asset);
	if (MaterialInterface)
	{
		UMaterial::OnMaterialCompilationFinished().AddSP(this, &SLandscapeAssetThumbnail::OnMaterialCompilationFinished);
	}
}

SLandscapeAssetThumbnail::~SLandscapeAssetThumbnail()
{
	UMaterial::OnMaterialCompilationFinished().RemoveAll(this);
}

void SLandscapeAssetThumbnail::OnMaterialCompilationFinished(UMaterialInterface* MaterialInterface)
{
	UMaterialInterface* MaterialAsset = Cast<UMaterialInterface>(AssetThumbnail->GetAsset());
	if (MaterialAsset)
	{
		if (MaterialAsset->IsDependent(MaterialInterface))
		{
			// Refresh thumbnail
			AssetThumbnail->SetAsset(AssetThumbnail->GetAsset());
		}
	}
}

void SLandscapeAssetThumbnail::SetAsset( UObject* Asset )
{
	AssetThumbnail->SetAsset( Asset );
}

//////////////////////////////////////////////////////////////////////////

void FLandscapeToolKit::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
}

void FLandscapeToolKit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
}

void FLandscapeToolKit::Init(const TSharedPtr< class IToolkitHost >& InitToolkitHost)
{
	LandscapeEditorWidgets = SNew(SLandscapeEditor, SharedThis(this));

	FModeToolkit::Init(InitToolkitHost);
}

FName FLandscapeToolKit::GetToolkitFName() const
{
	return FName("LandscapeEditor");
}

FText FLandscapeToolKit::GetBaseToolkitName() const
{
	return LOCTEXT("ToolkitName", "Landscape Editor");
}

class FEdModeLandscape* FLandscapeToolKit::GetEditorMode() const
{
	return (FEdModeLandscape*)GEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_Landscape);
}

TSharedPtr<SWidget> FLandscapeToolKit::GetInlineContent() const
{
	return LandscapeEditorWidgets;
}

void FLandscapeToolKit::NotifyToolChanged()
{
	LandscapeEditorWidgets->NotifyToolChanged();
}

void FLandscapeToolKit::NotifyBrushChanged()
{
	LandscapeEditorWidgets->NotifyBrushChanged();
}

//////////////////////////////////////////////////////////////////////////

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLandscapeEditor::Construct(const FArguments& InArgs, TSharedRef<FLandscapeToolKit> InParentToolkit)
{
	CommandList = InParentToolkit->GetToolkitCommands();

	//FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	//TSharedPtr<SLevelEditor> LevelEditor = StaticCastSharedPtr<SLevelEditor>(LevelEditorModule.GetFirstLevelEditor());
	//CommandList = LevelEditor->GetLevelEditorActions();

	// Modes:
	CommandList->MapAction(FLandscapeEditorCommands::Get().ManageMode, FUIAction(FExecuteAction::CreateSP(this, &SLandscapeEditor::OnChangeMode, FName(TEXT("ToolMode_Manage"))), FCanExecuteAction::CreateSP(this, &SLandscapeEditor::IsModeEnabled, FName(TEXT("ToolMode_Manage"))), FIsActionChecked::CreateSP(this, &SLandscapeEditor::IsModeActive, FName(TEXT("ToolMode_Manage")))));
	CommandList->MapAction(FLandscapeEditorCommands::Get().SculptMode, FUIAction(FExecuteAction::CreateSP(this, &SLandscapeEditor::OnChangeMode, FName(TEXT("ToolMode_Sculpt"))), FCanExecuteAction::CreateSP(this, &SLandscapeEditor::IsModeEnabled, FName(TEXT("ToolMode_Sculpt"))), FIsActionChecked::CreateSP(this, &SLandscapeEditor::IsModeActive, FName(TEXT("ToolMode_Sculpt")))));
	CommandList->MapAction(FLandscapeEditorCommands::Get().PaintMode,  FUIAction(FExecuteAction::CreateSP(this, &SLandscapeEditor::OnChangeMode, FName(TEXT("ToolMode_Paint"))),  FCanExecuteAction::CreateSP(this, &SLandscapeEditor::IsModeEnabled, FName(TEXT("ToolMode_Paint"))),  FIsActionChecked::CreateSP(this, &SLandscapeEditor::IsModeActive, FName(TEXT("ToolMode_Paint")))));

	FToolBarBuilder ModeSwitchButtons(CommandList, FMultiBoxCustomization::None);
	{
		ModeSwitchButtons.AddToolBarButton(FLandscapeEditorCommands::Get().ManageMode, NAME_None, LOCTEXT("Mode.Manage", "Manage"), LOCTEXT("Mode.Manage.Tooltip", "Contains tools to add a new landscape, import/export landscape, add/remove components and manage streaming"));
		ModeSwitchButtons.AddToolBarButton(FLandscapeEditorCommands::Get().SculptMode, NAME_None, LOCTEXT("Mode.Sculpt", "Sculpt"), LOCTEXT("Mode.Sculpt.Tooltip", "Contains tools that modify the shape of a landscape"));
		ModeSwitchButtons.AddToolBarButton(FLandscapeEditorCommands::Get().PaintMode,  NAME_None, LOCTEXT("Mode.Paint",  "Paint"),  LOCTEXT("Mode.Paint.Tooltip",  "Contains tools that paint materials on to a landscape"));
	}

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs(false, false, false);
	DetailsViewArgs.bHideActorNameArea = true;
	DetailsPanel = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsPanel->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &SLandscapeEditor::GetIsPropertyVisible));

	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		DetailsPanel->SetObject(LandscapeEdMode->UISettings);
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 5)
		[
			SAssignNew(Error, SErrorText)
		]
		+ SVerticalBox::Slot()
		.Padding(0)
		[
			SNew(SVerticalBox)
			.IsEnabled(this, &SLandscapeEditor::GetLandscapeEditorIsEnabled)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 5)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				.HAlign(HAlign_Center)
				[
					ModeSwitchButtons.MakeWidget()
				]
			]
			+ SVerticalBox::Slot()
			.Padding(0)
			[
				DetailsPanel.ToSharedRef()
			]
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

class FEdModeLandscape* SLandscapeEditor::GetEditorMode() const
{
	return (FEdModeLandscape*)GEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_Landscape);
}

void SLandscapeEditor::UpdateErrorText() const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		if (GEditor->bIsSimulatingInEditor)
		{
			if (LandscapeEdMode->NewLandscapePreviewMode != ENewLandscapePreviewMode::None)
			{
				Error->SetError(LOCTEXT("IsSimulatingError_create", "Can't create landscape while simulating!"));
			}
			else
			{
				Error->SetError(LOCTEXT("IsSimulatingError_edit", "Can't edit landscape while simulating!"));
			}
			return;
		}
		else if (GEditor->PlayWorld != NULL)
		{
			if (LandscapeEdMode->NewLandscapePreviewMode != ENewLandscapePreviewMode::None)
			{
				Error->SetError(LOCTEXT("IsPIEError_create", "Can't create landscape in PIE!"));
			}
			else
			{
				Error->SetError(LOCTEXT("IsPIEError_edit", "Can't edit landscape in PIE!"));
			}
			return;
		}
		else
		{
			if (LandscapeEdMode->NewLandscapePreviewMode == ENewLandscapePreviewMode::None &&
				!LandscapeEdMode->CurrentToolTarget.LandscapeInfo.IsValid())
			{
				Error->SetError(LOCTEXT("NoLandscapeError", "No Landscape!"));
				return;
			}
		}
	}

	Error->SetError(FText::GetEmpty());
}

bool SLandscapeEditor::GetLandscapeEditorIsEnabled() const
{
	UpdateErrorText();

	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		if (GEditor->PlayWorld == NULL)
		{
			return true;
		}
	}

	return false;
}

bool SLandscapeEditor::GetIsPropertyVisible(const UProperty* const Property) const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL && LandscapeEdMode->CurrentToolSet != NULL)
	{
		if (Property->HasMetaData("ShowForMask"))
		{
			const bool bMaskEnabled = LandscapeEdMode->CurrentToolSet->GetTool() &&
				LandscapeEdMode->CurrentToolSet->GetTool()->SupportsMask() &&
				LandscapeEdMode->CurrentToolTarget.LandscapeInfo.IsValid() &&
				LandscapeEdMode->CurrentToolTarget.LandscapeInfo->SelectedRegion.Num() > 0;

			if (bMaskEnabled)
			{
				return true;
			}
		}
		if (Property->HasMetaData("ShowForTools"))
		{
			const FName CurrentToolName = LandscapeEdMode->CurrentToolSet->GetToolSetName();

			TArray<FString> ShowForTools;
			Property->GetMetaData("ShowForTools").ParseIntoArray(&ShowForTools, TEXT(","), true);
			if (!ShowForTools.Contains(CurrentToolName.ToString()))
			{
				return false;
			}
		}
		if (Property->HasMetaData("ShowForBrushes"))
		{
			const FName CurrentBrushSetName = LandscapeEdMode->LandscapeBrushSets[LandscapeEdMode->CurrentBrushSetIndex].BrushSetName;
	//		const FName CurrentBrushName = LandscapeEdMode->CurrentBrush->GetBrushName();

			TArray<FString> ShowForBrushes;
			Property->GetMetaData("ShowForBrushes").ParseIntoArray(&ShowForBrushes, TEXT(","), true);
			if (!ShowForBrushes.Contains(CurrentBrushSetName.ToString()))
				//&& !ShowForBrushes.Contains(CurrentBrushName.ToString())
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

void SLandscapeEditor::OnChangeMode(FName ModeName)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		LandscapeEdMode->SetCurrentToolMode(ModeName);
	}
}

bool SLandscapeEditor::IsModeEnabled(FName ModeName) const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		// Manage is the only mode enabled if we have no landscape
		if (ModeName == "ToolMode_Manage" || LandscapeEdMode->GetLandscapeList().Num() > 0)
		{
			return true;
		}
	}

	return false;
}

bool SLandscapeEditor::IsModeActive(FName ModeName) const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolSet)
	{
		return LandscapeEdMode->CurrentToolMode->ToolModeName == ModeName;
	}

	return false;
}

void SLandscapeEditor::NotifyToolChanged()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		// Refresh details panel
		DetailsPanel->SetObject(LandscapeEdMode->UISettings, true);
	}
}

void SLandscapeEditor::NotifyBrushChanged()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		// Refresh details panel
		DetailsPanel->SetObject(LandscapeEdMode->UISettings, true);
	}
}

#undef LOCTEXT_NAMESPACE

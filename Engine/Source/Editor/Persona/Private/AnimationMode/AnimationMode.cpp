// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "PersonaPrivatePCH.h"

#include "Persona.h"
#include "Editor/Kismet/Public/BlueprintEditorTabs.h"
#include "Editor/KismetWidgets/Public/SSingleObjectDetailsPanel.h"

#include "AnimationMode.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "PersonaAnimationMode"

/////////////////////////////////////////////////////
// SAnimAssetPropertiesTabBody

class SAssetPropertiesTabBody : public SSingleObjectDetailsPanel
{
public:
	SLATE_BEGIN_ARGS(SAssetPropertiesTabBody) {}

	SLATE_ARGUMENT(FOnGetAsset, OnGetAsset)

	SLATE_ARGUMENT(FOnDetailsCreated, OnDetailsCreated)

	SLATE_END_ARGS()

private:
	FOnGetAsset OnGetAsset;

public:
	void Construct(const FArguments& InArgs)
	{
		OnGetAsset = InArgs._OnGetAsset;

		SSingleObjectDetailsPanel::Construct(SSingleObjectDetailsPanel::FArguments(), true, true);

		InArgs._OnDetailsCreated.ExecuteIfBound(PropertyView.ToSharedRef());
	}

	virtual EVisibility GetAssetDisplayNameVisibility() const
	{
		return (GetObjectToObserve() != NULL) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	virtual FText GetAssetDisplayName() const
	{
		if (UObject* Object = GetObjectToObserve())
		{
			return FText::FromString(Object->GetName());
		}
		else
		{
			return FText::GetEmpty();
		}
	}

	// SSingleObjectDetailsPanel interface
	virtual UObject* GetObjectToObserve() const override
	{
		if (OnGetAsset.IsBound())
		{
			return OnGetAsset.Execute();
	}
		return nullptr;
	}
	// End of SSingleObjectDetailsPanel interface
};

FAssetPropertiesSummoner::FAssetPropertiesSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp, FOnGetAsset InOnGetAsset, FOnDetailsCreated InOnDetailsCreated)
	: FWorkflowTabFactory(FPersonaTabs::AnimAssetPropertiesID, InHostingApp)
	, OnGetAsset(InOnGetAsset)
	, OnDetailsCreated(InOnDetailsCreated)
{
	TabLabel = LOCTEXT("AssetProperties_TabTitle", "Asset Details");
	TabIcon = FSlateIcon(FEditorStyle::GetStyleSetName(), "Persona.Tabs.AnimAssetDetails");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("AssetProperties_MenuTitle", "Asset Details");
	ViewMenuTooltip = LOCTEXT("AssetProperties_MenuToolTip", "Shows the asset properties");
}

TSharedPtr<SToolTip> FAssetPropertiesSummoner::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return  IDocumentation::Get()->CreateToolTip(LOCTEXT("AssetPropertiesTooltip", "The Asset Details tab lets you edit properties of the current asset (animation, blend space etc)."), NULL, TEXT("Shared/Editors/Persona"), TEXT("AnimationAssetDetail_Window"));
}

TSharedRef<SWidget> FAssetPropertiesSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SAssetPropertiesTabBody)
		.OnGetAsset(OnGetAsset)
		.OnDetailsCreated(OnDetailsCreated);
}

/////////////////////////////////////////////////////
// FAnimEditAppMode

FAnimEditAppMode::FAnimEditAppMode(TSharedPtr<FPersona> InPersona)
	: FPersonaAppMode(InPersona, FPersonaModes::AnimationEditMode)
{
	PersonaTabFactories.RegisterFactory(MakeShareable(new FSelectionDetailsSummoner(InPersona)));
	PersonaTabFactories.RegisterFactory(MakeShareable(new FAssetPropertiesSummoner(InPersona, FOnGetAsset::CreateSP(InPersona.Get(), &FPersona::HandleGetObject), FOnDetailsCreated())));

	TabLayout = FTabManager::NewLayout("Persona_AnimEditMode_Layout_v7")
		->AddArea
		(
			FTabManager::NewPrimaryArea() ->SetOrientation(Orient_Vertical)
			->Split
			(
				// Top toolbar area
				FTabManager::NewStack()
				->SetSizeCoefficient(0.186721f)
				->SetHideTabWell(true)
				->AddTab( InPersona->GetToolbarTabId(), ETabState::OpenedTab )
			)
			->Split
			(
				// Rest of screen
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					// Left 1/3rd - Skeleton and Anim properties
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.3f)
					->Split
					(
						FTabManager::NewStack()
						->AddTab( FPersonaTabs::SkeletonTreeViewID, ETabState::OpenedTab )
					)
					->Split
					(
						FTabManager::NewStack()
						->AddTab( FPersonaTabs::AnimAssetPropertiesID, ETabState::OpenedTab )
					)
				)
				->Split
				(
					// Middle 1/3rd - Viewport and anim document area
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.4f)
					->Split
					(
						FTabManager::NewStack()
						->SetHideTabWell(true)
						->AddTab( FPersonaTabs::PreviewViewportID, ETabState::OpenedTab )
					)
					->Split
					(
						FTabManager::NewStack()
						->AddTab( "Document", ETabState::ClosedTab )
					)
				)
				->Split
				(
					// Right 1/3rd - Details panel and quick browser
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.3f)
					->Split
					(
						FTabManager::NewStack()
						->AddTab(FPersonaTabs::AdvancedPreviewSceneSettingsID, ETabState::OpenedTab)
						->AddTab( FBlueprintEditorTabs::DetailsID, ETabState::OpenedTab )
					)
					->Split
					(
						FTabManager::NewStack()
						->AddTab( FPersonaTabs::AssetBrowserID, ETabState::OpenedTab )
					)
				)
			)
		);
}

#undef LOCTEXT_NAMESPACE
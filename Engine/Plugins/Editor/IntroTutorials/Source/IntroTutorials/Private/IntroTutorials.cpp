// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "SIntroTutorials.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Editor/MainFrame/Public/MainFrame.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/Kismet/Public/BlueprintEditorModule.h"
#include "../Plugins/Editor/EpicSurvey/Source/EpicSurvey/Public/IEpicSurveyModule.h"
#include "Editor/GameProjectGeneration/Public/GameProjectGenerationModule.h"
#include "Editor/UnrealEd/Public/EditorModes.h"
#include "Editor/UnrealEd/Public/SourceCodeNavigation.h"
#include "Editor/Matinee/Public/MatineeModule.h"

#include "EngineAnalytics.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"

#define LOCTEXT_NAMESPACE "IntroTutorials"

IMPLEMENT_MODULE(FIntroTutorials, IntroTutorials)

const FName FIntroTutorials::IntroTutorialTabName(TEXT("IntroTutorial"));

const FString FIntroTutorials::IntroTutorialConfigSection(TEXT("IntroTutorials"));
const FString FIntroTutorials::InEditorTutorialPath(TEXT("Shared/Tutorials/inEditorTutorial"));
const FString FIntroTutorials::WelcomeTutorialPath(TEXT("Shared/Tutorials/UE4Welcome"));
const FString FIntroTutorials::InEditorGamifiedTutorialPath(TEXT("Shared/Tutorials/inEditorGamifiedTutorial"));
const FString FIntroTutorials::HomePath(TEXT("Shared/Tutorials"));
const FString FIntroTutorials::BlueprintHomePath(TEXT("Shared/Tutorials/InBlueprintEditorTutorial"));

const FWelcomeTutorialProperties FIntroTutorials::UE4WelcomeTutorial(TEXT("Shared/Tutorials/UE4Welcome"), TEXT("SeenUE4Welcome"));	
const FWelcomeTutorialProperties FIntroTutorials::BlueprintHomeTutorial(TEXT("Shared/Tutorials/InBlueprintEditorTutorial"), TEXT("SeenBlueprintWelcome"));
const FWelcomeTutorialProperties FIntroTutorials::ClassBlueprintWelcomeTutorial(TEXT("Shared/Tutorials/InBlueprintEditorTutorial"), TEXT("SeenBlueprintWelcome_Class"), FString("5DCD4D58-9C9F-407F-8388-75D89CBBA7E8"));
const FWelcomeTutorialProperties FIntroTutorials::MacroLibraryBlueprintWelcomeTutorial(TEXT("Shared/Tutorials/BlueprintMacroLibInterfaceTutorial"), TEXT("SeenBlueprintWelcome_MacroLib"), FString("0D5081E0-F29A-4C35-B4DC-1E5E2825AB54"));
const FWelcomeTutorialProperties FIntroTutorials::InterfaceBlueprintWelcomeTutorial(TEXT("Shared/Tutorials/BlueprintInterfacesInterfaceTutorial"), TEXT("SeenBlueprintWelcome_Interface"), FString("E86E8C9A-4804-4680-B10F-BDC8E95C7AFD"));
const FWelcomeTutorialProperties FIntroTutorials::LevelScriptBlueprintWelcomeTutorial(TEXT("Shared/Tutorials/LevelBlueprintInterfaceTutorial"), TEXT("SeenBlueprintWelcome_LevelScript"), FString("B061E309-517D-4916-BFCB-E8104C8F4C35"));
const FWelcomeTutorialProperties FIntroTutorials::AddCodeToProjectWelcomeTutorial(TEXT("Shared/Tutorials/AddCodeToProjectTutorial"), TEXT("SeenAddCodeToProjectWelcome"), FString("D8D9A7E7-68B3-4CBE-80FE-4B88C47B7524"));
const FWelcomeTutorialProperties FIntroTutorials::MatineeEditorWelcomeTutorial(TEXT("Shared/Tutorials/InMatineeEditorTutorial"), TEXT("SeenMatineeEditorWelcome"), FString("6439C991-A77B-4B52-953D-3F29B1DD1860"));

FIntroTutorials::FIntroTutorials()
{
	bEnablePostTutorialSurveys = false;
}

/** Generates an analytics name for the given tutorial path string */
FString FIntroTutorials::AnalyticsEventNameFromTutorialPath(const FString& TutorialPath) const
{
	// strip off everything but the last folder, supporting both types of slashes
	FString RightStr;
	bool bSplit = TutorialPath.Split( TEXT("/"), NULL, &RightStr, ESearchCase::IgnoreCase, ESearchDir::FromEnd );
	if (!bSplit)
	{
		TutorialPath.Split( TEXT("\\"), NULL, &RightStr, ESearchCase::IgnoreCase, ESearchDir::FromEnd );
	}

	// then append that to the header
	// e.g. Rocket.Tutorials.ClosedInEditorTutorial
	FString OutStr(TEXT("Rocket.Tutorials.Closed"));
	OutStr += RightStr;

	return OutStr;
}

TSharedRef<FExtender> FIntroTutorials::AddSummonBlueprintTutorialsMenuExtender(const TSharedRef<FUICommandList> CommandList, const TArray<UObject*> EditingObjects) const
{
	UObject* PrimaryObject = nullptr;
	if(EditingObjects.Num() > 0)
	{
		PrimaryObject = EditingObjects[0];
	}

	TSharedRef<FExtender> Extender(new FExtender());

	Extender->AddMenuExtension(
		"HelpBrowse",
		EExtensionHook::After,
		CommandList,
		FMenuExtensionDelegate::CreateRaw(this, &FIntroTutorials::AddSummonBlueprintTutorialsMenuExtension, PrimaryObject));

	return Extender;
}

void FIntroTutorials::StartupModule()
{
	// This code can run with content commandlets. Slate is not initialized with commandlets and the below code will fail.
	if (!IsRunningCommandlet())
	{
		// Add tutorial for main frame opening
		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		MainFrameModule.OnMainFrameCreationFinished().AddRaw(this, &FIntroTutorials::MainFrameLoad);
		MainFrameModule.OnMainFrameSDKNotInstalled().AddRaw(this, &FIntroTutorials::HandleSDKNotInstalled);
		
		// Add menu option for level editor tutorial
		MainMenuExtender = MakeShareable(new FExtender);
		MainMenuExtender->AddMenuExtension("HelpBrowse", EExtensionHook::After, NULL, FMenuExtensionDelegate::CreateRaw(this, &FIntroTutorials::AddSummonTutorialsMenuExtension));
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>( "LevelEditor" );
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MainMenuExtender);

		// Add menu option to blueprint editor as well
		FBlueprintEditorModule& BPEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>( "Kismet" );
		BPEditorModule.GetMenuExtensibilityManager()->GetExtenderDelegates().Add(FAssetEditorExtender::CreateRaw(this, &FIntroTutorials::AddSummonBlueprintTutorialsMenuExtender));
		
		// Add hook for when specific asset editor is opened
		FAssetEditorManager::Get().OnAssetEditorOpened().AddRaw(this, &FIntroTutorials::OnAssetEditorOpened);

		IMatineeModule::Get().OnMatineeEditorOpened().AddRaw(this, &FIntroTutorials::OnMatineeEditorOpened);

		// Add hook for when AddToCodeProject dialog window is opened
		FGameProjectGenerationModule::Get().OnAddCodeToProjectDialogOpened().AddRaw(this, &FIntroTutorials::OnAddCodeToProjectDialogOpened);

		// Add hook for when editor changes modes (e.g. Place/Paint/Landscape/Foliage)
		GEditorModeTools().OnEditorModeChanged().AddRaw(this, &FIntroTutorials::OnEditorModeChanged);

		FSourceCodeNavigation::AccessOnCompilerNotFound().AddRaw( this, &FIntroTutorials::HandleCompilerNotFound );

		// set up some tutorial data

		// NOTE: we match classes based on an "is-a" relationship, so place more derived classes in the map before base classes so they are encountered first
		AssetEditorTutorialPropertyMap.Add(UTexture::StaticClass(), FWelcomeTutorialProperties(TEXT("Shared/Tutorials/InTextureEditorTutorial"), TEXT("SeenTextureEditorWelcome"), FString("6CC90D96-03D6-4847-B1DF-EB3018C36C4E")));
		AssetEditorTutorialPropertyMap.Add(UMaterial::StaticClass(), FWelcomeTutorialProperties(TEXT("Shared/Tutorials/InMaterialEditorTutorial"), TEXT("SeenMaterialEditorWelcome"), FString("2290389E-A428-46C1-B9BD-A7F110CDF83E")));
		AssetEditorTutorialPropertyMap.Add(UAnimBlueprint::StaticClass(), FWelcomeTutorialProperties(TEXT("Shared/Tutorials/InPersonaAnimBlueprintEditorTutorial"), TEXT("SeenPersonaAnimBlueprintEditorWelcome"), FString("FE5CD131-E4AF-4FE6-9269-12B573B66CA8")));
		AssetEditorTutorialPropertyMap.Add(UBlueprint::StaticClass(), FWelcomeTutorialProperties(FWelcomeTutorialProperties::FWelcomeTutorialPropertiesChooserDelegate::CreateRaw(this, &FIntroTutorials::ChooseBlueprintWelcomeTutorial)));
		AssetEditorTutorialPropertyMap.Add(UStaticMesh::StaticClass(), FWelcomeTutorialProperties(TEXT("Shared/Tutorials/InStaticMeshEditorTutorial"), TEXT("SeenStaticMeshEditorWelcome"), FString("762FCAD0-9384-4A6B-8D57-92DA771AD890")));
		AssetEditorTutorialPropertyMap.Add(USkeletalMesh::StaticClass(), FWelcomeTutorialProperties(TEXT("Shared/Tutorials/InSkeletalMeshEditorTutorial"), TEXT("SeenSkeletalMeshEditorWelcome"), FString("F8BF69A0-391F-4612-B289-B2B2B3F7F428")));
		AssetEditorTutorialPropertyMap.Add(UParticleSystem::StaticClass(), FWelcomeTutorialProperties(TEXT("Shared/Tutorials/InParticleSystemEditorTutorial"), TEXT("SeenParticleSystemEditorWelcome"), FString("B5ACB9D0-229B-4DA2-A3D6-001A368A48B1")));
		AssetEditorTutorialPropertyMap.Add(UDestructibleMesh::StaticClass(), FWelcomeTutorialProperties(TEXT("Shared/Tutorials/InDestructibleMeshEditorTutorial"), TEXT("SeenDestructibleMeshEditorWelcome"), FString("415C385F-C7D3-4CE8-BCC2-F19BB512AF06")));
		AssetEditorTutorialPropertyMap.Add(USoundCue::StaticClass(), FWelcomeTutorialProperties(TEXT("Shared/Tutorials/InSoundCueEditorTutorial"), TEXT("SeenSoundCueEditorWelcome"), FString("3C12B4B3-36F8-48D2-A8B0-874CCD5D3891")));
		AssetEditorTutorialPropertyMap.Add(UMaterialInstance::StaticClass(), FWelcomeTutorialProperties(TEXT("Shared/Tutorials/InMaterialInstanceEditorTutorial"), TEXT("SeenMaterialInstanceEditorWelcome"), FString("A1AAC488-8389-4B71-925D-7A2CFB65F8D4")));
		AssetEditorTutorialPropertyMap.Add(UPhysicsAsset::StaticClass(), FWelcomeTutorialProperties(TEXT("Shared/Tutorials/InPhATEditorTutorial"), TEXT("SeenPhATEditorWelcome"), FString("B576D8AB-9248-4FE4-80F7-96EDA87BBAD3")));

		// note these 3 go to the same editor
		AssetEditorTutorialPropertyMap.Add(UAnimMontage::StaticClass(), FWelcomeTutorialProperties(TEXT("Shared/Tutorials/InPersonaAnimEditorTutorial"), TEXT("SeenPersonaAnimEditorWelcome"), FString("8C7C3772-9135-426C-93A7-5937C5CBEA7B")));
		AssetEditorTutorialPropertyMap.Add(UAnimSequence::StaticClass(), FWelcomeTutorialProperties(TEXT("Shared/Tutorials/InPersonaAnimEditorTutorial"), TEXT("SeenPersonaAnimEditorWelcome"), FString("8C7C3772-9135-426C-93A7-5937C5CBEA7B")));
		AssetEditorTutorialPropertyMap.Add(UBlendSpace::StaticClass(), FWelcomeTutorialProperties(TEXT("Shared/Tutorials/InPersonaAnimEditorTutorial"), TEXT("SeenPersonaAnimEditorWelcome"), FString("8C7C3772-9135-426C-93A7-5937C5CBEA7B")));

		// map editor modes to tutorial properties
		//@todo Placement mode appears to be firing erroneously when switching to other modes, want to understand why before enabling this mode
		//EditorModeTutorialPropertyMap.Add(FBuiltinEditorModes::EM_Placement, FWelcomeTutorialProperties(TEXT("Shared/Tutorials/EditorPlacementModeTutorial"), TEXT("SeenEditorPlacementModeWelcome")));
		EditorModeTutorialPropertyMap.Add(FBuiltinEditorModes::EM_Landscape, FWelcomeTutorialProperties(TEXT("Shared/Tutorials/EditorLandscapeModeTutorial"), TEXT("SeenEditorLandscapeModeWelcome"), FString("8B33592A-CCEE-4129-AE3F-77A5B68955CB")));
		EditorModeTutorialPropertyMap.Add(FBuiltinEditorModes::EM_MeshPaint, FWelcomeTutorialProperties(TEXT("Shared/Tutorials/EditorMeshPaintModeTutorial"), TEXT("SeenEditorMeshPaintModeWelcome"), FString("8E36B45D-D249-42E1-ABEE-2ABCBCE8ED26")));
		EditorModeTutorialPropertyMap.Add(FBuiltinEditorModes::EM_Foliage, FWelcomeTutorialProperties(TEXT("Shared/Tutorials/EditorFoliageModeTutorial"), TEXT("SeenEditorFoliageModeWelcome"), FString("9DD00A25-16D5-40D5-BDC4-98DF70CEB0F7")));

		// init survey map
		{
			for (auto It = AssetEditorTutorialPropertyMap.CreateConstIterator(); It; ++It)
			{
				const FWelcomeTutorialProperties& Props = It.Value();
				TutorialSurveyMap.Add(Props.TutorialPath, Props.SurveyGuid);
			}
			for (auto It = EditorModeTutorialPropertyMap.CreateConstIterator(); It; ++It)
			{
				const FWelcomeTutorialProperties& Props = It.Value();
				TutorialSurveyMap.Add(Props.TutorialPath, Props.SurveyGuid);
			}
			TutorialSurveyMap.Add(ClassBlueprintWelcomeTutorial.TutorialPath, ClassBlueprintWelcomeTutorial.SurveyGuid);
			TutorialSurveyMap.Add(MacroLibraryBlueprintWelcomeTutorial.TutorialPath, MacroLibraryBlueprintWelcomeTutorial.SurveyGuid);
			TutorialSurveyMap.Add(InterfaceBlueprintWelcomeTutorial.TutorialPath, InterfaceBlueprintWelcomeTutorial.SurveyGuid);
			TutorialSurveyMap.Add(LevelScriptBlueprintWelcomeTutorial.TutorialPath, LevelScriptBlueprintWelcomeTutorial.SurveyGuid);
			TutorialSurveyMap.Add(AddCodeToProjectWelcomeTutorial.TutorialPath, AddCodeToProjectWelcomeTutorial.SurveyGuid);
			TutorialSurveyMap.Add(MatineeEditorWelcomeTutorial.TutorialPath, MatineeEditorWelcomeTutorial.SurveyGuid);

			FGuid SurveyGuid;

			// set up some paths for surveys and analytics
			SurveyGuid.ParseExact(FString("B6C19629-3172-4924-91FE-AC557F66703C"), EGuidFormats::DigitsWithHyphens, SurveyGuid);
			TutorialSurveyMap.Add(InEditorTutorialPath, SurveyGuid);

			SurveyGuid.ParseExact(FString("3ADE38A2-9FCA-4BA1-B0A3-F64DDFAB2A0E"), EGuidFormats::DigitsWithHyphens, SurveyGuid);
			TutorialSurveyMap.Add(InEditorGamifiedTutorialPath, SurveyGuid);
		}

		// maybe reset all the "have I seen this once" flags
		if (FParse::Param(FCommandLine::Get(), TEXT("tutorials")))
		{
			ResetWelcomeTutorials();
		}
	}
}

void FIntroTutorials::ShutdownModule()
{
	FSourceCodeNavigation::AccessOnCompilerNotFound().RemoveAll( this );

	GEditorModeTools().OnEditorModeChanged().RemoveAll(this);

	if (BlueprintEditorExtender.IsValid() && FModuleManager::Get().IsModuleLoaded("Kismet"))
	{
		FBlueprintEditorModule& BPEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
		BPEditorModule.GetMenuExtensibilityManager()->RemoveExtender(BlueprintEditorExtender);
	}
	if (MainMenuExtender.IsValid() && FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>( "LevelEditor" );
		LevelEditorModule.GetMenuExtensibilityManager()->RemoveExtender(MainMenuExtender);
	}
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		MainFrameModule.OnMainFrameCreationFinished().RemoveAll(this);
	}
}

void FIntroTutorials::AddSummonTutorialsMenuExtension(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("Tutorials", LOCTEXT("TutorialsLabel", "Tutorials"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("TutorialsMenuEntryTitle", "Tutorials"),
		LOCTEXT("TutorialsMenuEntryToolTip", "Opens up introductory tutorials covering the basics of using the Unreal Engine 4 Editor."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tutorials"),
		FUIAction(FExecuteAction::CreateRaw(this, &FIntroTutorials::SummonTutorialHome)));
	MenuBuilder.EndSection();
}

void FIntroTutorials::AddSummonBlueprintTutorialsMenuExtension(FMenuBuilder& MenuBuilder, UObject* PrimaryObject)
{
	MenuBuilder.BeginSection("Tutorials", LOCTEXT("TutorialsLabel", "Tutorials"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("BlueprintMenuEntryTitle", "Blueprint Overview"),
		LOCTEXT("BlueprintMenuEntryToolTip", "Opens up an introductory overview of Blueprints."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tutorials"),
		FUIAction(FExecuteAction::CreateRaw(this, &FIntroTutorials::SummonBlueprintTutorialHome, (UObject*)nullptr)));

	if(PrimaryObject != nullptr)
	{
		UBlueprint* BP = Cast<UBlueprint>(PrimaryObject);
		if(BP != nullptr)
		{
			UEnum* Enum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EBlueprintType"), true);
			check(Enum);
			MenuBuilder.AddMenuEntry(
				FText::Format(LOCTEXT("BlueprintTutorialsMenuEntryTitle", "{0} Tutorial"), Enum->GetEnumText(BP->BlueprintType)),
				LOCTEXT("BlueprintTutorialsMenuEntryToolTip", "Opens up an introductory tutorial covering this particular part of the Blueprint editor."),
				FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tutorials"),
				FUIAction(FExecuteAction::CreateRaw(this, &FIntroTutorials::SummonBlueprintTutorialHome, PrimaryObject)));
		}
	}

	MenuBuilder.EndSection();
}

void FIntroTutorials::MainFrameLoad(TSharedPtr<SWindow> InRootWindow, bool bIsNewProjectWindow)
{
	if (!bIsNewProjectWindow)
	{
		// Save off pointer to root window so we can parent the tutorial window to it when summoned
		if (InRootWindow.IsValid())
		{
			RootWindow = InRootWindow;
		}

		// See if we should show 'welcome' screen
		MaybeOpenWelcomeTutorial(UE4WelcomeTutorial.TutorialPath, UE4WelcomeTutorial.SeenOnceSettingName);
	}
}

void FIntroTutorials::SummonTutorialHome()
{
	SummonTutorialWindowForPage(HomePath);
}

void FIntroTutorials::SummonBlueprintTutorialHome(UObject* Asset)
{
	FWelcomeTutorialProperties const* Tutorial = ChooseBlueprintWelcomeTutorial(Asset);
	check(Tutorial);
	SummonTutorialWindowForPage(Tutorial->TutorialPath);

	// make sure weve seen this tutorial
	GConfig->SetBool(*IntroTutorialConfigSection, *Tutorial->SeenOnceSettingName, true, GEditorGameAgnosticIni);
}

void FIntroTutorials::SummonTutorialWindowForPage(const FString& Path)
{
	TSharedPtr<SWindow> Window;

	// If window already exists, bring it to the front
	if (TutorialWindow.IsValid())
	{
		Window = TutorialWindow.Pin();
		Window->BringToFront();
	}
	// If not, create a new window
	else
	{
		// Window
		Window = SNew(SWindow)
			.Title(LOCTEXT( "WindowTitle", "Tutorials" ))
			.ClientSize(FVector2D(660.f, 637.f))
			.SupportsMaximize(false)
			.SupportsMinimize(false);

		TutorialWindow = Window;

		if(RootWindow.IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(Window.ToSharedRef(), RootWindow.Pin().ToSharedRef());
		}

		// tutorial Widget
		TSharedRef<SIntroTutorials> TutorialWidgetRef = 
			SNew(SIntroTutorials) 
			.ParentWindow(Window)
			.HomeButtonVisibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateRaw(this, &FIntroTutorials::GetHomeButtonVisibility)))
			.OnGotoNextTutorial(FOnGotoNextTutorial::CreateRaw(this, &FIntroTutorials::HandleGotoNextTutorial));

		Window->SetContent(TutorialWidgetRef);

		Window->SetOnWindowClosed( FOnWindowClosed::CreateRaw(this, &FIntroTutorials::OnTutorialWindowClosed) );
		TutorialWidgetRef->SetOnGoHome( FOnGoHome::CreateRaw(this, &FIntroTutorials::OnTutorialDismissed) );

		TutorialWidget = TutorialWidgetRef;

		Window->BringToFront();
	}

	// Change page to desired path
	if(TutorialWidget.IsValid())
	{
		TutorialWidget.Pin()->ChangePage(Path);
	}
}

void FIntroTutorials::OnTutorialWindowClosed(const TSharedRef<SWindow>& Window)
{
	OnTutorialDismissed();
}

void FIntroTutorials::OnTutorialDismissed() const
{
	TSharedPtr<SIntroTutorials> WidgetPtr = TutorialWidget.Pin();
	if (WidgetPtr.IsValid())
	{
		FString const CurrentPagePath = WidgetPtr->GetCurrentPagePath();

		// submit analytics data
		if( FEngineAnalytics::IsAvailable() )
		{
			// prepare and send analytics data
			bool const bQuitOnWelcomeScreen = (CurrentPagePath == WelcomeTutorialPath);

			FString const CurrentExcerptTitle = bQuitOnWelcomeScreen ? TEXT("Welcome Screen") : WidgetPtr->GetCurrentExcerptTitle();
			int32 const CurrentExcerptIndex = bQuitOnWelcomeScreen ? -1 : WidgetPtr->GetCurrentExcerptIndex();
			float const CurrentPageElapsedTime = bQuitOnWelcomeScreen ? 0.f : WidgetPtr->GetCurrentPageElapsedTime();

			TArray<FAnalyticsEventAttribute> EventAttributes;
			EventAttributes.Add(FAnalyticsEventAttribute(TEXT("LastExcerptIndex"), CurrentExcerptIndex));
			EventAttributes.Add(FAnalyticsEventAttribute(TEXT("LastExcerptTitle"), CurrentExcerptTitle));
			EventAttributes.Add(FAnalyticsEventAttribute(TEXT("TimeSpentInTutorial"), CurrentPageElapsedTime));

			FString AnalyticsEventName = AnalyticsEventNameFromTutorialPath(CurrentPagePath);
			
			FEngineAnalytics::GetProvider().RecordEvent( AnalyticsEventName, EventAttributes );
		}

		// kick off survey, but not if it was dismissed immediately
		if ( bEnablePostTutorialSurveys && IEpicSurveyModule::IsAvailable() && (WidgetPtr->GetCurrentExcerptIndex() > 0) )
		{
			FGuid const* const SurveyID = TutorialSurveyMap.Find(CurrentPagePath);
			if (SurveyID && !SurveyID->IsValid())
			{
				// launch end-of-tutorial survey if one is desired
				IEpicSurveyModule::Get().PromptSurvey(*SurveyID);
			}
		}
	}	
}

void FIntroTutorials::OnAssetEditorOpened(UObject* Asset)
{
	FWelcomeTutorialProperties const* const FoundProps = FindAssetEditorTutorialProperties(Asset->GetClass());
	if (FoundProps)
	{
		// run delegate if it exists
		FWelcomeTutorialProperties const* const PropsToUse = FoundProps->ChooserDelegate.IsBound()
			? FoundProps->ChooserDelegate.Execute(Asset)
			: FoundProps;

		if (PropsToUse)
		{
			MaybeOpenWelcomeTutorial(PropsToUse->TutorialPath, PropsToUse->SeenOnceSettingName);
		}
	}
}


FWelcomeTutorialProperties const* FIntroTutorials::ChooseBlueprintWelcomeTutorial(UObject* BlueprintObject)
{
	bShowHome = false;
	TutorialChainMap.Empty();

	UBlueprint* BP = Cast<UBlueprint>(BlueprintObject);
	if (BP)
	{
		bool bSeenWelcome = false;
		GConfig->GetBool(*IntroTutorialConfigSection, *BlueprintHomeTutorial.SeenOnceSettingName, bSeenWelcome, GEditorGameAgnosticIni);
		if(!bSeenWelcome)
		{
			switch (BP->BlueprintType)
			{
			case BPTYPE_Normal:
				TutorialChainMap.Add(BlueprintHomePath, ClassBlueprintWelcomeTutorial.TutorialPath);
				break;
			case BPTYPE_MacroLibrary:
				TutorialChainMap.Add(BlueprintHomePath, MacroLibraryBlueprintWelcomeTutorial.TutorialPath);
				break;
			case BPTYPE_Interface:
				TutorialChainMap.Add(BlueprintHomePath, InterfaceBlueprintWelcomeTutorial.TutorialPath);
				break;
			case BPTYPE_LevelScript:
				TutorialChainMap.Add(BlueprintHomePath, LevelScriptBlueprintWelcomeTutorial.TutorialPath);
				break;
			}
		}
		else
		{
			switch (BP->BlueprintType)
			{
			case BPTYPE_Normal:
				return &ClassBlueprintWelcomeTutorial;
			case BPTYPE_MacroLibrary:
				return &MacroLibraryBlueprintWelcomeTutorial;
			case BPTYPE_Interface:
				return &InterfaceBlueprintWelcomeTutorial;
			case BPTYPE_LevelScript:
				return &LevelScriptBlueprintWelcomeTutorial;
			}
		}
	}

	return &BlueprintHomeTutorial;
}

FWelcomeTutorialProperties const* FIntroTutorials::FindAssetEditorTutorialProperties(UClass const* Class) const
{
	for (auto It = AssetEditorTutorialPropertyMap.CreateConstIterator(); It; ++It)
	{
		UClass* const KeyClass = It.Key();
		if (Class->IsChildOf(KeyClass))
		{
			return &It.Value();
		}
	}

	return NULL;
}

void FIntroTutorials::MaybeOpenWelcomeTutorial(const FString& TutorialPath, const FString& ConfigSettingName)
{
	// don't open if viewing any tutorial other than the index
	if (TutorialWidget.IsValid() && (TutorialWidget.Pin()->GetCurrentPagePath() != HomePath))
	{
		return;
	}

	bool bSeenWelcome = false;
	GConfig->GetBool(*IntroTutorialConfigSection, *ConfigSettingName, bSeenWelcome, GEditorGameAgnosticIni);

	if (!bSeenWelcome)
	{
		bool const bPageExists = IDocumentation::Get()->PageExists(*TutorialPath);
		if (bPageExists)
		{
			SummonTutorialWindowForPage(*TutorialPath);

			// Tell ini file that we've seen this now
			GConfig->SetBool(*IntroTutorialConfigSection, *ConfigSettingName, true, GEditorGameAgnosticIni);
		}
	}
}

void FIntroTutorials::MaybeOpenWelcomeTutorial(const FWelcomeTutorialProperties& TutorialProperties)
{
	MaybeOpenWelcomeTutorial(TutorialProperties.TutorialPath, TutorialProperties.SeenOnceSettingName);
}

template< typename KeyType >
void FIntroTutorials::ResetTutorialPropertyMap(TMap<KeyType, FWelcomeTutorialProperties> Map) const
{
	for (auto It = Map.CreateConstIterator(); It; ++It)
	{
		ResetTutorial(It.Value());
	}
}

void FIntroTutorials::ResetWelcomeTutorials() const
{
	ResetTutorialPropertyMap(AssetEditorTutorialPropertyMap);
	ResetTutorialPropertyMap(EditorModeTutorialPropertyMap);

	ResetTutorial(UE4WelcomeTutorial);
	ResetTutorial(BlueprintHomeTutorial);
	ResetTutorial(ClassBlueprintWelcomeTutorial);
	ResetTutorial(MacroLibraryBlueprintWelcomeTutorial);
	ResetTutorial(InterfaceBlueprintWelcomeTutorial);
	ResetTutorial(LevelScriptBlueprintWelcomeTutorial);
	ResetTutorial(AddCodeToProjectWelcomeTutorial);
	ResetTutorial(MatineeEditorWelcomeTutorial);
}

void FIntroTutorials::OnAddCodeToProjectDialogOpened()
{
	MaybeOpenWelcomeTutorial(AddCodeToProjectWelcomeTutorial);
}

void FIntroTutorials::OnMatineeEditorOpened()
{
	MaybeOpenWelcomeTutorial(MatineeEditorWelcomeTutorial);
}

void FIntroTutorials::OnEditorModeChanged(FEdMode* Mode, bool bEnteringMode)
{
	// editor-mode changed events can fire before the MainFrameLoad event, so wait until *after* the welcome tutorial to start showing editor-mode tutorials
	// this avoids erroneously suppressing a mode tutorial because it opened but was then immediately replaced by the UE4 welcome tutorial
	if (!HasSeenTutorial(UE4WelcomeTutorial))
		return;
	
	// only show tutorials when entering an editor mode, not when leaving it
	if (bEnteringMode)
	{
		FWelcomeTutorialProperties const* const FoundProp = EditorModeTutorialPropertyMap.Find(Mode->GetID());
		if (FoundProp)
		{
			MaybeOpenWelcomeTutorial(FoundProp->TutorialPath, FoundProp->SeenOnceSettingName);
		}
	}
}

void FIntroTutorials::HandleCompilerNotFound()
{
#if PLATFORM_WINDOWS
	SummonTutorialWindowForPage( TEXT( "Shared/Tutorials/InstallingVisualStudioTutorial" ) );
#elif PLATFORM_MAC
	SummonTutorialWindowForPage( TEXT( "Shared/Tutorials/InstallingXCodeTutorial" ) );
#endif
}

void FIntroTutorials::HandleSDKNotInstalled(const FString& PlatformName, const FString& DocLink)
{
	SummonTutorialWindowForPage(DocLink);
}

bool FIntroTutorials::HasSeenTutorial( const FWelcomeTutorialProperties& TutProps ) const
{
	bool bSeenTutorial = false;
	GConfig->GetBool(*IntroTutorialConfigSection, *TutProps.SeenOnceSettingName, bSeenTutorial, GEditorGameAgnosticIni);
	return bSeenTutorial;
}

void FIntroTutorials::ResetTutorial(const FWelcomeTutorialProperties& TutProps) const
{
	FString SettingName = TutProps.SeenOnceSettingName;
	if (!SettingName.IsEmpty())
	{
		GConfig->SetBool(*IntroTutorialConfigSection, *SettingName, false, GEditorGameAgnosticIni);
	}
}

EVisibility FIntroTutorials::GetHomeButtonVisibility() const
{
	return bShowHome ? EVisibility::Visible : EVisibility::Hidden;
}

FString FIntroTutorials::HandleGotoNextTutorial(const FString& InCurrentPage) const
{
	const FString* NextTutorial = TutorialChainMap.Find(InCurrentPage);
	if(NextTutorial != nullptr)
	{
		return *NextTutorial;
	}

	return FString();
}

#undef LOCTEXT_NAMESPACE

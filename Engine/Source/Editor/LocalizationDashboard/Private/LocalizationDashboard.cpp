// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LocalizationDashboardPrivatePCH.h"
#include "LocalizationDashboard.h"
#include "IDetailCustomization.h"
#include "EditorStyleSet.h"
#include "DetailLayoutBuilder.h"
#include "LocalizationTarget.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "PropertyHandle.h"
#include "SLocalizationDashboardTargetRow.h"
#include "TargetsTableEntry.h"
#include "Runtime/Core/Public/Features/IModularFeatures.h"
#include "ILocalizationDashboardModule.h"
#include "ILocalizationServiceProvider.h"
#include "IPropertyUtilities.h"
#include "MultiBoxBuilder.h"
#include "FeedbackContext.h"
#include "LocalizationCommandletTasks.h"
#include "FileHelpers.h"
#include "SLocalizationTargetEditor.h"
#include "IMainFrameModule.h"

#define LOCTEXT_NAMESPACE "LocalizationDashboard"

const FName FLocalizationDashboard::TabName("LocalizationDashboard");

class ILocalizationServiceProvider;

namespace
{
	struct FLocalizationServiceProviderWrapper;

	class FLocalizationTargetSetDetailsCustomization : public IDetailCustomization
	{
	public:
		FLocalizationTargetSetDetailsCustomization(ULocalizationTargetSet* const InTargetSet);
		void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	private:
		void BuildTargetsList();
		void RebuildTargetsList();

		FText GetCurrentServiceProviderDisplayName() const;
		TSharedRef<SWidget> ServiceProviderComboBox_OnGenerateWidget(TSharedPtr<FLocalizationServiceProviderWrapper> LSPWrapper) const;
		void ServiceProviderComboBox_OnSelectionChanged(TSharedPtr<FLocalizationServiceProviderWrapper> LSPWrapper, ESelectInfo::Type SelectInfo);

		void GatherAllTargets();
		void ImportAllTargets();
		void ExportAllTargets();
		void UpdateTargetFromReports(ULocalizationTarget* const LocalizationTarget);
		void CompileAllTargets();

		TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<IPropertyHandle> TargetObjectPropertyHandle, const TSharedRef<STableViewBase>& Table);
		FReply OnNewTargetButtonClicked();

	private:
		ULocalizationTargetSet* TargetSet;

		IDetailLayoutBuilder* DetailLayoutBuilder;

		TSharedPtr<IPropertyHandle> ServiceProviderPropertyHandle;
		IDetailCategoryBuilder* ServiceProviderCategoryBuilder;
		TArray< TSharedPtr<FLocalizationServiceProviderWrapper> > Providers;

		TSharedPtr<IPropertyHandle> TargetObjectsPropertyHandle;
		FSimpleDelegate TargetsArrayPropertyHandle_OnNumElementsChanged;
		TArray< TSharedPtr<IPropertyHandle> > TargetsList;
		TSharedPtr< SListView< TSharedPtr<IPropertyHandle> > > TargetsListView;

		/* If set, the entry at the index specified needs to be initialized as soon as possible. */
		int32 NewEntryIndexToBeInitialized;
	};
	struct FLocalizationServiceProviderWrapper
	{
		FLocalizationServiceProviderWrapper() : Provider(nullptr) {}
		FLocalizationServiceProviderWrapper(ILocalizationServiceProvider* const InProvider) : Provider(InProvider) {}

		ILocalizationServiceProvider* Provider;
	};

	FLocalizationTargetSetDetailsCustomization::FLocalizationTargetSetDetailsCustomization(ULocalizationTargetSet* const InTargetSet)
		: TargetSet(InTargetSet)
		, DetailLayoutBuilder(nullptr)
		, ServiceProviderCategoryBuilder(nullptr)
		, NewEntryIndexToBeInitialized(INDEX_NONE)
	{
		TArray<ILocalizationServiceProvider*> ActualProviders = ILocalizationDashboardModule::Get().GetLocalizationServiceProviders();
		for (ILocalizationServiceProvider* ActualProvider : ActualProviders)
		{
			TSharedPtr<FLocalizationServiceProviderWrapper> Provider = MakeShareable(new FLocalizationServiceProviderWrapper(ActualProvider));
			Providers.Add(Provider);
		}
	}

	class FindProviderPredicate
	{
	public:
		FindProviderPredicate(ILocalizationServiceProvider* const InActualProvider)
			: ActualProvider(InActualProvider)
		{
		}

		bool operator()(const TSharedPtr<FLocalizationServiceProviderWrapper>& Provider)
		{
			return Provider->Provider == ActualProvider;
		}

	private:
		ILocalizationServiceProvider* ActualProvider;
	};


	class FLocalizationDashboardCommands : public TCommands<FLocalizationDashboardCommands>
	{
	public:
		FLocalizationDashboardCommands() 
			: TCommands<FLocalizationDashboardCommands>("LocalizationDashboard", NSLOCTEXT("Contexts", "LocalizationDashboard", "Localization Dashboard"), NAME_None, FEditorStyle::GetStyleSetName())
		{
		}

		TSharedPtr<FUICommandInfo> GatherAllTargets;
		TSharedPtr<FUICommandInfo> ImportAllTargets;
		TSharedPtr<FUICommandInfo> ExportAllTargets;
		TSharedPtr<FUICommandInfo> CompileAllTargets;

		/** Initialize commands */
		virtual void RegisterCommands() override;
	};

	void FLocalizationDashboardCommands::RegisterCommands() 
	{
		UI_COMMAND( GatherAllTargets, "Gather All", "Gathers text for all targets in the project.", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( ImportAllTargets, "Import All", "Imports translations for all cultures of all targets in the project.", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( ExportAllTargets, "Export All", "Exports translations for all cultures of all targets in the project.", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( CompileAllTargets, "Compile All", "Compiles translations for all targets in the project.", EUserInterfaceActionType::Button, FInputGesture() );
	}

	void FLocalizationTargetSetDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
	{
		DetailLayoutBuilder = &DetailBuilder;

		{
			IDetailCategoryBuilder& DetailCategoryBuilder = DetailBuilder.EditCategory(FName("Targets"));
			TargetObjectsPropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ULocalizationTargetSet,TargetObjects));
			if (TargetObjectsPropertyHandle->IsValidHandle())
			{
				TargetObjectsPropertyHandle->MarkHiddenByCustomization();
				TargetsArrayPropertyHandle_OnNumElementsChanged = FSimpleDelegate::CreateSP(this, &FLocalizationTargetSetDetailsCustomization::RebuildTargetsList);
				TargetObjectsPropertyHandle->AsArray()->SetOnNumElementsChanged(TargetsArrayPropertyHandle_OnNumElementsChanged);

				FLocalizationDashboardCommands::Register();
				const TSharedRef< FUICommandList > CommandList = MakeShareable(new FUICommandList);
				FToolBarBuilder ToolBarBuilder( CommandList, FMultiBoxCustomization::AllowCustomization("LocalizationDashboard") );

				CommandList->MapAction( FLocalizationDashboardCommands::Get().GatherAllTargets, FExecuteAction::CreateSP(this, &FLocalizationTargetSetDetailsCustomization::GatherAllTargets));
				ToolBarBuilder.AddToolBarButton(FLocalizationDashboardCommands::Get().GatherAllTargets, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "LocalizationDashboard.GatherAllTargets"));

				CommandList->MapAction( FLocalizationDashboardCommands::Get().ImportAllTargets, FExecuteAction::CreateSP(this, &FLocalizationTargetSetDetailsCustomization::ImportAllTargets));
				ToolBarBuilder.AddToolBarButton(FLocalizationDashboardCommands::Get().ImportAllTargets, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "LocalizationDashboard.ImportForAllTargetsCultures"));

				CommandList->MapAction( FLocalizationDashboardCommands::Get().ExportAllTargets, FExecuteAction::CreateSP(this, &FLocalizationTargetSetDetailsCustomization::ExportAllTargets));
				ToolBarBuilder.AddToolBarButton(FLocalizationDashboardCommands::Get().ExportAllTargets, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "LocalizationDashboard.ExportForAllTargetsCultures"));

				CommandList->MapAction( FLocalizationDashboardCommands::Get().CompileAllTargets, FExecuteAction::CreateSP(this, &FLocalizationTargetSetDetailsCustomization::CompileAllTargets));
				ToolBarBuilder.AddToolBarButton(FLocalizationDashboardCommands::Get().CompileAllTargets, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "LocalizationDashboard.CompileAllTargets"));

				BuildTargetsList();

				DetailCategoryBuilder.AddCustomRow( LOCTEXT("TargetsFilterString", "Targets") )
					.WholeRowContent()
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							ToolBarBuilder.MakeWidget()
						]
						+SVerticalBox::Slot()
							.AutoHeight()
							[
								SAssignNew(TargetsListView, SListView< TSharedPtr<IPropertyHandle> >)
								.OnGenerateRow(this, &FLocalizationTargetSetDetailsCustomization::OnGenerateRow)
								.ListItemsSource(&TargetsList)
								.SelectionMode(ESelectionMode::None)
								.HeaderRow
								(
								SNew(SHeaderRow)
								+SHeaderRow::Column("Target")
								.DefaultLabel( LOCTEXT("TargetColumnLabel", "Target"))
								.HAlignHeader(HAlign_Left)
								.HAlignCell(HAlign_Left)
								.VAlignCell(VAlign_Center)
								+SHeaderRow::Column("Status")
								.DefaultLabel( LOCTEXT("StatusColumnLabel", "Status"))
								.HAlignHeader(HAlign_Center)
								.HAlignCell(HAlign_Center)
								.VAlignCell(VAlign_Center)
								+SHeaderRow::Column("Cultures")
								.DefaultLabel( LOCTEXT("CulturesColumnLabel", "Cultures"))
								.HAlignHeader(HAlign_Fill)
								.HAlignCell(HAlign_Fill)
								.VAlignCell(VAlign_Center)
								+SHeaderRow::Column("WordCount")
								.DefaultLabel( LOCTEXT("WordCountColumnLabel", "Word Count"))
								.HAlignHeader(HAlign_Center)
								.HAlignCell(HAlign_Center)
								.VAlignCell(VAlign_Center)
								+SHeaderRow::Column("Actions")
								.DefaultLabel( LOCTEXT("ActionsColumnLabel", "Actions"))
								.HAlignHeader(HAlign_Center)
								.HAlignCell(HAlign_Center)
								.VAlignCell(VAlign_Center)
								)
							]
						+SVerticalBox::Slot()
							.AutoHeight()
							.VAlign(VAlign_Center)
							[
								SNew(SButton)
								.Text(LOCTEXT("AddNewTargetButtonLabel", "Add New Target"))
								.OnClicked(this, &FLocalizationTargetSetDetailsCustomization::OnNewTargetButtonClicked)
							]
					];
			}
		}
	}

	void FLocalizationTargetSetDetailsCustomization::BuildTargetsList()
	{
		const TSharedPtr<IPropertyHandleArray> TargetObjectsArrayPropertyHandle = TargetObjectsPropertyHandle->AsArray();
		if (TargetObjectsArrayPropertyHandle.IsValid())
		{
			uint32 ElementCount = 0;
			TargetObjectsArrayPropertyHandle->GetNumElements(ElementCount);
			for (uint32 i = 0; i < ElementCount; ++i)
			{
				const TSharedPtr<IPropertyHandle> TargetObjectPropertyHandle = TargetObjectsArrayPropertyHandle->GetElement(i);
				TargetsList.Add(TargetObjectPropertyHandle);
			}
		}
	}

	void FLocalizationTargetSetDetailsCustomization::RebuildTargetsList()
	{
		const TSharedPtr<IPropertyHandleArray> TargetObjectsArrayPropertyHandle = TargetObjectsPropertyHandle->AsArray();
		if (TargetObjectsArrayPropertyHandle.IsValid() && NewEntryIndexToBeInitialized != INDEX_NONE)
		{
			const TSharedPtr<IPropertyHandle> TargetObjectPropertyHandle = TargetObjectsArrayPropertyHandle->GetElement(NewEntryIndexToBeInitialized);
			if (TargetObjectPropertyHandle.IsValid() && TargetObjectPropertyHandle->IsValidHandle())
			{
				ULocalizationTarget* const NewTarget = NewObject<ULocalizationTarget>(TargetSet);

				TArray<void*> RawData;
				TargetObjectsPropertyHandle->AccessRawData(RawData);
				void* RawDatum = RawData.Top();
				TArray<ULocalizationTarget*>& TargetObjectsArray = *(reinterpret_cast< TArray<ULocalizationTarget*>* >(RawDatum));

				FString NewTargetName = "NewTarget";
				auto TargetNameIsIdentical = [&](ULocalizationTarget* Target) -> bool
				{
					return (Target != NewTarget) && Target && (Target->Settings.Name == NewTargetName);
				};

				for (uint32 i = 0; TargetObjectsArray.ContainsByPredicate(TargetNameIsIdentical); ++i)
				{
					NewTargetName = FString::Printf(TEXT("NewTarget%u"), i);
				}

				NewTarget->Settings.Name = NewTargetName;
				NewTarget->Settings.NativeCultureStatistics.CultureName = FInternationalization::Get().GetCurrentCulture()->GetName();

				const UObject* SetValue = NewTarget;
				TargetObjectPropertyHandle->SetValue(SetValue);

				NewEntryIndexToBeInitialized = INDEX_NONE;
			}
		}

		TargetsList.Empty();
		BuildTargetsList();

		if (TargetsListView.IsValid())
		{
			TargetsListView->RequestListRefresh();
		}
	}

	FText FLocalizationTargetSetDetailsCustomization::GetCurrentServiceProviderDisplayName() const
	{
		ILocalizationServiceProvider* const LSP = ILocalizationDashboardModule::Get().GetCurrentLocalizationServiceProvider();
		return LSP ? LSP->GetDisplayName() : LOCTEXT("NoServiceProviderName", "None");
	}

	TSharedRef<SWidget> FLocalizationTargetSetDetailsCustomization::ServiceProviderComboBox_OnGenerateWidget(TSharedPtr<FLocalizationServiceProviderWrapper> LSPWrapper) const
	{
		ILocalizationServiceProvider* const LSP = LSPWrapper->Provider;
		return	SNew(STextBlock)
			.Text(LSP ? LSP->GetDisplayName() : LOCTEXT("NoServiceProviderName", "None"));
	}

	void FLocalizationTargetSetDetailsCustomization::ServiceProviderComboBox_OnSelectionChanged(TSharedPtr<FLocalizationServiceProviderWrapper> LSPWrapper, ESelectInfo::Type SelectInfo)
	{
		ILocalizationServiceProvider* const LSP = LSPWrapper->Provider;
		FString ServiceProviderName = LSP ? LSP->GetName().ToString() : TEXT("None");
		ServiceProviderPropertyHandle->SetValue(ServiceProviderName);

		if (LSP)
		{
			LSP->CustomizeSettingsDetails(*ServiceProviderCategoryBuilder);
		}
		DetailLayoutBuilder->ForceRefreshDetails();
	}

	void FLocalizationTargetSetDetailsCustomization::GatherAllTargets()
	{
		for (ULocalizationTarget* const LocalizationTarget : TargetSet->TargetObjects)
		{
			// Save unsaved packages.
			const bool bPromptUserToSave = true;
			const bool bSaveMapPackages = true;
			const bool bSaveContentPackages = true;
			const bool bFastSave = false;
			const bool bNotifyNoPackagesSaved = false;
			const bool bCanBeDeclined = true;
			bool DidPackagesNeedSaving;
			const bool WerePackagesSaved = FEditorFileUtils::SaveDirtyPackages(bPromptUserToSave, bSaveMapPackages, bSaveContentPackages, bFastSave, bNotifyNoPackagesSaved, bCanBeDeclined, &DidPackagesNeedSaving);

			if (DidPackagesNeedSaving && !WerePackagesSaved)
			{
				// Give warning dialog.
				const FText MessageText = NSLOCTEXT("LocalizationCultureActions", "UnsavedPackagesWarningDialogMessage", "There are unsaved changes. These changes may not be gathered from correctly.");
				const FText TitleText = NSLOCTEXT("LocalizationCultureActions", "UnsavedPackagesWarningDialogTitle", "Unsaved Changes Before Gather");
				switch(FMessageDialog::Open(EAppMsgType::OkCancel, MessageText, &TitleText))
				{
				case EAppReturnType::Cancel:
					{
						return;
					}
					break;
				}
			}

		}

		// Execute gather.
		const TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(DetailLayoutBuilder->GetDetailsView().AsShared());
		TArray<FLocalizationTargetSettings*> TargetSettings;
		for (ULocalizationTarget* const TargetObject : TargetSet->TargetObjects)
		{
			TargetSettings.Add(&TargetObject->Settings);
		}
		LocalizationCommandletTasks::GatherTargets(ParentWindow.ToSharedRef(), TargetSettings);

		for (ULocalizationTarget* const LocalizationTarget : TargetSet->TargetObjects)
		{
			UpdateTargetFromReports(LocalizationTarget);
		}
	}

	void FLocalizationTargetSetDetailsCustomization::ImportAllTargets()
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (DesktopPlatform)
		{
			void* ParentWindowWindowHandle = NULL;
			const TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(DetailLayoutBuilder->GetDetailsView().AsShared());
			if (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid())
			{
				ParentWindowWindowHandle = ParentWindow->GetNativeWindow()->GetOSWindowHandle();
			}

			
			const FString DefaultPath = FPaths::ConvertRelativePathToFull(FPaths::GameContentDir() / TEXT("Localization"));

			// Prompt the user for the directory
			FString OutputDirectory;
			if (DesktopPlatform->OpenDirectoryDialog(ParentWindowWindowHandle, LOCTEXT("ImportAllTranslationsDialogTitle", "Import All Translations from Directory").ToString(), DefaultPath, OutputDirectory))
			{
				TArray<FLocalizationTargetSettings*> TargetSettings;
				for (ULocalizationTarget* const TargetObject : TargetSet->TargetObjects)
				{
					TargetSettings.Add(&TargetObject->Settings);
				}
				LocalizationCommandletTasks::ImportTargets(ParentWindow.ToSharedRef(), TargetSettings, TOptional<FString>(OutputDirectory));

				for (ULocalizationTarget* const LocalizationTarget : TargetSet->TargetObjects)
				{
					UpdateTargetFromReports(LocalizationTarget);
				}
			}
		}
	}

	void FLocalizationTargetSetDetailsCustomization::ExportAllTargets()
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (DesktopPlatform)
		{
			void* ParentWindowWindowHandle = NULL;
			const TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(DetailLayoutBuilder->GetDetailsView().AsShared());
			if (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid())
			{
				ParentWindowWindowHandle = ParentWindow->GetNativeWindow()->GetOSWindowHandle();
			}

			const FString DefaultPath = FPaths::ConvertRelativePathToFull(FPaths::GameContentDir() / TEXT("Localization"));

			// Prompt the user for the directory
			FString OutputDirectory;
			if (DesktopPlatform->OpenDirectoryDialog(ParentWindowWindowHandle, LOCTEXT("ExportAllTranslationsDialogTitle", "Export All Translations to Directory").ToString(), DefaultPath, OutputDirectory))
			{
				TArray<FLocalizationTargetSettings*> TargetSettings;
				for (ULocalizationTarget* const TargetObject : TargetSet->TargetObjects)
				{
					TargetSettings.Add(&TargetObject->Settings);
				}
				LocalizationCommandletTasks::ExportTargets(ParentWindow.ToSharedRef(), TargetSettings, TOptional<FString>(OutputDirectory));
			}
		}
	}

	void FLocalizationTargetSetDetailsCustomization::UpdateTargetFromReports(ULocalizationTarget* const LocalizationTarget)
	{
		//TArray< TSharedPtr<IPropertyHandle> > WordCountPropertyHandles;

		//const TSharedPtr<IPropertyHandle> TargetSettingsPropertyHandle = TargetEditor->GetTargetSettingsPropertyHandle();
		//if (TargetSettingsPropertyHandle.IsValid() && TargetSettingsPropertyHandle->IsValidHandle())
		//{
		//	const TSharedPtr<IPropertyHandle> NativeCultureStatisticsPropertyHandle = TargetSettingsPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FLocalizationTargetSettings, NativeCultureStatistics));
		//	if (NativeCultureStatisticsPropertyHandle.IsValid() && NativeCultureStatisticsPropertyHandle->IsValidHandle())
		//	{
		//		const TSharedPtr<IPropertyHandle> WordCountPropertyHandle = NativeCultureStatisticsPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCultureStatistics, WordCount));
		//		if (WordCountPropertyHandle.IsValid() && WordCountPropertyHandle->IsValidHandle())
		//		{
		//			WordCountPropertyHandles.Add(WordCountPropertyHandle);
		//		}
		//	}

		//	const TSharedPtr<IPropertyHandle> SupportedCulturesStatisticsPropertyHandle = TargetSettingsPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FLocalizationTargetSettings, SupportedCulturesStatistics));
		//	if (SupportedCulturesStatisticsPropertyHandle.IsValid() && SupportedCulturesStatisticsPropertyHandle->IsValidHandle())
		//	{
		//		uint32 SupportedCultureCount = 0;
		//		SupportedCulturesStatisticsPropertyHandle->GetNumChildren(SupportedCultureCount);
		//		for (uint32 i = 0; i < SupportedCultureCount; ++i)
		//		{
		//			const TSharedPtr<IPropertyHandle> ElementPropertyHandle = SupportedCulturesStatisticsPropertyHandle->GetChildHandle(i);
		//			if (ElementPropertyHandle.IsValid() && ElementPropertyHandle->IsValidHandle())
		//			{
		//				const TSharedPtr<IPropertyHandle> WordCountPropertyHandle = SupportedCulturesStatisticsPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCultureStatistics, WordCount));
		//				if (WordCountPropertyHandle.IsValid() && WordCountPropertyHandle->IsValidHandle())
		//				{
		//					WordCountPropertyHandles.Add(WordCountPropertyHandle);
		//				}
		//			}
		//		}
		//	}
		//}

		//for (const TSharedPtr<IPropertyHandle>& WordCountPropertyHandle : WordCountPropertyHandles)
		//{
		//	WordCountPropertyHandle->NotifyPreChange();
		//}
		LocalizationTarget->Settings.UpdateWordCountsFromCSV();
		LocalizationTarget->Settings.UpdateStatusFromConflictReport();
		//for (const TSharedPtr<IPropertyHandle>& WordCountPropertyHandle : WordCountPropertyHandles)
		//{
		//	WordCountPropertyHandle->NotifyPostChange();
		//}
	}

	void FLocalizationTargetSetDetailsCustomization::CompileAllTargets()
	{
		// Execute compile.
		const TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(DetailLayoutBuilder->GetDetailsView().AsShared());
		TArray<FLocalizationTargetSettings*> TargetSettings;
		for (ULocalizationTarget* const TargetObject : TargetSet->TargetObjects)
		{
			TargetSettings.Add(&TargetObject->Settings);
		}
		LocalizationCommandletTasks::CompileTargets(ParentWindow.ToSharedRef(), TargetSettings);
	}

	TSharedRef<ITableRow> FLocalizationTargetSetDetailsCustomization::OnGenerateRow(TSharedPtr<IPropertyHandle> TargetObjectPropertyHandle, const TSharedRef<STableViewBase>& Table)
	{
		return SNew(SLocalizationDashboardTargetRow, Table, DetailLayoutBuilder->GetPropertyUtilities(), TargetObjectPropertyHandle.ToSharedRef());
	}

	FReply FLocalizationTargetSetDetailsCustomization::OnNewTargetButtonClicked()
	{
		if(TargetObjectsPropertyHandle.IsValid() && TargetObjectsPropertyHandle->IsValidHandle())
		{
			uint32 NewElementIndex;
			TargetObjectsPropertyHandle->AsArray()->GetNumElements(NewElementIndex);
			TargetObjectsPropertyHandle->AsArray()->AddItem();
			NewEntryIndexToBeInitialized = NewElementIndex;
		}

		return FReply::Handled();
	}
}

class SLocalizationDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLocalizationDashboard) {}
	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedPtr<SWindow>& OwningWindow, const TSharedRef<SDockTab>& OwningTab, ULocalizationTargetSet* const TargetSet);
	TWeakPtr<SDockTab> ShowTargetEditor(ULocalizationTarget* const LocalizationTarget);

private:
	static const FName ProjectTargetSetDetailsTabName;
	static const FName DocumentsTabName;

	TSharedPtr<FTabManager> TabManager;
	TMap< TWeakObjectPtr<ULocalizationTarget>, TWeakPtr<SDockTab> > TargetToTabMap;
};

const FName SLocalizationDashboard::ProjectTargetSetDetailsTabName("ProjectTargets");
const FName SLocalizationDashboard::DocumentsTabName("Documents");

void SLocalizationDashboard::Construct(const FArguments& InArgs, const TSharedPtr<SWindow>& OwningWindow, const TSharedRef<SDockTab>& OwningTab, ULocalizationTargetSet* const TargetSet)
{
	check(TargetSet);

	TabManager = FGlobalTabmanager::Get()->NewTabManager(OwningTab);

	const auto& PersistLayout = [](const TSharedRef<FTabManager::FLayout>& LayoutToSave)
	{
		FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, LayoutToSave);
	};
	TabManager->SetOnPersistLayout(FTabManager::FOnPersistLayout::CreateLambda(PersistLayout));

	const auto& CreateDetailsTab = [TargetSet](const FSpawnTabArgs& SpawnTabArgs) -> TSharedRef<SDockTab>
	{
		const TSharedRef<SDockTab> DockTab = SNew(SDockTab)
			.Label(LOCTEXT("ProjectTargetsTabLabel", "Project Targets"));

		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		FDetailsViewArgs DetailsViewArgs(false, false, false, FDetailsViewArgs::ENameAreaSettings::HideNameArea, false, nullptr, false, NAME_None);
		TSharedRef<IDetailsView> DetailsView = PropertyModule.CreateDetailView(DetailsViewArgs);

		DetailsView->RegisterInstancedCustomPropertyLayout(ULocalizationTargetSet::StaticClass(), FOnGetDetailCustomizationInstance::CreateLambda([&]()->TSharedRef<IDetailCustomization>{return MakeShareable(new FLocalizationTargetSetDetailsCustomization(TargetSet));}));
		DetailsView->SetObject(TargetSet, true);

		DockTab->SetContent(DetailsView);

		return DockTab;
	};
	const TSharedRef<FWorkspaceItem> TargetSetsWorkspaceMenuCategory = TabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("TargetSetsWorkspaceMenuCategory", "Target Sets"));
	FTabSpawnerEntry& ProjectTargetSetTabSpawnerEntry = TabManager->RegisterTabSpawner(ProjectTargetSetDetailsTabName, FOnSpawnTab::CreateLambda(CreateDetailsTab))
		.SetDisplayName(LOCTEXT("ProjectTargetsDetailTabSpawner", "Project Targets"));
	TargetSetsWorkspaceMenuCategory->AddItem(ProjectTargetSetTabSpawnerEntry.AsShared());

	TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("LocalizationDashboard_Experimental_V3")
		->AddArea
		(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Vertical)
		->Split
		(
		FTabManager::NewStack()
		->SetHideTabWell(true)
		->AddTab(ProjectTargetSetDetailsTabName, ETabState::OpenedTab)
		)
		->Split
		(
		FTabManager::NewStack()
		->SetHideTabWell(false)
		->AddTab(DocumentsTabName, ETabState::ClosedTab)
		)
		);

	Layout = FLayoutSaveRestore::LoadFromConfig(GEditorLayoutIni, Layout);

	const TSharedRef<FExtender> MenuExtender = MakeShareable(new FExtender());
	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>( "MainFrame" );
	const TSharedRef<SWidget> MenuWidget = MainFrameModule.MakeMainMenu( TabManager, MenuExtender );

	ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				MenuWidget
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
			TabManager->RestoreFrom(Layout, OwningWindow).ToSharedRef()
			]
		];
}

TWeakPtr<SDockTab> SLocalizationDashboard::ShowTargetEditor(ULocalizationTarget* const LocalizationTarget)
{
	// Create tab if not existent.
	TWeakPtr<SDockTab>& TargetEditorDockTab = TargetToTabMap.FindOrAdd(TWeakObjectPtr<ULocalizationTarget>(LocalizationTarget));

	if (!TargetEditorDockTab.IsValid())
	{
		ULocalizationTargetSet* const TargetSet = LocalizationTarget->GetTypedOuter<ULocalizationTargetSet>();

		const TSharedRef<SLocalizationTargetEditor> OurTargetEditor = SNew(SLocalizationTargetEditor, TargetSet, LocalizationTarget);
		const TSharedRef<SDockTab> NewTargetEditorTab = SNew(SDockTab)
			.TabRole(ETabRole::DocumentTab)
			.Label_Lambda( [LocalizationTarget]
			{
				return LocalizationTarget ? FText::FromString(LocalizationTarget->Settings.Name) : FText::GetEmpty();
			})
			[
				OurTargetEditor
			];

		TabManager->InsertNewDocumentTab(DocumentsTabName, FTabManager::ESearchPreference::RequireClosedTab, NewTargetEditorTab);
		TargetEditorDockTab = NewTargetEditorTab;
	}
	else
	{
		const TSharedPtr<SDockTab> OldTargetEditorTab = TargetEditorDockTab.Pin();
		TabManager->DrawAttention(OldTargetEditorTab.ToSharedRef());
	}

	return TargetEditorDockTab;
}

FLocalizationDashboard* FLocalizationDashboard::Instance = nullptr;

FLocalizationDashboard* FLocalizationDashboard::Get()
{
	return Instance;
}

void FLocalizationDashboard::Initialize()
{
	if (!Instance)
	{
		Instance = new FLocalizationDashboard();
	}
}

void FLocalizationDashboard::Terminate()
{
	if (Instance)
	{
		delete Instance;
	}
}

void FLocalizationDashboard::Show()
{
	FGlobalTabmanager::Get()->InvokeTab(TabName);
}

TWeakPtr<SDockTab> FLocalizationDashboard::ShowTargetEditorTab(ULocalizationTarget* const LocalizationTarget)
{
	if (LocalizationDashboardWidget.IsValid())
	{
		return LocalizationDashboardWidget->ShowTargetEditor(LocalizationTarget);
	}
	return nullptr;
}

FLocalizationDashboard::FLocalizationDashboard()
{
	RegisterTabSpawner();
}

FLocalizationDashboard::~FLocalizationDashboard()
{
	UnregisterTabSpawner();
}

void FLocalizationDashboard::RegisterTabSpawner()
{
	const auto& SpawnMainTab = [this](const FSpawnTabArgs& Args) -> TSharedRef<SDockTab>
	{
		const TSharedRef<SDockTab> DockTab = SNew(SDockTab)
			.Label(LOCTEXT("MainTabTitle", "Localization Dashboard"))
			.TabRole(ETabRole::MajorTab);

		DockTab->SetContent( SAssignNew(LocalizationDashboardWidget, SLocalizationDashboard, Args.GetOwnerWindow(), DockTab, GetMutableDefault<ULocalizationTargetSet>()) );

		return DockTab;
	};

	FGlobalTabmanager::Get()->RegisterTabSpawner(TabName, FOnSpawnTab::CreateLambda( TFunction<TSharedRef<SDockTab> (const FSpawnTabArgs&)>(SpawnMainTab) ) )
		.SetDisplayName(LOCTEXT("MainTabTitle", "Localization Dashboard"));
}

void FLocalizationDashboard::UnregisterTabSpawner()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(TabName);
}

#undef LOCTEXT_NAMESPACE
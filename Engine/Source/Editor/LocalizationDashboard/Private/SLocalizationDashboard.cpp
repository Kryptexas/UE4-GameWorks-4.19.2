// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LocalizationDashboardPrivatePCH.h"
#include "SLocalizationDashboard.h"
#include "IDetailCustomization.h"
#include "EditorStyleSet.h"
#include "DetailLayoutBuilder.h"
#include "ProjectLocalizationSettings.h"
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

#define LOCTEXT_NAMESPACE "LocalizationDashboard"

const FName SLocalizationDashboard::TabName("LocalizationDashboard");

class ILocalizationServiceProvider;

namespace
{
	struct FLocalizationServiceProviderWrapper;

	class FProjectLocalizationSettingsDetailsCustomization : public IDetailCustomization
	{
	public:
		FProjectLocalizationSettingsDetailsCustomization(UProjectLocalizationSettings* const InSettings);
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

		TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<IPropertyHandle> TargetObjectPropertyHandle, const TSharedRef<STableViewBase>& Table);
		FReply OnNewTargetButtonClicked();

	private:
		UProjectLocalizationSettings* Settings;

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

	FProjectLocalizationSettingsDetailsCustomization::FProjectLocalizationSettingsDetailsCustomization(UProjectLocalizationSettings* const InSettings)
		: Settings(InSettings)
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

		/** Initialize commands */
		virtual void RegisterCommands() override;
	};

	void FLocalizationDashboardCommands::RegisterCommands() 
	{
		UI_COMMAND( GatherAllTargets, "Gather All", "Gathers text for all targets in the project.", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( ImportAllTargets, "Import All", "Imports translations for all cultures of all targets in the project.", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( ExportAllTargets, "Export All", "Exports translations for all cultures of all targets in the project.", EUserInterfaceActionType::Button, FInputGesture() );
	}

	void FProjectLocalizationSettingsDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
	{
		DetailLayoutBuilder = &DetailBuilder;

		{
			ServiceProviderPropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UProjectLocalizationSettings, LocalizationServiceProvider));

			if (ServiceProviderPropertyHandle.IsValid() && ServiceProviderPropertyHandle->IsValidHandle())
			{
				ServiceProviderPropertyHandle->MarkHiddenByCustomization();
				//ServiceProviderCategoryBuilder = &( DetailBuilder.EditCategory( FName("LocalizationServiceProvider") ) );

				//FDetailWidgetRow& ServiceProviderDetailWidgetRow = ServiceProviderCategoryBuilder->AddCustomRow( LOCTEXT("LocalizationServiceProviderFilterString", "Localization Service Provider") );
				//ServiceProviderDetailWidgetRow.NameContent()
				//	[
				//		ServiceProviderPropertyHandle->CreatePropertyNameWidget()
				//	];

				//ILocalizationServiceProvider* const LSP = ILocalizationDashboardModule::Get().GetCurrentLocalizationServiceProvider();
				//TSharedPtr<FLocalizationServiceProviderWrapper>* LSPWrapper = Providers.FindByPredicate(FindProviderPredicate(LSP));
				//ServiceProviderDetailWidgetRow.ValueContent()
				//	[
				//		SNew(SComboBox<TSharedPtr<FLocalizationServiceProviderWrapper>>)
				//		.InitiallySelectedItem(LSPWrapper ? *LSPWrapper : nullptr)
				//		.OptionsSource(&Providers)
				//		.OnGenerateWidget(this, &FProjectLocalizationSettingsDetailsCustomization::ServiceProviderComboBox_OnGenerateWidget)
				//		.OnSelectionChanged(this, &FProjectLocalizationSettingsDetailsCustomization::ServiceProviderComboBox_OnSelectionChanged)
				//		.Content()
				//		[
				//			SNew(STextBlock)
				//			.Text(this, &FProjectLocalizationSettingsDetailsCustomization::GetCurrentServiceProviderDisplayName)
				//		]
				//	];

				//if (LSP)
				//{
				//	LSP->CustomizeSettingsDetails(*ServiceProviderCategoryBuilder);
				//}
			}
		}

		{
			IDetailCategoryBuilder& DetailCategoryBuilder = DetailBuilder.EditCategory(FName("Targets"));
			TargetObjectsPropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UProjectLocalizationSettings,TargetObjects));
			if (TargetObjectsPropertyHandle->IsValidHandle())
			{
				TargetObjectsPropertyHandle->MarkHiddenByCustomization();
				TargetsArrayPropertyHandle_OnNumElementsChanged = FSimpleDelegate::CreateSP(this, &FProjectLocalizationSettingsDetailsCustomization::RebuildTargetsList);
				TargetObjectsPropertyHandle->AsArray()->SetOnNumElementsChanged(TargetsArrayPropertyHandle_OnNumElementsChanged);

				FLocalizationDashboardCommands::Register();
				const TSharedRef< FUICommandList > CommandList = MakeShareable(new FUICommandList);
				FToolBarBuilder ToolBarBuilder( CommandList, FMultiBoxCustomization::AllowCustomization("LocalizationDashboard") );

				CommandList->MapAction( FLocalizationDashboardCommands::Get().GatherAllTargets, FExecuteAction::CreateSP(this, &FProjectLocalizationSettingsDetailsCustomization::GatherAllTargets));
				ToolBarBuilder.AddToolBarButton(FLocalizationDashboardCommands::Get().GatherAllTargets, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "LocalizationDashboard.GatherAllTargets"));

				CommandList->MapAction( FLocalizationDashboardCommands::Get().ImportAllTargets, FExecuteAction::CreateSP(this, &FProjectLocalizationSettingsDetailsCustomization::ImportAllTargets));
				ToolBarBuilder.AddToolBarButton(FLocalizationDashboardCommands::Get().ImportAllTargets, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "LocalizationDashboard.ImportForAllTargetsCultures"));

				CommandList->MapAction( FLocalizationDashboardCommands::Get().ExportAllTargets, FExecuteAction::CreateSP(this, &FProjectLocalizationSettingsDetailsCustomization::ExportAllTargets));
				ToolBarBuilder.AddToolBarButton(FLocalizationDashboardCommands::Get().ExportAllTargets, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "LocalizationDashboard.ExportForAllTargetsCultures"));

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
								.OnGenerateRow(this, &FProjectLocalizationSettingsDetailsCustomization::OnGenerateRow)
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
								.OnClicked(this, &FProjectLocalizationSettingsDetailsCustomization::OnNewTargetButtonClicked)
							]
					];
			}
		}
	}

	void FProjectLocalizationSettingsDetailsCustomization::BuildTargetsList()
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

	void FProjectLocalizationSettingsDetailsCustomization::RebuildTargetsList()
	{
		const TSharedPtr<IPropertyHandleArray> TargetObjectsArrayPropertyHandle = TargetObjectsPropertyHandle->AsArray();
		if (TargetObjectsArrayPropertyHandle.IsValid() && NewEntryIndexToBeInitialized != INDEX_NONE)
		{
			const TSharedPtr<IPropertyHandle> TargetObjectPropertyHandle = TargetObjectsArrayPropertyHandle->GetElement(NewEntryIndexToBeInitialized);
			if (TargetObjectPropertyHandle.IsValid() && TargetObjectPropertyHandle->IsValidHandle())
			{
				ULocalizationTarget* const NewTarget = Cast<ULocalizationTarget>(StaticConstructObject(ULocalizationTarget::StaticClass(), Settings));

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

	FText FProjectLocalizationSettingsDetailsCustomization::GetCurrentServiceProviderDisplayName() const
	{
		ILocalizationServiceProvider* const LSP = ILocalizationDashboardModule::Get().GetCurrentLocalizationServiceProvider();
		return LSP ? LSP->GetDisplayName() : LOCTEXT("NoServiceProviderName", "None");
	}

	TSharedRef<SWidget> FProjectLocalizationSettingsDetailsCustomization::ServiceProviderComboBox_OnGenerateWidget(TSharedPtr<FLocalizationServiceProviderWrapper> LSPWrapper) const
	{
		ILocalizationServiceProvider* const LSP = LSPWrapper->Provider;
		return	SNew(STextBlock)
			.Text(LSP ? LSP->GetDisplayName() : LOCTEXT("NoServiceProviderName", "None"));
	}

	void FProjectLocalizationSettingsDetailsCustomization::ServiceProviderComboBox_OnSelectionChanged(TSharedPtr<FLocalizationServiceProviderWrapper> LSPWrapper, ESelectInfo::Type SelectInfo)
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

	void FProjectLocalizationSettingsDetailsCustomization::GatherAllTargets()
	{
		for (ULocalizationTarget* const LocalizationTarget : Settings->TargetObjects)
		{
			// Save unsaved packages.
			bool DidPackagesNeedSaving;
			const bool WerePackagesSaved = FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, &DidPackagesNeedSaving);

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
		for (ULocalizationTarget* const TargetObject : Settings->TargetObjects)
		{
			TargetSettings.Add(&TargetObject->Settings);
		}
		LocalizationCommandletTasks::GatherTargets(ParentWindow.ToSharedRef(), TargetSettings);

		for (ULocalizationTarget* const LocalizationTarget : Settings->TargetObjects)
		{
			UpdateTargetFromReports(LocalizationTarget);
		}
	}

	void FProjectLocalizationSettingsDetailsCustomization::ImportAllTargets()
	{
		const TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(DetailLayoutBuilder->GetDetailsView().AsShared());
		TArray<FLocalizationTargetSettings*> TargetSettings;
		for (ULocalizationTarget* const TargetObject : Settings->TargetObjects)
		{
			TargetSettings.Add(&TargetObject->Settings);
		}
		LocalizationCommandletTasks::ImportTargets(ParentWindow.ToSharedRef(), TargetSettings);

		for (ULocalizationTarget* const LocalizationTarget : Settings->TargetObjects)
		{
			UpdateTargetFromReports(LocalizationTarget);
		}
	}

	void FProjectLocalizationSettingsDetailsCustomization::ExportAllTargets()
	{
		const TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(DetailLayoutBuilder->GetDetailsView().AsShared());
		TArray<FLocalizationTargetSettings*> TargetSettings;
		for (ULocalizationTarget* const TargetObject : Settings->TargetObjects)
		{
			TargetSettings.Add(&TargetObject->Settings);
		}
		LocalizationCommandletTasks::ExportTargets(ParentWindow.ToSharedRef(), TargetSettings);
	}

	void FProjectLocalizationSettingsDetailsCustomization::UpdateTargetFromReports(ULocalizationTarget* const LocalizationTarget)
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

	TSharedRef<ITableRow> FProjectLocalizationSettingsDetailsCustomization::OnGenerateRow(TSharedPtr<IPropertyHandle> TargetObjectPropertyHandle, const TSharedRef<STableViewBase>& Table)
	{
		return SNew(SLocalizationDashboardTargetRow, Table, DetailLayoutBuilder->GetPropertyUtilities(), TargetObjectPropertyHandle.ToSharedRef());
	}

	FReply FProjectLocalizationSettingsDetailsCustomization::OnNewTargetButtonClicked()
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

void SLocalizationDashboard::Construct(const FArguments& InArgs, UProjectLocalizationSettings* const InSettings)
{
	check(InSettings);

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs(false, false, false, FDetailsViewArgs::ENameAreaSettings::HideNameArea, false, nullptr, false, NAME_None);
	TSharedRef<IDetailsView> DetailsView = PropertyModule.CreateDetailView(DetailsViewArgs);
	DetailsView->RegisterInstancedCustomPropertyLayout(UProjectLocalizationSettings::StaticClass(), FOnGetDetailCustomizationInstance::CreateLambda([&]()->TSharedRef<IDetailCustomization>{return MakeShareable(new FProjectLocalizationSettingsDetailsCustomization(InSettings));}));

	ChildSlot
		[
			DetailsView
		];

	DetailsView->SetObject(InSettings, true);
}
#undef LOCTEXT_NAMESPACE
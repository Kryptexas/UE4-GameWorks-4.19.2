// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "DataTableEditorPrivatePCH.h"

#include "DataTableEditor.h"
#include "Toolkits/IToolkitHost.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "SSearchBox.h"
#include "SDockTab.h"
#include "SRowEditor.h"
#include "Engine/DataTable.h"
#include "Json.h"
 
#define LOCTEXT_NAMESPACE "DataTableEditor"

const FName FDataTableEditor::DataTableTabId( TEXT( "DataTableEditor_DataTable" ) );
const FName FDataTableEditor::RowEditorTabId(TEXT("DataTableEditor_RowEditor"));

void FDataTableEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	WorkspaceMenuCategory = TabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_Data Table Editor", "Data Table Editor"));

	TabManager->RegisterTabSpawner( DataTableTabId, FOnSpawnTab::CreateSP(this, &FDataTableEditor::SpawnTab_DataTable) )
		.SetDisplayName( LOCTEXT("DataTableTab", "Data Table") )
		.SetGroup( WorkspaceMenuCategory.ToSharedRef() );

	TabManager->RegisterTabSpawner(RowEditorTabId, FOnSpawnTab::CreateSP(this, &FDataTableEditor::SpawnTab_RowEditor))
		.SetDisplayName(LOCTEXT("RowEditorTab", "Row Editor"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FDataTableEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	TabManager->UnregisterTabSpawner( DataTableTabId );
	TabManager->UnregisterTabSpawner(RowEditorTabId);
}

FDataTableEditor::FDataTableEditor()
{
}

FDataTableEditor::~FDataTableEditor()
{
	if (DataTable.IsValid())
	{
		SaveLayoutData();
	}
}

void FDataTableEditor::PreChange(const class UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info)
{
}

void FDataTableEditor::PostChange(const class UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info)
{
	if (Struct && DataTable.IsValid() && (DataTable->RowStruct == Struct))
	{
		CachedDataTable.Empty();
		ReloadVisibleData();
	}
}

void FDataTableEditor::PreChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info)
{
}

void FDataTableEditor::PostChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info)
{
	FStringAssetReference::InvalidateTag(); // Should be removed after UE-5615 is fixed
	if (Changed == DataTable.Get())
	{
		CachedDataTable.Empty();
		ReloadVisibleData();
	}
}

void FDataTableEditor::InitDataTableEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table )
{
	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout( "Standalone_DataTableEditor_Layout" )
	->AddArea
	(
		FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->AddTab( DataTableTabId, ETabState::OpenedTab )
		)
		->Split
		(
			FTabManager::NewStack()
			->AddTab(RowEditorTabId, ETabState::OpenedTab)
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = false;
	FAssetEditorToolkit::InitAssetEditor( Mode, InitToolkitHost, FDataTableEditorModule::DataTableEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, Table );
	
	FDataTableEditorModule& DataTableEditorModule = FModuleManager::LoadModuleChecked<FDataTableEditorModule>( "DataTableEditor" );
	AddMenuExtender(DataTableEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	// @todo toolkit world centric editing
	/*// Setup our tool's layout
	if( IsWorldCentricAssetEditor() )
	{
		const FString TabInitializationPayload(TEXT(""));		// NOTE: Payload not currently used for table properties
		SpawnToolkitTab( DataTableTabId, TabInitializationPayload, EToolkitTabSpot::Details );
	}*/

	// NOTE: Could fill in asset editor commands here!
}

FName FDataTableEditor::GetToolkitFName() const
{
	return FName("DataTableEditor");
}

FText FDataTableEditor::GetBaseToolkitName() const
{
	return LOCTEXT( "AppLabel", "DataTable Editor" );
}


FString FDataTableEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "DataTable ").ToString();
}


FLinearColor FDataTableEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.0f, 0.0f, 0.2f, 0.5f );
}

FSlateColor FDataTableEditor::GetRowColor(FName RowName) const
{
	if (RowName == HighlightedRowName)
	{
		return FSlateColor(FColorList::Orange);
	}
	return FSlateColor::UseForeground();
}

FReply FDataTableEditor::OnRowClicked(const FGeometry&, const FPointerEvent&, FName RowName)
{
	if (HighlightedRowName != RowName)
	{
		SetHighlightedRow(RowName);
		CallbackOnRowHighlighted.ExecuteIfBound(HighlightedRowName);
	}

	return FReply::Handled();
}

float FDataTableEditor::GetColumnWidth(const int32 ColumnIndex)
{
	if (ColumnWidths.IsValidIndex(ColumnIndex))
	{
		return ColumnWidths[ColumnIndex].CurrentWidth;
	}
	return 0.0f;
}

void FDataTableEditor::OnColumnResized(const float NewWidth, const int32 ColumnIndex)
{
	if (ColumnWidths.IsValidIndex(ColumnIndex))
	{
		FColumnWidth& ColumnWidth = ColumnWidths[ColumnIndex];
		ColumnWidth.bIsAutoSized = false;
		ColumnWidth.CurrentWidth = NewWidth;

		// Update the persistent column widths in the layout data
		{
			if (!LayoutData.IsValid())
			{
				LayoutData = MakeShareable(new FJsonObject());
			}

			TSharedPtr<FJsonObject> LayoutColumnWidths;
			if (!LayoutData->HasField(TEXT("ColumnWidths")))
			{
				LayoutColumnWidths = MakeShareable(new FJsonObject());
				LayoutData->SetObjectField(TEXT("ColumnWidths"), LayoutColumnWidths);
			}
			else
			{
				LayoutColumnWidths = LayoutData->GetObjectField(TEXT("ColumnWidths"));
			}

			const FString& ColumnName = CachedRawColumnNames[ColumnIndex];
			LayoutColumnWidths->SetNumberField(ColumnName, NewWidth);
		}
	}
}

void FDataTableEditor::LoadLayoutData()
{
	LayoutData.Reset();

	if (!DataTable.IsValid())
	{
		return;
	}

	const FString LayoutDataFilename = FPaths::GameSavedDir() / TEXT("AssetData") / TEXT("DataTableEditorLayout") / DataTable->GetName() + TEXT(".json");

	FString JsonText;
	if (FFileHelper::LoadFileToString(JsonText, *LayoutDataFilename))
	{
		TSharedRef< TJsonReader<TCHAR> > JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonText);
		FJsonSerializer::Deserialize(JsonReader, LayoutData);
	}
}

void FDataTableEditor::SaveLayoutData()
{
	if (!DataTable.IsValid() || !LayoutData.IsValid())
	{
		return;
	}

	const FString LayoutDataFilename = FPaths::GameSavedDir() / TEXT("AssetData") / TEXT("DataTableEditorLayout") / DataTable->GetName() + TEXT(".json");

	FString JsonText;
	TSharedRef< TJsonWriter< TCHAR, TPrettyJsonPrintPolicy<TCHAR> > > JsonWriter = TJsonWriterFactory< TCHAR, TPrettyJsonPrintPolicy<TCHAR> >::Create(&JsonText);
	if (FJsonSerializer::Serialize(LayoutData.ToSharedRef(), JsonWriter))
	{
		FFileHelper::SaveStringToFile(JsonText, *LayoutDataFilename);
	}
}

TSharedRef<SWidget> FDataTableEditor::CreateGridPanel()
{
	TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);

	if (DataTable.IsValid())
	{
		if (CachedDataTable.Num() == 0)
		{
			CachedDataTable = DataTable->GetTableData();
			check(CachedDataTable.Num() > 0); // There should always be at least the column titles row

			CachedRawColumnNames = DataTable->GetUniqueColumnTitles();

			RowsVisibility.SetNum(CachedDataTable.Num());
			for (bool& RowVisibility : RowsVisibility)
			{
				RowVisibility = true;
			}

			ColumnWidths.SetNum(CachedRawColumnNames.Num());
			
			// Load the persistent column widths from the layout data
			{
				const TSharedPtr<FJsonObject>* LayoutColumnWidths = nullptr;
				if (LayoutData.IsValid() && LayoutData->TryGetObjectField(TEXT("ColumnWidths"), LayoutColumnWidths))
				{
					for(int32 ColumnIndex = 0; ColumnIndex < CachedRawColumnNames.Num(); ++ColumnIndex)
					{
						const FString& ColumnName = CachedRawColumnNames[ColumnIndex];

						double LayoutColumnWidth = 0.0f;
						if ((*LayoutColumnWidths)->TryGetNumberField(ColumnName, LayoutColumnWidth))
						{
							FColumnWidth& ColumnWidth = ColumnWidths[ColumnIndex];
							ColumnWidth.bIsAutoSized = false;
							ColumnWidth.CurrentWidth = static_cast<float>(LayoutColumnWidth);
						}
					}
				}
			}
		}

		check(CachedDataTable.Num() > 0 && CachedDataTable.Num() == RowsVisibility.Num());
		check(CachedDataTable.Num() > 0 && CachedDataTable[0].Num() == ColumnWidths.Num());
		check(CachedRawColumnNames.Num() > 0 && CachedRawColumnNames.Num() == ColumnWidths.Num());

		int32 VisibleRowIndex = 0;
		TArray<FString>& ColumnTitles = CachedDataTable[0];
		for(int32 RowIndex = 0; RowIndex < CachedDataTable.Num(); ++RowIndex)
		{
			if (RowsVisibility[RowIndex])
			{
				const bool bIsHeader = (RowIndex == 0);
				const FLinearColor RowColor = (VisibleRowIndex % 2 == 0) ? FLinearColor::Gray : FLinearColor::Black;

				TArray<FString>& Row = CachedDataTable[RowIndex];
				FName RowName(*Row[0]);
				TAttribute<FSlateColor> ForegroundColor = bIsHeader
					? FSlateColor::UseForeground()
					: TAttribute<FSlateColor>::Create(
					TAttribute<FSlateColor>::FGetter::CreateSP(this, &FDataTableEditor::GetRowColor, RowName));

				auto RowClickCallback = bIsHeader
					? FPointerEventHandler()
					: FPointerEventHandler::CreateSP(this, &FDataTableEditor::OnRowClicked, RowName);

				const float SplitterHandleSize = 2.0f;
				TSharedRef<SSplitter> RowSplitter = SNew(SSplitter)
					.PhysicalSplitterHandleSize(SplitterHandleSize)
					.HitDetectionSplitterHandleSize(4.0f);

				VerticalBox->AddSlot()
				[
					RowSplitter
				];

				float ColumnDesiredWidth = 0.0f;
				for(int32 ColumnIndex = 0; ColumnIndex < Row.Num(); ++ColumnIndex)
				{
					TSharedPtr<SWidget> ColumnEntryWidget;

					RowSplitter->AddSlot()
					.Value(TAttribute<float>::Create(TAttribute<float>::FGetter::CreateSP(this, &FDataTableEditor::GetColumnWidth, ColumnIndex)))
					.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &FDataTableEditor::OnColumnResized, ColumnIndex))
					.SizeRule(SSplitter::AbsoluteSize)
					[
						SAssignNew(ColumnEntryWidget, SBorder)
						.Padding(2.0f)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.BorderBackgroundColor(RowColor)
						.ForegroundColor(ForegroundColor)
						.OnMouseButtonDown(RowClickCallback)
						[
							SNew(STextBlock)
							.Text(FText::FromString(Row[ColumnIndex]))
							.ToolTipText(bIsHeader 
							?	(FText::Format(LOCTEXT("ColumnHeaderNameFmt", "Column '{0}'"), FText::FromString(ColumnTitles[ColumnIndex]))) 
							:	(FText::Format(LOCTEXT("ColumnRowNameFmt", "{0}: {1}"), FText::FromString(ColumnTitles[ColumnIndex]), FText::FromString(Row[ColumnIndex]))))
						]
					];

					FColumnWidth& ColumnWidth = ColumnWidths[ColumnIndex];
					if (ColumnWidth.bIsAutoSized)
					{
						ColumnEntryWidget->SlatePrepass(1.0f);
						ColumnWidth.CurrentWidth = FMath::Max(ColumnWidth.CurrentWidth, ColumnEntryWidget->GetDesiredSize().X + SplitterHandleSize);
					}
				}

				// Dummy splitter slot to allow the last column to be resized
				RowSplitter->AddSlot()
				.Value(TAttribute<float>::Create(TAttribute<float>::FGetter::CreateSP(this, &FDataTableEditor::GetColumnWidth, static_cast<int32>(INDEX_NONE))))
				.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &FDataTableEditor::OnColumnResized, static_cast<int32>(INDEX_NONE)))
				.SizeRule(SSplitter::AbsoluteSize)
				[
					SNullWidget::NullWidget
				];

				++VisibleRowIndex;
			}
		}

		// Update the currently selected row to try and ensure it's valid
		if (HighlightedRowName.IsNone() || !DataTable->RowMap.Contains(HighlightedRowName))
		{
			if (CachedDataTable.Num() > 2)
			{
				// 1 is the first row in the table
				// 0 is the name column
				SetHighlightedRow(*CachedDataTable[1][0]);
			}
			else
			{
				SetHighlightedRow(NAME_None);
			}
		}
	}

	return VerticalBox;
}

void FDataTableEditor::OnSearchTextChanged(const FText& SearchText)
{
	FString SearchFor = SearchText.ToString();
	if (!SearchFor.IsEmpty())
	{
		check(CachedDataTable.Num() == RowsVisibility.Num());

		// starting from index 1, because 0 is header
		for (int32 RowIdx = 1; RowIdx < CachedDataTable.Num(); ++RowIdx)
		{
			RowsVisibility[RowIdx] = false;

			for (int32 i = 0; i < CachedDataTable[RowIdx].Num(); ++i)
			{
				if (SearchFor.Len() <= CachedDataTable[RowIdx][i].Len())
				{
					if (CachedDataTable[RowIdx][i].Contains(SearchFor))
					{
						RowsVisibility[RowIdx] = true;
						break;
					}
				}
			}
		}
	}
	else
	{
		for (int32 RowIdx = 0; RowIdx < RowsVisibility.Num(); ++RowIdx)
		{
			RowsVisibility[RowIdx] = true;
		}
	}

	ReloadVisibleData();
}

void FDataTableEditor::ReloadVisibleData()
{
	if (ScrollBoxWidget.IsValid())
	{
		ScrollBoxWidget->ClearChildren();
		ScrollBoxWidget->AddSlot()
			[
				CreateGridPanel()
			];
	}
}

TSharedRef<SVerticalBox> FDataTableEditor::CreateContentBox()
{
	TSharedRef<SScrollBar> HorizontalScrollBar = SNew(SScrollBar)
		.Orientation(Orient_Horizontal)
		.Thickness(FVector2D(5, 5));

	TSharedRef<SScrollBar> VerticalScrollBar = SNew(SScrollBar)
		.Orientation(Orient_Vertical)
		.Thickness(FVector2D(5, 5));

	return SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(SearchBox, SSearchBox)
			.OnTextChanged(this, &FDataTableEditor::OnSearchTextChanged)
		]
		+SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			[
				SNew(SScrollBox)
				.Orientation(Orient_Horizontal)
				.ExternalScrollbar(HorizontalScrollBar)
				+SScrollBox::Slot()
				[
					SAssignNew(ScrollBoxWidget, SScrollBox)
					.Orientation(Orient_Vertical)
					.ExternalScrollbar(VerticalScrollBar)
					.ConsumeMouseWheel(EConsumeMouseWheel::Always) // Always consume the mouse wheel events to prevent the outer scroll box from scrolling
					+SScrollBox::Slot()
					[
						CreateGridPanel()
					]
				]
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				VerticalScrollBar
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			HorizontalScrollBar
		];
}

TSharedRef<SWidget> FDataTableEditor::CreateRowEditorBox()
{
	auto RowEditor = SNew(SRowEditor, DataTable.Get());
	RowEditor->RowSelectedCallback.BindSP(this, &FDataTableEditor::SetHighlightedRow);
	CallbackOnRowHighlighted.BindSP(RowEditor, &SRowEditor::SelectRow);
	return RowEditor;
}

TSharedRef<SDockTab> FDataTableEditor::SpawnTab_RowEditor(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == RowEditorTabId);

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("DataTableEditor.Tabs.Properties"))
		.Label(LOCTEXT("RowEditorTitle", "Row Editor"))
		.TabColorScale(GetTabColorScale())
		[
			SNew(SBorder)
			.Padding(2)
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Fill)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				CreateRowEditorBox()
			]
		];
}


TSharedRef<SDockTab> FDataTableEditor::SpawnTab_DataTable( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == DataTableTabId );

	DataTable = Cast<UDataTable>(GetEditingObject());

	LoadLayoutData();

	TSharedRef<SVerticalBox> ContentBox = CreateContentBox();

	GridPanelOwner = 
		SNew(SBorder)
		.Padding(2)
		.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
		[
			ContentBox
		];

	return SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("DataTableEditor.Tabs.Properties") )
		.Label( LOCTEXT("DataTableTitle", "Data Table") )
		.TabColorScale( GetTabColorScale() )
		[
			GridPanelOwner.ToSharedRef()
		];
}

void FDataTableEditor::SetHighlightedRow(FName Name)
{
	HighlightedRowName = Name;
}

#undef LOCTEXT_NAMESPACE

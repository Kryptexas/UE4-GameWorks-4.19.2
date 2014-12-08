// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizerPCH.h"
#include "SLogBar.h"
#include "Debug/LogVisualizerCameraController.h"
#include "Debug/ReporterGraph.h"
#include "MainFrame.h"
#include "DesktopPlatformModule.h"
#include "Json.h"
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"

#if WITH_EDITOR
#	include "Editor/UnrealEd/Public/EditorComponents.h"
#	include "Editor/UnrealEd/Public/EditorReimportHandler.h"
#	include "Editor/UnrealEd/Public/TexAlignTools.h"
#	include "Editor/UnrealEd/Public/TickableEditorObject.h"
#	include "UnrealEdClasses.h"
#	include "Editor/UnrealEd/Public/Editor.h"
#	include "Editor/UnrealEd/Public/EditorViewportClient.h"
#endif

#include "VisualLogger/VisualLoggerBinaryFileDevice.h"
#include "GameplayDebuggingComponent.h"
#include "LogVisualizerModule.h"
#include "SLogVisualizerReport.h"

#include "SFilterList.h"

#if ENABLE_VISUAL_LOG

#define LOCTEXT_NAMESPACE "SLogVisualizer"

const FName SLogVisualizer::NAME_LogName = TEXT("LogName");
const FName SLogVisualizer::NAME_StartTime = TEXT("StartTime");
const FName SLogVisualizer::NAME_EndTime = TEXT("EndTime");
const FName SLogVisualizer::NAME_LogTimeSpan = TEXT("LogTimeSpan");

namespace LogVisualizer
{
	static const FString LogFileDescription = LOCTEXT("FileTypeDescription", "Visual Log File").ToString();
	static const FString LoadFileTypes = FString::Printf(TEXT("%s (*.vlog;*.%s)|*.vlog;*.%s"), *LogFileDescription, VISLOG_FILENAME_EXT, VISLOG_FILENAME_EXT);
	static const FString SaveFileTypes = FString::Printf(TEXT("%s (*.%s)|*.%s"), *LogFileDescription, VISLOG_FILENAME_EXT, VISLOG_FILENAME_EXT);

	FORCEINLINE bool PointInFrustum(UCanvas* Canvas, const FVector& Location)
	{
		return Canvas->SceneView->ViewFrustum.IntersectBox(Location, FVector::ZeroVector);
	}

	FORCEINLINE void DrawText(UCanvas* Canvas, UFont* Font, const FString& TextToDraw, const FVector& WorldLocation)
	{
		if (PointInFrustum(Canvas, WorldLocation))
		{
			const FVector ScreenLocation = Canvas->Project(WorldLocation);
			Canvas->DrawText(Font, *TextToDraw, ScreenLocation.X, ScreenLocation.Y);
		}
	}

	FORCEINLINE void DrawTextCentered(UCanvas* Canvas, UFont* Font, const FString& TextToDraw, const FVector& WorldLocation)
	{
		if (PointInFrustum(Canvas, WorldLocation))
		{
			const FVector ScreenLocation = Canvas->Project(WorldLocation);
			float TextXL = 0.f;
			float TextYL = 0.f;
			Canvas->StrLen(Font, TextToDraw, TextXL, TextYL);
			Canvas->DrawText(Font, *TextToDraw, ScreenLocation.X - TextXL / 2.0f, ScreenLocation.Y - TextYL / 2.0f);
		}
	}

	FORCEINLINE void DrawTextShadowed(UCanvas* Canvas, UFont* Font, const FString& TextToDraw, const FVector& WorldLocation)
	{
		if (PointInFrustum(Canvas, WorldLocation))
		{
			const FVector ScreenLocation = Canvas->Project(WorldLocation);
			float TextXL = 0.f;
			float TextYL = 0.f;
			Canvas->StrLen(Font, TextToDraw, TextXL, TextYL);
			Canvas->SetDrawColor(FColor::Black);
			Canvas->DrawText(Font, *TextToDraw, 1 + ScreenLocation.X - TextXL / 2.0f, 1 + ScreenLocation.Y - TextYL / 2.0f);
			Canvas->SetDrawColor(FColor::White);
			Canvas->DrawText(Font, *TextToDraw, ScreenLocation.X - TextXL / 2.0f, ScreenLocation.Y - TextYL / 2.0f);
		}
	}
}

FColor SLogVisualizer::ColorPalette[] = {
	FColor(0xff00A480),
	FColorList::Aquamarine,
	FColorList::Cyan,
	FColorList::Brown,
	FColorList::Green,
	FColorList::Orange,
	FColorList::Magenta,
	FColorList::BrightGold,
	FColorList::NeonBlue,
	FColorList::MediumSlateBlue,
	FColorList::SpicyPink,
	FColorList::SpringGreen,
	FColorList::SteelBlue,
	FColorList::SummerSky,
	FColorList::Violet,
	FColorList::VioletRed,
	FColorList::YellowGreen,
	FColor(0xff62E200),
	FColor(0xff1F7B67),
	FColor(0xff62AA2A),
	FColor(0xff70227E),
	FColor(0xff006B53),
	FColor(0xff409300),
	FColor(0xff5D016D),
	FColor(0xff34D2AF),
	FColor(0xff8BF13C),
	FColor(0xffBC38D3),
	FColor(0xff5ED2B8),
	FColor(0xffA6F16C),
	FColor(0xffC262D3),
	FColor(0xff0F4FA8),
	FColor(0xff00AE68),
	FColor(0xffDC0055),
	FColor(0xff284C7E),
	FColor(0xff21825B),
	FColor(0xffA52959),
	FColor(0xff05316D),
	FColor(0xff007143),
	FColor(0xff8F0037),
	FColor(0xff4380D3),
	FColor(0xff36D695),
	FColor(0xffEE3B80),
	FColor(0xff6996D3),
	FColor(0xff60D6A7),
	FColor(0xffEE6B9E)
};

template <typename ItemType>
class SLogListView : public SListView<ItemType>
{
public:
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (!MouseEvent.IsLeftShiftDown())
		{
			return SListView<ItemType>::OnMouseWheel(MyGeometry, MouseEvent);
		};

		return FReply::Unhandled();
	}

		void RefreshList()
	{
		const TArray<ItemType>& ItemsSourceRef = (*this->ItemsSource);

		for (int32 Index = 0; Index < ItemsSourceRef.Num(); ++Index)
		{
			TSharedPtr< SLogsTableRow > TableRow = StaticCastSharedPtr< SLogsTableRow >(this->WidgetGenerator.GetWidgetForItem(ItemsSourceRef[Index]));
			if (TableRow.IsValid())
			{
				TableRow->UpdateEntries();
			}
		}

	}
};

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLogVisualizer::Construct(const FArguments& InArgs, FLogVisualizer* InLogVisualizer)
{
	LogVisualizer = InLogVisualizer;
	SortBy = ELogsSortMode::ByName;
	LogEntryIndex = INDEX_NONE;
	SelectedLogIndex = INDEX_NONE;
	LogsStartTime = FLT_MAX;
	LogsEndTime = -FLT_MAX;
	ScrollbarOffset = 0.f;
	ZoomSliderValue = 0.f;
	LastBarsOffset = 0.f;
	MinZoom = 1.0f;
	MaxZoom = 20.0f;
	CurrentViewedTime = 0.f;
	bDrawLogEntriesPath = true;
	bIgnoreTrivialLogs = true;
	HistogramPreviewWindow = 50;
	bShowHistogramLabelsOutside = false;
	bStickToLastData = false;
	bOffsetDataSet = false;
	bHistogramGraphsFilter = true;

	UsedCategories.Empty();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
		
			// Toolbar
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew( SOverlay )
				+ SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					// Record button
					+SHorizontalBox::Slot()
					.Padding(1)
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.OnClicked(this, &SLogVisualizer::OnRecordButtonClicked)
						.Content()
						[
							SNew(SImage)
							.Image(this, &SLogVisualizer::GetRecordButtonBrush)
						]
					]
					// 'Pause' toggle button
					+SHorizontalBox::Slot()
					.Padding(1)
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
						.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
						.Type(ESlateCheckBoxType::ToggleButton)
						.OnCheckStateChanged(this, &SLogVisualizer::OnPauseChanged)
						.IsChecked(this, &SLogVisualizer::GetPauseState)
						.Content()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("LogVisualizer.Pause"))
						]
					]
					// 'Camera' toggle button
					+SHorizontalBox::Slot()
					.Padding(1)
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
						.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
						.OnCheckStateChanged(this, &SLogVisualizer::OnToggleCamera)
						.IsChecked(this, &SLogVisualizer::GetToggleCameraState)
						.Content()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("LogVisualizer.Camera"))
						]
					]
					+SHorizontalBox::Slot()
					.MaxWidth(3)
					.Padding(1)
					[
						SNew(SSeparator)
						.Orientation(Orient_Vertical)
					]
					// 'Save' function
					+SHorizontalBox::Slot()
					.Padding(1)
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.OnClicked(this, &SLogVisualizer::OnSave)
						.Content()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("LogVisualizer.Save"))
						]
					]
					// 'Load' function
					+SHorizontalBox::Slot()
					.Padding(1)
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.OnClicked(this, &SLogVisualizer::OnLoad)
						.Content()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("LogVisualizer.Load"))
						]
					]
					// 'Remove' function
					+SHorizontalBox::Slot()
					.Padding(1)
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.OnClicked(this, &SLogVisualizer::OnRemove)
						.Content()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("LogVisualizer.Remove"))
						]
					]
					+SHorizontalBox::Slot()
					.MaxWidth(3)
					.Padding(1)
					[
						SNew(SSeparator)
						.Orientation(Orient_Vertical)
					]
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Right)
				.Padding(4)
				[
					SNew(SComboButton)
					.ComboButtonStyle(FEditorStyle::Get(), "ContentBrowser.Filters.Style")
					.ForegroundColor(FLinearColor::White)
					.ContentPadding(0)
					.ToolTipText(LOCTEXT("SettingsToolTip", "Log Visualizer settings."))
					.HasDownArrow(true)
					.ContentPadding(FMargin(1, 0))
					.ButtonContent()
					[
						SNew(STextBlock)
						.TextStyle(FEditorStyle::Get(), "ContentBrowser.Filters.Text")
						.Text(LOCTEXT("Settings", "Settings"))
					]
					.MenuContent()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SCheckBox)
								.OnCheckStateChanged(this, &SLogVisualizer::OnDrawLogEntriesPathChanged)
								.IsChecked(this, &SLogVisualizer::GetDrawLogEntriesPathState)
								.Content()
								[
									SNew(STextBlock)
									.Text(LOCTEXT("VisLogDrawLogsPath", "Draw Log\'s path"))
									.ToolTipText(LOCTEXT("VisLogDrawLogsPathTooltip", "Toggle whether path of composed of log entries\' locations"))
								]
							]
						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SCheckBox)
								.OnCheckStateChanged(this, &SLogVisualizer::OnIgnoreTrivialLogs)
								.IsChecked(this, &SLogVisualizer::GetIgnoreTrivialLogs)
								.Content()
								[
									SNew(STextBlock)
									.Text(LOCTEXT("VisLogIgnoreTrivialLogs", "Ignore trivial logs"))
									.ToolTipText(LOCTEXT("VisLogIgnoreTrivialLogsTooltip", "Whether to show trivial logs, i.e. the ones with only one entry."))
								]
							]
						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SCheckBox)
								.OnCheckStateChanged(this, &SLogVisualizer::OnChangeHistogramLabelLocation)
								.IsChecked(this, &SLogVisualizer::GetHistogramLabelLocation)
								.Content()
								[
									SNew(STextBlock)
									.Text(LOCTEXT("VisLogHistogramLabelLocation", "Set histogram labels outside graph"))
									.ToolTipText(LOCTEXT("VisLogHistogramLabelLocationTooltip", "Whether to show histogram labels inside graph or outside."))
								]
							]
						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SCheckBox)
								.OnCheckStateChanged(this, &SLogVisualizer::OnStickToLastData)
								.IsChecked(this, &SLogVisualizer::GetStickToLastData)
								.Content()
								[
									SNew(STextBlock)
									.Text(LOCTEXT("VisLogStickToLastData", "Stick to recent data"))
									.ToolTipText(LOCTEXT("VisLogStickToLastDataTooltip", "Whether to show the recent data or not."))
								]
							]
						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SCheckBox)
								.OnCheckStateChanged(this, &SLogVisualizer::OnOffsetDataSets)
								.IsChecked(this, &SLogVisualizer::GetOffsetDataSets)
								.Content()
								[
									SNew(STextBlock)
									.Text(LOCTEXT("VisLogOffsetDataSets", "Offset data sets"))
									.ToolTipText(LOCTEXT("VisLogOffsetDataSetsTooltip", "Offset data sets on graphs to make it easier to read"))
								]
							]
					]
				]
			]

			// Filters
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(FilterListPtr, SLogFilterList)
				.OnFilterChanged(this, &SLogVisualizer::OnLogCategoryFiltersChanged)
				.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("CategoryFilters")))
				/*.OnGetContextMenu(this, &SLogVisualizer::GetFilterContextMenu)*/
				/*.FrontendFilters(FrontendFilters)*/
			]

			+SVerticalBox::Slot()
			.FillHeight(5)
			[
				SNew(SSplitter)
				.Orientation(Orient_Vertical)

				+SSplitter::Slot()
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
					.Padding(1.0)
					[
						SAssignNew(LogsListWidget, SLogListView< TSharedPtr<FLogsListItem> >)
						.ItemHeight(20)
						// Called when the user double-clicks with LMB on an item in the list
						.OnMouseButtonDoubleClick(this, &SLogVisualizer::OnListDoubleClick)
						.OnContextMenuOpening(this, &SLogVisualizer::GetListRightClickMenuContent)
						.ListItemsSource(&LogsList)
						.SelectionMode(ESelectionMode::Multi)
						.OnGenerateRow(this, &SLogVisualizer::LogsListGenerateRow)
						.OnSelectionChanged(this, &SLogVisualizer::LogsListSelectionChanged)
						.HeaderRow(
							SNew(SHeaderRow)
							// ID
							+SHeaderRow::Column(NAME_LogName)
							.SortMode(this, &SLogVisualizer::GetLogsSortMode)
							.OnSort(this, &SLogVisualizer::OnSortByChanged)
							.HAlignCell(HAlign_Left)
							.FillWidth(0.25f)
							[
								SNew(SHorizontalBox)
								+SHorizontalBox::Slot()
								.AutoWidth()
								.HAlign(HAlign_Left)
								.Padding(0.0, 2.0)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("VisLogName", "Log Subject"))
								]
								+SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(5,0)
								[
									SAssignNew(LogNameFilterBox, SEditableTextBox)
									.SelectAllTextWhenFocused(true)
									.OnTextCommitted(this, &SLogVisualizer::FilterTextCommitted)
									.MinDesiredWidth(90.f)
									.RevertTextOnEscape(true)
								]
							]
							+SHeaderRow::Column(NAME_LogTimeSpan)
							.VAlignCell(VAlign_Center)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.HAlign(HAlign_Center)
								.Padding(0.0, 2.0)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("VisLogFilterName", "Quick Filter"))
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(5, 0)
								[
									SAssignNew(QuickFilterBox, SEditableTextBox)
									.SelectAllTextWhenFocused(true)
									.OnTextChanged(this, &SLogVisualizer::OnQuickFilterTextChanged, ETextCommit::Default)
									.MinDesiredWidth(170.f)
									.RevertTextOnEscape(true)
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SCheckBox)
									.OnCheckStateChanged(this, &SLogVisualizer::OnHistogramGraphsFilter)
									.IsChecked(this, &SLogVisualizer::GetHistogramGraphsFilter)
									.Content()
									[
										SNew(STextBlock)
										.Text(LOCTEXT("VisLogHistogramGraphsFilter", "histogram graphs filter"))
										.ToolTipText(LOCTEXT("VisLogHistogramGraphsFilterTooltip", "Whether to filter histogram graphs or regular categories."))
									]
								]
							]
						)
					]
				]
				+SSplitter::Slot()		
				[					
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
					.Padding(1.0)
					[
						SNew(SVerticalBox)

						+SVerticalBox::Slot()
						.AutoHeight()
						.MaxHeight(60)
						[
							SAssignNew( Timeline, STimeline )
							.MinValue(0.0f)
							.MaxValue(100.0f)
							.FixedLabelSpacing(100.f)
						]

						+SVerticalBox::Slot()
						.AutoHeight() 
						.Padding( 2 )
						.VAlign( VAlign_Fill )
						[
							SAssignNew( ScrollBar, SScrollBar )
							.Orientation( Orient_Horizontal )
							.OnUserScrolled( this, &SLogVisualizer::OnZoomScrolled )
						]

						+SVerticalBox::Slot()
						.AutoHeight() 
						.Padding( 2 )
						[
							SAssignNew(ZoomSlider, SSlider)
								.Value( this, &SLogVisualizer::GetZoomValue )
								.OnValueChanged( this, &SLogVisualizer::OnSetZoomValue )
						]
			
						+SVerticalBox::Slot()
						.Padding(2)
						.FillHeight(3)
						//.VAlign(VAlign_Fill)
						[
							SNew(SSplitter)

							+SSplitter::Slot()
							.Value(1)
							[
								SNew( SBorder )
								.Padding(1)
								.BorderImage( FEditorStyle::GetBrush( "ToolBar.Background" ) )
								[
									SAssignNew(StatusItemsView, STreeView<TSharedPtr<FLogStatusItem> >)
									.ItemHeight(40.0f)
									.TreeItemsSource(&StatusItems)
									.OnGenerateRow(this, &SLogVisualizer::HandleGenerateLogStatus)
									.OnGetChildren(this, &SLogVisualizer::OnLogStatusGetChildren)
									.SelectionMode(ESelectionMode::None)
								]
							]
							+SSplitter::Slot()
							.Value(3)
							[
								SNew( SBorder )
								.Padding(1)
								.BorderImage( FEditorStyle::GetBrush( "ToolBar.Background" ) )	
								[
									SAssignNew(LogsLinesWidget, SListView<TSharedPtr<FLogEntryItem> >)
									.ItemHeight(20)
									.ListItemsSource(&LogEntryLines)
									.SelectionMode(ESelectionMode::Single)
									.OnSelectionChanged(this, &SLogVisualizer::LogEntryLineSelectionChanged)
									.OnGenerateRow(this, &SLogVisualizer::LogEntryLinesGenerateRow)
								]
							]
						]
					]
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				// Status area
				SNew(STextBlock)
				.Text(this, &SLogVisualizer::GetStatusText)
			]
		]
	];

	LogVisualizer->OnLogAdded().AddSP(this, &SLogVisualizer::OnLogAdded);

	if (LogsList.Num() == 0)
	{
		Timeline->SetVisibility(EVisibility::Hidden);
		ScrollBar->SetVisibility(EVisibility::Hidden);
		ZoomSlider->SetVisibility(EVisibility::Hidden);
	}

	TArray<TSharedPtr<FActorsVisLog> >& Logs = LogVisualizer->Logs;
	TSharedPtr<FActorsVisLog>* SharedLog = Logs.GetData();
	for (int32 LogIndex = 0; LogIndex < Logs.Num(); ++LogIndex, ++SharedLog)
	{
		if (SharedLog->IsValid())
		{
			AddOrUpdateLog(LogIndex, SharedLog->Get());
		}
	}

	DoFullUpdate();
	
	LastBrowsePath = FPaths::GameLogDir();

	DrawingOnCanvasDelegate = FDebugDrawDelegate::CreateSP(this, &SLogVisualizer::DrawOnCanvas);
	UDebugDrawService::Register(TEXT("VisLog"), DrawingOnCanvasDelegate);
	UGameplayDebuggingComponent::OnDebuggingTargetChangedDelegate.AddSP(this, &SLogVisualizer::SelectionChanged);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SLogVisualizer::~SLogVisualizer()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->DestroyActor(FLogVisualizerModule::Get()->GetHelperActor(World));
	}
	UGameplayDebuggingComponent::OnDebuggingTargetChangedDelegate.RemoveAll(this);
	LogVisualizer->OnLogAdded().RemoveAll(this);
	UDebugDrawService::Unregister(DrawingOnCanvasDelegate);

	InvalidateCanvas();
}

void SLogVisualizer::OnListDoubleClick(TSharedPtr<FLogsListItem> LogListItem)
{
#if WITH_EDITOR
	FVector Orgin, Extent;

	bool bFoundActor = false;
	if (LogVisualizer->Logs.IsValidIndex(LogListItem->LogIndex))
	{
		TSharedPtr<FActorsVisLog>& Log = LogVisualizer->Logs[LogListItem->LogIndex];
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor->GetFName() == Log->Name)
			{
				Actor->GetActorBounds(false, Orgin, Extent);
				bFoundActor = true;
				break;
			}
		}
	}

	if (!bFoundActor)
	{
		Extent = FVector(150, 150, 150);
	}

	if (LogVisualizer->Logs.IsValidIndex(LogListItem->LogIndex))
	{
		TSharedPtr<FActorsVisLog>& Log = LogVisualizer->Logs[LogListItem->LogIndex];
		if (Log.IsValid() && Log->Entries.IsValidIndex(LogEntryIndex))
		{
			Orgin = Log->Entries[LogEntryIndex]->Location;
		}
	}


	UEditorEngine *EEngine = Cast<UEditorEngine>(GEngine);
	if (GIsEditor && EEngine != NULL)
	{
		for (auto ViewportClient : EEngine->AllViewportClients)
		{
			ViewportClient->FocusViewportOnBox(FBox::BuildAABB(Orgin, Extent));
		}
	}
	else if (ALogVisualizerCameraController::IsEnabled(GetWorld()) && CameraController.IsValid() && CameraController->GetSpectatorPawn())
	{
		ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(CameraController->Player);
		if (LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->Viewport)
		{
			FViewport* Viewport = LocalPlayer->ViewportClient->Viewport;

			FBox BoundingBox = FBox::BuildAABB(Orgin, Extent);
			const FVector Position = BoundingBox.GetCenter();
			float Radius = BoundingBox.GetExtent().Size();

			FViewportCameraTransform ViewTransform;
			ViewTransform.TransitionToLocation(Position, true);

			float NewOrthoZoom;
			const float AspectRatio = 1.777777f;
			uint32 MinAxisSize = (AspectRatio > 1.0f) ? Viewport->GetSizeXY().Y : Viewport->GetSizeXY().X;
			float Zoom = Radius / (MinAxisSize / 2);

			NewOrthoZoom = Zoom * (Viewport->GetSizeXY().X*15.0f);
			NewOrthoZoom = FMath::Clamp<float>(NewOrthoZoom, 250, MAX_FLT);
			ViewTransform.SetOrthoZoom(NewOrthoZoom);

			CameraController->GetSpectatorPawn()->TeleportTo(ViewTransform.GetLocation(), ViewTransform.GetRotation(), false, true);
		}
	}
#endif
}

TSharedPtr<SWidget> SLogVisualizer::GetListRightClickMenuContent()
{
	FMenuBuilder MenuBuilder(true, NULL);

	if (IsEnabled())
	{
		MenuBuilder.BeginSection("VisualLogReports", LOCTEXT("VisualLogReports", "VisualLog Reports"));
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("GenarateReport", "Genarate Report"),
				LOCTEXT("GenarateReportTooltip", "Genarate report from Visual Log events."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SLogVisualizer::GenerateReport))
				);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void SLogVisualizer::GenerateReport()
{
	TArray< TSharedPtr<FLogsListItem> > ItemsToSave = LogsListWidget->GetSelectedItems();
	TArray< TSharedPtr<FActorsVisLog> > AllLogs;

	TSharedPtr<FLogsListItem>* LogListItem = ItemsToSave.GetData();
	for (int32 ItemIndex = 0; ItemIndex < ItemsToSave.Num(); ++ItemIndex, ++LogListItem)
	{
		if (LogListItem->IsValid() && LogVisualizer->Logs.IsValidIndex((*LogListItem)->LogIndex))
		{
			TSharedPtr<FActorsVisLog> Log = LogVisualizer->Logs[(*LogListItem)->LogIndex];
			AllLogs.Add(Log);
		}
	}


	TSharedRef<SWindow> NewWindow = SNew(SWindow)
		.ClientSize(FVector2D(720, 768))
		.Title(NSLOCTEXT("LogVisualizerReport", "WindowTitle", "Log Visualizer Report"))
		[
			SNew(SLogVisualizerReport, this, AllLogs)
		];

	FSlateApplication::Get().AddWindow(NewWindow);
}


int32 SLogVisualizer::GetCurrentVisibleLogEntryIndex(const TArray<TSharedPtr<FVisualLogEntry> >& InVisibleEntries)
{
	if (LogVisualizer->Logs.IsValidIndex(SelectedLogIndex))
	{
		TSharedPtr<FActorsVisLog> Log = LogVisualizer->Logs[SelectedLogIndex];
		if (Log.IsValid() && Log->Entries.IsValidIndex(LogEntryIndex))
		{
			for (int32 Index = 0; Index < InVisibleEntries.Num(); ++Index)
			{
				if (InVisibleEntries[Index] == Log->Entries[LogEntryIndex])
				{
					return Index;
				}
			}
		}
	}

	return INDEX_NONE;
}

void SLogVisualizer::UpdateVisibleEntriesCache(const TSharedPtr<FActorsVisLog>& Log, int32 Index)
{
	int32 CurrentIndex = INDEX_NONE;
	int32 LogListNum = LogsList.Num();
	for (int32 LogIndex = 0; LogIndex < LogListNum; ++LogIndex)
	{
		if (LogsList[LogIndex]->LogIndex == Index)
		{
			CurrentIndex = LogIndex;
			break;
		}
	}

	float LastTimestamp = -1;
	if (CurrentIndex != INDEX_NONE)
	{
		LastTimestamp = LogsList[CurrentIndex]->LastEndTimestamp;
	}

	const int32 NumLogs = OutEntriesCached.Num();
	int32 CachedLogIndex = INDEX_NONE;
	for (int32 EntriesIndex = 0; EntriesIndex < NumLogs; ++EntriesIndex)
	{
		if (OutEntriesCached[EntriesIndex].Log == Log)
		{
			CachedLogIndex = EntriesIndex;
			break;
		}
	}

	if (CachedLogIndex == INDEX_NONE)
	{
		FCachedEntries CachedLog;
		CachedLog.Log = Log;
		CachedLogIndex = OutEntriesCached.Add(CachedLog);
	}

	if (FilterListPtr.IsValid())
	{
		for (int32 EntryIndex = 0; EntryIndex < Log->Entries.Num(); ++EntryIndex)
		{
			const TSharedPtr<FVisualLogEntry> CurrentEntry = Log->Entries[EntryIndex];
			if (CurrentEntry->TimeStamp <= LastTimestamp)
			{
				continue;
			}

			bool bAddedEntry = false;
			if (!bAddedEntry)
			{
				for (int32 LogLineIndex = 0; LogLineIndex < CurrentEntry->LogLines.Num(); ++LogLineIndex)
				{
					const FVisualLogLine &CurrentLine = CurrentEntry->LogLines[LogLineIndex];
					const FString CurrentCategory = CurrentLine.Category.ToString();
					if (FilterListPtr->IsFilterEnabled(CurrentCategory, CurrentLine.Verbosity))
					{
						OutEntriesCached[CachedLogIndex].CachedEntries.Add(CurrentEntry);
						bAddedEntry = true;
						break;
					}
				}
			}

			if (bAddedEntry)
			{
				continue;
			}

			for (int32 ElementIndex = 0; ElementIndex < CurrentEntry->ElementsToDraw.Num(); ++ElementIndex)
			{
				const FString CurrentCategory = CurrentEntry->ElementsToDraw[ElementIndex].Category.ToString();
				FVisualLogShapeElement &CurrentElement = CurrentEntry->ElementsToDraw[ElementIndex];
				if (CurrentElement.Category != NAME_None && (FilterListPtr->IsFilterEnabled(CurrentCategory, CurrentElement.Verbosity)))
				{
					OutEntriesCached[CachedLogIndex].CachedEntries.AddUnique(CurrentEntry);
					bAddedEntry = true;
					break;
				}
			}
			if (bAddedEntry)
			{
				continue;
			}

			for (int32 SampleIndex = 0; SampleIndex < CurrentEntry->HistogramSamples.Num(); ++SampleIndex)
			{
				const FVisualLogHistogramSample &CurrentSample = CurrentEntry->HistogramSamples[SampleIndex];
				const FName CurrentCategory = CurrentSample.Category;
				const FName CurrentDataName = CurrentSample.DataName;

				const bool bCurrentCategoryPassed = FilterListPtr->IsFilterEnabled(CurrentCategory.ToString(), ELogVerbosity::All);
				const bool bCurrentDataNamePassed = FilterListPtr->IsFilterEnabled(CurrentDataName.ToString(), ELogVerbosity::All);

				if (CurrentCategory != NAME_None && (bCurrentCategoryPassed &&	bCurrentDataNamePassed))
				{
					OutEntriesCached[CachedLogIndex].CachedEntries.AddUnique(CurrentEntry);
					bAddedEntry = true;
					break;
				}
			}

			if (bAddedEntry)
			{
				continue;
			}

			if (!bAddedEntry)
			{
				for (int32 LogLineIndex = 0; LogLineIndex < CurrentEntry->Events.Num(); ++LogLineIndex)
				{
					const FVisualLogEvent &CurrentLine = CurrentEntry->Events[LogLineIndex];
					const FString CurrentCategory = CurrentLine.Name;
					if (FilterListPtr->IsFilterEnabled(CurrentCategory, CurrentLine.Verbosity))
					{
						OutEntriesCached[CachedLogIndex].CachedEntries.Add(CurrentEntry);
						bAddedEntry = true;
						break;
					}
				}
			}

			if (bAddedEntry)
			{
				continue;
			}

			for (const auto CurrentBlock : CurrentEntry->DataBlocks)
			{
				const bool bCurrentCategoryPassed = FilterListPtr->IsFilterEnabled(CurrentBlock.Category.ToString(), ELogVerbosity::All);
				const bool bCurrentTagNamePassed = FilterListPtr->IsFilterEnabled(CurrentBlock.TagName.ToString(), ELogVerbosity::All);

				if (CurrentBlock.Category != NAME_None && (bCurrentCategoryPassed &&	bCurrentTagNamePassed))
				{
					OutEntriesCached[CachedLogIndex].CachedEntries.AddUnique(CurrentEntry);
					bAddedEntry = true;
					break;
				}
			}
		}
	}
	else
	{
		// if there is no LogFilter widget - show all
		OutEntriesCached[CachedLogIndex].CachedEntries = Log->Entries;
	}
	if (LogsList.IsValidIndex(CurrentIndex))
	{
		LogsList[CurrentIndex]->LastEndTimestamp = LogsList[CurrentIndex]->EndTimestamp;
	}
}

void SLogVisualizer::GetVisibleEntries(const TSharedPtr<FActorsVisLog>& Log, TArray<TSharedPtr<FVisualLogEntry> >& OutEntries)
{
	const int32 NumLogs = OutEntriesCached.Num();
	int32 CachedLogIndex = INDEX_NONE;
	for (int32 Index = 0; Index < NumLogs; ++Index)
	{
		if (OutEntriesCached[Index].Log == Log)
		{
			CachedLogIndex = Index;
			break;
		}
	}

	if (CachedLogIndex == INDEX_NONE)
	{
		FCachedEntries CachedLog;
		CachedLog.Log = Log;
		CachedLogIndex = OutEntriesCached.Add(CachedLog);
	}

	if (OutEntriesCached[CachedLogIndex].CachedEntries.Num() == 0)
	{
		// generate entries based on current filters
		OutEntriesCached[CachedLogIndex].CachedEntries.Reset();
		if (FilterListPtr.IsValid())
		{
			for (int32 i = 0; i < Log->Entries.Num(); ++i)
			{
				bool bAddedEntry = false;

				const TSharedPtr<FVisualLogEntry> CurrentEntry = Log->Entries[i];
				if (!bAddedEntry)
				{
					for (int32 j = 0; j < CurrentEntry->LogLines.Num(); ++j)
					{
						const FVisualLogLine &CurrentLine = CurrentEntry->LogLines[j];
						const FString CurrentCategory = CurrentLine.Category.ToString();
						if (FilterListPtr->IsFilterEnabled(CurrentCategory, CurrentLine.Verbosity))
						{
							OutEntriesCached[CachedLogIndex].CachedEntries.Add(CurrentEntry);
							bAddedEntry = true;
							break;
						}
					}
				}

				if (bAddedEntry)
				{
					continue;
				}

				for (int32 j = 0; j < CurrentEntry->ElementsToDraw.Num(); ++j)
				{
					const FString CurrentCategory = CurrentEntry->ElementsToDraw[j].Category.ToString();
					FVisualLogShapeElement &CurrentElement = CurrentEntry->ElementsToDraw[j];
					if (CurrentElement.Category != NAME_None && (FilterListPtr->IsFilterEnabled(CurrentCategory, CurrentElement.Verbosity)))
					{
						OutEntriesCached[CachedLogIndex].CachedEntries.AddUnique(CurrentEntry);
						bAddedEntry = true;
						break;
					}
				}
				if (bAddedEntry)
				{
					continue;
				}

				for (int32 SampleIndex = 0; SampleIndex < CurrentEntry->HistogramSamples.Num(); ++SampleIndex)
				{
					const FVisualLogHistogramSample &CurrentSample = CurrentEntry->HistogramSamples[SampleIndex];
					const FName CurrentCategory = CurrentSample.Category;
					const FName CurrentDataName = CurrentSample.DataName;

					const bool bCurrentCategoryPassed = FilterListPtr->IsFilterEnabled(CurrentCategory.ToString(), ELogVerbosity::All);
					const bool bCurrentDataNamePassed = FilterListPtr->IsFilterEnabled(CurrentDataName.ToString(), ELogVerbosity::All);

					if (CurrentCategory != NAME_None && (bCurrentCategoryPassed &&	bCurrentDataNamePassed))
					{
						OutEntriesCached[CachedLogIndex].CachedEntries.AddUnique(CurrentEntry);
						bAddedEntry = true;
						break;
					}
				}
				if (bAddedEntry)
				{
					continue;
				}

				for (int32 j = 0; j < CurrentEntry->Events.Num(); ++j)
				{
					const FString CurrentCategory = CurrentEntry->Events[j].Name;
					FVisualLogEvent &CurrentElement = CurrentEntry->Events[j];
					if (CurrentElement.Name.Len() > 0 && (FilterListPtr->IsFilterEnabled(CurrentCategory, CurrentElement.Verbosity)))
					{
						OutEntriesCached[CachedLogIndex].CachedEntries.AddUnique(CurrentEntry);
						bAddedEntry = true;
						break;
					}
				}
				if (bAddedEntry)
				{
					continue;
				}

				for (const auto CurrentData : CurrentEntry->DataBlocks)
				{
					const bool bCurrentCategoryPassed = FilterListPtr->IsFilterEnabled(CurrentData.Category.ToString(), ELogVerbosity::All);
					const bool bCurrentTagNamePassed = FilterListPtr->IsFilterEnabled(CurrentData.TagName.ToString(), ELogVerbosity::All);

					if (CurrentData.TagName != NAME_None && (bCurrentCategoryPassed &&	bCurrentTagNamePassed))
					{
						OutEntriesCached[CachedLogIndex].CachedEntries.AddUnique(CurrentEntry);
						bAddedEntry = true;
						break;
					}
				}
			}

		}
		else
		{
			// if there is no LogFilter widget - show all
			OutEntriesCached[CachedLogIndex].CachedEntries = Log->Entries;
		}
	}

	OutEntries.Reset();
	if (QuickFilterText.Len() > 0)
	{
		// filter our data using quick filter string
		const int32 NumEntries = OutEntriesCached[CachedLogIndex].CachedEntries.Num();
		for (int32 Index = 0; Index < NumEntries; ++Index)
		{
			TSharedPtr<FVisualLogEntry> LogEntry = OutEntriesCached[CachedLogIndex].CachedEntries[Index];
			if (!LogEntry.IsValid())
			{
				continue;
			}

			bool bAddedEntry = false;
			if (!bAddedEntry)
			{
				for (int32 LineIndex = 0; LineIndex < LogEntry->LogLines.Num(); ++LineIndex)
				{
					FString CurrentCategory = LogEntry->LogLines[LineIndex].Category.ToString();
					if (bHistogramGraphsFilter || CurrentCategory.Find(QuickFilterText) != INDEX_NONE)
					{
						OutEntries.AddUnique(LogEntry);
						bAddedEntry = true;
						break;
					}
				}
			}

			if (bAddedEntry)
			{
				continue;
			}

			for (int32 ElementIndex = 0; ElementIndex < LogEntry->ElementsToDraw.Num(); ++ElementIndex)
			{
				FString CurrentCategory = LogEntry->ElementsToDraw[ElementIndex].Category.ToString();
				if (bHistogramGraphsFilter || CurrentCategory.Find(QuickFilterText) != INDEX_NONE)
				{
					OutEntries.AddUnique(LogEntry);
					bAddedEntry = true;
					break;
				}
			}
			if (bAddedEntry)
			{
				continue;
			}

			for (int32 SampleIndex = 0; SampleIndex < LogEntry->HistogramSamples.Num(); ++SampleIndex)
			{
				const FName CurrentCategory = LogEntry->HistogramSamples[SampleIndex].Category;
				const FName CurrentGraphName = LogEntry->HistogramSamples[SampleIndex].GraphName;
				const FName CurrentDataName = LogEntry->HistogramSamples[SampleIndex].DataName;

				const bool bCurrentCategoryPassed = bHistogramGraphsFilter || (QuickFilterText.Len() == 0 || CurrentCategory.ToString().Find(QuickFilterText) != INDEX_NONE);
				const bool bCurrentDataNamePassed = !bHistogramGraphsFilter || (QuickFilterText.Len() == 0 || CurrentDataName.ToString().Find(QuickFilterText) != INDEX_NONE);

				if (bCurrentCategoryPassed &&	bCurrentDataNamePassed)
				{
					OutEntries.AddUnique(LogEntry);
					bAddedEntry = true;
					break;
				}
			}
			if (bAddedEntry)
			{
				continue;
			}

			for (int32 ElementIndex = 0; ElementIndex < LogEntry->Events.Num(); ++ElementIndex)
			{
				FString CurrentCategory = LogEntry->Events[ElementIndex].Name;
				if (bHistogramGraphsFilter || CurrentCategory.Find(QuickFilterText) != INDEX_NONE)
				{
					OutEntries.AddUnique(LogEntry);
					bAddedEntry = true;
					break;
				}
			}
			if (bAddedEntry)
			{
				continue;
			}


			for (const auto CurrentData : LogEntry->DataBlocks)
			{
				const bool bCurrentCategoryPassed = bHistogramGraphsFilter || (QuickFilterText.Len() == 0 || CurrentData.Category.ToString().Find(QuickFilterText) != INDEX_NONE);
				const bool bCurrentTagNamePassed = !bHistogramGraphsFilter || (QuickFilterText.Len() == 0 || CurrentData.TagName.ToString().Find(QuickFilterText) != INDEX_NONE);

				if (bCurrentCategoryPassed &&	bCurrentTagNamePassed)
				{
					OutEntries.AddUnique(LogEntry);
					bAddedEntry = true;
					break;
				}
			}

		} //for (int32 Index = 0; Index < NumEntries; ++Index)
	} //if (QuickFilterText.Len() > 0)
	else
	{
		OutEntries = OutEntriesCached[CachedLogIndex].CachedEntries;
	}
}

void SLogVisualizer::OnLogCategoryFiltersChanged()
{
	LogsList.Reset();
	OutEntriesCached.Reset();
	RebuildFilteredList();

	if (LogVisualizer && LogVisualizer->Logs.IsValidIndex(SelectedLogIndex))
	{
		TSharedPtr<FActorsVisLog> Log = LogVisualizer->Logs[SelectedLogIndex];
		if (Log.IsValid() && Log->Entries.IsValidIndex(LogEntryIndex))
		{
			ShowEntry(Log->Entries[LogEntryIndex].Get());
		}
	}
	InvalidateCanvas();
}

UWorld* SLogVisualizer::GetWorld() const
{
	// TODO: This needs to be an internalized reference
	UEditorEngine *EEngine = Cast<UEditorEngine>(GEngine);
	if (GIsEditor && EEngine != NULL)
	{
		// lets use PlayWorld during PIE/Simulate and regular world from editor otherwise, to draw debug information
		return EEngine->PlayWorld != NULL ? EEngine->PlayWorld : EEngine->GetEditorWorldContext().World();
		
	}
	else if (!GIsEditor)
	{
		return LogVisualizer->GetWorld();
	}

	return NULL;
}

void SLogVisualizer::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	TimeTillNextUpdate -= InDeltaTime;

	UWorld* World = GetWorld();
	if (World && !World->bPlayersOnly && TimeTillNextUpdate < 0 && LogVisualizer->IsRecording())
	{
		FVisualLogger::Get().Flush();
		DoTickUpdate();
		//DoFullUpdate();
	}

	if (bStickToLastData && World && !World->bPlayersOnly  && LogVisualizer->IsRecording() && World->IsGameWorld() && LogVisualizer && LogVisualizer->Logs.IsValidIndex(SelectedLogIndex))
	{
		TSharedPtr<FActorsVisLog> Log = LogVisualizer->Logs[SelectedLogIndex];
		if (Log->Entries.Num() > 0 && LogEntryIndex != Log->Entries.Num() - 1)
		{
			LogEntryIndex = Log->Entries.Num() - 1;
			ShowEntry(Log->Entries[LogEntryIndex].Get());
		}
	}
}

FReply SLogVisualizer::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (MouseEvent.IsLeftControlDown())
	{
		OnSetZoomValue(FMath::Clamp(ZoomSliderValue + MouseEvent.GetWheelDelta() * 0.05f, 0.f, 1.f));
		return FReply::Handled();
	}

	if (MouseEvent.IsLeftShiftDown())
	{
		OnSetHistogramWindowValue(MouseEvent.GetWheelDelta());
		return FReply::Handled();
	}

	return SCompoundWidget::OnMouseWheel(MyGeometry, MouseEvent);
}

FReply SLogVisualizer::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Left || Key == EKeys::Right)
	{
		int32 MoveBy = Key == EKeys::Left ? -1 : 1;
		if (InKeyEvent.IsLeftControlDown())
		{
			MoveBy *= 10;
		}

		IncrementCurrentLogIndex(MoveBy);

		return FReply::Handled();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

TSharedRef< SWidget > SLogVisualizer::MakeMainMenu()
{
	FMenuBarBuilder MenuBuilder( NULL );
	{
		// File
		MenuBuilder.AddPullDownMenu( 
			NSLOCTEXT("LogVisualizer", "FileMenu", "File"),
			NSLOCTEXT("LogVisualizer", "FileMenu_ToolTip", "Open the file menu"),
			FNewMenuDelegate::CreateSP( this, &SLogVisualizer::OpenSavedSession ) );

		// Help
		MenuBuilder.AddPullDownMenu( 
			NSLOCTEXT("LogVisualizer", "HelpMenu", "Help"),
			NSLOCTEXT("LogVisualizer", "HelpMenu_ToolTip", "Open the help menu"),
			FNewMenuDelegate::CreateSP( this, &SLogVisualizer::FillHelpMenu ) );
	}

	// Create the menu bar
	TSharedRef< SWidget > MenuBarWidget = MenuBuilder.MakeWidget();

	return MenuBarWidget;
}

void SLogVisualizer::FillHelpMenu(FMenuBuilder& MenuBuilder)
{
}

void SLogVisualizer::OpenSavedSession(FMenuBuilder& MenuBuilder)
{
}

//----------------------------------------------------------------------//
// non-slate
//----------------------------------------------------------------------//

void SLogVisualizer::SelectionChanged(AActor* DebuggedActor, bool bIsBeingDebuggedNow)
{
	if (DebuggedActor != NULL && bIsBeingDebuggedNow)
	{
		SelectActor(DebuggedActor);
	}
}

void SLogVisualizer::IncrementCurrentLogIndex(int32 IncrementBy)
{
	if (!LogVisualizer->Logs.IsValidIndex(SelectedLogIndex))
	{
		return;
	}

	TSharedPtr<FActorsVisLog> Log = LogVisualizer->Logs[SelectedLogIndex];
	check(Log.IsValid());

	int32 NewEntryIndex = FMath::Clamp(LogEntryIndex + IncrementBy, 0, Log->Entries.Num() - 1);

	if (FilterListPtr.IsValid())
	{
		while (NewEntryIndex >= 0 && NewEntryIndex < Log->Entries.Num())
		{
			bool bShouldShow = false;
			for (int32 LineIndex = 0; LineIndex < Log->Entries[NewEntryIndex]->LogLines.Num(); ++LineIndex)
			{
				if (FilterListPtr->IsFilterEnabled(Log->Entries[NewEntryIndex]->LogLines[LineIndex].Category.ToString(), Log->Entries[NewEntryIndex]->LogLines[LineIndex].Verbosity))
				{
					bShouldShow = true;
					break;
				}
			}

			if (!bShouldShow)
			{
				for (int32 LineIndex = 0; LineIndex < Log->Entries[NewEntryIndex]->ElementsToDraw.Num(); ++LineIndex)
				{
					if (Log->Entries[NewEntryIndex]->ElementsToDraw[LineIndex].Category == NAME_None || FilterListPtr->IsFilterEnabled(Log->Entries[NewEntryIndex]->ElementsToDraw[LineIndex].Category.ToString(), Log->Entries[NewEntryIndex]->ElementsToDraw[LineIndex].Verbosity))
					{
						bShouldShow = true;
						break;
					}
				}
			}

			if (!bShouldShow)
			{
				for (int32 SampleIndex = 0; SampleIndex < Log->Entries[NewEntryIndex]->HistogramSamples.Num(); ++SampleIndex)
				{
					const FName CurrentCategory = Log->Entries[NewEntryIndex]->HistogramSamples[SampleIndex].Category;
					const FName CurrentGraphName = Log->Entries[NewEntryIndex]->HistogramSamples[SampleIndex].GraphName;
					const FName CurrentDataName = Log->Entries[NewEntryIndex]->HistogramSamples[SampleIndex].DataName;
					if (CurrentCategory == NAME_None ||
						(FilterListPtr->IsFilterEnabled(CurrentCategory.ToString(), ELogVerbosity::All) && FilterListPtr->IsFilterEnabled(CurrentGraphName.ToString(), CurrentDataName.ToString(), ELogVerbosity::All)))
					{
						bShouldShow = true;
						break;
					}
				}
			}

			if (!bShouldShow)
			{
				for (int32 LineIndex = 0; LineIndex < Log->Entries[NewEntryIndex]->Events.Num(); ++LineIndex)
				{
					if (Log->Entries[NewEntryIndex]->Events[LineIndex].Name.Len() == 0 || FilterListPtr->IsFilterEnabled(Log->Entries[NewEntryIndex]->Events[LineIndex].Name, Log->Entries[NewEntryIndex]->Events[LineIndex].Verbosity))
					{
						bShouldShow = true;
						break;
					}
				}
			}

			if (!bShouldShow)
			{
				for (const auto CurrentData : Log->Entries[NewEntryIndex]->DataBlocks)
				{
					const FName CurrentCategory = CurrentData.Category;
					const FName CurrentTagName = CurrentData.TagName;
					if (CurrentCategory == NAME_None ||
						(FilterListPtr->IsFilterEnabled(CurrentCategory.ToString(), ELogVerbosity::All) && FilterListPtr->IsFilterEnabled(CurrentTagName.ToString(), ELogVerbosity::All)))
					{
						bShouldShow = true;
						break;
					}
				}
			}

			if (bShouldShow)
			{
				break;
			}

			NewEntryIndex += (IncrementBy > 0 ? 1 : -1);
		}
	}

	if (NewEntryIndex != LogEntryIndex && Log->Entries.IsValidIndex(NewEntryIndex))
	{
		LogEntryIndex = NewEntryIndex;
		ShowEntry(Log->Entries[NewEntryIndex].Get());
	}
}

void SLogVisualizer::AddOrUpdateLog(int32 LogIndex, const FActorsVisLog* Log)
{
	if (Log->Entries.Num() == 0)
	{
		return;
	}

	if (LogsList.Num() == 0)
	{
		Timeline->SetVisibility(EVisibility::Visible);
		ScrollBar->SetVisibility(EVisibility::Visible);
		ZoomSlider->SetVisibility(EVisibility::Visible);
	}

	const float StartTimestamp = Log->Entries[0]->TimeStamp;
	const float EndTimestamp = Log->Entries[Log->Entries.Num() - 1]->TimeStamp;
	

	int32 CurrentIndex = INDEX_NONE;
	int32 LogListNum = LogsList.Num();
	for (int32 ListIndex = 0; ListIndex < LogListNum; ++ListIndex)
	{
		if (LogsList[ListIndex]->LogIndex == LogIndex)
		{
			CurrentIndex = ListIndex;
			break;
		}
	}

	// add used categories
	for (int32 EntryIndex = 0; EntryIndex < Log->Entries.Num(); ++EntryIndex)
	{
		if (CurrentIndex != INDEX_NONE && Log->Entries[EntryIndex]->TimeStamp <= LogsList[CurrentIndex]->EndTimestamp)
		{
			continue;
		}

		for (auto Iter(Log->Entries[EntryIndex]->LogLines.CreateConstIterator()); Iter; Iter++)
		{
			int32 Index = UsedCategories.Find(Iter->Category.ToString());
			if (Index == INDEX_NONE)
			{
				Index = UsedCategories.Add(Iter->Category.ToString());
				FilterListPtr->AddFilter(Iter->Category.ToString(), GetColorForUsedCategory(Index));
			}
		}

		for (auto Iter(Log->Entries[EntryIndex]->ElementsToDraw.CreateConstIterator()); Iter; Iter++)
		{
			const FString CategoryAsString = Iter->Category != NAME_None ? Iter->Category.ToString() : TEXT("ShapeElement");

			int32 Index = UsedCategories.Find(CategoryAsString);
			if (Index == INDEX_NONE)
			{
				Index = UsedCategories.Add(CategoryAsString);
				FilterListPtr->AddFilter(CategoryAsString, GetColorForUsedCategory(Index));
			}
		}

		for (int32 SampleIndex = 0; SampleIndex < Log->Entries[EntryIndex]->HistogramSamples.Num(); ++SampleIndex)
		{
			const FString CategoryAsString = Log->Entries[EntryIndex]->HistogramSamples[SampleIndex].Category.ToString();

			int32 Index = UsedCategories.Find(CategoryAsString);
			if (Index == INDEX_NONE)
			{
				Index = UsedCategories.Add(CategoryAsString);
				FilterListPtr->AddFilter(CategoryAsString, GetColorForUsedCategory(Index));
			}

			const FString GraphNameAsString = Log->Entries[EntryIndex]->HistogramSamples[SampleIndex].GraphName.ToString();
			const FString DataNameAsString = Log->Entries[EntryIndex]->HistogramSamples[SampleIndex].DataName.ToString();
			FilterListPtr->AddGraphFilter(GraphNameAsString, DataNameAsString, FColor::White);
		}

		for (auto Iter(Log->Entries[EntryIndex]->Events.CreateConstIterator()); Iter; Iter++)
		{
			const FString CategoryAsString = Iter->Name.Len() != 0 ? Iter->Name : TEXT("Event");

			int32 Index = UsedCategories.Find(CategoryAsString);
			if (Index == INDEX_NONE)
			{
				Index = UsedCategories.Add(CategoryAsString);
				FilterListPtr->AddFilter(CategoryAsString, GetColorForUsedCategory(Index));
			}
		}

		for (const auto CurrentData : Log->Entries[EntryIndex]->DataBlocks)
		{
			int32 Index = UsedCategories.Find(CurrentData.Category.ToString());
			if (Index == INDEX_NONE)
			{
				Index = UsedCategories.Add(CurrentData.Category.ToString());
				FilterListPtr->AddFilter(CurrentData.Category.ToString(), GetColorForUsedCategory(Index));
			}
		}


	}

	if (CurrentIndex == INDEX_NONE)
	{
		LogsList.Add(MakeShareable(new FLogsListItem(Log->Name.ToString(), StartTimestamp, EndTimestamp, LogIndex)));
	}
	else
	{
		LogsList[CurrentIndex]->StartTimestamp = StartTimestamp;
		LogsList[CurrentIndex]->LastEndTimestamp = LogsList[CurrentIndex]->EndTimestamp;
		LogsList[CurrentIndex]->EndTimestamp= EndTimestamp;
	}
}

void SLogVisualizer::DoFullUpdate()
{
	TSharedPtr<FLogsListItem>* LogListItem = LogsList.GetData();
	for (int32 ItemIndex = 0; ItemIndex < LogsList.Num(); ++ItemIndex, ++LogListItem)
	{
		if (LogListItem->IsValid() && LogVisualizer->Logs.IsValidIndex((*LogListItem)->LogIndex))
		{
			TSharedPtr<FActorsVisLog>& Log = LogVisualizer->Logs[(*LogListItem)->LogIndex];
			if (Log->Entries.Num() > 0)
			{
				LogsStartTime = FMath::Min(Log->Entries[0]->TimeStamp, LogsStartTime);
				LogsEndTime = FMath::Max(Log->Entries[Log->Entries.Num() - 1]->TimeStamp, LogsEndTime);
			}
		}
	}

	Timeline->SetMinMaxValues(LogsStartTime, LogsEndTime);
	// set zoom max so that single even on SBarLogs has desired size on maximum zoom
	const float WidthPx = Timeline->GetDrawingGeometry().Size.X;
	if (WidthPx > 0)
	{
		const float OldMaxZoom = MaxZoom;
		const float PxPerTimeUnit = WidthPx * SLogBar::TimeUnit / (LogsEndTime - LogsStartTime);
		MaxZoom = SLogBar::MaxUnitSizePx / PxPerTimeUnit;
		if (MaxZoom < MinZoom)
		{
			MaxZoom = MinZoom;
		}

		ZoomSliderValue = MaxZoom * ZoomSliderValue / OldMaxZoom;
		// update 
	}

	LogsList.Reset();
	OutEntriesCached.Reset();
	RebuildFilteredList();

	TimeTillNextUpdate = 1.f / FullUpdateFrequency;
	InvalidateCanvas();
}

void SLogVisualizer::DoTickUpdate()
{
	TSharedPtr<FLogsListItem>* LogListItem = LogsList.GetData();
	for (int32 ItemIndex = 0; ItemIndex < LogsList.Num(); ++ItemIndex, ++LogListItem)
	{
		if (LogListItem->IsValid() && LogVisualizer->Logs.IsValidIndex((*LogListItem)->LogIndex))
		{
			TSharedPtr<FActorsVisLog>& Log = LogVisualizer->Logs[(*LogListItem)->LogIndex];
			if (Log->Entries.Num() > 0)
			{
				LogsStartTime = FMath::Min(Log->Entries[0]->TimeStamp, LogsStartTime);
				LogsEndTime = FMath::Max(Log->Entries[Log->Entries.Num() - 1]->TimeStamp, LogsEndTime);
			}
		}
	}

	Timeline->SetMinMaxValues(LogsStartTime, LogsEndTime);
	// set zoom max so that single even on SBarLogs has desired size on maximum zoom
	const float WidthPx = Timeline->GetDrawingGeometry().Size.X;
	if (WidthPx > 0)
	{
		const float OldMaxZoom = MaxZoom;
		const float PxPerTimeUnit = WidthPx * SLogBar::TimeUnit / (LogsEndTime - LogsStartTime);
		MaxZoom = SLogBar::MaxUnitSizePx / PxPerTimeUnit;
		if (MaxZoom < MinZoom)
		{
			MaxZoom = MinZoom;
		}

		ZoomSliderValue = MaxZoom * ZoomSliderValue / OldMaxZoom;
		// update 
	}

	RebuildFilteredList();

	TimeTillNextUpdate = 1.f / FullUpdateFrequency;
	InvalidateCanvas();
}

void SLogVisualizer::OnLogAdded()
{
	// take last log
	const int32 NewLogIndex = LogVisualizer->Logs.Num()-1;
	
	TSharedPtr<FLogsListItem> Item;
	for (int32 Index = 0; Index < LogsList.Num(); ++Index)
	{
		Item = LogsList[Index];
		TArray<TSharedPtr<FActorsVisLog> >& Logs = LogVisualizer->Logs;
		if (Item->Name == Logs[NewLogIndex]->Name.ToString())
		{
			break;
		}
	}
	
	if (!Item.IsValid())
	{
		AddOrUpdateLog(NewLogIndex, LogVisualizer->Logs[NewLogIndex].Get());
	}		

	RequestFullUpdate();
}

TSharedRef<ITableRow> SLogVisualizer::LogsListGenerateRow(TSharedPtr<FLogsListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SLogsTableRow, OwnerTable)
			.Item(InItem)
			.OwnerVisualizerWidget(SharedThis(this));
}

void SLogVisualizer::LogsListSelectionChanged(TSharedPtr<FLogsListItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
	//@todo find log entry closest to current time selection
	//LogEntryIndex = INDEX_NONE;
	const int32 NewLogIndex = SelectedItem.IsValid() ? SelectedItem->LogIndex : INDEX_NONE;
	if (NewLogIndex != SelectedLogIndex && NewLogIndex != INDEX_NONE)
	{
		SelectedLogIndex = NewLogIndex;
		
		if (LogVisualizer->Logs.IsValidIndex(NewLogIndex))
		{
			TSharedPtr<FActorsVisLog> Log = LogVisualizer->Logs[NewLogIndex];
			LogEntryIndex = Log->Entries.Num() - 1;
		}
	}
	
	if (LogVisualizer->Logs.IsValidIndex(SelectedLogIndex))
	{
		if (USelection* SelectedActors = GEditor->GetSelectedActors())
		{
			TSharedPtr<FActorsVisLog> Log = LogVisualizer->Logs[SelectedLogIndex];

			if (UWorld* World = GetWorld())
			{
				for (FConstPawnIterator Iterator = World->GetPawnIterator(); Iterator; ++Iterator)
				{
					if (APawn* CurrentPawn = *Iterator)
					{
						if (AController* CurrentController = CurrentPawn->GetController())
						{
							if (CurrentController->GetName() == Log->Name.ToString())
							{
								SelectedActors->Select(CurrentPawn);
							}
							else
							{
								SelectedActors->Deselect(CurrentPawn);
							}
						}
					}
				}
			}
		}
	}

	//SetCurrentViewedTime(CurrentViewedTime, /*bForce=*/true);
	
	LogsLinesWidget->RequestListRefresh();
	InvalidateCanvas();
}

TSharedRef<ITableRow> SLogVisualizer::LogEntryLinesGenerateRow(TSharedPtr<FLogEntryItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew( STableRow< TSharedPtr<FString> >, OwnerTable )
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5.0f, 0.0f))
			[
				SNew(STextBlock) 
				.ColorAndOpacity(FSlateColor(Item->CategoryColor))
				.Text(Item->Category)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5.0f, 0.0f))
			[
				SNew(STextBlock) 				
				.ColorAndOpacity(FSlateColor(FLinearColor::Gray))
				.Text(FString(TEXT("(")) + FString(FOutputDevice::VerbosityToString(Item->Verbosity)) + FString(TEXT(")")))
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5.0f, 0.0f))
			[
				SNew(STextBlock) 
				.Text(Item->Line)
			]			
		];
}

void SLogVisualizer::LogEntryLineSelectionChanged(TSharedPtr<FLogEntryItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
	TMap<FName, FVisualLogExtensionInterface*>& AllExtensions = FVisualLogger::Get().GetAllExtensions();
	for (auto Iterator = AllExtensions.CreateIterator(); Iterator; ++Iterator)
	{
		FVisualLogExtensionInterface* Extension = (*Iterator).Value;
		if (Extension != NULL)
		{
			if (SelectedItem.IsValid() == true)
			{
				Extension->LogEntryLineSelectionChanged(SelectedItem, SelectedItem->UserData, SelectedItem->TagName);
			}
			else
			{
				Extension->LogEntryLineSelectionChanged(SelectedItem, 0, NAME_None);
			}
		}
	}
}

bool SLogVisualizer::ShouldListLog(const TSharedPtr<FActorsVisLog>& Log)
{
	//// Check log name filter
	if (!Log.IsValid() || (LogNameFilterString.Len() > 0 && !Log->Name.ToString().Contains(LogNameFilterString)))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return !bIgnoreTrivialLogs || Log->Entries.Num() >= 2;
	}
	else
	{
		static TArray<TSharedPtr<FVisualLogEntry> > OutEntries;
		GetVisibleEntries(Log, OutEntries);
		return !bIgnoreTrivialLogs || OutEntries.Num() >= 2;
	}

	return true;
}

void SLogVisualizer::UpdateFilterInfo()
{
	// get filters
	LogNameFilterString = LogNameFilterBox->GetText().ToString();
}

void SLogVisualizer::SetCurrentViewedTime(float NewTime, const bool bForce)
{
	if (CurrentViewedTime == NewTime && bForce == false)
	{
		return;
	}

	CurrentViewedTime = NewTime;
	InvalidateCanvas();
}

void SLogVisualizer::InvalidateCanvas()
{
#if WITH_EDITOR
	UEditorEngine *EEngine = Cast<UEditorEngine>(GEngine);
	if (GIsEditor && EEngine != NULL)
	{
		for (int32 Index = 0; Index < EEngine->AllViewportClients.Num(); Index++)
		{
			FEditorViewportClient* ViewportClient = EEngine->AllViewportClients[Index];
			if (ViewportClient)
			{
				ViewportClient->Invalidate();
			}
		}
	}
#endif
}

void SLogVisualizer::RequestShowLogEntry(TSharedPtr<FLogsListItem> Item, TSharedPtr<FVisualLogEntry> LogEntry)
{
	ShowLogEntry(Item, LogEntry);
}

void SLogVisualizer::ShowLogEntry(TSharedPtr<FLogsListItem> Item, TSharedPtr<FVisualLogEntry> LogEntry)
{
	if(LogsListWidget->GetSelectedItems().Find(Item) == INDEX_NONE)
	{
		LogsListWidget->ClearSelection();
		LogsListWidget->SetItemSelection(Item, true);
	}

	if (LogVisualizer->Logs.IsValidIndex(SelectedLogIndex))
	{
		TSharedPtr<FActorsVisLog> Log = LogVisualizer->Logs[SelectedLogIndex];
		LogEntryIndex = Log->Entries.Find(LogEntry);
	}
	else
	{
		LogEntryIndex = INDEX_NONE;
	}

	ShowEntry(LogEntry.Get());
}

FLinearColor SLogVisualizer::GetColorForUsedCategory(int32 Index)
{
	if (Index >= 0 && Index < sizeof(ColorPalette) / sizeof(ColorPalette[0]))
	{
		return ColorPalette[Index];
	}

	static bool bReateColorList = false;
	static FColorList StaticColor;
	if (!bReateColorList)
	{
		bReateColorList = true;
		StaticColor.CreateColorMap();
	}
	return StaticColor.GetFColorByIndex(Index);
}

TSharedRef<ITableRow> SLogVisualizer::HandleGenerateLogStatus(TSharedPtr<FLogStatusItem> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	if (InItem->Children.Num() > 0)
	{
		return SNew(STableRow<TSharedPtr<FLogStatusItem> >, OwnerTable)
			[
				SNew(STextBlock).Text(InItem->ItemText)
			];
	}

	FString TooltipText = FString::Printf(TEXT("%s: %s"), *InItem->ItemText, *InItem->ValueText);
	return SNew(STableRow<TSharedPtr<FLogStatusItem> >, OwnerTable)
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("NoBorder") )
			.ToolTipText(TooltipText)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock).Text(InItem->ItemText).ColorAndOpacity(FColorList::Aquamarine)
				]
				+SHorizontalBox::Slot()
				.Padding(4.0f, 0, 0, 0)
				.AutoWidth()
				[
					SNew(STextBlock).Text(InItem->ValueText)
				]
			]
		];
}

void SLogVisualizer::OnLogStatusGetChildren(TSharedPtr<FLogStatusItem> InItem, TArray< TSharedPtr<FLogStatusItem> >& OutItems)
{
	OutItems = InItem->Children;
}

void SLogVisualizer::UpdateStatusItems(const FVisualLogEntry* LogEntry)
{
	TArray<FString> ExpandedCategories;
	for (int32 ItemIndex = 0; ItemIndex < StatusItems.Num(); ItemIndex++)
	{
		const bool bIsExpanded = StatusItemsView->IsItemExpanded(StatusItems[ItemIndex]);
		if (bIsExpanded)
		{
			ExpandedCategories.Add(StatusItems[ItemIndex]->ItemText);
		}
	}

	StatusItems.Empty();

	if (LogEntry)
	{
		FString TimestampDesc = FString::Printf(TEXT("%.2fs"), LogEntry->TimeStamp);
		StatusItems.Add(MakeShareable(new FLogStatusItem(LOCTEXT("VisLogTimestamp","Time").ToString(), TimestampDesc)));

		for (int32 CategoryIndex = 0; CategoryIndex < LogEntry->Status.Num(); CategoryIndex++)
		{
			if (LogEntry->Status[CategoryIndex].Data.Num() <= 0)
			{
				continue;
			}

			TSharedRef<FLogStatusItem> StatusItem = MakeShareable(new FLogStatusItem(LogEntry->Status[CategoryIndex].Category));
			for (int32 LineIndex = 0; LineIndex < LogEntry->Status[CategoryIndex].Data.Num(); LineIndex++)
			{
				FString KeyDesc, ValueDesc;
				const bool bHasValue = LogEntry->Status[CategoryIndex].GetDesc(LineIndex, KeyDesc, ValueDesc);
				if (bHasValue)
				{
					StatusItem->Children.Add(MakeShareable(new FLogStatusItem(KeyDesc, ValueDesc)));
				}
			}

			StatusItems.Add(StatusItem);
		}
	}

	StatusItemsView->RequestTreeRefresh();

	for (int32 ItemIndex = 0; ItemIndex < StatusItems.Num(); ItemIndex++)
	{
		for (const FString& Category : ExpandedCategories)
		{
			if (StatusItems[ItemIndex]->ItemText == Category)
			{
				StatusItemsView->SetItemExpansion(StatusItems[ItemIndex], true);
				break;
			}
		}
	}
}

void SLogVisualizer::ShowEntry(const FVisualLogEntry* LogEntry)
{
	UpdateStatusItems(LogEntry);
	LogEntryLines.Reset();
	
	const FVisualLogLine* LogLine = LogEntry->LogLines.GetData();
	for (int LineIndex = 0; LineIndex < LogEntry->LogLines.Num(); ++LineIndex, ++LogLine)
	{
		bool bShowLine = true;

		if (FilterListPtr.IsValid())
		{
			FString CurrentCategory = LogLine->Category.ToString();
			bShowLine = FilterListPtr->IsFilterEnabled(CurrentCategory, LogLine->Verbosity) && (bHistogramGraphsFilter || (QuickFilterText.Len() == 0 || CurrentCategory.Find(QuickFilterText) != INDEX_NONE));
			
		}

		if (bShowLine)
		{
			FLogEntryItem EntryItem;
			EntryItem.Category = LogLine->Category.ToString();

			int32 Index = UsedCategories.Find(EntryItem.Category);
			if (Index == INDEX_NONE)
			{
				Index = UsedCategories.Add(EntryItem.Category);
			}
			EntryItem.CategoryColor = GetColorForUsedCategory(Index);

			EntryItem.Verbosity = LogLine->Verbosity;
			EntryItem.Line = LogLine->Line;
			EntryItem.UserData = LogLine->UserData;
			EntryItem.TagName = LogLine->TagName;

			LogEntryLines.Add(MakeShareable(new FLogEntryItem(EntryItem)));
		}
	}

	for (auto& Event : LogEntry->Events)
	{
		bool bShowLine = true;

		if (FilterListPtr.IsValid())
		{
			FString CurrentCategory = Event.Name;
			bShowLine = FilterListPtr->IsFilterEnabled(CurrentCategory, Event.Verbosity) && (bHistogramGraphsFilter || (QuickFilterText.Len() == 0 || CurrentCategory.Find(QuickFilterText) != INDEX_NONE));
		}

		if (bShowLine)
		{
			FLogEntryItem EntryItem;
			EntryItem.Category = Event.Name;

			int32 Index = UsedCategories.Find(EntryItem.Category);
			if (Index == INDEX_NONE)
			{
				Index = UsedCategories.Add(EntryItem.Category);
			}
			EntryItem.CategoryColor = GetColorForUsedCategory(Index);

			EntryItem.Verbosity = Event.Verbosity;
			EntryItem.Line = FString::Printf(TEXT("Registered event: '%s' (%d times)%s"), *Event.Name, Event.Counter, Event.EventTags.Num() ? TEXT("\n") : TEXT(""));
			for (auto& EventTag : Event.EventTags)
			{
				EntryItem.Line += FString::Printf(TEXT("%d times for tag: '%s'\n"), EventTag.Value, *EventTag.Key.ToString());
			}
			EntryItem.UserData = Event.UserData;
			EntryItem.TagName = Event.TagName;

			LogEntryLines.Add(MakeShareable(new FLogEntryItem(EntryItem)));
		}

	}

	SetCurrentViewedTime(LogEntry->TimeStamp);

	UWorld* World = GetWorld();
	for (auto It = FVisualLogger::Get().GetAllExtensions().CreateIterator(); It; ++It)
	{
		(*It).Value->OnTimestampChange(LogEntry->TimeStamp, World, FLogVisualizerModule::Get()->GetHelperActor(World));
	}
	InvalidateCanvas();
	LogsLinesWidget->RequestListRefresh();
}

int32 SLogVisualizer::FindIndexInLogsList(const int32 LogIndex) const
{
	for (int32 Index = 0; Index < LogsList.Num(); ++Index)
	{
		if (LogsList[Index]->LogIndex == LogIndex)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void SLogVisualizer::RebuildFilteredList()
{
	// store current selection
	TArray< TSharedPtr<FLogsListItem> > ItemsToSelect = LogsListWidget->GetSelectedItems();

	for (int32 LogIndex = 0; LogIndex < LogVisualizer->Logs.Num(); ++LogIndex)
	{
		TSharedPtr<FActorsVisLog> Log = LogVisualizer->Logs[LogIndex];
		if (ShouldListLog(Log))
		{
			// Passed filter so add to filtered results (defer sorting until end)
			AddOrUpdateLog(LogIndex, Log.Get());
			UpdateVisibleEntriesCache(Log, LogIndex);
		}
	}

	// When underlying array changes, refresh list
	LogsListWidget->RequestListRefresh();
	LogsListWidget->RefreshList();

	// redo selection
	if (ItemsToSelect.Num() > 0)
	{
		TSharedPtr<FLogsListItem>* Item = ItemsToSelect.GetData();
		for (int32 ItemIndex = 0; ItemIndex < ItemsToSelect.Num(); ++ItemIndex, ++Item)
		{
			const int32 IndexInList = FindIndexInLogsList((*Item)->LogIndex);
			if (IndexInList != INDEX_NONE)
			{
				LogsListWidget->SetItemSelection(LogsList[IndexInList], true);
			}
		}
	}
}

float SLogVisualizer::GetZoomValue() const
{
	return ZoomSliderValue;
}

void SLogVisualizer::OnSetZoomValue( float NewValue )
{
	const float PrevZoom = GetZoom();
	const float PrevVisibleRange = 1.0f / PrevZoom;

	ZoomSliderValue = NewValue;
	const float Zoom = GetZoom();

	const float MaxOffset = GetMaxScrollOffsetFraction();
	const float MaxGraphOffset = GetMaxGraphOffset();
	
	const float ViewedTimeSpan = (LogsEndTime - LogsStartTime) / Zoom;
	const float ScrollOffsetFraction = FMath::Clamp((CurrentViewedTime - LogsStartTime - ViewedTimeSpan/2) / (LogsEndTime - LogsStartTime), 0.0f, MaxOffset);

	const float WidthPx = Timeline->GetDrawingGeometry().Size.X;
	const float GraphOffset = MaxOffset > 0 ? (ScrollOffsetFraction / MaxOffset) * MaxGraphOffset : 0.f;
	
	ZoomChangedNotify.Broadcast(Zoom, -GraphOffset);

	ScrollBar->SetState( ScrollOffsetFraction, 1.0f / Zoom );

	Timeline->SetZoom( Zoom );
	Timeline->SetOffset( -GraphOffset );

	ScrollbarOffset = -GraphOffset;
	InvalidateCanvas();
}

void SLogVisualizer::OnZoomScrolled(float InScrollOffsetFraction)
{
	if( ZoomSliderValue > 0.0f )
	{
		const float MaxOffset = GetMaxScrollOffsetFraction();
		const float MaxGraphOffset = GetMaxGraphOffset();
		InScrollOffsetFraction = FMath::Clamp( InScrollOffsetFraction, 0.0f, MaxOffset );
		float GraphOffset = -( InScrollOffsetFraction / MaxOffset ) * MaxGraphOffset;

		ScrollBar->SetState( InScrollOffsetFraction, 1.0f / GetZoom() );

		ZoomChangedNotify.Broadcast(GetZoom(), GraphOffset);

		Timeline->SetOffset( GraphOffset );

		ScrollbarOffset = GraphOffset;
		InvalidateCanvas();
	}
}

void SLogVisualizer::OnSetHistogramWindowValue(float NewValue)
{
	HistogramPreviewWindow = FMath::Clamp(HistogramPreviewWindow + NewValue * 1.0f, 0.0f, 100.0f);
	HistogramWindowChangedNotify.Broadcast(HistogramPreviewWindow);
	InvalidateCanvas();
}

void SLogVisualizer::OnDrawLogEntriesPathChanged(ESlateCheckBoxState::Type NewState)
{
	bDrawLogEntriesPath = (NewState == ESlateCheckBoxState::Checked);
	InvalidateCanvas();
}

ESlateCheckBoxState::Type SLogVisualizer::GetDrawLogEntriesPathState() const
{
	return bDrawLogEntriesPath ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SLogVisualizer::OnIgnoreTrivialLogs(ESlateCheckBoxState::Type NewState)
{
	bIgnoreTrivialLogs = (NewState == ESlateCheckBoxState::Checked);
	DoFullUpdate();
}

ESlateCheckBoxState::Type SLogVisualizer::GetIgnoreTrivialLogs() const
{
	return bIgnoreTrivialLogs ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SLogVisualizer::OnChangeHistogramLabelLocation(ESlateCheckBoxState::Type NewState)
{
	bShowHistogramLabelsOutside = (NewState == ESlateCheckBoxState::Checked);
	InvalidateCanvas();
}

ESlateCheckBoxState::Type SLogVisualizer::GetHistogramLabelLocation() const
{
	return bShowHistogramLabelsOutside ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SLogVisualizer::OnStickToLastData(ESlateCheckBoxState::Type NewState)
{
	bStickToLastData = (NewState == ESlateCheckBoxState::Checked);
	InvalidateCanvas();
}

ESlateCheckBoxState::Type SLogVisualizer::GetStickToLastData() const
{
	return bStickToLastData ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SLogVisualizer::OnToggleCamera(ESlateCheckBoxState::Type NewState)
{
	UWorld* World = GetWorld();
	if (ALogVisualizerCameraController::IsEnabled(World))
	{
		ALogVisualizerCameraController::DisableCamera(World);
	}
	else
	{
		ALogVisualizerCameraController::EnableCamera(World);
	}
	InvalidateCanvas();
}

ESlateCheckBoxState::Type SLogVisualizer::GetToggleCameraState() const
{
	return ALogVisualizerCameraController::IsEnabled(GetWorld())
		? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SLogVisualizer::OnOffsetDataSets(ESlateCheckBoxState::Type NewState)
{
	bOffsetDataSet = (NewState == ESlateCheckBoxState::Checked);
	InvalidateCanvas();
}

ESlateCheckBoxState::Type SLogVisualizer::GetOffsetDataSets() const
{
	return bOffsetDataSet ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SLogVisualizer::OnHistogramGraphsFilter(ESlateCheckBoxState::Type NewState)
{
	bHistogramGraphsFilter = (NewState == ESlateCheckBoxState::Checked);
	QuickFilterText.Empty();
	QuickFilterBox->SetText(FText::FromString(QuickFilterText));

	LogsList.Reset();
	OutEntriesCached.Reset();
	RebuildFilteredList();

	if (LogVisualizer && LogVisualizer->Logs.IsValidIndex(SelectedLogIndex))
	{
		TSharedPtr<FActorsVisLog> Log = LogVisualizer->Logs[SelectedLogIndex];
		if (Log.IsValid() && Log->Entries.IsValidIndex(LogEntryIndex))
		{
			ShowEntry(Log->Entries[LogEntryIndex].Get());
		}
	}
}

ESlateCheckBoxState::Type SLogVisualizer::GetHistogramGraphsFilter() const
{
	return bHistogramGraphsFilter ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

//----------------------------------------------------------------------//
// Drawing
//----------------------------------------------------------------------//
void SLogVisualizer::DrawOnCanvas(UCanvas* Canvas, APlayerController*)
{
	UWorld* World = GetWorld();
	if (World != NULL && LogVisualizer->Logs.IsValidIndex(SelectedLogIndex))
	{
		TSharedPtr<FActorsVisLog> Log = LogVisualizer->Logs[SelectedLogIndex];
		const TArray<TSharedPtr<FVisualLogEntry> >& Entries = Log->Entries;

		if (bDrawLogEntriesPath)
		{
			const TSharedPtr<FVisualLogEntry>* Entry = Entries.GetData();
			FVector Location = (*Entry)->Location;
			++Entry;

			for (int32 Index = 1; Index < Entries.Num(); ++Index, ++Entry)
			{
				const FVector CurrentLocation = (*Entry)->Location;
				DrawDebugLine(World, Location, CurrentLocation, FColor(160, 160, 240));
				Location = CurrentLocation;
			}
		}

		if (Entries.IsValidIndex(LogEntryIndex))
		{
			// draw all additional data stored in current entry
			const TSharedPtr<FVisualLogEntry>& Entry = Entries[LogEntryIndex];

			// mark current location
			DrawDebugCone(World, Entry->Location, /*Direction*/FVector(0, 0, 1), /*Length*/200.f
				, PI/64, PI/64, /*NumSides*/16, FColor::Red);
			
			UFont* Font = GEngine->GetSmallFont();
			FCanvasTextItem TextItem( FVector2D::ZeroVector, FText::GetEmpty(), Font, FLinearColor::White );
			
			const FString TimeStampString = FString::Printf(TEXT("%.2f"), Entry->TimeStamp);
			LogVisualizer::DrawTextShadowed(Canvas, Font, TimeStampString, Entry->Location);
			
			//let's draw histogram data

			struct FGraphLineData
			{
				FName DataName;
				TArray<FVector2D> Samples;
			};
			typedef TMap<FName, FGraphLineData > FGraphLines;

			struct FGraphData
			{
				FGraphData() : Min(FVector2D(FLT_MAX, FLT_MAX)), Max(FVector2D(FLT_MIN, FLT_MIN)) {}

				FVector2D Min, Max;
				TMap<FName, FGraphLineData> GraphLines;
			};

			TMap<FName, FGraphData>	CollectedGraphs;

			float MinTime, MaxTime;
			Timeline->GetMinMaxValues(MinTime, MaxTime);

			const float StartTime = Timeline->GetOffset() + MinTime;
			const float EndTime = StartTime + (MaxTime - MinTime) / this->GetZoom();
			const float WindowHalfWidth = (EndTime - StartTime) * HistogramPreviewWindow * 0.01 * 0.5;
			const FVector2D TimeStampWindow(Entry->TimeStamp - WindowHalfWidth, Entry->TimeStamp + WindowHalfWidth);

			int32 ColorIndex = 0;
			for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); ++EntryIndex)
			{
				const TSharedPtr<FVisualLogEntry>& CurrentEntry = Entries[EntryIndex];
				if (HistogramPreviewWindow <= 0)
				{
					if (CurrentEntry->TimeStamp > Entry->TimeStamp)
					{
						break;
					}
				}
				else
				{
					if (CurrentEntry->TimeStamp < TimeStampWindow.X)
					{
						continue;
					}

					if (CurrentEntry->TimeStamp > TimeStampWindow.Y)
					{
						break;
					}
				}
				const int32 SamplesNum = CurrentEntry->HistogramSamples.Num();
				for (int32 SampleIndex = 0; SampleIndex < SamplesNum; ++SampleIndex)
				{
					FVisualLogHistogramSample CurrentSample = CurrentEntry->HistogramSamples[SampleIndex];

					const FName CurrentCategory = CurrentSample.Category;
					const FName CurrentGraphName = CurrentSample.GraphName;
					const FName CurrentDataName = CurrentSample.DataName;

					const bool bIsValidByFilter = FilterListPtr->IsFilterEnabled(CurrentSample.GraphName.ToString(), CurrentSample.DataName.ToString(), ELogVerbosity::All);
					const bool bCurrentDataNamePassed = !bHistogramGraphsFilter || (QuickFilterText.Len() == 0 || CurrentDataName.ToString().Find(QuickFilterText) != INDEX_NONE);


					if (!FilterListPtr.IsValid() || (bIsValidByFilter /*&& bCurrentCategoryPassed*/ && bCurrentDataNamePassed))
					{
						FGraphData &GraphData = CollectedGraphs.FindOrAdd(CurrentSample.GraphName);
						FGraphLineData &LineData = GraphData.GraphLines.FindOrAdd(CurrentSample.DataName);
						LineData.DataName = CurrentSample.DataName;
						LineData.Samples.Add( CurrentSample.SampleValue );

						GraphData.Min.X = FMath::Min(GraphData.Min.X, CurrentSample.SampleValue.X);
						GraphData.Min.Y = FMath::Min(GraphData.Min.Y, CurrentSample.SampleValue.Y);

						GraphData.Max.X = FMath::Max(GraphData.Max.X, CurrentSample.SampleValue.X);
						GraphData.Max.Y = FMath::Max(GraphData.Max.Y, CurrentSample.SampleValue.Y);
					}
				}
			}

			const float GoldenRatioConjugate = 0.618033988749895f;
			int32 GraphIndex = 0;
			if (CollectedGraphs.Num() > 0)
			{
				const int NumberOfGraphs = CollectedGraphs.Num();
				const int32 NumberOfColumns = FMath::CeilToInt(FMath::Sqrt(NumberOfGraphs));
				int32 NumberOfRows = FMath::FloorToInt(NumberOfGraphs / NumberOfColumns);
				if (NumberOfGraphs - NumberOfRows * NumberOfColumns > 0)
				{
					NumberOfRows += 1;
				}

				const int32 MaxNumberOfGraphs = FMath::Max(NumberOfRows, NumberOfColumns);
				const float GraphWidth = 0.8f / NumberOfColumns;
				const float GraphHeight = 0.8f / NumberOfRows;

				const float XGraphSpacing = 0.2f / (MaxNumberOfGraphs + 1);
				const float YGraphSpacing = 0.2f / (MaxNumberOfGraphs + 1);

				const float StartX = XGraphSpacing;
				float StartY = 0.5 + (0.5 * NumberOfRows - 1) * (GraphHeight + YGraphSpacing);

				float CurrentX = StartX;
				float CurrentY = StartY;
				int32 GraphIndex = 0;
				int32 CurrentColumn = 0;
				int32 CurrentRow = 0;
				for (auto It(CollectedGraphs.CreateIterator()); It; ++It)
				{
					TWeakObjectPtr<UReporterGraph> HistogramGraph = Canvas->GetReporterGraph();
					if (!HistogramGraph.IsValid())
					{
						break;
					}
					HistogramGraph->SetNumGraphLines(It->Value.GraphLines.Num());
					int32 LineIndex = 0;
					UFont* Font = GEngine->GetSmallFont();
					int32 MaxStringSize = 0;
					float Hue = 0;// StartGoldenRatio[GraphIndex++];

					auto& CategoriesForGraph = UsedGraphCategories.FindOrAdd(It->Key.ToString());

					It->Value.GraphLines.KeySort(TLess<FName>());
					for (auto LinesIt(It->Value.GraphLines.CreateConstIterator()); LinesIt; ++LinesIt)
					{
						const FString DataName = LinesIt->Value.DataName.ToString();
						int32 CategoryIndex = CategoriesForGraph.Find(DataName);
						if (CategoryIndex == INDEX_NONE)
						{
							CategoryIndex = CategoriesForGraph.AddUnique(DataName);
						}
						Hue = CategoryIndex * GoldenRatioConjugate;
						if (Hue > 1)
						{
							Hue -= FMath::FloorToFloat(Hue);
						}

						HistogramGraph->GetGraphLine(LineIndex)->Color = FLinearColor::FGetHSV(Hue * 255, 0, 244);
						HistogramGraph->GetGraphLine(LineIndex)->LineName = DataName;
						HistogramGraph->GetGraphLine(LineIndex)->Data.Append(LinesIt->Value.Samples);

						int32 DummyY, CurrentX;
						StringSize(Font, CurrentX, DummyY, *LinesIt->Value.DataName.ToString());
						MaxStringSize = CurrentX > MaxStringSize ? CurrentX : MaxStringSize;

						++LineIndex;
					}

					FVector2D GraphSpaceSize;
					GraphSpaceSize.Y = GraphSpaceSize.X = 0.8f / CollectedGraphs.Num();

					HistogramGraph->SetGraphScreenSize(CurrentX, CurrentX + GraphWidth, CurrentY, CurrentY + GraphHeight);
					CurrentX += GraphWidth + XGraphSpacing;
					HistogramGraph->SetAxesMinMax(FVector2D(TimeStampWindow.X, It->Value.Min.Y), FVector2D(TimeStampWindow.Y, It->Value.Max.Y));
					
					HistogramGraph->DrawCursorOnGraph(true);
					HistogramGraph->UseTinyFont(CollectedGraphs.Num() >= 5);
					HistogramGraph->SetCursorLocation(Entry->TimeStamp);
					HistogramGraph->SetNumThresholds(0);
					HistogramGraph->SetStyles(EGraphAxisStyle::Grid, EGraphDataStyle::Lines);
					HistogramGraph->SetBackgroundColor( FColor(0,0,0, 200) );
					HistogramGraph->SetLegendPosition(bShowHistogramLabelsOutside ? ELegendPosition::Outside : ELegendPosition::Inside);
					HistogramGraph->OffsetDataSets(bOffsetDataSet);
					HistogramGraph->bVisible = true;
					HistogramGraph->Draw(Canvas);

					++GraphIndex;

					if (++CurrentColumn >= NumberOfColumns)
					{
						CurrentColumn = 0;
						CurrentRow++;

						CurrentX = StartX;
						CurrentY -= GraphHeight + YGraphSpacing;
					}
				}
			}

			AActor* HelperActor = FLogVisualizerModule::Get()->GetHelperActor(World);
			for (const auto CurrentData : Entry->DataBlocks)
			{
				const FName TagName = CurrentData.TagName;
				const bool bIsValidByFilter = FilterListPtr->IsFilterEnabled(CurrentData.Category.ToString(), ELogVerbosity::All) && FilterListPtr->IsFilterEnabled(CurrentData.TagName.ToString(), ELogVerbosity::All);
				FVisualLogExtensionInterface* Extension = FVisualLogger::Get().GetExtensionForTag(TagName);
				if (!Extension)
				{
					continue;
				}

				if (!bIsValidByFilter)
				{
					Extension->DisableDrawingForData(World, Canvas, HelperActor, TagName, CurrentData, Entry->TimeStamp);
				}
				else
				{
					Extension->DrawData(World, Canvas, HelperActor, TagName, CurrentData, Entry->TimeStamp);
				}
			}

			const FVisualLogShapeElement* ElementToDraw = Entry->ElementsToDraw.GetData();
			const int32 ElementsCount = Entry->ElementsToDraw.Num();
			
			for (int32 ElementIndex = 0; ElementIndex < ElementsCount; ++ElementIndex, ++ElementToDraw)
			{
				if (FilterListPtr.IsValid() && !FilterListPtr->IsFilterEnabled(ElementToDraw->Category.ToString(), ElementToDraw->Verbosity))
				{
					continue;
				}

				const FColor Color = ElementToDraw->GetFColor();
				Canvas->SetDrawColor(Color);

				switch(ElementToDraw->GetType())
				{
				case EVisualLoggerShapeElement::SinglePoint:
					{
						const float Radius = float(ElementToDraw->Radius);
						const bool bDrawLabel = ElementToDraw->Description.IsEmpty() == false;
						const FVector* Location = ElementToDraw->Points.GetData();
						for (int32 Index = 0; Index < ElementToDraw->Points.Num(); ++Index, ++Location)
						{
							DrawDebugSphere(World, *Location, Radius, 16, Color);
							if (bDrawLabel)
							{
								LogVisualizer::DrawText(Canvas, Font, FString::Printf(TEXT("%s_%d"), *ElementToDraw->Description, Index), *Location);
							}
						}
					}
					break;
				case EVisualLoggerShapeElement::Segment:
					{
						const float Thickness = float(ElementToDraw->Thicknes);
						const bool bDrawLabel = ElementToDraw->Description.IsEmpty() == false && ElementToDraw->Points.Num() > 2;
						const FVector* Location = ElementToDraw->Points.GetData();
						for (int32 Index = 0; Index + 1 < ElementToDraw->Points.Num(); Index += 2, Location += 2)
						{
							DrawDebugLine(World, *Location, *(Location + 1), Color
								, /*bPersistentLines*/false, /*LifeTime*/-1
								, /*DepthPriority*/0, Thickness);

							if (bDrawLabel)
							{
								const FString PrintString = FString::Printf(TEXT("%s_%d"), *ElementToDraw->Description, Index);
								LogVisualizer::DrawTextCentered(Canvas, Font, PrintString, (*Location + (*(Location + 1) - *Location) / 2));
							}
						}
						if (ElementToDraw->Description.IsEmpty() == false)
						{
							LogVisualizer::DrawTextCentered(Canvas, Font, ElementToDraw->Description
								, ElementToDraw->Points[0] + (ElementToDraw->Points[1] - ElementToDraw->Points[0]) / 2);
						}
					}
					break;
				case EVisualLoggerShapeElement::Path:
					{
						const float Thickness = float(ElementToDraw->Thicknes);
						FVector Location = ElementToDraw->Points[0];
						for (int32 Index = 1; Index < ElementToDraw->Points.Num(); ++Index)
						{
							const FVector CurrentLocation = ElementToDraw->Points[Index];
							DrawDebugLine(World, Location, CurrentLocation, Color
								, /*bPersistentLines*/false, /*LifeTime*/-1
								, /*DepthPriority*/0, Thickness);
							Location = CurrentLocation;
						}
					}
					break;
				case EVisualLoggerShapeElement::Box:
					{
						const float Thickness = float(ElementToDraw->Thicknes);
						const bool bDrawLabel = ElementToDraw->Description.IsEmpty() == false && ElementToDraw->Points.Num() > 2;
						const FVector* BoxExtent = ElementToDraw->Points.GetData();
						for (int32 Index = 0; Index + 1 < ElementToDraw->Points.Num(); Index += 2, BoxExtent += 2)
						{
							FBox Box(*BoxExtent, *(BoxExtent + 1));
							DrawDebugBox(World, Box.GetCenter(), Box.GetExtent(), Color
								, /*bPersistentLines*/false, /*LifeTime*/-1
								, /*DepthPriority*/0/*, Thickness*/);

							if (bDrawLabel)
							{
								const FString PrintString = FString::Printf(TEXT("%s_%d"), *ElementToDraw->Description, Index);
								LogVisualizer::DrawTextCentered(Canvas, Font, PrintString, Box.GetCenter());
							}
						}
						if (ElementToDraw->Description.IsEmpty() == false)
						{
							LogVisualizer::DrawTextCentered(Canvas, Font, ElementToDraw->Description
								, ElementToDraw->Points[0] + (ElementToDraw->Points[1] - ElementToDraw->Points[0]) / 2);
						}
					}
					break;
				case EVisualLoggerShapeElement::Cone:
					{
						const float Thickness = float(ElementToDraw->Thicknes);
						const bool bDrawLabel = ElementToDraw->Description.IsEmpty() == false;
						for (int32 Index = 0; Index + 2 < ElementToDraw->Points.Num(); Index += 3)
						{
							const FVector Orgin = ElementToDraw->Points[Index];
							const FVector Direction = ElementToDraw->Points[Index + 1];
							const FVector Angles = ElementToDraw->Points[Index + 2];
							DrawDebugCone(World, Orgin, Direction, Angles.X, Angles.Y, Angles.Z, 16, Color);
							if (bDrawLabel)
							{
								LogVisualizer::DrawTextCentered(Canvas, Font, ElementToDraw->Description, Orgin);
							}
						}
					}
					break;
				case EVisualLoggerShapeElement::Cylinder:
					{
						const float Thickness = float(ElementToDraw->Thicknes);
						const bool bDrawLabel = ElementToDraw->Description.IsEmpty() == false;
						for (int32 Index = 0; Index + 2 < ElementToDraw->Points.Num(); Index += 3)
						{
							const FVector Start = ElementToDraw->Points[Index];
							const FVector End = ElementToDraw->Points[Index + 1];
							const FVector OtherData = ElementToDraw->Points[Index + 2];
							DrawDebugCylinder(World, Start, End, OtherData.X, 16, Color);
							if (bDrawLabel)
							{
								LogVisualizer::DrawTextCentered(Canvas, Font, ElementToDraw->Description, Start);
							}
						}
					}
					break;
				case EVisualLoggerShapeElement::Capsule:
					{
						const float Thickness = float(ElementToDraw->Thicknes);
						const bool bDrawLabel = ElementToDraw->Description.IsEmpty() == false;
						for (int32 Index = 0; Index + 2 < ElementToDraw->Points.Num(); Index += 3)
						{
							const FVector Center = ElementToDraw->Points[Index];
							const FVector FirstData = ElementToDraw->Points[Index + 1];
							const FVector SecondData = ElementToDraw->Points[Index + 2];
							DrawDebugCapsule(World, Center, FirstData.X, FirstData.Y, FQuat(FirstData.Z, SecondData.X, SecondData.Y, SecondData.Z), Color);
							if (bDrawLabel)
							{
								LogVisualizer::DrawTextCentered(Canvas, Font, ElementToDraw->Description, Center);
							}
						}
					}
					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

const FSlateBrush* SLogVisualizer::GetRecordButtonBrush() const
{
	if(LogVisualizer->IsRecording())
	{
		// If recording, show stop button
		return 	FEditorStyle::GetBrush("LogVisualizer.Stop");
	}
	else
	{
		// If stopped, show record button
		return 	FEditorStyle::GetBrush("LogVisualizer.Record");
	}
}

FString SLogVisualizer::GetStatusText() const
{
	return TEXT("");
}

ESlateCheckBoxState::Type SLogVisualizer::GetPauseState() const
{
	UWorld* World = GetWorld();
	return (World != NULL && (World->bPlayersOnly || World->bPlayersOnlyPending)) ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

FReply SLogVisualizer::OnRecordButtonClicked()
{
	// Toggle recording state
	LogVisualizer->SetIsRecording(!LogVisualizer->IsRecording());

	return FReply::Handled();
}

FReply SLogVisualizer::OnLoad()
{
	TArray<FString> OpenFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bOpened = false;
	if ( DesktopPlatform )
	{
		void* ParentWindowWindowHandle = NULL;

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		bOpened = DesktopPlatform->OpenFileDialog(
			ParentWindowWindowHandle,
			LOCTEXT("OpenProjectBrowseTitle", "Open Project").ToString(),
			LastBrowsePath,
			TEXT(""),
			LogVisualizer::LoadFileTypes,
			EFileDialogFlags::None,
			OpenFilenames
			);
	}

	if ( bOpened )
	{
		if ( OpenFilenames.Num() > 0 )
		{
			LastBrowsePath = OpenFilenames[0];
			LoadFiles(OpenFilenames);
		}
	}

	DoFullUpdate();

	return FReply::Handled();
}

FReply SLogVisualizer::OnSave()
{
	// Prompt the user for the filenames
	TArray<FString> SaveFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bSaved = false;
	if ( DesktopPlatform )
	{
		void* ParentWindowWindowHandle = NULL;

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		bSaved = DesktopPlatform->SaveFileDialog(
			ParentWindowWindowHandle,
			LOCTEXT("NewProjectBrowseTitle", "Choose a project location").ToString(),
			LastBrowsePath,
			TEXT(""),
			LogVisualizer::SaveFileTypes,
			EFileDialogFlags::None,
			SaveFilenames
			);
	}

	if ( bSaved )
	{
		if ( SaveFilenames.Num() > 0 )
		{
			LastBrowsePath = SaveFilenames[0];
			SaveSelectedLogs(SaveFilenames[0]);
			/*CurrentProjectFilePath = FPaths::GetPath(FPaths::GetPath(SaveFilenames[0]));
			CurrentProjectFileName = FPaths::GetBaseFilename(SaveFilenames[0]);*/
		}
	}

	return FReply::Handled();
}

FReply SLogVisualizer::OnRemove()
{
	TArray< TSharedPtr<FLogsListItem> > ItemsToRemove = LogsListWidget->GetSelectedItems();
	if (ItemsToRemove.Num() > 0)
	{
		TArray<int32> IndicesToRemove;
		IndicesToRemove.AddUninitialized(ItemsToRemove.Num());
	
		for (int32 ListItemIndex = 0; ListItemIndex < ItemsToRemove.Num(); ++ListItemIndex)
		{
			IndicesToRemove[ListItemIndex] = ItemsToRemove[ListItemIndex]->LogIndex;
		}

		IndicesToRemove.Sort();

		int32 PrevIdx = -1;
		for (int32 LogToRemove = IndicesToRemove.Num() - 1; LogToRemove >= 0; --LogToRemove)
		{
			if (IndicesToRemove[LogToRemove] == PrevIdx)
			{
				continue;
			}

			LogVisualizer->Logs.RemoveAtSwap(IndicesToRemove[LogToRemove], 1, false);
			PrevIdx = IndicesToRemove[LogToRemove];

			const int32 IndexInList = FindIndexInLogsList(IndicesToRemove[LogToRemove]);
			if (IndexInList != INDEX_NONE)
			{
				LogsList.RemoveAtSwap(IndexInList);
			}
		}

		LogsListWidget->ClearSelection();

		LogsList.Reset();
		OutEntriesCached.Reset();
		RebuildFilteredList();
	}

	return FReply::Handled();
}

void SLogVisualizer::OnPauseChanged(ESlateCheckBoxState::Type NewState)
{
	UWorld* World = GetWorld();
	if (World != NULL)
	{
		if (NewState != ESlateCheckBoxState::Checked)
		{
			World->bPlayersOnly = false;
			World->bPlayersOnlyPending = false;

			ALogVisualizerCameraController::DisableCamera(World);
		}
		else
		{
			World->bPlayersOnlyPending = true;
			// switch debug cam on
			CameraController = ALogVisualizerCameraController::EnableCamera(World);
			if (CameraController.IsValid())
			{
				CameraController->OnActorSelected = ALogVisualizerCameraController::FActorSelectedDelegate::CreateSP(
					this, &SLogVisualizer::CameraActorSelected
				);
				CameraController->OnIterateLogEntries = ALogVisualizerCameraController::FLogEntryIterationDelegate::CreateSP(
					this, &SLogVisualizer::IncrementCurrentLogIndex
				);
			}
		}
	}
}

void SLogVisualizer::CameraActorSelected(AActor* SelectedActor)
{
	// find log corresponding to this Actor
	if (SelectedActor == NULL || LogVisualizer == NULL)
	{
		return;
	}

	SelectActor(SelectedActor);
}

void SLogVisualizer::SelectActor(AActor* SelectedActor)
{
	if (SelectedActor == NULL)
	{
		return;
	}
	const AActor* LogOwner = Cast<AActor>(FVisualLogger::Get().FindRedirection(SelectedActor));
	const int32 LogIndex = LogVisualizer->GetLogIndexForActor(LogOwner);
	if (LogVisualizer->Logs.IsValidIndex(LogIndex))
	{
		SelectedLogIndex = LogIndex;

		// find item pointing to given log index
		for (int32 ItemIndex = 0; ItemIndex < LogsList.Num(); ++ItemIndex)
		{
			if (LogsList[ItemIndex]->LogIndex == LogIndex)
			{
				TSharedPtr<FActorsVisLog> Log = LogVisualizer->Logs[SelectedLogIndex];
				ShowLogEntry(LogsList[ItemIndex], Log->Entries[Log->Entries.Num()-1]);
				break;
			}
		}
	}
}

void SLogVisualizer::OnQuickFilterTextChanged(const FText& CommentText, ETextCommit::Type CommitInfo)
{
	QuickFilterText = CommentText.ToString();
	OnLogCategoryFiltersChanged();
}

void SLogVisualizer::FilterTextCommitted(const FText& CommentText, ETextCommit::Type CommitInfo)
{
	UpdateFilterInfo();
	DoFullUpdate();
}

FString SLogVisualizer::GetLogEntryStatusText() const
{
	return TEXT("Pause game with Pause button\nand select log entry to start viewing\nlog's content");
}

void SLogVisualizer::OnSortByChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
{
	SortBy = ELogsSortMode::ByName;

	if (ColumnName == NAME_StartTime)
	{
		SortBy = ELogsSortMode::ByStartTime;
	}
	else if (ColumnName == NAME_EndTime)
	{
		SortBy = ELogsSortMode::ByEndTime;
	}

	LogsList.Reset();
	OutEntriesCached.Reset();
	RebuildFilteredList();
}

EColumnSortMode::Type SLogVisualizer::GetLogsSortMode() const
{
	return (SortBy == ELogsSortMode::ByName) ? EColumnSortMode::Ascending : EColumnSortMode::None;
}

void SLogVisualizer::LoadFiles(TArray<FString>& OpenFilenames)
{
	TArray<FVisualLogDevice::FVisualLogEntryItem> FrameCache;

	for (int FilenameIndex = 0; FilenameIndex < OpenFilenames.Num(); ++FilenameIndex)
	{
		FString CurrentFileName = OpenFilenames[FilenameIndex];
		const bool bIsBinaryFile = CurrentFileName.Find(TEXT(".bvlog")) != INDEX_NONE;
		if (!bIsBinaryFile)
		{
			FArchive* FileAr = IFileManager::Get().CreateFileReader(*CurrentFileName);
			if (FileAr != NULL)
			{
				TSharedPtr<FJsonObject> Object;
				TSharedRef<TJsonReader<UCS2CHAR> > Reader = TJsonReader<UCS2CHAR>::Create(FileAr);

				if (FJsonSerializer::Deserialize(Reader, Object))
				{
					TArray< TSharedPtr<FJsonValue> > JsonLogs = Object->GetArrayField(VisualLogJson::TAG_LOGS);
					for (int32 LogIndex = 0; LogIndex < JsonLogs.Num(); ++LogIndex)
					{
						TSharedPtr<FJsonObject> JsonLogObject = JsonLogs[LogIndex]->AsObject();
						if (JsonLogObject.IsValid() != false)
						{
							if (JsonLogObject->HasTypedField<EJson::String>(VisualLogJson::TAG_NAME))
							{
								TSharedPtr<FActorsVisLog> NewLog = MakeShareable(new FActorsVisLog(JsonLogs[LogIndex]));
								LogVisualizer->AddLoadedLog(NewLog);
							}
						}
					}
					bIgnoreTrivialLogs = false;
				}

				FileAr->Close();
			}
		}
		else
		{
			TArray<FVisualLogDevice::FVisualLogEntryItem> RecordedLogs;
			FArchive* FileArchive = IFileManager::Get().CreateFileReader(*(OpenFilenames[FilenameIndex]));
			FVisualLoggerHelpers::Serialize(*FileArchive, RecordedLogs);
			FrameCache.Append(RecordedLogs);
			FileArchive->Close();
			delete FileArchive;
			FileArchive = NULL;
		}
	}

	if (OpenFilenames.Num() > 0)
	{
		if (FrameCache.Num() > 0)
		{
			TMap<FName, TSharedPtr<FActorsVisLog>> HelperMap;

			for (auto& CurrentLog : FrameCache)
			{
				if (HelperMap.Contains(CurrentLog.OwnerName) == false)
				{
					HelperMap.Add(CurrentLog.OwnerName, MakeShareable(new FActorsVisLog(CurrentLog.OwnerName, NULL)));
				}
				HelperMap[CurrentLog.OwnerName]->Entries.Add(MakeShareable(new FVisualLogEntry(CurrentLog.Entry)));
			}

			for (auto& CurrentLog : HelperMap)
			{
				LogVisualizer->AddLoadedLog(CurrentLog.Value);
			}
		}

		LogsList.Reset();
		OutEntriesCached.Reset();
		RebuildFilteredList();
	}
}

void SLogVisualizer::SaveSelectedLogs(FString& Filename)
{
	FVisualLogger::Get().Flush();
	TArray< TSharedPtr<FLogsListItem> > ItemsToSave = LogsListWidget->GetSelectedItems();
	if (ItemsToSave.Num() == 0)
	{
		// store all 
		ItemsToSave = LogsList;
	}

	TArray<FVisualLogDevice::FVisualLogEntryItem> FrameCache;
	for (auto CurrentItem : ItemsToSave)
	{
		if (CurrentItem.IsValid() && LogVisualizer->Logs.IsValidIndex(CurrentItem->LogIndex))
		{
			TSharedPtr<FActorsVisLog> Log = LogVisualizer->Logs[CurrentItem->LogIndex];
			for (auto CurrentEntry : Log->Entries)
			{
				FrameCache.Add(FVisualLogDevice::FVisualLogEntryItem(Log->Name, *CurrentEntry.Get()));
			}
		}
	}

	if (FrameCache.Num())
	{
		FArchive* FileArchive = IFileManager::Get().CreateFileWriter(*Filename);
		FVisualLoggerHelpers::Serialize(*FileArchive, FrameCache);
		FileArchive->Close();
		delete FileArchive;
		FileArchive = NULL;
	}
}

#undef LOCTEXT_NAMESPACE

#endif //ENABLE_VISUAL_LOG

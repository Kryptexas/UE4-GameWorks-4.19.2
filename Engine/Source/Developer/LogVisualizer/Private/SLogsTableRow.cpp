// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizerPCH.h"
#include "SLogBar.h"

void SLogsTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	Item = InArgs._Item;
	OwnerVisualizerWidgetPtr = InArgs._OwnerVisualizerWidget;
	SMultiColumnTableRow< TSharedPtr<FLogsListItem> >::Construct(FSuperRowType::FArguments(), InOwnerTableView);

	/*if(Item->bIsGroup)
	{
		BorderImage = FEditorStyle::GetBrush("LogVisualizer.GroupBackground");
	}*/
}

TSharedRef<SWidget> SLogsTableRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	TSharedPtr<SLogVisualizer> OwnerVisualizerWidget = OwnerVisualizerWidgetPtr.Pin();

	const int32 LogId = Item->LogIndex;
	if(LogId >= OwnerVisualizerWidget->LogVisualizer->Logs.Num())
	{
		return	SNew(STextBlock)
				.Text(NSLOCTEXT("LogVisualizer", "RowCellError", "ERROR"));
	}

	FActorsVisLog& Log = *(OwnerVisualizerWidget->LogVisualizer->Logs[LogId]);

	if (ColumnName == SLogVisualizer::NAME_LogName)
	{	
		return SNew(SBox)
				.VAlign(VAlign_Center)
				.Padding(2)
				.Content()
				[
					SNew(STextBlock)
					.Text(Log.Name.ToString())
				];
	}
	else if (ColumnName == SLogVisualizer::NAME_LogTimeSpan)
	{
		TSharedPtr<FActorsVisLog>& Log = OwnerVisualizerWidget->LogVisualizer->Logs[Item->LogIndex];
		const float TotalSize = OwnerVisualizerWidget->LogsEndTime - OwnerVisualizerWidget->LogsStartTime;

		if (LogBar.IsValid())
		{
			OwnerVisualizerWidget->OnZoomChanged().Remove(
				SLogVisualizer::FOnZoomChanged::FDelegate::CreateSP(LogBar.Get(), &SLogBar::SetZoomAndOffset)
				);
		}

		LogBar = SNew( SLogBar )
			.OnSelectionChanged(this, &SLogsTableRow::OnBarGraphSelectionChanged)
			.OnGeometryChanged(this, &SLogsTableRow::OnBarGeometryChanged)
			.ShouldDrawSelection(this, &SLogsTableRow::ShouldDrawSelection)
			.DisplayedTime(OwnerVisualizerWidget.Get(), &SLogVisualizer::GetCurrentViewedTime)
			.CurrentEntryIndex(this, &SLogsTableRow::GetCurrentLogEntryIndex);

		OwnerVisualizerWidget->GetVisibleEntries(Log, VisibleEntries);
		LogBar->SetEntries(VisibleEntries, OwnerVisualizerWidget->LogsStartTime, TotalSize);
		LogBar->SetZoom( OwnerVisualizerWidget->GetZoom() );
		LogBar->SetOffset( OwnerVisualizerWidget->GetScrollbarOffset() );
		LogBar->SetRowIndex(IndexInList);

		OwnerVisualizerWidget->OnZoomChanged().Add(
			SLogVisualizer::FOnZoomChanged::FDelegate::CreateSP(LogBar.Get(), &SLogBar::SetZoomAndOffset)
			);

		return LogBar.ToSharedRef();
	}

	return SNullWidget::NullWidget;
}

int32 SLogsTableRow::GetCurrentLogEntryIndex() const
{
	TSharedPtr<SLogVisualizer> OwnerVisualizerWidget = OwnerVisualizerWidgetPtr.Pin();
	
	return OwnerVisualizerWidget->GetCurrentVisibleLogEntryIndex(VisibleEntries);
}

bool SLogsTableRow::ShouldDrawSelection()
{
	TSharedPtr<ITypedTableView<TSharedPtr<FLogsListItem> > > OwnerTable = OwnerTablePtr.Pin();
	return OwnerTable.IsValid() && OwnerTable->Private_IsItemSelected(Item);
}

void SLogsTableRow::OnBarGraphSelectionChanged( TSharedPtr<FVisLogEntry> Selection )
{
	if( Selection.IsValid() )
	{
		TSharedPtr<SLogVisualizer> OwnerVisualizerWidget = OwnerVisualizerWidgetPtr.Pin();
		if (OwnerVisualizerWidget.IsValid())
		{
			OwnerVisualizerWidget->RequestShowLogEntry(Item, Selection);
		}
	}
}

void SLogsTableRow::OnBarGeometryChanged( FGeometry Geometry )
{
	TSharedPtr<SLogVisualizer> OwnerVisualizerWidget = OwnerVisualizerWidgetPtr.Pin();

	if (OwnerVisualizerWidget.IsValid())
	{
		OwnerVisualizerWidget->GetTimeline()->SetDrawingGeometry( Geometry );
	}
}

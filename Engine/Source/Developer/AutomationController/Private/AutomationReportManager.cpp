// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AutomationReportManager.cpp: Implements the FAutomationReportManager class.
=============================================================================*/

#include "AutomationControllerPrivatePCH.h"


FAutomationReportManager::FAutomationReportManager()
{
	//ensure that we have a valid root to the hierarchy
	FAutomationTestInfo TestInfo;
	ReportRoot = MakeShareable(new FAutomationReport( TestInfo ));
}


void FAutomationReportManager::Empty()
{
	//ensure there is a root node
	ReportRoot->Empty();
}


void FAutomationReportManager::ResetForExecution()
{
	//Recursively prepare all tests for execution
	ReportRoot->ResetForExecution();
}


void FAutomationReportManager::StopRunningTests()
{
	ReportRoot->StopRunningTest();
}

TSharedPtr<IAutomationReport> FAutomationReportManager::GetNextReportToExecute(bool& bOutAllTestsComplete, const int32 ClusterIndex, const int32 NumDevicesInCluster)
{
	TSharedPtr<IAutomationReport> ReportToExecute = ReportRoot->GetNextReportToExecute(bOutAllTestsComplete, ClusterIndex, NumDevicesInCluster);
	return ReportToExecute;
}


TSharedPtr<IAutomationReport> FAutomationReportManager::EnsureReportExists(FAutomationTestInfo& TestInfo, const int32 ClusterIndex)
{
	TSharedPtr<IAutomationReport> NewStatus = ReportRoot->EnsureReportExists (TestInfo, ClusterIndex);
	return NewStatus;
}


void FAutomationReportManager::SetFilter( TSharedPtr< AutomationFilterCollection > InFilter )
{
	ReportRoot->SetFilter( InFilter );
}


TArray <TSharedPtr <IAutomationReport> >& FAutomationReportManager::GetFilteredReports()
{
	return ReportRoot->GetFilteredChildren();
}


void FAutomationReportManager::SetVisibleTestsEnabled(const bool bEnabled)
{
	ReportRoot->SetEnabled(bEnabled);
}


int32 FAutomationReportManager::GetEnabledTestsNum() const
{
	return ReportRoot->GetEnabledTestsNum();
}


const bool FAutomationReportManager::ExportReport(uint32 FileExportTypeMask, const int32 NumDeviceClusters)
{
	// The results log. Errors, warnings etc are added to this
	TArray<FString> ResultsLog;

	// Create the report be recursively going through the results tree
	FindLeafReport(ReportRoot, NumDeviceClusters, ResultsLog, FileExportTypeMask);

	// Has a report been generated
	bool ReportGenerated = false;

	// Ensure we have a log to write
	if (ResultsLog.Num())
	{
		// Create the file name
		FString FileName = FString::Printf( TEXT( "Automation%s.csv" ), *FDateTime::Now().ToString() );
		FString FileLocation = FPaths::ConvertRelativePathToFull(FPaths::AutomationDir());
		FString FullPath = FString::Printf( TEXT( "%s%s" ), *FileLocation, *FileName );

		// save file
		FArchive* LogFile = IFileManager::Get().CreateFileWriter( *FullPath );

		if (LogFile != NULL)
		{
			for (int32 Index = 0; Index < ResultsLog.Num(); ++Index)
			{
				FString LogEntry = FString::Printf( TEXT( "%s" ),
					*ResultsLog[ Index ]) + LINE_TERMINATOR;
				LogFile->Serialize( TCHAR_TO_ANSI( *LogEntry ), LogEntry.Len() );
			}

			LogFile->Close();
			delete LogFile;

			// A report has been generated
			ReportGenerated = true;
		}
	}

	return ReportGenerated;
}


void FAutomationReportManager::AddResultReport(TSharedPtr< IAutomationReport > InReport, const int32 NumDeviceClusters, TArray< FString >& ResultsLog, uint32 FileExportTypeMask)
{
	if (InReport->IsEnabled())
	{
		for (int32 ClusterIndex = 0; ClusterIndex < NumDeviceClusters; ++ClusterIndex)
		{
			FAutomationTestResults TestResults = InReport->GetResults( ClusterIndex );

			// Early out if we don't want this report
			bool AddReport = true;
			if  ( !EFileExportType::IsSet( FileExportTypeMask, EFileExportType::FET_All ) && !EFileExportType::IsSet( FileExportTypeMask, EFileExportType::FET_Status ) )
			{
				uint32 ResultMask = 0;

				if (TestResults.Errors.Num())
				{
					EFileExportType::SetFlag( ResultMask, EFileExportType::FET_Errors );
				}
				if (TestResults.Warnings.Num())
				{
					EFileExportType::SetFlag( ResultMask, EFileExportType::FET_Warnings );
				}
				if (TestResults.Logs.Num())
				{
					EFileExportType::SetFlag( ResultMask, EFileExportType::FET_Logs );
				}

				// Ensure we have a report that passes at least one filter
				if ( ( ResultMask & FileExportTypeMask ) == 0 )
				{
					AddReport = false;
				}
			}

			if (AddReport)
			{
				// Get the duration of the test
				float MinDuration;
				float MaxDuration;
				FString DurationString;
				if (InReport->GetDurationRange(MinDuration, MaxDuration ))
				{
					//if there is a duration range
					if (MinDuration != MaxDuration)
					{
						DurationString = FString::Printf( TEXT( "%4.4fs - %4.4fs" ), MinDuration, MaxDuration );
					}
					else
					{
						DurationString = FString::Printf( TEXT( "%4.4fs" ), MinDuration );
					}
				}

				// Build a status string that contains information about the test 
				FString Status;

				// Was the test a success
				if (( TestResults.Warnings.Num() == 0 ) && ( TestResults.Errors.Num() == 0 ) && ( InReport->GetState( ClusterIndex ) == EAutomationState::Success ))
				{
					Status = "Success";
				}
				else if( InReport->GetState( ClusterIndex ) == EAutomationState::NotEnoughParticipants )
				{
					Status = TEXT( "Could not run." );
				}
				else
				{
					Status = "Issues ";
					if (TestResults.Warnings.Num())
					{
						Status += "Warnings ";
					}
					if (TestResults.Errors.Num())
					{
						Status += "Errors";
					}
				}

				// Create the log string
				FString Info = FString::Printf( TEXT("DeviceName: %s, Platform: %s, Test Name: %s, Test Duration: %s, Status: %s" ),
					*InReport->GetGameInstanceName( ClusterIndex ),
					*FModuleManager::GetModuleChecked< IAutomationControllerModule >( "AutomationController" ).GetAutomationController()->GetDeviceTypeName( ClusterIndex ),
					*InReport->GetDisplayNameWithDecoration(),
					*DurationString,
					*Status );

				ResultsLog.Add( Info );

				// Pre and post fix strings to add to the errors and warnings for viewing in CSV. Spaces correctly, and allows commas in the message
				const FString ErrorPrefixText = TEXT( "\t,,,\"" );
				const FString ErrorSufixText = TEXT( "\"" );


				// Add any warning or errors
				if ( EFileExportType::IsSet( FileExportTypeMask, EFileExportType::FET_All ) || EFileExportType::IsSet( FileExportTypeMask, EFileExportType::FET_Errors ) )
				{
					for ( int32 ErrorIndex = 0; ErrorIndex < TestResults.Errors.Num(); ++ErrorIndex )
					{
						ResultsLog.Add( ErrorPrefixText + TestResults.Errors[ ErrorIndex ] + ErrorSufixText );
					}
				}
				if ( EFileExportType::IsSet( FileExportTypeMask, EFileExportType::FET_All ) || EFileExportType::IsSet( FileExportTypeMask, EFileExportType::FET_Warnings ) )
				{
					for ( int32 WarningIndex = 0; WarningIndex < TestResults.Warnings.Num(); ++WarningIndex )
					{
						ResultsLog.Add( ErrorPrefixText + TestResults.Warnings[ WarningIndex ] + ErrorSufixText );
					}
				}
				if ( EFileExportType::IsSet( FileExportTypeMask, EFileExportType::FET_All ) || EFileExportType::IsSet( FileExportTypeMask, EFileExportType::FET_Logs ) )
				{
					for ( int32 LogIndex = 0; LogIndex < TestResults.Logs.Num(); ++LogIndex )
					{
						ResultsLog.Add( ErrorPrefixText + TestResults.Logs[ LogIndex ] + ErrorSufixText );
					}
				}
			}
		}
	}
}


void FAutomationReportManager::FindLeafReport(TSharedPtr< IAutomationReport > InReport, const int32 NumDeviceClusters, TArray< FString >& ResultsLog, uint32 FileExportTypeMask)
{
	TArray< TSharedPtr< IAutomationReport > >& ChildReports = InReport->GetFilteredChildren();

	// If there are no child reports, we have reached a leaf that has had a test run on it.
	if (ChildReports.Num() == 0)
	{
		AddResultReport( InReport, NumDeviceClusters, ResultsLog, FileExportTypeMask );
	}
	else
	{
		// We still have some child nodes. We won't have run a test on this node
		for (int32 ChildIndex = 0; ChildIndex < ChildReports.Num(); ++ChildIndex)
		{
			FindLeafReport( ChildReports[ ChildIndex ], NumDeviceClusters, ResultsLog, FileExportTypeMask );
		}
	}
}

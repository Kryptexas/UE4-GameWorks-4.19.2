// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherProgressPage.h: Declares the SSessionLauncherProgressPage class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionLauncherProgress"


/**
 * Implements the launcher's progress page.
 */
class SSessionLauncherProgress
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherProgress) { }
	SLATE_END_ARGS()


public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherProgress( )
	{

	}


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 */
	void Construct( const FArguments& InArgs )
	{
		ChildSlot
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0, 16.0, 16.0, 0.0)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
								.Text(this, &SSessionLauncherProgress::HandleProgressTextBlockText)
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0, 4.0, 0.0, 0.0)
						[
							SAssignNew(ProgressBar, SProgressBar)
								.Percent(this, &SSessionLauncherProgress::HandleProgressBarPercent)
						]
				]

			+ SVerticalBox::Slot()
				.FillHeight(0.5)
				.Padding(0.0, 32.0, 8.0, 0.0)
				[
					SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.Padding(0.0f)
						[
							SAssignNew(TaskListView, SListView<ILauncherTaskPtr>)
								.HeaderRow
								(
									SNew(SHeaderRow)

									+ SHeaderRow::Column("Icon")
										.DefaultLabel(LOCTEXT("TaskListIconColumnHeader", " ").ToString())
										.FixedWidth(20.0)

									+ SHeaderRow::Column("Task")
										.DefaultLabel(LOCTEXT("TaskListTaskColumnHeader", "Task").ToString())
										.FillWidth(1.0)

									+ SHeaderRow::Column("Duration")
										.DefaultLabel(LOCTEXT("TaskListDurationColumnHeader", "Duration").ToString())
										.FixedWidth(64.0)

									+ SHeaderRow::Column("Status")
										.DefaultLabel(LOCTEXT("TaskListStatusColumnHeader", "Status").ToString())
										.FixedWidth(80.0)
								)
								.ListItemsSource(&TaskList)
								.OnGenerateRow(this, &SSessionLauncherProgress::HandleTaskListViewGenerateRow)
								.ItemHeight(24.0)
								.SelectionMode(ESelectionMode::Single)
						]
				]

			//content area for the log
			+ SVerticalBox::Slot()
				.FillHeight(0.5)
				.Padding(0.0, 32.0, 8.0, 0.0)
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(0.0f)
					[
						SAssignNew(MessageListView, SListView< TSharedPtr<FLauncherMessage> >)
						.HeaderRow
						(
							SNew(SHeaderRow)

							+ SHeaderRow::Column("Status")
							.DefaultLabel(LOCTEXT("TaskListStatusColumnHeader", "Status").ToString())
							.FillWidth(1.0)
						)
						.ListItemsSource(&MessageList)
						.OnGenerateRow(this, &SSessionLauncherProgress::HandleMessageListViewGenerateRow)
						.ItemHeight(24.0)
						.SelectionMode(ESelectionMode::Multi)
					]
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						// copy button
						SAssignNew(CopyButton, SButton)
						.ContentPadding(FMargin(6.0f, 2.0f))
						.IsEnabled(false)
						.Text(LOCTEXT("CopyButtonText", "Copy").ToString())
						.ToolTipText(LOCTEXT("CopyButtonTooltip", "Copy the selected log messages to the clipboard").ToString())
						.OnClicked(this, &SSessionLauncherProgress::HandleCopyButtonClicked)
					]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(4.0f, 0.0f, 0.0f, 0.0f)
						[
							// clear button
							SAssignNew(ClearButton, SButton)
							.ContentPadding(FMargin(6.0f, 2.0f))
							.IsEnabled(false)
							.Text(LOCTEXT("ClearButtonText", "Clear Log").ToString())
							.ToolTipText(LOCTEXT("ClearButtonTooltip", "Clear the log window").ToString())
							.OnClicked(this, &SSessionLauncherProgress::HandleClearButtonClicked)
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(4.0f, 0.0f, 0.0f, 0.0f)
						[
							// save button
							SAssignNew(SaveButton, SButton)
							.ContentPadding(FMargin(6.0f, 2.0f))
							.IsEnabled(false)
							.Text(LOCTEXT("ExportButtonText", "Save Log...").ToString())
							.ToolTipText(LOCTEXT("SaveButtonTooltip", "Save the entire log to a file").ToString())
							.Visibility((FDesktopPlatformModule::Get() != NULL) ? EVisibility::Visible : EVisibility::Collapsed)
							.OnClicked(this, &SSessionLauncherProgress::HandleSaveButtonClicked)
						]
				]
		];
	}

	/**
	 * Sets the launcher worker to track the progress for.
	 *
	 * @param Worker - The launcher worker.
	 */
	void SetLauncherWorker( const ILauncherWorkerRef& Worker )
	{
		LauncherWorker = Worker;

		Worker->GetTasks(TaskList);
		TaskListView->RequestListRefresh();

		MessageList.Reset();
		Worker->OnOutputReceived().AddRaw(this, &SSessionLauncherProgress::HandleOutputReceived);
		MessageListView->RequestListRefresh();
	}

	void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE
	{
		if (PendingMessages.Num() > 0)
		{
			FScopeLock ScopedLock(&CriticalSection);
			for (int32 Index = 0; Index < PendingMessages.Num(); ++Index)
			{
				MessageList.Add(PendingMessages[Index]);
			}
			PendingMessages.Reset();
			MessageListView->RequestListRefresh();
			MessageListView->RequestScrollIntoView(MessageList.Last());
		}
		SaveButton->SetEnabled(MessageList.Num() > 0);
		ClearButton->SetEnabled(MessageList.Num() > 0);
		CopyButton->SetEnabled(MessageListView->GetNumItemsSelected() > 0);
	}
private:

	void HandleOutputReceived(const FString& InMessage)
	{
		FScopeLock ScopedLock(&CriticalSection);
		ELogVerbosity::Type Verbosity = ELogVerbosity::Display;
		if (InMessage.Contains(TEXT("ERROR:"), ESearchCase::CaseSensitive))
		{
			Verbosity = ELogVerbosity::Error;
		}
		else if (InMessage.Contains(TEXT("WARNING:"), ESearchCase::CaseSensitive))
		{

			Verbosity = ELogVerbosity::Warning;
		}
		TSharedPtr<FLauncherMessage> Message = MakeShareable(new FLauncherMessage(InMessage, Verbosity));
		PendingMessages.Add(Message);
	}

	// Callback for getting the filled percentage of the progress bar.
	TOptional<float> HandleProgressBarPercent( ) const
	{
		if (TaskList.Num() > 0)
		{
			ILauncherWorkerPtr LauncherWorkerPtr = LauncherWorker.Pin();

			if (LauncherWorkerPtr.IsValid())
			{
				int32 NumFinished = 0;

				for (int32 TaskIndex = 0; TaskIndex < TaskList.Num(); ++TaskIndex)
				{
					if (TaskList[TaskIndex]->IsFinished())
					{
						++NumFinished;
					}
				}

				return ((float)NumFinished / TaskList.Num());
			}
		}

		return 0.0f;
	}

	// Callback for getting the text of the progress text box.
	FString HandleProgressTextBlockText( ) const
	{
		ILauncherWorkerPtr LauncherWorkerPtr = LauncherWorker.Pin();

		if (LauncherWorkerPtr.IsValid())
		{
			if ((LauncherWorkerPtr->GetStatus() == ELauncherWorkerStatus::Busy) ||
				(LauncherWorkerPtr->GetStatus() == ELauncherWorkerStatus::Canceling))
			{
				return LOCTEXT("OperationInProgressText", "Operation in progress...").ToString();
			}

			int32 NumCanceled = 0;
			int32 NumCompleted = 0;
			int32 NumFailed = 0;

			for (int32 TaskIndex = 0; TaskIndex < TaskList.Num(); ++TaskIndex)
			{
				ELauncherTaskStatus::Type TaskStatus = TaskList[TaskIndex]->GetStatus();

				if (TaskStatus == ELauncherTaskStatus::Canceled)
				{
					++NumCanceled;
				}
				else if (TaskStatus == ELauncherTaskStatus::Completed)
				{
					++NumCompleted;
				}
				else if (TaskStatus == ELauncherTaskStatus::Failed)
				{
					++NumFailed;
				}
			}

			return FString::Printf(*LOCTEXT("TasksFinishedFormatText", "Operation finished. Completed: %i, Failed: %i, Canceled: %i").ToString(), NumCompleted, NumFailed, NumCanceled);
		}

		return FString();
	}

	// Callback for generating a row in the task list view.
	TSharedRef<ITableRow> HandleTaskListViewGenerateRow( ILauncherTaskPtr InItem, const TSharedRef<STableViewBase>& OwnerTable ) const
	{
		return SNew(SSessionLauncherTaskListRow)
			.Task(InItem)
			.OwnerTableView(OwnerTable);
	}

	// Callback for generating a row in the task list view.
	TSharedRef<ITableRow> HandleMessageListViewGenerateRow( TSharedPtr<FLauncherMessage> InItem, const TSharedRef<STableViewBase>& OwnerTable ) const
	{
		return SNew(SSessionLauncherMessageListRow, OwnerTable)
			.Message(InItem)
			.ToolTipText(InItem->Message);
	}

	FReply HandleClearButtonClicked( )
	{
		ClearLog();
		return FReply::Handled();
	}

	FReply HandleCopyButtonClicked( )
	{
		CopyLog();
		return FReply::Handled();
	}

	FReply HandleSaveButtonClicked( )
	{
		SaveLog();
		return FReply::Handled();
	}

	void ClearLog()
	{
		MessageList.Reset();
		MessageListView->RequestListRefresh();
	}

	void CopyLog()
	{
		TArray<TSharedPtr<FLauncherMessage > > SelectedItems = MessageListView->GetSelectedItems();

		if (SelectedItems.Num() > 0)
		{
			FString SelectedText;

			for( int32 Index = 0; Index < SelectedItems.Num(); ++Index )
			{
				SelectedText += FString::Printf(TEXT("%s"),
					*SelectedItems[Index]->Message
					) + LINE_TERMINATOR;
			}

			FPlatformMisc::ClipboardCopy( *SelectedText );
		}
	}

	void SaveLog()
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

		if (DesktopPlatform != NULL)
		{
			TArray<FString> Filenames;

			if (DesktopPlatform->SaveFileDialog(
				NULL,
				LOCTEXT("SaveLogDialogTitle", "Save Log As...").ToString(),
				LastLogFileSaveDirectory,
				TEXT("Launcher.log"),
				TEXT("Log Files (*.log)|*.log"),
				EFileDialogFlags::None,
				Filenames))
			{
				if (Filenames.Num() > 0)
				{
					FString Filename = Filenames[0];

					// keep path as default for next time
					LastLogFileSaveDirectory = FPaths::GetPath(Filename);

					// add a file extension if none was provided
					if (FPaths::GetExtension(Filename).IsEmpty())
					{
						Filename += Filename + TEXT(".log");
					}

					// save file
					FArchive* LogFile = IFileManager::Get().CreateFileWriter(*Filename);

					if (LogFile != NULL)
					{
						for( int32 Index = 0; Index < MessageList.Num(); ++Index )
						{
							FString LogEntry = FString::Printf(TEXT("%s"),
								*MessageList[Index]->Message) + LINE_TERMINATOR;

							LogFile->Serialize(TCHAR_TO_ANSI(*LogEntry), LogEntry.Len());
						}

						LogFile->Close();

						delete LogFile;
					}
					else
					{
						FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SaveLogDialogFileError", "Failed to open the specified file for saving!"));
					}
				}
			}
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SaveLogDialogUnsupportedError", "Saving is not supported on this platform!"));
		}
	}

private:

	// Holds the launcher worker.
	TWeakPtr<ILauncherWorker> LauncherWorker;

	// Holds the output log.
	TArray<TSharedPtr<FString> > OutputList;

	// Holds the output list view.
	TSharedPtr<SListView<TSharedPtr<FString> > > OutputListView;

	// Holds the progress bar.
	TSharedPtr<SProgressBar> ProgressBar;

	// Holds the task list.
	TArray<ILauncherTaskPtr> TaskList;

	// Holds the message list.
	TArray< TSharedPtr<FLauncherMessage > > MessageList;

	// Holds the pending message list.
	TArray< TSharedPtr<FLauncherMessage > > PendingMessages;

	// Holds the message list view.
	TSharedPtr<SListView<TSharedPtr<FLauncherMessage>> > MessageListView;

	// Holds the task list view.
	TSharedPtr<SListView<ILauncherTaskPtr> > TaskListView;

	// Holds the box of task statuses.
	TSharedPtr<SVerticalBox> TaskStatusBox;

	// Critical section for updating the messages
	FCriticalSection CriticalSection;

	// Holds the directory where the log file was last saved to.
	FString LastLogFileSaveDirectory;

	// Holds the copy log button.
	TSharedPtr<SButton> CopyButton;

	// Holds the clear button.
	TSharedPtr<SButton> ClearButton;

	// Holds the save button.
	TSharedPtr<SButton> SaveButton;
};


#undef LOCTEXT_NAMESPACE

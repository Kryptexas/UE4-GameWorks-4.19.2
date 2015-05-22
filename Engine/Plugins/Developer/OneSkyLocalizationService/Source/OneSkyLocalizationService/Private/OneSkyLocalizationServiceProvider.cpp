// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OneSkyLocalizationServicePrivatePCH.h"
#include "OneSkyLocalizationServiceProvider.h"
#include "OneSkyLocalizationServiceCommand.h"
#include "OneSkyConnection.h"
#include "IOneSkyLocalizationServiceWorker.h"
#include "OneSkyLocalizationServiceModule.h"
#include "OneSkyLocalizationServiceSettings.h"
#include "MessageLog.h"
#include "Developer/LocalizationService/Public/ScopedLocalizationServiceProgress.h"
#if LOCALIZATION_SERVICES_WITH_SLATE

#include "Editor/PropertyEditor/Public/DetailLayoutBuilder.h"
#include "Editor/PropertyEditor/Public/DetailWidgetRow.h"
#include "Editor/PropertyEditor/Public/IDetailPropertyRow.h"
#include "Editor/PropertyEditor/Public/DetailCategoryBuilder.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"

#endif

static FName ProviderName("OneSky");

#define LOCTEXT_NAMESPACE "OneSkyLocalizationService"

/** Init of connection with source control server */
void FOneSkyLocalizationServiceProvider::Init(bool bForceConnection)
{
	// TODO: Test if connection works
	bServerAvailable = true;

	// TODO: this is called on startup, delay until we need it
	// Move to EstablishConnection?
	RetrieveProjectGroups(ELocalizationServiceOperationConcurrency::Asynchronous);
}

/** API Specific close the connection with localization provider server*/
void FOneSkyLocalizationServiceProvider::Close()
{
	if ( PersistentConnection )
	{
		PersistentConnection->Disconnect();
		delete PersistentConnection;
		PersistentConnection = NULL;
	}

	// clear the cache
	StateCache.Empty();
	ProjectGroupCache.Empty();
	ProjectCachePerGroupId.Empty();

	bServerAvailable = false;
}

TSharedRef<FOneSkyLocalizationServiceState, ESPMode::ThreadSafe> FOneSkyLocalizationServiceProvider::GetStateInternal(const FLocalizationServiceTranslationIdentifier& InTranslationId)
{
	TSharedRef<FOneSkyLocalizationServiceState, ESPMode::ThreadSafe>* State = StateCache.Find(InTranslationId);
	if(State != NULL)
	{
		// found cached item
		return (*State);
	}
	else
	{
		// cache an unknown state for this item
		TSharedRef<FOneSkyLocalizationServiceState, ESPMode::ThreadSafe> NewState = MakeShareable(new FOneSkyLocalizationServiceState(InTranslationId));
		StateCache.Add(InTranslationId, NewState);
		return NewState;
	}
}

bool FOneSkyLocalizationServiceProvider::RetrieveProjectGroups(ELocalizationServiceOperationConcurrency::Type InConcurrency)
{
	// TODO: This should do the same thing as GetStateInternal but for project groups
	// Create a data type to hold the information from a project group
	// This logic should be moved to a function similar to GetState, but for projects
	if (!IsEnabled())
	{
		return ELocalizationServiceOperationCommandResult::Failed;
	}

	// LIST PROJECT GROUPS TEST
	//Execute(ILocalizationServiceOperation::Create<FOneSkyListProjectGroupsOperation>(), TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);
	//TSharedRef<FOneSkyListProjectGroupsOperation, ESPMode::ThreadSafe> ListProjectGroupsOperation = ILocalizationServiceOperation::Create<FOneSkyListProjectGroupsOperation>();
	//ListProjectGroupsOperation->SetInStartPage(1);
	//ListProjectGroupsOperation->SetInItemsPerPage(1);
	//Execute(ListProjectGroupsOperation, TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);

	// SHOW PROJECT GROUP TEST
	//TSharedRef<FOneSkyShowProjectGroupOperation, ESPMode::ThreadSafe> ShowProjectGroupOperation = ILocalizationServiceOperation::Create<FOneSkyShowProjectGroupOperation>();
	//ShowProjectGroupOperation->SetInProjectGroupId(10222);
	//Execute(ShowProjectGroupOperation, TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);

	// CREATE PROJECT GROUP TEST
	//TSharedRef<FOneSkyCreateProjectGroupOperation, ESPMode::ThreadSafe> CreateProjectGroupOperation = ILocalizationServiceOperation::Create<FOneSkyCreateProjectGroupOperation>();
	//CreateProjectGroupOperation->SetInProjectGroupName("test group");
	//CreateProjectGroupOperation->SetInInProjectGroupBaseLocale("zh-TW");
	//Execute(CreateProjectGroupOperation, TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);

	// LIST PROJECT GROUP LANGUAGES TEST
	//TSharedRef<FOneSkyListProjectGroupLanguagesOperation, ESPMode::ThreadSafe> ListProjectGroupLanguagesOperation = ILocalizationServiceOperation::Create<FOneSkyListProjectGroupLanguagesOperation>();
	//ListProjectGroupLanguagesOperation->SetInProjectGroupId(10222);
	//Execute(ListProjectGroupLanguagesOperation, TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);

	// LIST PROJECTS IN GROUP TEST
	//TSharedRef<FOneSkyListProjectsInGroupOperation, ESPMode::ThreadSafe> ListProjectsInGroupOperation = ILocalizationServiceOperation::Create<FOneSkyListProjectsInGroupOperation>();
	//ListProjectsInGroupOperation->SetInProjectGroupId(10222);
	//Execute(ListProjectsInGroupOperation, TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);

	// SHOW PROJECT test
	//TSharedRef<FOneSkyShowProjectOperation, ESPMode::ThreadSafe> ShowProjectOperation = ILocalizationServiceOperation::Create<FOneSkyShowProjectOperation>();
	//ShowProjectOperation->SetInProjectId(27922);
	//Execute(ShowProjectOperation, TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);

	// CREATE PROJECT test
	//TSharedRef<FOneSkyCreateProjectOperation, ESPMode::ThreadSafe> CreateProjectOperation = ILocalizationServiceOperation::Create<FOneSkyCreateProjectOperation>();
	//CreateProjectOperation->SetInProjectGroupId(10222);
	//CreateProjectOperation->SetInProjectGroupName("test create project");
	//CreateProjectOperation->SetInInProjectDescription("test project description");
	//CreateProjectOperation->SetInInProjectType("website");
	//Execute(CreateProjectOperation, TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);

	// LIST PROJECT LANGUAGES TEST
	//TSharedRef<FOneSkyListProjectLanguagesOperation, ESPMode::ThreadSafe> ListProjectLanguagesOperation = ILocalizationServiceOperation::Create<FOneSkyListProjectLanguagesOperation>();
	//ListProjectLanguagesOperation->SetInProjectId(27922);
	//Execute(ListProjectLanguagesOperation, TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);

	// TRANSLATION STATUS TEST
	//TSharedRef<FOneSkyTranslationStatusOperation, ESPMode::ThreadSafe> TranslationStatusOperation = ILocalizationServiceOperation::Create<FOneSkyTranslationStatusOperation>();
	//TranslationStatusOperation->SetInProjectId(27922);
	//TranslationStatusOperation->SetInFileName("en.po");
	//TranslationStatusOperation->SetInLocale("ko");
	//Execute(TranslationStatusOperation, TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);

	// TRANSLATION EXPORT TEST
	//TSharedRef<FOneSkyTranslationExportOperation, ESPMode::ThreadSafe> TranslationExportLanguagesOperation = ILocalizationServiceOperation::Create<FOneSkyTranslationExportOperation>();
	//TranslationExportLanguagesOperation->SetInProjectId(27922);
	//TranslationExportLanguagesOperation->SetInSourceFileName("en.po");
	//TranslationExportLanguagesOperation->SetInExportFileName("D:\\out-ko.po");
	//TranslationExportLanguagesOperation->SetInLocale("ko");
	//Execute(TranslationExportLanguagesOperation, TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);

	// LIST UPLOADED FILES
	//TSharedRef<FOneSkyListUploadedFilesOperation, ESPMode::ThreadSafe> ListUploadedFilesOperation = ILocalizationServiceOperation::Create<FOneSkyListUploadedFilesOperation>();
	//ListUploadedFilesOperation->SetInProjectId(27922);
	//ListUploadedFilesOperation->SetInItemsPerPage(1);
	//ListUploadedFilesOperation->SetInStartPage(1);
	//Execute(ListUploadedFilesOperation, TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);

	// UPLOAD FILE
	//TSharedRef<FOneSkyUploadFileOperation, ESPMode::ThreadSafe> UploadFileOperation = ILocalizationServiceOperation::Create<FOneSkyUploadFileOperation>();
	//UploadFileOperation->SetInProjectId(27922);
	//UploadFileOperation->SetInLocalFilePathAndName("D:\\zh_CN.po");
	//UploadFileOperation->SetInOneSkyTargetFileName("en.po");
	//UploadFileOperation->SetInLocale("zh_CN");
	//UploadFileOperation->SetInIsKeepingAllStrings(false);
	//Execute(UploadFileOperation, TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);

	// LIST UPLOADED FILES
	//TSharedRef<FOneSkyListPhraseCollectionsOperation, ESPMode::ThreadSafe> ListPhraseCollectionsOperation = ILocalizationServiceOperation::Create<FOneSkyListPhraseCollectionsOperation>();
	//ListPhraseCollectionsOperation->SetInProjectId(27922);
	//ListPhraseCollectionsOperation->SetInItemsPerPage(1);
	//ListPhraseCollectionsOperation->SetInPage(1);
	//Execute(ListPhraseCollectionsOperation, TArray<FLocalizationServiceTranslationIdentifier>(), InConcurrency);

	return ELocalizationServiceOperationCommandResult::Succeeded;
}

FText FOneSkyLocalizationServiceProvider::GetStatusText() const
{
	FOneSkyLocalizationServiceModule& OneSkyLocalizationService = FModuleManager::LoadModuleChecked<FOneSkyLocalizationServiceModule>( "OneSkyLocalizationService" );
	const FOneSkyLocalizationServiceSettings& Settings = OneSkyLocalizationService.AccessSettings();

	FFormatNamedArguments Args;
	Args.Add( TEXT("IsEnabled"), IsEnabled() ? LOCTEXT("Yes", "Yes") : LOCTEXT("No", "No") );
	Args.Add( TEXT("IsConnected"), (IsEnabled() && IsAvailable()) ? LOCTEXT("Yes", "Yes") : LOCTEXT("No", "No") );
	Args.Add( TEXT("ConnectionName"), FText::FromString( Settings.GetConnectionName() ) );

	return FText::Format( LOCTEXT("OneSkyStatusText", "Enabled: {IsEnabled}\nConnected: {IsConnected}\nConnectionName: {ConnectionName}\n"), Args );
}

bool FOneSkyLocalizationServiceProvider::IsEnabled() const
{
	return true;
}

bool FOneSkyLocalizationServiceProvider::IsAvailable() const
{
	return bServerAvailable;
}

bool FOneSkyLocalizationServiceProvider::EstablishPersistentConnection()
{
	FOneSkyLocalizationServiceModule& OneSkyLocalizationService = FModuleManager::LoadModuleChecked<FOneSkyLocalizationServiceModule>( "OneSkyLocalizationService" );
	FOneSkyConnectionInfo ConnectionInfo = OneSkyLocalizationService.AccessSettings().GetConnectionInfo();

	bool bIsValidConnection = false;
	if ( !PersistentConnection )
	{
		PersistentConnection = new FOneSkyConnection(ConnectionInfo);
	}

	bIsValidConnection = PersistentConnection->IsValidConnection();
	if ( !bIsValidConnection )
	{
		delete PersistentConnection;
		PersistentConnection = new FOneSkyConnection(ConnectionInfo);
		bIsValidConnection = PersistentConnection->IsValidConnection();
	}

	bServerAvailable = bIsValidConnection;
	return bIsValidConnection;
}

const FName& FOneSkyLocalizationServiceProvider::GetName() const
{
	return ProviderName;
}

const FText FOneSkyLocalizationServiceProvider::GetDisplayName() const
{
	return LOCTEXT("OneSkyLocalizationService", "OneSky Localization Service");
}

ELocalizationServiceOperationCommandResult::Type FOneSkyLocalizationServiceProvider::GetState(const TArray<FLocalizationServiceTranslationIdentifier>& InTranslationIds, TArray< TSharedRef<ILocalizationServiceState, ESPMode::ThreadSafe> >& OutState, ELocalizationServiceCacheUsage::Type InStateCacheUsage)
{
	if(!IsEnabled())
	{
		return ELocalizationServiceOperationCommandResult::Failed;
	}

	if (InStateCacheUsage == ELocalizationServiceCacheUsage::ForceUpdate)
	{
		// TODO: Something like this?
		//for (TArray<FText>::TConstIterator It(InTexts); It; It++)
		{
			//TSharedRef<FOneSkyShowPhraseCollectionWorker, ESPMode::ThreadSafe> PhraseCollectionWorker = ILocalizationServiceOperation::Create<FOneSkyShowPhraseCollectionWorker>();

			//PhraseCollectionWorker->InCollectionKey = FTextInspector::GetKey(*It);
			//Execute(ILocalizationServiceOperation::Create<FLocalizationServiceUpdateStatus>(), InTranslationIds);
		}
	}

	for (TArray<FLocalizationServiceTranslationIdentifier>::TConstIterator It(InTranslationIds); It; It++)
	{
		TSharedRef<FOneSkyLocalizationServiceState, ESPMode::ThreadSafe>* State = StateCache.Find(*It);
		if(State != NULL)
		{
			// found cached item for this file, return that
			OutState.Add(*State);
		}
		else
		{
			// cache an unknown state for this item & return that
			TSharedRef<FOneSkyLocalizationServiceState, ESPMode::ThreadSafe> NewState = MakeShareable(new FOneSkyLocalizationServiceState(*It));
			StateCache.Add(*It, NewState);
			OutState.Add(NewState);
		}
	}

	return ELocalizationServiceOperationCommandResult::Succeeded;
}

ELocalizationServiceOperationCommandResult::Type FOneSkyLocalizationServiceProvider::Execute(const TSharedRef<ILocalizationServiceOperation, ESPMode::ThreadSafe>& InOperation, const TArray<FLocalizationServiceTranslationIdentifier>& InTranslationIds, ELocalizationServiceOperationConcurrency::Type InConcurrency, const FLocalizationServiceOperationComplete& InOperationCompleteDelegate)
{
	if(!IsEnabled())
	{
		return ELocalizationServiceOperationCommandResult::Failed;
	}

	// Query to see if the we allow this operation
	TSharedPtr<IOneSkyLocalizationServiceWorker, ESPMode::ThreadSafe> Worker = CreateWorker(InOperation->GetName());
	if(!Worker.IsValid())
	{
		// this operation is unsupported by this source control provider
		FFormatNamedArguments Arguments;
		Arguments.Add( TEXT("OperationName"), FText::FromName(InOperation->GetName()) );
		Arguments.Add( TEXT("ProviderName"), FText::FromName(GetName()) );
		FMessageLog("LocalizationService").Error(FText::Format(LOCTEXT("UnsupportedOperation", "Operation '{OperationName}' not supported by source control provider '{ProviderName}'"), Arguments));
		return ELocalizationServiceOperationCommandResult::Failed;
	}

	// fire off operation
	if(InConcurrency == ELocalizationServiceOperationConcurrency::Synchronous)
	{
		FOneSkyLocalizationServiceCommand* Command = new FOneSkyLocalizationServiceCommand(InOperation, Worker.ToSharedRef());
		Command->bAutoDelete = false;
		Command->OperationCompleteDelegate = InOperationCompleteDelegate;
		return ExecuteSynchronousCommand(*Command, InOperation->GetInProgressString(), true);
	}
	else
	{
		FOneSkyLocalizationServiceCommand* Command = new FOneSkyLocalizationServiceCommand(InOperation, Worker.ToSharedRef());
		Command->bAutoDelete = true;
		Command->OperationCompleteDelegate = InOperationCompleteDelegate;
		return IssueCommand(*Command, false);
	}
}

bool FOneSkyLocalizationServiceProvider::CanCancelOperation( const TSharedRef<ILocalizationServiceOperation, ESPMode::ThreadSafe>& InOperation ) const
{
	for (int32 CommandIndex = 0; CommandIndex < CommandQueue.Num(); ++CommandIndex)
	{
		FOneSkyLocalizationServiceCommand& Command = *CommandQueue[CommandIndex];
		if (Command.Operation == InOperation)
		{
			check(Command.bAutoDelete);
			return true;
		}
	}

	// operation was not in progress!
	return false;
}

void FOneSkyLocalizationServiceProvider::CancelOperation( const TSharedRef<ILocalizationServiceOperation, ESPMode::ThreadSafe>& InOperation )
{
	for (int32 CommandIndex = 0; CommandIndex < CommandQueue.Num(); ++CommandIndex)
	{
		FOneSkyLocalizationServiceCommand& Command = *CommandQueue[CommandIndex];
		if (Command.Operation == InOperation)
		{
			check(Command.bAutoDelete);
			Command.Cancel();
			return;
		}
	}
}

void FOneSkyLocalizationServiceProvider::OutputCommandMessages(const FOneSkyLocalizationServiceCommand& InCommand) const
{
	FMessageLog LocalizationServiceLog("LocalizationService");

	for (int32 ErrorIndex = 0; ErrorIndex < InCommand.ErrorMessages.Num(); ++ErrorIndex)
	{
		LocalizationServiceLog.Error(InCommand.ErrorMessages[ErrorIndex]);
	}

	for (int32 InfoIndex = 0; InfoIndex < InCommand.InfoMessages.Num(); ++InfoIndex)
	{
		LocalizationServiceLog.Info(InCommand.InfoMessages[InfoIndex]);
	}
}

void FOneSkyLocalizationServiceProvider::Tick()
{	
	bool bStatesUpdated = false;
	for (int32 CommandIndex = 0; CommandIndex < CommandQueue.Num(); ++CommandIndex)
	{
		FOneSkyLocalizationServiceCommand& Command = *CommandQueue[CommandIndex];
		if (Command.bExecuteProcessed)
		{
			// Remove command from the queue
			CommandQueue.RemoveAt(CommandIndex);

			// update connection state
			bServerAvailable = !Command.bConnectionDropped || Command.bCancelled;

			// let command update the states of any files
			bStatesUpdated |= Command.Worker->UpdateStates();

			// dump any messages to output log
			OutputCommandMessages(Command);

			// run the completion delegate if we have one bound
			ELocalizationServiceOperationCommandResult::Type Result = ELocalizationServiceOperationCommandResult::Failed;
			if(Command.bCommandSuccessful)
			{
				Result = ELocalizationServiceOperationCommandResult::Succeeded;
			}
			else if(Command.bCancelled)
			{
				Result = ELocalizationServiceOperationCommandResult::Cancelled;
			}
			Command.OperationCompleteDelegate.ExecuteIfBound(Command.Operation, Result);

			//commands that are left in the array during a tick need to be deleted
			if(Command.bAutoDelete)
			{
				// Only delete commands that are not running 'synchronously'
				delete &Command;
			}

			// only do one command per tick loop, as we dont want concurrent modification 
			// of the command queue (which can happen in the completion delegate)
			break;
		}
	}

	//if(bStatesUpdated)
	//{
	//	OnLocalizationServiceStateChanged.Broadcast();
	//}
}

static void PublicKeyChanged(const FText& NewText, ETextCommit::Type CommitType)
{
	FOneSkyLocalizationServiceModule::Get().AccessSettings().SetApiKey(NewText.ToString());
	FOneSkyLocalizationServiceModule::Get().AccessSettings().SaveSettings();
}

static void SecretKeyChanged(const FText& NewText, ETextCommit::Type CommitType)
{
	FOneSkyLocalizationServiceModule::Get().AccessSettings().SetApiSecret(NewText.ToString());
	FOneSkyLocalizationServiceModule::Get().AccessSettings().SaveSettings();
}

static void SaveSecretKeyChanged(ECheckBoxState CheckBoxState)
{
	FOneSkyLocalizationServiceModule::Get().AccessSettings().SetSaveSecretKey(CheckBoxState == ECheckBoxState::Checked);
	FOneSkyLocalizationServiceModule::Get().AccessSettings().SaveSettings();
}

#if LOCALIZATION_SERVICES_WITH_SLATE
void FOneSkyLocalizationServiceProvider::CustomizeSettingsDetails(IDetailCategoryBuilder& DetailCategoryBuilder) const
{
	FOneSkyConnectionInfo ConnectionInfo = FOneSkyLocalizationServiceModule::Get().AccessSettings().GetConnectionInfo();
	FText PublicKeyText = LOCTEXT("OneSkyPublicKeyLabel", "OneSky API Public Key");
	FDetailWidgetRow& PublicKeyRow = DetailCategoryBuilder.AddCustomRow(PublicKeyText);
	PublicKeyRow.NameContent()
		[
			SNew(STextBlock)
			.Text(PublicKeyText)
		];
	PublicKeyRow.ValueContent()
		[
			SNew(SEditableTextBox)
			.OnTextCommitted(FOnTextCommitted::CreateStatic(&PublicKeyChanged))
			.Text(FText::FromString(ConnectionInfo.ApiKey))
		];

	FText SecretKeyText = LOCTEXT("OneSkySecretKeyLabel", "OneSky API Secret Key");
	FDetailWidgetRow& SecretKeyRow = DetailCategoryBuilder.AddCustomRow(SecretKeyText);
	SecretKeyRow.NameContent()
		[
			SNew(STextBlock)
			.Text(SecretKeyText)
		];
	SecretKeyRow.ValueContent()
		[
			SNew(SEditableTextBox)
			.OnTextCommitted(FOnTextCommitted::CreateStatic(&SecretKeyChanged))
			.Text(FText::FromString(ConnectionInfo.ApiSecret))
		];

	FText SaveSecretKeyText = LOCTEXT("OneSkySaveSecret", "Remember Secret Key (WARNING: saved unencrypted)");
	FDetailWidgetRow& SaveSecretKeyRow = DetailCategoryBuilder.AddCustomRow(SaveSecretKeyText);
	SaveSecretKeyRow.NameContent()
		[
			SNew(STextBlock)
			.Text(SaveSecretKeyText)
		];
	SaveSecretKeyRow.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked(FOneSkyLocalizationServiceModule::Get().AccessSettings().GetSaveSecretKey() ?  ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged(FOnCheckStateChanged::CreateStatic(&SaveSecretKeyChanged))
		];
}

static void ProjectGroupChanged(const FText& NewText, ETextCommit::Type CommitType, FGuid TargetGuid)
{
	FOneSkyLocalizationTargetSetting* Settings = FOneSkyLocalizationServiceModule::Get().AccessSettings().GetSettingsForTarget(TargetGuid);
	FOneSkyLocalizationServiceModule::Get().AccessSettings().SetSettingsForTarget(TargetGuid, NewText.ToString(), Settings->OneSkyProjectName, Settings->OneSkyFileName);
}

static void ProjectChanged(const FText& NewText, ETextCommit::Type CommitType, FGuid TargetGuid)
{
	FOneSkyLocalizationTargetSetting* Settings = FOneSkyLocalizationServiceModule::Get().AccessSettings().GetSettingsForTarget(TargetGuid);
	FOneSkyLocalizationServiceModule::Get().AccessSettings().SetSettingsForTarget(TargetGuid, Settings->OneSkyProjectGroupName, NewText.ToString(), Settings->OneSkyFileName);
}

static void FileNameChanged(const FText& NewText, ETextCommit::Type CommitType, FGuid TargetGuid)
{
	FOneSkyLocalizationTargetSetting* Settings = FOneSkyLocalizationServiceModule::Get().AccessSettings().GetSettingsForTarget(TargetGuid);
	FOneSkyLocalizationServiceModule::Get().AccessSettings().SetSettingsForTarget(TargetGuid, Settings->OneSkyProjectGroupName, Settings->OneSkyProjectName, NewText.ToString());
}

void FOneSkyLocalizationServiceProvider::CustomizeTargetDetails(IDetailCategoryBuilder& DetailCategoryBuilder, const FGuid& TargetGuid) const
{
	//Target.Settings.Guid.ToString();
	FOneSkyLocalizationTargetSetting* Settings = FOneSkyLocalizationServiceModule::Get().AccessSettings().GetSettingsForTarget(TargetGuid);

	FText ProjectGroupText = LOCTEXT("OneSkyProjectGroupLabel", "OneSky Project Group");
	FDetailWidgetRow& ProjectGroupRow = DetailCategoryBuilder.AddCustomRow(ProjectGroupText);
	ProjectGroupRow.NameContent()
		[
			SNew(STextBlock)
			.Text(ProjectGroupText)
		];
	ProjectGroupRow.ValueContent()
		[
			SNew(SEditableTextBox)
			.OnTextCommitted(FOnTextCommitted::CreateStatic(&ProjectGroupChanged, TargetGuid))
			.Text(FText::FromString(Settings->OneSkyProjectGroupName))
		];

	FText ProjectText = LOCTEXT("OneSkyProjectLabel", "OneSky Project");
	FDetailWidgetRow& ProjectRow = DetailCategoryBuilder.AddCustomRow(ProjectText);
	ProjectRow.NameContent()
		[
			SNew(STextBlock)
			.Text(ProjectText)
		];
	ProjectRow.ValueContent()
		[
			SNew(SEditableTextBox)
			.OnTextCommitted(FOnTextCommitted::CreateStatic(&ProjectChanged, TargetGuid))
			.Text(FText::FromString(Settings->OneSkyProjectName))
		];

	FText FileText = LOCTEXT("OneSkyFileNameLabel", "OneSky File Name");
	FDetailWidgetRow& FileNameRow = DetailCategoryBuilder.AddCustomRow(FileText);
	FileNameRow.NameContent()
		[
			SNew(STextBlock)
			.Text(FileText)
		];
	FileNameRow.ValueContent()
		[
			SNew(SEditableTextBox)
			.OnTextCommitted(FOnTextCommitted::CreateStatic(&FileNameChanged, TargetGuid))
			.Text(FText::FromString(Settings->OneSkyFileName))
		];
}
#endif //LOCALIZATION_SERVICES_WITH_SLATE

TSharedPtr<IOneSkyLocalizationServiceWorker, ESPMode::ThreadSafe> FOneSkyLocalizationServiceProvider::CreateWorker(const FName& InOperationName) const
{
	const FGetOneSkyLocalizationServiceWorker* Operation = WorkersMap.Find(InOperationName);
	if(Operation != NULL)
	{
		return Operation->Execute();
	}
		
	return NULL;
}

void FOneSkyLocalizationServiceProvider::RegisterWorker( const FName& InName, const FGetOneSkyLocalizationServiceWorker& InDelegate )
{
	WorkersMap.Add( InName, InDelegate );
}

ELocalizationServiceOperationCommandResult::Type FOneSkyLocalizationServiceProvider::ExecuteSynchronousCommand(FOneSkyLocalizationServiceCommand& InCommand, const FText& Task, bool bSuppressResponseMsg)
{
	ELocalizationServiceOperationCommandResult::Type Result = ELocalizationServiceOperationCommandResult::Failed;

	struct Local
	{
		static void CancelCommand(FOneSkyLocalizationServiceCommand* InControlCommand)
		{
			InControlCommand->Cancel();
		}
	};

	// Display the progress dialog
	//FScopedLocalizationServiceProgress Progress(Task, FSimpleDelegate::CreateStatic(&Local::CancelCommand, &InCommand));

	// Perform the command asynchronously
	IssueCommand( InCommand, false );

	// Wait until the queue is empty. Only at this point is our command guaranteed to be removed from the queue
	while(CommandQueue.Num() > 0)
	{
		// Tick the command queue and update progress.
		Tick();

		//Progress.Tick();

		// Sleep for a bit so we don't busy-wait so much.
		FPlatformProcess::Sleep(0.01f);
	}

	if(InCommand.bCommandSuccessful)
	{
		Result = ELocalizationServiceOperationCommandResult::Succeeded;
	}
	else if(InCommand.bCancelled)
	{
		Result = ELocalizationServiceOperationCommandResult::Cancelled;
	}

	// If the command failed, inform the user that they need to try again
	if (!InCommand.bCancelled && Result != ELocalizationServiceOperationCommandResult::Succeeded && !bSuppressResponseMsg)
	{
		FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("OneSky_ServerUnresponsive", "OneSky server is unresponsive. Please check your connection and try again.") );
	}

	// Delete the command now
	check(!InCommand.bAutoDelete);
	delete &InCommand;

	return Result;
}

ELocalizationServiceOperationCommandResult::Type FOneSkyLocalizationServiceProvider::IssueCommand(FOneSkyLocalizationServiceCommand& InCommand, const bool bSynchronous)
{
	if ( !bSynchronous && GThreadPool != NULL )
	{
		// Queue this to our worker thread(s) for resolving
		GThreadPool->AddQueuedWork(&InCommand);
		CommandQueue.Add(&InCommand);
		return ELocalizationServiceOperationCommandResult::Succeeded;
	}
	else
	{
		InCommand.bCommandSuccessful = InCommand.DoWork();

		InCommand.Worker->UpdateStates();

		OutputCommandMessages(InCommand);

		// Callback now if present. When asynchronous, this callback gets called from Tick().
		ELocalizationServiceOperationCommandResult::Type Result = InCommand.bCommandSuccessful ? ELocalizationServiceOperationCommandResult::Succeeded : ELocalizationServiceOperationCommandResult::Failed;
		InCommand.OperationCompleteDelegate.ExecuteIfBound(InCommand.Operation, Result);

		return Result;
	}
}


#undef LOCTEXT_NAMESPACE

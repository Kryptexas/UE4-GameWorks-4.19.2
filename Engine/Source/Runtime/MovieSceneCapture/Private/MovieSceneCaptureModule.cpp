// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCapturePCH.h"
#include "MovieSceneCapture.h"
#include "MovieSceneCaptureModule.h"
#include "SDockTab.h"
#include "JsonObjectConverter.h"
#include "INotificationWidget.h"
#include "SNotificationList.h"
#include "NotificationManager.h"

#include "SlateExtras.h"

#if WITH_EDITOR
	#include "EditorStyle.h"
	#include "Editor.h"
	#include "PropertyEditing.h"
#endif

#define LOCTEXT_NAMESPACE "MovieSceneCapture"

const FName MovieSceneRenderTabId(TEXT("MovieSceneRenderTab"));

DECLARE_DELEGATE_RetVal_OneParam(FText, FOnStartCapture, UMovieSceneCapture*);

#if WITH_EDITOR
	class SRenderMovieSceneSettings : public SCompoundWidget, public FGCObject
	{
		SLATE_BEGIN_ARGS(SRenderMovieSceneSettings) : _InitialObject(nullptr) {}
			SLATE_EVENT(FOnStartCapture, OnStartCapture)
			SLATE_ARGUMENT(UMovieSceneCapture*, InitialObject)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

			FDetailsViewArgs DetailsViewArgs;
			DetailsViewArgs.bUpdatesFromSelection = false;
			DetailsViewArgs.bLockable = false;
			DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
			DetailsViewArgs.ViewIdentifier = "RenderMovieScene";

			DetailView = PropertyEditor.CreateDetailView(DetailsViewArgs);

			OnStartCapture = InArgs._OnStartCapture;

			ChildSlot
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				[
					DetailView.ToSharedRef()
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(ErrorText, STextBlock)
					.Visibility(EVisibility::Hidden)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(5.f)
				[
					SNew(SButton)
					.ContentPadding(FMargin(10, 5))
					.Text(LOCTEXT("Export", "Capture Movie"))
					.OnClicked(this, &SRenderMovieSceneSettings::OnStartClicked)
				]
			
			];

			if (InArgs._InitialObject)
			{
				SetObject(InArgs._InitialObject);
			}
		}

		void SetObject(UMovieSceneCapture* InMovieSceneCapture)
		{
			MovieSceneCapture = InMovieSceneCapture;

			DetailView->SetObject(InMovieSceneCapture);

			ErrorText->SetText(FText());
			ErrorText->SetVisibility(EVisibility::Hidden);
		}

		virtual void AddReferencedObjects( FReferenceCollector& Collector ) override
		{
			Collector.AddReferencedObject(MovieSceneCapture);
		}

	private:

		FReply OnStartClicked()
		{
			FText Error;
			if (OnStartCapture.IsBound())
			{
				Error = OnStartCapture.Execute(MovieSceneCapture);
			}

			ErrorText->SetText(Error);
			ErrorText->SetVisibility(Error.IsEmpty() ? EVisibility::Hidden : EVisibility::Visible);

			return FReply::Handled();
		}

		TSharedPtr<IDetailsView> DetailView;
		TSharedPtr<STextBlock> ErrorText;
		FOnStartCapture OnStartCapture;
		UMovieSceneCapture* MovieSceneCapture;
	};
#endif


DECLARE_DELEGATE_OneParam(FOnProcessClosed, int32);

class SCaptureMovieNotification : public SCompoundWidget, public INotificationWidget
{
public:
	SLATE_BEGIN_ARGS(SCaptureMovieNotification){}
		SLATE_EVENT(FOnProcessClosed, OnProcessClosed)
		SLATE_ARGUMENT(FString, BrowseToFolder)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FProcHandle InProcHandle)
	{
		OnProcessClosed = InArgs._OnProcessClosed;
		ProcHandle = InProcHandle;

		FString BrowseToFolder = FPaths::ConvertRelativePathToFull(InArgs._BrowseToFolder);
		auto OnBrowseToFolder = [=]{
			FPlatformProcess::ExploreFolder(*BrowseToFolder);
		};

		ChildSlot
		[
			SNew(SBorder)
			.Padding(FMargin(15.0f))
			.BorderImage(FCoreStyle::Get().GetBrush("NotificationList.ItemBackground"))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.Padding(FMargin(0,0,0,5.0f))
				.HAlign(HAlign_Right)
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SAssignNew(TextBlock, STextBlock)
						.Font(FCoreStyle::Get().GetFontStyle(TEXT("NotificationList.FontBold")))
						.Text(LOCTEXT("RenderingVideo", "Capturing video"))
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(15.f,0,0,0))
					[
						SAssignNew(Throbber, SThrobber)
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SAssignNew(Hyperlink, SHyperlink)
						.Visibility(EVisibility::Collapsed)
						.Text(LOCTEXT("OpenFolder", "Open Capture Folder..."))
						.OnNavigate_Lambda(OnBrowseToFolder)
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SAssignNew(Button, SButton)
						.Text(LOCTEXT("StopButton", "Stop Capture"))
						.OnClicked(this, &SCaptureMovieNotification::ButtonClicked)
					]
				]
			]
		];
	}

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
	{
		if (State != SNotificationItem::CS_Pending)
		{
			return;
		}

		if (!ProcHandle.IsValid() || !FPlatformProcess::IsProcRunning(ProcHandle))
		{
			int32 RetCode = 0;
			FPlatformProcess::GetProcReturnCode(ProcHandle, &RetCode);
			OnProcessClosed.ExecuteIfBound(RetCode);

			if (RetCode == 0)
			{
				TextBlock->SetText(LOCTEXT("Finished", "Capture Finished"));
			}
		}
	}

	virtual void OnSetCompletionState(SNotificationItem::ECompletionState InState)
	{
		State = InState;
		if (State != SNotificationItem::CS_Pending)
		{
			Hyperlink->SetVisibility(EVisibility::Visible);
			Throbber->SetVisibility(EVisibility::Collapsed);
			Button->SetVisibility(EVisibility::Collapsed);
		}
	}

	virtual TSharedRef< SWidget > AsWidget()
	{
		return AsShared();
	}

private:

	FReply ButtonClicked()
	{
		if (State == SNotificationItem::CS_Pending)
		{
			FPlatformProcess::TerminateProc(ProcHandle);
		}
		return FReply::Handled();
	}
	
private:
	TSharedPtr<SWidget> Button, Throbber, Hyperlink;
	TSharedPtr<STextBlock> TextBlock;
	SNotificationItem::ECompletionState State;
	FOnProcessClosed OnProcessClosed;
	FProcHandle ProcHandle;
};

class FMovieSceneCaptureModule : public IMovieSceneCaptureModule, public FTickableGameObject, public FGCObject
{
public:

	void TickCapture(UMovieSceneCapture* Capture)
	{
		ExistingCaptures.Add(Capture);
	}

	void StopTickingCapture(UMovieSceneCapture* Capture)
	{
		ExistingCaptures.Remove(Capture);
	}

private:

	TArray<UMovieSceneCapture*> ExistingCaptures;

	/** Tickable interface */
	virtual bool IsTickableInEditor() const override { return false; }
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FCachePutAsyncWorker, STATGROUP_ThreadPoolAsyncTasks); }
	virtual void Tick( float DeltaTime ) override
	{
		for (auto* Obj : ExistingCaptures)
		{
			Obj->CaptureFrame(DeltaTime);
		}
	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		for (auto* Obj : ExistingCaptures)
		{
			Collector.AddReferencedObject(Obj);
		}
	}

	/** End Tickable interface */
	virtual void StartupModule() override
	{
		FCoreDelegates::OnPreExit.AddRaw(this, &FMovieSceneCaptureModule::PreExit);
	}

	void PreExit()
	{
		TArray<UMovieSceneCapture*> ExistingCapturesCopy;
		Swap(ExistingCaptures, ExistingCapturesCopy);

		for (auto* Obj : ExistingCapturesCopy)
		{
			Obj->Close();
		}
	}

	virtual void PreUnloadCallback() override
	{
		InProgressCaptureNotification = nullptr;
		FCoreDelegates::OnPreExit.RemoveAll(this);
		PreExit();
	}

	virtual IMovieSceneCaptureInterface* InitializeFromCommandLine() override
	{
		if (GIsEditor)
		{
			return nullptr;
		}

		FString ManifestPath;
		if (!FParse::Value(FCommandLine::Get(), TEXT("-MovieSceneCaptureManifest="), ManifestPath) || ManifestPath.IsEmpty())
		{
			return nullptr;
		}

		UMovieSceneCapture* Capture = nullptr;
		FString Json;
		if (FFileHelper::LoadFileToString(Json, *ManifestPath))
		{
			TSharedPtr<FJsonObject> RootObject;
			TSharedRef<TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(Json);
			if (FJsonSerializer::Deserialize(JsonReader, RootObject) && RootObject.IsValid())
			{
				auto TypeField = RootObject->TryGetField(TEXT("Type"));
				if (!TypeField.IsValid())
				{
					return nullptr;
				}

				UClass* Class = FindObject<UClass>(nullptr, *TypeField->AsString());
				if (!Class)
				{
					return nullptr;
				}

				Capture = NewObject<UMovieSceneCapture>(GetTransientPackage(), Class);
				if (!Capture)
				{
					return nullptr;
				}

				auto DataField = RootObject->TryGetField(TEXT("Data"));
				if (!DataField.IsValid())
				{
					return nullptr;
				}

				if (!FJsonObjectConverter::JsonAttributesToUStruct(DataField->AsObject()->Values, Class, Capture, 0, 0))
				{
					return nullptr;
				}

				TickCapture(Capture);
				return Capture;
			}
		}

		return nullptr;
	}

	virtual IMovieSceneCaptureInterface* CreateMovieSceneCapture(FViewport* Viewport) override
	{
		UMovieSceneCapture* Capture = NewObject<UMovieSceneCapture>(GetTransientPackage());
		TickCapture(Capture);
		Capture->Initialize(Viewport);
		Capture->StartCapture();
		return Capture;
	}

	virtual IMovieSceneCaptureInterface* RetrieveMovieSceneInterface(FMovieSceneCaptureHandle Handle)
	{
		auto** Existing = ExistingCaptures.FindByPredicate([&](const UMovieSceneCapture* In){ return In->GetHandle() == Handle; });
		return Existing ? *Existing : nullptr;
	}

	virtual void OnMovieSceneCaptureFinished(IMovieSceneCaptureInterface* Capture)
	{
		StopTickingCapture(static_cast<UMovieSceneCapture*>(Capture));
	}

	virtual void DestroyMovieSceneCapture(FMovieSceneCaptureHandle Handle)
	{
		// Calling Close() can remove itself from the array
		auto Captures = ExistingCaptures;
		for (auto* Existing : Captures)
		{
			if (Existing->GetHandle() == Handle)
			{
				Existing->Close();
			}
		}
	}

	virtual void OpenCaptureSettings(const TSharedRef<FTabManager>& TabManager, UMovieSceneCapture* CaptureObject) override
	{
#if WITH_EDITOR
		TSharedPtr<SWindow> ExistingWindow = CaptureSettingsWindow.Pin();
		if (ExistingWindow.IsValid())
		{
			ExistingWindow->BringToFront();
		}
		else
		{
			ExistingWindow = SNew(SWindow)
				.Title( LOCTEXT("RenderMovieSettingsTitle", "Render Movie Settings") )
				.HasCloseButton(true)
				.SupportsMaximize(false)
				.SupportsMinimize(false)
				.ClientSize(FVector2D(350, 650));

			TSharedPtr<SDockTab> OwnerTab = TabManager->GetOwnerTab();
			TSharedPtr<SWindow> RootWindow = OwnerTab.IsValid() ? OwnerTab->GetParentWindow() : TSharedPtr<SWindow>();
			if(RootWindow.IsValid())
			{
				FSlateApplication::Get().AddWindowAsNativeChild(ExistingWindow.ToSharedRef(), RootWindow.ToSharedRef());
			}
			else
			{
				FSlateApplication::Get().AddWindow(ExistingWindow.ToSharedRef());
			}
		}

		ExistingWindow->SetContent(
			SNew(SRenderMovieSceneSettings)
			.InitialObject(CaptureObject)
			.OnStartCapture_Raw(this, &FMovieSceneCaptureModule::OnStartCapture)
		);

		CaptureSettingsWindow = ExistingWindow;
#endif
	}

	void OnMovieCaptureProcessClosed(int32 RetCode)
	{
		if (RetCode == 0)
		{
			InProgressCaptureNotification->SetCompletionState(SNotificationItem::CS_Success);
		}
		else
		{
			// todo: error to message log
			InProgressCaptureNotification->SetCompletionState(SNotificationItem::CS_Fail);
		}

		InProgressCaptureNotification->ExpireAndFadeout();
		InProgressCaptureNotification = nullptr;
	}

	FText OnStartCapture(UMovieSceneCapture* CaptureObject)
	{
		if (InProgressCaptureNotification.IsValid())
		{
			return LOCTEXT("AlreadyCapturing", "There is already a movie scene capture process open. Please close it and try again.");
		}

		// If buffer visualization dumping is enabled, we need to tell capture process to enable it too
		static const auto CVarDumpFrames = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.BufferVisualizationDumpFrames"));
		if (CVarDumpFrames && CVarDumpFrames->GetValueOnGameThread())
		{
			CaptureObject->bBufferVisualizationDumpFrames = true;
		}

		// Save out the capture manifest to json
		FString Filename = FPaths::GameSavedDir() / TEXT("MovieSceneCapture/Manifest.json");

		TSharedRef<FJsonObject> Object = MakeShareable(new FJsonObject);
		if (FJsonObjectConverter::UStructToJsonObject(CaptureObject->GetClass(), CaptureObject, Object, 0, 0))
		{
			TSharedRef<FJsonObject> RootObject = MakeShareable(new FJsonObject);
			RootObject->SetField(TEXT("Type"), MakeShareable(new FJsonValueString(CaptureObject->GetClass()->GetPathName())));
			RootObject->SetField(TEXT("Data"), MakeShareable(new FJsonValueObject(Object)));

			FString Json;
			TSharedRef<TJsonWriter<> > JsonWriter = TJsonWriterFactory<>::Create(&Json, 0);
			if (FJsonSerializer::Serialize( RootObject, JsonWriter ))
			{
				FFileHelper::SaveStringToFile(Json, *Filename);
			}
		}
		else
		{
			return LOCTEXT("UnableToSaveCaptureManifest", "Unable to save capture manifest");
		}

		// this parameter tells UE4Editor to run in game mode
		FString EditorCommandLine = FString::Printf(TEXT("-MovieSceneCaptureManifest=\"%s\" -game"), *Filename);
		if (!CaptureObject->AdditionalCommandLineArguments.IsEmpty())
		{
			EditorCommandLine.AppendChar(' ');
			EditorCommandLine.Append(CaptureObject->AdditionalCommandLineArguments);
		}

		if (!CaptureObject->Settings.bEnableTextureStreaming)
		{
			EditorCommandLine.Append(TEXT(" -NoTextureStreaming"));
		}
		
		EditorCommandLine += FString::Printf(TEXT(" -ResX=%d"), CaptureObject->Settings.Resolution.ResX);
		EditorCommandLine += FString::Printf(TEXT(" -ResY=%d"), CaptureObject->Settings.Resolution.ResY);

		// PlayMap - in editor. Not working yet.

		// FMovieSceneCaptureModule& Module = static_cast<FMovieSceneCaptureModule&>(IMovieSceneCaptureModule::Get());
		// Module.TickCapture(CaptureObject);

		// UGameViewportClient::OnViewportCreated().AddLambda([=](){

		// 	for (const FWorldContext& Context : GEngine->GetWorldContexts())
		// 	{
		// 		if (Context.WorldType == EWorldType::PIE)
		// 		{
		// 			CaptureObject->Initialize(Context.GameViewport->Viewport);
		// 			break;
		// 		}
		// 	}

		// });

		// FEditorDelegates::EndPIE.AddLambda([&](bool bIsSimulating){
		// 	Module.StopTickingCapture(CaptureObject);
		// });

		// GEditor->PlayMap();

		CaptureObject->SaveConfig();

		FString Params;
		if (FPaths::IsProjectFilePathSet())
		{
			Params = FString::Printf(TEXT("\"%s\" %s %s"), *FPaths::GetProjectFilePath(), *EditorCommandLine, *FCommandLine::GetSubprocessCommandline());
		}
		else
		{
			Params = FString::Printf(TEXT("%s %s %s"), FApp::GetGameName(), *EditorCommandLine, *FCommandLine::GetSubprocessCommandline());
		}

		FString GamePath = FPlatformProcess::GenerateApplicationPath(FApp::GetName(), FApp::GetBuildConfiguration());
		FProcHandle ProcessHandle = FPlatformProcess::CreateProc(*GamePath, *Params, true, false, false, NULL, 0, NULL, NULL);

		// @todo: progress reporting, UI feedback
		if (ProcessHandle.IsValid())
		{
			FNotificationInfo Info(
				SNew(SCaptureMovieNotification, ProcessHandle)
				.BrowseToFolder(CaptureObject->Settings.OutputDirectory.Path)
				.OnProcessClosed_Raw(this, &FMovieSceneCaptureModule::OnMovieCaptureProcessClosed)
			);
			Info.bFireAndForget = false;
			Info.ExpireDuration = 5.f;
			InProgressCaptureNotification = FSlateNotificationManager::Get().AddNotification(Info);
			InProgressCaptureNotification->SetCompletionState(SNotificationItem::CS_Pending);
		}
		

		return FText();
	}
private:
	/** */
	TWeakPtr<SWindow> CaptureSettingsWindow;
	TSharedPtr<SNotificationItem> InProgressCaptureNotification;

};

IMPLEMENT_MODULE( FMovieSceneCaptureModule, MovieSceneCapture )

#undef LOCTEXT_NAMESPACE
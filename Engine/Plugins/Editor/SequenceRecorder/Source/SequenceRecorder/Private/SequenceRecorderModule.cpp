// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SequenceRecorderPrivatePCH.h"
#include "ModuleManager.h"
#include "SSequenceRecorder.h"
#include "SequenceRecorderStyle.h"
#include "SDockTab.h"
#include "SequenceRecorderCommands.h"
#include "SequenceRecorder.h"
#include "SequenceRecorderSettings.h"
#include "ISequenceRecorder.h"

#define LOCTEXT_NAMESPACE "SequenceRecorder"

static const FName SequenceRecorderTabName(TEXT("SequenceRecorder"));

class FSequenceRecorderModule : public ISequenceRecorder, private FSelfRegisteringExec
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		FSequenceRecorderStyle::Initialize();

		FSequenceRecorderCommands::Register();

		if (GEditor)
		{
			// register main tick
			PostEditorTickHandle = GEditor->OnPostEditorTick().AddStatic(&FSequenceRecorderModule::TickSequenceRecorder);
		}

		// register Persona recorder
		FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>(TEXT("Persona"));
		PersonaModule.OnIsRecordingActive().BindStatic(&FSequenceRecorderModule::HandlePersonaIsRecordingActive);
		PersonaModule.OnRecord().BindStatic(&FSequenceRecorderModule::HandlePersonaRecord);
		PersonaModule.OnStopRecording().BindStatic(&FSequenceRecorderModule::HandlePersonaStopRecording);
		PersonaModule.OnGetCurrentRecording().BindStatic(&FSequenceRecorderModule::HandlePersonaCurrentRecording);
		PersonaModule.OnGetCurrentRecordingTime().BindStatic(&FSequenceRecorderModule::HandlePersonaCurrentRecordingTime);
		PersonaModule.OnTickRecording().BindStatic(&FSequenceRecorderModule::HandlePersonaTickRecording);

		// register 'keep simulation changes' recorder
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		LevelEditorModule.OnCaptureSingleFrameAnimSequence().BindStatic(&FSequenceRecorderModule::HandleCaptureSingleFrameAnimSequence);

		// register level editor menu extender
		LevelEditorMenuExtenderDelegate = FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::CreateStatic(&FSequenceRecorderModule::ExtendLevelViewportContextMenu);
		auto& MenuExtenders = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();
		MenuExtenders.Add(LevelEditorMenuExtenderDelegate);
		LevelEditorExtenderDelegateHandle = MenuExtenders.Last().GetHandle();

		// register standalone UI
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SequenceRecorderTabName, FOnSpawnTab::CreateStatic(&FSequenceRecorderModule::SpawnSequenceRecorderTab))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
			.SetDisplayName(LOCTEXT("SequenceRecorderTabTitle", "Sequence Recorder"))
			.SetTooltipText(LOCTEXT("SequenceRecorderTooltipText", "Open the Sequence Recorder tab."))
			.SetIcon(FSlateIcon(FSequenceRecorderStyle::Get()->GetStyleSetName(), "SequenceRecorder.TabIcon"));
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		if (FSlateApplication::IsInitialized())
		{
			FGlobalTabmanager::Get()->UnregisterTabSpawner(SequenceRecorderTabName);
		}

		if(FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
		{
			FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
			LevelEditorModule.OnCaptureSingleFrameAnimSequence().Unbind();
			LevelEditorModule.GetAllLevelViewportContextMenuExtenders().RemoveAll([&](const FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors& Delegate) {
				return Delegate.GetHandle() == LevelEditorExtenderDelegateHandle;
			});
		}

		if (FModuleManager::Get().IsModuleLoaded(TEXT("Persona")))
		{
			FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>(TEXT("Persona"));
			PersonaModule.OnIsRecordingActive().Unbind();
			PersonaModule.OnRecord().Unbind();
			PersonaModule.OnStopRecording().Unbind();
			PersonaModule.OnGetCurrentRecording().Unbind();
			PersonaModule.OnGetCurrentRecordingTime().Unbind();
			PersonaModule.OnTickRecording().Unbind();
		}

		if (GEditor)
		{
			GEditor->OnPostEditorTick().Remove(PostEditorTickHandle);
		}

		FSequenceRecorderStyle::Shutdown();
#endif
	}

	// FSelfRegisteringExec implementation
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override
	{
#if WITH_EDITOR
		if (FParse::Command(&Cmd, TEXT("RecordAnimation")))
		{
			return HandleRecordAnimationCommand(InWorld, Cmd, Ar);
		}
		else if (FParse::Command(&Cmd, TEXT("StopRecordingAnimation")))
		{
			return HandleStopRecordAnimationCommand(InWorld, Cmd, Ar);
		}
#endif
		return false;
	}

	static bool HandleRecordAnimationCommand(UWorld* InWorld, const TCHAR* InStr, FOutputDevice& Ar)
	{
#if WITH_EDITOR
		const TCHAR* Str = InStr;
		// parse actor name
		TCHAR ActorName[128];
		AActor* FoundActor = nullptr;
		if (FParse::Token(Str, ActorName, ARRAY_COUNT(ActorName), 0))
		{
			FString const ActorNameStr = FString(ActorName);
			for (ULevel const* Level : InWorld->GetLevels())
			{
				if (Level)
				{
					for (AActor* Actor : Level->Actors)
					{
						if (Actor)
						{
							if (Actor->GetName() == ActorNameStr)
							{
								FoundActor = Actor;
								break;
							}
						}
					}
				}
			}
		}

		if (FoundActor)
		{
			USkeletalMeshComponent* const SkelComp = FoundActor->FindComponentByClass<USkeletalMeshComponent>();
			if (SkelComp)
			{
				TCHAR AssetPath[256];
				FParse::Token(Str, AssetPath, ARRAY_COUNT(AssetPath), 0);
				FString const AssetName = FPackageName::GetLongPackageAssetName(AssetPath);
				return FAnimationRecorderManager::Get().RecordAnimation(SkelComp, AssetPath, AssetName);
			}
		}
#endif
		return false;
	}

	static bool HandleStopRecordAnimationCommand(UWorld* InWorld, const TCHAR* InStr, FOutputDevice& Ar)
	{
#if WITH_EDITOR
		const TCHAR* Str = InStr;

		// parse actor name
		TCHAR ActorName[128];
		AActor* FoundActor = nullptr;
		bool bStopAll = false;
		if (FParse::Token(Str, ActorName, ARRAY_COUNT(ActorName), 0))
		{
			FString const ActorNameStr = FString(ActorName);

			if (ActorNameStr.ToLower() == TEXT("all"))
			{
				bStopAll = true;
			}
			else if (InWorld)
			{
				// search for the actor by name
				for (ULevel* Level : InWorld->GetLevels())
				{
					if (Level)
					{
						for (AActor* Actor : Level->Actors)
						{
							if (Actor)
							{
								if (Actor->GetName() == ActorNameStr)
								{
									FoundActor = Actor;
									break;
								}
							}
						}
					}
				}
			}
		}

		if (bStopAll)
		{
			FAnimationRecorderManager::Get().StopRecordingAllAnimations();
			return true;
		}
		else if (FoundActor)
		{
			USkeletalMeshComponent* const SkelComp = FoundActor->FindComponentByClass<USkeletalMeshComponent>();
			if (SkelComp)
			{
				FAnimationRecorderManager::Get().StopRecordingAnimation(SkelComp);
				return true;
			}
		}

#endif
		return false;
	}

	// ISequenceRecorder interface
	virtual bool StartRecording(UWorld* World, const FSequenceRecorderActorFilter& ActorFilter) override
	{
		return FSequenceRecorder::Get().StartRecordingForReplay(World, ActorFilter);
	}

	virtual void StopRecording() override
	{
		FSequenceRecorder::Get().StopRecording();
	}

	virtual bool IsRecording() override
	{
		return FSequenceRecorder::Get().IsRecording();
	}

	static void TickSequenceRecorder(float DeltaSeconds)
	{
		if (!IsRunningDedicatedServer() && !IsRunningCommandlet())
		{
			FSequenceRecorder::Get().Tick(DeltaSeconds);
		}
	}

	static UAnimSequence* HandleCaptureSingleFrameAnimSequence(USkeletalMeshComponent* Component)
	{
		FAnimationRecorder Recorder;
		if (Recorder.TriggerRecordAnimation(Component))
		{
			class UAnimSequence * Sequence = Recorder.GetAnimationObject();
			if (Sequence)
			{
				Recorder.StopRecord(false);
				return Sequence;
			}
		}

		return nullptr;
	}

	static void HandlePersonaIsRecordingActive(USkeletalMeshComponent* Component, bool& bIsRecording)
	{
		bIsRecording = FAnimationRecorderManager::Get().IsRecording(Component);
	}

	static void HandlePersonaRecord(USkeletalMeshComponent* Component)
	{
		FAnimationRecorderManager::Get().RecordAnimation(Component);
	}

	static void HandlePersonaStopRecording(USkeletalMeshComponent* Component)
	{
		FAnimationRecorderManager::Get().StopRecordingAnimation(Component);
	}

	static void HandlePersonaTickRecording(USkeletalMeshComponent* Component, float DeltaSeconds)
	{
		FAnimationRecorderManager::Get().Tick(Component, DeltaSeconds);
	}

	static void HandlePersonaCurrentRecording(USkeletalMeshComponent* Component, UAnimSequence*& OutSequence)
	{
		OutSequence = FAnimationRecorderManager::Get().GetCurrentlyRecordingSequence(Component);
	}

	static void HandlePersonaCurrentRecordingTime(USkeletalMeshComponent* Component, float& OutTime)
	{
		OutTime = FAnimationRecorderManager::Get().GetCurrentRecordingTime(Component);
	}

	static TSharedRef<SDockTab> SpawnSequenceRecorderTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		const TSharedRef<SDockTab> MajorTab =
			SNew(SDockTab)
			.Icon(FSequenceRecorderStyle::Get()->GetBrush("SequenceRecorder.TabIcon"))
			.TabRole(ETabRole::NomadTab);

		MajorTab->SetContent(SNew(SSequenceRecorder));

		return MajorTab;
	}

	static TSharedRef<FExtender> ExtendLevelViewportContextMenu(const TSharedRef<FUICommandList> CommandList, const TArray<AActor*> SelectedActors)
	{
		TSharedRef<FExtender> Extender(new FExtender());

		// check if actors are already registered for recording
		bool bCanAddRecording = false;
		bool bCanRemoveRecording = false;

		FSequenceRecorder& SequenceRecorder = FSequenceRecorder::Get();

		for (AActor* Actor : SelectedActors)
		{
			const bool bIsQueued = SequenceRecorder.IsRecordingQueued(Actor);
			bCanRemoveRecording |= bIsQueued;
			bCanAddRecording |= !bIsQueued;
		}

		if (bCanAddRecording || bCanRemoveRecording)
		{
			// Add the sequence recorder sub-menu extender
			Extender->AddMenuExtension(
				"ActorSelectVisibilityLevels",
				EExtensionHook::After,
				nullptr,
				FMenuExtensionDelegate::CreateStatic(&FSequenceRecorderModule::CreateLevelViewportContextMenuEntries, bCanAddRecording, bCanRemoveRecording));
		}

		return Extender;
	}

	static void CreateLevelViewportContextMenuEntries(FMenuBuilder& MenuBuilder, bool bCanAddRecording, bool bCanRemoveRecording)
	{
		MenuBuilder.BeginSection("SequenceRecording", LOCTEXT("SequenceRecordingLevelEditorHeading", "Sequence Recording"));

		if (bCanAddRecording)
		{
			FUIAction Action_AddQuickRecording(FExecuteAction::CreateStatic(&FSequenceRecorderModule::AddActorsForQuickRecording));

			MenuBuilder.AddMenuEntry(
				LOCTEXT("MenuExtensionAddQuickRecording", "Trigger Actor Recording"),
				LOCTEXT("MenuExtensionAddQuickRecording_Tooltip", "Set up the selected actors for recording and trigger recording immediately"),
				FSlateIcon(),
				Action_AddQuickRecording,
				NAME_None,
				EUserInterfaceActionType::Button);

			FUIAction Action_AddRecording(FExecuteAction::CreateStatic(&FSequenceRecorderModule::AddActorsForRecording));

			MenuBuilder.AddMenuEntry(
				LOCTEXT("MenuExtensionAddRecording", "Queue Actor Recording"),
				LOCTEXT("MenuExtensionAddRecording_Tooltip", "Queue up the selected actors for recording"),
				FSlateIcon(),
				Action_AddRecording,
				NAME_None,
				EUserInterfaceActionType::Button);
		}

		if (bCanRemoveRecording)
		{
			FUIAction Action_RemoveRecording(FExecuteAction::CreateStatic(&FSequenceRecorderModule::RemoveActorsForRecording));

			MenuBuilder.AddMenuEntry(
				LOCTEXT("MenuExtensionRemoveRecording", "Removed Queued Recording"),
				LOCTEXT("MenuExtensionRemoveRecording_Tooltip", "Remove the selected actors from the recording queue"),
				FSlateIcon(),
				Action_RemoveRecording,
				NAME_None,
				EUserInterfaceActionType::Button);
		}

		MenuBuilder.EndSection();
	}

	static void AddActorsForRecording()
	{
		FSequenceRecorder& SequenceRecorder = FSequenceRecorder::Get();

		TArray<AActor*> SelectedActors;
		GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);
		for(AActor* Actor : SelectedActors)
		{
			SequenceRecorder.AddNewQueuedRecording(Actor, nullptr, GetDefault<USequenceRecorderSettings>()->SequenceLength);
		}
	}

	static void AddActorsForQuickRecording()
	{
		AddActorsForRecording();

		FSequenceRecorder::Get().StartRecording();
	}

	static void RemoveActorsForRecording()
	{
		FSequenceRecorder& SequenceRecorder = FSequenceRecorder::Get();

		TArray<AActor*> SelectedActors;
		GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);
		for (AActor* Actor : SelectedActors)
		{
			SequenceRecorder.RemoveQueuedRecording(Actor);
		}
	}

	FDelegateHandle PostEditorTickHandle;

	FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors LevelEditorMenuExtenderDelegate;

	FDelegateHandle LevelEditorExtenderDelegateHandle;
};

IMPLEMENT_MODULE( FSequenceRecorderModule, SequenceRecorder )

#undef LOCTEXT_NAMESPACE
// Copyright 2017 Google Inc.


#include "TangoAreaLearningLibrary.h"
#include "TangoAreaLearning.h"
#include "LatentActions.h"
#include "RunnableThread.h"
#include "Runnable.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "TangoBlueprintFunctionLibrary.h"

#if PLATFORM_ANDROID
#include "tango_client_api_dynamic.h"
#include <cstdlib>
#endif

FTangoAreaDescriptionMetadata UTangoAreaLearningLibrary::CurrentTangoLocalAreaDescriptionMetadata;
FThreadSafeBool UTangoAreaLearningLibrary::bCurrentTangoLocalAreaDescriptionMetadataIsValid(false);
FThreadSafeBool UTangoAreaLearningLibrary::bTangoIsConnected(false);
FDelegateHandle UTangoAreaLearningLibrary::OnTangoConnectedHandle;

class FImportExportAction : public FPendingLatentAction
{
	bool& OutWasSuccessful;
	FString InUUID;
	FString Filename;
	FName ExecutionFunction;
	FDelegateHandle EventDelegateHandle;
	ETangoLocalAreaLearningEventType Type;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;
	int32 Result;
	bool bStarted;
	void HandleEvent(FTangoLocalAreaLearningEvent Event)
	{
		if (Event.EventType == Type)
		{
			Result = Event.EventValue;
		}
	}
public:
	virtual void UpdateOperation(FLatentResponse& Response) override
	{
#if PLATFORM_ANDROID
		if (!bStarted)
		{
			EventDelegateHandle = UTangoBlueprintFunctionLibrary::GetTangoLocalAreaLearningEventDelegate().AddRaw(this, &FImportExportAction::HandleEvent);
			bStarted = true;
			switch (Type)
			{
			case ETangoLocalAreaLearningEventType::IMPORT_RESULT:
				DoImport();
				break;
			case ETangoLocalAreaLearningEventType::EXPORT_RESULT:
				DoExport();
				break;
			}
		}
		if (Result >= 0)
		{
			UTangoBlueprintFunctionLibrary::GetTangoLocalAreaLearningEventDelegate().Remove(EventDelegateHandle);
			EventDelegateHandle.Reset();
			OutWasSuccessful = Result == 1;
			Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
		}
#else
		OutWasSuccessful = false;
		Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
#endif
	}

	FImportExportAction(
		ETangoLocalAreaLearningEventType InType,
		const FString &InFilename,
		const FString &InInUUID,
		bool& InWasSuccessful,
		const FLatentActionInfo& LatentInfo)
		: OutWasSuccessful(InWasSuccessful)
		, InUUID(InInUUID)
		, Filename(InFilename)
		, ExecutionFunction(LatentInfo.ExecutionFunction)
		, Type(InType)
		, OutputLink(LatentInfo.Linkage)
		, CallbackTarget(LatentInfo.CallbackTarget)
		, Result(-1)
		, bStarted(false)
	{
	}

	void DoExport();
	void DoImport();
};

class FSaveAreaDescriptionAction : public FPendingLatentAction
{
	float PercentDone;
	FECEF_Transform AreaDescriptionRealWorldPose;
	FTangoAreaDescriptionMetadata& Result;
	bool &bWasSuccessful;
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;
	FRunnableThread* Thread;
	bool bStarted;
	FDelegateHandle EventDelegateHandle;
	class MyRunner : public FRunnable
	{
		FSaveAreaDescriptionAction* Target;
	public:
		MyRunner(FSaveAreaDescriptionAction* InTarget) : Target(InTarget) {}
		virtual uint32 Run() override
		{
			Target->Run();
			return 0;
		}
	};
	MyRunner* Runner;
public:
	FSaveAreaDescriptionAction(
		FTangoAreaDescriptionMetadata& InResult,
		bool& InWasSuccessful,
		const FECEF_Transform& InRealWorldPose,
		const FLatentActionInfo& LatentInfo)
		: PercentDone(0.0f)
		, AreaDescriptionRealWorldPose(InRealWorldPose)
		, Result(InResult)
		, bWasSuccessful(InWasSuccessful)
		, ExecutionFunction(LatentInfo.ExecutionFunction)
		, OutputLink(LatentInfo.Linkage)
		, CallbackTarget(LatentInfo.CallbackTarget)
		, Thread(nullptr)
		, bStarted(false)
	{
	}

	void HandleEvent(FTangoLocalAreaLearningEvent Event)
	{
		if (Event.EventType == ETangoLocalAreaLearningEventType::SAVE_PROGRESS)
		{
			UpdatePercentDone(Event.EventValue / 100.0f);
		}
	}

	virtual ~FSaveAreaDescriptionAction()
	{
		Finish();
	}

	void Run();

	bool DoSave();

	void SetIsSuccessful(bool bValue)
	{
		bWasSuccessful = bValue;
	}

	void UpdatePercentDone(float NewValue)
	{
		UE_LOG(LogTangoAreaLearning, Log, TEXT("Save ADF PercentDone: %f"), NewValue);
		PercentDone = NewValue;
	}

	void Finish()
	{
		if (Thread != nullptr)
		{
			Thread->WaitForCompletion();
			delete Thread;
			Thread = nullptr;
		}
		if (Runner != nullptr)
		{
			delete Runner;
			Runner = nullptr;
		}
	}
	virtual void UpdateOperation(FLatentResponse& Response) override
	{
#if PLATFORM_ANDROID
		if (!bStarted)
		{
			EventDelegateHandle = UTangoBlueprintFunctionLibrary::GetTangoLocalAreaLearningEventDelegate().AddRaw(this, &FSaveAreaDescriptionAction::HandleEvent);
			bStarted = true;
			Runner = new MyRunner(this);
			Thread = FRunnableThread::Create(Runner, TEXT("SaveAreaDescription"));
		}
		if (PercentDone == 1.0)
		{
			UTangoBlueprintFunctionLibrary::GetTangoLocalAreaLearningEventDelegate().Remove(EventDelegateHandle);
			EventDelegateHandle.Reset();
			Finish();
		}
		Response.FinishAndTriggerIf(PercentDone == 1.0, ExecutionFunction, OutputLink, CallbackTarget);
#else
		bWasSuccessful = false;
		Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
#endif
	}

#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	virtual FString GetDescription() const override
	{
		return FString::Printf(*NSLOCTEXT("SaveAreaDescriptionAction",
			"PercentDone", "Saving (%.3f done)").ToString(), PercentDone);
	}
#endif
};

void UTangoAreaLearningLibrary::ExportLocalAreaDescription(
	UObject* WorldContextObject, struct FLatentActionInfo LatentInfo,
	const FString& AreaDescriptionID, const FString& Filename, bool& bWasSuccessful
)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.
			FindExistingAction<FImportExportAction>(
				LatentInfo.CallbackTarget,
				LatentInfo.UUID) == nullptr)
		{
			LatentActionManager.AddNewAction(
				LatentInfo.CallbackTarget, LatentInfo.UUID,
				new FImportExportAction(
					ETangoLocalAreaLearningEventType::EXPORT_RESULT,
					Filename,
					AreaDescriptionID,
					bWasSuccessful,
					LatentInfo));
		}
	}
}

void UTangoAreaLearningLibrary::ImportLocalAreaDescription(
	UObject* WorldContextObject, struct FLatentActionInfo LatentInfo,
	const FString& Filename, bool& bWasSuccessful
)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.
			FindExistingAction<FImportExportAction>(
				LatentInfo.CallbackTarget,
				LatentInfo.UUID) == nullptr)
		{
			LatentActionManager.AddNewAction(
				LatentInfo.CallbackTarget, LatentInfo.UUID,
				new FImportExportAction(
					ETangoLocalAreaLearningEventType::IMPORT_RESULT,
					Filename,
					FString(),
					bWasSuccessful,
					LatentInfo));
		}
	}
}

void UTangoAreaLearningLibrary::SaveCurrentLocalAreaDescription(
	UObject* WorldContextObject, FLatentActionInfo LatentInfo,
	const FECEF_Transform& ECEF_Transform, FTangoAreaDescriptionMetadata& Result, bool& bSuccessful)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.
			FindExistingAction<FSaveAreaDescriptionAction>(
				LatentInfo.CallbackTarget,
				LatentInfo.UUID) == NULL)
		{
			LatentActionManager.AddNewAction(
				LatentInfo.CallbackTarget, LatentInfo.UUID,
				new FSaveAreaDescriptionAction(
					Result, bSuccessful,
					ECEF_Transform,
					LatentInfo));
			return;
		}
		UE_LOG(LogTangoAreaLearning, Error, TEXT("SaveCurrentArea: Save already in progress"));
	}
	else
	{
		UE_LOG(LogTangoAreaLearning,
			Error, TEXT("SaveCurrentArea: Can't access world"));
	}
}

void FSaveAreaDescriptionAction::Run()
{
	bWasSuccessful = DoSave();
	PercentDone = 1.0;
}

#if PLATFORM_ANDROID
static bool GetAreaDescriptionMetadata(
	const FString& AreaDescriptionUUID, TangoUUID* UUID, TangoAreaDescriptionMetadata* Result)
{
	if (AreaDescriptionUUID.Len() > TANGO_UUID_LEN)
	{
		UE_LOG(LogTangoAreaLearning, Error, TEXT("Invalid Local Area Description ID: %s"), *AreaDescriptionUUID);
		return false;
	}
	char* Cstr = TCHAR_TO_UTF8(*AreaDescriptionUUID);
	strcpy(*UUID, Cstr);
	if (Result != nullptr)
	{
		if (TangoService_getAreaDescriptionMetadata_dynamic(*UUID, Result) != TANGO_SUCCESS)
		{
			UE_LOG(LogTangoAreaLearning, Error, TEXT("getAreaDescriptionMetadata failed for : %s"), *AreaDescriptionUUID);
			return false;
		}
	}
	return true;
}
#endif

void FImportExportAction::DoExport()
{
	UTangoBlueprintFunctionLibrary::RequestExportLocalAreaDescription(InUUID, Filename);
}

void FImportExportAction::DoImport()
{
	UTangoBlueprintFunctionLibrary::RequestImportLocalAreaDescription(Filename);
}

bool FSaveAreaDescriptionAction::DoSave()
{
#if PLATFORM_ANDROID
	TangoUUID UUID;
	if (TangoService_saveAreaDescription_dynamic(&UUID) != TANGO_SUCCESS)
	{
		UE_LOG(LogTangoAreaLearning, Error, TEXT("SaveCurrentArea: Save was not successful"));
		return false;
	}
	TangoAreaDescriptionMetadata Metadata;
	if (TangoService_getAreaDescriptionMetadata_dynamic(UUID, &Metadata) != TANGO_SUCCESS)
	{
		UE_LOG(LogTangoAreaLearning, Error, TEXT("SaveCurrentArea: failed to get metadata"));
		return false;
	}
	double Transform[7];
	for (int32 i = 0; i < 3; i++)
	{
		Transform[i] = AreaDescriptionRealWorldPose.Position[i];
	}
	for (int32 i = 0; i < 4; i++)
	{
		Transform[3 + i] = AreaDescriptionRealWorldPose.Orientation[i];
	}
	bool Succeeded = true;
	if (TangoAreaDescriptionMetadata_set_dynamic(Metadata, "transformation", sizeof Transform, (const char*)Transform) != TANGO_SUCCESS
		|| TangoService_saveAreaDescriptionMetadata_dynamic(UUID, Metadata) != TANGO_SUCCESS)
	{
		UE_LOG(LogTangoAreaLearning, Error, TEXT(" SaveCurrentArea: Save occurred but metadata could not be set: %s"), *FString(UUID));
		Succeeded = false;
	}
	else
	{
		Result.ID = FString(UUID);
		Result.ECEF_Transform = AreaDescriptionRealWorldPose;
	}
	TangoAreaDescriptionMetadata_free_dynamic(Metadata);
	return Succeeded;
#else
	return false;
#endif
}

void UTangoAreaLearningLibrary::GetCurrentLocalAreaDescriptionMetadata(
	FTangoAreaDescriptionMetadata& Result,
	bool& bWasSuccessful)
{
	bWasSuccessful = bCurrentTangoLocalAreaDescriptionMetadataIsValid;
	if (bWasSuccessful)
	{
		Result = CurrentTangoLocalAreaDescriptionMetadata;
		return;
	}
	FTangoConfiguration Config;
	if (!UTangoBlueprintFunctionLibrary::GetCurrentTangoConfig(Config))
	{
		return;
	}
	if (Config.LocalAreaDescriptionID.Len() == 0)
	{
		return;
	}
	GetLocalAreaDescriptionMetadata(Config.LocalAreaDescriptionID, Result, bWasSuccessful);
	if (bWasSuccessful)
	{
		CurrentTangoLocalAreaDescriptionMetadata = Result;
		bCurrentTangoLocalAreaDescriptionMetadataIsValid.AtomicSet(true);
		if (!OnTangoConnectedHandle.IsValid())
		{
			OnTangoConnectedHandle = UTangoBlueprintFunctionLibrary::GetTangoConnectionEventDelegate().
				AddStatic(&UTangoAreaLearningLibrary::OnTangoConnect);
		}
	}
}

void UTangoAreaLearningLibrary::GetLocalAreaDescriptionMetadata(
	const FString& LocalAreaDescriptionID,
	FTangoAreaDescriptionMetadata& Result,
	bool& bWasSuccessful)
{
	bWasSuccessful = false;
	if (LocalAreaDescriptionID.Len() == 0)
	{
		return;
	}
#if PLATFORM_ANDROID
	TangoAreaDescriptionMetadata Metadata;
	TangoUUID UUID;
	if (!GetAreaDescriptionMetadata(LocalAreaDescriptionID, &UUID, &Metadata))
	{
		return;
	}
	Result.ID = LocalAreaDescriptionID;
	double* Transformation;
	size_t Size;
	if (TangoAreaDescriptionMetadata_get_dynamic(
		Metadata, "transformation",
		&Size, (char**)&Transformation
	) == TANGO_SUCCESS)
	{
		for (int32 i = 0; i < 3; i++)
		{
			Result.ECEF_Transform.Position[i] = Transformation[i];
		}
		for (int32 i = 0; i < 4; i++)
		{
			Result.ECEF_Transform.Orientation[i] = Transformation[3 + i];
		}
	}
	else
	{
		UE_LOG(LogTangoAreaLearning, Warning, TEXT("TangoAreaDescriptionMetadata_get failed for transformation %s"), *LocalAreaDescriptionID);
		for (int32 i = 0; i < 3; i++)
		{
			Result.ECEF_Transform.Position[i] = 0;
			Result.ECEF_Transform.Orientation[i] = 0;
		}
		Result.ECEF_Transform.Orientation[3] = 1;
	}
	TangoAreaDescriptionMetadata_free_dynamic(Metadata);
	bWasSuccessful = true;
#endif
}

void UTangoAreaLearningLibrary::DeleteLocalAreaDescription(
	const FString& LocalAreaDescriptionID,
	bool& bWasSuccessful)
{
	bWasSuccessful = false;
#if PLATFORM_ANDROID
	TangoUUID UUID;
	if (!GetAreaDescriptionMetadata(LocalAreaDescriptionID, &UUID, nullptr))
	{
		return;
	}
	bWasSuccessful = TangoService_deleteAreaDescription_dynamic(UUID) == TANGO_SUCCESS;
	if (!bWasSuccessful)
	{
		UE_LOG(LogTangoAreaLearning, Error, TEXT("deleteAreaDescription failed for %s"), *LocalAreaDescriptionID);
	}
#endif
}

void UTangoAreaLearningLibrary::ListLocalAreaDescriptionMetadata(
	TArray<FTangoAreaDescriptionMetadata>& Areas,
	bool& bWasSuccessful)
{
	bWasSuccessful = false;
#if PLATFORM_ANDROID
	char* uuid_list = nullptr;
	if (TangoService_getAreaDescriptionUUIDList_dynamic(&uuid_list) == TANGO_SUCCESS)
	{
		TArray<FString> IDs;
		FString(uuid_list).ParseIntoArray(IDs, TEXT(","), true);
		Areas.Reset();
		for (int32 i = 0; i < IDs.Num(); i++)
		{
			FTangoAreaDescriptionMetadata Area;
			bool bSucceeded;
			GetLocalAreaDescriptionMetadata(IDs[i], Area, bSucceeded);
			if (bSucceeded)
			{
				Areas.Add(Area);
			}
		}
		bWasSuccessful = true;
	}
	else
	{
		UE_LOG(LogTangoAreaLearning, Error, TEXT("getAreaDescriptionUUIDList failed"));
	}
#endif
}

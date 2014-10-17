// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "VisualLog.h"
#include "VisualLogger/VisualLogger.h"

#if ENABLE_VISUAL_LOG 

DEFINE_LOG_CATEGORY(LogVisual);

TMap<class UObject*, TArray<TWeakObjectPtr<const class UObject> > > FVisualLogger::RedirectionMap;

FVisualLogger::FVisualLogger()
{
	BlockAllCategories(false);
	AddDevice(&FVisualLog::GetStatic());
	SetIsRecording(GEngine ? !!GEngine->bEnableVisualLogRecordingOnStart : false);
	SetIsRecordingOnServer(false);

	if (FParse::Param(FCommandLine::Get(), TEXT("EnableAILogging")))
	{
		SetIsRecording(true);
		SetIsRecordingToFile(true);
	}
}

FVisualLogger::~FVisualLogger()
{
	RemoveDevice(&FVisualLog::GetStatic());
}

void FVisualLogger::Shutdown()
{
	SetIsRecording(false);
	SetIsRecordingToFile(false);
}

void FVisualLogger::Cleanup(bool bReleaseMemory)
{
	const bool WasRecordingToFile = IsRecordingToFile();
	if (WasRecordingToFile)
	{
		SetIsRecordingToFile(false);
	}

	auto& OutputDevices = FVisualLogger::Get().OutputDevices;
	for (auto* Device : OutputDevices)
	{
		Device->Cleanup(bReleaseMemory);
	}

	RedirectionMap.Reset();
	LastUniqueIds.Reset();
	CurrentEntryPerObject.Reset();

	if (WasRecordingToFile)
	{
		SetIsRecordingToFile(true);
	}
}

int32 FVisualLogger::GetUniqueId(float Timestamp)
{
	return LastUniqueIds.FindOrAdd(Timestamp)++;
}

void FVisualLogger::Redirect(class UObject* FromObject, class UObject* ToObject)
{
	if (FromObject == ToObject)
	{
		return;
	}

	UObject* OldRedirection = FindRedirection(FromObject);
	UObject* NewRedierection = FindRedirection(ToObject);

	auto OldArray = RedirectionMap.Find(OldRedirection);
	if (OldArray)
	{
		OldArray->RemoveSingleSwap(FromObject);
	}

	RedirectionMap.FindOrAdd(NewRedierection).AddUnique(FromObject);

	UE_CVLOG(FromObject != NULL, FromObject, LogVisual, Log, TEXT("Redirected '%s' to '%s'"), *FromObject->GetName(), *NewRedierection->GetName());
}

class UObject* FVisualLogger::FindRedirection(const class UObject* Object)
{
	if (RedirectionMap.Contains(Object) == false)
	{
		for (auto& Redirection : RedirectionMap)
		{
			if (Redirection.Value.Find(Object) != INDEX_NONE)
			{
				return FindRedirection(Redirection.Key);
			}
		}
	}

	return const_cast<class UObject*>(Object);
}

void FVisualLogger::SetIsRecording(bool InIsRecording) 
{ 
	if (IsRecordingToFile())
	{
		SetIsRecordingToFile(false);
	}
	bIsRecording = InIsRecording; 
};

void FVisualLogger::SetIsRecordingToFile(bool InIsRecording)
{
	if (!bIsRecording && InIsRecording)
	{
		SetIsRecording(true);
	}

	FString BaseFileName = TEXT("VisualLog");
	FString UserDefinedName;
	FString MapName = GWorld.GetReference() ? GWorld->GetMapName() : TEXT("");
	if (LogFileNameGetter.IsBound())
	{
		UserDefinedName = LogFileNameGetter.Execute();
	}
	if (UserDefinedName.Len() > 0)
	{
		if (MapName.Len() > 0)
		{
			BaseFileName = FString::Printf(TEXT("%s_%s"), *UserDefinedName, *MapName);
		}
		else
		{
			BaseFileName = UserDefinedName;
		}
	}

	if (bIsRecordingToFile && !InIsRecording)
	{
		for (auto* Device : OutputDevices)
		{
			if (Device->HasFlags(VisualLogger::CanSaveToFile))
			{
				Device->StopRecordingToFile(GWorld.GetReference() ? GWorld->TimeSeconds : StartRecordingToFileTime);
			}
		}
	}
	else if (!bIsRecordingToFile && InIsRecording)
	{
		StartRecordingToFileTime = GWorld.GetReference() ? GWorld->TimeSeconds : 0;
		for (auto* Device : OutputDevices)
		{
			if (Device->HasFlags(VisualLogger::CanSaveToFile))
			{
				Device->SetFileName(BaseFileName);
				Device->StartRecordingToFile(StartRecordingToFileTime);
			}
		}
	}

	bIsRecordingToFile = InIsRecording;
}

#endif //ENABLE_VISUAL_LOG
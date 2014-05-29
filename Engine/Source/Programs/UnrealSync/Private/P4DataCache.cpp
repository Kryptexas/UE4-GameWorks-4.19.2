// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealSync.h"

#include "P4DataCache.h"

#include "UATHelper.h"

bool FP4DataCache::LoadFromLog(const FString& UnrealSyncListLog)
{
	const FString Prefix = "UnrealSyncList.Print: ";

	FString Log = UnrealSyncListLog;
	FString CurrentLine;
	FString Rest;

	TArray<FString> LabelDescs;

	while (Log.Split("\n", &CurrentLine, &Rest, ESearchCase::CaseSensitive))
	{
		if (CurrentLine.StartsWith(Prefix))
		{
			LabelDescs.Add(CurrentLine.Mid(Prefix.Len()).TrimTrailing());
		}

		Log = Rest;
	}

	for (auto LabelDesc : LabelDescs)
	{
		FString Name;
		FString TicksStr;

		if (!LabelDesc.Split(" ", &Name, &TicksStr, ESearchCase::CaseSensitive))
		{
			return false;
		}

		int64 Ticks = FCString::Atoi64(*TicksStr);

		Labels.Add(FP4Label(Name, Ticks));
	}

	Labels.Sort(
		[](const FP4Label& LabelA, const FP4Label& LabelB)
		{
			return LabelA.GetDate() > LabelB.GetDate();
		});

	return true;
}

const TArray<FP4Label>& FP4DataCache::GetLabels()
{
	return Labels;
}

FP4Label::FP4Label(const FString& Name, const FDateTime& Date)
	: Name(Name), Date(Date)
{

}

const FString& FP4Label::GetName() const
{
	return Name;
}

const FDateTime& FP4Label::GetDate() const
{
	return Date;
}

FP4DataLoader::FP4DataLoader(FOnLoadingFinished OnLoadingFinished)
	: OnLoadingFinished(OnLoadingFinished), bTerminate(false)
{
	Thread = FRunnableThread::Create(this, TEXT("P4 Data Loading"));
}

uint32 FP4DataLoader::Run()
{
	TSharedPtr<FP4DataCache> Data = MakeShareable(new FP4DataCache());

	class UATProgress
	{
	public:
		UATProgress(const bool& bTerminate)
			: bTerminate(bTerminate)
		{
		}

		bool OnProgress(const FString& Input)
		{
			Output += Input;

			return !bTerminate;
		}

		FString Output;

	private:
		const bool &bTerminate;
	};

	UATProgress Progress(bTerminate);
	if (!RunUATProgress("UnrealSyncList -type=all -ticks",
		FOnUATMadeProgress::CreateRaw(&Progress, &UATProgress::OnProgress)))
	{
		OnLoadingFinished.ExecuteIfBound(nullptr);
		return 0;
	}

	if (!Data->LoadFromLog(Progress.Output))
	{
		OnLoadingFinished.ExecuteIfBound(nullptr);
	}

	OnLoadingFinished.ExecuteIfBound(Data);

	return 0;
}

void FP4DataLoader::Terminate()
{
	bTerminate = true;
	Thread->WaitForCompletion();
}

FP4DataLoader::~FP4DataLoader()
{
	Terminate();
}

bool FP4DataLoader::IsInProgress() const
{
	return bInProgress;
}

void FP4DataLoader::Exit()
{
	bInProgress = false;
}

bool FP4DataLoader::Init()
{
	bInProgress = true;

	return true;
}

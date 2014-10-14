// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#include "VisualLog.h"
#include "VisualLogger/VisualLogger.h"
#include "VisualLogger/VisualLoggerAutomationTests.h"

UVisualLoggerAutomationTests::UVisualLoggerAutomationTests(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

}


#if ENABLE_VISUAL_LOG

class FVisualLoggerTestDevice : public FVisualLogDevice
{
public:
	FVisualLoggerTestDevice();
	virtual void Cleanup(bool bReleaseMemory = false) override;
	virtual void StartRecordingToFile(float TImeStamp) override;
	virtual void StopRecordingToFile(float TImeStamp) override;
	virtual void SetFileName(const FString& InFileName) override;
	virtual void Serialize(const class UObject* LogOwner, const FVisualLogEntry& LogEntry) override;

	class UObject* LastObject;
	FVisualLogEntry LastEntry;
};

FVisualLoggerTestDevice::FVisualLoggerTestDevice()
{
	Cleanup();
}

void FVisualLoggerTestDevice::Cleanup(bool bReleaseMemory)
{
	LastObject = NULL;
	LastEntry.Reset();
}

void FVisualLoggerTestDevice::StartRecordingToFile(float TImeStamp)
{

}

void FVisualLoggerTestDevice::StopRecordingToFile(float TImeStamp)
{

}

void FVisualLoggerTestDevice::SetFileName(const FString& InFileName)
{

}

void FVisualLoggerTestDevice::Serialize(const class UObject* LogOwner, const FVisualLogEntry& LogEntry)
{
	LastObject = const_cast<class UObject*>(LogOwner);
	LastEntry = LogEntry;
}

#define CHECK_SUCCESS(__Test__) \
if (!(__Test__)) \
{ \
	TestTrue(FString::Printf( TEXT("%s (%s:%d)"), TEXT(#__Test__), TEXT(__FILE__), __LINE__ ), __Test__); \
	return false; \
}
#define CHECK_FAIL(__Test__) \
if ((__Test__)) \
{ \
	TestFalse(FString::Printf( TEXT("%s (%s:%d)"), TEXT(#__Test__), TEXT(__FILE__), __LINE__ ), __Test__); \
	return false; \
}

struct FTestDeviceContext
{
	FTestDeviceContext() 
	{
		Device.Cleanup();
		FVisualLogger::Get().SetIsRecording(false);
		FVisualLogger::Get().Cleanup();
		FVisualLogger::Get().AddDevice(&Device);
	}

	~FTestDeviceContext() 
	{
		FVisualLogger::Get().SetIsRecording(false);
		FVisualLogger::Get().RemoveDevice(&Device);
		FVisualLogger::Get().Cleanup();
		Device.Cleanup();
	}

	FVisualLoggerTestDevice Device;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVisualLogTest, "Engine.VisualLogger.Logging simple text", EAutomationTestFlags::ATF_Game | EAutomationTestFlags::ATF_Editor)
/**
*
*
* @param Parameters - Unused for this test
* @return	TRUE if the test was successful, FALSE otherwise
*/

bool FVisualLogTest::RunTest(const FString& Parameters)
{
	FTestDeviceContext Context;

	FVisualLogger::Get().SetIsRecording(false);
	CHECK_FAIL(FVisualLogger::Get().IsRecording());

	UE_VLOG(GWorld, LogVisual, Log, TEXT("Hello World"));
	CHECK_SUCCESS(Context.Device.LastObject == NULL);
	CHECK_SUCCESS(Context.Device.LastEntry.TimeStamp == -1);

	FVisualLogger::Get().SetIsRecording(true);
	CHECK_SUCCESS(FVisualLogger::Get().IsRecording());

	{
		const FString TextToLog = TEXT("Hello World");
		UE_VLOG(GWorld, LogVisual, Log, *TextToLog);
		CHECK_SUCCESS(Context.Device.LastObject != GWorld);
		CHECK_SUCCESS(Context.Device.LastEntry.TimeStamp == -1);
		FVisualLogEntry* CurrentEntry = FVisualLogger::Get().GetEntryToWrite(GWorld, GWorld->TimeSeconds, VisualLogger::DontCreate);
		CHECK_SUCCESS(CurrentEntry != NULL);
		CHECK_SUCCESS(CurrentEntry->TimeStamp == GWorld->TimeSeconds);
		CHECK_SUCCESS(CurrentEntry->LogLines.Num() == 1);
		CHECK_SUCCESS(CurrentEntry->LogLines[0].Category == LogVisual.GetCategoryName());
		CHECK_SUCCESS(CurrentEntry->LogLines[0].Line == TextToLog);

		FVisualLogger::Get().GetEntryToWrite(GWorld, GWorld->TimeSeconds+0.1); //generate new entry and serialize old one
		CHECK_SUCCESS(Context.Device.LastEntry.TimeStamp == GWorld->TimeSeconds);
		CHECK_SUCCESS(Context.Device.LastObject == GWorld);
		CHECK_SUCCESS(Context.Device.LastEntry.LogLines.Num() == 1);
		CHECK_SUCCESS(Context.Device.LastEntry.LogLines[0].Category == LogVisual.GetCategoryName());
		CHECK_SUCCESS(Context.Device.LastEntry.LogLines[0].Line == TextToLog);
	}

	FVisualLogger::Get().SetIsRecording(false);
	CHECK_FAIL(FVisualLogger::Get().IsRecording());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVisualLogSegmentsTest, "Engine.VisualLogger.Logging segment shape", EAutomationTestFlags::ATF_Game | EAutomationTestFlags::ATF_Editor)
bool FVisualLogSegmentsTest::RunTest(const FString& Parameters)
{
	FTestDeviceContext Context;

	FVisualLogger::Get().SetIsRecording(false);
	CHECK_FAIL(FVisualLogger::Get().IsRecording());

	UE_VLOG_SEGMENT(GWorld, LogVisual, Log, FVector(0, 0, 0), FVector(1, 0, 0), FColor::Red, TEXT("Simple segment log"));
	CHECK_SUCCESS(Context.Device.LastObject == NULL);
	CHECK_SUCCESS(Context.Device.LastEntry.TimeStamp == -1);

	FVisualLogger::Get().SetIsRecording(true);
	CHECK_SUCCESS(FVisualLogger::Get().IsRecording());
	{
		const FVector StartPoint(0, 0, 0);
		const FVector EndPoint(1, 0, 0);
		UE_VLOG_SEGMENT(GWorld, LogVisual, Log, StartPoint, EndPoint, FColor::Red, TEXT("Simple segment log"));
		CHECK_SUCCESS(Context.Device.LastObject == NULL);
		CHECK_SUCCESS(Context.Device.LastEntry.TimeStamp == -1);
		FVisualLogEntry* CurrentEntry = FVisualLogger::Get().GetEntryToWrite(GWorld, GWorld->TimeSeconds, VisualLogger::DontCreate);
		CHECK_SUCCESS(CurrentEntry != NULL);
		CHECK_SUCCESS(CurrentEntry->TimeStamp == GWorld->TimeSeconds);
		CHECK_SUCCESS(CurrentEntry->ElementsToDraw.Num() == 1);
		CHECK_SUCCESS(CurrentEntry->ElementsToDraw[0].GetType() == FVisualLogEntry::FElementToDraw::Segment);
		CHECK_SUCCESS(CurrentEntry->ElementsToDraw[0].Points.Num() == 2);
		CHECK_SUCCESS(CurrentEntry->ElementsToDraw[0].Points[0] == StartPoint);
		CHECK_SUCCESS(CurrentEntry->ElementsToDraw[0].Points[1] == EndPoint);
	}
	return true;
}

#endif //ENABLE_VISUAL_LOG
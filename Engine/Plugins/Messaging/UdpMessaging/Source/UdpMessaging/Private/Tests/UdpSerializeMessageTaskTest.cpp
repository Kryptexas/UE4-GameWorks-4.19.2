// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "HAL/Event.h"
#include "Misc/AutomationTest.h"
#include "Async/TaskGraphInterfaces.h"
#include "Transport/UdpSerializedMessage.h"
#include "IMessageContext.h"
#include "Transport/UdpSerializeMessageTask.h"
#include "Tests/UdpMessagingTestTypes.h"



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUdpSerializeMessageTaskTest, "System.Core.Messaging.Transports.Udp.UdpSerializedMessage", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)


namespace UdpSerializeMessageTaskTest
{
	const FTimespan MaxWaitTime(0, 0, 10);
}


bool FUdpSerializeMessageTaskTest::RunTest(const FString& Parameters)
{
	using namespace UdpSerializeMessageTaskTest;

	const auto Context = MakeShared<FUdpMockMessageContext, ESPMode::ThreadSafe>(new FUdpMockMessage);

	// synchronous reference serialization
	const auto Message1 = MakeShared<FUdpSerializedMessage, ESPMode::ThreadSafe>();

	FUdpSerializeMessageTask Task1(Context, Message1, nullptr);
	{
		Task1.DoTask(FTaskGraphInterface::Get().GetCurrentThreadIfKnown(), FGraphEventRef());
	}

	// asynchronous serialization
	TSharedRef<FEvent, ESPMode::ThreadSafe> CompletionEvent = MakeShareable(FPlatformProcess::GetSynchEventFromPool(), [](FEvent* EventToDelete)
	{
		FPlatformProcess::ReturnSynchEventToPool(EventToDelete);
	});

	TSharedRef<FUdpSerializedMessage, ESPMode::ThreadSafe> Message2 = MakeShareable(new FUdpSerializedMessage);
	TGraphTask<FUdpSerializeMessageTask>::CreateTask().ConstructAndDispatchWhenReady(Context, Message2, CompletionEvent);

	const bool Completed = CompletionEvent->Wait(MaxWaitTime);

	const TArray<uint8>& DataArray1 = Message1->GetDataArray();
	const TArray<uint8>& DataArray2 = Message2->GetDataArray();

	const void* Data1 = DataArray1.GetData();
	const void* Data2 = DataArray2.GetData();

	const bool Equal = (FMemory::Memcmp(Data1, Data2, DataArray1.Num()) == 0);

	TestEqual(TEXT("Synchronous message serialization must succeed"), Message1->GetState(), EUdpSerializedMessageState::Complete);
	TestTrue(TEXT("Asynchronous message serialization must complete"), Completed);
	TestEqual(TEXT("Asynchronous message serialization must succeed"), Message2->GetState(), EUdpSerializedMessageState::Complete);
	TestTrue(TEXT("Synchronous and asynchronous message serialization must yield same results"), Equal);

	return (Completed && Equal);
}

void EmptyLinkFunctionForStaticInitializationUdpSerializeMessageTaskTest()
{
	// This function exists to prevent the object file containing this test from being excluded by the linker, because it has no publically referenced symbols.
}

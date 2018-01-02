// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "VoicePacketBuffer.h"

#define DEBUG_SORTING 0
#define DEBUG_POPPING 0
#define DEBUG_SKIPSILENCE 1

FVoicePacketBuffer::FVoicePacketBuffer(int32 BufferSize, int32 InNumSamplesUntilIdling, uint64 InStartSample)
	: ListHead(nullptr)
	, SampleCounter(InStartSample)
	, IdleSampleCounter(0)
	, NumSamplesUntilIdling(InNumSamplesUntilIdling)
{
	PacketBuffer.AddDefaulted(BufferSize);
	for (int32 Index = 0; Index < BufferSize; Index++)
	{
		PacketBuffer[Index].IndexInPacketBuffer = Index;
		FreedIndicesQueue.Enqueue(Index);
	}
}

int32 FVoicePacketBuffer::PopAudio(float* DestBuffer, uint32 RequestedSamples)
{
#if DEBUG_POPPING
	UE_LOG(LogTemp, Log, TEXT("Beginning to pop %d samples..."), RequestedSamples);
#endif

	uint32 DestBufferSampleIndex = 0;
	// start churning through packets.
	// TODO: Worry about threading issues here.
	while (DestBufferSampleIndex < RequestedSamples)
	{
#if DEBUG_POPPING
		UE_LOG(LogTemp, Log, TEXT("  Loop start, index %d "), DestBufferSampleIndex);
#endif
		//If we don't have any data, output silence and short circuit.
		if (ListHead == nullptr)
		{
			FMemory::Memset(&(DestBuffer[DestBufferSampleIndex]), 0, (RequestedSamples - DestBufferSampleIndex) * sizeof(float));
			IdleSampleCounter += RequestedSamples;
			DestBufferSampleIndex += RequestedSamples;
			if (IdleSampleCounter > NumSamplesUntilIdling)
			{
				UE_LOG(LogTemp, Warning, TEXT("Voip listener now idle."));
				bIsIdle = true;
			}

			return 0;
		}
		// If our current packet is more than 0 samples later than our sample counter,
		// fill silence until we get to the sample index.
		else if (ListHead->StartSample > SampleCounter)
		{
#if DEBUG_SKIPSILENCE
			SampleCounter = ListHead->StartSample;
#else
			const int32 SampleOffset = ListHead->StartSample - SampleCounter;
			const int32 NumSilentSamples = FMath::Min(SampleOffset, (int32) (RequestedSamples - DestBufferSampleIndex));
#if DEBUG_POPPING
			UE_LOG(LogTemp, Log, TEXT("    Sample counter %d behind next packet. Injecting %d samples of silence."), SampleOffset, NumSilentSamples);
#endif
			FMemory::Memset(&(DestBuffer[DestBufferSampleIndex]), 0, NumSilentSamples * sizeof(float));
			DestBufferSampleIndex += NumSilentSamples;
			SampleCounter += NumSilentSamples;
			IdleSampleCounter += NumSilentSamples;
			if (IdleSampleCounter > NumSamplesUntilIdling)
			{
				UE_LOG(LogTemp, Warning, TEXT("Voip listener now idle."));
				bIsIdle = true;
			}
#endif
		}
		// If we have less samples in the current packet than we need to churn through,
		// copy over the rest of the samples in this packet and move on to the next packet.
		else if (ListHead->SamplesLeft <=  (int32) (RequestedSamples - DestBufferSampleIndex))
		{
			bIsIdle = false;
			const uint32 NumSamplesLeftInPacketBuffer = ListHead->SamplesLeft;
			const int32 PacketSampleIndex = ListHead->BufferNumSamples - NumSamplesLeftInPacketBuffer;
			const float* PacketBufferPtr = &(ListHead->AudioBuffer[PacketSampleIndex]);
			FMemory::Memcpy(&(DestBuffer[DestBufferSampleIndex]), PacketBufferPtr, NumSamplesLeftInPacketBuffer * sizeof(float));
			SampleCounter += NumSamplesLeftInPacketBuffer;
			DestBufferSampleIndex += NumSamplesLeftInPacketBuffer;
#if DEBUG_POPPING
			check((ListHead->SamplesLeft - NumSamplesLeftInPacketBuffer) == 0);
			UE_LOG(LogTemp, Log, TEXT("    Copied %d samples from packet and reached end of packet."), NumSamplesLeftInPacketBuffer);
#endif
			//Delete this packet and move on to the next one.
			FreedIndicesQueue.Enqueue(ListHead->IndexInPacketBuffer);
			ListHead = ListHead->NextPacket;
		}
		// If we have more samples in the current packet than we need to churn through,
		// copy over samples and update the state in the packet node.
		else
		{
			int32 SamplesLeft = RequestedSamples - DestBufferSampleIndex;
			const int32 PacketSampleIndex = ListHead->BufferNumSamples - ListHead->SamplesLeft;
			const float* PacketBufferPtr = &(ListHead->AudioBuffer[PacketSampleIndex]);
			FMemory::Memcpy(&(DestBuffer[DestBufferSampleIndex]), PacketBufferPtr, SamplesLeft * sizeof(float));
#if DEBUG_POPPING
			check(DestBufferSampleIndex + SamplesLeft == RequestedSamples);
			UE_LOG(LogTemp, Log, TEXT("    Copied %d samples from packet and reached end of buffer."), SamplesLeft);
#endif
			ListHead->SamplesLeft -= SamplesLeft;
			SampleCounter += SamplesLeft;
			DestBufferSampleIndex += SamplesLeft;
			IdleSampleCounter = 0;
			bIsIdle = false;
		}
#if DEBUG_POPPING
		UE_LOG(LogTemp, Log, TEXT("  Ended loop with %d samples popped to buffer."), DestBufferSampleIndex);
#endif
	}
#if DEBUG_POPPING
	check(DestBufferSampleIndex == RequestedSamples);
	UE_LOG(LogTemp, Log, TEXT("Finished PopAudio."));
#endif
	return RequestedSamples - IdleSampleCounter;
}

void FVoicePacketBuffer::Reset(int32 InStartSample)
{
	FreedIndicesQueue.Empty();
	for (int32 Index = 0; Index < PacketBuffer.Num(); Index++)
	{
		PacketBuffer[Index].NextPacket = nullptr;
		FreedIndicesQueue.Enqueue(Index);
	}
	ListHead = nullptr;
	IdleSampleCounter = 0;
	bIsIdle = false;
	SampleCounter = InStartSample;

}

void FVoicePacketBuffer::PushPacket(const void* InBuffer, int32 NumBytes, uint64 InStartSample, EVoipStreamDataFormat Format)
{
	//To do- trash buffers that are too far behind our start sample.
	if (SampleCounter > InStartSample + NumBytes)
	{
		SampleCounter = InStartSample;
	}

	int32 Index = INDEX_NONE;

	// Add Packet to buffer. If we have any used up packets, overwrite them.
	// Otherwise, add a new packet to the end of the buffer, if we haven't filled our preallocated buffer.
	if (FreedIndicesQueue.Dequeue(Index))
	{
		PacketBuffer[Index].Initialize(InBuffer, NumBytes, InStartSample, Format);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Voice packet buffer filled to capacity of %d packets; packet dropped."), PacketBuffer.Num());
		return;
	}

	check(Index != INDEX_NONE);

	// The packet needs to know where it is in the buffer, so it can enqueue it's index
	// so that it can be overwritten after it's used.
	PacketBuffer[Index].IndexInPacketBuffer = Index;
	FSortedVoicePacketNode* PacketNodePtr = &(PacketBuffer[Index]);

	// If our buffer is empty, we can just place this packet at the beginning.
	// Otherwise, we need to traverse our sorted list and insert the packet at
	// the correct place based on StartSample.
	// TODO: Worry about threading issues here.
	if (ListHead == nullptr)
	{
		ListHead = PacketNodePtr;
		SampleCounter = InStartSample;
	}
	// If the packet coming in is earlier than the current packet,
	// insert this packet at the beginning.
	else if (InStartSample < ListHead->StartSample)
	{
		PacketNodePtr->NextPacket = ListHead;
		ListHead = PacketNodePtr;
	}
	// Otherwise, we have to scan through our packet list to insert this packet.
	else
	{
		FSortedVoicePacketNode* CurrentPacket = ListHead;
		FSortedVoicePacketNode* NextPacket = CurrentPacket->NextPacket;

		//Scan through list and insert this packet in the correct place.
		while (NextPacket != nullptr)
		{
			if (NextPacket->StartSample > InStartSample)
			{
				// The packet after this comes after this packet. Insert this packet.
				CurrentPacket->NextPacket = PacketNodePtr;
				CurrentPacket->NextPacket->NextPacket = NextPacket;
				return;
			}
			else
			{
				// Step through to the next packet.
				CurrentPacket = NextPacket;
				NextPacket = NextPacket->NextPacket;
			}
		}

		// If we've made it to this point, it means the pushed packet is the last packet chronologically.
		// Thus, we insert this packet at the end of the list.
		CurrentPacket->NextPacket = PacketNodePtr;
	}

#if DEBUG_SORTING
	UE_LOG(LogTemp, Log, TEXT("Packet sort order: "));
	FSortedVoicePacketNode* PacketPtr = ListHead;
	int32 LoggedPatcketIndex = 0;
	uint64 CachedSampleStart = 0;
	while (PacketPtr != nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("    Packet %d: Sample Start %d"), ++LoggedPatcketIndex, PacketPtr->StartSample);
		check(PacketPtr->StartSample > CachedSampleStart);
		CachedSampleStart = PacketPtr->StartSample;
		PacketPtr = PacketPtr->NextPacket;
	}
	if (LoggedPatcketIndex > 6)
	{
		//Put a breakpoint here to check out sample counts.
		UE_LOG(LogTemp, Log, TEXT("Several Packets buffered."));
	}
	UE_LOG(LogTemp, Log, TEXT("End of packet list."));
#endif
}
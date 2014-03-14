// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UdpMessageSegmenter.h: Declares the FUdpMessageSegmenter class.
=============================================================================*/

#pragma once


/**
 * Implements a message segmenter.
 *
 * This class breaks up a message into smaller sized segments that fit into UDP datagrams.
 * It also tracks the segments that still need to be sent.
 */
class FUdpMessageSegmenter
{
public:

	/**
	 * Default constructor.
	 */
	FUdpMessageSegmenter( )
		: MessageReader(NULL)
	{ }

	/**
	 * Creates and initializes a new message segmenter.
	 *
	 * @param InMessage - The message to segment.
	 */
	FUdpMessageSegmenter(const IMessageDataRef& InMessage, uint16 InSegmentSize)
		: Message(InMessage)
		, MessageReader(NULL)
		, SegmentSize(InSegmentSize)
	{ }

	/**
	 * Destructor.
	 */
	~FUdpMessageSegmenter( )
	{
		if (MessageReader != NULL)
		{
			delete MessageReader;
		}
	}

public:

	/**
	 * Gets the total size of the message in bytes.
	 *
	 * @return Message size.
	 */
	int64 GetMessageSize() const
	{
		if (MessageReader == NULL)
		{
			return 0;
		}

		return MessageReader->TotalSize();
	}

	/**
	 * Gets the next pending segment.
	 *
	 * @param OutData - Will hold the segment data.
	 * @param OutSegment - Will hold the segment number.
	 *
	 * @return true if a segment was returned, false if there are no more pending segments.
	 */
	bool GetNextPendingSegment(TArray<uint8>& OutData, uint16& OutSegment) const
	{
		if (MessageReader == NULL)
		{
			return false;
		}

		for (TConstSetBitIterator<> It(PendingSegments); It; ++It)
		{
			OutSegment = It.GetIndex();

			uint32 SegmentOffset = OutSegment * SegmentSize;
			int32 ActualSegmentSize = MessageReader->TotalSize() - SegmentOffset;

			if (ActualSegmentSize > SegmentSize)
			{
				ActualSegmentSize = SegmentSize;
			}

			OutData.Reset(ActualSegmentSize);
			OutData.AddUninitialized(ActualSegmentSize);

			MessageReader->Seek(SegmentOffset);
			MessageReader->Serialize(OutData.GetTypedData(), ActualSegmentSize);

			//FMemory::Memcpy(OutData.GetTypedData(), Message->GetTypedData() + SegmentOffset, ActualSegmentSize);

			return true;
		}

		return false;
	}

	/**
	 * Gets the number of segments that haven't been received yet.
	 *
	 * @return Number of pending segments.
	 */
	uint16 GetPendingSegmentsCount() const
	{
		return PendingSegmentsCount;
	}

	/**
	 * Gets the total number of segments that make up the message.
	 *
	 * @return Segment count.
	 */
	uint16 GetSegmentCount() const
	{
		return PendingSegments.Num();
	}

	/**
	 * Initializes the segmenter.
	 */
	void Initialize()
	{
		if (MessageReader != NULL)
		{
			return;
		}

		if (Message->GetState() == EMessageDataState::Complete)
		{
			MessageReader = Message->CreateReader();
			PendingSegmentsCount = (MessageReader->TotalSize() + SegmentSize - 1) / SegmentSize;
			PendingSegments.Init(true, PendingSegmentsCount);
		}
	}

	/**
	 * Checks whether all segments have been sent.
	 *
	 * @return true if all segments were sent, false otherwise.
	 */
	bool IsComplete() const
	{
		return (PendingSegmentsCount == 0);
	}

	/**
	 * Checks whether this segmenter has been initialized.
	 *
	 * @return true if it is initialized, false otherwise.
	 */
	bool IsInitialized() const
	{
		return (MessageReader != NULL);
	}

	/**
	 * Checks whether this segmenter is invalid.
	 *
	 * @return true if the segmenter is invalid, false otherwise.
	 */
	bool IsInvalid() const
	{
		return (Message->GetState() == EMessageDataState::Invalid);
	}

	/**
	 * Marks the specified segment as sent.
	 *
	 * @param Segment - The sent segment.
	 */
	void MarkAsSent(uint16 Segment)
	{
		if (Segment < PendingSegments.Num())
		{
			PendingSegments[Segment] = false;
			--PendingSegmentsCount;
		}
	}

	/**
	 * Marks the entire message for retransmission.
	 */
	void MarkForRetransmission()
	{
		PendingSegments.Init(true, PendingSegments.Num());
	}

	/**
	 * Marks the specified segments for retransmission.
	 *
	 * @param Segments - The data segments to retransmit.
	 */
	void MarkForRetransmission(const TArray<uint16>& Segments)
	{
		for (int32 Index = 0; Index < Segments.Num(); ++Index)
		{
			uint16 Segment = Segments[Index];

			if (Segment < PendingSegments.Num())
			{
				PendingSegments[Segment] = true;
			}
		}
	}

private:

	// Holds the message.
	IMessageDataPtr Message;

	// temp hack to support new transport API
	FArchive* MessageReader;

	// Holds an array of bits that indicate which segments still need to be sent.
	TBitArray<> PendingSegments;

	// Holds the number of segments that haven't been sent yet.
	uint16 PendingSegmentsCount;

	// Holds the segment size.
	uint16 SegmentSize;
};

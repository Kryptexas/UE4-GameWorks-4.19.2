// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "VisualLogger/VisualLogger.h"

#if ENABLE_VISUAL_LOG

class FVisualLoggerCircularBufferDevice : public FVisualLogDevice
{
public:
	static FVisualLoggerCircularBufferDevice& Get()
	{
		static FVisualLoggerCircularBufferDevice GDevice;
		return GDevice;
	}

	FVisualLoggerCircularBufferDevice();
	virtual void Cleanup(bool bReleaseMemory = false) override;
	virtual void Serialize(const class UObject* LogOwner, FName OwnerName, FName OwnerClassName, const FVisualLogEntry& LogEntry) override;
	virtual void GetRecordedLogs(TArray<FVisualLogEntryItem>& RecordedLogs) const override { RecordedLogs = FrameCache; }
	virtual bool HasFlags(int32 InFlags) const override { return !!(InFlags & (EVisualLoggerDeviceFlags::NoFlags)); }

	void DumpBuffer();
	int32 GetCircularBufferSize() { return CircularBufferSize; }
	void SetCircularBufferSize(int32 BufferSize) { CircularBufferSize = BufferSize; }

protected:
	int32 bUseCompression : 1;
	float FrameCacheLenght;
	int32 CircularBufferSize;
	TArray<FVisualLogEntryItem> FrameCache;
};
#endif

// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "IcmpPrivatePCH.h"
#include "Icmp.h"
#include "SocketSubsystem.h"
#include "IpAddress.h"

FIcmpEchoResult IcmpEchoImpl(const FString& TargetAddress, float Timeout);

#if !PLATFORM_SUPPORTS_ICMP
FIcmpEchoResult IcmpEchoImpl(const FString& TargetAddress, float Timeout)
{
	FIcmpEchoResult Result;
	Result.Status = EIcmpResponseStatus::NotImplemented;
	return Result;
}
#endif

// Calculate one's complement checksum
int CalculateChecksum(uint8* Address, int Length)
{
	uint16* Paired = reinterpret_cast<uint16*>(Address);
	int Sum = 0;

	while (Length > 1)
	{
		Sum += *Paired++;
		Length -= 2;
	}

	if (Length == 1)
	{
		// Add the last odd byte
		Sum += *reinterpret_cast<uint8*>(Paired);
	}

	// Carry over overflow back to the LSB
	Sum = (Sum >> 16) + (Sum & 0xFFFF);
	// And in case the overflow caused another overflow, add it back again
	Sum += (Sum >> 16);

	return ~Sum;
}

bool ResolveIp(const FString& HostName, FString& OutIp)
{
	ISocketSubsystem* SocketSub = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (SocketSub)
	{
		TSharedRef<FInternetAddr> HostAddr = SocketSub->CreateInternetAddr();
		ESocketErrors HostResolveError = SocketSub->GetHostByName(TCHAR_TO_ANSI(*HostName), *HostAddr);
		if (HostResolveError == SE_NO_ERROR || HostResolveError == SE_EWOULDBLOCK)
		{
			OutIp = HostAddr->ToString(false);
			return true;
		}
	}

	return false;
}

class FIcmpAsyncResult
	: public FTickerObjectBase
{
public:

	FIcmpAsyncResult(const FString& TargetAddress, float Timeout, FIcmpEchoResultCallback InCallback)
		: FTickerObjectBase(0)
		, Callback(InCallback)
		, bThreadCompleted(false)
	{
		TFunction<FIcmpEchoResult()> Task = [this, TargetAddress, Timeout]()
		{
			auto Result = IcmpEchoImpl(TargetAddress, Timeout);
			bThreadCompleted = true;
			return Result;
		};
		FutureResult = Async(EAsyncExecution::ThreadPool, Task);
	}

	virtual ~FIcmpAsyncResult()
	{
		check(IsInGameThread());
		FutureResult.Wait();
	}

private:
	virtual bool Tick(float DeltaTime) override
	{
		if (bThreadCompleted)
		{
			Callback(FutureResult.Get());
			delete this;
			return false;
		}
		return true;
	}

	/** Callback when the icmp result returns */
	FIcmpEchoResultCallback Callback;
	/** Thread task complete */
	FThreadSafeBool bThreadCompleted;
	/** Async result future */
	TFuture<FIcmpEchoResult> FutureResult;
};

void FIcmp::IcmpEcho(const FString& TargetAddress, float Timeout, FIcmpEchoResultCallback HandleResult)
{
	new FIcmpAsyncResult(TargetAddress, Timeout, HandleResult);
}
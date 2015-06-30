// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PerfCounters.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

#define JSON_ARRAY_NAME					TEXT("PerfCounters")
#define JSON_PERFCOUNTER_NAME			TEXT("Name")
#define JSON_PERFCOUNTER_SIZE_IN_BYTES	TEXT("SizeInBytes")

FPerfCounters::FPerfCounters(const FString& InUniqueInstanceId)
: UniqueInstanceId(InUniqueInstanceId)
, Socket(nullptr)
{
}

FPerfCounters::~FPerfCounters()
{
	if (Socket)
	{
		ISocketSubsystem* SocketSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		if (SocketSystem)
		{
			SocketSystem->DestroySocket(Socket);
		}
		Socket = nullptr;
	}
}

bool FPerfCounters::Initialize()
{
	// get the requested port from the command line (if specified)
	int32 StatsPort = -1;
	FParse::Value(FCommandLine::Get(), TEXT("statsPort="), StatsPort);
	if (StatsPort < 0)
	{
		UE_LOG(LogPerfCounters, Log, TEXT("FPerfCounters JSON socket disabled."));
		return true;
	}

	// get the socket subsystem
	ISocketSubsystem* SocketSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (SocketSystem == nullptr)
	{
		UE_LOG(LogPerfCounters, Error, TEXT("FPerfCounters unable to get socket subsystem"));
		return false;
	}

	// make our listen socket
	Socket = SocketSystem->CreateSocket(NAME_Stream, TEXT("FPerfCounters"));
	if (Socket == nullptr)
	{
		UE_LOG(LogPerfCounters, Error, TEXT("FPerfCounters unable to allocate stream socket"));
		return false;
	}

	// make us non blocking
	Socket->SetNonBlocking(true);

	// create a localhost binding for the requested port
	TSharedRef<FInternetAddr> LocalhostAddr = SocketSystem->CreateInternetAddr(0x7f000001 /* 127.0.0.1 */, StatsPort);
	if (!Socket->Bind(*LocalhostAddr))
	{
		UE_LOG(LogPerfCounters, Error, TEXT("FPerfCounters unable to bind to %s"), *LocalhostAddr->ToString(true));
		return false;
	}
	StatsPort = Socket->GetPortNo();

	// log the port
	UE_LOG(LogPerfCounters, Display, TEXT("FPerfCounters listening on port %d"), StatsPort);

	// for now, jack this up so we can send in one go
	int32 NewSize;
	Socket->SetSendBufferSize(512 * 1024, NewSize); // best effort 512k buffer to avoid not being able to send in one go

	// listen on the port
	if (!Socket->Listen(16))
	{
		UE_LOG(LogPerfCounters, Error, TEXT("FPerfCounters unable to listen on socket"));
		return false;
	}

	return true;
}

FString FPerfCounters::ToJson() const
{
	FString JsonStr;
	TSharedRef< TJsonWriter<> > Json = TJsonWriterFactory<>::Create(&JsonStr);
	Json->WriteObjectStart();
	for (const auto& It : PerfCounterMap)
	{
		const FJsonVariant& JsonValue = It.Value;
		switch (JsonValue.Format)
		{
		case FJsonVariant::String:
			Json->WriteValue(It.Key, JsonValue.StringValue);
			break;
		case FJsonVariant::Number:
			Json->WriteValue(It.Key, JsonValue.NumberValue);
			break;
		case FJsonVariant::Callback:
			if (JsonValue.CallbackValue.IsBound())
			{
				Json->WriteIdentifierPrefix(It.Key);
				JsonValue.CallbackValue.Execute(Json);
			}
			else
			{
				// write an explict null since the callback is unbound and the implication is this would have been an object
				Json->WriteNull(It.Key);
			}
			break;
		case FJsonVariant::Null:
		default:
			// don't write anything since wash may expect a scalar
			break;
		}
	}
	Json->WriteObjectEnd();
	Json->Close();
	return JsonStr;
}

static bool SendAsUtf8(FSocket* Conn, const FString& Message)
{
	FTCHARToUTF8 ConvertToUtf8(*Message);
	int32 BytesSent = 0;
	return Conn->Send(reinterpret_cast<const uint8*>(ConvertToUtf8.Get()), ConvertToUtf8.Length(), BytesSent) && BytesSent == ConvertToUtf8.Length();
}

bool FPerfCounters::Tick(float DeltaTime)
{
	// if we didn't get a socket, don't tick
	if (Socket == nullptr)
	{
		return false;
	}

	// accept any connections
	static const FString PerfCounterRequest = TEXT("FPerfCounters Request");
	FSocket* IncomingConnection = Socket->Accept(PerfCounterRequest);
	if (IncomingConnection)
	{
		// make sure this is non-blocking
		bool bSuccess = false;
		IncomingConnection->SetNonBlocking(true);

		// read any data that's ready
		// NOTE: this is not a full HTTP implementation, just enough to be usable by curl
		uint8 Buffer[2*1024] = { 0 };
		int32 DataLen = 0;
		if (IncomingConnection->Recv(Buffer, sizeof(Buffer)-1, DataLen, ESocketReceiveFlags::None))
		{
			// scan the buffer for a line
			FUTF8ToTCHAR WideBuffer(reinterpret_cast<const ANSICHAR*>(Buffer));
			const TCHAR* BufferEnd = FCString::Strstr(WideBuffer.Get(), TEXT("\r\n"));
			if (BufferEnd != nullptr)
			{
				// crack into pieces
				FString MainLine(BufferEnd - WideBuffer.Get(), WideBuffer.Get());
				TArray<FString> Tokens;
				MainLine.ParseIntoArrayWS(Tokens);
				if (Tokens.Num() >= 2)
				{
					FString Body;
					int ResponseCode = 200;

					// handle the request
					if (Tokens[0] != TEXT("GET"))
					{
						Body = FString::Printf(TEXT("{ \"error\": \"Method %s not allowed\" }"), *Tokens[0]);
						ResponseCode = 405;
					}
					else if (Tokens[1] == TEXT("/stats"))
					{
						Body = ToJson();
					}
					else
					{
						Body = FString::Printf(TEXT("{ \"error\": \"%s not found\" }"), *Tokens[1]);
						ResponseCode = 404;
					}

					// send the response headers
					FString Header = FString::Printf(TEXT("HTTP/1.0 %d\r\nContent-Length: %d\r\nContent-Type: application/json\r\n\r\n"), ResponseCode, Body.Len());
					if (SendAsUtf8(IncomingConnection, Header) && SendAsUtf8(IncomingConnection, Body))
					{
						bSuccess = true;
					}
				}
			}
		}

		// log if we didn't succeed
		if (!bSuccess)
		{
			UE_LOG(LogPerfCounters, Warning, TEXT("FPerfCounters was unable to send a JSON response (or sent partial response)"));
		}

		// close the socket (whether we processed or not
		IncomingConnection->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(IncomingConnection);
	}

	// keep ticking
	return true;
}

void FPerfCounters::SetNumber(const FString& Name, double Value) 
{
	FJsonVariant& JsonValue = PerfCounterMap.FindOrAdd(Name);
	JsonValue.Format = FJsonVariant::Number;
	JsonValue.NumberValue = Value;
}

void FPerfCounters::SetString(const FString& Name, const FString& Value)
{
	FJsonVariant& JsonValue = PerfCounterMap.FindOrAdd(Name);
	JsonValue.Format = FJsonVariant::String;
	JsonValue.StringValue = Value;
}

void FPerfCounters::SetJson(const FString& Name, const FProduceJsonCounterValue& Callback)
{
	FJsonVariant& JsonValue = PerfCounterMap.FindOrAdd(Name);
	JsonValue.Format = FJsonVariant::Callback;
	JsonValue.CallbackValue = Callback;
}

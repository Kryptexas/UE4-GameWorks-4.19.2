#include "PacketHandler.h"
#include "ReliabilityHandlerComponent.h"

IMPLEMENT_MODULE(FPacketHandlerComponentModuleInterface, PacketHandler);

DEFINE_LOG_CATEGORY(PacketHandlerLog);

// BUFFERED PACKET
BufferedPacket::BufferedPacket()
: Data(nullptr)
, BytesCount(0)
, SendTime(0.f)
, Id(0)
{
}

BufferedPacket::~BufferedPacket()
{
	if(Data != nullptr)
	{
		delete Data;
	}
}

// PACKET HANDLER
PacketHandler::PacketHandler()
: Time(0.f)
, State(Handler::State::Uninitialized)
, ReliabilityComponent(nullptr)
{
	OutgoingPacket.SetAllowResize(true);
	OutgoingPacket.AllowAppend(true);
}

void PacketHandler::Tick(float DeltaTime)
{
	Time += DeltaTime;

	switch (State)
	{
		case Handler::State::InitializingProcessors:
		{
			break;
		}
		case Handler::State::Initialized:
		{
			break;
		}
	}

	for (int32 i = 0; i < HandlerComponents.Num(); ++i)
	{
		HandlerComponents[i]->Tick(DeltaTime);
	}
}

void PacketHandler::Initialize(Handler::Mode InMode)
{
	struct ComponentAndOptions
	{
		FString ComponentName;
		FString Options;
	};

	Mode = InMode;

	FString Components;
	GConfig->GetString(TEXT("PacketHandlerComponents"), TEXT("Components"), Components, GEngineIni);

	TArray<ComponentAndOptions> ComponentsArray;
	ComponentsArray.Add(ComponentAndOptions());

	int32 i = 0;
	int32 ComponentsCount = 0;
	while(i < Components.Len())
	{
		TCHAR c = Components[i];

		// End of string
		if(c == ',')
		{
			// If not end of string add a new string
			if(i + 1 < Components.Len())
			{
				ComponentsArray.Add(ComponentAndOptions());
				++ComponentsCount;
			}
		}
		// Parsing Options
		else if(c == '(')
		{
			// Skip '('
			++i;

			// Parse until end of options
			while(i < Components.Len())
			{
				c = Components[i];

				// End of options
				if(c == ')')
				{
					break;
				}
				// Append char to options
				else
				{
					ComponentsArray[ComponentsCount].Options.AppendChar(c);
				}	

				++i;
			}
		}
		// Append char to component name if not whitespace
		else if(c != ' ' )
		{
			ComponentsArray[ComponentsCount].ComponentName.AppendChar(c);
		}

		++i;
	}

	for (i = 0; i < ComponentsArray.Num(); i++)
	{
		// Skip adding reliability component as it is added automatically in the handler
		if(ComponentsArray[i].ComponentName == TEXT("ReliabilityHandlerComponent"))
		{
			continue;
		}

		TSharedPtr<IModuleInterface> Interface = FModuleManager::Get().LoadModule(FName(*ComponentsArray[i].ComponentName));

		if(Interface.IsValid())
		{
			TSharedPtr<FPacketHandlerComponentModuleInterface> PacketHandlerInterface(static_cast<FPacketHandlerComponentModuleInterface*>(&(*Interface)));

			if(PacketHandlerInterface.IsValid())
			{
				// Options Specified
				if(ComponentsArray[i].Options.Len() > 0)
				{
					Add(PacketHandlerInterface->CreateComponentInstance(ComponentsArray[i].Options));
				}
				// No Options Specified
				else
				{
					Add(PacketHandlerInterface->CreateComponentInstance());
				}
			}
		}
		else
		{
			UE_LOG(PacketHandlerLog, Warning, TEXT("Unable to Load Module: %s"), *ComponentsArray[i].ComponentName);
		}
	}
}

void PacketHandler::Add(HandlerComponent* NewHandler)
{
	if (State != Handler::State::Uninitialized)
	{
		LowLevelFatalError(TEXT("Handler added during runtime."));
	}

	HandlerComponents.Add(NewHandler);
	NewHandler->Handler = this;
	NewHandler->Initialize();
}

const ProcessedPacket PacketHandler::Outgoing(uint8* Packet, int32 Count)
{
	OutgoingPacket.Reset();

	switch (State)
	{
		case Handler::State::Uninitialized:
		{
			if(ReliabilityComponent == nullptr)
			{
				ReliabilityComponent = new ReliabilityHandlerComponent;
				Add(ReliabilityComponent);
			}

			// Have handlers to initialize other than reliability
			if (HandlerComponents.Num() > 1)
			{
				State = Handler::State::InitializingProcessors;
			} 
			else
			{
				State = Handler::State::Initialized;
				break;
			}
		}
		case Handler::State::InitializingProcessors:
		{
			// Buffer any packets being sent from game code
			// until processors are initialized
			if (Count > 0)
			{
				BufferedPacket* NewBufferedPacket = new BufferedPacket;
				BufferedPackets.Add(NewBufferedPacket);

				uint8* Data = new uint8[Count];
				memcpy(Data, Packet, Count);
				NewBufferedPacket->Data = Data;
				NewBufferedPacket->BytesCount = Count;

				Count = 0;
				Packet = nullptr;
			}

			break;
		}
		case Handler::State::Initialized:
		{
			OutgoingPacket.Serialize(Packet, Count);
			break;
		}
		default:
		{
			break;
		}
	}

	// Queue packet for resending before handling
	if(Count > 0 && ReliabilityComponent != nullptr)
	{
		ReliabilityComponent->QueuePacketForResending(Packet, Count);
	}

	// Handle
	for (int32 i = 0; i < HandlerComponents.Num(); ++i)
	{
		if (HandlerComponents[i]->IsActive())
		{
			HandlerComponents[i]->Outgoing(OutgoingPacket);
		}
	}

	return ProcessedPacket(OutgoingPacket.GetData(), OutgoingPacket.GetNumBytes());
}

const ProcessedPacket PacketHandler::Incoming(uint8* Packet, int32 Count)
{
	FBitReader ProcessedPacketReader(Packet, Count * 8);

	switch (State)
	{
		case Handler::State::Uninitialized:
		{
			if (ReliabilityComponent == nullptr)
			{
				ReliabilityComponent = new ReliabilityHandlerComponent;
				Add(ReliabilityComponent);
			}

			if (HandlerComponents.Num() > 1)
			{
				State = Handler::State::InitializingProcessors;
			}
			else
			{
				State = Handler::State::Initialized;
			}
		}
		case Handler::State::InitializingProcessors:
		{
			break;
		}
		case Handler::State::Initialized:
		{
			break;
		}
		default:
		{
			break;
		}
	}

	// Handle
	for (int32 i = HandlerComponents.Num() - 1; i >= 0; --i)
	{
		if (HandlerComponents[i]->IsActive() && ProcessedPacketReader.GetNumBytes() > 0)
		{
			HandlerComponents[i]->Incoming(ProcessedPacketReader);
		}
	}

	IncomingPacket = ProcessedPacketReader;

	return ProcessedPacket(IncomingPacket.GetData(), IncomingPacket.GetBytesLeft());
}

void PacketHandler::SetState(Handler::State InState)
{
	if (InState == State)
	{
		LowLevelFatalError(TEXT("Set new Packet Processor State to the state it is currently in."));
	} 
	else
	{
		State = InState;
	}
}

void PacketHandler::HandlerInitialized()
{
	// If any buffered packets, add to queue
	for (int32 i = 0; i < BufferedPackets.Num(); ++i)
	{
		QueuedPackets.Enqueue(BufferedPackets[i]);
		BufferedPackets[i] = nullptr;
	}

	SetState(Handler::State::Initialized);
}

void PacketHandler::HandlerComponentInitialized()
{
	// Check if all handlers are initialized
	if (State != Handler::State::Initialized)
	{
		for (int32 i = 0; i < HandlerComponents.Num(); ++i)
		{
			if (HandlerComponents[i]->IsInitialized() == false)
			{
				return;
			}
		}

		HandlerInitialized();
	}
}

BufferedPacket* PacketHandler::GetQueuedPacket()
{
	BufferedPacket* QueuedPacket = nullptr;
	QueuedPackets.Dequeue(QueuedPacket);
	return QueuedPacket;
}

void PacketHandler::QueuePacketForSending(BufferedPacket* PacketToQueue)
{

}

// HANDLER COMPONENT
HandlerComponent::HandlerComponent()
: State(Handler::Component::State::UnInitialized)
, bActive(false)
, bInitialized(false)
{

}

bool HandlerComponent::IsActive() const
{
	return bActive;
}

void HandlerComponent::SetActive(bool Active)
{
	bActive = Active;
}

void HandlerComponent::SetState(Handler::Component::State InState)
{
	State = InState;
}

void HandlerComponent::Initialized()
{
	bInitialized = true;
	Handler->HandlerComponentInitialized();
}

bool HandlerComponent::IsInitialized() const
{
	return bInitialized;
}

// PROCESSED PACKET
ProcessedPacket::ProcessedPacket(uint8* InData, int32 InCount)
{
	Data = InData;
	Count = InCount;
}
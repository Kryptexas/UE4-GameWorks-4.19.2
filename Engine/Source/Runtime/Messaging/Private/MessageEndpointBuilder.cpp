#include "MessagingPrivatePCH.h"
#include "MessageEndpointBuilder.h"

FMessageEndpointPtr FMessageEndpointBuilder::Build()
{
	FMessageEndpointPtr Endpoint;

	IMessageBusPtr Bus = BusPtr.Pin();

	if (Bus.IsValid())
	{
		Endpoint = MakeShareable(new FMessageEndpoint(Name, Bus.ToSharedRef(), Handlers));
		Bus->Register(Endpoint->GetAddress(), Endpoint.ToSharedRef());

		if (Disabled)
		{
			Endpoint->Disable();
		}

		if (InboxEnabled)
		{
			Endpoint->EnableInbox();
			Endpoint->SetRecipientThread(ENamedThreads::AnyThread);
		}
		else
		{
			Endpoint->SetRecipientThread(RecipientThread);
		}
	}

	return Endpoint;
}
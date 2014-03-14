// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LiveEditorListenServerPrivatePCH.h"
#include "Networking.h"

namespace nLiveEditorListenServer
{
	static UProperty *GetPropertyByNameRecurse( UStruct *InStruct, const FString &TokenString, void ** hContainerPtr )
	{
		FString FirstToken;
		FString RemainingTokens;
		int32 SplitIndex;
		if ( TokenString.FindChar( '.', SplitIndex ) )
		{
			FirstToken = TokenString.LeftChop( TokenString.Len()-SplitIndex );
			RemainingTokens = TokenString.RightChop(SplitIndex+1);
		}
		else
		{
			FirstToken = TokenString;
			RemainingTokens = FString(TEXT(""));
		}

		for (TFieldIterator<UProperty> PropertyIt(InStruct, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			UProperty* Property = *PropertyIt;
			FName PropertyName = Property->GetFName();
			if ( FirstToken == PropertyName.ToString() )
			{
				if ( RemainingTokens.Len() == 0 )
				{
					check( *hContainerPtr != NULL );
					return Property;
				}
				else
				{
					UStructProperty *StructProp = Cast<UStructProperty>(Property);
					if ( StructProp )
					{
						check( *hContainerPtr != NULL );
						*hContainerPtr = Property->ContainerPtrToValuePtr<void>(*hContainerPtr);
						return GetPropertyByNameRecurse( StructProp->Struct, RemainingTokens, hContainerPtr );
					}
				}
			}
		}

		return NULL;
	}
}

DEFINE_LOG_CATEGORY_STATIC(LiveEditorListenServer, Log, All);
#define DEFAULT_LISTEN_ENDPOINT FIPv4Endpoint(FIPv4Address(127, 0, 0, 1), LIVEEDITORLISTENSERVER_DEFAULT_PORT)

IMPLEMENT_MODULE( FLiveEditorListenServer, LiveEditorListenServer )

FLiveEditorListenServer::FLiveEditorListenServer()
:	ObjectCreateListener(NULL),
	ObjectDeleteListener(NULL),
	TickObject(NULL),
	Listener(NULL)
{
}

void FLiveEditorListenServer::StartupModule()
{
#if !UE_BUILD_SHIPPING
	//FLiveEditorListenServer does nothing in the Editor
	if ( GIsEditor )
	{
		return;
	}

	Listener = new FTcpListener( DEFAULT_LISTEN_ENDPOINT );
	Listener->OnConnectionAccepted().BindRaw(this, &FLiveEditorListenServer::HandleListenerConnectionAccepted);

	InstallHooks();
#endif
}

void FLiveEditorListenServer::ShutdownModule()
{
#if !UE_BUILD_SHIPPING
	RemoveHooks();
#endif
}

void FLiveEditorListenServer::InstallHooks()
{
	ObjectCreateListener = new nLiveEditorListenServer::FCreateListener(this);
	GUObjectArray.AddUObjectCreateListener( ObjectCreateListener );

	ObjectDeleteListener = new nLiveEditorListenServer::FDeleteListener(this);
	GUObjectArray.AddUObjectDeleteListener( ObjectDeleteListener );

	TickObject = new nLiveEditorListenServer::FTickObject(this);

	MapLoadObserver = MakeShareable( new nLiveEditorListenServer::FMapLoadObserver(this) );
	FCoreDelegates::PostLoadMap.AddSP(MapLoadObserver.Get(), &nLiveEditorListenServer::FMapLoadObserver::OnPostLoadMap);
	FCoreDelegates::PreLoadMap.AddSP(MapLoadObserver.Get(), &nLiveEditorListenServer::FMapLoadObserver::OnPreLoadMap);
}

void FLiveEditorListenServer::RemoveHooks()
{
	if ( Listener )
	{
		Listener->Stop();
		delete Listener;
		Listener = NULL;
	}

	if ( !PendingClients.IsEmpty() )
	{
		FSocket *Client = NULL;
		while( PendingClients.Dequeue(Client) )
		{
			Client->Close();
		}
	}
	for ( TArray<class FSocket*>::TIterator ClientIt(Clients); ClientIt; ++ClientIt )
	{
		(*ClientIt)->Close();
	}

	delete TickObject;
	TickObject = NULL;

	delete ObjectCreateListener;
	ObjectCreateListener = NULL;

	delete ObjectDeleteListener;
	ObjectDeleteListener = NULL;

	FCoreDelegates::PostLoadMap.RemoveAll(MapLoadObserver.Get());
	FCoreDelegates::PreLoadMap.RemoveAll(MapLoadObserver.Get());
	MapLoadObserver = NULL;
}

void FLiveEditorListenServer::OnObjectCreation( const class UObjectBase *NewObject )
{
	ObjectCache.OnObjectCreation( NewObject );
}

void FLiveEditorListenServer::OnObjectDeletion( const class UObjectBase *NewObject )
{
	ObjectCache.OnObjectDeletion( NewObject );
}

void FLiveEditorListenServer::OnPreLoadMap()
{
	ObjectCache.EndCache();
}
void FLiveEditorListenServer::OnPostLoadMap()
{
	ObjectCache.StartCache();
}

bool FLiveEditorListenServer::HandleListenerConnectionAccepted( FSocket* ClientSocket, const FIPv4Endpoint& ClientEndpoint )
{
	PendingClients.Enqueue(ClientSocket);
	return true;
}

void FLiveEditorListenServer::Tick( float DeltaTime )
{
#if !UE_BUILD_SHIPPING
	if ( !PendingClients.IsEmpty() )
	{
		FSocket *Client = NULL;
		while( PendingClients.Dequeue(Client) )
		{
			Clients.Add(Client);
		}
	}

	// remove closed connections
	for (int32 ClientIndex = Clients.Num() - 1; ClientIndex >= 0; --ClientIndex)
	{
		if ( Clients[ClientIndex]->GetConnectionState() != SCS_Connected )
		{
			Clients.RemoveAtSwap(ClientIndex);
		}
	}

	//poll for data
	for ( TArray<class FSocket*>::TIterator ClientIt(Clients); ClientIt; ++ClientIt )
	{
		FSocket *Client = *ClientIt;
		uint32 DataSize = 0;
		while ( Client->HasPendingData(DataSize) && DataSize > 0 )
		{
			FArrayReaderPtr Datagram = MakeShareable(new FArrayReader(true));
			Datagram->Init(FMath::Min(DataSize, 65507u));

			int32 BytesRead = 0;
			if ( Client->Recv(Datagram->GetData(), Datagram->Num(), BytesRead) )
			{
				nLiveEditorListenServer::FNetworkMessage Message;
				*Datagram << Message;

				ReplicateChanges( Message );
			}
		}
	}
#endif
}

static void SetPropertyValue( UObject *Target, const FString &PropertyName, const FString &PropertyValue )
{
	if ( Target == NULL )
		return;

	void *ContainerPtr = Target;
	UProperty *Prop = nLiveEditorListenServer::GetPropertyByNameRecurse( Target->GetClass(), PropertyName, &ContainerPtr );
	if ( !Prop || !Prop->IsA( UNumericProperty::StaticClass() ) )
	{
		return;
	}

	check( ContainerPtr != NULL );

	if ( Prop->IsA( UByteProperty::StaticClass() ) )
	{
		UByteProperty *NumericProp = CastChecked<UByteProperty>(Prop);
		uint8 Value = FCString::Atoi( *PropertyValue );
		NumericProp->SetPropertyValue_InContainer(ContainerPtr, Value);
	}
	else if ( Prop->IsA( UInt8Property::StaticClass() ) )
	{
		UInt8Property *NumericProp = CastChecked<UInt8Property>(Prop);
		int32 Value = FCString::Atoi( *PropertyValue );
		NumericProp->SetPropertyValue_InContainer(ContainerPtr, Value);
	}
	else if ( Prop->IsA( UInt16Property::StaticClass() ) )
	{
		UInt16Property *NumericProp = CastChecked<UInt16Property>(Prop);
		int16 Value = FCString::Atoi( *PropertyValue );
		NumericProp->SetPropertyValue_InContainer(ContainerPtr, Value);
	}
	else if ( Prop->IsA( UIntProperty::StaticClass() ) )
	{
		UIntProperty *NumericProp = CastChecked<UIntProperty>(Prop);
		int32 Value = FCString::Atoi( *PropertyValue );
		NumericProp->SetPropertyValue_InContainer(ContainerPtr, Value);
	}
	else if ( Prop->IsA( UInt64Property::StaticClass() ) )
	{
		UInt64Property *NumericProp = CastChecked<UInt64Property>(Prop);
		int64 Value = FCString::Atoi64( *PropertyValue );
		NumericProp->SetPropertyValue_InContainer(ContainerPtr, Value);
	}
	else if ( Prop->IsA( UUInt16Property::StaticClass() ) )
	{
		UUInt16Property *NumericProp = CastChecked<UUInt16Property>(Prop);
		uint16 Value = FCString::Atoi( *PropertyValue );
		NumericProp->SetPropertyValue_InContainer(ContainerPtr, Value);
	}
	else if ( Prop->IsA( UUInt32Property::StaticClass() ) )
	{
		UUInt32Property *NumericProp = CastChecked<UUInt32Property>(Prop);
		uint32 Value = FCString::Atoi( *PropertyValue );
		NumericProp->SetPropertyValue_InContainer(ContainerPtr, Value);
	}
	else if ( Prop->IsA( UInt64Property::StaticClass() ) )
	{
		UInt64Property *NumericProp = CastChecked<UInt64Property>(Prop);
		uint64 Value = FCString::Atoi64( *PropertyValue );
		NumericProp->SetPropertyValue_InContainer(ContainerPtr, Value);
	}
	else if ( Prop->IsA( UFloatProperty::StaticClass() ) )
	{
		UFloatProperty *NumericProp = CastChecked<UFloatProperty>(Prop);
		float Value = FCString::Atof( *PropertyValue );
		NumericProp->SetPropertyValue_InContainer(ContainerPtr, Value);
	}
	else if ( Prop->IsA( UDoubleProperty::StaticClass() ) )
	{
		UDoubleProperty *NumericProp = CastChecked<UDoubleProperty>(Prop);
		double Value = FCString::Atod( *PropertyValue );
		NumericProp->SetPropertyValue_InContainer(ContainerPtr, Value);
	}
}

void FLiveEditorListenServer::ReplicateChanges( const nLiveEditorListenServer::FNetworkMessage &Message )
{
	if ( Message.Type == nLiveEditorListenServer::CLASSDEFAULT_OBJECT_PROPERTY )
	{
		//UClass *BaseClass = LoadClass<UObject>( NULL, *Message.Payload.ClassName, NULL, 0, NULL );
		UClass *BaseClass = FindObject<UClass>(ANY_PACKAGE, *Message.Payload.ClassName);
		if ( BaseClass )
		{
			SetPropertyValue( BaseClass->ClassDefaultObject, Message.Payload.PropertyName, Message.Payload.PropertyValue );

			TArray< TWeakObjectPtr<UObject> > Replicants;
			ObjectCache.FindObjectDependants( BaseClass->ClassDefaultObject, Replicants );
			for ( TArray< TWeakObjectPtr<UObject> >::TIterator It(Replicants); It; ++It )
			{
				if ( !(*It).IsValid() )
					continue;

				UObject *Object = (*It).Get();
				check( Object->IsA(BaseClass) );
				SetPropertyValue( Object, Message.Payload.PropertyName, Message.Payload.PropertyValue );
			}
		}
	}
}

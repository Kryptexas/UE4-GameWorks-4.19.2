// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#if USE_HTTP_FOR_NFS

#include "NetworkFileSystemPrivatePCH.h"
#include "NetworkFileServerHttp.h"

#include "AllowWindowsPlatformTypes.h"
	#include "libwebsockets.h"
#include "HideWindowsPlatformTypes.h"
////////////////////////////////////////////////////////////////////////// 
// LibWebsockets specific structs. 

// a object of this type is associated by libwebsocket to every http session. 
struct PerSessionData
{
	// data being received. 
	TArray<uint8> In; 
	// data being sent out. 
	TArray<uint8> Out; 
};


// protocol array. 
static struct libwebsocket_protocols protocols[] = {
	/* first protocol must always be HTTP handler */
	{
			"http-only",   // name
			FNetworkFileServerHttp::CallBack_HTTP, // callback
			sizeof(PerSessionData),             // per_session_data_size
			15 * 1024,
			15 * 1024
	},
	{
		NULL, NULL, 0   /* End of list */
	}
};
//////////////////////////////////////////////////////////////////////////


FNetworkFileServerHttp::FNetworkFileServerHttp(
	int32 InPort, 
	const FFileRequestDelegate* InFileRequestDelegate, 
	const FRecompileShadersDelegate* InRecompileShadersDelegate, 
	const TArray<ITargetPlatform*>& InActiveTargetPlatforms
	)
	:ActiveTargetPlatforms(InActiveTargetPlatforms)
	,Port(InPort)
{
	UE_LOG(LogFileServer, Warning, TEXT("Unreal Network Http File Server starting up..."));

	if (InFileRequestDelegate && InFileRequestDelegate->IsBound())
	{
		FileRequestDelegate = *InFileRequestDelegate;
	}

	if (InRecompileShadersDelegate && InRecompileShadersDelegate->IsBound())
	{
		RecompileShadersDelegate = *InRecompileShadersDelegate;
	}


	StopRequested.Reset();
	Ready.Reset();

	// spin up the worker thread, this will block till Init has executed on the freshly spinned up thread, Ready will have appropriate value 
	// set by the end of this function. 
	WorkerThread = FRunnableThread::Create(this,TEXT("FNetworkFileServer"), 8 * 1024, TPri_AboveNormal);
}


bool FNetworkFileServerHttp::IsItReadyToAcceptConnections(void) const
{
	return (Ready.GetValue() != 0); 
}

#if UE_BUILD_DEBUG 
void libwebsocket_debugLog(int level, const char *line)
{ 
	UE_LOG(LogFileServer, Warning, TEXT(" LibWebsocket: %s"), ANSI_TO_TCHAR(line));
}
#endif 

bool FNetworkFileServerHttp::Init()
{
	// setup log level.
#if UE_BUILD_DEBUG 
	lws_set_log_level( LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_DEBUG , libwebsocket_debugLog);
#endif 

	struct lws_context_creation_info info;
	memset(&info,0,sizeof(lws_context_creation_info));

	// look up libwebsockets.h for details. 
	info.port = Port;
	// we listen on all available interfaces. 
	info.iface = NULL;
	// serve only the http protocols. 
	info.protocols = protocols;
	// no extensions
	info.extensions = NULL; 
	info.gid = -1;
	info.uid = -1;
	info.options = 0;
	// tack on this object. 
	info.user = this; 
	
	context = libwebsocket_create_context(&info);

	if (context == NULL) {
		UE_LOG(LogFileServer, Fatal, TEXT(" Could not create a libwebsocket content for port : %d"), Port);
		return false;
	}

	Ready.Set(true);
	return true;
}


bool FNetworkFileServerHttp::GetAddressList(TArray<TSharedPtr<FInternetAddr> >& OutAddresses) const
{
	// if Init failed, its already too late. 
	ensure( context != nullptr);

	// we are listening to all local interfaces. 
	ISocketSubsystem::Get()->GetLocalAdapterAddresses(OutAddresses);
	// Fix up ports. 
	for (int32 AddressIndex = 0; AddressIndex < OutAddresses.Num(); ++AddressIndex)
	{
		OutAddresses[AddressIndex]->SetPort(Port);
	}

	return true;
}

int32 FNetworkFileServerHttp::NumConnections() const
{
	return	RequestHandlers.Num();
}

void FNetworkFileServerHttp::Shutdown()
{ 
	// Allow multiple calls to this function. 
	if ( WorkerThread )
	{
		WorkerThread->Kill(true); // Kill Nicely. Wait for everything to shutdown. 
		delete WorkerThread; 
		WorkerThread = NULL; 
	}
}

uint32 FNetworkFileServerHttp::Run()
{
	UE_LOG(LogFileServer, Display, TEXT("Unreal Network File Http Server is ready for client connections "));

	// start servicing. 

	// service libwebsocket context. 
	while(!StopRequested.GetValue())
	{
		// service libwebsocket, have a slight delay so it doesn't spin on zero load. 
		libwebsocket_service(context, 10); 
		libwebsocket_callback_on_writable_all_protocol(&protocols[0]);
	}

	UE_LOG(LogFileServer, Display, TEXT("Unreal Network File Http Server is now Shutting down "));
	return true;
}

// Called internally by FRunnableThread::Kill. 
void FNetworkFileServerHttp::Stop()
{
	StopRequested.Set(true);
}

void FNetworkFileServerHttp::Exit()
{
	// let's start shutting down. 
	// fires a LWS_CALLBACK_PROTOCOL_DESTROY callback, we clean up after ourselves there.
	libwebsocket_context_destroy(context);
	context = NULL;
}

FNetworkFileServerHttp::~FNetworkFileServerHttp()
{
	Shutdown();
	// delete our request handlers. 
	for ( auto& Element : RequestHandlers)
	{
		delete Element.Value; 
	}
	// make sure context has been already cleaned up. 
	check( context == NULL ); 
}

FNetworkFileServerClientConnection* FNetworkFileServerHttp::CreateNewConnection()
{
	return new FNetworkFileServerClientConnection (FileRequestDelegate,RecompileShadersDelegate,ActiveTargetPlatforms); 
}

// Have a similar process function for the normal tcp connection. 
void FNetworkFileServerHttp::Process(FArchive& In, FArchive&Out, FNetworkFileServerHttp* Server)
{
	int loops = 0; 
	while(!In.AtEnd())
	{
		UE_LOG(LogFileServer, Log, TEXT("In %d "), loops++);
		// Every Request has a Guid attached to it - similar to Web session IDs. 
		FGuid ClientGuid;
		In << ClientGuid; 

		UE_LOG(LogFileServer, Log, TEXT("Recieved GUID %s"), *ClientGuid.ToString());

		FNetworkFileServerClientConnection* Connection = NULL; 
		if (Server->RequestHandlers.Contains(ClientGuid))
		{
			UE_LOG(LogFileServer, Log, TEXT("Picking up an existing handler" ));
			Connection = Server->RequestHandlers[ClientGuid]; 
		}
		else
		{
			UE_LOG(LogFileServer, Log, TEXT("Creating a handler" ));
			Connection = Server->CreateNewConnection(); 
			Server->RequestHandlers.Add(ClientGuid,Connection);
		}

		Connection->ProcessPayload(In, Out);
	}
}

// This static function handles all callbacks coming in and when context is services via libwebsocket_service
// return value of -1, closes the connection.
int FNetworkFileServerHttp::CallBack_HTTP(	
			struct libwebsocket_context *context, 
			struct libwebsocket *wsi, 
			enum libwebsocket_callback_reasons reason, 
			void *user, 
			void *in, 
			size_t len)
{
	PerSessionData* bufferInfo = (PerSessionData*)user;
	FNetworkFileServerHttp* Server = (FNetworkFileServerHttp*)libwebsocket_context_user(context); 

	switch (reason)
	{

	case LWS_CALLBACK_HTTP: 

		// hang on to socket even if there's no data for atleast 60 secs. 
		libwebsocket_set_timeout(wsi, NO_PENDING_TIMEOUT, 60);

		/* if it was not legal POST URL, let it continue and accept data */
		if (!lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI))
		{
			char *requested_uri = (char *) in;

			// client request the base page. e.g  http://unrealfileserver:port/ 
			// just return a banner, probably add some more information, e,g Version, Config, Game. etc. 
			if ( FCString::Strcmp(ANSI_TO_TCHAR(requested_uri), TEXT("/")) == 0 )
			{
				TCHAR Buffer[1024]; 
				TCHAR ServerBanner[] = TEXT("<HTML>This is Unreal File Server</HTML>");
				int x  = FCString::Sprintf(
					Buffer,
					TEXT("HTTP/1.0 200 OK\x0d\x0a Server: Unreal File Server\x0d\x0a Connection: close\x0d\x0a Content-Type: text/html; charset=utf-8\x0d\x0a Content-Length: %u\x0d\x0a\x0d\x0a%s"), 
					FCString::Strlen(ServerBanner),
					ServerBanner
					);

				// very small data being sent, its fine to just send.  
				libwebsocket_write(wsi,(unsigned char*)TCHAR_TO_ANSI(Buffer),FCString::Strlen(Buffer), LWS_WRITE_HTTP);
			}
			else
			{
				// client has asked for a file. ( only html/js files are served.) 
					
				// what type is being served. 
				FString FilePath = FPaths::GameDir() / TEXT("Binaries/HTML5") +  FString((char*)in); 
				TCHAR *Mime = NULL; 

				if ( FilePath.Contains(".html") )
				{
					Mime = TEXT("text/html;charset=UTF-8");
				}
				else if ( FilePath.Contains(".js"))
				{
					Mime = TEXT("application/javascript;charset=UTF-8");
				}
				else
				{
					// bad client. bail. 
					return -1;
				}

				UE_LOG(LogFileServer, Log, TEXT("Serving file %s with mime %s "), *FilePath, (Mime));

				FString AbsoluteFilePath = FPaths::ConvertRelativePathToFull(FilePath);
				AbsoluteFilePath.ReplaceInline(TEXT("/"),TEXT("\\"));

				// we are going to read the complete file in memory and then serve it in batches. 
				// rather than reading and sending in batches because Unreal NFS servers are not running in memory 
				// constrained env and the added complexity is not worth it. 

				FILE *f = fopen(TCHAR_TO_ANSI(*AbsoluteFilePath), "rb");

				if (f == NULL )
				{
					// umm. we didn't find file, we should tell the client that we couldn't find it. 
					// send 404.
					char Header[]= 	"HTTP/1.1 404 Not Found\x0d\x0a"
									"Server: Unreal File Server\x0d\x0a"
									"Connection: close\x0d\x0a";

					libwebsocket_write(wsi,(unsigned char*)Header,FCStringAnsi::Strlen(Header), LWS_WRITE_HTTP);
					// chug along, client will close the  connection. 
					break; 
				}

				// find the size of the file. 
				fseek(f, 0, SEEK_END);
				long fsize = ftell(f);
				fseek(f, 0, SEEK_SET);

				// file up the header. 
				TCHAR header[1024];
				int x  = FCString::Sprintf(header,
					TEXT("HTTP/1.1 200 OK\x0d\x0a Server: Unreal File Server\x0d\x0a Connection: close\x0d\x0a Content-Type: %s \x0d\x0a Content-Length: %u\x0d\x0a\x0d\x0a"),Mime,fsize);

				// make space for the whole file in our out buffer. 
				bufferInfo->Out.Append((const unsigned char*)TCHAR_TO_ANSI(header),x); 
				bufferInfo->Out.AddUninitialized(fsize);
				// read
				fread(bufferInfo->Out.GetData() + x, fsize, 1, f);
				fclose(f);
				// we need to write back to the client, queue up a write callback. 
				libwebsocket_callback_on_writable(context, wsi);
			}
		}
		else
		{
			// we got a post request!,  queue up a write callback. 
			libwebsocket_callback_on_writable(context, wsi);
		}

		break;
	case LWS_CALLBACK_HTTP_BODY: 
		{
			// post data is coming in, push it on to our incoming buffer.  
			UE_LOG(LogFileServer, Log, TEXT("Incoming HTTP Partial Body Size %d, total size  %d"),len, len+ bufferInfo->In.Num());
			bufferInfo->In.Append((uint8*)in,len);
			// we received some data - update time out.
			libwebsocket_set_timeout(wsi, NO_PENDING_TIMEOUT, 60);
		}
		break;
	case LWS_CALLBACK_HTTP_BODY_COMPLETION: 
		{
			// we have all the post data from the client. 
			// create archives and process them. 
			UE_LOG(LogFileServer, Log, TEXT("Incoming HTTP total size  %d"), bufferInfo->In.Num());
			FMemoryReader Reader(bufferInfo->In);
			FBufferArchive Writer;

			FNetworkFileServerHttp::Process(Reader,Writer,Server);

			// even if we have 0 data to push, tell the client that we don't any data. 
			char header[1024];
			int x  = FCStringAnsi::Sprintf(
				(ANSICHAR*)header,
				"HTTP/1.1 200 OK\x0d\x0a"
				"Server: Unreal File Server\x0d\x0a"
				"Connection: close\x0d\x0a"
				"Content-Type: application/octet-stream \x0d\x0a"
				"Content-Length: %u\x0d\x0a\x0d\x0a", 
				Writer.Num()
				);

			// Add Http Header 
			bufferInfo->Out.Append((unsigned char*)header,x); 
			// Add Binary Data.  
			bufferInfo->Out.Append(Writer);

			// we have enqueued data increase timeout and push a writable callback. 
			libwebsocket_set_timeout(wsi, NO_PENDING_TIMEOUT, 60);
			libwebsocket_callback_on_writable(context, wsi);

		}
		break;
	case LWS_CALLBACK_CLOSED_HTTP: 
		// client went away or 
		//clean up. 
		bufferInfo->In.Empty();
		bufferInfo->Out.Empty(); 

		break;

	case LWS_CALLBACK_PROTOCOL_DESTROY: 
		// we are going away. 

		break; 

	case LWS_CALLBACK_HTTP_WRITEABLE: 

		// get rid of superfluous write  callbacks.
		if ( bufferInfo == NULL )
			break;

		// we have data o send out.
		if (bufferInfo->Out.Num())
		{
			int SentSize = libwebsocket_write(wsi,(unsigned char*)bufferInfo->Out.GetData(),bufferInfo->Out.Num(), LWS_WRITE_HTTP);
			// get rid of the data that has been sent. 
			bufferInfo->Out.RemoveAt(0,SentSize); 
		}

		break;

	default:
		break;
	}

	return 0; 
}

#endif
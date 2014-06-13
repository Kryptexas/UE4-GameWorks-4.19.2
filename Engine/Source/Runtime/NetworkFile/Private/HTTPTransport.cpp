#include "NetworkFilePrivatePCH.h"

#if ENABLE_HTTP_FOR_NFS

#include "HTTPTransport.h"
#include "NetworkMessage.h"

#if PLATFORM_HTML5_WIN32 
#include "WinHttp.h"
#endif 


FHTTPTransport::FHTTPTransport()
	:Guid(FGuid::NewGuid())
{
}

bool FHTTPTransport::Initialize(const TCHAR* HostIp)
{
	TCHAR Url[1048];
	FCString::Sprintf( Url, TEXT("http://%s:%d"), HostIp, (int)DEFAULT_FILE_SERVING_PORT); 

#if !PLATFORM_HTML5
	HttpRequest = FHttpModule::Get().CreateRequest(); 
	HttpRequest->SetURL(Url);
#endif 

#if PLATFORM_HTML5_WIN32
	HTML5Win32::NFSHttp::Init(TCHAR_TO_ANSI(Url));
#endif 

	TArray<uint8> In,Out; 
	bool RetResult = SendPayloadAndReceiveResponse(In,Out); 
	return RetResult; 
}

bool FHTTPTransport::SendPayloadAndReceiveResponse(TArray<uint8>& In, TArray<uint8>& Out)
{
#if !PLATFORM_HTML5
	
	if (GIsRequestingExit) // We have already lost HTTP Module. 
		return false; 

	class HTTPRequestHandler
	{

	public:
		HTTPRequestHandler(TArray<uint8>& InOut)
			:Out(InOut)
		{} 
		void HttpRequestComplete(	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
		{
			if (HttpResponse.IsValid())
				Out.Append(HttpResponse->GetContent());
		}
	private: 
		TArray<uint8>& Out; 
	};

	HTTPRequestHandler Handler(Out);

	HttpRequest->OnProcessRequestComplete().BindRaw(&Handler,&HTTPRequestHandler::HttpRequestComplete );
	if ( In.Num() )
	{
		HttpRequest->SetVerb("POST");

		FBufferArchive Ar; 
		Ar << Guid; 
		Ar.Append(In); 

		HttpRequest->SetContent(Ar);
	}
	else
	{
		HttpRequest->SetVerb("GET");
	}

	HttpRequest->ProcessRequest();

	FDateTime StartTime; 
	FTimespan Span = FDateTime::UtcNow() - StartTime; 

	while( HttpRequest->GetStatus() != EHttpRequestStatus::Failed && HttpRequest->GetStatus() != EHttpRequestStatus::Succeeded &&  Span.GetSeconds() < 10 )
	{
		HttpRequest->Tick(0);
		Span = FDateTime::UtcNow() - StartTime; 
	}


	if (HttpRequest->GetStatus() == EHttpRequestStatus::Succeeded)
		return true; 

	HttpRequest->CancelRequest();

	return false; 
#else  // PLATFORM_HTML5

	FBufferArchive Ar; 
	if ( In.Num() )
	{
		Ar << Guid; 
	}

	Ar.Append(In); 
	unsigned char *OutData = NULL; 
	unsigned int OutSize= 0; 

#if PLATFORM_HTML5_WIN32


	bool RetVal = HTML5Win32::NFSHttp::SendPayLoadAndRecieve(Ar.GetData(), Ar.Num(), &OutData, OutSize);
	if (OutSize)
	{
		Out.Append(OutData,OutSize); 
		free (OutData); 
	}

	return RetVal; 
#endif 

#if PLATFORM_HTML5_BROWSER
	UE_SendAndRecievePayLoad("http://127.0.0.1:9000",(char*)Ar.GetData(),Ar.Num(),(char**)&OutData,(int*)&OutSize);
	Out.Append(OutData,OutSize); 
	// don't go through the Unreal Memory system. 
	::free (OutData); 

	return true; 
#endif 
#endif

}

#endif 
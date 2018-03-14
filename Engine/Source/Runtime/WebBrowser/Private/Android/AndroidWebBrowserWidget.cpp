// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AndroidWebBrowserWidget.h"
#include "AndroidWebBrowserWindow.h"
#include "AndroidWebBrowserDialog.h"
#include "AndroidJSScripting.h"
#include "AndroidApplication.h"
#include "AndroidWindow.h"
#include "AndroidJava.h"
#include "Async.h"
#include "ScopeLock.h"
#include "RHICommandList.h"
#include "RenderingThread.h"
#include "ExternalTexture.h"
#include "SlateTextures.h"
#include "SlateMaterialBrush.h"
#include "SharedPointer.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "WebBrowserTextureSample.h"

// For UrlDecode
#include "Http.h"

#include <jni.h>

FCriticalSection SAndroidWebBrowserWidget::WebControlsCS;
TMap<jlong, TWeakPtr<SAndroidWebBrowserWidget>> SAndroidWebBrowserWidget::AllWebControls;

TSharedPtr<SAndroidWebBrowserWidget> SAndroidWebBrowserWidget::GetWidgetPtr(JNIEnv* JEnv, jobject Jobj)
{
	FScopeLock L(&WebControlsCS);

	jclass Class = JEnv->GetObjectClass(Jobj);
	jmethodID JMethod = JEnv->GetMethodID(Class, "GetNativePtr", "()J");
	check(JMethod != nullptr);

	int64 ObjAddr = JEnv->CallLongMethod(Jobj, JMethod);

	TWeakPtr<SAndroidWebBrowserWidget> WebControl = AllWebControls.FindRef(ObjAddr);
	return (WebControl.IsValid()) ? WebControl.Pin() : TSharedPtr<SAndroidWebBrowserWidget>();
}

SAndroidWebBrowserWidget::~SAndroidWebBrowserWidget()
{
	if (JavaWebBrowser.IsValid())
	{
		if (GSupportsImageExternal && !FAndroidMisc::ShouldUseVulkan())
		{
			// Unregister the external texture on render thread
			FTextureRHIRef VideoTexture = JavaWebBrowser->GetVideoTexture();

			JavaWebBrowser->SetVideoTexture(nullptr);
			JavaWebBrowser->Release();

			struct FReleaseVideoResourcesParams
			{
				FTextureRHIRef VideoTexture;
				FGuid PlayerGuid;
			};

			FReleaseVideoResourcesParams ReleaseVideoResourcesParams = { VideoTexture, WebBrowserTexture->GetExternalTextureGuid() };

			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(AndroidWebBrowserWriteVideoSample,
				FReleaseVideoResourcesParams, Params, ReleaseVideoResourcesParams,
				{
					FExternalTextureRegistry::Get().UnregisterExternalTexture(Params.PlayerGuid);
					// @todo: this causes a crash
					//					Params.VideoTexture->Release();
				});
		}
		else
		{
			JavaWebBrowser->SetVideoTexture(nullptr);
			JavaWebBrowser->Release();
		}

	}
	delete TextureSamplePool;
	TextureSamplePool = nullptr;
	
	WebBrowserTextureSamplesQueue->RequestFlush();

	if (WebBrowserMaterial != nullptr)
	{
		WebBrowserMaterial->RemoveFromRoot();
		WebBrowserMaterial = nullptr;
	}

	if (WebBrowserTexture != nullptr)
	{
		WebBrowserTexture->RemoveFromRoot();
		WebBrowserTexture = nullptr;
	}

	FScopeLock L(&WebControlsCS);
	AllWebControls.Remove(reinterpret_cast<jlong>(this));
}

void SAndroidWebBrowserWidget::Construct(const FArguments& Args)
{
	{
		FScopeLock L(&WebControlsCS);
		AllWebControls.Add(reinterpret_cast<jlong>(this), StaticCastSharedRef<SAndroidWebBrowserWidget>(AsShared()));
	}

	IsAndroid3DBrowser = true;
	WebBrowserWindowPtr = Args._WebBrowserWindow;
	HistorySize = 0;
	HistoryPosition = 0;
	
	FIntPoint viewportSize = WebBrowserWindowPtr.Pin()->GetViewportSize();
	JavaWebBrowser = MakeShared<FJavaAndroidWebBrowser, ESPMode::ThreadSafe>(false, FAndroidMisc::ShouldUseVulkan(), viewportSize.X, viewportSize.Y,
		reinterpret_cast<jlong>(this), !(UE_BUILD_SHIPPING || UE_BUILD_TEST), Args._UseTransparency);

	TextureSamplePool = new FWebBrowserTextureSamplePool();
	WebBrowserTextureSamplesQueue = MakeShared<FWebBrowserTextureSampleQueue, ESPMode::ThreadSafe>();
	WebBrowserTexture = nullptr;
	WebBrowserMaterial = nullptr;
	WebBrowserBrush = nullptr;

	// create external texture
	WebBrowserTexture = NewObject<UWebBrowserTexture>((UObject*)GetTransientPackage(), NAME_None, RF_Transient | RF_Public);

	if (WebBrowserTexture != nullptr)
	{
		WebBrowserTexture->UpdateResource();
		WebBrowserTexture->AddToRoot();
	}

	// create wrapper material
	UMaterial* Material = LoadObject<UMaterial>(nullptr, TEXT("/WebBrowserWidget/WebTexture_M"), nullptr, LOAD_None, nullptr);
	if (Material)
	{
		// create wrapper material
		WebBrowserMaterial = UMaterialInstanceDynamic::Create(Material, nullptr);

		if (WebBrowserMaterial)
		{
			WebBrowserMaterial->SetTextureParameterValue("SlateUI", WebBrowserTexture);
			WebBrowserMaterial->AddToRoot();

			// create Slate brush
			WebBrowserBrush = MakeShareable(new FSlateBrush());
			{
				WebBrowserBrush->SetResourceObject(WebBrowserMaterial);
			}
		}
	}
	
	check(JavaWebBrowser.IsValid());

	JavaWebBrowser->LoadURL(Args._InitialURL);
}

void SAndroidWebBrowserWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (WebBrowserWindowPtr.IsValid() && WebBrowserWindowPtr.Pin()->GetParentWindow().IsValid())
	{
		bool ShouldSetAndroid3DBrowser = WebBrowserWindowPtr.Pin()->GetParentWindow().Get()->IsVirtualWindow();
		if(IsAndroid3DBrowser != ShouldSetAndroid3DBrowser)
		{
			IsAndroid3DBrowser = ShouldSetAndroid3DBrowser;
			JavaWebBrowser->SetAndroid3DBrowser(IsAndroid3DBrowser);
		}
	}

	if (!JavaWebBrowser.IsValid())
	{	
		return;
	}
	// deal with resolution changes (usually from streams)
	if (JavaWebBrowser->DidResolutionChange())
	{
		JavaWebBrowser->SetVideoTextureValid(false);
	}

	// Calculate UIScale, which can vary frame-to-frame thanks to device rotation
	// UI Scale is calculated relative to vertical axis of 1280x720 / 720x1280
	float UIScale;
	FPlatformRect ScreenRect = FAndroidWindow::GetScreenRect();
	int32_t ScreenWidth, ScreenHeight;
	FAndroidWindow::CalculateSurfaceSize(FAndroidWindow::GetHardwareWindow(), ScreenWidth, ScreenHeight);
	if (ScreenWidth > ScreenHeight)
	{
		UIScale = (float)ScreenHeight / (ScreenRect.Bottom - ScreenRect.Top);
	}
	else
	{
		UIScale = (float)ScreenHeight / (ScreenRect.Bottom - ScreenRect.Top);
	}

	FVector2D Position = AllottedGeometry.GetAccumulatedRenderTransform().GetTranslation() * UIScale;
	FVector2D Size = TransformVector(AllottedGeometry.GetAccumulatedRenderTransform(), AllottedGeometry.GetLocalSize()) * UIScale;

	// Convert position to integer coordinates
	FIntPoint IntPos(FMath::RoundToInt(Position.X), FMath::RoundToInt(Position.Y));
	// Convert size to integer taking the rounding of position into account to avoid double round-down or double round-up causing a noticeable error.
	FIntPoint IntSize = FIntPoint(FMath::RoundToInt(Position.X + Size.X), FMath::RoundToInt(Size.Y + Position.Y)) - IntPos;

	JavaWebBrowser->Update(IntPos.X, IntPos.Y, IntSize.X, IntSize.Y);


	if (IsAndroid3DBrowser)
	{
		if (WebBrowserTexture)
		{
			TSharedPtr<FWebBrowserTextureSample, ESPMode::ThreadSafe> WebBrowserTextureSample;
			WebBrowserTextureSamplesQueue->Peek(WebBrowserTextureSample);

			WebBrowserTexture->TickResource(WebBrowserTextureSample);
		}

		if (FAndroidMisc::ShouldUseVulkan())
		{
			// create new video sample
			auto NewTextureSample = TextureSamplePool->AcquireShared();

			FIntPoint viewportSize = WebBrowserWindowPtr.Pin()->GetViewportSize();

			if (!NewTextureSample->Initialize(viewportSize))
			{
				return;
			}

			struct FWriteWebBrowserParams
			{
				TWeakPtr<FJavaAndroidWebBrowser, ESPMode::ThreadSafe> JavaWebBrowserPtr;
				TWeakPtr<FWebBrowserTextureSampleQueue, ESPMode::ThreadSafe> WebBrowserTextureSampleQueuePtr;
				TSharedRef<FWebBrowserTextureSample, ESPMode::ThreadSafe> NewTextureSamplePtr;
				int32 SampleCount;
			}
			WriteWebBrowserParams = { JavaWebBrowser, WebBrowserTextureSamplesQueue, NewTextureSample, (int32)(viewportSize.X * viewportSize.Y * sizeof(int32)) };

			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(WriteAndroidWebBrowser, FWriteWebBrowserParams, Params, WriteWebBrowserParams,
			{
				auto PinnedJavaWebBrowser = Params.JavaWebBrowserPtr.Pin();
				auto PinnedSamples = Params.WebBrowserTextureSampleQueuePtr.Pin();

				if (!PinnedJavaWebBrowser.IsValid() || !PinnedSamples.IsValid())
				{
					return;
				}

				bool bRegionChanged = false;

				// write frame into buffer
				void* Buffer = nullptr;
				int64 SampleCount = 0;

				if (!PinnedJavaWebBrowser->GetVideoLastFrameData(Buffer, SampleCount, &bRegionChanged))
				{
					FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Fetch RT: ShouldUseVulkan couldn't get texture buffer"));
					return;
				}

				if (SampleCount != Params.SampleCount)
				{
					FPlatformMisc::LowLevelOutputDebugStringf(TEXT("SAndroidWebBrowserWidget::Fetch: Sample count mismatch (Buffer=%llu, Available=%llu"), Params.SampleCount, SampleCount);
				}
				check(Params.SampleCount <= SampleCount);

				// must make a copy (buffer is owned by Java, not us!)
				Params.NewTextureSamplePtr->InitializeBuffer(Buffer, true);

				PinnedSamples->RequestFlush();
				PinnedSamples->Enqueue(Params.NewTextureSamplePtr);
			});
		}
		else if (GSupportsImageExternal && WebBrowserTexture != nullptr)
		{
			struct FWriteWebBrowserParams
			{
				TWeakPtr<FJavaAndroidWebBrowser, ESPMode::ThreadSafe> JavaWebBrowserPtr;
				FGuid PlayerGuid;
				FIntPoint Size;
			};

			FIntPoint viewportSize = WebBrowserWindowPtr.Pin()->GetViewportSize();

			FWriteWebBrowserParams WriteWebBrowserParams = { JavaWebBrowser, WebBrowserTexture->GetExternalTextureGuid(), viewportSize };
			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(WriteAndroidWebBrowser, FWriteWebBrowserParams, Params, WriteWebBrowserParams,
			{
				auto PinnedJavaWebBrowser = Params.JavaWebBrowserPtr.Pin();

				if (!PinnedJavaWebBrowser.IsValid())
				{
					return;
				}

				FTextureRHIRef VideoTexture = PinnedJavaWebBrowser->GetVideoTexture();
				if (VideoTexture == nullptr)
				{
					FRHIResourceCreateInfo CreateInfo;
					FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
					FIntPoint Size = Params.Size;
					VideoTexture = RHICmdList.CreateTextureExternal2D(Size.X, Size.Y, PF_R8G8B8A8, 1, 1, 0, CreateInfo);
					PinnedJavaWebBrowser->SetVideoTexture(VideoTexture);

					if (VideoTexture == nullptr)
					{
						UE_LOG(LogAndroid, Warning, TEXT("CreateTextureExternal2D failed!"));
						return;
					}

					PinnedJavaWebBrowser->SetVideoTextureValid(false);
					FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Fetch RT: Created VideoTexture: %d - %s (%d, %d)"), *reinterpret_cast<int32*>(VideoTexture->GetNativeResource()), *Params.PlayerGuid.ToString(), Size.X, Size.Y);
				}

				int32 TextureId = *reinterpret_cast<int32*>(VideoTexture->GetNativeResource());
				bool bRegionChanged = false;
				if (PinnedJavaWebBrowser->UpdateVideoFrame(TextureId, &bRegionChanged))
				{
					// if region changed, need to reregister UV scale/offset
					FPlatformMisc::LowLevelOutputDebugStringf(TEXT("UpdateVideoFrame RT: %s"), *Params.PlayerGuid.ToString());
					if (bRegionChanged)
					{
						PinnedJavaWebBrowser->SetVideoTextureValid(false);
						FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Fetch RT: %s"), *Params.PlayerGuid.ToString());
					}
				}

				if (!PinnedJavaWebBrowser->IsVideoTextureValid())
				{
					FSamplerStateInitializerRHI SamplerStateInitializer(SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp);
					FSamplerStateRHIRef SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);
					FExternalTextureRegistry::Get().RegisterExternalTexture(Params.PlayerGuid, VideoTexture, SamplerStateRHI);
					FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Fetch RT: Register Guid: %s"), *Params.PlayerGuid.ToString());

					PinnedJavaWebBrowser->SetVideoTextureValid(true);
				}
			});
		}
		else
		{
			// create new video sample
			auto NewTextureSample = TextureSamplePool->AcquireShared();

			FIntPoint viewportSize = WebBrowserWindowPtr.Pin()->GetViewportSize();

			if (!NewTextureSample->Initialize(viewportSize))
			{
				return;
			}

			// populate & add sample (on render thread)
			struct FWriteWebBrowserParams
			{
				TWeakPtr<FJavaAndroidWebBrowser, ESPMode::ThreadSafe> JavaWebBrowserPtr;
				TWeakPtr<FWebBrowserTextureSampleQueue, ESPMode::ThreadSafe> WebBrowserTextureSampleQueuePtr;
				TSharedRef<FWebBrowserTextureSample, ESPMode::ThreadSafe> NewTextureSamplePtr;
				int32 SampleCount;
			}
			WriteWebBrowserParams = { JavaWebBrowser, WebBrowserTextureSamplesQueue, NewTextureSample, (int32)(viewportSize.X * viewportSize.Y * sizeof(int32)) };

			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(WriteAndroidWebBrowser, FWriteWebBrowserParams, Params, WriteWebBrowserParams,
			{
				auto PinnedJavaWebBrowser = Params.JavaWebBrowserPtr.Pin();
				auto PinnedSamples = Params.WebBrowserTextureSampleQueuePtr.Pin();

				if (!PinnedJavaWebBrowser.IsValid() || !PinnedSamples.IsValid())
				{
					return;
				}

				// write frame into texture
				FRHITexture2D* Texture = Params.NewTextureSamplePtr->InitializeTexture();

				if (Texture != nullptr)
				{
					int32 Resource = *reinterpret_cast<int32*>(Texture->GetNativeResource());
					if (!PinnedJavaWebBrowser->GetVideoLastFrame(Resource))
					{
						return;
					}
				}

				PinnedSamples->RequestFlush();
				PinnedSamples->Enqueue(Params.NewTextureSamplePtr);
			});
		}
	}
}


int32 SAndroidWebBrowserWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (IsAndroid3DBrowser && WebBrowserBrush.IsValid())
	{
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), WebBrowserBrush.Get(), ESlateDrawEffect::None);
	}
	return LayerId;
}

FVector2D SAndroidWebBrowserWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(640, 480);
}

void SAndroidWebBrowserWidget::ExecuteJavascript(const FString& Script)
{
	JavaWebBrowser->ExecuteJavascript(Script);
}

void SAndroidWebBrowserWidget::LoadURL(const FString& NewURL)
{
	JavaWebBrowser->LoadURL(NewURL);
}

void SAndroidWebBrowserWidget::LoadString(const FString& Contents, const FString& BaseUrl)
{
	JavaWebBrowser->LoadString(Contents, BaseUrl);
}

void SAndroidWebBrowserWidget::StopLoad()
{
	JavaWebBrowser->StopLoad();
}

void SAndroidWebBrowserWidget::Reload()
{
	JavaWebBrowser->Reload();
}

void SAndroidWebBrowserWidget::Close()
{
	JavaWebBrowser->Release();
	WebBrowserWindowPtr.Reset();
}

void SAndroidWebBrowserWidget::GoBack()
{
	JavaWebBrowser->GoBack();
}

void SAndroidWebBrowserWidget::GoForward()
{
	JavaWebBrowser->GoForward();
}


bool SAndroidWebBrowserWidget::CanGoBack()
{
	return HistoryPosition > 1;
}

bool SAndroidWebBrowserWidget::CanGoForward()
{
	return HistoryPosition < HistorySize-1;
}

jbyteArray SAndroidWebBrowserWidget::HandleShouldInterceptRequest(jstring JUrl)
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();

	const char* JUrlChars = JEnv->GetStringUTFChars(JUrl, 0);
	FString Url = UTF8_TO_TCHAR(JUrlChars);
	JEnv->ReleaseStringUTFChars(JUrl, JUrlChars);

	FString Response;
	bool bOverrideResponse = false;
	int32 Position = Url.Find(*FAndroidJSScripting::JSMessageTag, ESearchCase::CaseSensitive);
	if (Position >= 0)
	{
		AsyncTask(ENamedThreads::GameThread, [Url, Position, this]()
		{
			if (WebBrowserWindowPtr.IsValid())
			{
				TSharedPtr<FAndroidWebBrowserWindow> BrowserWindow = WebBrowserWindowPtr.Pin();
				if (BrowserWindow.IsValid())
				{
					FString Origin = Url.Left(Position);
					FString Message = Url.RightChop(Position + FAndroidJSScripting::JSMessageTag.Len());

					TArray<FString> Params;
					Message.ParseIntoArray(Params, TEXT("/"), false);
					if (Params.Num() > 0)
					{
						for (int I = 0; I < Params.Num(); I++)
						{
							Params[I] = FPlatformHttp::UrlDecode(Params[I]);
						}

						FString Command = Params[0];
						Params.RemoveAt(0, 1);
						BrowserWindow->OnJsMessageReceived(Command, Params, Origin);
					}
					else
					{
						GLog->Logf(ELogVerbosity::Error, TEXT("Invalid message from browser view: %s"), *Message);
					}
				}
			}
		});
		bOverrideResponse = true;
	}
	else
	{
		if (WebBrowserWindowPtr.IsValid())
		{
			TSharedPtr<FAndroidWebBrowserWindow> BrowserWindow = WebBrowserWindowPtr.Pin();
			if (BrowserWindow.IsValid() && BrowserWindow->OnLoadUrl().IsBound())
			{
				FString Method = TEXT(""); // We don't support passing anything but the requested URL
				bOverrideResponse = BrowserWindow->OnLoadUrl().Execute(Method, Url, Response);
			}
		}
	}

	if ( bOverrideResponse )
	{
		FTCHARToUTF8 Converter(*Response);
		jbyteArray Buffer = JEnv->NewByteArray(Converter.Length());
		JEnv->SetByteArrayRegion(Buffer, 0, Converter.Length(), reinterpret_cast<const jbyte *>(Converter.Get()));
		return Buffer;
	}
	return nullptr;
}

bool SAndroidWebBrowserWidget::HandleShouldOverrideUrlLoading(jstring JUrl)
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();

	const char* JUrlChars = JEnv->GetStringUTFChars(JUrl, 0);
	FString Url = UTF8_TO_TCHAR(JUrlChars);
	JEnv->ReleaseStringUTFChars(JUrl, JUrlChars);
	bool Retval = false;

	if (WebBrowserWindowPtr.IsValid())
	{
		TSharedPtr<FAndroidWebBrowserWindow> BrowserWindow = WebBrowserWindowPtr.Pin();
		if (BrowserWindow.IsValid())
		{
			if (BrowserWindow->OnBeforeBrowse().IsBound())
			{
				FWebNavigationRequest RequestDetails;
				RequestDetails.bIsRedirect = false;
				RequestDetails.bIsMainFrame = true; // shouldOverrideUrlLoading is only called on the main frame

				Retval = BrowserWindow->OnBeforeBrowse().Execute(Url, RequestDetails);
			}
		}
	}
	return Retval;
}

bool SAndroidWebBrowserWidget::HandleJsDialog(TSharedPtr<IWebBrowserDialog>& Dialog)
{
	bool Retval = false;
	if (WebBrowserWindowPtr.IsValid())
	{
		TSharedPtr<FAndroidWebBrowserWindow> BrowserWindow = WebBrowserWindowPtr.Pin();
		if (BrowserWindow.IsValid() && BrowserWindow->OnShowDialog().IsBound())
		{
			EWebBrowserDialogEventResponse EventResponse = BrowserWindow->OnShowDialog().Execute(TWeakPtr<IWebBrowserDialog>(Dialog));
			switch (EventResponse)
			{
			case EWebBrowserDialogEventResponse::Handled:
				Retval = true;
				break;
			case EWebBrowserDialogEventResponse::Continue:
				Dialog->Continue(true, (Dialog->GetType() == EWebBrowserDialogType::Prompt) ? Dialog->GetDefaultPrompt() : FText::GetEmpty());
				Retval = true;
				break;
			case EWebBrowserDialogEventResponse::Ignore:
				Dialog->Continue(false);
				Retval = true;
				break;
			case EWebBrowserDialogEventResponse::Unhandled:
			default:
				Retval = false;
				break;
			}
		}
	}
	return Retval;
}

void SAndroidWebBrowserWidget::HandleReceivedTitle(jstring JTitle)
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();

	const char* JTitleChars = JEnv->GetStringUTFChars(JTitle, 0);
	FString Title = UTF8_TO_TCHAR(JTitleChars);
	JEnv->ReleaseStringUTFChars(JTitle, JTitleChars);

	if (WebBrowserWindowPtr.IsValid())
	{
		TSharedPtr<FAndroidWebBrowserWindow> BrowserWindow = WebBrowserWindowPtr.Pin();
		if (BrowserWindow.IsValid())
		{
			BrowserWindow->SetTitle(Title);
		}
	}
}

void SAndroidWebBrowserWidget::HandlePageLoad(jstring JUrl, bool bIsLoading, int InHistorySize, int InHistoryPosition)
{
	HistorySize = InHistorySize;
	HistoryPosition = InHistoryPosition;

	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();

	const char* JUrlChars = JEnv->GetStringUTFChars(JUrl, 0);
	FString Url = UTF8_TO_TCHAR(JUrlChars);
	JEnv->ReleaseStringUTFChars(JUrl, JUrlChars);
	if (WebBrowserWindowPtr.IsValid())
	{
		TSharedPtr<FAndroidWebBrowserWindow> BrowserWindow = WebBrowserWindowPtr.Pin();
		if (BrowserWindow.IsValid())
		{
			BrowserWindow->NotifyDocumentLoadingStateChange(Url, bIsLoading);
		}
	}
}

void SAndroidWebBrowserWidget::HandleReceivedError(jint ErrorCode, jstring /* ignore */, jstring JUrl)
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();

	const char* JUrlChars = JEnv->GetStringUTFChars(JUrl, 0);
	FString Url = UTF8_TO_TCHAR(JUrlChars);
	JEnv->ReleaseStringUTFChars(JUrl, JUrlChars);
	if (WebBrowserWindowPtr.IsValid())
	{
		TSharedPtr<FAndroidWebBrowserWindow> BrowserWindow = WebBrowserWindowPtr.Pin();
		if (BrowserWindow.IsValid())
		{
			BrowserWindow->NotifyDocumentError(Url, ErrorCode);
		}
	}
}

// Native method implementations:

JNI_METHOD jbyteArray Java_com_epicgames_ue4_WebViewControl_00024ViewClient_shouldInterceptRequestImpl(JNIEnv* JEnv, jobject Client, jstring JUrl)
{
	TSharedPtr<SAndroidWebBrowserWidget> Widget = SAndroidWebBrowserWidget::GetWidgetPtr(JEnv, Client);
	if (Widget.IsValid())
	{
		return Widget->HandleShouldInterceptRequest(JUrl);
	}
	else
	{
		return nullptr;
	}
}

JNI_METHOD jboolean Java_com_epicgames_ue4_WebViewControl_00024ViewClient_shouldOverrideUrlLoading(JNIEnv* JEnv, jobject Client, jobject /* ignore */, jstring JUrl)
{
	TSharedPtr<SAndroidWebBrowserWidget> Widget = SAndroidWebBrowserWidget::GetWidgetPtr(JEnv, Client);
	if (Widget.IsValid())
	{
		return Widget->HandleShouldOverrideUrlLoading(JUrl);
	}
	else
	{
		return false;
	}
}

JNI_METHOD void Java_com_epicgames_ue4_WebViewControl_00024ViewClient_onPageLoad(JNIEnv* JEnv, jobject Client, jstring JUrl, jboolean bIsLoading, jint HistorySize, jint HistoryPosition)
{
	TSharedPtr<SAndroidWebBrowserWidget> Widget = SAndroidWebBrowserWidget::GetWidgetPtr(JEnv, Client);
	if (Widget.IsValid())
	{
		Widget->HandlePageLoad(JUrl, bIsLoading, HistorySize, HistoryPosition);
	}
}

JNI_METHOD void Java_com_epicgames_ue4_WebViewControl_00024ViewClient_onReceivedError(JNIEnv* JEnv, jobject Client, jobject /* ignore */, jint ErrorCode, jstring Description, jstring JUrl)
{
	TSharedPtr<SAndroidWebBrowserWidget> Widget = SAndroidWebBrowserWidget::GetWidgetPtr(JEnv, Client);
	if (Widget.IsValid())
	{
		Widget->HandleReceivedError(ErrorCode, Description, JUrl);
	}
}

JNI_METHOD jboolean Java_com_epicgames_ue4_WebViewControl_00024ChromeClient_onJsAlert(JNIEnv* JEnv, jobject Client, jobject /* ignore */, jstring JUrl, jstring Message, jobject Result)
{
	TSharedPtr<SAndroidWebBrowserWidget> Widget = SAndroidWebBrowserWidget::GetWidgetPtr(JEnv, Client);
	if (Widget.IsValid())
	{
		return Widget->HandleJsDialog(EWebBrowserDialogType::Alert, JUrl, Message, Result);
	}
	else
	{
		return false;
	}
}

JNI_METHOD jboolean Java_com_epicgames_ue4_WebViewControl_00024ChromeClient_onJsBeforeUnload(JNIEnv* JEnv, jobject Client, jobject /* ignore */, jstring JUrl, jstring Message, jobject Result)
{
	TSharedPtr<SAndroidWebBrowserWidget> Widget = SAndroidWebBrowserWidget::GetWidgetPtr(JEnv, Client);
	if (Widget.IsValid())
	{
		return Widget->HandleJsDialog(EWebBrowserDialogType::Unload, JUrl, Message, Result);
	}
	else
	{
		return false;
	}
}

JNI_METHOD jboolean Java_com_epicgames_ue4_WebViewControl_00024ChromeClient_onJsConfirm(JNIEnv* JEnv, jobject Client, jobject /* ignore */, jstring JUrl, jstring Message, jobject Result)
{
	TSharedPtr<SAndroidWebBrowserWidget> Widget = SAndroidWebBrowserWidget::GetWidgetPtr(JEnv, Client);
	if (Widget.IsValid())
	{
		return Widget->HandleJsDialog(EWebBrowserDialogType::Confirm, JUrl, Message, Result);
	}
	else
	{
		return false;
	}
}

JNI_METHOD jboolean Java_com_epicgames_ue4_WebViewControl_00024ChromeClient_onJsPrompt(JNIEnv* JEnv, jobject Client, jobject /* ignore */, jstring JUrl, jstring Message, jstring DefaultValue, jobject Result)
{
	TSharedPtr<SAndroidWebBrowserWidget> Widget = SAndroidWebBrowserWidget::GetWidgetPtr(JEnv, Client);
	if (Widget.IsValid())
	{
		return Widget->HandleJsPrompt(JUrl, Message, DefaultValue, Result);
	}
	else
	{
		return false;
	}
}

JNI_METHOD void Java_com_epicgames_ue4_WebViewControl_00024ChromeClient_onReceivedTitle(JNIEnv* JEnv, jobject Client, jobject /* ignore */, jstring Title)
{
	TSharedPtr<SAndroidWebBrowserWidget> Widget = SAndroidWebBrowserWidget::GetWidgetPtr(JEnv, Client);
	if (Widget.IsValid())
	{
		Widget->HandleReceivedTitle(Title);
	}
}

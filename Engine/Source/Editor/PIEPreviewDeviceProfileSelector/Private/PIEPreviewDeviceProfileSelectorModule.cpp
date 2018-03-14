// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PIEPreviewDeviceProfileSelectorModule.h"
#include "FileHelper.h"
#include "JsonObject.h"
#include "JsonReader.h"
#include "JsonSerializer.h"
#include "PIEPreviewDeviceSpecification.h"
#include "JsonObjectConverter.h"
#include "MaterialShaderQualitySettings.h"
#include "RHI.h"
#include "TabManager.h"
#include "CoreGlobals.h"
#include "ModuleManager.h"
#include "ConfigCacheIni.h"
#include "Internationalization/Culture.h"
#include "PIEPreviewWindowStyle.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPIEPreviewDevice, Log, All); 
DEFINE_LOG_CATEGORY(LogPIEPreviewDevice);
IMPLEMENT_MODULE(FPIEPreviewDeviceModule, PIEPreviewDeviceProfileSelector);

void FPIEPreviewDeviceModule::StartupModule()
{
}

void FPIEPreviewDeviceModule::ShutdownModule()
{
}

FString const FPIEPreviewDeviceModule::GetRuntimeDeviceProfileName()
{
	if (!bInitialized)
	{
		InitPreviewDevice();
	}

	return DeviceProfile;
}

void FPIEPreviewDeviceModule::InitPreviewDevice()
{
	bInitialized = true;

	if (ReadDeviceSpecification())
	{
		ERHIFeatureLevel::Type PreviewFeatureLevel = ERHIFeatureLevel::Num;
		switch (DeviceSpecs->DevicePlatform)
		{
			case EPIEPreviewDeviceType::Android:
			{
				IDeviceProfileSelectorModule* AndroidDeviceProfileSelector = FModuleManager::LoadModulePtr<IDeviceProfileSelectorModule>("AndroidDeviceProfileSelector");
				if (AndroidDeviceProfileSelector)
				{
					FPIEAndroidDeviceProperties& AndroidProperties = DeviceSpecs->AndroidProperties;

					TMap<FString, FString> DeviceParameters;
					DeviceParameters.Add("GPUFamily", AndroidProperties.GPUFamily);
					DeviceParameters.Add("GLVersion", AndroidProperties.GLVersion);
					DeviceParameters.Add("VulkanVersion", AndroidProperties.VulkanVersion);
					DeviceParameters.Add("AndroidVersion", AndroidProperties.AndroidVersion);
					DeviceParameters.Add("DeviceMake", AndroidProperties.DeviceMake);
					DeviceParameters.Add("DeviceModel", AndroidProperties.DeviceModel);
					DeviceParameters.Add("UsingHoudini", AndroidProperties.UsingHoudini ? "true" : "false");

					FString PIEProfileName = AndroidDeviceProfileSelector->GetDeviceProfileName(DeviceParameters);
					if (!PIEProfileName.IsEmpty())
					{
						DeviceProfile = PIEProfileName;
					}
				}
				break;
			}
			case EPIEPreviewDeviceType::IOS:
			{
				FPIEIOSDeviceProperties& IOSProperties = DeviceSpecs->IOSProperties;
				DeviceProfile = IOSProperties.DeviceModel;
				break;
			}
		}
		RHISetMobilePreviewFeatureLevel(GetPreviewDeviceFeatureLevel());
	}
}

static void ApplyRHIOverrides(FPIERHIOverrideState* RHIOverrideState)
{
	check(RHIOverrideState);
	GMaxTextureDimensions.SetPreviewOverride(RHIOverrideState->MaxTextureDimensions);
	GMaxShadowDepthBufferSizeX.SetPreviewOverride(RHIOverrideState->MaxShadowDepthBufferSizeX);
	GMaxShadowDepthBufferSizeY.SetPreviewOverride(RHIOverrideState->MaxShadowDepthBufferSizeY);
	GMaxCubeTextureDimensions.SetPreviewOverride(RHIOverrideState->MaxCubeTextureDimensions);
	GRHISupportsInstancing.SetPreviewOverride(RHIOverrideState->SupportsInstancing);
	GSupportsMultipleRenderTargets.SetPreviewOverride(RHIOverrideState->SupportsMultipleRenderTargets);
	GSupportsRenderTargetFormat_PF_FloatRGBA.SetPreviewOverride(RHIOverrideState->SupportsRenderTargetFormat_PF_FloatRGBA);
	GSupportsRenderTargetFormat_PF_G8.SetPreviewOverride(RHIOverrideState->SupportsRenderTargetFormat_PF_G8);
}

const void FPIEPreviewDeviceModule::GetPreviewDeviceResolution(int32& ScreenWidth, int32& ScreenHeight)
{
	if (!DeviceSpecs.IsValid()) 
	{
		return;
	}
	const int32 JSON_VALUE_NOT_SET = 0;
	ScreenWidth = DeviceSpecs->ResolutionX;
	if (DeviceSpecs->ResolutionYImmersiveMode != JSON_VALUE_NOT_SET)
		DeviceSpecs->ResolutionY = DeviceSpecs->ResolutionYImmersiveMode;
	ScreenHeight = DeviceSpecs->ResolutionY;
	static IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MobileContentScaleFactor"));
	float RequestedContentScaleFactor = CVar->GetFloat();
	switch (DeviceSpecs->DevicePlatform)
	{
		case EPIEPreviewDeviceType::Android:
		{
			/********************************************/
			/**code from FAndroidWindow GetScreenRect()**/
			/***GearVr and Daydream have been left out***/
			/********************************************/

			// CSF is a multiplier to 1280x720
			// determine mosaic requirements:
			static auto* MobileHDRCvar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.MobileHDR"));
			const bool bMobileHDR = (MobileHDRCvar && MobileHDRCvar->GetValueOnAnyThread() == 1);

			static auto* MobileHDR32bppModeCvar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.MobileHDR32bppMode"));
			const int32 MobileHDR32Mode = MobileHDR32bppModeCvar->GetValueOnAnyThread();

			bool bMosaicEnabled = false;
			bool bHDR32ModeOverridden = false;
			bool bDeviceRequiresHDR32bpp = false;
			bool bDeviceRequiresMosaic = false;
			
			bDeviceRequiresHDR32bpp = GSupportsRenderTargetFormat_PF_FloatRGBA;// !FAndroidMisc::SupportsFloatingPointRenderTargets();
			bDeviceRequiresMosaic = bDeviceRequiresHDR32bpp && GSupportsShaderFramebufferFetch/*&& !FAndroidMisc::SupportsShaderFramebufferFetch()*/;

			bHDR32ModeOverridden = MobileHDR32Mode != 0;
			bMosaicEnabled = bDeviceRequiresMosaic && (!bHDR32ModeOverridden || MobileHDR32Mode == 1);
			
			// get the aspect ratio of the physical screen
			bool GAndroidIsPortrait = false;

			// some phones gave it the other way (so, if swap if the app is landscape, but width < height)
			if ((GAndroidIsPortrait && ScreenWidth > ScreenHeight) ||
				(!GAndroidIsPortrait && ScreenWidth < ScreenHeight))
			{
				Swap(ScreenWidth, ScreenHeight);
			}

			// ensure the size is divisible by a specified amount
			// do not convert to a surface size that is larger than native resolution
			const int DividableBy = 8;
			ScreenWidth = (ScreenWidth / DividableBy) * DividableBy;
			ScreenHeight = (ScreenHeight / DividableBy) * DividableBy;
			float AspectRatio = (float)ScreenWidth / (float)ScreenHeight;

			int32 MaxWidth = ScreenWidth;
			int32 MaxHeight = ScreenHeight;


			UE_LOG(LogAndroid, Log, TEXT("Mobile HDR: %s"), bMobileHDR ? TEXT("YES") : TEXT("no"));
			if (bMobileHDR)
			{
				UE_LOG(LogAndroid, Log, TEXT("Device requires 32BPP mode : %s"), bDeviceRequiresHDR32bpp ? TEXT("YES") : TEXT("no"));
				UE_LOG(LogAndroid, Log, TEXT("Device requires mosaic: %s"), bDeviceRequiresMosaic ? TEXT("YES") : TEXT("no"));

				if (bHDR32ModeOverridden)
				{
					UE_LOG(LogAndroid, Log, TEXT("--- Enabling 32 BPP override with 'r.MobileHDR32bppMode' = %d"), MobileHDR32Mode);
					UE_LOG(LogAndroid, Log, TEXT("  32BPP mode : YES"));
					UE_LOG(LogAndroid, Log, TEXT("  32BPP mode requires mosaic: %s"), bMosaicEnabled ? TEXT("YES") : TEXT("no"));
					UE_LOG(LogAndroid, Log, TEXT("  32BPP mode requires RGBE: %s"), MobileHDR32Mode == 2 ? TEXT("YES") : TEXT("no"));
				}

				if (bMosaicEnabled)
				{
					UE_LOG(LogAndroid, Log, TEXT("Using mosaic rendering due to lack of Framebuffer Fetch support."));
					if (GetPreviewDeviceFeatureLevel() == ERHIFeatureLevel::ES3_1)
					{
						const int32 OldMaxWidth = MaxWidth;
						const int32 OldMaxHeight = MaxHeight;

						if (GAndroidIsPortrait)
						{
							MaxHeight = FPlatformMath::Min(MaxHeight, 1024);
							MaxWidth = MaxHeight * AspectRatio;
						}
						else
						{
							MaxWidth = FPlatformMath::Min(MaxWidth, 1024);
							MaxHeight = MaxWidth / AspectRatio;
						}

						UE_LOG(LogAndroid, Log, TEXT("Limiting MaxWidth=%d and MaxHeight=%d due to mosaic rendering on ES2 device (was %dx%d)"), MaxWidth, MaxHeight, OldMaxWidth, OldMaxHeight);
					}
				}
			}

			// 0 means to use native size
			int32 Width, Height;
			if (RequestedContentScaleFactor == 0.0f)
			{
				Width = MaxWidth;
				Height = MaxHeight;
				UE_LOG(LogAndroid, Log, TEXT("Setting Width=%d and Height=%d (requested scale = 0 = auto)"), Width, Height);
			}
			else
			{
				if (GAndroidIsPortrait)
				{
					Height = 1280 * RequestedContentScaleFactor;
				}
				else
				{
					Height = 720 * RequestedContentScaleFactor;
				}

				// apply the aspect ration to get the width
				Width = Height * AspectRatio;

				// clamp to native resolution
				Width = FPlatformMath::Min(Width, MaxWidth);
				Height = FPlatformMath::Min(Height, MaxHeight);

				UE_LOG(LogAndroid, Log, TEXT("Setting Width=%d and Height=%d (requested scale = %f)"), Width, Height, RequestedContentScaleFactor);
			}

			ScreenWidth = Width;
			ScreenHeight = Height;
			break;
		}
		case EPIEPreviewDeviceType::IOS:
		{
			bool GIOSIsPortrait = false;

			// some phones gave it the other way (so, if swap if the app is landscape, but width < height)
			if ((GIOSIsPortrait && ScreenWidth > ScreenHeight) ||
				(!GIOSIsPortrait && ScreenWidth < ScreenHeight))
			{
				Swap(ScreenWidth, ScreenHeight);
			}
			break;
		}
	}
}


TSharedRef<SWindow> FPIEPreviewDeviceModule::CreatePIEPreviewDeviceWindow(FVector2D ClientSize, FText WindowTitle, EAutoCenter AutoCenterType, FVector2D ScreenPosition, TOptional<float> MaxWindowWidth, TOptional<float> MaxWindowHeight)
{
	static FWindowStyle BackgroundlessStyle = FCoreStyle::Get().GetWidgetStyle<FWindowStyle>("Window");
	BackgroundlessStyle.SetBackgroundBrush(FSlateNoResource());

	return SNew(SPIEPreviewWindow)
		.Type(EWindowType::GameWindow)
		.Style(&BackgroundlessStyle)
		.ClientSize(ClientSize)
		.Title(WindowTitle)
		.AutoCenter(AutoCenterType)
		.ScreenPosition(ScreenPosition)
		.MaxWidth(MaxWindowWidth)
		.MaxHeight(MaxWindowHeight)
		.FocusWhenFirstShown(true)
		.SaneWindowPlacement(AutoCenterType == EAutoCenter::None)
		.UseOSWindowBorder(false)
		.CreateTitleBar(true)
		.ShouldPreserveAspectRatio(true)
		.LayoutBorder(FMargin(0))
		.SizingRule(ESizingRule::FixedSize)
		.HasCloseButton(true)
		.SupportsMinimize(true)
		.SupportsMaximize(true);
}

void FPIEPreviewDeviceModule::ApplyPreviewDeviceState()
{
	if (!DeviceSpecs.IsValid())
	{
		return;
	}
	FPIEPreviewWindowCoreStyle::InitializePIECoreStyle();
	EShaderPlatform PreviewPlatform = SP_NumPlatforms;
	ERHIFeatureLevel::Type PreviewFeatureLevel = GetPreviewDeviceFeatureLevel();
	FPIERHIOverrideState* RHIOverrideState = nullptr;
	switch (DeviceSpecs->DevicePlatform)
	{
		case EPIEPreviewDeviceType::Android:
		{
			if (PreviewFeatureLevel == ERHIFeatureLevel::ES2)
			{
				PreviewPlatform = SP_OPENGL_ES2_ANDROID;
				RHIOverrideState = &DeviceSpecs->AndroidProperties.GLES2RHIState;
			}
			else
			{
				PreviewPlatform = SP_OPENGL_ES3_1_ANDROID;
				RHIOverrideState = &DeviceSpecs->AndroidProperties.GLES31RHIState;
			}
		}
		break;
		case EPIEPreviewDeviceType::IOS:
		{
			if (PreviewFeatureLevel == ERHIFeatureLevel::ES2)
			{
				PreviewPlatform = SP_OPENGL_ES2_IOS;
				RHIOverrideState = &DeviceSpecs->IOSProperties.GLES2RHIState;
			}
			else
			{
				PreviewPlatform = SP_METAL_MACES3_1;
				RHIOverrideState = &DeviceSpecs->IOSProperties.MetalRHIState;
			}
		}
		break;
	}

	if(PreviewPlatform != SP_NumPlatforms)
	{
		UMaterialShaderQualitySettings* MaterialShaderQualitySettings = UMaterialShaderQualitySettings::Get();
		FName QualityPreviewShaderPlatform = LegacyShaderPlatformToShaderFormat(PreviewPlatform);
		MaterialShaderQualitySettings->GetShaderPlatformQualitySettings(QualityPreviewShaderPlatform);
		MaterialShaderQualitySettings->SetPreviewPlatform(QualityPreviewShaderPlatform);
	}

	ApplyRHIOverrides(RHIOverrideState);

	// TODO: Localization
	FString AppTitle = FGlobalTabmanager::Get()->GetApplicationTitle().ToString() + "Previewing: "+ PreviewDevice;
	FGlobalTabmanager::Get()->SetApplicationTitle(FText::FromString(AppTitle));
	
	int32 ScreenWidth, ScreenHeight;
	GetPreviewDeviceResolution(ScreenWidth, ScreenHeight);
	EWindowMode::Type InWindowMode = EWindowMode::Windowed;
	FString WindowModeSuffix;
	switch (InWindowMode)
	{
	case EWindowMode::Windowed:
	{
		WindowModeSuffix = TEXT("w");
	} break;
	case EWindowMode::WindowedFullscreen:
	{
		WindowModeSuffix = TEXT("wf");
	} break;
	case EWindowMode::Fullscreen:
	{
		WindowModeSuffix = TEXT("f");
	} break;
	}
	
	FString NewValue = FString::Printf(TEXT("%dx%d%s"), ScreenWidth, ScreenHeight, *WindowModeSuffix);
	static IConsoleVariable* CVarSystemRes = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SetRes"));
	CVarSystemRes->Set(*NewValue, ECVF_SetByConsole);
	IConsoleManager::Get().CallAllConsoleVariableSinks();
}

const FPIEPreviewDeviceContainer& FPIEPreviewDeviceModule::GetPreviewDeviceContainer()
{
	if (!EnumeratedDevices.GetRootCategory().IsValid())
	{
		EnumeratedDevices.EnumerateDeviceSpecifications(GetDeviceSpecificationContentDir());
	}
	return EnumeratedDevices;
}

FString FPIEPreviewDeviceModule::GetDeviceSpecificationContentDir()
{
	return FPaths::EngineContentDir() / TEXT("Editor") / TEXT("PIEPreviewDeviceSpecs");
}

FString FPIEPreviewDeviceModule::FindDeviceSpecificationFilePath(const FString& SearchDevice)
{
	const FPIEPreviewDeviceContainer& PIEPreviewDeviceContainer = GetPreviewDeviceContainer();
	FString FoundPath;

	int32 FoundIndex;
	if (PIEPreviewDeviceContainer.GetDeviceSpecifications().Find(SearchDevice, FoundIndex))
	{
		TSharedPtr<FPIEPreviewDeviceContainerCategory> SubCategory = PIEPreviewDeviceContainer.FindDeviceContainingCategory(FoundIndex);
		if(SubCategory.IsValid())
		{
			FoundPath = SubCategory->GetSubDirectoryPath() / SearchDevice + ".json";
		}
	}
	return FoundPath;
}

bool FPIEPreviewDeviceModule::ReadDeviceSpecification()
{
	DeviceSpecs = nullptr;

	if (!FParse::Value(FCommandLine::Get(), GetPreviewDeviceCommandSwitch(), PreviewDevice))
	{
		return false;
	}

	const FString Filename = FindDeviceSpecificationFilePath(PreviewDevice);

	FString Json;
	if (FFileHelper::LoadFileToString(Json, *Filename))
	{
		TSharedPtr<FJsonObject> RootObject;
		TSharedRef<TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(Json);
		if (FJsonSerializer::Deserialize(JsonReader, RootObject) && RootObject.IsValid())
		{
			// We need to initialize FPIEPreviewDeviceSpecifications early as device profiles need to be evaluated before ProcessNewlyLoadedUObjects can be called.
			CreatePackage(nullptr, TEXT("/Script/PIEPreviewDeviceProfileSelector"));

			DeviceSpecs = MakeShareable(new FPIEPreviewDeviceSpecifications());

			if (!FJsonObjectConverter::JsonAttributesToUStruct(RootObject->Values, FPIEPreviewDeviceSpecifications::StaticStruct(), DeviceSpecs.Get(), 0, 0))
			{
				DeviceSpecs = nullptr;
			}
		}
	}
	bool bValidDeviceSpec = DeviceSpecs.IsValid();
	if (!bValidDeviceSpec)
	{
		UE_LOG(LogPIEPreviewDevice, Warning, TEXT("Could not load device specifications for preview target device '%s'"), *PreviewDevice);
	}
	return bValidDeviceSpec;
}

ERHIFeatureLevel::Type FPIEPreviewDeviceModule::GetPreviewDeviceFeatureLevel() const
{
	check(DeviceSpecs.IsValid());

	switch (DeviceSpecs->DevicePlatform)
	{
		case EPIEPreviewDeviceType::Android:
		{
			FString SubVersion;
			// Check for ES3.1+ support from GLVersion, TODO: check other ES31 feature level constraints, see android's PlatformInitOpenGL
			const bool bDeviceSupportsES31 = DeviceSpecs->AndroidProperties.GLVersion.Split(TEXT("OpenGL ES 3."), nullptr, &SubVersion) && FCString::Atoi(*SubVersion) >= 1;

			// check the project's gles support:
			bool bProjectBuiltForES2 = false, bProjectBuiltForES31 = false;
			GConfig->GetBool(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"), TEXT("bBuildForES31"), bProjectBuiltForES31, GEngineIni);
			GConfig->GetBool(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"), TEXT("bBuildForES2"), bProjectBuiltForES2, GEngineIni);

			// Android Preview Device is currently expected to work on gles.
			check(bProjectBuiltForES2 || bProjectBuiltForES31);

			// Projects without ES2 support can only expect to run on ES31 devices.
			check(bProjectBuiltForES2 || bDeviceSupportsES31);

			// ES3.1+ devices fallback to ES2 if the project itself doesn't support ES3.1
			return bDeviceSupportsES31 && bProjectBuiltForES31 ? ERHIFeatureLevel::ES3_1 : ERHIFeatureLevel::ES2;
		}
		case EPIEPreviewDeviceType::IOS:
		{
			bool bProjectBuiltForES2 = false, bProjectBuiltForMetal = false, bProjectBuiltForMRTMetal = false;
			GConfig->GetBool(TEXT("/Script/IOSRuntimeSettings.IOSRuntimeSettings"), TEXT("bSupportsMetal"), bProjectBuiltForMetal, GEngineIni);
			GConfig->GetBool(TEXT("/Script/IOSRuntimeSettings.IOSRuntimeSettings"), TEXT("bSupportsOpenGLES2"), bProjectBuiltForES2, GEngineIni);
			GConfig->GetBool(TEXT("/Script/IOSRuntimeSettings.IOSRuntimeSettings"), TEXT("bSupportsMetalMRT"), bProjectBuiltForMRTMetal, GEngineIni);
			
			const bool bDeviceSupportsMetal = DeviceSpecs->IOSProperties.MetalRHIState.MaxTextureDimensions > 0;

			// not supporting preview for MRT metal 
			check(!bProjectBuiltForMRTMetal);

			// atleast one of these should be valid!
			check(bProjectBuiltForES2 || bProjectBuiltForMetal);

			// if device doesnt support metal the project must have ES2 enabled.
			check(bProjectBuiltForES2 || (bProjectBuiltForMetal && bDeviceSupportsMetal));

			return (bDeviceSupportsMetal && bProjectBuiltForMetal) ? ERHIFeatureLevel::ES3_1 : ERHIFeatureLevel::ES2;
		}
	}

	checkNoEntry();
	return  ERHIFeatureLevel::Num;
}

// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LaunchPrivatePCH.h"
#include "ExceptionHandling.h"
#include "AndroidPlatformCrashContext.h"

#include "AndroidJNI.h"
#include "AndroidApplication.h"
#include <Android/asset_manager.h>
#include <Android/asset_manager_jni.h>

#define JNI_CURRENT_VERSION JNI_VERSION_1_6

JavaVM* GJavaVM;
jobject GJavaGlobalThis = NULL;

// Pointer to target widget for virtual keyboard contents
static IVirtualKeyboardEntry *VirtualKeyboardWidget = NULL;

extern FString GFilePathBase;
extern FString GFontPathBase;
extern bool GOBBinAPK;

//////////////////////////////////////////////////////////////////////////

//Declare all the static members of the class defs 
jclass JDef_GameActivity::ClassID;
jmethodID JDef_GameActivity::AndroidThunkJava_KeepScreenOn;
jmethodID JDef_GameActivity::AndroidThunkJava_Vibrate;
jmethodID JDef_GameActivity::AndroidThunkJava_ShowConsoleWindow;
jmethodID JDef_GameActivity::AndroidThunkJava_ShowVirtualKeyboardInput;
jmethodID JDef_GameActivity::AndroidThunkJava_LaunchURL;
jmethodID JDef_GameActivity::AndroidThunkJava_ResetAchievements;
jmethodID JDef_GameActivity::AndroidThunkJava_ShowAdBanner;
jmethodID JDef_GameActivity::AndroidThunkJava_HideAdBanner;
jmethodID JDef_GameActivity::AndroidThunkJava_CloseAdBanner;
jmethodID JDef_GameActivity::AndroidThunkJava_GetAssetManager;
jmethodID JDef_GameActivity::AndroidThunkJava_Minimize;
jmethodID JDef_GameActivity::AndroidThunkJava_ForceQuit;
jmethodID JDef_GameActivity::AndroidThunkJava_GetFontDirectory;
jmethodID JDef_GameActivity::AndroidThunkJava_IsMusicActive;
jmethodID JDef_GameActivity::AndroidThunkJava_InitHMDs;
jmethodID JDef_GameActivity::AndroidThunkJava_IapSetupService;
jmethodID JDef_GameActivity::AndroidThunkJava_IapQueryInAppPurchases;
jmethodID JDef_GameActivity::AndroidThunkJava_IapBeginPurchase;
jmethodID JDef_GameActivity::AndroidThunkJava_IapIsAllowedToMakePurchases;

DEFINE_LOG_CATEGORY_STATIC(LogEngine, Log, All);

//Game-specific crash reporter
void EngineCrashHandler(const FGenericCrashContext& GenericContext)
{
	const FAndroidCrashContext& Context = static_cast< const FAndroidCrashContext& >( GenericContext );

	static int32 bHasEntered = 0;
	if (FPlatformAtomics::InterlockedCompareExchange(&bHasEntered, 1, 0) == 0)
	{
		const SIZE_T StackTraceSize = 65535;
		ANSICHAR* StackTrace = (ANSICHAR*)FMemory::Malloc(StackTraceSize);
		StackTrace[0] = 0;

		// Walk the stack and dump it to the allocated memory.
		FPlatformStackWalk::StackWalkAndDump(StackTrace, StackTraceSize, 0, Context.Context);
		UE_LOG(LogEngine, Error, TEXT("%s"), ANSI_TO_TCHAR(StackTrace));
		FMemory::Free(StackTrace);

		GError->HandleError();
		FPlatformMisc::RequestExit(true);
	}
}

void AndroidThunkCpp_KeepScreenOn(bool Enable)
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		// call the java side
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_KeepScreenOn, Enable);
	}
}

void AndroidThunkCpp_Vibrate(int64_t Duration)
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		// call the java side
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_Vibrate, Duration);
	}
}

// Call the Java side code for initializing VR HMD modules
void AndroidThunkCpp_InitHMDs()
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_InitHMDs);
	}
}

void AndroidThunkCpp_ShowConsoleWindow()
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		// figure out all the possible texture formats that are allowed
		TArray<FString> PossibleTargetPlatforms;
		FPlatformMisc::GetValidTargetPlatforms(PossibleTargetPlatforms);

		// separate the format suffixes with commas
		FString ConsoleText;
		for (int32 FormatIndex = 0; FormatIndex < PossibleTargetPlatforms.Num(); FormatIndex++)
		{
			const FString& Format = PossibleTargetPlatforms[FormatIndex];
			int32 UnderscoreIndex;
			if (Format.FindLastChar('_', UnderscoreIndex))
			{
				if (ConsoleText != TEXT(""))
				{
					ConsoleText += ", ";
				}
	
				ConsoleText += Format.Mid(UnderscoreIndex+1);
			}
		}

		// call the java side
		jstring ConsoleTextJava	= Env->NewStringUTF(TCHAR_TO_UTF8(*ConsoleText));
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_ShowConsoleWindow, ConsoleTextJava);
		Env->DeleteLocalRef(ConsoleTextJava);
	}
}

void AndroidThunkCpp_ShowVirtualKeyboardInput(TSharedPtr<IVirtualKeyboardEntry> TextWidget, int32 InputType, const FString& Label, const FString& Contents)
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		// remember target widget for contents
		VirtualKeyboardWidget = &(*TextWidget);

		// call the java side
		jstring LabelJava = Env->NewStringUTF(TCHAR_TO_UTF8(*Label));
		jstring ContentsJava = Env->NewStringUTF(TCHAR_TO_UTF8(*Contents));
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_ShowVirtualKeyboardInput, InputType, LabelJava, ContentsJava);
		Env->DeleteLocalRef(ContentsJava);
		Env->DeleteLocalRef(LabelJava);
	}
}

//This function is declared in the Java-defined class, GameActivity.java: "public native void nativeVirtualKeyboardResult(bool update, String contents);"
extern "C" void Java_com_epicgames_ue4_GameActivity_nativeVirtualKeyboardResult(JNIEnv* jenv, jobject thiz, jboolean update, jstring contents)
{
	// update text widget with new contents if OK pressed
	if (update == JNI_TRUE)
	{
		if (VirtualKeyboardWidget != NULL)
		{
			const char* javaChars = jenv->GetStringUTFChars(contents, 0);
			VirtualKeyboardWidget->SetTextFromVirtualKeyboard(FText::FromString(FString(UTF8_TO_TCHAR(javaChars))));

			//Release the string
			jenv->ReleaseStringUTFChars(contents, javaChars);
		}
	}

	// release reference
	VirtualKeyboardWidget = NULL;
}

void AndroidThunkCpp_LaunchURL(const FString& URL)
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		jstring Argument = Env->NewStringUTF(TCHAR_TO_UTF8(*URL));
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_LaunchURL, Argument);
		Env->DeleteLocalRef(Argument);
	}
}

void AndroidThunkCpp_ResetAchievements()
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_ResetAchievements);
	}
}

void AndroidThunkCpp_ShowAdBanner(const FString& AdUnitID, bool bShowOnBottomOfScreen)
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
 	{
		jstring AdUnitIDArg = Env->NewStringUTF(TCHAR_TO_UTF8(*AdUnitID));
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_ShowAdBanner, AdUnitIDArg, bShowOnBottomOfScreen);
		Env->DeleteLocalRef(AdUnitIDArg);
 	}
}

void AndroidThunkCpp_HideAdBanner()
{
 	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
 	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_HideAdBanner);
 	}
}

void AndroidThunkCpp_CloseAdBanner()
{
 	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
 	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_CloseAdBanner);
 	}
}

namespace
{
	jobject GJavaAssetManager = NULL;
	AAssetManager* GAssetManagerRef = NULL;
}

jobject AndroidJNI_GetJavaAssetManager()
{
	if (!GJavaAssetManager)
	{
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			GJavaAssetManager = Env->CallObjectMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_GetAssetManager);
			Env->NewGlobalRef(GJavaAssetManager);
		}
	}
	return GJavaAssetManager;
}

AAssetManager * AndroidThunkCpp_GetAssetManager()
{
	if (!GAssetManagerRef)
	{
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			jobject JavaAssetMgr = AndroidJNI_GetJavaAssetManager();
			GAssetManagerRef = AAssetManager_fromJava(Env, JavaAssetMgr);
		}
	}

	return GAssetManagerRef;
}

void AndroidThunkCpp_Minimize()
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_Minimize);
	}
}

void AndroidThunkCpp_ForceQuit()
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_ForceQuit);
	}
}

bool AndroidThunkCpp_IsMusicActive()
{
	bool active = false;
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		active = (bool)Env->CallBooleanMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_IsMusicActive);
	}
	
	return active;
}

void AndroidThunkCpp_Iap_SetupIapService(const FString& InProductKey)
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		if (GJavaGlobalThis)
		{
			jstring ProductKey = Env->NewStringUTF(TCHAR_TO_UTF8(*InProductKey));
			Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_IapSetupService, ProductKey);
			Env->DeleteLocalRef(ProductKey);
		}
		else
		{
			FPlatformMisc::LowLevelOutputDebugString(L"[JNI] - JNI is invalid");
		}
	}
}

bool AndroidThunkCpp_Iap_QueryInAppPurchases(const TArray<FString>& ProductIDs, const TArray<bool>& bConsumable)
{
	FPlatformMisc::LowLevelOutputDebugString(L"[JNI] - AndroidThunkCpp_Iap_QueryInAppPurchases");
	bool bResult = false;

	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		if (GJavaGlobalThis)
		{
			static jclass StringClass = Env->FindClass("java/lang/String");

			// Populate some java types with the provided product information
			jobjectArray ProductIDArray = (jobjectArray)Env->NewObjectArray(ProductIDs.Num(), StringClass, NULL);
			jbooleanArray ConsumeArray = (jbooleanArray)Env->NewBooleanArray(ProductIDs.Num());

			jboolean* ConsumeArrayValues = Env->GetBooleanArrayElements(ConsumeArray, 0);
			for (uint32 Param = 0; Param < ProductIDs.Num(); Param++)
			{
				jstring StringValue = Env->NewStringUTF(TCHAR_TO_UTF8(*ProductIDs[Param]));
				Env->SetObjectArrayElement(ProductIDArray, Param, StringValue);
				Env->DeleteLocalRef(StringValue);

				ConsumeArrayValues[Param] = bConsumable[Param];
			}
			Env->ReleaseBooleanArrayElements(ConsumeArray, ConsumeArrayValues, 0);

			// Execute the java code for this operation
			bResult = (bool)Env->CallBooleanMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_IapQueryInAppPurchases, ProductIDArray, ConsumeArray);

			// clean up references
			Env->DeleteLocalRef(ProductIDArray);
			Env->DeleteLocalRef(ConsumeArray);
		}
		else
		{
			FPlatformMisc::LowLevelOutputDebugString(L"[JNI] - JNI is invalid");
		}
	}

	return bResult;
}

bool AndroidThunkCpp_Iap_BeginPurchase(const FString& ProductID, const bool bConsumable)
{
	FPlatformMisc::LowLevelOutputDebugString(L"[JNI] - AndroidThunkCpp_Iap_BeginPurchase");
	bool bResult = false;
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		if (GJavaGlobalThis)
		{
			jstring ProductIdJava = Env->NewStringUTF(TCHAR_TO_UTF8(*ProductID));
			bResult = (bool)Env->CallBooleanMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_IapBeginPurchase, ProductIdJava, bConsumable);
			Env->DeleteLocalRef(ProductIdJava);

			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("JDef_GameActivity::AndroidThunkJava_IapBeginPurchase - '%s'"), bResult ? TEXT("true") : TEXT("false"));
		}
		else
		{
			FPlatformMisc::LowLevelOutputDebugString(L"[JNI] - JNI is invalid");
		}
	}

	return bResult;
}

bool AndroidThunkCpp_Iap_IsAllowedToMakePurchases()
{
	FPlatformMisc::LowLevelOutputDebugString(L"[JNI] - AndroidThunkCpp_Iap_IsAllowedToMakePurchases");
	bool bResult = false;
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		if (GJavaGlobalThis)
		{
			bResult = (bool)Env->CallBooleanMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_IapIsAllowedToMakePurchases);
		}
		else
		{
			FPlatformMisc::LowLevelOutputDebugString(L"[JNI] - JNI is invalid");
		}
	}
	return bResult;
}


//The JNI_OnLoad function is triggered by loading the game library from 
//the Java source file.
//	static
//	{
//		System.loadLibrary("MyGame");
//	}
//
// Use the JNI_OnLoad function to map all the class IDs and method IDs to their respective
// variables. That way, later when the Java functions need to be called, the IDs will be ready.
// It is much slower to keep looking up the class and method IDs.

#if UE_BUILD_SHIPPING
#define CHECK_JNI_RESULT( Id )
#else
#define CHECK_JNI_RESULT( Id ) \
	if ( Id == 0 ) \
	{ \
		FPlatformMisc::LowLevelOutputDebugString(TEXT("JNI_OnLoad: Failed to find " #Id)); \
	}
#endif

JNIEXPORT jint JNI_OnLoad(JavaVM* InJavaVM, void* InReserved)
{
	FPlatformMisc::LowLevelOutputDebugString(L"In the JNI_OnLoad function");

	JNIEnv* env = NULL;
	InJavaVM->GetEnv((void **)&env, JNI_CURRENT_VERSION);

	// if you have problems with stuff being missing esspecially in distribution builds then it could be because proguard is stripping things from java
	// check proguard-project.txt and see if your stuff is included in the exceptions
	GJavaVM = InJavaVM;
	FAndroidApplication::InitializeJavaEnv(GJavaVM, JNI_CURRENT_VERSION, GJavaGlobalThis);

	JDef_GameActivity::ClassID = env->FindClass("com/epicgames/ue4/GameActivity");
	CHECK_JNI_RESULT( JDef_GameActivity::ClassID );

	JDef_GameActivity::AndroidThunkJava_KeepScreenOn = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_KeepScreenOn", "(Z)V");
	CHECK_JNI_RESULT(JDef_GameActivity::AndroidThunkJava_KeepScreenOn);
	JDef_GameActivity::AndroidThunkJava_Vibrate = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_Vibrate", "(J)V");
	CHECK_JNI_RESULT(JDef_GameActivity::AndroidThunkJava_Vibrate);
	JDef_GameActivity::AndroidThunkJava_ShowConsoleWindow = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_ShowConsoleWindow", "(Ljava/lang/String;)V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_ShowConsoleWindow );
	JDef_GameActivity::AndroidThunkJava_ShowVirtualKeyboardInput = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_ShowVirtualKeyboardInput", "(ILjava/lang/String;Ljava/lang/String;)V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_ShowVirtualKeyboardInput );
	JDef_GameActivity::AndroidThunkJava_LaunchURL = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_LaunchURL", "(Ljava/lang/String;)V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_LaunchURL );
	JDef_GameActivity::AndroidThunkJava_ResetAchievements = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_ResetAchievements", "()V");
	JDef_GameActivity::AndroidThunkJava_ShowAdBanner = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_ShowAdBanner", "(Ljava/lang/String;Z)V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_ShowAdBanner );
	JDef_GameActivity::AndroidThunkJava_HideAdBanner = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_HideAdBanner", "()V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_HideAdBanner );
	JDef_GameActivity::AndroidThunkJava_CloseAdBanner = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_CloseAdBanner", "()V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_CloseAdBanner );
	JDef_GameActivity::AndroidThunkJava_GetAssetManager = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_GetAssetManager", "()Landroid/content/res/AssetManager;");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_GetAssetManager );

	JDef_GameActivity::AndroidThunkJava_Minimize = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_Minimize", "()V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_Minimize );
	JDef_GameActivity::AndroidThunkJava_ForceQuit = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_ForceQuit", "()V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_ForceQuit );
	
	JDef_GameActivity::AndroidThunkJava_GetFontDirectory = env->GetStaticMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_GetFontDirectory", "()Ljava/lang/String;");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_GetFontDirectory );
	
	JDef_GameActivity::AndroidThunkJava_IsMusicActive = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_IsMusicActive", "()Z");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_IsMusicActive );	

	JDef_GameActivity::AndroidThunkJava_InitHMDs = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_InitHMDs", "()V");
	CHECK_JNI_RESULT(JDef_GameActivity::AndroidThunkJava_InitHMDs);

	// In app purchase functionality
	JDef_GameActivity::AndroidThunkJava_IapSetupService = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_IapSetupService", "(Ljava/lang/String;)V");
	CHECK_JNI_RESULT(JDef_GameActivity::AndroidThunkJava_IapSetupService);

	JDef_GameActivity::AndroidThunkJava_IapQueryInAppPurchases = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_IapQueryInAppPurchases", "([Ljava/lang/String;[Z)Z");
	CHECK_JNI_RESULT(JDef_GameActivity::AndroidThunkJava_IapQueryInAppPurchases);

	JDef_GameActivity::AndroidThunkJava_IapBeginPurchase = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_IapBeginPurchase", "(Ljava/lang/String;Z)Z");
	CHECK_JNI_RESULT(JDef_GameActivity::AndroidThunkJava_IapBeginPurchase);

	JDef_GameActivity::AndroidThunkJava_IapIsAllowedToMakePurchases = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_IapIsAllowedToMakePurchases", "()Z");
	CHECK_JNI_RESULT(JDef_GameActivity::AndroidThunkJava_IapIsAllowedToMakePurchases);

	// hook signals
#if UE_BUILD_DEBUG
	if( GAlwaysReportCrash )
#else
	if( !FPlatformMisc::IsDebuggerPresent() || GAlwaysReportCrash )
#endif
	{
		// disable crash handler.. getting better stack traces from system logcat for now
//		FPlatformMisc::SetCrashHandler(EngineCrashHandler);
	}

	// Cache path to external storage
	jclass EnvClass = env->FindClass("android/os/Environment");
	jmethodID getExternalStorageDir = env->GetStaticMethodID(EnvClass, "getExternalStorageDirectory", "()Ljava/io/File;");
	jobject externalStoragePath = env->CallStaticObjectMethod(EnvClass, getExternalStorageDir, nullptr);
	jmethodID getFilePath = env->GetMethodID(env->FindClass("java/io/File"), "getPath", "()Ljava/lang/String;");
	jstring pathString = (jstring)env->CallObjectMethod(externalStoragePath, getFilePath, nullptr);
	const char *nativePathString = env->GetStringUTFChars(pathString, 0);
	// Copy that somewhere safe 
	GFilePathBase = FString(nativePathString);
	
	// then release...
	env->ReleaseStringUTFChars(pathString, nativePathString);
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Path found as '%s'\n"), *GFilePathBase);

	// Next we check to see if the OBB file is in the APK
	jmethodID isOBBInAPKMethod = env->GetStaticMethodID(JDef_GameActivity::ClassID, "isOBBInAPK", "()Z");
	GOBBinAPK = (bool)env->CallStaticBooleanMethod(JDef_GameActivity::ClassID, isOBBInAPKMethod, nullptr);

	// Get the system font directory
	jstring fontPath = (jstring)env->CallStaticObjectMethod(JDef_GameActivity::ClassID, JDef_GameActivity::AndroidThunkJava_GetFontDirectory);
	const char * nativeFontPathString = env->GetStringUTFChars(fontPath, 0);
	GFontPathBase = FString(nativeFontPathString);
	env->ReleaseStringUTFChars(fontPath, nativeFontPathString);
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Font Path found as '%s'\n"), *GFontPathBase);

	// Wire up to core delegates, so core code can call out to Java
	DECLARE_DELEGATE_OneParam(FAndroidLaunchURLDelegate, const FString&);
	extern CORE_API FAndroidLaunchURLDelegate OnAndroidLaunchURL;
	OnAndroidLaunchURL = FAndroidLaunchURLDelegate::CreateStatic(&AndroidThunkCpp_LaunchURL);

	FPlatformMisc::LowLevelOutputDebugString(L"In the JNI_OnLoad function 5");


	return JNI_CURRENT_VERSION;
}

//Native-defined functions

//This function is declared in the Java-defined class, GameActivity.java: "public native void nativeSetGlobalActivity();"
extern "C" void Java_com_epicgames_ue4_GameActivity_nativeSetGlobalActivity(JNIEnv* jenv, jobject thiz)
{
	if (!GJavaGlobalThis)
	{
		GJavaGlobalThis = jenv->NewGlobalRef(thiz);
		FAndroidApplication::InitializeJavaEnv(GJavaVM, JNI_CURRENT_VERSION, GJavaGlobalThis);
		if(!GJavaGlobalThis)
		{
			FPlatformMisc::LowLevelOutputDebugString(L"Error setting the global GameActivity activity");
			check(false);
		}
	}
}


extern "C" bool Java_com_epicgames_ue4_GameActivity_nativeIsShippingBuild(JNIEnv* LocalJNIEnv, jobject LocalThiz)
{
#if UE_BUILD_SHIPPING
	return JNI_TRUE;
#else
	return JNI_FALSE;
#endif
}

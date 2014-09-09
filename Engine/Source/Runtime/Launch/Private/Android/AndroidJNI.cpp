// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LaunchPrivatePCH.h"
#include "ExceptionHandling.h"

#include "AndroidJNI.h"
#include <Android/asset_manager.h>
#include <Android/asset_manager_jni.h>

#define JNI_CURRENT_VERSION JNI_VERSION_1_6

#define USE_JNI_HELPER 0

JavaVM* GJavaVM;
jobject GJavaGlobalThis = NULL;

// Pointer to target widget for virtual keyboard contents
static SVirtualKeyboardEntry *VirtualKeyboardWidget = NULL;

extern FString GFilePathBase;
extern FString GFontPathBase;
extern bool GOBBinAPK;

#if USE_JNI_HELPER

//////////////////////////////////////////////////////////////////////////
// FJNIHelper

// Caches access to the environment, attached to the current thread
class FJNIHelper : public TThreadSingleton<FJNIHelper>
{
public:
	static JNIEnv* GetEnvironment()
	{
		return Get().CachedEnv;
	}

private:
	JNIEnv* CachedEnv = NULL;

private:
	friend class TThreadSingleton<FJNIHelper>;

	FJNIHelper()
		: CachedEnv(nullptr)
	{
		check(GJavaVM);
		GJavaVM->GetEnv((void **)&CachedEnv, JNI_CURRENT_VERSION);

		const jint AttachResult = GJavaVM->AttachCurrentThread(&CachedEnv, nullptr);
		if (AttachResult == JNI_ERR)
		{
			FPlatformMisc::LowLevelOutputDebugString(TEXT("FJNIHelper failed to attach thread to Java VM!"));
			check(false);
		}
	}

	~FJNIHelper()
	{
		check(GJavaVM);
		const jint DetachResult = GJavaVM->DetachCurrentThread();
		if (DetachResult == JNI_ERR)
		{
			FPlatformMisc::LowLevelOutputDebugString(TEXT("FJNIHelper failed to detach thread from Java VM!"));
			check(false);
		}

		CachedEnv = nullptr;
	}
};

DECLARE_THREAD_SINGLETON( FJNIHelper );

#endif // USE_JNI_HELPER

JNIEnv* GetJavaEnv(bool bRequireGlobalThis)
{
	//@TODO: ANDROID: Remove the other version if the helper works well
#if USE_JNI_HELPER
	if (!bRequireGlobalThis || (GJavaGlobalThis != nullptr))
	{
		return FJNIHelper::GetEnvironment();
	}
	else
	{
		return nullptr;
	}
#else
	JNIEnv* Env = NULL;
	GJavaVM->GetEnv((void **)&Env, JNI_CURRENT_VERSION);

	jint AttachResult = GJavaVM->AttachCurrentThread(&Env, NULL);
	if (AttachResult == JNI_ERR)
	{
		FPlatformMisc::LowLevelOutputDebugString(L"UNIT TEST -- Failed to get the JNI environment!");
		check(false);
		return nullptr;
	}

	return (!bRequireGlobalThis || (GJavaGlobalThis != nullptr)) ? Env : nullptr;
#endif
}

//////////////////////////////////////////////////////////////////////////

//Declare all the static members of the class defs 
jclass JDef_GameActivity::ClassID;
jmethodID JDef_GameActivity::AndroidThunkJava_ShowConsoleWindow;
jmethodID JDef_GameActivity::AndroidThunkJava_ShowVirtualKeyboardInput;
jmethodID JDef_GameActivity::AndroidThunkJava_LaunchURL;
jmethodID JDef_GameActivity::AndroidThunkJava_ShowLeaderboard;
jmethodID JDef_GameActivity::AndroidThunkJava_ShowAchievements;
jmethodID JDef_GameActivity::AndroidThunkJava_QueryAchievements;
jmethodID JDef_GameActivity::AndroidThunkJava_ResetAchievements;
jmethodID JDef_GameActivity::AndroidThunkJava_WriteLeaderboardValue;
jmethodID JDef_GameActivity::AndroidThunkJava_GooglePlayConnect;
jmethodID JDef_GameActivity::AndroidThunkJava_WriteAchievement;
jmethodID JDef_GameActivity::AndroidThunkJava_ShowAdBanner;
jmethodID JDef_GameActivity::AndroidThunkJava_HideAdBanner;
jmethodID JDef_GameActivity::AndroidThunkJava_CloseAdBanner;
jmethodID JDef_GameActivity::AndroidThunkJava_GetAssetManager;
jmethodID JDef_GameActivity::AndroidThunkJava_Minimize;
jmethodID JDef_GameActivity::AndroidThunkJava_ForceQuit;
jmethodID JDef_GameActivity::AndroidThunkJava_GetFontDirectory;

jclass JDef_GameActivity::JavaAchievementClassID;
jfieldID JDef_GameActivity::AchievementIDField;
jfieldID JDef_GameActivity::AchievementProgressField;

DEFINE_LOG_CATEGORY_STATIC(LogEngine, Log, All);

//Game-specific crash reporter
void EngineCrashHandler(const FGenericCrashContext & GenericContext)
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

void AndroidThunkCpp_ShowConsoleWindow()
{
	if (JNIEnv* Env = GetJavaEnv())
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

void AndroidThunkCpp_ShowVirtualKeyboardInput(TSharedPtr<SVirtualKeyboardEntry> TextWidget, int32 InputType, const FString& Label, const FString& Contents)
{
	if (JNIEnv* Env = GetJavaEnv())
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
			VirtualKeyboardWidget->SetText(FText::FromString(FString(UTF8_TO_TCHAR(javaChars))));

			//Release the string
			jenv->ReleaseStringUTFChars(contents, javaChars);
		}
	}

	// release reference
	VirtualKeyboardWidget = NULL;
}

void AndroidThunkCpp_LaunchURL(const FString& URL)
{
	if (JNIEnv* Env = GetJavaEnv())
	{
		jstring Argument = Env->NewStringUTF(TCHAR_TO_UTF8(*URL));
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_LaunchURL, Argument);
		Env->DeleteLocalRef(Argument);
	}
}

void AndroidThunkCpp_ShowLeaderboard(const FString& CategoryName)
{
	if (JNIEnv* Env = GetJavaEnv())
	{
		jstring Argument = Env->NewStringUTF(TCHAR_TO_UTF8(*CategoryName));
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_ShowLeaderboard, Argument);
		Env->DeleteLocalRef(Argument);
	}
}

void AndroidThunkCpp_ShowAchievements()
{
 	if (JNIEnv* Env = GetJavaEnv())
 	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_ShowAchievements);
 	}
}

void AndroidThunkCpp_ResetAchievements()
{
	if (JNIEnv* Env = GetJavaEnv())
	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_ResetAchievements);
	}
}

void AndroidThunkCpp_WriteLeaderboardValue(const FString& LeaderboardName, int64_t Value)
{
	if (JNIEnv* Env = GetJavaEnv())
	{
		jstring LeaderboardNameArg = Env->NewStringUTF(TCHAR_TO_UTF8(*LeaderboardName));
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_WriteLeaderboardValue, LeaderboardNameArg, Value);
		Env->DeleteLocalRef(LeaderboardNameArg);
	}
}

void AndroidThunkCpp_GooglePlayConnect()
{
	if (JNIEnv* Env = GetJavaEnv())
	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_GooglePlayConnect);
	}
}

void AndroidThunkCpp_WriteAchievement(const FString& AchievementID, float PercentComplete)
{
	if (JNIEnv* Env = GetJavaEnv())
	{
		jstring AchievementIDArg = Env->NewStringUTF(TCHAR_TO_UTF8(*AchievementID));
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_WriteAchievement, AchievementIDArg, PercentComplete);
		Env->DeleteLocalRef(AchievementIDArg);
	}
}

void AndroidThunkCpp_QueryAchievements()
{
	if (JNIEnv* Env = GetJavaEnv())
	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_QueryAchievements);
	}
}


void AndroidThunkCpp_ShowAdBanner(const FString& AdUnitID, bool bShowOnBottomOfScreen)
{
	if (JNIEnv* Env = GetJavaEnv())
 	{
		jstring AdUnitIDArg = Env->NewStringUTF(TCHAR_TO_UTF8(*AdUnitID));
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_ShowAdBanner, AdUnitIDArg, bShowOnBottomOfScreen);
		Env->DeleteLocalRef(AdUnitIDArg);
 	}
}

void AndroidThunkCpp_HideAdBanner()
{
 	if (JNIEnv* Env = GetJavaEnv())
 	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_HideAdBanner);
 	}
}

void AndroidThunkCpp_CloseAdBanner()
{
 	if (JNIEnv* Env = GetJavaEnv())
 	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_CloseAdBanner);
 	}
}

namespace
{
	jobject GJavaAssetManager = NULL;
	AAssetManager* GAssetManagerRef = NULL;
}

AAssetManager * AndroidThunkCpp_GetAssetManager()
{
	if (!GAssetManagerRef)
	{
		if (JNIEnv* Env = GetJavaEnv())
		{
			GJavaAssetManager = Env->CallObjectMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_GetAssetManager);
			Env->NewGlobalRef(GJavaAssetManager);
			GAssetManagerRef = AAssetManager_fromJava(Env, GJavaAssetManager);

		}
	}

	return GAssetManagerRef;
}

void AndroidThunkCpp_Minimize()
{
	if (JNIEnv* Env = GetJavaEnv())
	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_Minimize);
	}
}

void AndroidThunkCpp_ForceQuit()
{
	if (JNIEnv* Env = GetJavaEnv())
	{
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_ForceQuit);
	}
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

	JDef_GameActivity::ClassID = env->FindClass("com/epicgames/ue4/GameActivity");
	CHECK_JNI_RESULT( JDef_GameActivity::ClassID );

	JDef_GameActivity::AndroidThunkJava_ShowConsoleWindow = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_ShowConsoleWindow", "(Ljava/lang/String;)V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_ShowConsoleWindow );
	JDef_GameActivity::AndroidThunkJava_ShowVirtualKeyboardInput = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_ShowVirtualKeyboardInput", "(ILjava/lang/String;Ljava/lang/String;)V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_ShowVirtualKeyboardInput );
	JDef_GameActivity::AndroidThunkJava_LaunchURL = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_LaunchURL", "(Ljava/lang/String;)V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_LaunchURL );
	JDef_GameActivity::AndroidThunkJava_ShowLeaderboard = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_ShowLeaderboard", "(Ljava/lang/String;)V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_ShowLeaderboard );
	JDef_GameActivity::AndroidThunkJava_ShowAchievements = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_ShowAchievements", "()V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_ShowAchievements );
	JDef_GameActivity::AndroidThunkJava_QueryAchievements = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_QueryAchievements", "()V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_QueryAchievements );
	JDef_GameActivity::AndroidThunkJava_ResetAchievements = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_ResetAchievements", "()V");
	JDef_GameActivity::AndroidThunkJava_WriteLeaderboardValue = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_WriteLeaderboardValue", "(Ljava/lang/String;J)V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_WriteLeaderboardValue );
	JDef_GameActivity::AndroidThunkJava_GooglePlayConnect = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_GooglePlayConnect", "()V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_GooglePlayConnect );
	JDef_GameActivity::AndroidThunkJava_WriteAchievement = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_WriteAchievement", "(Ljava/lang/String;F)V");
	CHECK_JNI_RESULT( JDef_GameActivity::AndroidThunkJava_WriteAchievement );
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
	

	// Set up achievement query IDs
	JDef_GameActivity::JavaAchievementClassID = env->FindClass("com/epicgames/ue4/GameActivity$JavaAchievement");
	CHECK_JNI_RESULT( JDef_GameActivity::JavaAchievementClassID );
	JDef_GameActivity::AchievementIDField = env->GetFieldID(JDef_GameActivity::JavaAchievementClassID, "ID", "Ljava/lang/String;");
	CHECK_JNI_RESULT( JDef_GameActivity::AchievementIDField );
	JDef_GameActivity::AchievementProgressField = env->GetFieldID(JDef_GameActivity::JavaAchievementClassID, "Progress", "D");
	CHECK_JNI_RESULT( JDef_GameActivity::AchievementProgressField );
	

	// hook signals
#if UE_BUILD_DEBUG
	if( GAlwaysReportCrash )
#else
	if( !FPlatformMisc::IsDebuggerPresent() || GAlwaysReportCrash )
#endif
	{
		FPlatformMisc::SetCrashHandler(EngineCrashHandler);
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


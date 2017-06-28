/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "ip_shared.h"
#include "GoogleVRAdbUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogInstantPreview, Log, All);

inline ip_static_server_handle InstantPreviewGetServerHandle() {
	// Get the base directory of this plugin
	FString BaseDir = FPaths::Combine(FPaths::EngineDir(), TEXT("Binaries/ThirdParty/GoogleInstantPreview"));
	// Add on the relative location of the third party dll and load it
	FString LibraryPath;
	FString LibraryDir;
#if PLATFORM_WINDOWS
#if PLATFORM_64BITS
	LibraryDir = FPaths::Combine(*BaseDir, TEXT("x64/Release"));
#else  // PLATFORM_32BITS
	LibraryDir = FPaths::Combine(*BaseDir, TEXT("Win32/Release"));
#endif  // PLATFORM_32BITS
	LibraryPath = FPaths::Combine(*LibraryDir, TEXT("ip_shared.dll"));
#elif PLATFORM_MAC
	LibraryDir = FPaths::Combine(*BaseDir, TEXT("Mac/Release"));
	LibraryPath = FPaths::Combine(*LibraryDir, TEXT("libip_shared.dylib"));
#endif // PLATFORM_WINDOWS
	void* instantPreviewLibraryHandle;
	if (!LibraryDir.IsEmpty() && !LibraryPath.IsEmpty())
	{
		FPlatformProcess::AddDllDirectory(*LibraryDir);
		instantPreviewLibraryHandle = FPlatformProcess::GetDllHandle(*LibraryPath);
	}
	else
	{
		instantPreviewLibraryHandle = nullptr;
	}
	FString AdbPath;
	GetAdbPath(AdbPath);
	ip_static_server_handle ServerHandle = ip_static_server_start("0.0.0.0:49838", true, TCHAR_TO_ANSI(*AdbPath));

	if (!ip_static_server_is_adb_available(ServerHandle)) {
		UE_LOG(LogInstantPreview, Warning, TEXT("Adb Not Detected.  Please set the ANDROID_HOME environment variable to your Android SDK directory and restart the Unreal editor."));
	}

	return ServerHandle;
}

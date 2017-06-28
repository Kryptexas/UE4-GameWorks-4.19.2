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

using System.IO;
using UnrealBuildTool;

public class GoogleInstantPreview : ModuleRules
{
	public GoogleInstantPreview(ReadOnlyTargetRules Target)
		: base (Target)
	{
        Type = ModuleType.External;

		string GoogleInstantPreviewTargetDir = Path.Combine(UEBuildConfiguration.UEThirdPartyBinariesDirectory, "GoogleInstantPreview");
		string GoogleInstantPreviewSourceDir = Path.Combine(UEBuildConfiguration.UEThirdPartySourceDirectory, "GoogleInstantPreview");

		PublicIncludePaths.Add(GoogleInstantPreviewSourceDir);

		if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
		{
			string IpSharedPlatform = Target.Platform == UnrealTargetPlatform.Win64 ? "x64" : "Win32";
			string IpSharedLibSourceDir = Path.Combine(GoogleInstantPreviewSourceDir, IpSharedPlatform, "Release");
			PublicLibraryPaths.Add(IpSharedLibSourceDir);
			PublicAdditionalLibraries.Add("ip_shared.lib");
			string[] dllDeps = {
				"ip_shared.dll",
				"libeay32.dll",
				"ssleay32.dll",
				"zlib.dll",
			};
			string IpSharedLibTargetDir = Path.Combine(GoogleInstantPreviewTargetDir, IpSharedPlatform, "Release");
			if (!Directory.Exists(IpSharedLibTargetDir))
			{
				Directory.CreateDirectory(IpSharedLibTargetDir);
			}
			foreach (string dll in dllDeps)
			{
				PublicDelayLoadDLLs.Add(dll);
				string dllPath = Path.Combine(IpSharedLibTargetDir, dll);
				RuntimeDependencies.Add(new RuntimeDependency(dllPath));
            }
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			string[] dylibDeps = {
					"libip_shared.dylib",
					"libgrpc++.dylib",
					"libgpr.dylib",
					"libgrpc.dylib",
			};
			string IpSharedLibTargetDir = Path.Combine(GoogleInstantPreviewTargetDir, "Mac", "Release");
			if (!Directory.Exists(IpSharedLibTargetDir))
			{
				Directory.CreateDirectory(IpSharedLibTargetDir);
			}
			foreach (string dylib in dylibDeps)
			{
				string dylibPath = Path.Combine(IpSharedLibTargetDir, dylib);
                PublicDelayLoadDLLs.Add(dylibPath);
			}
		}
	}
}

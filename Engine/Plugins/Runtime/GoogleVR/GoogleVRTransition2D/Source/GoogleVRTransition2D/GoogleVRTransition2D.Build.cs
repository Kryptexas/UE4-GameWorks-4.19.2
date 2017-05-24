/* Copyright 2016 Google Inc.
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

public class GoogleVRTransition2D : ModuleRules
{
	public GoogleVRTransition2D(ReadOnlyTargetRules Target) : base(Target)
	{
		
		PublicIncludePaths.AddRange(
			new string[] {
				"GoogleVRTransition2D/Public",
				"Runtime/Launch/Public"
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"GoogleVRTransition2D/Private",
				
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",

				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
		
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "GoogleVRTransition2D" });

			string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, BuildConfiguration.RelativeEnginePath);
			AdditionalPropertiesForReceipt.Add(new ReceiptProperty("AndroidPlugin", Path.Combine(PluginPath, "GoogleVRTransition2D_APL.xml")));
		}
	}
}

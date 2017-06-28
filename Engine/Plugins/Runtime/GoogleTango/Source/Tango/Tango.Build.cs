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


namespace UnrealBuildTool.Rules
{
    public class Tango : ModuleRules
    {
        public Tango(ReadOnlyTargetRules Target) : base(Target)
        {
            PrivateIncludePaths.AddRange(
                new string[]
                {
                    "Tango/Private",
                }
            );

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "RHI",
                    "RenderCore",
                    "ShaderCore",
                    "HeadMountedDisplay", // For IMotionController interface.
                    "AndroidPermission",
                    "MRMesh",
                    "TangoRenderer",
                    "TangoSDK",
                    "EcefTools"
                }
            );
            
            PrivateIncludePathModuleNames.AddRange(
                new string[]
                {
                    "Settings" // For editor settings panel.
                }
            );

            if (Target.Platform == UnrealTargetPlatform.Android)
            {
                // Additional dependencies on android...
                PrivateDependencyModuleNames.Add("Launch");

                // Register Plugin Language
                AdditionalPropertiesForReceipt.Add(new ReceiptProperty("AndroidPlugin", Path.Combine(ModuleDirectory, "Tango_APL.xml")));
            }
        }
    }
}

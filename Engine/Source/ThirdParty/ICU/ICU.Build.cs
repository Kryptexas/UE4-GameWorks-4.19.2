// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
using System.IO;

public class ICU : ModuleRules
{
    enum EICULinkType
    {
        None,
        Static,
        Dynamic
    }

	public ICU(TargetInfo Target)
	{
		Type = ModuleType.External;

		string ICURootPath = UEBuildConfiguration.UEThirdPartySourceDirectory + "ICU/icu4c-53_1/";

		// Includes
		PublicSystemIncludePaths.Add(ICURootPath + "include" + "/");

		string PlatformFolderName = Target.Platform.ToString();

        EICULinkType ICULinkType;
        switch (Target.Type)
        {
            case TargetRules.TargetType.Game:
            case TargetRules.TargetType.Client:
            case TargetRules.TargetType.Server:
            case TargetRules.TargetType.RocketGame:
                ICULinkType = EICULinkType.Static;
                break;
            case TargetRules.TargetType.Editor:
            case TargetRules.TargetType.Program:
                ICULinkType = EICULinkType.Dynamic;
                break;
            default:
                ICULinkType = EICULinkType.None;
                break;
        }

        // link statically on Linux until we figure out so location for deployment/local builds
        if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            ICULinkType = EICULinkType.Static;
        }

        string TargetSpecificPath = ICURootPath + PlatformFolderName + "/";
		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32))
		{
			string VSVersionFolderName = "VS" + WindowsPlatform.GetVisualStudioCompilerVersionName();
			TargetSpecificPath += VSVersionFolderName + "/";

			string[] LibraryNameStems =
			{
				"dt",   // Data
				"uc",   // Unicode Common
				"in",   // Internationalization
				"le",   // Layout Engine
				"lx",   // Layout Extensions
				"io"	// Input/Output
			};
			string LibraryNamePostfix = (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT) ?
				"d" : string.Empty;

            // Library Paths
            PublicLibraryPaths.Add(TargetSpecificPath + "lib" + "/");

            switch(ICULinkType)
            {
            case EICULinkType.Static:
			    foreach (string Stem in LibraryNameStems)
			    {
				    string LibraryName = "sicu" + Stem + LibraryNamePostfix + "." + "lib";
				    PublicAdditionalLibraries.Add(LibraryName);
			    }
                break;
            case EICULinkType.Dynamic:
			foreach (string Stem in LibraryNameStems)
			{
                        string LibraryName = "icu" + Stem + LibraryNamePostfix + "." + "lib";
				PublicAdditionalLibraries.Add(LibraryName);
			}

                foreach (string Stem in LibraryNameStems)
                {
                    string LibraryName = "icu" + Stem + LibraryNamePostfix + "53" + "." + "dll";
                    PublicDelayLoadDLLs.Add(LibraryName);
                }
                break;
            }
		}
		else if ((Target.Platform == UnrealTargetPlatform.Mac) ||
			(Target.Platform == UnrealTargetPlatform.Linux))
		{
            string StaticLibraryExtension = "a";
            string DynamicLibraryExtension;
            switch (Target.Platform)
            {
            case UnrealTargetPlatform.Mac:
                DynamicLibraryExtension = "dylib";
                break;
            case UnrealTargetPlatform.Linux:
                TargetSpecificPath += Target.Architecture + "/";
                DynamicLibraryExtension = "so";
                break;
            default: // Should be impossible, but the compiler won't accept not having DynamicLibraryExtension assigned.
                DynamicLibraryExtension = string.Empty;
                break;
            }

			string[] LibraryNameStems =
			{
				"data", // Data
				"uc",   // Unicode Common
				"i18n", // Internationalization
				"le",   // Layout Engine
				"lx",   // Layout Extensions
				"io"	// Input/Output
			};
            string LibraryNamePostfix = (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT) ?
                "d" : string.Empty;

            // Library Paths
			string LibraryNamePrefix = "libicu";

            switch (ICULinkType)
            {
                case EICULinkType.Static:
                    foreach (string Stem in LibraryNameStems)
                    {
                        string LibraryName = LibraryNamePrefix + Stem + LibraryNamePostfix + "." + StaticLibraryExtension;
                        PublicAdditionalLibraries.Add(TargetSpecificPath + "lib/" + LibraryName);
                    }
                    break;
                case EICULinkType.Dynamic:
                    foreach (string Stem in LibraryNameStems)
                    {
                        string LibraryName = LibraryNamePrefix + Stem + LibraryNamePostfix + ".53.1" + "." + DynamicLibraryExtension;
                        string LibraryPath = UEBuildConfiguration.UEThirdPartyBinariesDirectory + "ICU/icu4c-53_1/Mac/" + LibraryName;
                        PublicDelayLoadDLLs.Add(LibraryPath);
                        PublicAdditionalShadowFiles.Add(LibraryPath);
                    }
                    break;
            }
        }
		else if (Target.Platform == UnrealTargetPlatform.PS4)
		{
			string LibraryNamePrefix = "sicu";
			string[] LibraryNameStems =
			{
				"dt",	// Data
				"uc",   // Unicode Common
				"in",	// Internationalization
				"le",   // Layout Engine
				"lx",   // Layout Extensions
				"io"	// Input/Output
			};
			string LibraryNamePostfix = (Target.Configuration == UnrealTargetConfiguration.Debug) ?
				"d" : string.Empty;
			string LibraryExtension = "lib";
			foreach (string Stem in LibraryNameStems)
			{
				string LibraryName = ICURootPath + "PS4/lib/" + LibraryNamePrefix + Stem + LibraryNamePostfix + "." + LibraryExtension;
				PublicAdditionalLibraries.Add(LibraryName);
			}
		}


		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			PublicAdditionalLibraries.Add("dl");
		}

		// common defines
		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
            (Target.Platform == UnrealTargetPlatform.Win32) ||
            (Target.Platform == UnrealTargetPlatform.Linux) ||
            (Target.Platform == UnrealTargetPlatform.Mac ||
			(Target.Platform == UnrealTargetPlatform.PS4)))
		{
			// Definitions
			Definitions.Add("U_USING_ICU_NAMESPACE=0"); // Disables a using declaration for namespace "icu".
			Definitions.Add("U_STATIC_IMPLEMENTATION"); // Necessary for linking to ICU statically.
			Definitions.Add("U_NO_DEFAULT_INCLUDE_UTF_HEADERS=1"); // Disables unnecessary inclusion of headers - inclusions are for ease of use.
			Definitions.Add("UNISTR_FROM_CHAR_EXPLICIT=explicit"); // Makes UnicodeString constructors for ICU character types explicit.
			Definitions.Add("UNISTR_FROM_STRING_EXPLICIT=explicit"); // Makes UnicodeString constructors for "char"/ICU string types explicit.
            Definitions.Add("UCONFIG_NO_TRANSLITERATION=1"); // Disables declarations and compilation of unused ICU transliteration functionality.
		}
		
		if (Target.Platform == UnrealTargetPlatform.PS4)
		{
			// Definitions			
			Definitions.Add(("ICU_NO_USER_DATA_OVERRIDE=1"));
		}
	}
}

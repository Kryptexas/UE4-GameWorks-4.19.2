// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class ICU : ModuleRules
{
	public ICU(TargetInfo Target)
	{
		Type = ModuleType.External;

		string ICURootPath = UEBuildConfiguration.UEThirdPartyDirectory + "ICU/icu4c-51_2/";

		// Includes
		PublicIncludePaths.Add(ICURootPath + "include" + "/");

		string PlatformFolderName = Target.Platform.ToString();

		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32))
		{
			string VSVersionFolderName = "VS" + WindowsPlatform.GetVisualStudioCompilerVersionName();
			string TargetSpecificPath = ICURootPath + PlatformFolderName + "/" + VSVersionFolderName + "/";

			// Library Paths
			PublicLibraryPaths.Add(TargetSpecificPath + "lib" + "/");

			// Libraries
			string LibraryNamePrefix = "sicu";
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
			string LibraryExtension = "lib";

			foreach (string Stem in LibraryNameStems)
			{
				string LibraryName = LibraryNamePrefix + Stem + LibraryNamePostfix + "." + LibraryExtension;
				PublicAdditionalLibraries.Add(LibraryName);
			}
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			string LibraryNamePrefix = "libicu";
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
			string LibraryExtension = "a";

			foreach (string Stem in LibraryNameStems)
			{
				string LibraryName = ICURootPath + "Mac/" + LibraryNamePrefix + Stem + LibraryNamePostfix + "." + LibraryExtension;
				PublicAdditionalLibraries.Add(LibraryName);
			}
		}

		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32) ||
			(Target.Platform == UnrealTargetPlatform.Mac))
		{
			// Definitions
			Definitions.Add("U_USING_ICU_NAMESPACE=0"); // Disables a using declaration for namespace "icu".
			Definitions.Add("U_STATIC_IMPLEMENTATION"); // Necessary for linking to ICU statically.
			Definitions.Add("U_NO_DEFAULT_INCLUDE_UTF_HEADERS=1"); // Disables unnecessary inclusion of headers - inclusions are for ease of use.
			Definitions.Add("UNISTR_FROM_CHAR_EXPLICIT=explicit"); // Makes UnicodeString constructors for ICU character types explicit.
			Definitions.Add("UNISTR_FROM_STRING_EXPLICIT=explicit"); // Makes UnicodeString constructors for "char"/ICU string types explicit.
		}
	}
}

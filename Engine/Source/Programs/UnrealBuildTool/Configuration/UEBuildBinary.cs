// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Xml;

namespace UnrealBuildTool
{	
	/// <summary>
	/// The type of binary to build
	/// </summary>
	public enum UEBuildBinaryType
	{
		Executable,
		DynamicLinkLibrary,
		StaticLibrary
	};

	/// <summary>
	/// UEBuildBinary configuration
	/// Configuration class for a UEBuildBinary.
	/// Exposes the configuration values of the BuildBinary class without exposing the functions.
	/// </summary>
	public class UEBuildBinaryConfiguration
	{
		/// <summary>
		/// The type of binary to build
		/// </summary>
		public UEBuildBinaryType Type;

		/// <summary>
		/// The output file path. This must be set before a binary can be built using it.
		/// </summary>
		public string OutputFilePath;

		/// <summary>
		/// Original output filepath. This is the original binary name before hot-reload suffix has been appended to it.
		/// </summary>
		public string OriginalOutputFilePath;

		/// <summary>
		/// The intermediate directory for this binary. Modules should create separate intermediate directories below this. Must be set before a binary can be built using it.
		/// </summary>
		public string IntermediateDirectory;

		/// <summary>
		/// If true, build exports lib
		/// </summary>
		public bool bAllowExports = false;
		
		/// <summary>
		/// If true, create a separate import library
		/// </summary>
		public bool bCreateImportLibrarySeparately = false;

		/// <summary>
		/// If true, include dependent libraries in the static library being built
		/// </summary>
		public bool bIncludeDependentLibrariesInLibrary = false;

		/// <summary>
		/// If false, this binary will not be compiled and it is only used to set up link environments
		/// </summary>
		public bool bAllowCompilation = true;

        /// <summary>
        /// For most binaries, this is false. If this is a cross-platform binary build for a specific platform (for example XB1 DLL for a windows editor) this will be true.
        /// </summary>
        public bool bIsCrossTarget = false;

        /// <summary>
		/// If true, the binary is being compiled as a monolithic build
		/// </summary>
		public bool bCompileMonolithic = false;

		/// <summary>
		/// The build target configuration being compiled
		/// </summary>
		public UnrealTargetConfiguration TargetConfiguration = UnrealTargetConfiguration.Development;

		/// <summary>
		/// The name of the target being compiled
		/// </summary>
		public string TargetName = "";

		/// <summary>
		/// The projectfile path
		/// </summary>
		public string ProjectFilePath = "";
		
		/// <summary>
		/// List of modules to link together into this executable
		/// </summary>
		public List<string> ModuleNames = new List<string>();

		/// <summary>
		/// The configuration class for a binary build.
		/// </summary>
		/// <param name="InType"></param>
		/// <param name="InOutputFilePath"></param>
		/// <param name="bInAllowExports"></param>
		/// <param name="bInCreateImportLibrarySeparately"></param>
        /// <param name="bInIsCrossTarget">For most binaries, this is false. If this is a cross-platform binary build for a specific platform (for example XB1 DLL for a windows editor) this will be true.</param>
        /// <param name="InProjectFilePath"></param>
		/// <param name="InModuleNames"></param>
		public UEBuildBinaryConfiguration(
				UEBuildBinaryType InType,
				string InOutputFilePath = null,
				string InIntermediateDirectory = null,
				bool bInAllowExports = false,
				bool bInCreateImportLibrarySeparately = false,
				bool bInIncludeDependentLibrariesInLibrary = false,
				bool bInAllowCompilation = true,
                bool bInIsCrossTarget = false,
                bool bInCompileMonolithic = false,
				UnrealTargetConfiguration InTargetConfiguration = UnrealTargetConfiguration.Development,
				string InTargetName = "",
				string InProjectFilePath = "",
				List<string> InModuleNames = null
			)
		{
			Type = InType;
			OutputFilePath = InOutputFilePath;
			IntermediateDirectory = InIntermediateDirectory;
			bAllowExports = bInAllowExports;
			bCreateImportLibrarySeparately = bInCreateImportLibrarySeparately;
			bIncludeDependentLibrariesInLibrary = bInIncludeDependentLibrariesInLibrary;
			bAllowCompilation = bInAllowCompilation;
            bIsCrossTarget = bInIsCrossTarget;
            bCompileMonolithic = bInCompileMonolithic;
			TargetConfiguration = InTargetConfiguration;
			TargetName = InTargetName;
			ProjectFilePath = InProjectFilePath;
			ModuleNames = InModuleNames;
		}
	}


	/// <summary>
	/// A binary built by UBT.
	/// </summary>
	public abstract class UEBuildBinary
	{
		/// <summary>
		/// The target which owns this binary.
		/// </summary>
		public UEBuildTarget Target;

		/// <summary>
		/// The build binary configuration data
		/// </summary>
		public UEBuildBinaryConfiguration Config = null;

		/// <summary>
		/// Create an instance of the class with the given configuration data
		/// </summary>
		/// <param name="InConfig">The build binary configuration to initialize the class with</param>
		public UEBuildBinary( UEBuildTarget InTarget, UEBuildBinaryConfiguration InConfig)
		{
			Debug.Assert(InConfig.OutputFilePath != null && InConfig.IntermediateDirectory != null);
			Target = InTarget;
			Config = InConfig;
		}

		/// <summary>
		/// Called to resolve module names and uniquely bind modules to a binary.
		/// </summary>
		/// <param name="BuildTarget">The build target the modules are being bound for</param>
		/// <param name="Target">The target info</param>
		public virtual void BindModules() {}

		/// <summary>
		/// Builds the binary.
		/// </summary>
		/// <param name="CompileEnvironment">The environment to compile the binary in</param>
		/// <param name="LinkEnvironment">The environment to link the binary in</param>
		/// <returns></returns>
		public abstract IEnumerable<FileItem> Build(CPPEnvironment CompileEnvironment,LinkEnvironment LinkEnvironment);

		/// <summary>
		/// Writes an XML summary of the build environment for this binary
		/// </summary>
		/// <param name="CompileEnvironment">The environment to compile the binary in</param>
		/// <param name="LinkEnvironment">The environment to link the binary in</param>
		/// <param name="Writer">The output XML writer</param>
		/// <returns></returns>
		public abstract void WriteBuildEnvironment(CPPEnvironment CompileEnvironment, LinkEnvironment LinkEnvironment, XmlWriter Writer);

		/// <summary>
		/// Called to allow the binary to modify the link environment of a different binary containing 
		/// a module that depends on a module in this binary. */
		/// </summary>
		/// <param name="DependentLinkEnvironment">The link environment of the dependency</param>
		public virtual void SetupDependentLinkEnvironment(ref LinkEnvironment DependentLinkEnvironment) {}

		/// <summary>
		/// Called to allow the binary to to determine if it matches the Only module "short module name".
		/// </summary>
		/// <param name="OnlyModules"></param>
		/// <returns>The OnlyModule if found, null if not</returns>
        public virtual OnlyModule FindOnlyModule(List<OnlyModule> OnlyModules)
        {
            return null;
        }

		/// <summary>
		/// Generates a list of all modules referenced by this binary
		/// </summary>
		/// <param name="bIncludeDynamicallyLoaded">True if dynamically loaded modules (and all of their dependent modules) should be included.</param>
		/// <param name="bForceCircular">True if circular dependencies should be process</param>
		/// <returns>List of all referenced modules</returns>
		public virtual List<UEBuildModule> GetAllDependencyModules( bool bIncludeDynamicallyLoaded, bool bForceCircular )
		{
			return new List<UEBuildModule>();
		}

		/// <summary>
		/// Process all modules that aren't yet bound, creating binaries for modules that don't yet have one (if needed),
		/// and updating modules for circular dependencies.
		/// </summary>
		/// <param name="Target">The target we are currently building</param>
		/// <param name="bBuildOnlyModules">True to build only specific modules, false for all</param>
		/// <param name="ModulesToBuild">The specific modules to build</param>
		/// <returns>List of newly-created binaries (may be empty)</returns>
		public virtual List<UEBuildBinary> ProcessUnboundModules()
		{
			return null;
		}

		/// <summary>
		/// Sets whether to create a separate import library to resolve circular dependencies for this binary
		/// </summary>
		/// <param name="bInCreateImportLibrarySeparately">True to create a separate import library</param>
		public virtual void SetCreateImportLibrarySeparately( bool bInCreateImportLibrarySeparately )
		{
		}

		/// <summary>
		/// Sets whether to include dependent libraries when building a static library
		/// </summary>
		/// <param name="bInIncludeDependentLibrariesInLibrary">True to include dependent libraries</param>
		public virtual void SetIncludeDependentLibrariesInLibrary(bool bInIncludeDependentLibrariesInLibrary)
		{
		}

		/// <summary>
		/// Adds a module to the binary.
		/// </summary>
		/// <param name="ModuleName">The module to add</param>
		public virtual void AddModule( string ModuleName )
		{
		}

		/// <summary>
		/// Helper function to get the console app BinaryName-Cmd.exe filename based on the binary filename.
		/// </summary>
		/// <param name="BinaryPath">Full path to the binary exe.</param>
		/// <returns></returns>
		public static string GetAdditionalConsoleAppPath(string BinaryPath)
		{
			return Path.Combine(Path.GetDirectoryName(BinaryPath), Path.GetFileNameWithoutExtension(BinaryPath) + "-Cmd" + Path.GetExtension(BinaryPath));
		}
	};

	/// <summary>
	/// A binary built by UBT from a set of C++ modules.
	/// </summary>
	public class UEBuildBinaryCPP : UEBuildBinary
	{
		public HashSet<string> ModuleNames
		{
			get;
			private set;
		}
		private bool bCreateImportLibrarySeparately;
		private bool bIncludeDependentLibrariesInLibrary;

		/// <summary>
		/// Create an instance initialized to the given configuration
		/// </summary>
		/// <param name="InConfig">The build binary configuration to initialize the instance to</param>
		public UEBuildBinaryCPP( UEBuildTarget InTarget, UEBuildBinaryConfiguration InConfig )
			: base( InTarget, InConfig )
		{
			ModuleNames = new HashSet<string>(InConfig.ModuleNames);
			bCreateImportLibrarySeparately = InConfig.bCreateImportLibrarySeparately;
			bIncludeDependentLibrariesInLibrary = InConfig.bIncludeDependentLibrariesInLibrary;
		}

		/// <summary>
		/// Adds a module to the binary.
		/// </summary>
		/// <param name="ModuleName">The module to add</param>
		public override void AddModule(string ModuleName)
		{
			if( !ModuleNames.Contains( ModuleName ) )
			{
				ModuleNames.Add( ModuleName );
			}
		}

		// UEBuildBinary interface.

		/// <summary>
		/// Called to resolve module names and uniquely bind modules to a binary.
		/// </summary>
		/// <param name="BuildTarget">The build target the modules are being bound for</param>
		/// <param name="Target">The target info</param>
		public override void BindModules()
		{
			foreach(var ModuleName in ModuleNames)
			{
				var Module = Target.FindOrCreateModuleByName(ModuleName);
				if(Module.Binary != null)
				{
					throw new BuildException("Module \"{0}\" linked into both {1} and {2}, which creates ambiguous linkage for dependents.", ModuleName, Module.Binary.Config.OutputFilePath, Config.OutputFilePath);
				}
				Module.Binary = this;
				Module.bIncludedInTarget = true;

				// We set whether the binary is being compiled monolithic here to know later - specifically
				// when we are determining whether to use SharedPCHs or not for static lib builds of plugins.
				Config.bCompileMonolithic = Target.ShouldCompileMonolithic();

				// We also need to know what the actual build target configuration is later in the process
				// where we do not have access to the Target itself... This is for generating the paths
				// to the plugins.
				Config.TargetConfiguration = Target.Configuration;
				Config.TargetName = Target.GetAppName();

				if ((Target.Rules == null) || (Target.Rules.bOutputToEngineBinaries == false))
				{
					// Fix up the binary path if this is module specifies an alternate output directory
					Config.OutputFilePath = Module.FixupOutputPath(Config.OutputFilePath);
				}
			}
		}

		/// <summary>
		/// Generates a list of all modules referenced by this binary
		/// </summary>
		/// <param name="bIncludeDynamicallyLoaded">True if dynamically loaded modules (and all of their dependent modules) should be included.</param>
		/// <param name="bForceCircular">True if circular dependencies should be process</param>
		/// <returns>List of all referenced modules</returns>
		public override List<UEBuildModule> GetAllDependencyModules(bool bIncludeDynamicallyLoaded, bool bForceCircular)
		{
			var OrderedModules = new List<UEBuildModule>();
			var ReferencedModules = new Dictionary<string, UEBuildModule>( StringComparer.InvariantCultureIgnoreCase );
			foreach( var ModuleName in ModuleNames )
			{
				if( !ReferencedModules.ContainsKey( ModuleName ) )
				{
					var Module = Target.GetModuleByName( ModuleName );
					ReferencedModules[ ModuleName ] = Module;

					Module.GetAllDependencyModules(ref ReferencedModules, ref OrderedModules, bIncludeDynamicallyLoaded, bForceCircular);

					OrderedModules.Add( Module );
				}
			}

			return OrderedModules;
		}

		/// <summary>
		/// Process all modules that aren't yet bound, creating binaries for modules that don't yet have one (if needed),
		/// and updating modules for circular dependencies.
		/// </summary>
		/// <param name="Target">The target we are currently building</param>
		/// <param name="bBuildOnlyModules">True to build only specific modules, false for all</param>
		/// <param name="ModulesToBuild">The specific modules to build</param>
		/// <returns>List of newly-created binaries (may be empty)</returns>
		public override List<UEBuildBinary> ProcessUnboundModules()
		{
			var Binaries = new Dictionary<string, UEBuildBinary>( StringComparer.InvariantCultureIgnoreCase );
			foreach( var ModuleName in ModuleNames )
			{
				var Module = Target.FindOrCreateModuleByName(ModuleName);
				Module.RecursivelyProcessUnboundModules( Target, ref Binaries, this );
			}

			// Now build a final list of newly-created binaries that were bound to.  The hash may contain duplicates, so
			// we filter those out here.
			var BinaryList = new List<UEBuildBinary>();
			foreach( var CurBinary in Binaries.Values )
			{
				// Never include ourselves in the new binary list (monolithic case)
				if( CurBinary != this )
				{
					if( !BinaryList.Contains( CurBinary ) )
					{
						BinaryList.Add( CurBinary );
					}
				}
			}
			return BinaryList;
		}

		/// <summary>
		/// Sets whether to create a separate import library to resolve circular dependencies for this binary
		/// </summary>
		/// <param name="bInCreateImportLibrarySeparately">True to create a separate import library</param>
		public override void SetCreateImportLibrarySeparately(bool bInCreateImportLibrarySeparately)
		{
			bCreateImportLibrarySeparately = bInCreateImportLibrarySeparately;
		}

		/// <summary>
		/// Sets whether to include dependent libraries when building a static library
		/// </summary>
		/// <param name="bInIncludeDependentLibrariesInLibrary"></param>
		public override void SetIncludeDependentLibrariesInLibrary(bool bInIncludeDependentLibrariesInLibrary)
		{
			bIncludeDependentLibrariesInLibrary = bInIncludeDependentLibrariesInLibrary;
		}

		/// <summary>
		/// Builds the binary.
		/// </summary>
		/// <param name="CompileEnvironment">The environment to compile the binary in</param>
		/// <param name="LinkEnvironment">The environment to link the binary in</param>
		/// <returns></returns>
		public override IEnumerable<FileItem> Build(CPPEnvironment CompileEnvironment, LinkEnvironment LinkEnvironment)
		{
			// Determine the type of binary we're linking.
			switch (Config.Type)
			{
				case UEBuildBinaryType.DynamicLinkLibrary:
					CompileEnvironment.Config.bIsBuildingDLL = true;
					CompileEnvironment.Config.bIsBuildingLibrary = false;
					break;
				case UEBuildBinaryType.StaticLibrary:
					CompileEnvironment.Config.bIsBuildingDLL = false;
					CompileEnvironment.Config.bIsBuildingLibrary = true;
					break;
				default:
					CompileEnvironment.Config.bIsBuildingDLL = false;
					CompileEnvironment.Config.bIsBuildingLibrary = false;
					break;
			};


			var OutputFiles = new List<FileItem>();

			var BinaryCompileEnvironment = new CPPEnvironment(CompileEnvironment);
			var BinaryLinkEnvironment = new LinkEnvironment(LinkEnvironment);

			// Process each module that is linked into the binary.
			var BinaryDependencies = new List<UEBuildBinary>();
			var LinkEnvironmentVisitedModules = new Dictionary<UEBuildModule, bool>();

			// @Hack: This to prevent UHT from listing CoreUObject.generated.inl as its dependency.
			// We flag the compile environment when we build UHT so that we don't need to check
			// this for each file when generating their dependencies.
			BinaryCompileEnvironment.bHackHeaderGenerator = ModuleNames.Contains("UnrealHeaderTool");

			// Set the original file name macro; used in PCLaunch.rc to set the binary metadata fields.
			var OriginalFilename = !String.IsNullOrEmpty(Config.OriginalOutputFilePath) ?
				Path.GetFileName(Config.OriginalOutputFilePath) :
				Path.GetFileName(Config.OutputFilePath);
			BinaryCompileEnvironment.Config.Definitions.Add("ORIGINAL_FILE_NAME=\"" + OriginalFilename + "\"");

			foreach (var ModuleName in ModuleNames)
			{
				var Module = Target.GetModuleByName(ModuleName);

				// Compile each module.
				Log.TraceVerbose("Compile module: " + ModuleName);

				var LinkInputFiles = Module.Compile(CompileEnvironment, BinaryCompileEnvironment, Config.bCompileMonolithic);

				// NOTE: Because of 'Shared PCHs', in monolithic builds the same PCH file may appear as a link input
				// multiple times for a single binary.  We'll check for that here, and only add it once.  This avoids
				// a linker warning about redundant .obj files. The same is true for .res files. We only take the first one.
				bool bHasResFile = false;
				foreach (var InputFile in BinaryLinkEnvironment.InputFiles)
				{
					if (InputFile.AbsolutePath.EndsWith(".res")) 
					{ 
						bHasResFile = true;
						break;
					}
				}

				foreach( var LinkInputFile in LinkInputFiles )
				{
					bool bIsResourceFile = LinkInputFile.AbsolutePath.EndsWith(".res");
					if( !BinaryLinkEnvironment.InputFiles.Contains( LinkInputFile ) && (!bHasResFile || !bIsResourceFile))
					{
						BinaryLinkEnvironment.InputFiles.Add( LinkInputFile );
						bHasResFile = bHasResFile || bIsResourceFile;

					}
				}

				// Allow the module to modify the link environment for the binary.
				Module.SetupPrivateLinkEnvironment(ref BinaryLinkEnvironment,ref BinaryDependencies,ref LinkEnvironmentVisitedModules);
			}

			// Allow the binary dependencies to modify the link environment.
			foreach(var BinaryDependency in BinaryDependencies)
			{
				BinaryDependency.SetupDependentLinkEnvironment(ref BinaryLinkEnvironment);
			}


			// Set the link output file.
			BinaryLinkEnvironment.Config.OutputFilePath = Config.OutputFilePath;

			// Set whether the link is allowed to have exports.
			BinaryLinkEnvironment.Config.bHasExports = Config.bAllowExports;

			// Set the output folder for intermediate files
			BinaryLinkEnvironment.Config.IntermediateDirectory = Config.IntermediateDirectory;

			// Put the non-executable output files (PDB, import library, etc) in the same directory as the production
			BinaryLinkEnvironment.Config.OutputDirectory = Path.GetDirectoryName(Config.OutputFilePath);

			// Determine the type of binary we're linking.
			switch (Config.Type)
			{
				case UEBuildBinaryType.DynamicLinkLibrary:
					BinaryLinkEnvironment.Config.bIsBuildingDLL = true;
					BinaryLinkEnvironment.Config.bIsBuildingLibrary = false;
					break;
				case UEBuildBinaryType.StaticLibrary:
					BinaryLinkEnvironment.Config.bIsBuildingDLL = false;
					BinaryLinkEnvironment.Config.bIsBuildingLibrary = true;
					break;
				default:
					BinaryLinkEnvironment.Config.bIsBuildingDLL = false;
					BinaryLinkEnvironment.Config.bIsBuildingLibrary = false;
					break;
			};

			if( ProjectFileGenerator.bGenerateProjectFiles )
			{
				// We're generating projects.  Since we only need include paths and definitions, there is no need
				// to go ahead and run through the linking logic.
				OutputFiles = BinaryLinkEnvironment.InputFiles;
			}
			else if( BuildConfiguration.bEnableCodeAnalysis )
			{
				// We're only analyzing code, so we won't actually link any executables.  Instead, our output
				// files will simply be the .obj files that were compiled during static analysis.
				OutputFiles = BinaryLinkEnvironment.InputFiles;
			}
			else
			{
				if(bCreateImportLibrarySeparately)
				{
					// Mark the link environment as cross-referenced.
					BinaryLinkEnvironment.Config.bIsCrossReferenced = true;

					if (BinaryLinkEnvironment.Config.TargetPlatform != CPPTargetPlatform.Mac)
					{
						// Create the import library.
						OutputFiles.Add(BinaryLinkEnvironment.LinkExecutable(true));
					}
				}

				BinaryLinkEnvironment.Config.bIncludeDependentLibrariesInLibrary = bIncludeDependentLibrariesInLibrary;

				// Link the binary.
				FileItem Executable = BinaryLinkEnvironment.LinkExecutable(false);
				OutputFiles.Add(Executable);

				// Produce additional console app if requested
				if (BinaryLinkEnvironment.Config.CanProduceAdditionalConsoleApp && UEBuildConfiguration.bBuildEditor)
				{					
					// Produce additional binary but link it as a console app
					LinkEnvironment ConsoleAppLinkEvironment = new LinkEnvironment(BinaryLinkEnvironment);					
					ConsoleAppLinkEvironment.Config.bIsBuildingConsoleApplication = true;
					ConsoleAppLinkEvironment.Config.WindowsEntryPointOverride = "WinMainCRTStartup";		// For WinMain() instead of "main()" for Launch module
					ConsoleAppLinkEvironment.Config.OutputFilePath = GetAdditionalConsoleAppPath(ConsoleAppLinkEvironment.Config.OutputFilePath);

					// Link the console app executable
					OutputFiles.Add(ConsoleAppLinkEvironment.LinkExecutable(false));
				}

				// if building for Mac on a Mac, use actions to finalize the builds (otherwise, we use Deploy)
				if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac &&
					BinaryLinkEnvironment.Config.TargetPlatform == CPPTargetPlatform.Mac)
				{
					if (!BinaryLinkEnvironment.Config.bIsBuildingDLL && !BinaryLinkEnvironment.Config.bIsBuildingLibrary)
					{
						MacToolChain Mac = (MacToolChain)UEToolChain.GetPlatformToolChain(CPPTargetPlatform.Mac);
						FileItem FixDylibOutputFile = Mac.FixDylibDependencies(BinaryLinkEnvironment, Executable);
						OutputFiles.Add(FixDylibOutputFile);
						if (!BinaryLinkEnvironment.Config.bIsBuildingConsoleApplication)
						{
							OutputFiles.Add(Mac.CreateAppBundle(BinaryLinkEnvironment, Executable, FixDylibOutputFile));
						}
					}
				}
			}
			
			return OutputFiles;
		}

		/// <summary>
		/// Writes an XML summary of the build environment for this binary
		/// </summary>
		/// <param name="CompileEnvironment">The environment to compile the binary in</param>
		/// <param name="LinkEnvironment">The environment to link the binary in</param>
		/// <param name="Writer">The output XML writer</param>
		/// <returns></returns>
		public override void WriteBuildEnvironment(CPPEnvironment CompileEnvironment, LinkEnvironment LinkEnvironment, XmlWriter Writer)
		{
			// Output documentation for each module
			Writer.WriteStartElement("binary");
			Writer.WriteAttributeString("name", Path.GetFileName(Config.OutputFilePath));
			Writer.WriteAttributeString("type", "cpp");
			foreach (var ModuleName in ModuleNames)
			{
				UEBuildModule Module = Target.GetModuleByName(ModuleName);
				Module.WriteBuildEnvironment(CompileEnvironment, Writer);
			}
			Writer.WriteEndElement();
		}


		/// <summary>
		/// Called to allow the binary to modify the link environment of a different binary containing 
		/// a module that depends on a module in this binary.
		/// </summary>
		/// <param name="DependentLinkEnvironment">The link environment of the dependency</param>
		public override void SetupDependentLinkEnvironment(ref LinkEnvironment DependentLinkEnvironment)
		{
			string LibraryFileName;
			if (Config.Type == UEBuildBinaryType.StaticLibrary || DependentLinkEnvironment.Config.TargetPlatform == CPPTargetPlatform.Mac)
			{
				LibraryFileName = Config.OutputFilePath;
			}
			else
			{
				LibraryFileName = Path.Combine(Config.IntermediateDirectory, Path.GetFileNameWithoutExtension(Config.OutputFilePath) + ".lib");
			}
			DependentLinkEnvironment.Config.AdditionalLibraries.Add(LibraryFileName);
		}

		/// <summary>
		/// Called to allow the binary to to determine if it matches the Only module "short module name".
		/// </summary>
		/// <param name="OnlyModules"></param>
		/// <returns>The OnlyModule if found, null if not</returns>
		public override OnlyModule FindOnlyModule(List<OnlyModule> OnlyModules)
		{
			foreach (var ModuleName in ModuleNames)
			{
				foreach (var OnlyModule in OnlyModules)
				{
					if (OnlyModule.OnlyModuleName.ToLower() == ModuleName.ToLower())
					{
						return OnlyModule;
					}
				}
			}
			return null;
		}

		// Object interface.

		/// <summary>
		/// ToString implementation
		/// </summary>
		/// <returns>Returns the OutputFilePath for this binary</returns>
		public override string ToString()
		{
			return Config.OutputFilePath;
		}
	};

	/// <summary>
	/// A DLL built by MSBuild from a C# project.
	/// </summary>
	public class UEBuildBinaryCSDLL : UEBuildBinary
	{
		/// <summary>
		/// Create an instance initialized to the given configuration
		/// </summary>
		/// <param name="InConfig">The build binary configuration to initialize the instance to</param>
		public UEBuildBinaryCSDLL(UEBuildTarget InTarget, UEBuildBinaryConfiguration InConfig)
			: base(InTarget, InConfig)
		{
		}

		/// <summary>
		/// Builds the binary.
		/// </summary>
		/// <param name="CompileEnvironment">The environment to compile the binary in</param>
		/// <param name="LinkEnvironment">The environment to link the binary in</param>
		/// <returns></returns>
		public override IEnumerable<FileItem> Build(CPPEnvironment CompileEnvironment, LinkEnvironment LinkEnvironment)
		{
			var ProjectCSharpEnviroment = new CSharpEnvironment();
			if (LinkEnvironment.Config.TargetConfiguration == CPPTargetConfiguration.Debug)
			{ 
				ProjectCSharpEnviroment.TargetConfiguration = CSharpTargetConfiguration.Debug;
			}
			else
			{
				ProjectCSharpEnviroment.TargetConfiguration = CSharpTargetConfiguration.Development;
			}
			ProjectCSharpEnviroment.EnvironmentTargetPlatform = LinkEnvironment.Config.TargetPlatform;

			// Currently only supported by windows...
			UEToolChain.GetPlatformToolChain(CPPTargetPlatform.Win64).CompileCSharpProject(
				ProjectCSharpEnviroment, Config.ProjectFilePath, Config.OutputFilePath);

			return new FileItem[] { FileItem.GetItemByPath(Config.OutputFilePath) };
		}

		/// <summary>
		/// Writes an XML summary of the build environment for this binary
		/// </summary>
		/// <param name="CompileEnvironment">The environment to compile the binary in</param>
		/// <param name="LinkEnvironment">The environment to link the binary in</param>
		/// <param name="Writer">The output XML writer</param>
		/// <returns></returns>
		public override void WriteBuildEnvironment(CPPEnvironment CompileEnvironment, LinkEnvironment LinkEnvironment, XmlWriter Writer)
		{
			Writer.WriteStartElement("binary");
			Writer.WriteAttributeString("name", Path.GetFileName(Config.OutputFilePath));
			Writer.WriteAttributeString("type", "cs");

			Writer.WriteStartElement("project");
			Writer.WriteAttributeString("path", Config.ProjectFilePath);
			Writer.WriteEndElement();

			Writer.WriteEndElement();
		}
	};
}

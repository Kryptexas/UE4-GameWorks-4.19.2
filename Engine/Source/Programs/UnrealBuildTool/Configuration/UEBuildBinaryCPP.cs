using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnrealBuildTool
{
	/// <summary>
	/// A binary built by UBT from a set of C++ modules.
	/// </summary>
	class UEBuildBinaryCPP : UEBuildBinary
	{
		public readonly List<UEBuildModule> Modules = new List<UEBuildModule>();
		private bool bCreateImportLibrarySeparately;
		private bool bIncludeDependentLibrariesInLibrary;
		private List<string> DependentLinkLibraries;

		/// <summary>
		/// Create an instance initialized to the given configuration
		/// </summary>
		/// <param name="InConfig">The build binary configuration to initialize the instance to</param>
		public UEBuildBinaryCPP(UEBuildBinaryConfiguration InConfig)
			: base(InConfig)
		{
			bCreateImportLibrarySeparately = InConfig.bCreateImportLibrarySeparately;
			bIncludeDependentLibrariesInLibrary = InConfig.bIncludeDependentLibrariesInLibrary;
		}

		/// <summary>
		/// Adds a module to the binary.
		/// </summary>
		/// <param name="Module">The module to add</param>
		public override void AddModule(UEBuildModule Module)
		{
			if (!Modules.Contains(Module))
			{
				Modules.Add(Module);
			}
		}

		// UEBuildBinary interface.

		/// <summary>
		/// Creates all the modules referenced by this target.
		/// </summary>
		public override void CreateAllDependentModules(UEBuildModule.CreateModuleDelegate CreateModule)
		{
			if (Config.bHasModuleRules)
			{
				foreach (UEBuildModule Module in Modules)
				{
					Module.RecursivelyCreateModules(CreateModule, "Target");
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
			List<UEBuildModule> ReferencedModules = new List<UEBuildModule>();
			HashSet<UEBuildModule> IgnoreReferencedModules = new HashSet<UEBuildModule>();

			foreach (UEBuildModule Module in Modules)
			{
				if (!IgnoreReferencedModules.Contains(Module))
				{
					IgnoreReferencedModules.Add(Module);

					Module.GetAllDependencyModules(ReferencedModules, IgnoreReferencedModules, bIncludeDynamicallyLoaded, bForceCircular, bOnlyDirectDependencies: false);

					ReferencedModules.Add(Module);
				}
			}

			return ReferencedModules;
		}

		/// <summary>
		/// Generates a list of all modules referenced by this binary
		/// </summary>
		/// <param name="ReferencedBy">Map of module to the module that referenced it</param>
		/// <returns>List of all referenced modules</returns>
		public override void FindModuleReferences(Dictionary<UEBuildModule, UEBuildModule> ReferencedBy)
		{
			List<UEBuildModule> ReferencedModules = new List<UEBuildModule>();
			foreach(UEBuildModule Module in Modules)
			{
				ReferencedModules.Add(Module);
				ReferencedBy.Add(Module, null);
			}

			List<UEBuildModule> DirectlyReferencedModules = new List<UEBuildModule>();
			HashSet<UEBuildModule> VisitedModules = new HashSet<UEBuildModule>();
			for(int Idx = 0; Idx < ReferencedModules.Count; Idx++)
			{
				UEBuildModule SourceModule = ReferencedModules[Idx];

				// Find all the direct references from this module
				DirectlyReferencedModules.Clear();
				SourceModule.GetAllDependencyModules(DirectlyReferencedModules, VisitedModules, false, false, true);

				// Set up the references for all the new modules
				foreach(UEBuildModule DirectlyReferencedModule in DirectlyReferencedModules)
				{
					if(!ReferencedBy.ContainsKey(DirectlyReferencedModule))
					{
						ReferencedBy.Add(DirectlyReferencedModule, SourceModule);
						ReferencedModules.Add(DirectlyReferencedModule);
					}
				}
			}
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

		bool IsBuildingDll(UEBuildBinaryType Type)
		{
			return Type == UEBuildBinaryType.DynamicLinkLibrary;
		}

		bool IsBuildingLibrary(UEBuildBinaryType Type)
		{
			return Type == UEBuildBinaryType.StaticLibrary;
		}

		/// <summary>
		/// Builds the binary.
		/// </summary>
		/// <param name="Target">The target rules</param>
		/// <param name="ToolChain">The toolchain to use for building</param>
		/// <param name="CompileEnvironment">The environment to compile the binary in</param>
		/// <param name="LinkEnvironment">The environment to link the binary in</param>
		/// <param name="SharedPCHs">List of available shared PCHs</param>
		/// <param name="ActionGraph">The graph to add build actions to</param>
		/// <returns>Set of build products</returns>
		public override IEnumerable<FileItem> Build(ReadOnlyTargetRules Target, UEToolChain ToolChain, CppCompileEnvironment CompileEnvironment, LinkEnvironment LinkEnvironment, List<PrecompiledHeaderTemplate> SharedPCHs, ActionGraph ActionGraph)
		{
			// Setup linking environment.
			LinkEnvironment BinaryLinkEnvironment = SetupBinaryLinkEnvironment(Target, ToolChain, LinkEnvironment, CompileEnvironment, SharedPCHs, ActionGraph);

			// If we're generating projects, we only need include paths and definitions, there is no need to run the linking logic.
			if (ProjectFileGenerator.bGenerateProjectFiles)
			{
				return BinaryLinkEnvironment.InputFiles;
			}

			// If linking is disabled, our build products are just the compiled object files
			if (Target.bDisableLinking)
			{
				return BinaryLinkEnvironment.InputFiles;
			}

			// Return linked files.
			return SetupOutputFiles(ToolChain, CompileEnvironment, BinaryLinkEnvironment, ActionGraph);
		}

		/// <summary>
		/// Called to allow the binary to modify the link environment of a different binary containing 
		/// a module that depends on a module in this binary.
		/// </summary>
		/// <param name="DependentLinkEnvironment">The link environment of the dependency</param>
		public override void SetupDependentLinkEnvironment(LinkEnvironment DependentLinkEnvironment)
		{
			// Cache the list of libraries in the dependent link environment between calls. We typically run this code path many times for each module.
			if (DependentLinkLibraries == null)
			{
				DependentLinkLibraries = new List<string>();
				foreach (FileReference OutputFilePath in Config.OutputFilePaths)
				{
					FileReference LibraryFileName;
					if (Config.Type == UEBuildBinaryType.StaticLibrary || DependentLinkEnvironment.Platform == CppPlatform.Mac || DependentLinkEnvironment.Platform == CppPlatform.Linux)
					{
						LibraryFileName = OutputFilePath;
					}
					else
					{
						LibraryFileName = FileReference.Combine(Config.IntermediateDirectory, OutputFilePath.GetFileNameWithoutExtension() + ".lib");
					}
					DependentLinkLibraries.Add(LibraryFileName.FullName);
				}
			}
			DependentLinkEnvironment.AdditionalLibraries.AddRange(DependentLinkLibraries);
		}

		/// <summary>
		/// Called to allow the binary to to determine if it matches the Only module "short module name".
		/// </summary>
		/// <param name="OnlyModules"></param>
		/// <returns>The OnlyModule if found, null if not</returns>
		public override OnlyModule FindOnlyModule(List<OnlyModule> OnlyModules)
		{
			foreach (UEBuildModule Module in Modules)
			{
				foreach (OnlyModule OnlyModule in OnlyModules)
				{
					if (OnlyModule.OnlyModuleName.ToLower() == Module.Name.ToLower())
					{
						return OnlyModule;
					}
				}
			}
			return null;
		}

		public override List<UEBuildModule> FindGameModules()
		{
			List<UEBuildModule> GameModules = new List<UEBuildModule>();
			foreach (UEBuildModule Module in Modules)
			{
				if (!UnrealBuildTool.IsUnderAnEngineDirectory(Module.ModuleDirectory))
				{
					GameModules.Add(Module);
				}
			}
			return GameModules;
		}

		/// <summary>
		/// Enumerates resources which the toolchain may need may produced additional build products from. Some platforms (eg. Mac, Linux) can link directly 
		/// against .so/.dylibs, but they are also copied to the output folder by the toolchain.
		/// </summary>
		/// <param name="Libraries">List to which libraries required by this module are added</param>
		/// <param name="BundleResources">List of bundle resources required by this module</param>
		public override void GatherAdditionalResources(List<string> Libraries, List<UEBuildBundleResource> BundleResources)
		{
			base.GatherAdditionalResources(Libraries, BundleResources);

			foreach(UEBuildModule Module in Modules)
			{
				Module.GatherAdditionalResources(Libraries, BundleResources);
			}
		}

		// Object interface.

		/// <summary>
		/// ToString implementation
		/// </summary>
		/// <returns>Returns the OutputFilePath for this binary</returns>
		public override string ToString()
		{
			return Config.OutputFilePath.FullName;
		}

		private LinkEnvironment SetupBinaryLinkEnvironment(ReadOnlyTargetRules Target, UEToolChain ToolChain, LinkEnvironment LinkEnvironment, CppCompileEnvironment CompileEnvironment, List<PrecompiledHeaderTemplate> SharedPCHs, ActionGraph ActionGraph)
		{
			LinkEnvironment BinaryLinkEnvironment = new LinkEnvironment(LinkEnvironment);
			HashSet<UEBuildModule> LinkEnvironmentVisitedModules = new HashSet<UEBuildModule>();
			List<UEBuildBinary> BinaryDependencies = new List<UEBuildBinary>();
			CompileEnvironment.bIsBuildingDLL = IsBuildingDll(Config.Type);
			CompileEnvironment.bIsBuildingLibrary = IsBuildingLibrary(Config.Type);

			CppCompileEnvironment BinaryCompileEnvironment = new CppCompileEnvironment(CompileEnvironment);

			// @todo: This should be in some Windows code somewhere...
			// Set the original file name macro; used in PCLaunch.rc to set the binary metadata fields.
			string OriginalFilename = (Config.OriginalOutputFilePaths != null) ?
				Config.OriginalOutputFilePaths[0].GetFileName() :
				Config.OutputFilePaths[0].GetFileName();
			BinaryCompileEnvironment.Definitions.Add("ORIGINAL_FILE_NAME=\"" + OriginalFilename + "\"");

			foreach (UEBuildModule Module in Modules)
			{
				List<FileItem> LinkInputFiles;
				if (Module.Binary == null || Module.Binary == this)
				{
					// Compile each module.
					Log.TraceVerbose("Compile module: " + Module.Name);
					LinkInputFiles = Module.Compile(Target, ToolChain, BinaryCompileEnvironment, SharedPCHs, ActionGraph);

					// NOTE: Because of 'Shared PCHs', in monolithic builds the same PCH file may appear as a link input
					// multiple times for a single binary.  We'll check for that here, and only add it once.  This avoids
					// a linker warning about redundant .obj files. 
					foreach (FileItem LinkInputFile in LinkInputFiles)
					{
						if (!BinaryLinkEnvironment.InputFiles.Contains(LinkInputFile))
						{
							BinaryLinkEnvironment.InputFiles.Add(LinkInputFile);
						}
					}
				}
				else
				{
					BinaryDependencies.Add(Module.Binary);
				}

				// Allow the module to modify the link environment for the binary.
				Module.SetupPrivateLinkEnvironment(this, BinaryLinkEnvironment, BinaryDependencies, LinkEnvironmentVisitedModules);
			}


			// Allow the binary dependencies to modify the link environment.
			foreach (UEBuildBinary BinaryDependency in BinaryDependencies)
			{
				BinaryDependency.SetupDependentLinkEnvironment(BinaryLinkEnvironment);
			}

			// Remove the default resource file on Windows (PCLaunch.rc) if the user has specified their own
			if (BinaryLinkEnvironment.InputFiles.Select(Item => Path.GetFileName(Item.AbsolutePath).ToLower()).Any(Name => Name.EndsWith(".res") && !Name.EndsWith(".inl.res") && Name != "pclaunch.rc.res"))
			{
				BinaryLinkEnvironment.InputFiles.RemoveAll(x => Path.GetFileName(x.AbsolutePath).ToLower() == "pclaunch.rc.res");
			}

			// Set the link output file.
			BinaryLinkEnvironment.OutputFilePaths = Config.OutputFilePaths.ToList();

			// Set whether the link is allowed to have exports.
			BinaryLinkEnvironment.bHasExports = Config.bAllowExports;

			// Set the output folder for intermediate files
			BinaryLinkEnvironment.IntermediateDirectory = Config.IntermediateDirectory;

			// Put the non-executable output files (PDB, import library, etc) in the same directory as the production
			BinaryLinkEnvironment.OutputDirectory = Config.OutputFilePaths[0].Directory;

			// Setup link output type
			BinaryLinkEnvironment.bIsBuildingDLL = IsBuildingDll(Config.Type);
			BinaryLinkEnvironment.bIsBuildingLibrary = IsBuildingLibrary(Config.Type);

			BinaryLinkEnvironment.ProjectFile = Target.ProjectFile;

			// If we don't have any resource file, use the default or compile a custom one for this module
			if(BinaryLinkEnvironment.Platform == CppPlatform.Win32 || BinaryLinkEnvironment.Platform == CppPlatform.Win64)
			{
				if (!BinaryLinkEnvironment.InputFiles.Any(x => x.Reference.HasExtension(".res")))
				{
					if(BinaryLinkEnvironment.DefaultResourceFiles.Count > 0)
					{
						// Use the default resource file if possible
						BinaryLinkEnvironment.InputFiles.AddRange(BinaryLinkEnvironment.DefaultResourceFiles);
					}
					else
					{
						// Otherwise compile the default resource file per-binary, so that it gets the correct ORIGINAL_FILE_NAME macro.
						CppCompileEnvironment BinaryResourceCompileEnvironment = new CppCompileEnvironment(BinaryCompileEnvironment);
						BinaryResourceCompileEnvironment.OutputDirectory = DirectoryReference.Combine(BinaryResourceCompileEnvironment.OutputDirectory, Modules.First().Name);

						FileItem DefaultResourceFile = FileItem.GetItemByFileReference(FileReference.Combine(UnrealBuildTool.EngineSourceDirectory, "Runtime", "Launch", "Resources", "Windows", "PCLaunch.rc"));
						CPPOutput DefaultResourceOutput = ToolChain.CompileRCFiles(BinaryResourceCompileEnvironment, new List<FileItem> { DefaultResourceFile }, ActionGraph);
						BinaryLinkEnvironment.InputFiles.AddRange(DefaultResourceOutput.ObjectFiles);
					}
				}
			}

			// Add all the common resource files
			BinaryLinkEnvironment.InputFiles.AddRange(BinaryLinkEnvironment.CommonResourceFiles);

			return BinaryLinkEnvironment;
		}

		private List<FileItem> SetupOutputFiles(UEToolChain ToolChain, CppCompileEnvironment BinaryCompileEnvironment, LinkEnvironment BinaryLinkEnvironment, ActionGraph ActionGraph)
		{
			//
			// Regular linking action.
			//
			List<FileItem> OutputFiles = new List<FileItem>();
			if (bCreateImportLibrarySeparately)
			{
				// Mark the link environment as cross-referenced.
				BinaryLinkEnvironment.bIsCrossReferenced = true;

				if (BinaryLinkEnvironment.Platform != CppPlatform.Mac && BinaryLinkEnvironment.Platform != CppPlatform.Linux)
				{
					// Create the import library.
					OutputFiles.AddRange(ToolChain.LinkAllFiles(BinaryLinkEnvironment, true, ActionGraph));
				}
			}

			BinaryLinkEnvironment.bIncludeDependentLibrariesInLibrary = bIncludeDependentLibrariesInLibrary;

			// Link the binary.
			FileItem[] Executables = ToolChain.LinkAllFiles(BinaryLinkEnvironment, false, ActionGraph);
			OutputFiles.AddRange(Executables);

			// Produce additional console app if requested
			if (Config.bBuildAdditionalConsoleApp)
			{
				// Produce additional binary but link it as a console app
				LinkEnvironment ConsoleAppLinkEvironment = new LinkEnvironment(BinaryLinkEnvironment);
				ConsoleAppLinkEvironment.bIsBuildingConsoleApplication = true;
				ConsoleAppLinkEvironment.WindowsEntryPointOverride = "WinMainCRTStartup";		// For WinMain() instead of "main()" for Launch module
				ConsoleAppLinkEvironment.OutputFilePaths = ConsoleAppLinkEvironment.OutputFilePaths.Select(Path => GetAdditionalConsoleAppPath(Path)).ToList();

				// Link the console app executable
				OutputFiles.AddRange(ToolChain.LinkAllFiles(ConsoleAppLinkEvironment, false, ActionGraph));
			}

			foreach (FileItem Executable in Executables)
			{
				OutputFiles.AddRange(ToolChain.PostBuild(Executable, BinaryLinkEnvironment, ActionGraph));
			}

			return OutputFiles;
		}
	}
}

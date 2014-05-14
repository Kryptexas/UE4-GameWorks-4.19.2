// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Xml;

namespace UnrealBuildTool
{
	/** Type of module. */
	public enum UEBuildModuleType
	{
		EngineModule,
		GameModule,
	}
	
	/** A unit of code compilation and linking. */
	public abstract class UEBuildModule
	{
		/** Converts an optional string list parameter to a well-defined list. */
		protected static List<string> ListFromOptionalEnumerableStringParameter(IEnumerable<string> InEnumerableStrings)
		{
			return InEnumerableStrings == null ? new List<string>() : new List<string>(InEnumerableStrings);
		}

		/** The target which owns this module. */
		public readonly UEBuildTarget Target;
	
		/** The name that uniquely identifies the module. */
		public readonly string Name;

		/** The type of module being built. Used to switch between debug/development and precompiled/source configurations. */
		public UEBuildModuleType Type;

		/** Path to the module directory */
		public readonly string ModuleDirectory;

		/** An optional output path for the module */
		public readonly string OutputDirectory;

		/** The binary the module will be linked into for the current target.  Only set after UEBuildBinary.BindModules is called. */
		public UEBuildBinary Binary = null;

		/** Whether this module is included in the current target.  Only set after UEBuildBinary.BindModules is called. */
		public bool bIncludedInTarget = false;

		protected readonly List<string> PublicDefinitions;
		protected readonly List<string> PublicIncludePaths;
		protected readonly List<string> PrivateIncludePaths;
		protected readonly List<string> PublicSystemIncludePaths;
		protected readonly List<string> PublicLibraryPaths;
		protected readonly List<string> PublicAdditionalLibraries;
		protected readonly List<string> PublicFrameworks;
		protected readonly List<UEBuildFramework> PublicAdditionalFrameworks;
		protected readonly List<string> PublicAdditionalShadowFiles;

		/** Names of modules with header files that this module's public interface needs access to. */
		protected List<string> PublicIncludePathModuleNames;

		/** Names of modules that this module's public interface depends on. */
		protected List<string> PublicDependencyModuleNames;

		/** Names of DLLs that this module should delay load */
		protected List<string> PublicDelayLoadDLLs;

		/** Names of modules with header files that this module's private implementation needs access to. */
		protected List<string> PrivateIncludePathModuleNames;
	
		/** Names of modules that this module's private implementation depends on. */
		protected List<string> PrivateDependencyModuleNames;

		/** If any of this module's dependent modules circularly reference this module, then they must be added to this list */
		public HashSet<string> CircularlyReferencedDependentModules
		{
			get;
			protected set;
		}

		/** Extra modules this module may require at run time */
		protected List<string> DynamicallyLoadedModuleNames;

        /** Extra modules this module may require at run time, that are on behalf of another platform (i.e. shader formats and the like) */
        protected List<string> PlatformSpecificDynamicallyLoadedModuleNames;

		public UEBuildModule(
			UEBuildTarget InTarget,
			string InName,
			UEBuildModuleType InType,
			string InModuleDirectory,
			string InOutputDirectory,
			IEnumerable<string> InPublicDefinitions = null,
			IEnumerable<string> InPublicIncludePaths = null,
			IEnumerable<string> InPublicSystemIncludePaths = null,
			IEnumerable<string> InPublicLibraryPaths = null,
			IEnumerable<string> InPublicAdditionalLibraries = null,
			IEnumerable<string> InPublicFrameworks = null,
			IEnumerable<UEBuildFramework> InPublicAdditionalFrameworks = null,
			IEnumerable<string> InPublicAdditionalShadowFiles = null,
			IEnumerable<string> InPublicIncludePathModuleNames = null,
			IEnumerable<string> InPublicDependencyModuleNames = null,
			IEnumerable<string> InPublicDelayLoadDLLs = null,
			IEnumerable<string> InPrivateIncludePaths = null,
			IEnumerable<string> InPrivateIncludePathModuleNames = null,
			IEnumerable<string> InPrivateDependencyModuleNames = null,
			IEnumerable<string> InCircularlyReferencedDependentModules = null,
			IEnumerable<string> InDynamicallyLoadedModuleNames = null,
            IEnumerable<string> InPlatformSpecificDynamicallyLoadedModuleNames = null
			)
		{
			Target = InTarget;
			Name = InName;
			Type = InType;
			ModuleDirectory = InModuleDirectory;
			OutputDirectory = InOutputDirectory;
			PublicDefinitions = ListFromOptionalEnumerableStringParameter(InPublicDefinitions);
			PublicIncludePaths = ListFromOptionalEnumerableStringParameter(InPublicIncludePaths);
			PublicSystemIncludePaths = ListFromOptionalEnumerableStringParameter(InPublicSystemIncludePaths);
			PublicLibraryPaths = ListFromOptionalEnumerableStringParameter(InPublicLibraryPaths);
			PublicAdditionalLibraries = ListFromOptionalEnumerableStringParameter(InPublicAdditionalLibraries);
			PublicFrameworks = ListFromOptionalEnumerableStringParameter(InPublicFrameworks);
			PublicAdditionalFrameworks = InPublicAdditionalFrameworks == null ? new List<UEBuildFramework>() : new List<UEBuildFramework>( InPublicAdditionalFrameworks );
			PublicAdditionalShadowFiles = ListFromOptionalEnumerableStringParameter(InPublicAdditionalShadowFiles);
			PublicIncludePathModuleNames = ListFromOptionalEnumerableStringParameter( InPublicIncludePathModuleNames );
			PublicDependencyModuleNames = ListFromOptionalEnumerableStringParameter(InPublicDependencyModuleNames);
			PublicDelayLoadDLLs = ListFromOptionalEnumerableStringParameter(InPublicDelayLoadDLLs);
			PrivateIncludePaths = ListFromOptionalEnumerableStringParameter(InPrivateIncludePaths);
			PrivateIncludePathModuleNames = ListFromOptionalEnumerableStringParameter( InPrivateIncludePathModuleNames );
			PrivateDependencyModuleNames = ListFromOptionalEnumerableStringParameter( InPrivateDependencyModuleNames );
			CircularlyReferencedDependentModules = new HashSet<string>( ListFromOptionalEnumerableStringParameter( InCircularlyReferencedDependentModules ) );
			DynamicallyLoadedModuleNames = ListFromOptionalEnumerableStringParameter( InDynamicallyLoadedModuleNames );
            PlatformSpecificDynamicallyLoadedModuleNames = ListFromOptionalEnumerableStringParameter(InPlatformSpecificDynamicallyLoadedModuleNames);

			Target.RegisterModule(this);
		}

		/** Adds a public definition */
		public virtual void AddPublicDefinition(string Definition)
		{
			PublicDefinitions.Add(Definition);
		}

		/** Adds a public include path. */
		public virtual void AddPublicIncludePath(string PathName, bool bCheckForDuplicates = true)
		{
			if (bCheckForDuplicates == true)
			{
				if (PublicIncludePaths.Contains(PathName))
				{
					return;
				}
			}
			PublicIncludePaths.Add(PathName);
		}

		/** Adds a public library path */
		public virtual void AddPublicLibraryPath(string PathName, bool bCheckForDuplicates = true)
		{
			if (bCheckForDuplicates == true)
			{
				if (PublicLibraryPaths.Contains(PathName))
				{
					return;
				}
			}
			PublicLibraryPaths.Add(PathName);
		}

		/** Adds a public additional library */
		public virtual void AddPublicAdditionalLibrary(string LibraryName, bool bCheckForDuplicates = true)
		{
			if (bCheckForDuplicates == true)
			{
				if (PublicAdditionalLibraries.Contains(LibraryName))
				{
					return;
				}
			}
			PublicAdditionalLibraries.Add(LibraryName);
		}

		/** Adds a public framework */
		public virtual void AddPublicFramework(string LibraryName, bool bCheckForDuplicates = true)
		{
			if (bCheckForDuplicates == true)
			{
				if (PublicFrameworks.Contains(LibraryName))
				{
					return;
				}
			}
			PublicFrameworks.Add(LibraryName);
		}

		/** Adds a public additional framework */
		public virtual void AddPublicAdditionalFramework(UEBuildFramework Framework, bool bCheckForDuplicates = true)
		{
			if (bCheckForDuplicates == true)
			{
				if ( PublicAdditionalFrameworks.Contains( Framework ) )
				{
					return;
				}
			}
			PublicAdditionalFrameworks.Add( Framework );
		}

		/** Adds a file that will be synced to a remote machine if a remote build is being executed */
		public virtual void AddPublicAdditionalShadowFile(string FileName)
		{
			PublicAdditionalShadowFiles.Add(FileName);
		}

		/** Adds a public include path module. */
		public virtual void AddPublicIncludePathModule(string ModuleName, bool bCheckForDuplicates = true)
		{
			if (bCheckForDuplicates == true)
			{
				if (PublicIncludePathModuleNames.Contains(ModuleName))
				{
					return;
				}
			}
			PublicIncludePathModuleNames.Add(ModuleName);
		}

		/** Adds a public dependency module. */
		public virtual void AddPublicDependencyModule(string ModuleName, bool bCheckForDuplicates = true)
		{
			if (bCheckForDuplicates == true)
			{
				if (PublicDependencyModuleNames.Contains(ModuleName))
				{
					return;
				}
			}
			PublicDependencyModuleNames.Add(ModuleName);
		}
		
		/** Adds a dynamically loaded module. */
		public virtual void AddDynamicallyLoadedModule(string ModuleName, bool bCheckForDuplicates = true)
		{
			if (bCheckForDuplicates == true)
			{
				if (DynamicallyLoadedModuleNames.Contains(ModuleName))
				{
					return;
				}
			}
			DynamicallyLoadedModuleNames.Add(ModuleName);
		}
        /** Adds a dynamically loaded module platform specifc. */
		public virtual void AddPlatformSpecificDynamicallyLoadedModule(string ModuleName, bool bCheckForDuplicates = true)
        {
            if (bCheckForDuplicates == true)
            {
                if (PlatformSpecificDynamicallyLoadedModuleNames.Contains(ModuleName))
                {
                    return;
                }
            }
            PlatformSpecificDynamicallyLoadedModuleNames.Add(ModuleName);
        }
		/** Adds a public delay load DLL */
		public virtual void AddPublicDelayLoadDLLs(string DelayLoadDLLName, bool bCheckForDuplicates = true)
		{
			if (bCheckForDuplicates == true)
			{
				if (PublicDelayLoadDLLs.Contains(DelayLoadDLLName))
				{
					return;
				}
			}
			PublicDelayLoadDLLs.Add(DelayLoadDLLName);
		}

		/** Adds a private include path. */
		public virtual void AddPrivateIncludePath(string PathName, bool bCheckForDuplicates = true)
		{
			if (bCheckForDuplicates == true)
			{
				if (PrivateIncludePaths.Contains(PathName))
				{
					return;
				}
			}
			PrivateIncludePaths.Add(PathName);
		}

		/** Adds a private include path module. */
		public virtual void AddPrivateIncludePathModule(string ModuleName, bool bCheckForDuplicates = true)
		{
			if (bCheckForDuplicates == true)
			{
				if (PrivateIncludePathModuleNames.Contains(ModuleName))
				{
					return;
				}
			}
			PrivateIncludePathModuleNames.Add(ModuleName);
		}

		/** Adds a dependency module. */
		public virtual void AddPrivateDependencyModule(string ModuleName, bool bCheckForDuplicates = true)
		{
			if (bCheckForDuplicates == true)
			{
				if (PrivateDependencyModuleNames.Contains(ModuleName))
				{
					return;
				}
			}
			PrivateDependencyModuleNames.Add(ModuleName);
		}

		/** Tells the module that a specific dependency is circularly reference */
		public virtual void AddCircularlyReferencedDependentModule(string ModuleName)
		{
			CircularlyReferencedDependentModules.Add( ModuleName );
		}

		/** Adds an extra module name */
		public virtual void AddExtraModuleName(string ModuleName)
		{
			DynamicallyLoadedModuleNames.Add( ModuleName );
		}

		/**
		 * Clears all public include paths.
		 */
		internal void ClearPublicIncludePaths()
		{
			PublicIncludePaths.Clear();
		}

		/** 
		 * Clears all public library paths.
		 * This is to allow a given platform the opportunity to remove set libraries.
		 */
		internal void ClearPublicLibraryPaths()
		{
			PublicLibraryPaths.Clear();
		}

		/** 
		 * Clears all public additional libraries from the module.
		 * This is to allow a given platform the opportunity to remove set libraries.
		 */
		internal void ClearPublicAdditionalLibraries()
		{
			PublicAdditionalLibraries.Clear();
		}

		/// <summary>
		/// If present, remove the given library from the PublicAdditionalLibraries array
		/// </summary>
		/// <param name="InLibrary">The library to remove from the list</param>
		internal void RemovePublicAdditionalLibrary(string InLibrary)
		{
			PublicAdditionalLibraries.Remove(InLibrary);
		}

		/**
		 * If found, remove the given entry from the definitions of the module.
		 */
		internal void RemovePublicDefinition(string InDefintion)
		{
			PublicDefinitions.Remove(InDefintion);
		}

		/** Return the (optional) output directory */
		public string GetOutputDirectory()
		{
			return OutputDirectory;
		}

		/** Fix up the output path */
		public virtual string FixupOutputPath(string InOutputPath)
		{
			string ModuleOutputPath = InOutputPath;
			if ((OutputDirectory.Length != 0) && (UnrealBuildTool.BuildingRocket() == false))
			{
				// Use from 'Binaries/' on as it is setup w/ the correct platform already
				int BinariesIndex = ModuleOutputPath.LastIndexOf("Binaries");
				if (BinariesIndex != -1)
				{
					ModuleOutputPath = OutputDirectory + ModuleOutputPath.Substring(BinariesIndex);
				}
			}

			return ModuleOutputPath;
		}

		/** Sets up the environment for compiling any module that includes the public interface of this module. */
		protected virtual void SetupPublicCompileEnvironment(
			UEBuildBinary SourceBinary,
			bool bIncludePathsOnly,
			ref List<string> IncludePaths,
			ref List<string> SystemIncludePaths,
			ref List<string> Definitions,
			ref Dictionary<UEBuildModule, bool> VisitedModules
			)
		{
			// There may be circular dependencies in compile dependencies, so we need to avoid reentrance.
			if(!VisitedModules.ContainsKey(this))
			{
				VisitedModules.Add(this,true);

				// Add this module's public include paths and definitions.
				IncludePaths.AddRange(PublicIncludePaths);
				SystemIncludePaths.AddRange(PublicSystemIncludePaths);
				Definitions.AddRange(PublicDefinitions);
				
				// If this module is being built into a DLL or EXE, set up an IMPORTS or EXPORTS definition for it.
				if (Binary != null)
				{
					if (ProjectFileGenerator.bGenerateProjectFiles || (Binary.Config.Type == UEBuildBinaryType.StaticLibrary))
					{
						// When generating IntelliSense files, never add dllimport/dllexport specifiers as it
						// simply confuses the compiler
						Definitions.Add( string.Format( "{0}_API=", Name.ToUpperInvariant() ) );
					}
					else if( Binary == SourceBinary )
					{
						if( Binary.Config.bAllowExports )
						{
							Log.TraceVerbose( "{0}: Exporting {1} from {2}", Path.GetFileNameWithoutExtension( SourceBinary.Config.OutputFilePath ), Name, Path.GetFileNameWithoutExtension( Binary.Config.OutputFilePath ) );
							Definitions.Add( string.Format( "{0}_API=DLLEXPORT", Name.ToUpperInvariant() ) );
						}
						else
						{
							Log.TraceVerbose( "{0}: Not importing/exporting {1} (binary: {2})", Path.GetFileNameWithoutExtension( SourceBinary.Config.OutputFilePath ), Name, Path.GetFileNameWithoutExtension( Binary.Config.OutputFilePath ) );
							Definitions.Add( string.Format( "{0}_API=", Name.ToUpperInvariant() ) );
						}
					}
					else
					{
						// @todo SharedPCH: Public headers included from modules that are not importing the module of that public header, seems invalid.  
						//		Those public headers have no business having APIs in them.  OnlineSubsystem has some public headers like this.  Without changing
						//		this, we need to suppress warnings at compile time.
						if( bIncludePathsOnly )
						{
							Log.TraceVerbose( "{0}: Include paths only for {1} (binary: {2})", Path.GetFileNameWithoutExtension( SourceBinary.Config.OutputFilePath ), Name, Path.GetFileNameWithoutExtension( Binary.Config.OutputFilePath ) );
							Definitions.Add( string.Format( "{0}_API=", Name.ToUpperInvariant() ) );
						}
						else if (Binary.Config.bAllowExports)
						{
							Log.TraceVerbose( "{0}: Importing {1} from {2}", Path.GetFileNameWithoutExtension( SourceBinary.Config.OutputFilePath ), Name, Path.GetFileNameWithoutExtension( Binary.Config.OutputFilePath ) );
							Definitions.Add( string.Format( "{0}_API=DLLIMPORT", Name.ToUpperInvariant() ) );
						}
						else
						{
							Log.TraceVerbose("{0}: Not importing/exporting {1} (binary: {2})", Path.GetFileNameWithoutExtension(SourceBinary.Config.OutputFilePath), Name, Path.GetFileNameWithoutExtension(Binary.Config.OutputFilePath));
							Definitions.Add(string.Format("{0}_API=", Name.ToUpperInvariant()));
						}
					}
				}

				if( !bIncludePathsOnly )
				{
					// Recurse on this module's public dependencies.
					foreach(var DependencyName in PublicDependencyModuleNames)
					{
						var DependencyModule = Target.GetModuleByName(DependencyName);
						DependencyModule.SetupPublicCompileEnvironment(SourceBinary,bIncludePathsOnly,ref IncludePaths,ref SystemIncludePaths,ref Definitions,ref VisitedModules);
					}
				}

				// Now add an include paths from modules with header files that we need access to, but won't necessarily be importing
				foreach( var IncludePathModuleName in PublicIncludePathModuleNames )
				{
					bool bInnerIncludePathsOnly = true;
					var IncludePathModule = Target.GetModuleByName(IncludePathModuleName);
					IncludePathModule.SetupPublicCompileEnvironment( SourceBinary, bInnerIncludePathsOnly, ref IncludePaths, ref SystemIncludePaths, ref Definitions, ref VisitedModules );
				}

				// Add the module's directory to the include path, so we can root #includes to it
				IncludePaths.Add(ModuleDirectory);
			}
		}
		
		/** Sets up the environment for compiling this module. */
		protected virtual void SetupPrivateCompileEnvironment(
			ref List<string> IncludePaths,
			ref List<string> SystemIncludePaths,
			ref List<string> Definitions
			)
		{
			var VisitedModules = new Dictionary<UEBuildModule, bool>();

			// Add this module's private include paths and definitions.
			IncludePaths.AddRange( PrivateIncludePaths );

			// Allow the module's public dependencies to modify the compile environment.
			bool bIncludePathsOnly = false;
			SetupPublicCompileEnvironment(Binary,bIncludePathsOnly,ref IncludePaths,ref SystemIncludePaths,ref Definitions,ref VisitedModules);

			// Also allow the module's private dependencies to modify the compile environment.
			foreach(var DependencyName in PrivateDependencyModuleNames)
			{
				var DependencyModule = Target.GetModuleByName(DependencyName);
				DependencyModule.SetupPublicCompileEnvironment(Binary,bIncludePathsOnly,ref IncludePaths,ref SystemIncludePaths,ref Definitions,ref VisitedModules);
			}

			// Add include paths from modules with header files that our private files need access to, but won't necessarily be importing
			foreach( var IncludePathModuleName in PrivateIncludePathModuleNames )
			{
				bool bInnerIncludePathsOnly = true;
				var IncludePathModule = Target.GetModuleByName(IncludePathModuleName);
				IncludePathModule.SetupPublicCompileEnvironment( Binary, bInnerIncludePathsOnly, ref IncludePaths, ref SystemIncludePaths, ref Definitions, ref VisitedModules );
			}
		}

		/** Sets up the environment for linking any module that includes the public interface of this module. */ 
		protected virtual void SetupPublicLinkEnvironment(
			UEBuildBinary SourceBinary,
			ref List<string> LibraryPaths,
			ref List<string> AdditionalLibraries,
			ref List<string> Frameworks,
			ref List<UEBuildFramework> AdditionalFrameworks,
			ref List<string> AdditionalShadowFiles,
			ref List<string> DelayLoadDLLs,
			ref List<UEBuildBinary> BinaryDependencies,
			ref Dictionary<UEBuildModule, bool> VisitedModules
			)
		{
			// There may be circular dependencies in compile dependencies, so we need to avoid reentrance.
			if(!VisitedModules.ContainsKey(this))
			{
				VisitedModules.Add(this,true);

				// Only process modules that are included in the current target.
				if(bIncludedInTarget)
				{
					// Add this module's binary to the binary dependencies.
					if(Binary != null
					&& Binary != SourceBinary
					&& !BinaryDependencies.Contains(Binary))
					{
						BinaryDependencies.Add(Binary);
					}

					// If this module belongs to a static library that we are not currently building, recursively add the link environment settings for all of its dependencies too.
					// Keep doing this until we reach a module that is not part of a static library (or external module, since they have no associated binary).
					// Static libraries do not contain the symbols for their dependencies, so we need to recursively gather them to be linked into other binary types.
					bool bIsBuildingAStaticLibrary = (SourceBinary != null && SourceBinary.Config.Type == UEBuildBinaryType.StaticLibrary);
					bool bIsModuleBinaryAStaticLibrary = (Binary != null && Binary.Config.Type == UEBuildBinaryType.StaticLibrary);
					if (!bIsBuildingAStaticLibrary && bIsModuleBinaryAStaticLibrary)
					{
						// Gather all dependencies and recursively call SetupPublicLinkEnvironmnet
						List<string> AllDependencyModuleNames = new List<string>(PrivateDependencyModuleNames);
						AllDependencyModuleNames.AddRange(PublicDependencyModuleNames);

						foreach (var DependencyName in AllDependencyModuleNames)
						{
							var DependencyModule = Target.GetModuleByName(DependencyName);
							bool bIsExternalModule = (DependencyModule as UEBuildExternalModule != null);
							bool bIsInStaticLibrary = (DependencyModule.Binary != null && DependencyModule.Binary.Config.Type == UEBuildBinaryType.StaticLibrary);
							if (bIsExternalModule || bIsInStaticLibrary)
							{
								DependencyModule.SetupPublicLinkEnvironment(SourceBinary, ref LibraryPaths, ref AdditionalLibraries, ref Frameworks,
									ref AdditionalFrameworks, ref AdditionalShadowFiles, ref DelayLoadDLLs, ref BinaryDependencies, ref VisitedModules);
							}
						}
					}

					// Add this module's public include library paths and additional libraries.
					LibraryPaths.AddRange(PublicLibraryPaths);
					AdditionalLibraries.AddRange(PublicAdditionalLibraries);
					Frameworks.AddRange(PublicFrameworks);
					// Remember the module so we can refer to it when needed
					foreach ( var Framework in PublicAdditionalFrameworks )
					{
						Framework.OwningModule = this;
					}
					AdditionalFrameworks.AddRange(PublicAdditionalFrameworks);
					AdditionalShadowFiles.AddRange(PublicAdditionalShadowFiles);
					DelayLoadDLLs.AddRange(PublicDelayLoadDLLs);
				}
			}
		}

		/** Sets up the environment for linking this module. */
		public virtual void SetupPrivateLinkEnvironment(
			ref LinkEnvironment LinkEnvironment,
			ref List<UEBuildBinary> BinaryDependencies,
			ref Dictionary<UEBuildModule, bool> VisitedModules
			)
		{
			// Allow the module's public dependencies to add library paths and additional libraries to the link environment.
			SetupPublicLinkEnvironment(Binary,ref LinkEnvironment.Config.LibraryPaths,ref LinkEnvironment.Config.AdditionalLibraries,ref LinkEnvironment.Config.Frameworks,
				ref LinkEnvironment.Config.AdditionalFrameworks,ref LinkEnvironment.Config.AdditionalShadowFiles,ref LinkEnvironment.Config.DelayLoadDLLs,ref BinaryDependencies,ref VisitedModules);

			// Also allow the module's public and private dependencies to modify the link environment.
			List<string> AllDependencyModuleNames = new List<string>(PrivateDependencyModuleNames);
			AllDependencyModuleNames.AddRange(PublicDependencyModuleNames);

			foreach (var DependencyName in AllDependencyModuleNames)
			{
				var DependencyModule = Target.GetModuleByName(DependencyName);
				DependencyModule.SetupPublicLinkEnvironment(Binary,ref LinkEnvironment.Config.LibraryPaths,ref LinkEnvironment.Config.AdditionalLibraries,ref LinkEnvironment.Config.Frameworks,
					ref LinkEnvironment.Config.AdditionalFrameworks,ref LinkEnvironment.Config.AdditionalShadowFiles,ref LinkEnvironment.Config.DelayLoadDLLs,ref BinaryDependencies,ref VisitedModules);
			}
		}

		/** Compiles the module, and returns a list of files output by the compiler. */
		public abstract List<FileItem> Compile( CPPEnvironment GlobalCompileEnvironment, CPPEnvironment CompileEnvironment, bool bCompileMonolithic );

		/** Writes the build environment for this module */
		public abstract void WriteBuildEnvironment(CPPEnvironment CompileEnvironment, XmlWriter Writer);
		
		// Object interface.
		public override string ToString()
		{
			return Name;
		}


		/**
		 * Gets all of the modules referenced by this module
		 * 
		 * @param	ReferencedModules	Hash of all referenced modules
		 * @param	OrderedModules		Ordered list of the referenced modules
		 * @param	bIncludeDynamicallyLoaded	True if dynamically loaded modules (and all of their dependent modules) should be included.
		 * @param	bForceCircular	True if circular dependecies should be processed
		 */
		public virtual void GetAllDependencyModules(ref Dictionary<string, UEBuildModule> ReferencedModules, ref List<UEBuildModule> OrderedModules, bool bIncludeDynamicallyLoaded, bool bForceCircular)
		{
		}

		/**
		 * Gathers and binds binaries for this module and all of it's dependent modules
		 *
		 * @param	Target				The target we are currently building
		 * @param	Binaries			Dictionary of all binaries we've gathered so far
		 * @param	ExecutableBinary	The binary for the target's main executable (used only when linking monolithically)
		 * @param	bBuildOnlyModules	True to build only specific modules, false for all
		 * @param	ModulesToBuild		The specific modules to build
		 */
		public virtual void RecursivelyProcessUnboundModules(UEBuildTarget Target, ref Dictionary<string, UEBuildBinary> Binaries, UEBuildBinary ExecutableBinary)
		{
		}

		/**
		 * Recursively adds modules that are referenced only for include paths (not actual dependencies).
		 * 
		 * @param	Target	The target we are currently building
		 * @param	bPublicIncludesOnly	True to only add modules for public includes, false to add all include path modules.  Regardless of what you pass here, recursive adds will only add public include path modules.
		 */
		public virtual void RecursivelyAddIncludePathModules( UEBuildTarget Target, bool bPublicIncludesOnly )
		{
		}
	};

	/** A module that is never compiled by us, and is only used to group include paths and libraries into a dependency unit. */
	class UEBuildExternalModule : UEBuildModule
	{
		public UEBuildExternalModule(
			UEBuildTarget InTarget,
			UEBuildModuleType InType,
			string InName,
			string InModuleDirectory,
			string InOutputDirectory,
			IEnumerable<string> InPublicDefinitions = null,
			IEnumerable<string> InPublicIncludePaths = null,
			IEnumerable<string> InPublicSystemIncludePaths = null,
			IEnumerable<string> InPublicLibraryPaths = null,
			IEnumerable<string> InPublicAdditionalLibraries = null,
			IEnumerable<string> InPublicFrameworks = null,
			IEnumerable<UEBuildFramework> InPublicAdditionalFrameworks = null,
			IEnumerable<string> InPublicAdditionalShadowFiles = null,
			IEnumerable<string> InPublicDependencyModuleNames = null,
			IEnumerable<string> InPublicDelayLoadDLLs = null		// Delay loaded DLLs that should be setup when including this module
			)
		: base(
			InTarget:						InTarget,
			InType:							InType,
			InName:							InName,
			InModuleDirectory:				InModuleDirectory,
			InOutputDirectory:				InOutputDirectory,
			InPublicDefinitions:			InPublicDefinitions,
			InPublicIncludePaths:			InPublicIncludePaths,
			InPublicSystemIncludePaths:		InPublicSystemIncludePaths,
			InPublicLibraryPaths:			InPublicLibraryPaths,
			InPublicAdditionalLibraries:	InPublicAdditionalLibraries,
			InPublicFrameworks:             InPublicFrameworks,
			InPublicAdditionalFrameworks:	InPublicAdditionalFrameworks,
			InPublicAdditionalShadowFiles:  InPublicAdditionalShadowFiles,
			InPublicDependencyModuleNames:	InPublicDependencyModuleNames,
			InPublicDelayLoadDLLs:			InPublicDelayLoadDLLs
			)
		{
			bIncludedInTarget = true;
		}

		// UEBuildModule interface.
		public override List<FileItem> Compile(CPPEnvironment GlobalCompileEnvironment, CPPEnvironment CompileEnvironment, bool bCompileMonolithic)
		{
			return new List<FileItem>();
		}

		public override void WriteBuildEnvironment(CPPEnvironment CompileEnvironment, XmlWriter Writer)
		{
			Writer.WriteStartElement("module");
			Writer.WriteAttributeString("name", Name);
			Writer.WriteAttributeString("path", ModuleDirectory);
			Writer.WriteAttributeString("type", "external");
			Writer.WriteEndElement();
		}
	};

	/** A module that is compiled from C++ code. */
	public class UEBuildModuleCPP : UEBuildModule
	{
		public class AutoGenerateCppInfoClass
		{
			/** The filename of the *.generated.cpp file which was generated for the module */
			public readonly string Filename;

			public AutoGenerateCppInfoClass(string InFilename)
			{
				Debug.Assert(InFilename != null);

				Filename = InFilename;
			}
		}

		/** Information about the .inl file . */
		public AutoGenerateCppInfoClass AutoGenerateCppInfo = null;

		public class SourceFilesClass
		{
			public readonly List<FileItem> MissingFiles = new List<FileItem>();
			public readonly List<FileItem> CPPFiles     = new List<FileItem>();
			public readonly List<FileItem> CFiles       = new List<FileItem>();
			public readonly List<FileItem> CCFiles      = new List<FileItem>();
			public readonly List<FileItem> MMFiles      = new List<FileItem>();
			public readonly List<FileItem> RCFiles      = new List<FileItem>();
			public readonly List<FileItem> OtherFiles   = new List<FileItem>();

			public int Count
			{
				get
				{
					return MissingFiles.Count +
					       CPPFiles    .Count +
					       CFiles      .Count +
					       CCFiles     .Count +
					       MMFiles     .Count +
					       RCFiles     .Count +
					       OtherFiles  .Count;
				}
			}
		}

		/** A list of the absolute paths of source files in the module. */
		public readonly SourceFilesClass SourceFiles = new SourceFilesClass();

		/** The preprocessor definitions used to compile this module's private implementation. */
		List<string> Definitions;

		/// When set, allows this module to report compiler definitions and include paths for Intellisense
		IntelliSenseGatherer IntelliSenseGatherer;

		/** Whether this module contains performance critical code. */
		ModuleRules.CodeOptimization OptimizeCode = ModuleRules.CodeOptimization.Default;

		/** Whether this module should use a "shared PCH" header if possible.  Disable this for modules that have lots of global includes
			in a private PCH header (e.g. game modules) */
		public bool bAllowSharedPCH = true;

		/** Header file name for a shared PCH provided by this module.  Must be a valid relative path to a public C++ header file.
			This should only be set for header files that are included by a significant number of other C++ modules. */
		public string SharedPCHHeaderFile = String.Empty;

		/** Use run time type information */
		public bool bUseRTTI = false;

		/** Enable buffer security checks. This should usually be enabled as it prevents severe security risks. */
		public bool bEnableBufferSecurityChecks = true;

		/** If true and unity builds are enabled, this module will build without unity. */
		public bool bFasterWithoutUnity = false;

		/** Overrides BuildConfiguration.MinFilesUsingPrecompiledHeader if non-zero. */
		public int MinFilesUsingPrecompiledHeaderOverride = 0;

		/** Enable exception handling */
		public bool bEnableExceptions = false;

		/** Enable inlining */
		public bool bEnableInlining = true;

		/** Path to this module's redist static library */
		public string RedistStaticLibraryPath = null;

		/** Whether we're building the redist static library (as well as using it). */
		public bool bBuildingRedistStaticLibrary = false;

		public class ProcessedDependenciesClass
		{
			/** The file, if any, which is used as the unique PCH for this module */
			public FileItem UniquePCHHeaderFile = null;
		}

		/** The processed dependencies for the class */
		public ProcessedDependenciesClass ProcessedDependencies = null;

		public UEBuildModuleCPP(
			UEBuildTarget InTarget,
			string InName,
			UEBuildModuleType InType,
			string InModuleDirectory,
			string InOutputDirectory,
			IntelliSenseGatherer InIntelliSenseGatherer,
			IEnumerable<FileItem> InSourceFiles,
			IEnumerable<string> InPublicIncludePaths,
			IEnumerable<string> InPublicSystemIncludePaths,
			IEnumerable<string> InDefinitions,
			IEnumerable<string> InPublicIncludePathModuleNames,
			IEnumerable<string> InPublicDependencyModuleNames,
			IEnumerable<string> InPublicDelayLoadDLLs,
			IEnumerable<string> InPublicAdditionalLibraries,
			IEnumerable<string> InPublicFrameworks,
			IEnumerable<UEBuildFramework> InPublicAdditionalFrameworks,
			IEnumerable<string> InPublicAdditionalShadowFiles,
			IEnumerable<string> InPrivateIncludePaths,
			IEnumerable<string> InPrivateIncludePathModuleNames,
			IEnumerable<string> InPrivateDependencyModuleNames,
			IEnumerable<string> InCircularlyReferencedDependentModules,
			IEnumerable<string> InDynamicallyLoadedModuleNames,
            IEnumerable<string> InPlatformSpecificDynamicallyLoadedModuleNames,
            ModuleRules.CodeOptimization InOptimizeCode,
			bool InAllowSharedPCH,
			string InSharedPCHHeaderFile,
			bool InUseRTTI,
			bool InEnableBufferSecurityChecks,
			bool InFasterWithoutUnity,
			int InMinFilesUsingPrecompiledHeaderOverride,
			bool InEnableExceptions,
			bool InEnableInlining
			)
			: base(	InTarget,
					InName, 
					InType,
					InModuleDirectory,
					InOutputDirectory, 
					InDefinitions, 
					InPublicIncludePaths,
					InPublicSystemIncludePaths, 
					null, 
					InPublicAdditionalLibraries,
					InPublicFrameworks,
					InPublicAdditionalFrameworks,
					InPublicAdditionalShadowFiles,
					InPublicIncludePathModuleNames,
					InPublicDependencyModuleNames,
					InPublicDelayLoadDLLs,
					InPrivateIncludePaths, 
					InPrivateIncludePathModuleNames,
					InPrivateDependencyModuleNames, 
					InCircularlyReferencedDependentModules,
					InDynamicallyLoadedModuleNames,
                    InPlatformSpecificDynamicallyLoadedModuleNames
				)
		{
			IntelliSenseGatherer = InIntelliSenseGatherer;

			foreach (var SourceFile in InSourceFiles)
			{
				string Extension = Path.GetExtension(SourceFile.AbsolutePath).ToUpperInvariant();
				if (!SourceFile.bExists)
				{
					SourceFiles.MissingFiles.Add(SourceFile);
				}
				else if (Extension == ".CPP")
				{
					SourceFiles.CPPFiles.Add(SourceFile);
				}
				else if (Extension == ".C")
				{
					SourceFiles.CFiles.Add(SourceFile);
				}
				else if (Extension == ".CC")
				{
					SourceFiles.CCFiles.Add(SourceFile);
				}
				else if (Extension == ".MM" || Extension == ".M")
				{
					SourceFiles.MMFiles.Add(SourceFile);
				}
				else if (Extension == ".RC")
				{
					SourceFiles.RCFiles.Add(SourceFile);
				}
				else
				{
					SourceFiles.OtherFiles.Add(SourceFile);
				}
			}

			Definitions = ListFromOptionalEnumerableStringParameter(InDefinitions);
			foreach (var Def in Definitions)
			{
				Log.TraceVerbose("Compile Env {0}: {1}", Name, Def);
			}
			OptimizeCode                           = InOptimizeCode;
			bAllowSharedPCH                        = InAllowSharedPCH;
			SharedPCHHeaderFile                    = InSharedPCHHeaderFile;
			bUseRTTI                               = InUseRTTI;
			bEnableBufferSecurityChecks 		   = InEnableBufferSecurityChecks;
			bFasterWithoutUnity                    = InFasterWithoutUnity;
			MinFilesUsingPrecompiledHeaderOverride = InMinFilesUsingPrecompiledHeaderOverride;
			bEnableExceptions                      = InEnableExceptions;
			bEnableInlining                        = InEnableInlining;
		}

		// UEBuildModule interface.
		public override List<FileItem> Compile(CPPEnvironment GlobalCompileEnvironment, CPPEnvironment CompileEnvironment, bool bCompileMonolithic)
		{
			UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatformForCPPTargetPlatform(CompileEnvironment.Config.TargetPlatform);

			var LinkInputFiles = new List<FileItem>();
			if( ProjectFileGenerator.bGenerateProjectFiles && IntelliSenseGatherer == null)
			{
				// Nothing to do for IntelliSense, bail out early
				return LinkInputFiles;
			}

			if(RedistStaticLibraryPath != null && !bBuildingRedistStaticLibrary)
			{
				// The redist static library will be added in SetupPrivateLinkEnvironment
				return LinkInputFiles;
			}

			var ModuleCompileEnvironment = CreateModuleCompileEnvironment(CompileEnvironment);

			if( IntelliSenseGatherer != null )
			{
				// Update project file's set of preprocessor definitions and include paths
				IntelliSenseGatherer.AddIntelliSensePreprocessorDefinitions( ModuleCompileEnvironment.Config.Definitions );
				IntelliSenseGatherer.AddInteliiSenseIncludePaths( ModuleCompileEnvironment.Config.SystemIncludePaths, bAddingSystemIncludes: true );
				IntelliSenseGatherer.AddInteliiSenseIncludePaths( ModuleCompileEnvironment.Config.IncludePaths, bAddingSystemIncludes: false );

				// Bail out.  We don't need to actually compile anything while generating project files.
				return LinkInputFiles;
			}

			// Throw an error if the module's source file list referenced any non-existent files.
			if (SourceFiles.MissingFiles.Count > 0)
			{
				throw new BuildException(
					"UBT ERROR: Module \"{0}\" references non-existent files:\n{1} (perhaps a file was added to the project but not checked in)",
					Name,
					string.Join("\n", SourceFiles.MissingFiles.Select(M => M.AbsolutePath))
				);
			}

			// For an executable or a static library do not use the default RC file - 
			// If the executable wants it, it will be in their source list anyway.
			// The issue here is that when making a monolithic game, the processing
			// of the other game modules will stomp the game-specific rc file.
			if (Binary.Config.Type == UEBuildBinaryType.DynamicLinkLibrary)
			{
				// Add default PCLaunch.rc file if this module has no own resource file specified
				if (SourceFiles.RCFiles.Count <= 0)
				{
					string DefRC = Path.Combine(Directory.GetCurrentDirectory(), "Runtime/Launch/Resources/Windows/PCLaunch.rc");
					FileItem Item = FileItem.GetItemByFullPath(DefRC);
					SourceFiles.RCFiles.Add(Item);
				}
			}

			// Check to see if this is an Engine module (including program or plugin modules).  That is, the module is located under the "Engine" folder
			var IsGameModule = !Utils.IsFileUnderDirectory( this.ModuleDirectory, Path.Combine( ProjectFileGenerator.EngineRelativePath ) );

			// Should we force a precompiled header to be generated for this module?  Usually, we only bother with a
			// precompiled header if there are at least several source files in the module (after combining them for unity
			// builds.)  But for game modules, it can be convenient to always have a precompiled header to single-file
			// changes to code is really quick to compile.
			int MinFilesUsingPrecompiledHeader = BuildConfiguration.MinFilesUsingPrecompiledHeader;
			if( MinFilesUsingPrecompiledHeaderOverride != 0 )
			{
				MinFilesUsingPrecompiledHeader = MinFilesUsingPrecompiledHeaderOverride;
			}
			else if( IsGameModule && BuildConfiguration.bForcePrecompiledHeaderForGameModules )
			{
				// This is a game module with only a small number of source files, so go ahead and force a precompiled header
				// to be generated to make incremental changes to source files as fast as possible for small projects.
				MinFilesUsingPrecompiledHeader = 1;
			}


			// Should we use unity build mode for this module?
			bool bModuleUsesUnityBuild = BuildConfiguration.bUseUnityBuild;
			if( bFasterWithoutUnity )
			{
				bModuleUsesUnityBuild = false;
			}
			else if( !BuildConfiguration.bForceUnityBuild && IsGameModule && SourceFiles.CPPFiles.Count < BuildConfiguration.MinGameModuleSourceFilesForUnityBuild )
			{
				// Game modules with only a small number of source files are usually better off having faster iteration times
				// on single source file changes, so we forcibly disable unity build for those modules
				bModuleUsesUnityBuild = false;
			}

			// The environment with which to compile the CPP files
			var CPPCompileEnvironment = ModuleCompileEnvironment;

			// Precompiled header support.
			bool bWasModuleCodeCompiled = false;
			if (BuildPlatform.ShouldUsePCHFiles(CompileEnvironment.Config.TargetPlatform, CompileEnvironment.Config.TargetConfiguration))
			{
				var PCHTimerStart = DateTime.UtcNow;

				// The code below will figure out whether this module will either use a "unique PCH" (private PCH that will only be included by
				// this module's code files), or a "shared PCH" (potentially included by many code files in many modules.)  Only one or the other
				// will be used.
				FileItem SharedPCHHeaderFile = null;

				// In the case of a shared PCH, we also need to keep track of which module that PCH's header file is a member of
				string SharedPCHModuleName = String.Empty;

				bool bDisableSharedPCHFiles = (Binary.Config.bCompileMonolithic && CompileEnvironment.Config.bIsBuildingLibrary);
				if( BuildConfiguration.bUseSharedPCHs && bDisableSharedPCHFiles )
				{
					Log.TraceVerbose("Module '{0}' was not allowed to use SharedPCHs, because we're compiling to a library in monolithic mode", this.Name );
				}

				bool bUseSharedPCHFiles  = BuildConfiguration.bUseSharedPCHs && (bDisableSharedPCHFiles == false);
				bool bIsASharedPCHModule = bUseSharedPCHFiles && GlobalCompileEnvironment.SharedPCHHeaderFiles.Any( PCH => PCH.Module == this );

				// Map from pch header string to the source files that use that PCH
				var UsageMapPCH = new Dictionary<string, List<FileItem>>( StringComparer.InvariantCultureIgnoreCase );

				// Determine what potential precompiled header is used by each source file.
				double SharedPCHTotalTime = 0.0;
				foreach( var CPPFile in SourceFiles.CPPFiles )
				{
					if (bUseSharedPCHFiles)
					{
						var SharedPCHStartTime = DateTime.UtcNow;

						// When compiling in modular mode, we can't use a shared PCH file when compiling a module
						// with exports, because the shared PCH can only have imports in it to work correctly.
						// @todo SharedPCH: If we ever have SharedPCH headers that themselves belong to modules which never use DLL Exports, we can avoid
						// generating TWO PCH files by checking for that here.  For now, we always assume that SharedPCH headers have exports when
						// compiling in modular mode.
						if( bAllowSharedPCH && ( !bIsASharedPCHModule || bCompileMonolithic ) )
						{
							// Figure out which shared PCH tier we're in
							int LargestSharedPCHHeaderFileIndex = -1;
							{
								var AllIncludedFiles = ModuleCompileEnvironment.GetIncludeDependencies( CPPFile );
								foreach( var IncludedFile in AllIncludedFiles )
								{
									// These PCHs are ordered from least complex to most complex.  We'll start at the last one and search backwards.
									for( var SharedPCHHeaderFileIndex = GlobalCompileEnvironment.SharedPCHHeaderFiles.Count - 1; SharedPCHHeaderFileIndex > LargestSharedPCHHeaderFileIndex; --SharedPCHHeaderFileIndex )
									{
										var CurSharedPCHHeaderFile = GlobalCompileEnvironment.SharedPCHHeaderFiles[ SharedPCHHeaderFileIndex ];
										if( IncludedFile == CurSharedPCHHeaderFile.PCHHeaderFile )
										{
											LargestSharedPCHHeaderFileIndex = SharedPCHHeaderFileIndex;
											break;
										}
									}

									if( LargestSharedPCHHeaderFileIndex == GlobalCompileEnvironment.SharedPCHHeaderFiles.Count - 1)
									{
										// We've determined that the module is using our most complex PCH header, so we can early-out
										break;
									}
								}
							}

							if( LargestSharedPCHHeaderFileIndex > -1 )
							{
								var LargestIncludedSharedPCHHeaderFile = GlobalCompileEnvironment.SharedPCHHeaderFiles[LargestSharedPCHHeaderFileIndex];
								if( SharedPCHHeaderFile == null )
								{
									SharedPCHModuleName = LargestIncludedSharedPCHHeaderFile.Module.Name;
									SharedPCHHeaderFile = LargestIncludedSharedPCHHeaderFile.PCHHeaderFile;
								}
								else if( SharedPCHHeaderFile != LargestIncludedSharedPCHHeaderFile.PCHHeaderFile )
								{
									// @todo UBT perf: It is fairly costly to perform this test, as we could easily early-out after we have SharedPCHHeaderFile and not bother testing the rest of the files in the module.  But this can be useful to find abusive modules that include a shared PCH header in the bowels of a non-PCH private source file.
									Console.WriteLine( "WARNING: File '{0}' doesn't use same 'shared' precompiled header as other files in this module: '{1}' vs '{2}'.  This can greatly impact compile times.  Make sure that your module's private PCH header includes the 'largest' shared PCH header that your module uses", 
										CPPFile.AbsolutePath, SharedPCHHeaderFile, LargestIncludedSharedPCHHeaderFile.PCHHeaderFile );
								}
							}
							else
							{
								Log.TraceVerbose("File {0} doesn't use a SharedPCH!", CPPFile.AbsolutePath);
							}

							SharedPCHTotalTime += ( DateTime.UtcNow - SharedPCHStartTime ).TotalSeconds;
						}
						else
						{
							Log.TraceVerbose("File '{0}' cannot create or use SharedPCHs, because its module '{1}' needs its own private PCH", CPPFile.AbsolutePath, this.Name);
						}
					}

					if( CPPFile.PrecompiledHeaderIncludeFilename == null )
					{
						throw new BuildException( "No PCH usage for file \"{0}\" . Missing #include header?", CPPFile.AbsolutePath );
					}

					// Create a new entry if not in the pch usage map
					UsageMapPCH.GetOrAddNew( CPPFile.PrecompiledHeaderIncludeFilename ).Add( CPPFile );
				}

				if( BuildConfiguration.bPrintDebugInfo )
				{
					Log.TraceVerbose( "{0} PCH files for module {1}:", UsageMapPCH.Count, Name );
					int MostFilesIncluded = 0;
					foreach( var CurPCH in UsageMapPCH )
					{
						if( CurPCH.Value.Count > MostFilesIncluded )
						{
							MostFilesIncluded = CurPCH.Value.Count;
						}

						Log.TraceVerbose("   {0}  ({1} files including it: {2}, ...)", CurPCH.Key, CurPCH.Value.Count, CurPCH.Value[0].AbsolutePath);
					}
				}

				if( UsageMapPCH.Count > 1 )
				{
					// Keep track of the PCH file that is most used within this module
					string MostFilesAreIncludingPCH = string.Empty;
					int MostFilesIncluded = 0;
					foreach( var CurPCH in UsageMapPCH.Where( PCH => PCH.Value.Count > MostFilesIncluded ) )
					{
						MostFilesAreIncludingPCH = CurPCH.Key;
						MostFilesIncluded        = CurPCH.Value.Count;
					}

					// Find all of the files that are not including our "best" PCH header
					var FilesNotIncludingBestPCH = new StringBuilder();
					foreach( var CurPCH in UsageMapPCH.Where( PCH => PCH.Key != MostFilesAreIncludingPCH ) )
					{
						foreach( var SourceFile in CurPCH.Value )
						{
							FilesNotIncludingBestPCH.AppendFormat( "{0} (including {1})\n", SourceFile.AbsolutePath, CurPCH.Key );
						}
					}

					// Bail out and let the user know which source files may need to be fixed up
					throw new BuildException(
						"All source files in module \"{0}\" must include the same precompiled header first.  Currently \"{1}\" is included by most of the source files.  The following source files are not including \"{1}\" as their first include:\n\n{2}",
						Name,
						MostFilesAreIncludingPCH,
						FilesNotIncludingBestPCH );
				}

				// The precompiled header environment for all source files in this module that use a precompiled header, if we even need one
				PrecompileHeaderEnvironment ModulePCHEnvironment = null;

				// If there was one header that was included first by enough C++ files, use it as the precompiled header.
				// Only use precompiled headers for projects with enough files to make the PCH creation worthwhile.
				if( SharedPCHHeaderFile != null || SourceFiles.CPPFiles.Count >= MinFilesUsingPrecompiledHeader )
				{
					FileItem PCHToUse;

					if( SharedPCHHeaderFile != null )
					{
						ModulePCHEnvironment = ApplySharedPCH(GlobalCompileEnvironment, CompileEnvironment, ModuleCompileEnvironment, SourceFiles.CPPFiles, ref SharedPCHHeaderFile);
						if (ModulePCHEnvironment != null)
						{
							// @todo SharedPCH: Ideally we would exhaustively check for a compatible compile environment (definitions, imports/exports, etc)
							//    Currently, it's possible for the shared PCH to be compiled differently depending on which module UBT happened to have
							//    include it first during the build phase.  This could create problems with determinstic builds, or turn up compile
							//    errors unexpectedly due to compile environment differences.
							Log.TraceVerbose("Module " + Name + " uses existing SharedPCH '" + ModulePCHEnvironment.PrecompiledHeaderIncludeFilename + "' (from module " + ModulePCHEnvironment.ModuleName + ")");
						}

						PCHToUse = SharedPCHHeaderFile;
					}
					else
					{
						PCHToUse = ProcessedDependencies.UniquePCHHeaderFile;
					}

					if (PCHToUse != null)
					{
						// Update all CPPFiles to point to the PCH
						foreach (var CPPFile in SourceFiles.CPPFiles)
						{
							CPPFile.PCHHeaderNameInCode              = PCHToUse.AbsolutePath;
							CPPFile.PrecompiledHeaderIncludeFilename = PCHToUse.AbsolutePath;
						}
					}

					// A shared PCH was not already set up for us, so set one up.
					if( ModulePCHEnvironment == null )
					{
						var PCHHeaderFile = ProcessedDependencies.UniquePCHHeaderFile;
						var PCHModuleName = this.Name;
						if( SharedPCHHeaderFile != null )
						{
							PCHHeaderFile = SharedPCHHeaderFile;
							PCHModuleName = SharedPCHModuleName;
						}
						var PCHHeaderNameInCode = SourceFiles.CPPFiles[ 0 ].PCHHeaderNameInCode;

						ModulePCHEnvironment = new PrecompileHeaderEnvironment( PCHModuleName, PCHHeaderNameInCode, PCHHeaderFile, ModuleCompileEnvironment.Config.CLRMode, ModuleCompileEnvironment.Config.OptimizeCode );

						Log.TraceVerbose( "PCH file \"{0}\" generated for module \"{1}\" .", PCHHeaderFile.AbsolutePath, Name );

						if( SharedPCHHeaderFile != null )
						{
							// Add to list of shared PCH environments
							GlobalCompileEnvironment.SharedPCHEnvironments.Add( ModulePCHEnvironment );
							Log.TraceVerbose( "Module " + Name + " uses new SharedPCH '" + ModulePCHEnvironment.PrecompiledHeaderIncludeFilename + "'" );
						}
						else
						{
							Log.TraceVerbose( "Module " + Name + " uses a unique PCH '" + ModulePCHEnvironment.PrecompiledHeaderIncludeFilename + "'" );
						}
					}
				}

				// Compile the C++ source or the unity C++ files that use a PCH environment.
				if( ModulePCHEnvironment != null )
				{
					// Setup a new compile environment for this module's source files.  It's pretty much the exact same as the
					// module's compile environment, except that it will include a PCH file.
					var ModulePCHCompileEnvironment = new CPPEnvironment( ModuleCompileEnvironment );
					ModulePCHCompileEnvironment.Config.PrecompiledHeaderAction          = PrecompiledHeaderAction.Include;
					ModulePCHCompileEnvironment.Config.PrecompiledHeaderIncludeFilename = ModulePCHEnvironment.PrecompiledHeaderIncludeFilename.AbsolutePath;
					ModulePCHCompileEnvironment.Config.PCHHeaderNameInCode              = ModulePCHEnvironment.PCHHeaderNameInCode;

					if( SharedPCHHeaderFile != null )
					{
						// Shared PCH headers need to be force included, because we're basically forcing the module to use
						// the precompiled header that we want, instead of the "first include" in each respective .cpp file
						ModulePCHCompileEnvironment.Config.bForceIncludePrecompiledHeader = true;
					}

					var CPPFilesToBuild = SourceFiles.CPPFiles;
					if (bModuleUsesUnityBuild)
					{
						// unity files generated for only the set of files which share the same PCH environment
						CPPFilesToBuild = Unity.GenerateUnityCPPs( CPPFilesToBuild, ModulePCHCompileEnvironment, Name );
					}

					// Check if there are enough unity files to warrant pch generation (and we haven't already generated the shared one)
					if( ModulePCHEnvironment.PrecompiledHeaderFile == null && ( SharedPCHHeaderFile != null || CPPFilesToBuild.Count >= MinFilesUsingPrecompiledHeader ) )
					{
						bool bAllowDLLExports = true;
						var PCHOutputDirectory = ModuleCompileEnvironment.Config.OutputDirectory;
						var PCHModuleName = this.Name;

						if( SharedPCHHeaderFile != null )
						{
							// Disallow DLLExports when generating shared PCHs.  These headers aren't able to export anything, because they're potentially shared between many modules.
							bAllowDLLExports = false;

							// Save shared PCHs to a specific folder
							PCHOutputDirectory = Path.Combine( CompileEnvironment.Config.OutputDirectory, "SharedPCHs" );

							// Use a fake module name for "shared" PCHs.  It may be used by many modules, so we don't want to use this module's name.
							PCHModuleName = "Shared";
						}

						var PCHOutput = PrecompileHeaderEnvironment.GeneratePCHCreationAction( 
							CPPFilesToBuild[0].PCHHeaderNameInCode,
							ModulePCHEnvironment.PrecompiledHeaderIncludeFilename,
							ModuleCompileEnvironment, 
							PCHOutputDirectory,
							PCHModuleName, 
							bAllowDLLExports );
						ModulePCHEnvironment.PrecompiledHeaderFile = PCHOutput.PrecompiledHeaderFile;
							
						ModulePCHEnvironment.OutputObjectFiles.Clear();
						ModulePCHEnvironment.OutputObjectFiles.AddRange( PCHOutput.ObjectFiles );
					}

					if( ModulePCHEnvironment.PrecompiledHeaderFile != null )
					{
						// Link in the object files produced by creating the precompiled header.
						LinkInputFiles.AddRange( ModulePCHEnvironment.OutputObjectFiles );

						// if pch action was generated for the environment then use pch
						ModulePCHCompileEnvironment.PrecompiledHeaderFile = ModulePCHEnvironment.PrecompiledHeaderFile;

						// Use this compile environment from now on
						CPPCompileEnvironment = ModulePCHCompileEnvironment;
					}

					LinkInputFiles.AddRange( CPPCompileEnvironment.CompileFiles( CPPFilesToBuild, Name ).ObjectFiles );
					bWasModuleCodeCompiled = true;
				}

				if( BuildConfiguration.bPrintPerformanceInfo )
				{
					var TotalPCHTime = DateTime.UtcNow - PCHTimerStart;
					Trace.TraceInformation( "PCH time for " + Name + " is " + TotalPCHTime.TotalSeconds + "s (shared PCHs: " + SharedPCHTotalTime + "s)" );
				}
			}

			if( !bWasModuleCodeCompiled && SourceFiles.CPPFiles.Count > 0 )
			{
				var CPPFilesToCompile = SourceFiles.CPPFiles;
				if (bModuleUsesUnityBuild)
				{
					CPPFilesToCompile = Unity.GenerateUnityCPPs( CPPFilesToCompile, ModuleCompileEnvironment, Name );
				}
				LinkInputFiles.AddRange( CPPCompileEnvironment.CompileFiles( CPPFilesToCompile, Name ).ObjectFiles );
			}

			if (AutoGenerateCppInfo != null && !ModuleCompileEnvironment.bHackHeaderGenerator)
			{
				var GeneratedCppFileItem = FileItem.GetItemByPath( AutoGenerateCppInfo.Filename );

				LinkInputFiles.AddRange( CPPCompileEnvironment.CompileFiles( new List<FileItem>{ GeneratedCppFileItem }, Name ).ObjectFiles );
			}

			// Compile C files directly.
			LinkInputFiles.AddRange(ModuleCompileEnvironment.CompileFiles(SourceFiles.CFiles, Name).ObjectFiles);

			// Compile CC files directly.
			LinkInputFiles.AddRange(ModuleCompileEnvironment.CompileFiles(SourceFiles.CCFiles, Name).ObjectFiles);

			// Compile MM files directly.
			LinkInputFiles.AddRange(ModuleCompileEnvironment.CompileFiles(SourceFiles.MMFiles, Name).ObjectFiles);

			// If we're building Rocket, generate a static library for this module
			if(RedistStaticLibraryPath != null)
			{
				// Create a link environment for it
				LinkEnvironment RedistLinkEnvironment = new LinkEnvironment();
				RedistLinkEnvironment.InputFiles.AddRange(LinkInputFiles);
				RedistLinkEnvironment.Config.TargetArchitecture    = CompileEnvironment.Config.TargetArchitecture;
				RedistLinkEnvironment.Config.TargetConfiguration   = CompileEnvironment.Config.TargetConfiguration;
				RedistLinkEnvironment.Config.TargetPlatform        = CompileEnvironment.Config.TargetPlatform;
				RedistLinkEnvironment.Config.bIsBuildingDLL        = false;
				RedistLinkEnvironment.Config.bIsBuildingLibrary    = true;
				RedistLinkEnvironment.Config.IntermediateDirectory = Binary.Config.IntermediateDirectory;
				RedistLinkEnvironment.Config.OutputFilePath        = RedistStaticLibraryPath;

				// Replace the items built so far with the library
				RedistLinkEnvironment.LinkExecutable(false);
				LinkInputFiles.Clear();
			}

			// Compile RC files.
			LinkInputFiles.AddRange(ModuleCompileEnvironment.CompileRCFiles(SourceFiles.RCFiles).ObjectFiles);

			return LinkInputFiles;
		}

		private PrecompileHeaderEnvironment ApplySharedPCH(CPPEnvironment GlobalCompileEnvironment, CPPEnvironment CompileEnvironment, CPPEnvironment ModuleCompileEnvironment, List<FileItem> CPPFiles, ref FileItem SharedPCHHeaderFile)
		{
			// Check to see if we have a PCH header already setup that we can use
			var SharedPCHHeaderFileCopy = SharedPCHHeaderFile;
			var SharedPCHEnvironment = GlobalCompileEnvironment.SharedPCHEnvironments.Find(Env => Env.PrecompiledHeaderIncludeFilename == SharedPCHHeaderFileCopy);
			if (SharedPCHEnvironment == null)
				return null;

			// Don't mix CLR modes
			if (SharedPCHEnvironment.CLRMode != ModuleCompileEnvironment.Config.CLRMode)
			{
				Log.TraceVerbose("Module {0} cannot use existing SharedPCH '{1}' (from module '{2}') because CLR modes don't match", Name, SharedPCHEnvironment.PrecompiledHeaderIncludeFilename.AbsolutePath, SharedPCHEnvironment.ModuleName);
				SharedPCHHeaderFile = null;
				return null;
			}

			// Don't mix non-optimized code with optimized code (PCHs won't be compatible)
			var SharedPCHCodeOptimization = SharedPCHEnvironment.OptimizeCode;
			var ModuleCodeOptimization    = ModuleCompileEnvironment.Config.OptimizeCode;

			if (CompileEnvironment.Config.TargetConfiguration != CPPTargetConfiguration.Debug)
			{
				if (SharedPCHCodeOptimization == ModuleRules.CodeOptimization.InNonDebugBuilds)
				{
					SharedPCHCodeOptimization = ModuleRules.CodeOptimization.Always;
				}

				if (ModuleCodeOptimization == ModuleRules.CodeOptimization.InNonDebugBuilds)
				{
					ModuleCodeOptimization = ModuleRules.CodeOptimization.Always;
				}
			}

			if (SharedPCHCodeOptimization != ModuleCodeOptimization)
			{
				Log.TraceVerbose("Module {0} cannot use existing SharedPCH '{1}' (from module '{2}') because optimization levels don't match", Name, SharedPCHEnvironment.PrecompiledHeaderIncludeFilename.AbsolutePath, SharedPCHEnvironment.ModuleName);
				SharedPCHHeaderFile = null;
				return null;
			}

			return SharedPCHEnvironment;
		}

		public void ProcessAllCppDependencies(CPPEnvironment ModuleCompileEnvironment)
		{
			if (ProcessedDependencies != null)
				return;

			FileItem UniquePCH = null;
			foreach( var CPPFile in SourceFiles.CPPFiles )
			{
				// Find headers used by the source file.
				var PCH = ProcessDependencies(CPPFile, ModuleCompileEnvironment);
				UniquePCH = UniquePCH ?? PCH;
			}

			ProcessedDependencies = new ProcessedDependenciesClass{ UniquePCHHeaderFile = UniquePCH };
		}

		private FileItem ProcessDependencies(FileItem CPPFile, CPPEnvironment ModuleCompileEnvironment)
		{
			List<DependencyInclude> DirectIncludeFilenames = CPPEnvironment.GetDirectIncludeDependencies(CPPFile, ModuleCompileEnvironment.Config.TargetPlatform);
			if (BuildConfiguration.bPrintDebugInfo)
			{
				Log.TraceVerbose("Found direct includes for {0}: {1}", Path.GetFileName(CPPFile.AbsolutePath), string.Join(", ", DirectIncludeFilenames.Select(F => F.IncludeName)));
			}

			if (DirectIncludeFilenames.Count == 0)
				return null;

			var FirstInclude = DirectIncludeFilenames[0];

			// The pch header should always be the first include in the source file.
			// NOTE: This is not an absolute path.  This is just the literal include string from the source file!
			CPPFile.PCHHeaderNameInCode = FirstInclude.IncludeName;

			// Resolve the PCH header to an absolute path.
			// Check NullOrEmpty here because if the file could not be resolved we need to throw an exception
			if (!string.IsNullOrEmpty(FirstInclude.IncludeResolvedName) &&
				// ignore any preexisting resolve cache if we are not configured to use it.
				BuildConfiguration.bUseIncludeDependencyResolveCache &&
				// if we are testing the resolve cache, we force UBT to resolve every time to look for conflicts
				!BuildConfiguration.bTestIncludeDependencyResolveCache)
			{
				CPPFile.PrecompiledHeaderIncludeFilename = FirstInclude.IncludeResolvedName;
				return FileItem.GetItemByFullPath(CPPFile.PrecompiledHeaderIncludeFilename);
			}

			string SourceFilesDirectory = Path.GetDirectoryName(CPPFile.AbsolutePath);

			// search the include paths to resolve the file.
			FileItem PrecompiledHeaderIncludeFile = ModuleCompileEnvironment.FindIncludedFile(CPPFile.PCHHeaderNameInCode, !BuildConfiguration.bCheckExternalHeadersForModification, SourceFilesDirectory);
			if (PrecompiledHeaderIncludeFile == null)
				throw new BuildException("The first include statement in source file '{0}' is trying to include the file '{1}' as the precompiled header for module '{2}', but that file could not be located in any of the module's include search paths.", CPPFile.AbsolutePath, CPPFile.PCHHeaderNameInCode, this.Name);

			CPPEnvironment.IncludeDependencyCache.CacheResolvedIncludeFullPath(CPPFile, 0, PrecompiledHeaderIncludeFile.AbsolutePath);
			CPPFile.PrecompiledHeaderIncludeFilename = PrecompiledHeaderIncludeFile.AbsolutePath;

			return PrecompiledHeaderIncludeFile;
		}

		/// <summary>
		/// Creates a compile environment from a base environment based on the module settings.
		/// </summary>
		/// <param name="BaseCompileEnvironment">An existing environment to base the module compile environment on.</param>
		/// <returns>The new module compile environment.</returns>
		public CPPEnvironment CreateModuleCompileEnvironment(CPPEnvironment BaseCompileEnvironment)
		{
			var Result = new CPPEnvironment(BaseCompileEnvironment);

			// Override compile environment
			Result.Config.bFasterWithoutUnity                    = bFasterWithoutUnity;
			Result.Config.OptimizeCode                           = OptimizeCode;
			Result.Config.bUseRTTI                               = bUseRTTI;
			Result.Config.bEnableBufferSecurityChecks            = bEnableBufferSecurityChecks;
			Result.Config.bFasterWithoutUnity                    = bFasterWithoutUnity;
			Result.Config.MinFilesUsingPrecompiledHeaderOverride = MinFilesUsingPrecompiledHeaderOverride;
			Result.Config.bEnableExceptions                      = bEnableExceptions;
			Result.Config.bEnableInlining                        = bEnableInlining;
			Result.Config.OutputDirectory                        = Path.Combine(Binary.Config.IntermediateDirectory, Name);

			// Switch the optimization flag if we're building a game module
			if (Target.Configuration == UnrealTargetConfiguration.DebugGame && Type == UEBuildModuleType.GameModule)
			{
				Result.Config.TargetConfiguration = CPPTargetConfiguration.Debug;
			}

			// Add the module's private definitions.
			Result.Config.Definitions.AddRange(Definitions);

			// Setup the compile environment for the module.
			SetupPrivateCompileEnvironment(ref Result.Config.IncludePaths, ref Result.Config.SystemIncludePaths, ref Result.Config.Definitions);

			return Result;
		}

		public override void SetupPrivateLinkEnvironment(
			ref LinkEnvironment LinkEnvironment,
			ref List<UEBuildBinary> BinaryDependencies,
			ref Dictionary<UEBuildModule, bool> VisitedModules
			)
		{
			base.SetupPrivateLinkEnvironment(ref LinkEnvironment,ref BinaryDependencies,ref VisitedModules);

			if(RedistStaticLibraryPath != null)
			{
				LinkEnvironment.Config.AdditionalLibraries.Add(RedistStaticLibraryPath);
			}
		}

		public override void WriteBuildEnvironment(CPPEnvironment CompileEnvironment, XmlWriter Writer)
		{
			// Get the compile environment
			List<string> PrivateIncludePaths = new List<string>();
			List<string> PrivateSystemIncludePaths = new List<string>();
			List<string> PrivateDefinitions = new List<string>();
			SetupPrivateCompileEnvironment(ref PrivateIncludePaths, ref PrivateSystemIncludePaths, ref PrivateDefinitions);

			// Add all of the include paths
			List<string> IncludePaths = new List<string>();
			IncludePaths.AddRange(CompileEnvironment.Config.SystemIncludePaths);
			IncludePaths.AddRange(PrivateSystemIncludePaths);
			IncludePaths.AddRange(CompileEnvironment.Config.IncludePaths);
			IncludePaths.AddRange(PrivateIncludePaths);

			// Build the full list of definitions
			List<string> Definitions = new List<string>();
			Definitions.AddRange(CompileEnvironment.Config.Definitions);
			Definitions.AddRange(PrivateDefinitions);

			// Build a list of all the dependencies
			List<string> Dependencies = new List<string>();
			Dependencies.AddRange(PublicDependencyModuleNames);
			Dependencies.AddRange(PrivateDependencyModuleNames);

			// Write the output XML
			Writer.WriteStartElement("module");
			Writer.WriteAttributeString("name", Name);
			Writer.WriteAttributeString("path", ModuleDirectory);
			Writer.WriteAttributeString("type", "cpp");
			Writer.WriteStartElement("dependencies");
			foreach (string Dependency in Dependencies)
			{
				Writer.WriteStartElement("dependency");
				Writer.WriteAttributeString("module", Dependency);
				Writer.WriteEndElement();
			}
			Writer.WriteEndElement();
			Writer.WriteStartElement("definitions");
			foreach (string Definition in Definitions)
			{
				Writer.WriteStartElement("definition");
				Writer.WriteAttributeString("name", Definition);
				Writer.WriteEndElement();
			}
			Writer.WriteEndElement();
			Writer.WriteStartElement("includes");
			foreach (string IncludePath in IncludePaths)
			{
				Writer.WriteStartElement("include");
				Writer.WriteAttributeString("path", IncludePath);
				Writer.WriteEndElement();
			}
			Writer.WriteEndElement();
			Writer.WriteEndElement();
		}

		public HashSet<FileItem> _AllClassesHeaders     = new HashSet<FileItem>();
		public HashSet<FileItem> _PublicUObjectHeaders  = new HashSet<FileItem>();
		public HashSet<FileItem> _PrivateUObjectHeaders = new HashSet<FileItem>();

		private UHTModuleInfo? CachedModuleUHTInfo = null;

		/// <summary>
		/// If any of this module's source files contain UObject definitions, this will return those header files back to the caller
		/// </summary>
		/// <param name="PublicUObjectClassesHeaders">All UObjects headers in the module's Classes folder (legacy)</param>
		/// <param name="PublicUObjectHeaders">Dependent UObject headers in Public source folders</param>
		/// <param name="PrivateUObjectHeaders">Dependent UObject headers not in Public source folders</param>
		/// <returns>
		public UHTModuleInfo GetUHTModuleInfo(CPPEnvironment CompileEnvironment)
		{
			if (CachedModuleUHTInfo.HasValue)
				return CachedModuleUHTInfo.Value;

			var ClassesFolder = Path.Combine(this.ModuleDirectory, "Classes");
			var PublicFolder  = Path.Combine(this.ModuleDirectory, "Public");

			var ClassesFiles = Directory.GetFiles( this.ModuleDirectory, "*.h", SearchOption.AllDirectories );
			foreach (var ClassHeader in ClassesFiles)
			{
				var UObjectHeaderFileItem = FileItem.GetExistingItemByPath( ClassHeader );
				var FileContents = Utils.ReadAllText(UObjectHeaderFileItem.AbsolutePath);
				if (!Regex.IsMatch(FileContents, "^\\s*U(CLASS|STRUCT|ENUM|INTERFACE|DELEGATE)\\b", RegexOptions.Multiline))
					continue;

				UObjectHeaderFileItem.HasUObjects = true;
				if (UObjectHeaderFileItem.AbsolutePath.StartsWith(ClassesFolder))
				{
					_AllClassesHeaders.Add(UObjectHeaderFileItem);
				}
				else if (UObjectHeaderFileItem.AbsolutePath.StartsWith(PublicFolder))
				{
					_PublicUObjectHeaders.Add(UObjectHeaderFileItem);
				}
				else
				{
					_PrivateUObjectHeaders.Add(UObjectHeaderFileItem);
				}
			}

			CachedModuleUHTInfo = new UHTModuleInfo {
				ModuleName                  = this.Name,
				ModuleDirectory             = this.ModuleDirectory,
				PublicUObjectClassesHeaders = _AllClassesHeaders    .ToList(),
				PublicUObjectHeaders        = _PublicUObjectHeaders .ToList(),
				PrivateUObjectHeaders       = _PrivateUObjectHeaders.ToList()
			};

			return CachedModuleUHTInfo.Value;
		}

		public override void GetAllDependencyModules( ref Dictionary<string, UEBuildModule> ReferencedModules, ref List<UEBuildModule> OrderedModules, bool bIncludeDynamicallyLoaded, bool bForceCircular )
		{
			var AllModuleNames = new List<string>();
			AllModuleNames.AddRange(PrivateDependencyModuleNames);
			AllModuleNames.AddRange(PublicDependencyModuleNames);
			if( bIncludeDynamicallyLoaded )
			{
				AllModuleNames.AddRange(DynamicallyLoadedModuleNames);
                AllModuleNames.AddRange(PlatformSpecificDynamicallyLoadedModuleNames);
            }

			foreach (var DependencyName in AllModuleNames)
			{
				if (!ReferencedModules.ContainsKey(DependencyName))
				{
					// Don't follow circular back-references!
					bool bIsCircular = CircularlyReferencedDependentModules.Contains( DependencyName );
					if (bForceCircular || !bIsCircular)
					{
						var Module = Target.GetModuleByName( DependencyName );
						ReferencedModules[ DependencyName ] = Module;

						// Recurse into dependent modules first
						Module.GetAllDependencyModules(ref ReferencedModules, ref OrderedModules, bIncludeDynamicallyLoaded, bForceCircular);

						OrderedModules.Add( Module );
					}
				}
			}
		}

		public override void RecursivelyAddIncludePathModules( UEBuildTarget Target, bool bPublicIncludesOnly )
		{
			var AllIncludePathModuleNames = new List<string>();
			AllIncludePathModuleNames.AddRange( PublicIncludePathModuleNames );
			if( !bPublicIncludesOnly )
			{
				AllIncludePathModuleNames.AddRange( PrivateIncludePathModuleNames );
			}
			foreach( var IncludePathModuleName in AllIncludePathModuleNames )
			{
				var IncludePathModule = Target.FindOrCreateModuleByName( IncludePathModuleName );

				// No need to do anything here.  We just want to make sure that we've instantiated our representation of the
				// module so that we can find it later when processing include path module names.  Unlike actual dependency
				// modules, these include path modules may never be "bound" (have Binary or bIncludedInTarget member variables set)

				// We'll also need to make sure we know about all dependent public include path modules, too!
				IncludePathModule.RecursivelyAddIncludePathModules( Target, bPublicIncludesOnly:true );
			}
		}

		public override void RecursivelyProcessUnboundModules(UEBuildTarget Target, ref Dictionary<string, UEBuildBinary> Binaries, UEBuildBinary ExecutableBinary)
		{
			try
			{
				// Make sure this module is bound to a binary
				if( !bIncludedInTarget )
				{
					throw new BuildException( "Module '{0}' should already have been bound to a binary!", Name );
				}

				var AllModuleNames = new List<string>();
				AllModuleNames.AddRange( PrivateDependencyModuleNames );
				AllModuleNames.AddRange( PublicDependencyModuleNames );
				AllModuleNames.AddRange( DynamicallyLoadedModuleNames );
				AllModuleNames.AddRange( PlatformSpecificDynamicallyLoadedModuleNames );

				foreach( var DependencyName in AllModuleNames )
				{
					var DependencyModule = Target.FindOrCreateModuleByName(DependencyName);

					// Skip modules that are included with the target (externals)
					if( !DependencyModule.bIncludedInTarget )
					{
						if( !Binaries.ContainsKey( DependencyModule.Name ) )
						{
							UEBuildBinary BinaryToBindTo;
							if (Target.ShouldCompileMonolithic())
							{
								// When linking monolithically, any unbound modules will be linked into the main executable
								BinaryToBindTo = ExecutableBinary;
							}
							else
							{
								// Is this a Rocket module?
								bool bIsRocketModule = RulesCompiler.IsRocketProjectModule(DependencyName);

								// Is this a plugin module?
								var PluginInfo = Plugins.GetPluginInfoForModule( DependencyName );

								string OutputFilePath = Target.MakeBinaryPath(DependencyModule.Name, Target.GetAppName() + "-" + DependencyModule.Name, UEBuildBinaryType.DynamicLinkLibrary, Target.Rules.Type, bIsRocketModule, PluginInfo, "");

								// If it's an engine module, output intermediates to the engine intermediates directory. 
								string IntermediateDirectory = Binary.Config.IntermediateDirectory;
								if (IntermediateDirectory != Target.EngineIntermediateDirectory && Path.GetFullPath(DependencyModule.ModuleDirectory).StartsWith(Path.GetFullPath(BuildConfiguration.RelativeEnginePath)))
								{
									IntermediateDirectory = Target.EngineIntermediateDirectory;
								}

								// When using modular linkage, unbound modules will be linked into their own DLL files
								UEBuildBinaryConfiguration Config = new UEBuildBinaryConfiguration( InType: UEBuildBinaryType.DynamicLinkLibrary,
																									InOutputFilePath: OutputFilePath,
																									InIntermediateDirectory: IntermediateDirectory,
																									bInAllowExports: true,
																									InModuleNames: new List<string> { DependencyModule.Name },
																									InTargetName: Target.GetAppName(),
																									bInIsCrossTarget: PlatformSpecificDynamicallyLoadedModuleNames.Contains(DependencyName) && !DynamicallyLoadedModuleNames.Contains(DependencyName),
																									InTargetConfiguration: Target.Configuration,
																									bInCompileMonolithic: Target.ShouldCompileMonolithic() );

								// Fix up the binary path if this is module specifies an alternate output directory
								Config.OutputFilePath = DependencyModule.FixupOutputPath(Config.OutputFilePath);

								BinaryToBindTo = new UEBuildBinaryCPP( Target, Config );
							}

							Binaries[ DependencyModule.Name ] = BinaryToBindTo;

							// Bind this module
							DependencyModule.Binary = BinaryToBindTo;
							DependencyModule.bIncludedInTarget = true;

							// Also add binaries for this module's dependencies
							DependencyModule.RecursivelyProcessUnboundModules( Target, ref Binaries, ExecutableBinary );
						}
					}

					if (Target.ShouldCompileMonolithic() == false)
					{
						// Check to see if there is a circular relationship between the module and it's referencer
						if( DependencyModule.Binary != null )
						{
							if( CircularlyReferencedDependentModules.Contains( DependencyName ) )
							{
								DependencyModule.Binary.SetCreateImportLibrarySeparately( true );
							}
						}
					}
				}

				// Also make sure module entries are created for any module that is pulled in as an "include path" module.
				// These modules are never linked in unless they were referenced as an actual dependency of a different module,
				// but we still need to keep track of them so that we can find their include paths when setting up our
				// module's include paths.
				RecursivelyAddIncludePathModules( Target, bPublicIncludesOnly:false );
			}
			catch (System.Exception ex)
			{
				throw new ModuleProcessingException(this, ex);
			}
		}


		/// <summary>
		/// Determines where generated code files will be stored for this module
		/// </summary>
		/// <param name="ModuleDirectory">Module's base directory</param>
		/// <param name="ModuleName">Name of module</param>
		/// <returns></returns>
		public static string GetGeneratedCodeDirectoryForModule(UEBuildTarget Target, string ModuleDirectory, string ModuleName)
		{
			string BaseDirectory = null;
			if ((Target.ShouldCompileMonolithic() || Target.Rules.Type == TargetRules.TargetType.Program) && 
				(!UnrealBuildTool.BuildingRocket()) &&
				(!UnrealBuildTool.RunningRocket() || Utils.IsFileUnderDirectory(ModuleDirectory, UnrealBuildTool.GetUProjectPath())))
			{
				// Monolithic configurations and programs have their intermediate headers stored under their
				// respective project folders with the exception of rocket which always stores engine modules in the engine folder.
				string RootDirectory = UnrealBuildTool.GetUProjectPath();
				if (String.IsNullOrEmpty(RootDirectory))
				{
					// Intermediates under Engine intermediate folder (program name will be appended later)
					RootDirectory = Path.GetFullPath(BuildConfiguration.RelativeEnginePath);
				}
				BaseDirectory = Path.Combine(RootDirectory, BuildConfiguration.PlatformIntermediateFolder, Target.GetTargetName(), "Inc");
			}
			else if (Plugins.IsPluginModule(ModuleName))
			{
				// Plugin module
				BaseDirectory = Plugins.GetPluginInfoForModule(ModuleName).IntermediateIncPath;
			}
			else
			{
				var AllProjectFolders = UEBuildTarget.DiscoverAllGameFolders();
				BaseDirectory = AllProjectFolders.Find(ProjectFolder => Utils.IsFileUnderDirectory( ModuleDirectory, ProjectFolder ));
				if (BaseDirectory == null)
				{
					// Must be an engine module or program module
					BaseDirectory = ProjectFileGenerator.EngineRelativePath;
				}

				BaseDirectory = Path.GetFullPath(Path.Combine(BaseDirectory, BuildConfiguration.PlatformIntermediateFolder, "Inc"));
			}

			// Construct the intermediate path.
			var GeneratedCodeDirectory = Path.Combine(BaseDirectory, ModuleName);
			return GeneratedCodeDirectory + Path.DirectorySeparatorChar;
		}

	};

	/** A module that is compiled from C++ CLR code. */
	class UEBuildModuleCPPCLR : UEBuildModuleCPP
	{
		/** The assemblies referenced by the module's private implementation. */
		List<string> PrivateAssemblyReferences;

		public UEBuildModuleCPPCLR(
			UEBuildTarget InTarget,
			string InName,
			UEBuildModuleType InType,
			string InModuleDirectory,
			string InOutputDirectory,
			IntelliSenseGatherer InIntelliSenseGatherer,
			IEnumerable<FileItem> InSourceFiles,
			IEnumerable<string> InPublicIncludePaths,
			IEnumerable<string> InPublicSystemIncludePaths,
			IEnumerable<string> InDefinitions,
			IEnumerable<string> InPrivateAssemblyReferences,
			IEnumerable<string> InPublicIncludePathModuleNames,
			IEnumerable<string> InPublicDependencyModuleNames,
			IEnumerable<string> InPublicDelayLoadDLLs,
			IEnumerable<string> InPublicAdditionalLibraries,
			IEnumerable<string> InPublicFrameworks,
			IEnumerable<UEBuildFramework> InPublicAdditionalFrameworks,
			IEnumerable<string> InPublicAdditionalShadowFiles,
			IEnumerable<string> InPrivateIncludePaths,
			IEnumerable<string> InPrivateIncludePathModuleNames,
			IEnumerable<string> InPrivateDependencyModuleNames,
			IEnumerable<string> InCircularlyReferencedDependentModules,
			IEnumerable<string> InDynamicallyLoadedModuleNames,
            IEnumerable<string> InPlatformSpecificDynamicallyLoadedModuleNames,
            ModuleRules.CodeOptimization InOptimizeCode,
			bool InAllowSharedPCH,
			string InSharedPCHHeaderFile,
			bool InUseRTTI,
			bool InEnableBufferSecurityChecks,
			bool InFasterWithoutUnity,
			int InMinFilesUsingPrecompiledHeaderOverride,
			bool InEnableExceptions,
			bool InEnableInlining
			)
		: base(InTarget,InName,InType,InModuleDirectory,InOutputDirectory,InIntelliSenseGatherer,
			InSourceFiles,InPublicIncludePaths,InPublicSystemIncludePaths,InDefinitions,
			InPublicIncludePathModuleNames,InPublicDependencyModuleNames,InPublicDelayLoadDLLs,InPublicAdditionalLibraries,InPublicFrameworks,InPublicAdditionalFrameworks,InPublicAdditionalShadowFiles,
			InPrivateIncludePaths,InPrivateIncludePathModuleNames,InPrivateDependencyModuleNames,
            InCircularlyReferencedDependentModules, InDynamicallyLoadedModuleNames, InPlatformSpecificDynamicallyLoadedModuleNames, InOptimizeCode,
			InAllowSharedPCH, InSharedPCHHeaderFile, InUseRTTI, InEnableBufferSecurityChecks, InFasterWithoutUnity, InMinFilesUsingPrecompiledHeaderOverride,
			InEnableExceptions, InEnableInlining)
		{
			PrivateAssemblyReferences = ListFromOptionalEnumerableStringParameter(InPrivateAssemblyReferences);
		}

		// UEBuildModule interface.
		public override List<FileItem> Compile( CPPEnvironment GlobalCompileEnvironment, CPPEnvironment CompileEnvironment, bool bCompileMonolithic )
		{
			var ModuleCLREnvironment = new CPPEnvironment(CompileEnvironment);

			// Setup the module environment for the project CLR mode
			ModuleCLREnvironment.Config.CLRMode = CPPCLRMode.CLREnabled;

			// Add the private assembly references to the compile environment.
			foreach(var PrivateAssemblyReference in PrivateAssemblyReferences)
			{
				ModuleCLREnvironment.AddPrivateAssembly(PrivateAssemblyReference);
			}

			// Pass the CLR compilation environment to the standard C++ module compilation code.
			return base.Compile(GlobalCompileEnvironment, ModuleCLREnvironment, bCompileMonolithic );
		}

		public override void SetupPrivateLinkEnvironment(
			ref LinkEnvironment LinkEnvironment,
			ref List<UEBuildBinary> BinaryDependencies,
			ref Dictionary<UEBuildModule, bool> VisitedModules
			)
		{
			base.SetupPrivateLinkEnvironment(ref LinkEnvironment,ref BinaryDependencies,ref VisitedModules);

			// Setup the link environment for linking a CLR binary.
			LinkEnvironment.Config.CLRMode = CPPCLRMode.CLREnabled;
		}
	}

	public class UEBuildFramework
	{
		public UEBuildFramework( string InFrameworkName )
		{
			FrameworkName = InFrameworkName;
		}

		public UEBuildFramework( string InFrameworkName, string InFrameworkZipPath, string InCopyBundledAssets )
		{
			FrameworkName		= InFrameworkName;
			FrameworkZipPath	= InFrameworkZipPath;
			CopyBundledAssets	= InCopyBundledAssets;
		}

		public UEBuildModule	OwningModule		= null;
		public string			FrameworkName		= null;
		public string			FrameworkZipPath	= null;
		public string			CopyBundledAssets	= null;
	}
}

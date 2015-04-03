// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Linq;
using System.Xml.Serialization;
using System.ComponentModel;

namespace UnrealBuildTool
{
	public enum BuildProductType
	{
		Executable,
		DynamicLibrary,
		StaticLibrary,
		ImportLibrary,
		SymbolFile,
		RequiredResource,
	}

	[Serializable]
	public class BuildProduct
	{
		[XmlAttribute]
		public string Path;

		[XmlAttribute]
		public BuildProductType Type;

		[XmlAttribute, DefaultValue(false)]
		public bool IsPrecompiled;

		private BuildProduct()
		{
		}

		public BuildProduct(string InPath, BuildProductType InType)
		{
			Path = InPath;
			Type = InType;
		}

		public override string ToString()
		{
			return Path;
		}
	}

	[Serializable]
	public class RuntimeDependency
	{
		[XmlAttribute]
		public string Path;

		[XmlAttribute, DefaultValue(null)]
		public string StagePath;

		private RuntimeDependency()
		{
		}

		public RuntimeDependency(string InPath)
		{
			Path = InPath;
		}

		public RuntimeDependency(string InPath, string InStagePath)
		{
			Path = InPath;
			StagePath = InStagePath;
		}

		public RuntimeDependency(RuntimeDependency InOther)
		{
			Path = InOther.Path;
			StagePath = InOther.StagePath;
		}

		public override string ToString()
		{
			return (StagePath == null)? Path : String.Format("{0} -> {1}", Path, StagePath);
		}
	}

	/// <summary>
	/// Stores a record of a built target, with all metadata that other tools may need to know about the build.
	/// </summary>
	[Serializable]
	public class BuildReceipt
	{
		const string EngineDirVariable = "$(EngineDir)";
		const string ProjectDirVariable = "$(ProjectDir)";

		[XmlArrayItem("BuildProduct")]
		public List<BuildProduct> BuildProducts = new List<BuildProduct>();

		[XmlArrayItem("RuntimeDependency")]
		public List<RuntimeDependency> RuntimeDependencies = new List<RuntimeDependency>();

		/// <summary>
		/// Default constructor
		/// </summary>
		public BuildReceipt()
		{
		}

		/// <summary>
		/// Adds a build product to the receipt. Does not check whether it already exists.
		/// </summary>
		/// <param name="Path">Path to the build product.</param>
		/// <param name="Type">Type of build product.</param>
		/// <returns>The BuildProduct object that was created</returns>
		public BuildProduct AddBuildProduct(string Path, BuildProductType Type)
		{
			BuildProduct NewBuildProduct = new BuildProduct(Path, Type);
			BuildProducts.Add(NewBuildProduct);
			return NewBuildProduct;
		}

		/// <summary>
		/// Constructs a runtime dependency object and adds it to the receipt.
		/// </summary>
		/// <param name="Path">Source path for the dependency</param>
		/// <param name="StagePath">Location for the dependency in the staged build</param>
		/// <returns>The RuntimeDependency object that was created</returns>
		public RuntimeDependency AddRuntimeDependency(string Path, string StagePath)
		{
			RuntimeDependency NewRuntimeDependency = new RuntimeDependency(Path, StagePath);
			RuntimeDependencies.Add(NewRuntimeDependency);
			return NewRuntimeDependency;
		}

		/// <summary>
		/// Appends another receipt to this one.
		/// </summary>
		/// <param name="Other">Receipt which should be appended</param>
		public void Append(BuildReceipt Other)
		{
			foreach(BuildProduct OtherBuildProduct in Other.BuildProducts)
			{
				BuildProducts.Add(OtherBuildProduct);
			}
			foreach(RuntimeDependency OtherRuntimeDependency in Other.RuntimeDependencies)
			{
				RuntimeDependencies.Add(OtherRuntimeDependency);
			}
		}

		/// <summary>
		/// Inserts $(EngineDir) and $(ProjectDir) variables into a path string, so it can be used on different machines.
		/// </summary>
		/// <param name="InputPath">Input path</param>
		/// <param name="EngineDir">The engine directory. Relative paths are ok.</param>
		/// <param name="ProjectDir">The project directory. Relative paths are ok.</param>
		/// <returns>New string with the base directory replaced, or the original string</returns>
		public static string InsertPathVariables(string InputPath, string EngineDir, string ProjectDir)
		{
			if(!InputPath.StartsWith("$("))
			{
				string FullInputPath = Path.GetFullPath(InputPath);

				string FullEngineDir = Path.GetFullPath(EngineDir).TrimEnd(Path.DirectorySeparatorChar);
				if(FullInputPath.StartsWith(FullEngineDir + Path.DirectorySeparatorChar, StringComparison.InvariantCultureIgnoreCase))
				{
					return EngineDirVariable + FullInputPath.Substring(FullEngineDir.Length);
				}

				string FullProjectDir = Path.GetFullPath(ProjectDir).TrimEnd(Path.DirectorySeparatorChar);
				if(FullInputPath.StartsWith(FullProjectDir + Path.DirectorySeparatorChar, StringComparison.InvariantCultureIgnoreCase))
				{
					return ProjectDirVariable + FullInputPath.Substring(FullProjectDir.Length);
				}
			}
			return InputPath;
		}

		/// <summary>
		/// Expands any $(EngineDir) and $(ProjectDir) variables in the given path.
		/// </summary>
		/// <param name="InputPath">Input path</param>
		/// <param name="EngineDir">The engine directory. Relative paths will be converted to absolute paths in the output string.</param>
		/// <param name="ProjectDir">The project directory. Relative paths will be converted to absolute paths in the output string</param>
		/// <returns>Full path with variable strings replaced, or the original string.</returns>
		public static string ExpandPathVariables(string InputPath, string EngineDir, string ProjectDir)
		{
			if(InputPath.StartsWith("$("))
			{
				if(InputPath.StartsWith(EngineDirVariable))
				{
					return Path.GetFullPath(EngineDir.TrimEnd('/', '\\') + InputPath.Substring(EngineDirVariable.Length));
				}
				else if(InputPath.StartsWith(ProjectDirVariable))
				{
					return Path.GetFullPath(ProjectDir.TrimEnd('/', '\\') + InputPath.Substring(ProjectDirVariable.Length));
				}
			}
			return InputPath;
		}

		/// <summary>
		/// Read a receipt from disk.
		/// </summary>
		/// <param name="FileName">Filename to read from</param>
		public static BuildReceipt Read(string FileName)
		{
			XmlSerializer Serializer = new XmlSerializer(typeof(BuildReceipt));
			using(StreamReader Reader = new StreamReader(FileName))
			{
				return (BuildReceipt)Serializer.Deserialize(Reader);
			}
		}

		/// <summary>
		/// Write the receipt to disk.
		/// </summary>
		/// <param name="FileName">Output filename</param>
		public void Write(string FileName)
		{
			XmlSerializer Serializer = new XmlSerializer(typeof(BuildReceipt));
			using(StreamWriter Writer = new StreamWriter(FileName))
			{
				Serializer.Serialize(Writer, this);
			}
		}
	}
}

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace APIDocTool
{
	public class BuildModule
	{
		public string Name;
		public string Path;
		public string Type;
		public List<string> Dependencies = new List<string>();
		public List<string> Definitions = new List<string>();
		public List<string> IncludePaths = new List<string>();

		public BuildModule(string InName, string InPath, string InType)
		{
			Name = InName;
			Path = InPath;
			Type = InType;
		}

		public override string ToString()
		{
			return Name;
		}

		public static BuildModule FromXml(XmlNode Node)
		{
			BuildModule NewModule = new BuildModule(Node.Attributes["name"].Value, Node.Attributes["path"].Value, Node.Attributes["type"].Value);
			if (NewModule.Type == "cpp")
			{
				using (XmlNodeList DependencyNodeList = Node.SelectNodes("dependencies/dependency"))
				{
					foreach (XmlNode DependencyNode in DependencyNodeList)
					{
						NewModule.Dependencies.Add(DependencyNode.Attributes["module"].Value);
					}
				}
				using (XmlNodeList DefinitionNodeList = Node.SelectNodes("definitions/definition"))
				{
					foreach (XmlNode DefinitionNode in DefinitionNodeList)
					{
						NewModule.Definitions.Add(DefinitionNode.Attributes["name"].Value);
					}
				}
				using (XmlNodeList IncludeNodeList = Node.SelectNodes("includes/include"))
				{
					foreach (XmlNode IncludeNode in IncludeNodeList)
					{
						NewModule.IncludePaths.Add(IncludeNode.Attributes["path"].Value);
					}
				}
			}
			return NewModule;
		}
	}

	public class BuildBinary
	{
		public string Name;
		public string Type;
		public List<BuildModule> Modules = new List<BuildModule>();

		public BuildBinary(string InName, string InType)
		{
			Name = InName;
			Type = InType;
		}

		public BuildBinary Filter(List<string> ModuleNames)
		{
			BuildBinary NewBinary = new BuildBinary(Name, Type);
			foreach (BuildModule Module in Modules)
			{
				if (ModuleNames.Contains(Module.Name))
				{
					NewBinary.Modules.Add(Module);
				}
			}
			return NewBinary;
		}

		public override string ToString()
		{
			return Name;
		}

		public static BuildBinary FromXml(XmlNode Node)
		{
			BuildBinary NewBinary = new BuildBinary(Node.Attributes["name"].Value, Node.Attributes["type"].Value);
			if (NewBinary.Type == "cpp")
			{
				using (XmlNodeList ModuleNodeList = Node.SelectNodes("module"))
				{
					foreach (XmlNode ModuleNode in ModuleNodeList)
					{
						BuildModule NewModule = BuildModule.FromXml(ModuleNode);
						NewBinary.Modules.Add(NewModule);
					}
				}
			}
			return NewBinary;
		}
	}

	public class BuildTarget
	{
		public string Name;
		public List<BuildBinary> Binaries = new List<BuildBinary>();

		public BuildTarget(string InName)
		{
			Name = InName;
		}

		public BuildTarget Filter(List<string> ModuleNames)
		{
			BuildTarget NewTarget = new BuildTarget(Name);
			foreach (BuildBinary Binary in Binaries)
			{
				BuildBinary NewBinary = Binary.Filter(ModuleNames);
				if (NewBinary.Modules.Count > 0) NewTarget.Binaries.Add(NewBinary);
			}
			return NewTarget;
		}

		public override string ToString()
		{
			return Name;
		}

		public static BuildTarget FromXml(XmlNode Node)
		{
			BuildTarget NewTarget = new BuildTarget(Node.Attributes["name"].Value);
			using (XmlNodeList BinaryNodeList = Node.SelectNodes("binary"))
			{
				foreach (XmlNode BinaryNode in BinaryNodeList)
				{
					BuildBinary NewBinary = BuildBinary.FromXml(BinaryNode);
					NewTarget.Binaries.Add(NewBinary);
				}
			}
			return NewTarget;
		}

		public static BuildTarget Read(string InputPath)
		{
			// Parse the xml document
			XmlDocument TargetDocument = Utility.ReadXmlDocument(InputPath);

			// Read the target definition
			XmlNode TargetNode = TargetDocument.SelectSingleNode("target");
			return BuildTarget.FromXml(TargetNode);
		}
	}
}

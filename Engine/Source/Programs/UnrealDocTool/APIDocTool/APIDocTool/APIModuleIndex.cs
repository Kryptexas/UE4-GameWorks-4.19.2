// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using DoxygenLib;
using System.IO;

namespace APIDocTool
{
	class APIModuleCategory
	{
		public string Name;
		public List<string> MajorModules = new List<string>();
		public List<string> MinorModules = new List<string>();
		public List<APIModuleCategory> Categories = new List<APIModuleCategory>();

		public APIModuleCategory(string InName)
		{
			Name = InName;
		}

		public void GetModules(List<string> Modules)
		{
			Modules.AddRange(MajorModules);
			Modules.AddRange(MinorModules);

			foreach (APIModuleCategory Category in Categories)
			{
				Category.GetModules(Modules);
			}
		}

		public void AddModuleByPath(string Name, string BaseSrcDir, bool bIsMajorModule)
		{
			// Find the description and category
			string ModuleSettingsPath = "Module." + Name;
			string CategoryText = Program.Settings.FindValueOrDefault(ModuleSettingsPath + ".Category", null);

			// Get a default category based on the module directory if there's not one set
			if (CategoryText == null)
			{
				const string EngineSourcePath = "Engine/Source/";
				string NormalizedSrcPath = Path.GetFullPath(BaseSrcDir).Replace('\\', '/').TrimEnd('/');

				int CategoryMinIdx = NormalizedSrcPath.IndexOf(EngineSourcePath);
				if(CategoryMinIdx != -1) CategoryMinIdx += EngineSourcePath.Length;
					
				int CategoryMaxIdx = NormalizedSrcPath.LastIndexOf('/');
				if (CategoryMinIdx >= 0 && CategoryMaxIdx > CategoryMinIdx)
				{
					CategoryText = NormalizedSrcPath.Substring(CategoryMinIdx, CategoryMaxIdx - CategoryMinIdx).Replace('/', '|');
				}
				else
				{
					CategoryText = "Other";
				}
			}

			// Build a module from all the members
			AddModuleByCategory(Name, CategoryText, bIsMajorModule);
		}
		
		public void AddModuleByCategory(string Name, string CategoryText, bool bIsMajorModule)
		{
			APIModuleCategory Category = AddCategory(CategoryText);
			if (bIsMajorModule)
			{
				Category.MajorModules.Add(Name);
			}
			else
			{
				Category.MinorModules.Add(Name);
			}
		}

		public void AddModules(KeyValuePair<string, string>[] NamePathList)
		{
			// Read the layout settings from the ini file
			string[] CategoryOrder = Program.Settings.FindValueOrDefault("Index.CategoryOrder", "").Split('\n');
			string[] ModuleOrder = Program.Settings.FindValueOrDefault("Index.ModuleOrder", "").Split('\n');
			HashSet<string> MajorModules = new HashSet<string>(Program.Settings.FindValueOrDefault("Index.MajorModules", "").Split('\n'));

			// Add all the categories specified in the order
			foreach (string CategoryName in CategoryOrder)
			{
				AddCategory(CategoryName);
			}

			// Add all the module names whose order is specified
			foreach (string ModuleName in ModuleOrder)
			{
				for(int Idx = 0; Idx < NamePathList.Length; Idx++)
				{
					if (NamePathList[Idx].Key == ModuleName)
					{
						AddModuleByPath(ModuleName, NamePathList[Idx].Value, MajorModules.Contains(ModuleName));
						break;
					}
				}
			}

			// Add all the other modules to the tree
			HashSet<string> AddedModules = new HashSet<string>(ModuleOrder);
			foreach(KeyValuePair<string, string> NamePathPair in NamePathList)
			{
				if (!AddedModules.Contains(NamePathPair.Key))
				{
					AddModuleByPath(NamePathPair.Key, NamePathPair.Value, MajorModules.Contains(NamePathPair.Key));
				}
			}
		}

		public APIModuleCategory AddCategory(string Path)
		{
			return AddCategory(Path.Split('|'), 0);
		}

		private APIModuleCategory AddCategory(string[] Path, int PathIdx)
		{
			// If we're at the end of the path, return this category
			if (PathIdx == Path.Length)
			{
				return this;
			}

			// Otherwise try to find an existing category
			foreach (APIModuleCategory Category in Categories)
			{
				if (String.Compare(Category.Name, Path[PathIdx], true) == 0)
				{
					return Category.AddCategory(Path, PathIdx + 1);
				}
			}

			// Otherwise create a new category, and add it to that
			APIModuleCategory NewCategory = new APIModuleCategory(Path[PathIdx]);
			Categories.Add(NewCategory);
			return NewCategory.AddCategory(Path, PathIdx + 1);
		}

		public bool IsEmpty
		{
			get { return MajorModules.Count == 0 && MinorModules.Count == 0 && Categories.All(x => x.IsEmpty); }
		}

		public void Write(UdnWriter Writer, int Depth, Dictionary<string, APIModule> Modules)
		{
			if (!IsEmpty)
			{
				// Find all the major modules in this category
				if (MajorModules.Count > 0)
				{
					Writer.WriteList("Name", "Description", MajorModules.Select(x => Modules[x].GetListItem()));
				}

				// Write all the minor modules in this category
				if (MinorModules.Count > 0)
				{
					Writer.EnterRegion("syntax");
					Writer.WriteFilterList(MinorModules.Select(x => Modules[x].GetFilterListItem()).ToArray());
					Writer.LeaveRegion();
				}

				// Write all the subcategories
				foreach (APIModuleCategory Category in Categories)
				{
					Writer.WriteHeading(Depth, Category.Name);
					Writer.WriteLine();
					Category.Write(Writer, Depth + 1, Modules);
				}
			}
		}

		public override string ToString()
		{
			return Name;
		}
	}

	class APIModuleIndex : APIPage
	{
		public APIModuleCategory Category;
		public List<APIModule> Modules = new List<APIModule>();
		public Dictionary<string, APIModule> ModuleLookup = new Dictionary<string,APIModule>();

		public APIModuleIndex(APIPage Parent, APIModuleCategory InCategory, IEnumerable<DoxygenModule> InModules)
			: base(Parent, InCategory.Name)
		{
			Category = InCategory;

			List<string> ModuleNames = new List<string>();
			Category.GetModules(ModuleNames);

			foreach (DoxygenModule InModule in InModules.Where(x => ModuleNames.Contains(x.Name)))
			{
				APIModule Module = APIModule.Build(this, InModule);
				Modules.Add(Module);
				ModuleLookup.Add(Module.Name, Module);
			}
		}

		public override void GatherReferencedPages(List<APIPage> Pages)
		{
			Pages.AddRange(Modules);
		}

		public override SitemapNode CreateSitemapNode()
		{
			SitemapNode Node = new SitemapNode(Name, SitemapLinkPath);
			Node.Children.AddRange(Modules.OrderBy(x => x.Name).Select(x => x.CreateSitemapNode()));
			return Node;
		}

		public override void AddToManifest(UdnManifest Manifest)
		{
			Manifest.Add("ModuleIndex:" + Name, this);

			foreach (APIModule Module in Modules)
			{
				Module.AddToManifest(Manifest);
			}
		}

		public void WriteModuleList(UdnWriter Writer, int Depth)
		{
			Category.Write(Writer, Depth, ModuleLookup);
		}

		public override void WritePage(UdnManifest Manifest, string OutputPath)
		{
			using (UdnWriter Writer = new UdnWriter(OutputPath))
			{
				Writer.WritePageHeader(Name, PageCrumbs, "Module Index");
				WriteModuleList(Writer, 2);
			}
		}
	}
}

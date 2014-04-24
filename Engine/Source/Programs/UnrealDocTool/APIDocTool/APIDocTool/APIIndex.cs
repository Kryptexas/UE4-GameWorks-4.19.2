// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using DoxygenLib;
using System.Diagnostics;

namespace APIDocTool
{
	class APIIndex : APIPage
	{
		public List<APIPage> Pages = new List<APIPage>();
		public List<APIModuleIndex> ChildModuleIndexes = new List<APIModuleIndex>();

		public APIGettingStarted GettingStarted;
		public APIModuleIndex ModuleIndex;
		public APIConstantIndex ConstantIndex;
		public APIFunctionIndex FunctionIndex;
		public APITypeIndex TypeIndex;
		public APIHierarchy RecordHierarchy;

		public static APIModuleCategory FindRootCategoryByPath(List<APIModuleCategory> Categories, string InPath)
		{
			const string EngineSourcePath = "Engine/Source/";
			string NormalizedSrcPath = Path.GetFullPath(InPath).Replace('\\', '/').TrimEnd('/');

			int CategoryMinIdx = NormalizedSrcPath.LastIndexOf(EngineSourcePath);
			if (CategoryMinIdx != -1)
			{
				CategoryMinIdx += EngineSourcePath.Length;

				int CategoryMaxIdx = CategoryMinIdx;
				while (CategoryMaxIdx < NormalizedSrcPath.Length && NormalizedSrcPath[CategoryMaxIdx] != '/') CategoryMaxIdx++;

				string CategoryName = NormalizedSrcPath.Substring(CategoryMinIdx, CategoryMaxIdx - CategoryMinIdx) + " Modules";

				APIModuleCategory Category = Categories.FirstOrDefault(x => x.Name.ToLower() == CategoryName.ToLower());
				if(Category != null) return Category;
			}

			return Categories.Last();
		}

		public APIIndex(List<DoxygenModule> InModules)
			: base(null, "API")
		{
			// Build a mapping of module names to modules
			Dictionary<string, DoxygenModule> NameToModuleMap = new Dictionary<string,DoxygenModule>();
			foreach(DoxygenModule Module in InModules)
			{
				NameToModuleMap.Add(Module.Name, Module);
			}

			// Add some default categories to fix the order
			APIModuleCategory RootCategory = new APIModuleCategory("All Modules");
			RootCategory.Categories.Add(new APIModuleCategory("Runtime Modules"));
			RootCategory.Categories.Add(new APIModuleCategory("Editor Modules"));
			RootCategory.Categories.Add(new APIModuleCategory("Developer Modules"));

			// Add all of the modules to a category
			Dictionary<DoxygenModule, APIModuleCategory> ModuleToCategory = new Dictionary<DoxygenModule, APIModuleCategory>();
			foreach(DoxygenModule Module in InModules)
			{
				APIModuleCategory Category = FindRootCategoryByPath(RootCategory.Categories, Module.BaseSrcDir);
				ModuleToCategory.Add(Module, Category);
			}

			// Create all the index pages
			foreach (APIModuleCategory Category in RootCategory.Categories)
			{
				APIModuleIndex ChildModuleIndex = new APIModuleIndex(this, Category);
				ChildModuleIndexes.Add(ChildModuleIndex);
			}
			Pages.AddRange(ChildModuleIndexes);

			// Populate all the categories and create all the module pages
			foreach(KeyValuePair<DoxygenModule, APIModuleCategory> Pair in ModuleToCategory)
			{
				APIModuleIndex Parent = ChildModuleIndexes.First(x => x.Category == Pair.Value);

				APIModule Module = APIModule.Build(Parent, Pair.Key);
				Parent.Children.Add(Module);

				Pair.Value.AddModule(Module);
			}

			// Expand the Core section by default
			APIModuleCategory DefaultCategory = RootCategory.Categories[0].Categories.FirstOrDefault(x => x.Name == "Core Runtime Modules");
			if (DefaultCategory != null) DefaultCategory.Expanded = true;

			// Get all the members that were created as part of building the modules. After this point we'll only create index pages.
			List<APIMember> AllMembers = new List<APIMember>(GatherPages().OfType<APIMember>().OrderBy(x => x.FullName));
			foreach (APIMember Member in AllMembers)
			{
				Member.Link();
			}
			foreach (APIMember Member in AllMembers)
			{
				Member.PostLink();
			}

			// Create the quick start page
			GettingStarted = new APIGettingStarted(this);
			Pages.Add(GettingStarted);

			// Create an index of all the modules
			ModuleIndex = new APIModuleIndex(this, RootCategory);
			Pages.Add(ModuleIndex);

			// Create an index of all the constants
			ConstantIndex = new APIConstantIndex(this, AllMembers);
			Pages.Add(ConstantIndex);

			// Create an index of all the functions
			FunctionIndex = new APIFunctionIndex(this, AllMembers.OfType<APIFunction>().Where(x => !(x.ScopeParent is APIRecord)));
			Pages.Add(FunctionIndex);

			// Create an index of all the types
			TypeIndex = new APITypeIndex(this, AllMembers);
			Pages.Add(TypeIndex);

			// Create an index of all the classes
			RecordHierarchy = APIHierarchy.Build(this, AllMembers.OfType<APIRecord>());
			Pages.Add(RecordHierarchy);
		}

		public void WriteSitemapContents(string OutputPath)
		{
			List<SitemapNode> RootNodes = new List<SitemapNode>();
			RootNodes.Add(new SitemapNode("Contents", SitemapLinkPath));
			RootNodes.Add(new SitemapNode("Getting started", GettingStarted.SitemapLinkPath));
			foreach (APIModuleIndex ChildModuleIndex in ChildModuleIndexes)
			{
				RootNodes.Add(ChildModuleIndex.CreateSitemapNode());
			}
			RootNodes.Add(new SitemapNode("All modules", ModuleIndex.SitemapLinkPath));
			RootNodes.Add(new SitemapNode("All constants", ConstantIndex.SitemapLinkPath));
			RootNodes.Add(new SitemapNode("All functions", FunctionIndex.SitemapLinkPath));
			RootNodes.Add(new SitemapNode("All types", TypeIndex.SitemapLinkPath));
			RootNodes.Add(new SitemapNode("Class hierarchy", RecordHierarchy.SitemapLinkPath));
			Sitemap.WriteContentsFile(OutputPath, RootNodes);
		}

		public void WriteSitemapIndex(string OutputPath)
		{
			// Gather all the pages
			List<APIPage> AllPages = new List<APIPage>(GatherPages());

			// Find the shortest link path to each member
			Dictionary<string, List<APIMember>> MemberIndexLinks = new Dictionary<string, List<APIMember>>();
			foreach (APIMember Member in AllPages.OfType<APIMember>())
			{
				// Find or create the list of members for this link path
				List<APIMember> Members;
				if(!MemberIndexLinks.TryGetValue(Member.FullName, out Members))
				{
					Members = new List<APIMember>();
					MemberIndexLinks.Add(Member.FullName, Members);
				}

				// Remove any members 
				Members.RemoveAll(x => x.HasParent(Member));
				if(!Members.Any(x => Member.HasParent(x))) Members.Add(Member);
			}

			// Create all the root pages
			List<SitemapNode> RootNodes = new List<SitemapNode>();
			foreach(APIPage Page in AllPages)
			{
				if (!(Page is APIMember))
				{
					RootNodes.Add(new SitemapNode(Page.Name, Page.SitemapLinkPath));
				}
			}
			foreach(KeyValuePair<string, List<APIMember>> MemberIndexLink in MemberIndexLinks)
			{
				foreach(APIMember Member in MemberIndexLink.Value)
				{
					RootNodes.Add(new SitemapNode(MemberIndexLink.Key, Member.SitemapLinkPath));
				}
			}

			// Write the index file
			Sitemap.WriteIndexFile(OutputPath, RootNodes);
		}

		public override void GatherReferencedPages(List<APIPage> ReferencedPages)
		{
			ReferencedPages.AddRange(ChildModuleIndexes);
			ReferencedPages.AddRange(Pages);
		}

		public override void AddToManifest(UdnManifest Manifest)
		{
			foreach (APIPage Page in Pages)
			{
				Page.AddToManifest(Manifest);
			}
		}

		public override void WritePage(UdnManifest Manifest, string OutputPath)
		{
			using (UdnWriter Writer = new UdnWriter(OutputPath))
			{
				Writer.WritePageHeader("Unreal Engine API Reference", PageCrumbs, "Unreal Engine API Reference");

				Writer.WriteHeading(2, "Disclaimer");

				Writer.WriteLine("The API reference is an early work in progress, and some information may be missing or out of date. It serves mainly as a low level index of Engine classes and functions.");
				Writer.WriteLine("For tutorials, walkthroughs and detailed guides to programming with Unreal, please see the [Unreal Engine Programming](http://docs.unrealengine.com/latest/INT/Programming/index.html) home on the web.");

				Writer.WriteHeading(2, "Introduction");

				Writer.WriteLine(Manifest, "The Unreal Engine consists of a large number of modules divided into three categories: the {Runtime|ModuleIndex:Runtime Modules}, functionality for the {Editor|ModuleIndex:Editor Modules}, and {Developer|ModuleIndex:Developer Modules} utilities. Full documentation for each of these categories is below.");
				Writer.WriteLine();
				Writer.WriteLine("To explore the API from some of the most frequently encountered Unreal concepts and types, see the API __[getting started](" + GettingStarted.LinkPath + ")__ page.");

				Writer.WriteHeading(2, "Contents");

				Writer.EnterRegion("memberindexlinks");
				Writer.WriteLine("[Getting started]({0}) &middot; [All modules]({1}) &middot; [All constants]({2}) &middot; [All functions]({3}) &middot; [All types]({4}) &middot; [Class hierarchy]({5})", GettingStarted.LinkPath, ModuleIndex.LinkPath, ConstantIndex.LinkPath, FunctionIndex.LinkPath, TypeIndex.LinkPath, RecordHierarchy.LinkPath);
				Writer.LeaveRegion();

				foreach(APIModuleIndex ChildModuleIndex in ChildModuleIndexes)
				{
					Writer.WriteHeading(2, ChildModuleIndex.Name);
					Writer.EnterRegion("modules-list");
					ChildModuleIndex.WriteExpandableModuleList(Writer, ChildModuleIndex.Name.Replace(' ', '_'));
					Writer.LeaveRegion();
				}

				Writer.WriteLine("<br>");
			}
		}
	}
}

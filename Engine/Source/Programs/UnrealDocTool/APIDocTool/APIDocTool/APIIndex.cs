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
		public static string[] QuickLinkPaths = 
		{
			"API\\Runtime\\CoreUObject\\UObject\\UObject",
			"API\\Runtime\\Engine\\GameFramework\\AActor",
			"API\\Runtime\\Engine\\Components\\UActorComponent",
		};

		public List<APIPage> Pages = new List<APIPage>();
		public List<APIModuleIndex> ModuleIndexes = new List<APIModuleIndex>();

		public APIConstantIndex ConstantIndex;
		public APIFunctionIndex FunctionIndex;
		public APITypeIndex TypeIndex;
		public APIHierarchy RecordHierarchy;

		public List<APIMember> QuickLinks = new List<APIMember>();

		public APIIndex(List<DoxygenModule> InModules)
			: base(null, "API")
		{
			// Build a mapping of module names to modules
			Dictionary<string, DoxygenModule> NameToModuleMap = new Dictionary<string,DoxygenModule>();
			foreach(DoxygenModule Module in InModules)
			{
				NameToModuleMap.Add(Module.Name, Module);
			}

			// Create a module category tree
			APIModuleCategory RootCategory = new APIModuleCategory(null);
			RootCategory.AddModules(InModules.Select(x => new KeyValuePair<string, string>(x.Name, x.BaseSrcDir)).ToArray());
			Debug.Assert(RootCategory.MinorModules.Count == 0 && RootCategory.MajorModules.Count == 0);

			// Create all the index pages
			foreach (APIModuleCategory Category in RootCategory.Categories)
			{
				if (!Category.IsEmpty)
				{
					APIModuleIndex ModuleIndex = new APIModuleIndex(this, Category, InModules);
					ModuleIndexes.Add(ModuleIndex);
				}
			}
			Pages.AddRange(ModuleIndexes);

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

			// Create all the quick links
			foreach (string QuickLinkPath in QuickLinkPaths)
			{
				APIMember Member = AllMembers.FirstOrDefault(x => x.LinkPath == QuickLinkPath);
				if (Member != null) QuickLinks.Add(Member);
			}
		}

		public void WriteSitemapContents(string OutputPath)
		{
			List<SitemapNode> RootNodes = new List<SitemapNode>();
			RootNodes.Add(new SitemapNode("Introduction", SitemapLinkPath));
			foreach (APIModuleIndex ModuleIndex in ModuleIndexes)
			{
				RootNodes.Add(ModuleIndex.CreateSitemapNode());
			}
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
			ReferencedPages.AddRange(ModuleIndexes);
			ReferencedPages.AddRange(Pages);
		}

		public override void AddToManifest(UdnManifest Manifest)
		{
			foreach (APIPage Page in Pages)
			{
				Page.AddToManifest(Manifest);
			}
		}

		public string GetModuleIndexLink(string Name)
		{
			foreach(APIModuleIndex ModuleIndex in ModuleIndexes)
			{
				if (ModuleIndex.Name == Name)
				{
					return String.Format("[{0}]({1})", Name, ModuleIndex.LinkPath);
				}
			}
			return Name;
		}

		public string MakeLinkString(Dictionary<string, List<APIPage>> Manifest, string Text)
		{
			StringBuilder Output = new StringBuilder();
			for (int Idx = 0; Idx < Text.Length; Idx++)
			{
				if(Text[Idx] == '{')
				{
					if(Text[Idx + 1] == '{')
					{
						Output.Append(Text[++Idx]);
					}
					else
					{
						int EndIdx = Text.IndexOf('}', Idx + 1);
						string LinkText = Text.Substring(Idx + 1, EndIdx - Idx - 1);
						string LinkName = LinkText;

						int LinkNameIdx = LinkText.IndexOf('|');
						if(LinkNameIdx != -1)
						{
							LinkName = LinkText.Substring(LinkNameIdx + 1);
							LinkText = LinkText.Substring(0, LinkNameIdx);
						}
					
						List<APIPage> LinkPages;
						if (Manifest.TryGetValue(LinkName, out LinkPages))
						{
							Output.AppendFormat("[{0}]({1})", LinkText, LinkPages[0].LinkPath);
						}
						else
						{
							Output.Append(LinkText);
						}
						Idx = EndIdx;
					}
				}
				else
				{
					Output.Append(Text[Idx]);
				}
			}
			return Output.ToString();
		}

		public void WriteManifestLinks(UdnWriter Writer, Dictionary<string, List<APIPage>> Manifest, string Text)
		{
			Writer.WriteLine(MakeLinkString(Manifest, Text));
		}

		public void WriteIndexLinks(UdnWriter Writer, Dictionary<string, List<APIPage>> Manifest, string Text)
		{
			Writer.EnterTag("[REGION:memberindexlinks]");
			Writer.WriteLine(MakeLinkString(Manifest, Text));
			Writer.LeaveTag("[/REGION]");
		}

		public override void WritePage(UdnManifest Manifest, string OutputPath)
		{
			using (UdnWriter Writer = new UdnWriter(OutputPath))
			{
				Writer.WritePageHeader("Unreal Engine API Reference", PageCrumbs, "Unreal Engine API Reference");

				Writer.WriteHeading(2, "Disclaimer");

				Writer.WriteLine("The API reference is an early work in progress, and some information may be missing or out of date. It serves mainly as a low level index of Engine classes and functions.");
				Writer.WriteLine("For tutorials, walkthroughs and detailed guides to programming with Unreal, please see the [Unreal Engine Programming](http://docs.unrealengine.com/latest/INT/Programming/index.html) home on the web.");

				Writer.WriteHeading(2, "Orientation");

				Writer.WriteLine("Games, programs and the Unreal Editor are all *targets* built by UnrealBuildTool. Each target is compiled from C++ *modules*, each implementing a particular area of functionality. Your game is a target, and your game code is implemented in one or more modules.");
				Writer.WriteLine();
				Writer.WriteLine("Code in each module can use other modules by referencing them in their *build rules*. Build rules for each module are given by C# scripts with the .build.cs extension. *Target rules* are given by C# scripts with the .target.cs extension.");
				Writer.WriteLine();
				Writer.WriteLine(Manifest, "The Unreal Engine consists of a large number of modules divided into three categories: the common {Runtime|ModuleIndex:Runtime}, {Editor|ModuleIndex:Editor} specific functionality, and {Developer|ModuleIndex:Developer} utilities. Three of the most important runtime modules are {Core|Filter:Core}, {CoreUObject|Filter:CoreUObject} and {Engine|Filter:Engine}.");

				Writer.WriteHeading(2, "Core");

				Writer.WriteLine(Manifest, "The **{Core|Filter:Core}** module provides a common framework for Unreal modules to communicate; a standard set of types, a {math library|Filter:Core.Math}, a {container|Filter:Core.Containers} library, and a lot of the hardware abstraction that allows Unreal to run on so many platforms.");
				Writer.WriteLine();
				Writer.WriteLine("| Topic | Links |");
				Writer.WriteLine("| ----- | ----- |");
				Writer.WriteLine(Manifest, "|Basic types:| bool &middot; float/double &middot; {int8}/{int16}/{int32}/{int64} &middot; {uint8}/{uint16}/{uint32}/{uint64} &middot; {ANSICHAR} &middot; {TCHAR} &middot; {FString}|");
				Writer.WriteLine(Manifest, "|Math:| {FMath} &middot; {FVector} &middot; {FRotator} &middot; {FTransform} &middot; {FMatrix} &middot; {More...|Filter:Core.Math} |");
				Writer.WriteLine(Manifest, "|Containers:| {TArray} &middot; {TList} &middot; {TMap} &middot; {More...|Filter:Core.Containers} |");
				Writer.WriteLine(Manifest, "|Other:| {FName} &middot; {FArchive} &middot; {FOutputDevice} |");
				Writer.WriteLine();

				Writer.WriteHeading(2, "CoreUObject");

				Writer.WriteLine(Manifest, "The **{CoreUObject|Filter:CoreUObject}** module defines UObject, the base class for all managed objects in Unreal. Managed objects are key to integrating with the editor, for serialization, network replication, and runtime type information. UObject derived classes are garbage collected.");
				Writer.WriteLine();
				Writer.WriteLine("| Topic | Links |");
				Writer.WriteLine("| ----- | ----- |");
				Writer.WriteLine(Manifest, "|Classes:|{UObject} &middot; {UClass} &middot; {UProperty} &middot; {UPackage}|");
				Writer.WriteLine(Manifest, "|Functions:|{ConstructObject} &middot; {FindObject} &middot; {Cast} &middot; {CastChecked}|");
				Writer.WriteLine();

				Writer.WriteHeading(2, "Engine");

				Writer.WriteLine(Manifest, "The **{Engine|Filter:Engine}** module contains functionality you’d associate with a game. The game world, actors, characters, physics and special effects are all defined here.");
				Writer.WriteLine();
				Writer.WriteLine("| Topic | Links |");
				Writer.WriteLine("| ----- | ----- |");
				Writer.WriteLine(Manifest, "|Actors:|{AActor} &middot; {AVolume} &middot; {AGameMode} &middot; {AHUD} &middot; {More...|Hierarchy:AActor}");
				Writer.WriteLine(Manifest, "|Pawns:|{APawn} &middot; {ACharacter} &middot; {AWheeledVehicle}");
				Writer.WriteLine(Manifest, "|Controllers:|{AController} &middot; {AAIController} &middot; {APlayerController}"); 
				Writer.WriteLine(Manifest, "|Components:|{UActorComponent} &middot; {UBrainComponent} &middot; {UInputComponent} &middot; {USkeletalMeshComponent} &middot; {UParticleSystemComponent} &middot; {More...|Hierarchy:UActorComponent}|");
				Writer.WriteLine(Manifest, "|Gameplay:|{UPlayer} &middot; {ULocalPlayer} &middot; {UWorld} &middot; {ULevel}");
				Writer.WriteLine(Manifest, "|Assets:|{UTexture} &middot; {UMaterial} &middot; {UStaticMesh} &middot; {USkeletalMesh} &middot; {UParticleSystem}|");

				Writer.WriteLine("<br>");
			}
		}
	}
}

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.IO;
using System.Diagnostics;
using DoxygenLib;

namespace APIDocTool
{
	public enum APIProtection
	{
		Public,
		Protected,
		Private,
	}

	public enum APIRecordType
	{
		Class,
		Interface,
		Struct,
		Union,
	}

    public class APIRecord : APIMember
    {
		private static Dictionary<string, APIRecord> TemplateRecords = new Dictionary<string, APIRecord>();

		public readonly DoxygenEntity Entity;
		public readonly XmlNode Node;
		public readonly string RefId;
		public readonly APIProtection Protection;
		public readonly APIRecordType RecordType;

		public string BriefDescription;
		public string FullDescription;

		public string TemplateSignature;
		public string Definition;
		public List<string> BaseDefinitions = new List<string>();
		public string DeclarationFile;

		public MetadataDirective MetadataDirective;

		public List<KeyValuePair<XmlNode, APIRecord>> BaseRecords;
		public List<APIRecord> DerivedRecords = new List<APIRecord>();
		public APIHierarchy HierarchyPage;
		public APIHierarchyNode HierarchyNode;

        public List<List<APIRecord>> Crumbs = new List<List<APIRecord>>();

		public bool bIsTemplate;
		public bool bIsTemplateSpecialization;
		public List<APIRecord> TemplateSpecializations = new List<APIRecord>();

		// If this record is a delegate definition, the parameters for calling it
		public APIEventParameters DelegateEventParameters;

		public APIRecord(APIPage InParent, DoxygenEntity InEntity)
			: base(InParent, GetNameFromNode(InEntity.Node))
		{
			// Set all the readonly vars
			Entity = InEntity;
			Node = InEntity.Node;
			RefId = Node.Attributes["id"].InnerText;
			Protection = ParseProtection(Node);

			// Set the record type
			switch (Node.Attributes["kind"].Value)
			{
				case "class":
					RecordType = Name.StartsWith("I")? APIRecordType.Interface : APIRecordType.Class;
					break;
				case "struct":
					RecordType = APIRecordType.Struct;
					break;
				case "union":
					RecordType = APIRecordType.Union;
					break;
			}

			// Register the record for incoming links
			AddRefLink(RefId, this);

			// Add it to the template list
			if(Node.SelectSingleNode("templateparamlist") != null)
			{
				bIsTemplate = true;
				bIsTemplateSpecialization = Name.Contains('<');
				if(!bIsTemplateSpecialization && !TemplateRecords.ContainsKey(FullName)) TemplateRecords.Add(FullName, this);
			}
        }

		public override void AddToManifest(UdnManifest Manifest)
		{
			if (!bIsTemplateSpecialization)
			{
				base.AddToManifest(Manifest);

				if (HierarchyPage != null)
				{
					Manifest.Add("Hierarchy:" + FullName, HierarchyPage);
				}
			}
		}

		public static string GetNameFromNode(XmlNode Node)
		{
			string Name = Node.SelectSingleNode("compoundname").InnerText;

			// Find the last index of a scope operator, ignoring nested scopes in template operators
			int ScopeIndex = 0;
			for (int Index = 0, Depth = 0; Index < Name.Length; Index++)
			{
				if (Name[Index] == '(' || Name[Index] == '<' || Name[Index] == '{')
				{
					Depth++;
				}
				else if (Name[Index] == ')' || Name[Index] == '>' || Name[Index] == '}')
				{
					Depth--;
				}
				else if(Index + 2 < Name.Length && Name[Index] == ':' && Name[Index + 1] == ':' && Depth == 0)
				{
					ScopeIndex = Index + 2;
				}
			}

			// Return the last part of the name
			return Name.Substring(ScopeIndex);
		}

		public override void Link()
		{
			// Get all the relative classes
			BaseRecords = ParseRelatives(Node, "basecompoundref");

			// Add this record to all of its base classes
			foreach (KeyValuePair<XmlNode, APIRecord> BaseRecord in BaseRecords)
			{
				if (BaseRecord.Value != null)
				{
					BaseRecord.Value.DerivedRecords.Add(this);
				}
			}

			// Get the descriptions
			ParseBriefAndFullDescription(Node, out BriefDescription, out FullDescription);

			// Find the metadata for this definition
			MetadataDirective = MetadataCache.FindMetadataForMember(Node);

			// Build the definition
			TemplateSignature = ParseTemplateSignature(Node);
			Definition = Node.Attributes["kind"].InnerText + " " + Name;

			// Append all the base classes
			foreach(KeyValuePair<XmlNode, APIRecord> BaseRecord in BaseRecords)
			{
				string BaseDefinition = BaseRecord.Key.Attributes["prot"].Value;
				if(BaseRecord.Key.Attributes["virt"].Value == "virtual")
				{
					BaseDefinition += " virtual";
				}
				if(BaseRecord.Value == null)
				{
					BaseDefinition += " " + BaseRecord.Key.InnerText;
				}
				else
				{
					BaseDefinition += " [" + BaseRecord.Key.InnerText + "](" + BaseRecord.Value.LinkPath + ")";
				}
				BaseDefinitions.Add(BaseDefinition);
			}

			// Parse the file that it's declared in
			if(Entity.File != null)
			{
				int SourceIndex = Entity.File.IndexOf("/Engine/Source/");
				if (SourceIndex != -1) DeclarationFile = Entity.File.Substring(SourceIndex);
			}

			// If it's a specialization, add it to the parent class
			if(bIsTemplateSpecialization)
			{
				int StripIdx = FullName.LastIndexOf('<');
				if(StripIdx != -1)
				{
					APIRecord OriginalTemplateRecord;
					if(TemplateRecords.TryGetValue(FullName.Substring(0, StripIdx), out OriginalTemplateRecord))
					{
						OriginalTemplateRecord.TemplateSpecializations.Add(this);
					}
				}
			}
		}

		public override void PostLink()
		{
			if (BaseRecords.Count > 0 || DerivedRecords.Count > 0)
			{
				// Create the full hierarchy
				APIHierarchyNode RootNode = new APIHierarchyNode("root", false);
				APIHierarchyNode NextNode = RootNode.AddRoot(this);
				NextNode.AddChildren(this);

				// If it's too big for displaying inline, create a separate page and link to it
				if (!Program.Settings.FindValueOrDefault("HierarchyPages.Classes", "").Split('\n').Contains(FullName))
				{
					HierarchyPage = null;
					HierarchyNode = RootNode;
				}
				else
				{
					NextNode.LinkPath = LinkPath;

					HierarchyPage = new APIHierarchy(this, Name + " Hierarchy", "Class hierarchy for " + Name, RootNode);
					HierarchyNode = new APIHierarchyNode("root", false);

					APIHierarchyNode LastNode = HierarchyNode.AddRoot(this);
					string DerivedMessage = String.Format("&#43; {0} derived classes", NextNode.CountChildren());
					LastNode.Children.Add(new APIHierarchyNode(DerivedMessage, HierarchyPage.LinkPath, true));
				}
			}
		}

		public override void GatherReferencedPages(List<APIPage> Pages)
		{
			base.GatherReferencedPages(Pages);
			if(HierarchyPage != null) Pages.Add(HierarchyPage);
		}

		private static List<KeyValuePair<XmlNode, APIRecord>> ParseRelatives(XmlNode ParentNode, string XPath)
		{
			List<KeyValuePair<XmlNode, APIRecord>> Records = new List<KeyValuePair<XmlNode,APIRecord>>();
			using (XmlNodeList NodeList = ParentNode.SelectNodes(XPath))
			{
				foreach (XmlNode Node in NodeList)
				{
					XmlAttribute RefAttribute = Node.Attributes["refid"];
					APIMember Member = (RefAttribute != null)? ResolveRefLink(RefAttribute.Value) : null;
					Records.Add(new KeyValuePair<XmlNode, APIRecord>(Node, Member as APIRecord));
				}
			}
			return Records;
		}

		private static void WriteRelatedClass(UdnWriter Writer, KeyValuePair<XmlNode, APIRecord> Pair)
		{
			if (Pair.Value == null)
			{
				Writer.WriteLine("{0}", Pair.Key.InnerText);
			}
			else
			{
				Writer.WriteLine("[{0}]({1})  ", Pair.Key.InnerText, Pair.Value.LinkPath);
			}
		}

		private static void WriteRelatedClasses(UdnWriter Writer, string Indent, List<KeyValuePair<XmlNode, APIRecord>> Records)
		{
			foreach (KeyValuePair<XmlNode, APIRecord> Record in Records)
			{
				if (Record.Value == null)
				{
					Writer.WriteLine("{0} {1}  ", Indent, Record.Key.InnerText);
				}
				else
				{
					Writer.WriteLine("{0} [{1}]({2})  ", Indent, Record.Key.InnerText, Record.Value.LinkPath);
				}
			}
		}

		private void WriteDefinition(UdnWriter Writer)
		{
			List<string> Lines = new List<string>();

			if (MetadataDirective != null)
			{
				Lines.Add(MetadataDirective.ToMarkdown());
			}
			if (TemplateSignature != null)
			{
				Lines.Add(TemplateSignature);
			}
			if(BaseDefinitions.Count == 0)
			{
				Lines.Add(Definition);
			}
			else if(BaseDefinitions.Count == 1)
			{
				Lines.Add(Definition + " : " + BaseDefinitions[0]);
			}
			else
			{
				Lines.Add(Definition + " :");
				for(int Idx = 0; Idx < BaseDefinitions.Count; Idx++)
				{
					Lines.Add(UdnWriter.TabSpaces + BaseDefinitions[Idx] + ((Idx < BaseDefinitions.Count - 1)? "," : ""));
				}
			}

			WriteNestedSimpleCode(Writer, Lines);
		}

		protected bool HasAnyPrivateFunction(string Name)
		{
			using (XmlNodeList FunctionNodeList = Node.SelectNodes("sectiondef/memberdef"))
			{
				foreach (XmlNode FunctionNode in FunctionNodeList)
				{
					if (FunctionNode.Attributes["kind"].Value == "function" && FunctionNode.Attributes["prot"].Value == "private")
					{
						XmlNode NameNode = FunctionNode.SelectSingleNode("name");
						if (NameNode != null && NameNode.InnerText == Name)
						{
							return true;
						}
					}
				}
			}
			return false;
		}

		public override void WritePage(UdnManifest Manifest, string OutputPath)
        {
			using (UdnWriter Writer = new UdnWriter(OutputPath))
			{
				Writer.WritePageHeader(Name, PageCrumbs, BriefDescription);
				Writer.EnterTag("[OBJECT:Class]");

				// Write the brief description
				Writer.WriteLine("[PARAM:briefdesc]");
				if (!Utility.IsNullOrWhitespace(BriefDescription))
				{
					Writer.WriteLine(BriefDescription);
				}
				Writer.WriteLine("[/PARAM]");

				// Write the hierarchy
				Writer.EnterTag("[PARAM:hierarchy]");
				if (HierarchyNode != null)
				{
					Writer.EnterSection("hierarchy", "Inheritance Hierarchy");
					Writer.EnterTag("[REGION:hierarchy]");
					APIHierarchy.WriteHierarchy(Writer, HierarchyNode, "hrch");
					Writer.LeaveTag("[/REGION]");
					Writer.LeaveSection();
				}
				Writer.LeaveTag("[/PARAM]");

				// Write the record definition
				Writer.EnterTag("[PARAM:syntax]");
				if (!Utility.IsNullOrWhitespace(Definition))
				{
					Writer.EnterSection("syntax", "Syntax");
					WriteDefinition(Writer);
					Writer.LeaveSection();
				}
				Writer.LeaveTag("[/PARAM]");

				// Write the metadata section
				Writer.WriteLine("[PARAM:meta]");
				if (MetadataDirective != null)
				{
					MetadataDirective.WriteListSection(Writer, "metadata", "Metadata", MetadataLookup.ClassTags);
				}
				Writer.WriteLine("[/PARAM]");

				// Build a list of all the functions
				List<APIFunction> AllFunctions = new List<APIFunction>();
				AllFunctions.AddRange(Children.OfType<APIFunction>().Where(x => x.Protection != APIProtection.Private));
				AllFunctions.AddRange(Children.OfType<APIFunctionGroup>().SelectMany(x => x.Children.OfType<APIFunction>()).Where(x => x.Protection != APIProtection.Private));
				AllFunctions.Sort((x, y) => String.Compare(x.Name, y.Name));

				// Write all the specializations
				Writer.EnterTag("[PARAM:specializations]");
				if (TemplateSpecializations.Count > 0)
				{
					Writer.EnterSection("specializations", "Specializations");
					foreach (APIRecord Specialization in TemplateSpecializations)
					{
						Writer.WriteLine("[{0}]({1})  ", Specialization.Name, Specialization.LinkPath);
					}
					Writer.LeaveSection();
				}
				Writer.LeaveTag("[/PARAM]");

				// Write all the typedefs
				Writer.EnterTag("[PARAM:typedefs]");
				Writer.WriteListSection("typedefs", "Typedefs", "Name", "Description", Children.OfType<APITypeDef>().Select(x => x.GetListItem()));
				Writer.LeaveTag("[/PARAM]");

				// Write all the constructors
				Writer.EnterTag("[PARAM:constructors]");
				if (!APIFunction.WriteListSection(Writer, "constructor", "Constructors", AllFunctions.Where(x => x.FunctionType == APIFunctionType.Constructor).OrderBy(x => x.LinkPath)) && HasAnyPrivateFunction(Name))
				{
					Writer.EnterSection("constructor", "Constructors");
					Writer.WriteLine("No constructors are accessible with public or protected access.");
					Writer.LeaveSection();
				}
				Writer.LeaveTag("[/PARAM]");

				// Write all the destructors
				Writer.EnterTag("[PARAM:destructors]");
				if (!APIFunction.WriteListSection(Writer, "destructor", "Destructors", AllFunctions.Where(x => x.FunctionType == APIFunctionType.Destructor)) && HasAnyPrivateFunction("~" + Name))
				{
					Writer.EnterSection("destructors", "Destructors");
					Writer.WriteLine("No destructors are accessible with public or protected access.");
					Writer.LeaveSection();
				}
				Writer.LeaveTag("[/PARAM]");

				// Write all the enums
				Writer.EnterTag("[PARAM:enums]");
				Writer.WriteListSection("enums", "Enums", "Name", "Description", Children.OfType<APIEnum>().OrderBy(x => x.Name).Select(x => x.GetListItem()));
				Writer.LeaveTag("[/PARAM]");

				// Write all the inner structures
				Writer.EnterTag("[PARAM:classes]");
				Writer.WriteListSection("classes", "Classes", "Name", "Description", Children.OfType<APIRecord>().Where(x => x.Protection != APIProtection.Private).OrderBy(x => x.Name).Select(x => x.GetListItem()));
				Writer.LeaveTag("[/PARAM]");

				// Write all the constants
				Writer.EnterTag("[PARAM:constants]");
				Writer.WriteListSection("constants", "Constants", "Name", "Description", Children.OfType<APIConstant>().Select(x => x.GetListItem()));
				Writer.LeaveTag("[/PARAM]");

				// Write all the variables
				Writer.EnterTag("[PARAM:variables]");
				Writer.WriteListSection("variables", "Variables", "Name", "Description", Children.OfType<APIVariable>().Where(x => x.Protection != APIProtection.Private).OrderBy(x => x.Name).Select(x => x.GetListItem()));
				Writer.LeaveTag("[/PARAM]");

				// Write the functions
				Writer.EnterTag("[PARAM:methods]");
				APIFunction.WriteListSection(Writer, "methods", "Methods", AllFunctions.Where(x => x.FunctionType == APIFunctionType.Normal));
				Writer.LeaveTag("[/PARAM]");

				// Write the operator list
				Writer.EnterTag("[PARAM:operators]");
				APIFunction.WriteListSection(Writer, "operators", "Operators", AllFunctions.Where(x => x.FunctionType == APIFunctionType.UnaryOperator || x.FunctionType == APIFunctionType.BinaryOperator));
				Writer.LeaveTag("[/PARAM]");

				// Write class description
				Writer.EnterTag("[PARAM:description]");
				if (!Utility.IsNullOrWhitespace(FullDescription) && FullDescription != BriefDescription)
				{
					Writer.EnterSection("description", "Remarks");
					Writer.WriteLine(FullDescription);
					Writer.LeaveSection();
				}
				Writer.LeaveTag("[/PARAM]");

				// Write the marshalling parameters
				Writer.EnterTag("[PARAM:marshalling]");
				if (DelegateEventParameters != null)
				{
					Writer.EnterSection("marshalling", "Marshalling");
					Writer.WriteLine("Parameters are marshalled using [{0}]({1})", DelegateEventParameters.FullName, DelegateEventParameters.LinkPath);
					Writer.LeaveSection();
				}
				Writer.LeaveTag("[/PARAM]");

				// Write the declaration file
				Writer.EnterTag("[PARAM:declaration]");
				if (!Utility.IsNullOrWhitespace(DeclarationFile))
				{
					Writer.EnterSection("declaration", "Declaration");
					Writer.WriteEscapedLine(DeclarationFile);
					Writer.LeaveSection();
				}
				Writer.LeaveTag("[/PARAM]");

				Writer.LeaveTag("[/OBJECT]");
			}
        }

		public UdnIconListItem GetListItem()
		{
			List<Icon> Icons = new List<Icon>();
			if (RecordType == APIRecordType.Class || RecordType == APIRecordType.Interface)
			{
				Icons.Add(APIDocTool.Icons.Class[(int)Protection]);
				if (MetadataDirective != null)
				{
					Icons.Add(APIDocTool.Icons.ReflectedClass);
					Icons.AddRange(MetadataDirective.Icons);
				}
			}
			else
			{
				Icons.Add(APIDocTool.Icons.Struct[(int)Protection]);
				if (MetadataDirective != null)
				{
					Icons.Add(APIDocTool.Icons.ReflectedStruct);
					Icons.AddRange(MetadataDirective.Icons);
				}
			}
			return new UdnIconListItem(Icons, Name, BriefDescription, LinkPath);
		}
	}
}

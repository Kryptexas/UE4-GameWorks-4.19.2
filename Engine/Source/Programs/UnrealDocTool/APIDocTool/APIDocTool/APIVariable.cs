// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.IO;

namespace APIDocTool
{
    public class APIVariable : APIMember
    {
		public readonly XmlNode Node;
		public readonly APIProtection Protection;

        public Boolean Mutable;
        public String Bitfield = "";

		public string BriefDescription = "";
		public string FullDescription = "";

		public MetadataDirective MetadataDirective;

		public string Definition = "";

		public string Type = "";

		public bool IsMutable = false;
		public bool IsStatic = false;

		public List<string> Warnings = new List<string>();

		public APIVariable(APIPage InParent, XmlNode InNode)
			: base(InParent, InNode.SelectSingleNode("name").InnerText)
		{
			Node = InNode;
			Protection = ParseProtection(Node);
			AddRefLink(Node.Attributes["id"].InnerText, this);
		}

		public bool IsDeprecated()
		{
			return Name.EndsWith("_DEPRECATED");
		}

		public override void Link()
		{
			// Parse the metadata
			MetadataDirective = MetadataCache.FindMetadataForMember(Node, "UPROPERTY");

			ParseBriefAndFullDescription(Node, out BriefDescription, out FullDescription);

			GetModifiers();

			XmlNode DefNode = Node.SelectSingleNode("definition");
			if (DefNode != null)
			{
				Definition = ConvertToMarkdown(DefNode);
			}

			XmlNode type = Node.SelectSingleNode("type");
			Type = ConvertToMarkdown(type);

			XmlNodeList SimpleNodes = Node.SelectNodes("detaileddescription/para/simplesect");
			foreach (XmlNode node in SimpleNodes)
			{
				switch (node.Attributes.GetNamedItem("kind").InnerText)
				{
					case "warning":
						Warnings.Add(ConvertToMarkdown(node.SelectSingleNode("para")).TrimStart(':'));
						break;
				}
			}
		}

        public void GetModifiers()
        {
            XmlNode BitfieldNode = Node.SelectSingleNode("bitfield");
            if (BitfieldNode != null)
            {
                Bitfield = BitfieldNode.InnerText;
            }
			IsMutable = Node.Attributes.GetNamedItem("mutable").InnerText == "yes";
			IsStatic = Node.Attributes.GetNamedItem("static").InnerText == "yes";
        }

		private void WriteDefinition(UdnWriter Writer)
		{
			List<string> Lines = new List<string>();

			if (MetadataDirective != null)
			{
				Lines.AddRange(MetadataDirective.ToMarkdown());
			}

			StringBuilder Definition = new StringBuilder();
			if (IsStatic) Definition.Append("static ");
			if (IsMutable) Definition.Append("mutable ");
			Definition.Append(Type + " " + Name);
			if (Bitfield != "") Definition.Append(": " + Bitfield);
			Definition.Append("  ");
			Lines.Add(Definition.ToString());

			WriteNestedSimpleCode(Writer, Lines);
		}

		public override void WritePage(UdnManifest Manifest, string OutputPath)
        {
			using (UdnWriter Writer = new UdnWriter(OutputPath))
			{
				Writer.WritePageHeader(Name, PageCrumbs, BriefDescription);

				// Write the warnings
				if (Warnings.Count > 0)
				{
					Writer.EnterTag("[REGION:warning]");
					Writer.WriteLine("**Warnings**");
					foreach (string Warning in Warnings)
					{
						Writer.WriteLine("* " + Warning);
					}
					Writer.LeaveTag("[/REGION]");
				}

				// Write the syntax
				Writer.EnterSection("syntax", "Syntax");
				WriteDefinition(Writer);
				Writer.LeaveSection();

				// Write the description
				if (!Utility.IsNullOrWhitespace(BriefDescription) || !Utility.IsNullOrWhitespace(FullDescription))
				{
					Writer.EnterSection("description", "Remarks");
					if (!Utility.IsNullOrWhitespace(BriefDescription) && BriefDescription != FullDescription)
					{
						Writer.WriteLine(BriefDescription);
					}
					if (!Utility.IsNullOrWhitespace(FullDescription))
					{
						Writer.WriteLine(FullDescription);
					}
					Writer.LeaveSection();
				}
			}
        }

		public UdnIconListItem GetListItem()
		{
			List<Icon> Icons = new List<Icon>();
			Icons.Add(APIDocTool.Icons.Variable[(int)Protection]);
			if (IsStatic)
			{
				Icons.Add(APIDocTool.Icons.StaticVariable);
			}
			if (MetadataDirective != null)
			{
				Icons.Add(APIDocTool.Icons.ReflectedVariable);
				Icons.AddRange(MetadataDirective.Icons);
			}
			return new UdnIconListItem(Icons, Name, BriefDescription, LinkPath);
		}
	}
}

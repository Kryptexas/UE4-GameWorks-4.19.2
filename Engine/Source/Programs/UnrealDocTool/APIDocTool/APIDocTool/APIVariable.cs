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

		public override void WritePage(UdnManifest Manifest, string OutputPath)
        {
			using (UdnWriter Writer = new UdnWriter(OutputPath))
			{
				Writer.WritePageHeader(Name, PageCrumbs, BriefDescription);

				Writer.EnterTag("[OBJECT:Variable]");

				// Write the brief description
				Writer.EnterTag("[PARAM:briefdesc]");
				if (!Utility.IsNullOrWhitespace(BriefDescription) && BriefDescription != FullDescription)
				{
					Writer.WriteLine(BriefDescription);
				}
				Writer.LeaveTag("[/PARAM]");

				// Write the warnings
				Writer.EnterTag("[PARAM:warnings]");
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
				Writer.LeaveTag("[/PARAM]");

				// Write the syntax
				Writer.EnterTag("[PARAM:syntax]");
				Writer.EnterSection("syntax", "Syntax");
				Writer.EnterTag("[REGION:simplecode_api]");
				if (MetadataDirective != null) Writer.WriteLine(MetadataDirective.ToMarkdown() + "  ");
				if (IsStatic) Writer.Write("static ");
				if (IsMutable) Writer.Write("mutable ");
				Writer.Write(Type + " " + Name);
				if (Bitfield != "") Writer.Write(": " + Bitfield);
				Writer.WriteLine("  ");
				Writer.LeaveTag("[/REGION]");
				Writer.LeaveSection();
				Writer.LeaveTag("[/PARAM]");

				// Write the metadata
				Writer.EnterTag("[PARAM:meta]");
				if (MetadataDirective != null)
				{
					MetadataDirective.WriteListSection(Writer, "metadata", "Metadata", MetadataLookup.PropertyTags);
				}
				Writer.LeaveTag("[/PARAM]");

				// Write the description
				Writer.EnterTag("[PARAM:description]");
				if (!Utility.IsNullOrWhitespace(FullDescription))
				{
					Writer.EnterSection("description", "Remarks");
					Writer.WriteLine(FullDescription);
					Writer.LeaveSection();
				}
				Writer.LeaveTag("[/PARAM]");

				// Leave the object tag
				Writer.LeaveTag("[/OBJECT]");
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

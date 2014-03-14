// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.IO;
using DoxygenLib;

namespace APIDocTool
{
	public class APIEnum : APIMember
    {
		public readonly XmlNode Node;
		public APIProtection Protection;

		public MetadataDirective MetadataDirective;

		public string BriefDescription = "";
		public string FullDescription = "";

		public string Definition = "";

		public List<APIEnumValue> Values = new List<APIEnumValue>();

		public APIEnum(APIPage InParent, DoxygenEntity InEntity, string InName)
			: this(InParent, InEntity.Node, InName)
		{
		}

		public APIEnum(APIPage InParent, XmlNode InNode, string InName)
			: base(InParent, InName)
        {
			// Set the defaults
			Node = InNode;
			Protection = ParseProtection(Node);

			// Read the values
			using(XmlNodeList ValueNodeList = Node.SelectNodes("enumvalue"))
			{
				foreach(XmlNode ValueNode in ValueNodeList)
				{
					APIEnumValue Value = new APIEnumValue(ValueNode);
					AddRefLink(Value.Id, this);
					Values.Add(Value);
				}
			}

			// Add it as a link target
			AddRefLink(Node.Attributes["id"].InnerText, this);
        }

		public override void Link()
        {
			// Read the metadata
			MetadataDirective = MetadataCache.FindMetadataForMember(Node);

			// Get the description
			ParseBriefAndFullDescription(Node, out BriefDescription, out FullDescription);

			// Link all the values
			foreach (APIEnumValue Value in Values)
			{
				Value.Link();
			}
        }

		private void WriteDefinition(UdnWriter Writer)
		{
			List<string> Lines = new List<string>();

			Lines.Add(String.Format("enum {0}", ShortName.StartsWith("@")? "" : ShortName));
			Lines.Add(Utility.EscapeText("{"));
			if (Values.Count > 0)
			{
				int NamePadding = Values.Max(x => x.Name.Length) + 4;
				foreach (APIEnumValue Value in Values)
				{
					string ValueLine = UdnWriter.TabSpaces + Value.Name;
					if (Value.Initializer != null)
					{
						for (int Idx = 0; Idx < NamePadding - Value.Name.Length; Idx++) ValueLine += UdnWriter.Space;
						ValueLine += Value.Initializer;
					}
					Lines.Add(ValueLine + ",");
				}
			}
			Lines.Add(Utility.EscapeText("}"));

			WriteNestedSimpleCode(Writer, Lines);
		}

		public override void WritePage(UdnManifest Manifest, string OutputPath)
		{
			using (UdnWriter Writer = new UdnWriter(OutputPath))
			{
				Writer.WritePageHeader(AnnotateAnonymousNames(Name), PageCrumbs, BriefDescription);

				Writer.EnterTag("[OBJECT:Enum]");

				Writer.EnterTag("[PARAM:briefdesc]");
				if (!Utility.IsNullOrWhitespace(BriefDescription))
				{
					Writer.WriteLine(BriefDescription);
				}
				Writer.LeaveTag("[/PARAM]");

				// Write the syntax
				Writer.EnterTag("[PARAM:syntax]");
				Writer.EnterSection("syntax", "Syntax");
				WriteDefinition(Writer);
				Writer.LeaveSection();
				Writer.LeaveTag("[/PARAM]");

				// Write the metadata
				Writer.EnterTag("[PARAM:meta]");
				if (MetadataDirective != null)
				{
					MetadataDirective.WriteListSection(Writer, "metadata", "Metadata", MetadataLookup.EnumTags);
				}
				Writer.LeaveTag("[/PARAM]");

				// Write the enum values
				Writer.EnterTag("[PARAM:values]");
				Writer.WriteListSection("values", "Values", "Name", "Description", Values.Select(x => x.GetListItem()));
				Writer.LeaveTag("[/PARAM]");

				// Write the description
				Writer.EnterTag("[PARAM:description]");
				if (!Utility.IsNullOrWhitespace(FullDescription) && FullDescription != BriefDescription)
				{
					Writer.EnterSection("description", "Remarks");
					Writer.WriteLine(FullDescription);
					Writer.LeaveSection();
				}
				Writer.LeaveTag("[/PARAM]");

				Writer.LeaveTag("[/OBJECT]");
			}
        }

		public UdnIconListItem GetListItem()
		{
			List<Icon> SummaryIcons = new List<Icon>() { Icons.Enum[(int)Protection] };
			if (MetadataDirective != null)
			{
				SummaryIcons.Add(Icons.ReflectedEnum);
				SummaryIcons.AddRange(MetadataDirective.Icons);
			}
			return new UdnIconListItem(SummaryIcons, Name, BriefDescription, LinkPath);
		}
	}
}

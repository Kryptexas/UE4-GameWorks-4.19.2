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
    class APITypeDef : APIMember
    {
		public readonly DoxygenEntity Entity;
		public readonly XmlNode Node;
		public readonly string Id;
		public string Type;
		public string BriefDescription;
		public string FullDescription;

		public APITypeDef(APIPage InParent, DoxygenEntity InEntity) 
			: base(InParent, InEntity.Node.SelectSingleNode("name").InnerText)
        {
			Entity = InEntity;
			Node = Entity.Node;
			Id = Node.Attributes["id"].Value;
			AddRefLink(Id, this);
		}

		public override void Link()
		{
			Type = ConvertToMarkdown(Node.SelectSingleNode("type"));
			ParseBriefAndFullDescription(Node, out BriefDescription, out FullDescription);
			ReplaceMarkdownVariables(ref BriefDescription);
			ReplaceMarkdownVariables(ref FullDescription);
		}

		public UdnListItem GetListItem()
		{
			return new UdnListItem(Name, BriefDescription, LinkPath);
		}

		public void ReplaceMarkdownVariables(ref string Text)
		{
			Text = Text.Replace("&#123;&#123; typedef-type &#125;&#125;", Type);
		}

		public override void WritePage(UdnManifest Manifest, string Path)
		{
			using (UdnWriter Writer = new UdnWriter(Path))
			{
				Writer.WritePageHeader(Name, PageCrumbs, BriefDescription);

				Writer.EnterTag("[OBJECT:Typedef]");

				// Write the brief description
				Writer.EnterTag("[PARAM:briefdesc]");
				if (!Utility.IsNullOrWhitespace(BriefDescription) && BriefDescription != FullDescription)
				{
					Writer.WriteLine(BriefDescription);
				}
				Writer.LeaveTag("[/PARAM]");

				// Write the type
				Writer.EnterTag("[PARAM:type]");
				Writer.EnterSection("type", "Type");
				WriteNestedSimpleCode(Writer, new List<string> { "typedef " + Type + " " + Name });
				Writer.LeaveSection();
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

				// Write the references
				Writer.EnterTag("[PARAM:references]");
				WriteReferencesSection(Writer, Entity);
				Writer.LeaveTag("[/PARAM]");

				// Leave the object tag
				Writer.LeaveTag("[/OBJECT]");
			}
		}
    }
}

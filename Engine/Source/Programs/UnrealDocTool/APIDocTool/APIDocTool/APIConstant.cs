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
	public abstract class APIConstant : APIMember
	{
		public string BriefDescription;
		public string FullDescription;

		public APIConstant(APIPage InParent, string InName)
			: base(InParent, InName)
		{
		}

		protected abstract void WriteDefinition(UdnWriter Writer);

		public override void WritePage(UdnManifest Manifest, string OutputPath)
		{
			using (UdnWriter Writer = new UdnWriter(OutputPath))
			{
				Writer.WritePageHeader(Name, PageCrumbs, BriefDescription);

				Writer.EnterTag("[OBJECT:Constant]");

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

		public UdnListItem GetListItem()
		{
			return new UdnListItem(Name, BriefDescription, LinkPath);
		}
	}

	public class APIConstantEnum : APIConstant
	{
		public readonly XmlNode EnumNode;
		public readonly XmlNode ValueNode;
		public string Initializer;

		public APIConstantEnum(APIPage InParent, XmlNode InEnumNode, XmlNode InValueNode, string InName)
			: base(InParent, InName)
		{
			EnumNode = InEnumNode;
			ValueNode = InValueNode;
		}

		public override void Link()
		{
			// Get the description
			ParseBriefAndFullDescription(EnumNode, out BriefDescription, out FullDescription);
			if (Utility.IsNullOrWhitespace(BriefDescription) && Utility.IsNullOrWhitespace(FullDescription))
			{
				ParseBriefAndFullDescription(ValueNode, out BriefDescription, out FullDescription);
			}

			// Get the initializer
			XmlNode InitializerNode = ValueNode.SelectSingleNode("initializer");
			Initializer = (InitializerNode == null) ? "" : InitializerNode.InnerText;
		}

		public static IEnumerable<APIConstant> Read(APIPage Parent, DoxygenEntity Entity)
		{
			XmlNode EnumNode = Entity.Node;
			using (XmlNodeList ValueNodeList = EnumNode.SelectNodes("enumvalue"))
			{
				foreach (XmlNode ValueNode in ValueNodeList)
				{
					string Name = ValueNode.SelectSingleNode("name").InnerText;
					APIConstant Constant = new APIConstantEnum(Parent, EnumNode, ValueNode, Name);
					yield return Constant;
				}
			}
		}

		protected override void WriteDefinition(UdnWriter Writer)
		{
			List<string> Lines = new List<string>();

			Lines.Add("enum");
			Lines.Add(Utility.EscapeText("{"));
			Lines.Add(UdnWriter.TabSpaces + Name + " " + Initializer);
			Lines.Add(Utility.EscapeText("}"));

			WriteNestedSimpleCode(Writer, Lines);
		}
	}

	public class APIConstantVariable : APIConstant
	{
		public readonly DoxygenEntity Entity;
		public readonly XmlNode Node;

		public string Type;
		public string Initializer;

		public APIConstantVariable(APIPage InParent, DoxygenEntity InEntity)
			: base(InParent, InEntity.Name)
		{
			Entity = InEntity;
			Node = Entity.Node;
		}

		public override void Link()
		{
			// Get the description
			ParseBriefAndFullDescription(Node, out BriefDescription, out FullDescription);

			// Get the type
			XmlNode TypeNode = Node.SelectSingleNode("type");
			Type = ConvertToMarkdown(TypeNode);

			// Get the initializer
			XmlNode InitializerNode = Node.SelectSingleNode("initializer");
			Initializer = (InitializerNode == null) ? null : InitializerNode.InnerText;
		}

		protected override void WriteDefinition(UdnWriter Writer)
		{
			string Definition = "static " + Type + " " + Name;
			if(Initializer != null) Definition += " " + Initializer;
			Definition += ";";
			WriteNestedSimpleCode(Writer, new List<string>{ Definition });
		}
	}
}

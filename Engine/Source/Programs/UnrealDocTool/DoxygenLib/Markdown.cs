// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace DoxygenLib
{
	public static class Markdown
	{
		const bool bDocToolCanParseExternalLinks = false;

		public delegate string ResolveLinkDelegate(string Id);

		/** Parses markdown text out of an XML node 
		 * 
		 * @param child			xml node we want to parse into Markdown 
		 * @param Indent		indenting prefix for each newline of text in the converted markdown
		 * @param ResolveLink	delegate to call to resolve doxygen id's into link paths. may be null.
		 * @return				a string containing the text from the child node 
		 */
		public static string ParseXml(XmlNode child, string Indent, ResolveLinkDelegate ResolveLink)
		{
			string output = "";

			XmlNode parent = child.ParentNode;
			if (child.HasChildNodes)
			{
				switch (child.Name)
				{
					case "para":
						if (parent.Name != "listitem" && parent.Name != "parameterdescription" && parent.Name != "simplesect")
						{
							output += Environment.NewLine;
							output += ParseChildren(child, Indent, ResolveLink);
						}
						else if (parent.Name == "listitem")
						{
							output += Environment.NewLine;
							if (child == parent.FirstChild)
							{
								output += Indent + "* " + ParseChildren(child, Indent, ResolveLink);
							}
							else
							{
								output += Environment.NewLine + Indent + "\t" + ParseChildren(child, Indent, ResolveLink);
							}
						}
						else if (parent.Name == "parameterdescription" || parent.Name == "simplesect")
						{
							output += ParseChildren(child, Indent, ResolveLink);
						}
						break;
					case "ulink":
						if (bDocToolCanParseExternalLinks)
						{
							output += Indent + String.Format("[{0}]({1})", ParseChildren(child, Indent, ResolveLink), child.Attributes.GetNamedItem("url").InnerText);
						}
						else
						{
							output += Indent + EscapeText(child.Attributes.GetNamedItem("url").InnerText);
						}
						break;
					case "ref":
						XmlAttribute RefAttribute = child.Attributes["refid"];
						if (RefAttribute != null && ResolveLink != null)
						{
							string LinkPath = ResolveLink(RefAttribute.Value);
							if(LinkPath != null)
							{
								output += String.Format("[{0}]({1})", ParseChildren(child, Indent, ResolveLink), LinkPath);
								break;
							}
						}
						output += ParseChildren(child, Indent, ResolveLink);
						break;
					case "bold":
						output += Indent + String.Format("**{0}**", ParseChildren(child, Indent, ResolveLink));
						break;
					case "emphasis":
						output += Indent + String.Format("_{0}_", ParseChildren(child, Indent, ResolveLink));
						break;
					case "computeroutput":
						output += Indent + String.Format("`{0}`", ParseChildren(child, Indent, ResolveLink));
						break;
					case "itemizedlist":
						if (parent.ParentNode.Name == "listitem")
						{
							output += ParseChildren(child, Indent + "\t", ResolveLink);
						}
						else
						{
							output += Environment.NewLine + ParseChildren(child, Indent, ResolveLink) + Environment.NewLine + Environment.NewLine;
						}
						break;
					case "parameterlist":
						break;
					case "parameteritem":
						break;
					case "parameternamelist":
						break;
					case "simplesect":
						break;
					case "xrefsect":
						break;
					default:
						output += ParseChildren(child, Indent, ResolveLink);
						break;
				}
			}
			else
			{
				output += Indent + EscapeText(child.InnerText);
			}

			return output;
		}

		/** Class ParseXML for all child nodes of the given child
		 * 
		 * @param child xml node we want to parse all children into Markdown 
		 * @return a string containing the text from the child node 
		 */
		static String ParseChildren(XmlNode child, string Indent, ResolveLinkDelegate ResolveLink)
		{
			String output = "";

			foreach (XmlNode childNode in child.ChildNodes)
			{
				if (output.Length > 0 && output[output.Length - 1] != ' ') output += " ";
				output += ParseXml(childNode, Indent, ResolveLink);
			}

			return output;
		}

		public static string EscapeText(string Text)
		{
			const string MarkdownCharacters = "_[]*{}()%#";

			StringBuilder Result = new StringBuilder();
			for (int Idx = 0; Idx < Text.Length; Idx++)
			{
				if (Text[Idx] == '&')
				{
					Result.Append("&amp;");
				}
				else if (Text[Idx] == '<')
				{
					Result.Append("&lt;");
				}
				else if (Text[Idx] == '>')
				{
					Result.Append("&gt;");
				}
				else if (Text[Idx] == '"')
				{
					Result.Append("&quot;");
				}
				else if (Text[Idx] == '\t')
				{
					Result.Append("    ");
				}
				else if (Text[Idx] < 0x20 || Text[Idx] > 0x7e || MarkdownCharacters.Contains(Text[Idx]))
				{
					Result.AppendFormat("&#{0:00};", (int)Text[Idx]);
				}
				else
				{
					Result.Append(Text[Idx]);
				}
			}
			return Result.ToString();
		}
	}
}

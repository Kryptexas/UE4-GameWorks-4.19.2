using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace APIDocTool
{
	class APIActionPin : APIPage
	{
		public readonly string Tooltip;
		public readonly string TypeText;

		public readonly string PinCategory;
		public readonly string PinSubCategory;
		public readonly string PinSubCategoryObject;

		public readonly bool bInputPin;
		public readonly bool bIsConst;
		public readonly bool bIsReference;
		public readonly bool bIsArray;

		public static string GetPinName(Dictionary<string, object> PinProperties)
		{
			object Value;
			string Name;

			if (PinProperties.TryGetValue("Name", out Value))
			{
				Name = (string)Value;
			}
			else
			{
				Debug.Assert((string)PinProperties["PinCategory"] == "exec");
				
				Name = ((string)PinProperties["Direction"] == "input" ? "In" : "Out");
			}

			return Name;
		}

		public APIActionPin(APIPage InParent, string InName, Dictionary<string, object> PinProperties)
			: base(InParent, InName)
		{
			object Value;

			Debug.Assert(InName == GetPinName(PinProperties));

			bInputPin = (string)PinProperties["Direction"] == "input";
			PinCategory = (string)PinProperties["PinCategory"];
			TypeText = (string)PinProperties["TypeText"];

			if (PinProperties.TryGetValue("Tooltip", out Value))
			{
				Tooltip = String.Concat(((string)Value).Split('\n').Select(Line => Line.Trim() + '\n'));
//				Tooltip = new string(Array.FindAll<char>(((string)Value).ToCharArray(), c => c != '\t'));
			}
			else
			{
				Tooltip = "";
			}

			if (PinProperties.TryGetValue("PinSubCategory", out Value))
			{
				PinSubCategory = (string)Value;
			}
			else
			{
				PinSubCategory = "";
			}

			if (PinProperties.TryGetValue("PinSubCategoryObject", out Value))
			{
				PinSubCategoryObject = (string)Value;
			}
			else
			{
				PinSubCategoryObject = "";
			}

			if (PinProperties.TryGetValue("IsConst", out Value))
			{
				bIsConst = Convert.ToBoolean((string)Value);
			}

			if (PinProperties.TryGetValue("IsReference", out Value))
			{
				bIsReference = Convert.ToBoolean((string)Value);
			}

			if (PinProperties.TryGetValue("IsArray", out Value))
			{
				bIsArray = Convert.ToBoolean((string)Value);
			}
		}

		private string GetPinCategory()
		{
			if (PinCategory == "struct")
			{
				if (PinSubCategoryObject == "Vector")
				{
					return "vector";
				}
				else if (PinSubCategoryObject == "Transform")
				{
					return "transform";
				}
				else if (PinSubCategoryObject == "Rotator")
				{
					return "rotator";
				}
			}

			return PinCategory;
		}

		public override void WritePage(UdnManifest Manifest, string OutputPath)
		{
			using (UdnWriter Writer = new UdnWriter(OutputPath))
			{
				Writer.WritePageHeader(Name, PageCrumbs, Name);
			}
		}

		public override void GatherReferencedPages(List<APIPage> Pages)
		{

		}

		public override void AddToManifest(UdnManifest Manifest)
		{
			Manifest.Add(Name, this);
		}

		public void WritePin(UdnWriter Writer)
		{
			// Get all the icons
			List<Icon> ItemIcons = new List<Icon>();
			if (bIsArray)
			{
				ItemIcons.Add(Icons.ArrayVariablePinIcons[GetPinCategory()]);
			}
			else
			{
				ItemIcons.Add(Icons.VariablePinIcons[GetPinCategory()]);
			}

			Writer.WriteObject("ActionPinListItem", "icons", ItemIcons, "name", Name, "type", TypeText, "tooltip", Tooltip);
		}
	}
}

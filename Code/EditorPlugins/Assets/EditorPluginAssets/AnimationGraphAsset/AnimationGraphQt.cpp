#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphQt.h>

plQtAnimationGraphNode::plQtAnimationGraphNode() = default;

void plQtAnimationGraphNode::UpdateState()
{
  plQtNode::UpdateState();

  plStringBuilder sTitle;

  const plRTTI* pRtti = GetObject()->GetType();

  plVariant customTitle = GetObject()->GetTypeAccessor().GetValue("CustomTitle");
  if (customTitle.IsValid() && customTitle.CanConvertTo<plString>())
  {
    sTitle = customTitle.ConvertTo<plString>();
  }

  if (sTitle.IsEmpty())
  {
    if (const plTitleAttribute* pAttr = pRtti->GetAttributeByType<plTitleAttribute>())
    {
      sTitle = pAttr->GetTitle();

      plStringBuilder tmp, tmp2;
      plVariant val;

      // replace enum properties with translated strings
      {
        plHybridArray<const plAbstractProperty*, 32> properties;
        pRtti->GetAllProperties(properties);

        for (const auto& prop : properties)
        {
          if (prop->GetSpecificType()->IsDerivedFrom<plEnumBase>() || prop->GetSpecificType()->IsDerivedFrom<plBitflagsBase>())
          {
            val = GetObject()->GetTypeAccessor().GetValue(prop->GetPropertyName());

            plReflectionUtils::EnumerationToString(prop->GetSpecificType(), val.ConvertTo<plInt64>(), tmp);

            tmp2.Set("{", prop->GetPropertyName(), "}");
            sTitle.ReplaceAll(tmp2, plTranslate(tmp));
          }
        }
      }

      // replace the rest
      while (true)
      {
        const char* szOpen = sTitle.FindSubString("{");

        if (szOpen == nullptr)
          break;

        const char* szClose = sTitle.FindSubString("}", szOpen);

        if (szClose == nullptr)
          break;

        tmp.SetSubString_FromTo(szOpen + 1, szClose);

        // three array indices should be enough for everyone
        if (tmp.TrimWordEnd("[0]"))
          val = GetObject()->GetTypeAccessor().GetValue(tmp, 0);
        else if (tmp.TrimWordEnd("[1]"))
          val = GetObject()->GetTypeAccessor().GetValue(tmp, 1);
        else if (tmp.TrimWordEnd("[2]"))
          val = GetObject()->GetTypeAccessor().GetValue(tmp, 2);
        else
          val = GetObject()->GetTypeAccessor().GetValue(tmp);

        if (val.IsValid())
        {

          tmp.Format("{}", val);

          if (plConversionUtils::IsStringUuid(tmp))
          {
            if (auto pAsset = plAssetCurator::GetSingleton()->FindSubAsset(tmp))
            {
              tmp = pAsset->GetName();
            }
          }

          sTitle.ReplaceSubString(szOpen, szClose + 1, tmp);
        }
        else
        {
          sTitle.ReplaceSubString(szOpen, szClose + 1, "");
        }
      }
    }
  }

  sTitle.ReplaceAll("''", "");
  sTitle.ReplaceAll("\"\"", "");
  sTitle.ReplaceAll("  ", " ");
  sTitle.Trim(" ");

  if (sTitle.GetCharacterCount() > 30)
  {
    sTitle.Shrink(0, sTitle.GetCharacterCount() - 31);
    sTitle.Append("...");
  }

  if (!sTitle.IsEmpty())
  {
    m_pTitleLabel->setPlainText(sTitle.GetData());
  }
}

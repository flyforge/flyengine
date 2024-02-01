#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/Implementation/Helper.h>

namespace plShaderHelper
{

  void plTextSectionizer::Clear()
  {
    m_Sections.Clear();
    m_sText.Clear();
  }

  void plTextSectionizer::AddSection(const char* szName) { m_Sections.PushBack(plTextSection(szName)); }

  void plTextSectionizer::Process(const char* szText)
  {
    for (plUInt32 i = 0; i < m_Sections.GetCount(); ++i)
      m_Sections[i].Reset();

    m_sText = szText;


    for (plUInt32 s = 0; s < m_Sections.GetCount(); ++s)
    {
      m_Sections[s].m_szSectionStart = m_sText.FindSubString_NoCase(m_Sections[s].m_sName.GetData());

      if (m_Sections[s].m_szSectionStart != nullptr)
        m_Sections[s].m_Content = plStringView(m_Sections[s].m_szSectionStart + m_Sections[s].m_sName.GetElementCount());
    }

    for (plUInt32 s = 0; s < m_Sections.GetCount(); ++s)
    {
      if (m_Sections[s].m_szSectionStart == nullptr)
        continue;

      plUInt32 uiLine = 1;

      const char* sz = m_sText.GetData();
      while (sz < m_Sections[s].m_szSectionStart)
      {
        if (*sz == '\n')
          ++uiLine;

        ++sz;
      }

      m_Sections[s].m_uiFirstLine = uiLine;

      for (plUInt32 s2 = 0; s2 < m_Sections.GetCount(); ++s2)
      {
        if (s == s2)
          continue;

        if (m_Sections[s2].m_szSectionStart > m_Sections[s].m_szSectionStart)
        {
          const char* szContentStart = m_Sections[s].m_Content.GetStartPointer();
          const char* szSectionEnd = plMath::Min(m_Sections[s].m_Content.GetEndPointer(), m_Sections[s2].m_szSectionStart);

          m_Sections[s].m_Content = plStringView(szContentStart, szSectionEnd);
        }
      }
    }
  }

  plStringView plTextSectionizer::GetSectionContent(plUInt32 uiSection, plUInt32& out_uiFirstLine) const
  {
    out_uiFirstLine = m_Sections[uiSection].m_uiFirstLine;
    return m_Sections[uiSection].m_Content;
  }

  void GetShaderSections(const char* szContent, plTextSectionizer& out_sections)
  {
    out_sections.Clear();

    out_sections.AddSection("[PLATFORMS]");
    out_sections.AddSection("[PERMUTATIONS]");
    out_sections.AddSection("[MATERIALPARAMETER]");
    out_sections.AddSection("[MATERIALCONFIG]");
    out_sections.AddSection("[RENDERSTATE]");
    out_sections.AddSection("[SHADER]");
    out_sections.AddSection("[VERTEXSHADER]");
    out_sections.AddSection("[HULLSHADER]");
    out_sections.AddSection("[DOMAINSHADER]");
    out_sections.AddSection("[GEOMETRYSHADER]");
    out_sections.AddSection("[PIXELSHADER]");
    out_sections.AddSection("[COMPUTESHADER]");
    out_sections.AddSection("[TEMPLATE_VARS]");

    out_sections.Process(szContent);
  }

  plUInt32 CalculateHash(const plArrayPtr<plPermutationVar>& vars)
  {
    plHybridArray<plUInt64, 128> buffer;
    buffer.SetCountUninitialized(vars.GetCount() * 2);

    for (plUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      auto& var = vars[i];
      buffer[i * 2 + 0] = var.m_sName.GetHash();
      buffer[i * 2 + 1] = var.m_sValue.GetHash();
    }

    auto bytes = buffer.GetByteArrayPtr();
    return plHashingUtils::xxHash32(bytes.GetPtr(), bytes.GetCount());
  }
} // namespace plShaderHelper



#pragma once

#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>

namespace plShaderHelper
{
  class PL_RENDERERCORE_DLL plTextSectionizer
  {
  public:
    void Clear();

    void AddSection(const char* szName);

    void Process(const char* szText);

    plStringView GetSectionContent(plUInt32 uiSection, plUInt32& out_uiFirstLine) const;

  private:
    struct plTextSection
    {
      plTextSection(const char* szName)
        : m_sName(szName)

      {
      }

      void Reset()
      {
        m_szSectionStart = nullptr;
        m_Content = plStringView();
        m_uiFirstLine = 0;
      }

      plString m_sName;
      const char* m_szSectionStart = nullptr;
      plStringView m_Content;
      plUInt32 m_uiFirstLine = 0;
    };

    plStringBuilder m_sText;
    plHybridArray<plTextSection, 16> m_Sections;
  };

  struct plShaderSections
  {
    enum Enum
    {
      PLATFORMS,
      PERMUTATIONS,
      MATERIALPARAMETER,
      MATERIALCONFIG,
      RENDERSTATE,
      SHADER,
      VERTEXSHADER,
      HULLSHADER,
      DOMAINSHADER,
      GEOMETRYSHADER,
      PIXELSHADER,
      COMPUTESHADER,
      TEMPLATE_VARS
    };
  };

  PL_RENDERERCORE_DLL void GetShaderSections(const char* szContent, plTextSectionizer& out_sections);

  plUInt32 CalculateHash(const plArrayPtr<plPermutationVar>& vars);
} // namespace plShaderHelper

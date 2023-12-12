
#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/AdobeCUBEReader.h>
#include <Foundation/CodeUtils/Tokenizer.h>

// This file implements a simple reader for the Adobe CUBE LUT file format.
// The specification can be found here (at the time of this writing):
// https://wwwimages2.adobe.com/content/dam/acom/en/products/speedgrade/cc/pdfs/cube-lut-specification-1.0.pdf

namespace
{
  bool GetVec3FromLine(plHybridArray<const plToken*, 32> line, plUInt32 skip, plVec3& out)
  {
    if (line.GetCount() < (skip + 3 + 2))
    {
      return false;
    }

    if ((line[skip + 0]->m_iType != plTokenType::Float && line[skip + 0]->m_iType != plTokenType::Integer) ||
        line[skip + 1]->m_iType != plTokenType::Whitespace ||
        (line[skip + 2]->m_iType != plTokenType::Float && line[skip + 2]->m_iType != plTokenType::Integer) ||
        line[skip + 3]->m_iType != plTokenType::Whitespace ||
        (line[skip + 4]->m_iType != plTokenType::Float && line[skip + 4]->m_iType != plTokenType::Integer))
    {
      return false;
    }

    double res = 0;
    plString sVal = line[skip + 0]->m_DataView;

    if (plConversionUtils::StringToFloat(sVal, res).Failed())
      return false;

    out.x = static_cast<float>(res);

    sVal = line[skip + 2]->m_DataView;
    if (plConversionUtils::StringToFloat(sVal, res).Failed())
      return false;

    out.y = static_cast<float>(res);


    sVal = line[skip + 4]->m_DataView;
    if (plConversionUtils::StringToFloat(sVal, res).Failed())
      return false;

    out.z = static_cast<float>(res);

    return true;
  }
} // namespace

plAdobeCUBEReader::plAdobeCUBEReader() = default;
plAdobeCUBEReader::~plAdobeCUBEReader() = default;

plStatus plAdobeCUBEReader::ParseFile(plStreamReader& Stream, plLogInterface* pLog /*= nullptr*/)
{
  plString sContent;
  sContent.ReadAll(Stream);

  plTokenizer tokenizer;
  tokenizer.SetTreatHashSignAsLineComment(true);

  tokenizer.Tokenize(
    plArrayPtr<const plUInt8>((const plUInt8*)sContent.GetData(), sContent.GetElementCount()), pLog ? pLog : plLog::GetThreadLocalLogSystem());


  auto tokens = tokenizer.GetTokens();

  plHybridArray<const plToken*, 32> line;
  plUInt32 firstToken = 0;

  while (tokenizer.GetNextLine(firstToken, line).Succeeded())
  {
    if (line[0]->m_iType == plTokenType::LineComment || line[0]->m_iType == plTokenType::Newline)
      continue;

    if (line[0]->m_DataView == "TITLE")
    {
      if (line.GetCount() < 3)
      {
        return plStatus(plFmt("LUT file has invalid TITLE line."));
      }

      if (line[1]->m_iType != plTokenType::Whitespace && line[2]->m_iType != plTokenType::String1)
      {
        return plStatus(plFmt("LUT file has invalid TITLE line, expected TITLE<whitespace>\"<string>\"."));
      }

      m_sTitle = line[2]->m_DataView;

      continue;
    }
    else if (line[0]->m_DataView == "DOMAIN_MIN")
    {
      if (!::GetVec3FromLine(line, 2, m_vDomainMin))
      {
        return plStatus(plFmt("LUT file has invalid DOMAIN_MIN line."));
      }

      continue;
    }
    else if (line[0]->m_DataView == "DOMAIN_MAX")
    {
      if (!::GetVec3FromLine(line, 2, m_vDomainMax))
      {
        return plStatus(plFmt("LUT file has invalid DOMAIN_MAX line."));
      }

      continue;
    }
    else if (line[0]->m_DataView == "LUT_1D_SIZE")
    {
      return plStatus(plFmt("LUT file specifies a 1D LUT which is currently not implemented."));
    }
    else if (line[0]->m_DataView == "LUT_3D_SIZE")
    {
      if (m_uiLUTSize > 0)
      {
        return plStatus(plFmt("LUT file has more than one LUT_3D_SIZE entry. Aborting parse."));
      }

      if (line.GetCount() < 3)
      {
        return plStatus(plFmt("LUT file has invalid LUT_3D_SIZE line."));
      }

      if (line[1]->m_iType != plTokenType::Whitespace && line[2]->m_iType != plTokenType::Integer)
      {
        return plStatus(plFmt("LUT file has invalid LUT_3D_SIZE line, expected LUT_3D_SIZE<whitespace><N>."));
      }

      const plString sVal = line[2]->m_DataView;
      if (plConversionUtils::StringToUInt(sVal, m_uiLUTSize).Failed())
      {
        return plStatus(plFmt("LUT file has invalid LUT_3D_SIZE line, couldn't parse LUT size as plUInt32."));
      }

      if (m_uiLUTSize < 2 || m_uiLUTSize > 256)
      {
        return plStatus(plFmt("LUT file has invalid LUT_3D_SIZE size, got {0} - but must be in range 2, 256.", m_uiLUTSize));
      }

      m_LUTValues.Reserve(m_uiLUTSize * m_uiLUTSize * m_uiLUTSize);

      continue;
    }

    if (line[0]->m_iType == plTokenType::Float || line[0]->m_iType == plTokenType::Integer)
    {
      if (m_uiLUTSize == 0)
      {
        return plStatus(plFmt("LUT data before LUT size was specified."));
      }

      plVec3 lineValues;
      if (!::GetVec3FromLine(line, 0, lineValues))
      {
        return plStatus(plFmt("LUT data couldn't be read."));
      }

      m_LUTValues.PushBack(lineValues);
    }
  }

  if (m_vDomainMin.x > m_vDomainMax.x || m_vDomainMin.y > m_vDomainMax.y || m_vDomainMin.z > m_vDomainMax.z)
  {
    return plStatus("LUT file has invalid domain min/max values.");
  }

  if (m_LUTValues.GetCount() != (m_uiLUTSize * m_uiLUTSize * m_uiLUTSize))
  {
    return plStatus(plFmt("LUT data incomplete, read {0} values but expected {1} values given a LUT size of {2}.", m_LUTValues.GetCount(),
      (m_uiLUTSize * m_uiLUTSize * m_uiLUTSize), m_uiLUTSize));
  }

  return plStatus(PLASMA_SUCCESS);
}

plVec3 plAdobeCUBEReader::GetDomainMin() const
{
  return m_vDomainMin;
}

plVec3 plAdobeCUBEReader::GetDomainMax() const
{
  return m_vDomainMax;
}

plUInt32 plAdobeCUBEReader::GetLUTSize() const
{
  return m_uiLUTSize;
}

const plString& plAdobeCUBEReader::GetTitle() const
{
  return m_sTitle;
}

plVec3 plAdobeCUBEReader::GetLUTEntry(plUInt32 r, plUInt32 g, plUInt32 b) const
{
  return m_LUTValues[GetLUTIndex(r, g, b)];
}

plUInt32 plAdobeCUBEReader::GetLUTIndex(plUInt32 r, plUInt32 g, plUInt32 b) const
{
  return b * m_uiLUTSize * m_uiLUTSize + g * m_uiLUTSize + r;
}

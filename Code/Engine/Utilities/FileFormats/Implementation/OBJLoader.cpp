#include <Utilities/UtilitiesPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Utilities/FileFormats/OBJLoader.h>

plOBJLoader::FaceVertex::FaceVertex()
{
  m_uiPositionID = 0;
  m_uiNormalID = 0;
  m_uiTexCoordID = 0;
}

plOBJLoader::Face::Face()
{
  m_uiMaterialID = 0;
}

void plOBJLoader::Clear()
{
  m_Positions.Clear();
  m_Normals.Clear();
  m_TexCoords.Clear();
  m_Faces.Clear();
  m_Materials.Clear();
}

static plStringView ReadLine(plStringView& ref_sPos)
{
  while (ref_sPos.GetCharacter() != '\0' && plStringUtils::IsWhiteSpace(ref_sPos.GetCharacter()))
    ++ref_sPos;

  const char* szStart = ref_sPos.GetStartPointer();

  while (ref_sPos.GetCharacter() != '\0' && ref_sPos.GetCharacter() != '\r' && ref_sPos.GetCharacter() != '\n')
    ++ref_sPos;

  const char* szEnd = ref_sPos.GetStartPointer();

  while (ref_sPos.GetCharacter() != '\0' && plStringUtils::IsWhiteSpace(ref_sPos.GetCharacter()))
    ++ref_sPos;

  return plStringView(szStart, szEnd);
}

static plStringView ReadString(plStringView& ref_sPos)
{
  while (ref_sPos.GetCharacter() != '\0' && plStringUtils::IsWhiteSpace(ref_sPos.GetCharacter()))
    ++ref_sPos;

  const char* szStart = ref_sPos.GetStartPointer();

  while (ref_sPos.GetCharacter() != '\0' && !plStringUtils::IsWhiteSpace(ref_sPos.GetCharacter()))
    ++ref_sPos;

  const char* szEnd = ref_sPos.GetStartPointer();

  while (ref_sPos.GetCharacter() != '\0' && plStringUtils::IsWhiteSpace(ref_sPos.GetCharacter()))
    ++ref_sPos;

  return plStringView(szStart, szEnd);
}

static bool SkipSlash(plStringView& ref_sPos)
{
  if (ref_sPos.GetCharacter() != '/')
    return false;

  ++ref_sPos;

  return (ref_sPos.GetCharacter() != ' ' && ref_sPos.GetCharacter() != '\t');
}

plResult plOBJLoader::LoadOBJ(const char* szFile, bool bIgnoreMaterials)
{
  plFileReader File;
  if (File.Open(szFile).Failed())
    return PL_FAILURE;

  plString sContent;
  sContent.ReadAll(File);

  // which data has been found in the file
  bool bContainsTexCoords = false;
  bool bContainsNormals = false;

  plUInt32 uiCurMaterial = 0xFFFFFFFF;

  plStringView sText = sContent;

  plUInt32 uiPositionOffset = m_Positions.GetCount();
  plUInt32 uiNormalOffset = m_Normals.GetCount();
  plUInt32 uiTexCoordOffset = m_TexCoords.GetCount();

  while (sText.IsValid())
  {
    plStringView sLine = ReadLine(sText);
    const plStringView sFirst = ReadString(sLine);

    if (sFirst.IsEqual_NoCase("v")) // line declares a vertex
    {
      plVec3 v(0.0f);
      plConversionUtils::ExtractFloatsFromString(sLine.GetStartPointer(), 3, &v.x);

      m_Positions.PushBack(v);
    }
    else if (sFirst.IsEqual_NoCase("vt")) // line declares a texture coordinate
    {
      bContainsTexCoords = true;

      plVec3 v(0.0f);
      plConversionUtils::ExtractFloatsFromString(sLine.GetStartPointer(), 3, &v.x); // reads up to three texture-coordinates

      m_TexCoords.PushBack(v);
    }
    else if (sFirst.IsEqual_NoCase("vn")) // line declares a normal
    {
      bContainsNormals = true;

      plVec3 v(0.0f);
      plConversionUtils::ExtractFloatsFromString(sLine.GetStartPointer(), 3, &v.x);
      v.Normalize(); // make sure normals are indeed normalized

      m_Normals.PushBack(v);
    }
    else if (sFirst.IsEqual_NoCase("f")) // line declares a face
    {
      Face face;
      face.m_uiMaterialID = uiCurMaterial;

      const char* szCurPos;

      // loop through all vertices, that are found
      while (sLine.IsValid())
      {
        plInt32 id;

        // read the position index
        if (plConversionUtils::StringToInt(sLine.GetStartPointer(), id, &szCurPos).Failed())
          break; // nothing found, face-declaration is finished

        sLine.SetStartPosition(szCurPos);

        FaceVertex Vertex;
        Vertex.m_uiPositionID = uiPositionOffset + id - 1; // OBJ indices start at 1, so decrement them to start at 0

        // tex-coords were declared, so they will be used in the faces
        if (bContainsTexCoords)
        {
          if (!SkipSlash(sLine))
            break;

          if (plConversionUtils::StringToInt(sLine.GetStartPointer(), id, &szCurPos).Failed())
            break;

          sLine.SetStartPosition(szCurPos);

          Vertex.m_uiTexCoordID = uiTexCoordOffset + id - 1; // OBJ indices start at 1, so decrement them to start at 0
        }

        // normals were declared, so they will be used in the faces
        if (bContainsNormals)
        {
          if (!SkipSlash(sLine))
            break;

          if (plConversionUtils::StringToInt(sLine.GetStartPointer(), id, &szCurPos).Failed())
            break;

          sLine.SetStartPosition(szCurPos);

          Vertex.m_uiNormalID = uiNormalOffset + id - 1; // OBJ indices start at 1, so decrement them to start at 0
        }

        // stores the next vertex of the face
        face.m_Vertices.PushBack(Vertex);
      }

      // only allow faces with at least 3 vertices
      if (face.m_Vertices.GetCount() >= 3)
      {
        plVec3 v1, v2, v3;
        v1 = m_Positions[face.m_Vertices[0].m_uiPositionID];
        v2 = m_Positions[face.m_Vertices[1].m_uiPositionID];
        v3 = m_Positions[face.m_Vertices[2].m_uiPositionID];

        face.m_vNormal.CalculateNormal(v1, v2, v3).IgnoreResult();

        // done reading the face, store it
        m_Faces.PushBack(face);
      }
    }
    else if (sFirst.IsEqual_NoCase("usemtl")) // next material to be used for the following faces
    {
      if (bIgnoreMaterials)
        uiCurMaterial = 0xFFFFFFFF;
      else
      {
        // look-up the ID of this material

        bool bExisted = false;
        auto mat = m_Materials.FindOrAdd(sLine, &bExisted).Value();

        if (!bExisted)
          mat.m_uiMaterialID = m_Materials.GetCount() - 1;

        uiCurMaterial = mat.m_uiMaterialID;
      }
    }
  }

  return PL_SUCCESS;
}

void plOBJLoader::SortFacesByMaterial()
{
  // sort all faces by material-ID
  m_Faces.Sort();
}

void plOBJLoader::ComputeTangentSpaceVectors()
{
  // cannot compute tangents without texture-coordinates
  if (!HasTextureCoordinates())
    return;

  for (plUInt32 f = 0; f < m_Faces.GetCount(); ++f)
  {
    Face& face = m_Faces[f];

    const plVec3 p1 = m_Positions[face.m_Vertices[0].m_uiPositionID];
    const plVec3 p2 = m_Positions[face.m_Vertices[1].m_uiPositionID];
    const plVec3 p3 = m_Positions[face.m_Vertices[2].m_uiPositionID];

    const plVec3 tc1 = m_TexCoords[face.m_Vertices[0].m_uiTexCoordID];
    const plVec3 tc2 = m_TexCoords[face.m_Vertices[1].m_uiTexCoordID];
    const plVec3 tc3 = m_TexCoords[face.m_Vertices[2].m_uiTexCoordID];

    plVec3 v2v1 = p2 - p1;
    plVec3 v3v1 = p3 - p1;

    float c2c1_T = tc2.x - tc1.x;
    float c2c1_B = tc2.y - tc1.y;

    float c3c1_T = tc3.x - tc1.x;
    float c3c1_B = tc3.y - tc1.y;

    float fDenominator = c2c1_T * c3c1_B - c3c1_T * c2c1_B;

    float fScale1 = 1.0f / fDenominator;

    plVec3 T, B;
    T = plVec3(
      (c3c1_B * v2v1.x - c2c1_B * v3v1.x) * fScale1, (c3c1_B * v2v1.y - c2c1_B * v3v1.y) * fScale1, (c3c1_B * v2v1.z - c2c1_B * v3v1.z) * fScale1);

    B = plVec3(
      (-c3c1_T * v2v1.x + c2c1_T * v3v1.x) * fScale1, (-c3c1_T * v2v1.y + c2c1_T * v3v1.y) * fScale1, (-c3c1_T * v2v1.z + c2c1_T * v3v1.z) * fScale1);

    T.Normalize();
    B.Normalize();

    face.m_vTangent = T;
    face.m_vBiTangent = face.m_vNormal.CrossRH(face.m_vTangent).GetNormalized();
  }
}

plResult plOBJLoader::LoadMTL(const char* szFile, const char* szMaterialBasePath)
{
  plFileReader File;
  if (File.Open(szFile).Failed())
    return PL_FAILURE;

  plString sContent;
  sContent.ReadAll(File);

  plStringView sText = sContent;

  plString sCurMatName;
  plStringBuilder sTemp;

  while (sText.IsValid())
  {
    plStringView sLine = ReadLine(sText);
    const plStringView sFirst = ReadString(sLine);

    if (sFirst.IsEqual_NoCase("newmtl")) // declares a new material with a given name
    {
      sCurMatName = sLine;

      bool bExisted = false;
      auto mat = m_Materials.FindOrAdd(sCurMatName, &bExisted).Value();

      if (!bExisted)
      {
        mat.m_uiMaterialID = m_Materials.GetCount() - 1;
      }
    }
    else if (sFirst.IsEqual_NoCase("map_Kd"))
    {
      sTemp = szMaterialBasePath;
      sTemp.AppendPath(sLine.GetStartPointer());

      m_Materials[sCurMatName].m_sDiffuseTexture = sTemp;
    }
  }

  return PL_SUCCESS;
}



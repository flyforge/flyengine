#include <Foundation/FoundationPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/RttiConverter.h>

PL_ENUMERABLE_CLASS_IMPLEMENTATION(plGraphPatch);

plGraphPatch::plGraphPatch(const char* szType, plUInt32 uiTypeVersion, PatchType type)
  : m_szType(szType)
  , m_uiTypeVersion(uiTypeVersion)
  , m_PatchType(type)
{
}

const char* plGraphPatch::GetType() const
{
  return m_szType;
}

plUInt32 plGraphPatch::GetTypeVersion() const
{
  return m_uiTypeVersion;
}


plGraphPatch::PatchType plGraphPatch::GetPatchType() const
{
  return m_PatchType;
}



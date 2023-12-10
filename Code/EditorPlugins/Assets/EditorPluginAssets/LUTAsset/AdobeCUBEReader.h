
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Types/Status.h>

class plLogInterface;
class plStreamReader;

/// \brief Simple implementation to read Adobe CUBE LUT files
///
/// Currently only reads 3D LUTs as this is the data we need for our lookup textures in the tone mapping step.
class plAdobeCUBEReader
{
public:
  plAdobeCUBEReader();
  ~plAdobeCUBEReader();

  plStatus ParseFile(plStreamReader& inout_stream, plLogInterface* pLog = nullptr);

  plVec3 GetDomainMin() const;
  plVec3 GetDomainMax() const;

  plUInt32 GetLUTSize() const;
  const plString& GetTitle() const;

  plVec3 GetLUTEntry(plUInt32 r, plUInt32 g, plUInt32 b) const;

protected:
  plUInt32 m_uiLUTSize = 0;
  plString m_sTitle = "<UNTITLED>";

  plVec3 m_vDomainMin = plVec3::MakeZero();
  plVec3 m_vDomainMax = plVec3(1.0f);

  plDynamicArray<plVec3> m_LUTValues;

  plUInt32 GetLUTIndex(plUInt32 r, plUInt32 g, plUInt32 b) const;
};

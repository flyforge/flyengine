#pragma once

#include "CppStructure.h"
#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>

class plFileWriter;
class CppStructure;

enum class TargetType
{
  None,
  Value,
  Reference,
  Union,
  InterfaceStruct,
};

class DLangGenerator
{
public:
  DLangGenerator();
  ~DLangGenerator();

  void SetStructure(const CppStructure& structure);

  void ClearOutput();
  const plStringView GetOutput() const { return m_sOutput.GetView(); }

  plResult GenerateStructure(const char* szClassName, TargetType targetType, bool bWithSurroundings = true);
  plResult GenerateEnum(const char* szEnumName);

  void BeginStructure(const CE_Structure& ce, TargetType targetType, bool bWithSurroundings);
  void EndStructure(const CE_Structure& ce, bool bWithSurroundings);

  plResult WriteMembers(const CE_Structure& ce);
  plResult WriteMethod(const CE_Method& ce);
  plResult WriteConstructor(const CE_Constructor& ce);
  plResult WriteField(const CE_Field& ce);
  plResult WriteEnum(const CE_Enumeration& ce);
  plResult WriteVariable(const CE_Variable& ce);
  plResult WriteTypedef(const CE_Typedef& ce);

  plResult BuildType(plStringBuilder& out, const plString& sTypeID, bool* bIsValueType = nullptr) const;

  void WriteVisibility(CE_Visibility vis);

  CE_Visibility m_lastVisibility = CE_Visibility::None;

  void Indent() { m_iIndentation += 1; }
  void Unindent() { m_iIndentation -= 1; }

  void WriteIndentation(plInt32 iIndentOffset = 0);
  void EndLine();
  void WriteIndentedLine(const plFormatString& fmt, plInt32 iIndentOffset = 0);

  void WhitelistType(const char* szTypeName, TargetType type);
  bool IsTypeWhitelisted(const plString& type, bool* bIsValueType) const;

  static bool IsAllowedDefaultArg(plString& arg);

protected:
  const CppStructure* m_pStructure = nullptr;

  plStringBuilder m_sOutput;
  int m_iIndentation = 0;
  int m_iSubStructure = 0;

  TargetType m_TargetType = TargetType::None;
  plMap<plString, TargetType> m_WhitelistedTypes;
};

#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <GameEngine/GameEngineDLL.h>

class plOpenDdlWriter;
class plOpenDdlReaderElement;

class PLASMA_GAMEENGINE_DLL plGameAppInputConfig
{
public:
  constexpr static plUInt32 MaxInputSlotAlternatives = 3;

  static constexpr const plStringView s_sConfigFile = ":project/RuntimeConfigs/InputConfig.ddl"_plsv;

  plGameAppInputConfig();

  void Apply() const;
  void WriteToDDL(plOpenDdlWriter& writer) const;
  void ReadFromDDL(const plOpenDdlReaderElement* pAction);

  static void ApplyAll(const plArrayPtr<plGameAppInputConfig>& actions);
  static void WriteToDDL(plStreamWriter& stream, const plArrayPtr<plGameAppInputConfig>& actions);
  static void ReadFromDDL(plStreamReader& stream, plHybridArray<plGameAppInputConfig, 32>& out_actions);

  plString m_sInputSet;
  plString m_sInputAction;

  plString m_sInputSlotTrigger[MaxInputSlotAlternatives];

  float m_fInputSlotScale[MaxInputSlotAlternatives];

  bool m_bApplyTimeScaling = true;
};

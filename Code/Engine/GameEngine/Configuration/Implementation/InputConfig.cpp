#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <GameEngine/Configuration/InputConfig.h>

PL_CHECK_AT_COMPILETIME_MSG(plGameAppInputConfig::MaxInputSlotAlternatives == plInputActionConfig::MaxInputSlotAlternatives, "Max values should be kept in sync");

plGameAppInputConfig::plGameAppInputConfig()
{
  for (plUInt16 i = 0; i < MaxInputSlotAlternatives; ++i)
  {
    m_fInputSlotScale[i] = 1.0f;
    m_sInputSlotTrigger[i] = plInputSlot_None;
  }
}

void plGameAppInputConfig::Apply() const
{
  plInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = m_bApplyTimeScaling;

  for (plUInt32 i = 0; i < MaxInputSlotAlternatives; ++i)
  {
    cfg.m_sInputSlotTrigger[i] = m_sInputSlotTrigger[i];
    cfg.m_fInputSlotScale[i] = m_fInputSlotScale[i];
  }

  plInputManager::SetInputActionConfig(m_sInputSet, m_sInputAction, cfg, true);
}

void plGameAppInputConfig::WriteToDDL(plStreamWriter& inout_stream, const plArrayPtr<plGameAppInputConfig>& actions)
{
  plOpenDdlWriter writer;
  writer.SetCompactMode(false);
  writer.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Readable);
  writer.SetPrimitiveTypeStringMode(plOpenDdlWriter::TypeStringMode::Compliant);
  writer.SetOutputStream(&inout_stream);

  for (const plGameAppInputConfig& config : actions)
  {
    config.WriteToDDL(writer);
  }
}

void plGameAppInputConfig::WriteToDDL(plOpenDdlWriter& ref_writer) const
{
  ref_writer.BeginObject("InputAction");
  {
    plOpenDdlUtils::StoreString(ref_writer, m_sInputSet, "Set");
    plOpenDdlUtils::StoreString(ref_writer, m_sInputAction, "Action");
    plOpenDdlUtils::StoreBool(ref_writer, m_bApplyTimeScaling, "TimeScale");

    for (int i = 0; i < 3; ++i)
    {
      if (!m_sInputSlotTrigger[i].IsEmpty())
      {
        ref_writer.BeginObject("Slot");
        {
          plOpenDdlUtils::StoreString(ref_writer, m_sInputSlotTrigger[i], "Key");
          plOpenDdlUtils::StoreFloat(ref_writer, m_fInputSlotScale[i], "Scale");
        }
        ref_writer.EndObject();
      }
    }
  }
  ref_writer.EndObject();
}

void plGameAppInputConfig::ReadFromDDL(plStreamReader& inout_stream, plHybridArray<plGameAppInputConfig, 32>& out_actions)
{
  plOpenDdlReader reader;

  if (reader.ParseDocument(inout_stream, 0, plLog::GetThreadLocalLogSystem()).Failed())
    return;

  const plOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const plOpenDdlReaderElement* pAction = pRoot->GetFirstChild(); pAction != nullptr; pAction = pAction->GetSibling())
  {
    if (!pAction->IsCustomType("InputAction"))
      continue;

    plGameAppInputConfig& cfg = out_actions.ExpandAndGetRef();

    cfg.ReadFromDDL(pAction);
  }
}

void plGameAppInputConfig::ReadFromDDL(const plOpenDdlReaderElement* pInput)
{
  const plOpenDdlReaderElement* pSet = pInput->FindChildOfType(plOpenDdlPrimitiveType::String, "Set");
  const plOpenDdlReaderElement* pAction = pInput->FindChildOfType(plOpenDdlPrimitiveType::String, "Action");
  const plOpenDdlReaderElement* pTimeScale = pInput->FindChildOfType(plOpenDdlPrimitiveType::Bool, "TimeScale");


  if (pSet)
    m_sInputSet = pSet->GetPrimitivesString()[0];

  if (pAction)
    m_sInputAction = pAction->GetPrimitivesString()[0];

  if (pTimeScale)
    m_bApplyTimeScaling = pTimeScale->GetPrimitivesBool()[0];

  plInt32 iSlot = 0;
  for (const plOpenDdlReaderElement* pSlot = pInput->GetFirstChild(); pSlot != nullptr; pSlot = pSlot->GetSibling())
  {
    if (!pSlot->IsCustomType("Slot"))
      continue;

    const plOpenDdlReaderElement* pKey = pSlot->FindChildOfType(plOpenDdlPrimitiveType::String, "Key");
    const plOpenDdlReaderElement* pScale = pSlot->FindChildOfType(plOpenDdlPrimitiveType::Float, "Scale");

    if (pKey)
      m_sInputSlotTrigger[iSlot] = pKey->GetPrimitivesString()[0];

    if (pScale)
      m_fInputSlotScale[iSlot] = pScale->GetPrimitivesFloat()[0];

    ++iSlot;

    if (iSlot >= MaxInputSlotAlternatives)
      break;
  }
}

void plGameAppInputConfig::ApplyAll(const plArrayPtr<plGameAppInputConfig>& actions)
{
  for (const plGameAppInputConfig& config : actions)
  {
    config.Apply();
  }
}



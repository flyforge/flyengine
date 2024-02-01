#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
PL_IMPLEMENT_MESSAGE_TYPE(plMsgDamage);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgDamage, 1, plRTTIDefaultAllocator<plMsgDamage>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Damage", m_fDamage),
    PL_MEMBER_PROPERTY("HitObjectName", m_sHitObjectName),
    PL_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    PL_MEMBER_PROPERTY("ImpactDirection", m_vImpactDirection),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;


PL_STATICLINK_FILE(GameEngine, GameEngine_Messages_Implementation_DamageMessage);

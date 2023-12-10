#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgDamage);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgDamage, 1, plRTTIDefaultAllocator<plMsgDamage>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Damage", m_fDamage),
    PLASMA_MEMBER_PROPERTY("HitObjectName", m_sHitObjectName),
    PLASMA_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    PLASMA_MEMBER_PROPERTY("ImpactDirection", m_vImpactDirection),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Messages_Implementation_DamageMessage);

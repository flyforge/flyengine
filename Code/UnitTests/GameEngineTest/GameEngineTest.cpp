#include <GameEngineTest/GameEngineTestPCH.h>

#include <RendererCore/Textures/TextureUtils.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

PLASMA_TESTFRAMEWORK_ENTRY_POINT_BEGIN("GameEngineTest", "GameEngine Tests")
{
  plTextureUtils::s_bForceFullQualityAlways = true; // never allow to use low-res textures
  plTestFramework::GetInstance()->SetTestTimeout(1000 * 60 * 20);
  plTestFramework::s_bCallstackOnAssert = true;
}
PLASMA_TESTFRAMEWORK_ENTRY_POINT_END()

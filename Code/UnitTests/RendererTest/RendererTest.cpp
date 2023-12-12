#include <RendererTest/RendererTestPCH.h>

#include <RendererCore/Textures/TextureUtils.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

PLASMA_TESTFRAMEWORK_ENTRY_POINT_BEGIN("RendererTest", "Renderer Tests")
{
  plTextureUtils::s_bForceFullQualityAlways = true; // never allow to use low-res textures
}
PLASMA_TESTFRAMEWORK_ENTRY_POINT_END()

#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"

plTestAppRun plRendererTestBasics::SubtestLineRendering()
{
  BeginFrame();

  plColor clear(0, 0, 0, 0);
  ClearScreen(clear);

  RenderLineObjects(plShaderBindFlags::Default);

  PLASMA_TEST_IMAGE(0, 150);

  EndFrame();

  return m_iFrame < 0 ? plTestAppRun::Continue : plTestAppRun::Quit;
}

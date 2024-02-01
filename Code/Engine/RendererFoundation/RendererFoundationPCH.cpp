#include <RendererFoundation/RendererFoundationPCH.h>

PL_STATICLINK_LIBRARY(RendererFoundation)
{
  if (bReturn)
    return;

  PL_STATICLINK_REFERENCE(RendererFoundation_Device_Implementation_SharedTextureSwapChain);
  PL_STATICLINK_REFERENCE(RendererFoundation_Device_Implementation_SwapChain);
  PL_STATICLINK_REFERENCE(RendererFoundation_Profiling_Implementation_Profiling);
  PL_STATICLINK_REFERENCE(RendererFoundation_RendererReflection);
}

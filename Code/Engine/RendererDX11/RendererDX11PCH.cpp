#include <RendererDX11/RendererDX11PCH.h>

PLASMA_STATICLINK_LIBRARY(RendererDX11)
{
  if (bReturn)
    return;

  PLASMA_STATICLINK_REFERENCE(RendererDX11_Context_Implementation_ContextDX11);
  PLASMA_STATICLINK_REFERENCE(RendererDX11_Device_Implementation_DeviceDX11);
  PLASMA_STATICLINK_REFERENCE(RendererDX11_Device_Implementation_SwapChainDX11);
  PLASMA_STATICLINK_REFERENCE(RendererDX11_Resources_Implementation_BufferDX11);
  PLASMA_STATICLINK_REFERENCE(RendererDX11_Resources_Implementation_FenceDX11);
  PLASMA_STATICLINK_REFERENCE(RendererDX11_Resources_Implementation_QueryDX11);
  PLASMA_STATICLINK_REFERENCE(RendererDX11_Resources_Implementation_RenderTargetViewDX11);
  PLASMA_STATICLINK_REFERENCE(RendererDX11_Resources_Implementation_ResourceViewDX11);
  PLASMA_STATICLINK_REFERENCE(RendererDX11_Resources_Implementation_TextureDX11);
  PLASMA_STATICLINK_REFERENCE(RendererDX11_Resources_Implementation_UnorderedAccessViewDX11);
  PLASMA_STATICLINK_REFERENCE(RendererDX11_Shader_Implementation_ShaderDX11);
  PLASMA_STATICLINK_REFERENCE(RendererDX11_Shader_Implementation_VertexDeclarationDX11);
  PLASMA_STATICLINK_REFERENCE(RendererDX11_State_Implementation_StateDX11);
}

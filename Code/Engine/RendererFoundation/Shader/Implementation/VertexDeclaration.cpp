#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Shader/VertexDeclaration.h>

plGALVertexDeclaration::plGALVertexDeclaration(const plGALVertexDeclarationCreationDescription& Description)
  : plGALObject(Description)
{
}

plGALVertexDeclaration::~plGALVertexDeclaration() = default;



PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_VertexDeclaration);
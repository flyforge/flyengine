
PLASMA_ALWAYS_INLINE plDebugRenderer::Line::Line() = default;

PLASMA_ALWAYS_INLINE plDebugRenderer::Line::Line(const plVec3& vStart, const plVec3& vEnd)
  : m_start(vStart)
  , m_end(vEnd)
{
}

PLASMA_ALWAYS_INLINE plDebugRenderer::Line::Line(const plVec3& vStart, const plVec3& vEnd, const plColor& color)
  : m_start(vStart)
  , m_end(vEnd)
  , m_startColor(color)
  , m_endColor(color)
{
}

//////////////////////////////////////////////////////////////////////////

PLASMA_ALWAYS_INLINE plDebugRenderer::Triangle::Triangle() = default;

PLASMA_ALWAYS_INLINE plDebugRenderer::Triangle::Triangle(const plVec3& v0, const plVec3& v1, const plVec3& v2)

{
  m_position[0] = v0;
  m_position[1] = v1;
  m_position[2] = v2;
}

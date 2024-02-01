plInt32 plImageFilterWeights::GetFirstSourceSampleIndex(plUInt32 uiDstSampleIndex) const
{
  plSimdFloat dstSampleInSourceSpace = (plSimdFloat(uiDstSampleIndex) + plSimdFloat(0.5f)) * m_fDestToSourceScale;

  return plInt32(plMath::Floor(dstSampleInSourceSpace - m_fWidthInSourceSpace));
}

inline plArrayPtr<const float> plImageFilterWeights::ViewWeights() const
{
  return m_Weights;
}

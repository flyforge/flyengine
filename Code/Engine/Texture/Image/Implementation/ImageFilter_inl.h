plInt32 plImageFilterWeights::GetFirstSourceSampleIndex(plUInt32 dstSampleIndex) const
{
  plSimdFloat dstSampleInSourceSpace = (plSimdFloat(dstSampleIndex) + plSimdFloat(0.5f)) * m_destToSourceScale;

  return plInt32(plMath::Floor(dstSampleInSourceSpace - m_widthInSourceSpace));
}

inline plArrayPtr<const float> plImageFilterWeights::ViewWeights() const
{
  return m_weights;
}

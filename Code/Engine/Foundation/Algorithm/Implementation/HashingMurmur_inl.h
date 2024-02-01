
namespace plInternal
{
  constexpr plUInt32 MURMUR_M = 0x5bd1e995;
  constexpr plUInt32 MURMUR_R = 24;

  template <size_t N, size_t Loop>
  struct CompileTimeMurmurHash
  {
    constexpr PL_ALWAYS_INLINE plUInt32 operator()(plUInt32 uiHash, const char (&str)[N], size_t i) const
    {
      return CompileTimeMurmurHash<N, Loop - 4>()(CompileTimeMurmurHash<N, 4>()(uiHash, str, i), str, i + 4);
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 4>
  {
    static constexpr PL_ALWAYS_INLINE plUInt32 helper(plUInt32 k) { return (k ^ (k >> MURMUR_R)) * MURMUR_M; }

    constexpr PL_ALWAYS_INLINE plUInt32 operator()(plUInt32 uiHash, const char (&str)[N], size_t i) const
    {
      // In C++11 constexpr local variables are not allowed. Need to express the following without "plUInt32 k"
      // (this restriction is lifted in C++14's generalized constexpr)
      // plUInt32 k = ((str[i + 0]) | ((str[i + 1]) << 8) | ((str[i + 2]) << 16) | ((str[i + 3]) << 24));
      // k *= MURMUR_M;
      // k ^= (k >> MURMUR_R);
      // k *= MURMUR_M;
      // return (hash * MURMUR_M) ^ k;

      return (uiHash * MURMUR_M) ^ helper(((str[i + 0]) | ((str[i + 1]) << 8) | ((str[i + 2]) << 16) | ((str[i + 3]) << 24)) * MURMUR_M);
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 3>
  {
    constexpr PL_ALWAYS_INLINE plUInt32 operator()(plUInt32 uiHash, const char (&str)[N], size_t i) const
    {
      return (uiHash ^ (str[i + 2] << 16) ^ (str[i + 1] << 8) ^ (str[i + 0])) * MURMUR_M;
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 2>
  {
    constexpr PL_ALWAYS_INLINE plUInt32 operator()(plUInt32 uiHash, const char (&str)[N], size_t i) const
    {
      return (uiHash ^ (str[i + 1] << 8) ^ (str[i])) * MURMUR_M;
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 1>
  {
    constexpr PL_ALWAYS_INLINE plUInt32 operator()(plUInt32 uiHash, const char (&str)[N], size_t i) const { return (uiHash ^ (str[i])) * MURMUR_M; }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 0>
  {
    constexpr PL_ALWAYS_INLINE plUInt32 operator()(plUInt32 uiHash, const char (&str)[N], size_t i) const { return uiHash; }
  };

  constexpr plUInt32 rightShift_and_xorWithPrevSelf(plUInt32 h, plUInt32 uiShift) { return h ^ (h >> uiShift); }
} // namespace plInternal

template <size_t N>
constexpr PL_ALWAYS_INLINE plUInt32 plHashingUtils::MurmurHash32String(const char (&str)[N], plUInt32 uiSeed)
{
  // In C++11 constexpr local variables are not allowed. Need to express the following without "plUInt32 h"
  // (this restriction is lifted in C++14's generalized constexpr)
  // const plUInt32 uiStrlen = (plUInt32)(N - 1);
  // plUInt32 h = plInternal::CompileTimeMurmurHash<N - 1>(uiSeed ^ uiStrlen, str, 0);
  // h ^= h >> 13;
  // h *= plInternal::MURMUR_M;
  // h ^= h >> 15;
  // return h;

  return plInternal::rightShift_and_xorWithPrevSelf(
    plInternal::rightShift_and_xorWithPrevSelf(plInternal::CompileTimeMurmurHash<N, N - 1>()(uiSeed ^ static_cast<plUInt32>(N - 1), str, 0), 13) *
      plInternal::MURMUR_M,
    15);
}

PL_ALWAYS_INLINE plUInt32 plHashingUtils::MurmurHash32String(plStringView sStr, plUInt32 uiSeed)
{
  return MurmurHash32(sStr.GetStartPointer(), sStr.GetElementCount(), uiSeed);
}


inline plColorLinear16f::plColorLinear16f() = default;

inline plColorLinear16f::plColorLinear16f(plFloat16 r, plFloat16 g, plFloat16 b, plFloat16 a)
  : r(r)
  , g(g)
  , b(b)
  , a(a)
{
}

inline plColorLinear16f::plColorLinear16f(const plColor& color)
  : r(color.r)
  , g(color.g)
  , b(color.b)
  , a(color.a)
{
}

inline plColor plColorLinear16f::ToLinearFloat() const
{
  return plColor(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a));
}

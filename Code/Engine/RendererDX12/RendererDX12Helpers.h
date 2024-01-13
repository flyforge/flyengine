#pragma once

#include <RendererDX12/RendererDX12PCH.h>

inline namespace
{
  inline std::string HrToString(HRESULT hr)
  {
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
  }

  class HrException : public std::runtime_error
  {
  public:
    HrException(HRESULT hr)
      : std::runtime_error(HrToString(hr))
      , m_hr(hr)
    {
    }

    HRESULT Error() const { return m_hr; }

  private:
    const HRESULT m_hr;
  };

  #define SAFE_RELEASE(p) \
  if (p)                \
  (p)->Release();


  inline void ThrowIfFailed(HRESULT hr,const char* Message,bool throwexec = false)
  {
    if (FAILED(hr))
    {
      plLog::Error("RendererDX12: {0} HRESULT CODE: {1}", Message, HrToString(hr));
      if (throwexec == true)
        throw HrException(hr);
    }
  }
}
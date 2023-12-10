#pragma once

#include <Foundation/Basics.h>

/// \brief Collection of helper methods when working with endianess "problems"
struct PLASMA_FOUNDATION_DLL plEndianHelper
{

  /// \brief Returns true if called on a big endian system, false otherwise.
  ///
  /// \note Note that usually the compile time decisions with the defines PLASMA_PLATFORM_LITTLE_ENDIAN, PLASMA_PLATFORM_BIG_ENDIAN is preferred.
  static inline bool IsBigEndian()
  {
    const int i = 1;
    return (*(char*)&i) == 0;
  }

  /// \brief Returns true if called on a little endian system, false otherwise.
  ///
  /// \note Note that usually the compile time decisions with the defines PLASMA_PLATFORM_LITTLE_ENDIAN, PLASMA_PLATFORM_BIG_ENDIAN is preferred.
  static inline bool IsLittleEndian() { return !IsBigEndian(); }

  /// \brief Switches endianess of the given array of words (16 bit values).
  static inline void SwitchWords(plUInt16* pWords, plUInt32 uiCount) // [tested]
  {
    for (plUInt32 i = 0; i < uiCount; i++)
      pWords[i] = Switch(pWords[i]);
  }

  /// \brief Switches endianess of the given array of double words (32 bit values).
  static inline void SwitchDWords(plUInt32* pDWords, plUInt32 uiCount) // [tested]
  {
    for (plUInt32 i = 0; i < uiCount; i++)
      pDWords[i] = Switch(pDWords[i]);
  }

  /// \brief Switches endianess of the given array of quad words (64 bit values).
  static inline void SwitchQWords(plUInt64* pQWords, plUInt32 uiCount) // [tested]
  {
    for (plUInt32 i = 0; i < uiCount; i++)
      pQWords[i] = Switch(pQWords[i]);
  }

  /// \brief Returns a single switched word (16 bit value).
  static PLASMA_ALWAYS_INLINE plUInt16 Switch(plUInt16 uiWord) // [tested]
  {
    return (((uiWord & 0xFF) << 8) | ((uiWord >> 8) & 0xFF));
  }

  /// \brief Returns a single switched double word (32 bit value).
  static PLASMA_ALWAYS_INLINE plUInt32 Switch(plUInt32 uiDWord) // [tested]
  {
    return (((uiDWord & 0xFF) << 24) | (((uiDWord >> 8) & 0xFF) << 16) | (((uiDWord >> 16) & 0xFF) << 8) | ((uiDWord >> 24) & 0xFF));
  }

  /// \brief Returns a single switched quad word (64 bit value).
  static PLASMA_ALWAYS_INLINE plUInt64 Switch(plUInt64 uiQWord) // [tested]
  {
    return (((uiQWord & 0xFF) << 56) | ((uiQWord & 0xFF00) << 40) | ((uiQWord & 0xFF0000) << 24) | ((uiQWord & 0xFF000000) << 8) |
            ((uiQWord & 0xFF00000000) >> 8) | ((uiQWord & 0xFF0000000000) >> 24) | ((uiQWord & 0xFF000000000000) >> 40) |
            ((uiQWord & 0xFF00000000000000) >> 56));
  }

  /// \brief Switches a value in place (template accepts pointers for 2, 4 & 8 byte data types)
  template <typename T>
  static void SwitchInPlace(T* pValue) // [tested]
  {
    PLASMA_CHECK_AT_COMPILETIME_MSG(
      (sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8), "Switch in place only works for type equivalents of plUInt16, plUInt32, plUInt64!");

    if (sizeof(T) == 2)
    {
      struct TAnd16BitUnion
      {
        union
        {
          plUInt16 BitValue;
          T TValue;
        };
      };

      TAnd16BitUnion Temp;
      Temp.TValue = *pValue;
      Temp.BitValue = Switch(Temp.BitValue);

      *pValue = Temp.TValue;
    }
    else if (sizeof(T) == 4)
    {
      struct TAnd32BitUnion
      {
        union
        {
          plUInt32 BitValue;
          T TValue;
        };
      };

      TAnd32BitUnion Temp;
      Temp.TValue = *pValue;
      Temp.BitValue = Switch(Temp.BitValue);

      *pValue = Temp.TValue;
    }
    else if (sizeof(T) == 8)
    {
      struct TAnd64BitUnion
      {
        union
        {
          plUInt64 BitValue;
          T TValue;
        };
      };

      TAnd64BitUnion Temp;
      Temp.TValue = *pValue;
      Temp.BitValue = Switch(Temp.BitValue);

      *pValue = Temp.TValue;
    }
  }

#if PLASMA_ENABLED(PLASMA_PLATFORM_LITTLE_ENDIAN)

  static PLASMA_ALWAYS_INLINE void LittleEndianToNative(plUInt16* /*pWords*/, plUInt32 /*uiCount*/) {}

  static PLASMA_ALWAYS_INLINE void NativeToLittleEndian(plUInt16* /*pWords*/, plUInt32 /*uiCount*/) {}

  static PLASMA_ALWAYS_INLINE void LittleEndianToNative(plUInt32* /*pDWords*/, plUInt32 /*uiCount*/) {}

  static PLASMA_ALWAYS_INLINE void NativeToLittleEndian(plUInt32* /*pDWords*/, plUInt32 /*uiCount*/) {}

  static PLASMA_ALWAYS_INLINE void LittleEndianToNative(plUInt64* /*pQWords*/, plUInt32 /*uiCount*/) {}

  static PLASMA_ALWAYS_INLINE void NativeToLittleEndian(plUInt64* /*pQWords*/, plUInt32 /*uiCount*/) {}

  static PLASMA_ALWAYS_INLINE void BigEndianToNative(plUInt16* pWords, plUInt32 uiCount) { SwitchWords(pWords, uiCount); }

  static PLASMA_ALWAYS_INLINE void NativeToBigEndian(plUInt16* pWords, plUInt32 uiCount) { SwitchWords(pWords, uiCount); }

  static PLASMA_ALWAYS_INLINE void BigEndianToNative(plUInt32* pDWords, plUInt32 uiCount) { SwitchDWords(pDWords, uiCount); }

  static PLASMA_ALWAYS_INLINE void NativeToBigEndian(plUInt32* pDWords, plUInt32 uiCount) { SwitchDWords(pDWords, uiCount); }

  static PLASMA_ALWAYS_INLINE void BigEndianToNative(plUInt64* pQWords, plUInt32 uiCount) { SwitchQWords(pQWords, uiCount); }

  static PLASMA_ALWAYS_INLINE void NativeToBigEndian(plUInt64* pQWords, plUInt32 uiCount) { SwitchQWords(pQWords, uiCount); }

#elif PLASMA_ENABLED(PLASMA_PLATFORM_BIG_ENDIAN)

  static PLASMA_ALWAYS_INLINE void LittleEndianToNative(plUInt16* pWords, plUInt32 uiCount) { SwitchWords(pWords, uiCount); }

  static PLASMA_ALWAYS_INLINE void NativeToLittleEndian(plUInt16* pWords, plUInt32 uiCount) { SwitchWords(pWords, uiCount); }

  static PLASMA_ALWAYS_INLINE void LittleEndianToNative(plUInt32* pDWords, plUInt32 uiCount) { SwitchDWords(pDWords, uiCount); }

  static PLASMA_ALWAYS_INLINE void NativeToLittleEndian(plUInt32* pDWords, plUInt32 uiCount) { SwitchDWords(pDWords, uiCount); }

  static PLASMA_ALWAYS_INLINE void LittleEndianToNative(plUInt64* pQWords, plUInt32 uiCount) { SwitchQWords(pQWords, uiCount); }

  static PLASMA_ALWAYS_INLINE void NativeToLittleEndian(plUInt64* pQWords, plUInt32 uiCount) { SwitchQWords(pQWords, uiCount); }

  static PLASMA_ALWAYS_INLINE void BigEndianToNative(plUInt16* /*pWords*/, plUInt32 /*uiCount*/) {}

  static PLASMA_ALWAYS_INLINE void NativeToBigEndian(plUInt16* /*pWords*/, plUInt32 /*uiCount*/) {}

  static PLASMA_ALWAYS_INLINE void BigEndianToNative(plUInt32* /*pWords*/, plUInt32 /*uiCount*/) {}

  static PLASMA_ALWAYS_INLINE void NativeToBigEndian(plUInt32* /*pWords*/, plUInt32 /*uiCount*/) {}

  static PLASMA_ALWAYS_INLINE void BigEndianToNative(plUInt64* /*pWords*/, plUInt32 /*uiCount*/) {}

  static PLASMA_ALWAYS_INLINE void NativeToBigEndian(plUInt64* /*pWords*/, plUInt32 /*uiCount*/) {}

#endif


  /// \brief Switches a given struct according to the layout described in the szFormat parameter
  ///
  /// The format string may contain the characters:
  ///  - c, b for a member of 1 byte
  ///  - w, s for a member of 2 bytes (word, plUInt16)
  ///  - d for a member of 4 bytes (DWORD, plUInt32)
  ///  - q for a member of 8 bytes (DWORD, plUInt64)
  static void SwitchStruct(void* pDataPointer, const char* szFormat);

  /// \brief Templated helper method for SwitchStruct
  template <typename T>
  static void SwitchStruct(T* pDataPointer, const char* szFormat) // [tested]
  {
    SwitchStruct(static_cast<void*>(pDataPointer), szFormat);
  }

  /// \brief Switches a given set of struct according to the layout described in the szFormat parameter
  ///
  /// The format string may contain the characters:
  ///  - c, b for a member of 1 byte
  ///  - w, s for a member of 2 bytes (word, plUInt16)
  ///  - d for a member of 4 bytes (DWORD, plUInt32)
  ///  - q for a member of 8 bytes (DWORD, plUInt64)
  static void SwitchStructs(void* pDataPointer, const char* szFormat, plUInt32 uiStride, plUInt32 uiCount); // [tested]

  /// \brief Templated helper method for SwitchStructs
  template <typename T>
  static void SwitchStructs(T* pDataPointer, const char* szFormat, plUInt32 uiCount) // [tested]
  {
    SwitchStructs(static_cast<void*>(pDataPointer), szFormat, sizeof(T), uiCount);
  }
};

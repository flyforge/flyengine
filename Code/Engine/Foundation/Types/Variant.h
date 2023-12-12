#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Types/TypedPointer.h>
#include <Foundation/Types/Types.h>
#include <Foundation/Types/VariantType.h>

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <Foundation/Utilities/ConversionUtils.h>

class plRTTI;

/// \brief Defines a reference to an immutable object owned by an plVariant.
///
/// Used to store custom types inside an plVariant. As lifetime is governed by the plVariant, it is generally not safe to store an plTypedObject.
/// This class is needed to be able to differentiate between plVariantType::TypedPointer and plVariantType::TypedObject e.g. in plVariant::DispatchTo.
/// \sa plVariant, PLASMA_DECLARE_CUSTOM_VARIANT_TYPE
struct plTypedObject
{
  PLASMA_DECLARE_POD_TYPE();
  const void* m_pObject = nullptr;
  const plRTTI* m_pType = nullptr;

  bool operator==(const plTypedObject& rhs) const
  {
    return m_pObject == rhs.m_pObject;
  }
  bool operator!=(const plTypedObject& rhs) const
  {
    return m_pObject != rhs.m_pObject;
  }
};

/// \brief plVariant is a class that can store different types of variables, which is useful in situations where it is not clear up front,
/// which type of data will be passed around.
///
/// The variant supports a fixed list of types that it can store (\see plVariant::Type). All types of 16 bytes or less in size can be stored
/// without requiring a heap allocation. For larger types memory is allocated on the heap. In general variants should be used for code that
/// needs to be flexible. Although plVariant is implemented very efficiently, it should be avoided to use plVariant in code that needs to be
/// fast.
class PLASMA_FOUNDATION_DLL plVariant
{
public:
  using Type = plVariantType;
  template <typename T>
  using TypeDeduction = plVariantTypeDeduction<T>;

  /// \brief helper struct to wrap a string pointer
  struct StringWrapper
  {
    PLASMA_ALWAYS_INLINE StringWrapper(const char* szStr)
      : m_str(szStr)
    {
    }
    const char* m_str;
  };

  /// \brief Initializes the variant to be 'Invalid'
  plVariant(); // [tested]

  /// \brief Copies the data from the other variant.
  ///
  /// \note If the data of the variant needed to be allocated on the heap, it will be shared among variants.
  /// Thus, once you have stored such a type inside a variant, you can copy it to other variants, without introducing
  /// additional memory allocations.
  plVariant(const plVariant& other); // [tested]

  /// \brief Moves the data from the other variant.
  plVariant(plVariant&& other) noexcept; // [tested]

  plVariant(const bool& value);
  plVariant(const plInt8& value);
  plVariant(const plUInt8& value);
  plVariant(const plInt16& value);
  plVariant(const plUInt16& value);
  plVariant(const plInt32& value);
  plVariant(const plUInt32& value);
  plVariant(const plInt64& value);
  plVariant(const plUInt64& value);
  plVariant(const float& value);
  plVariant(const double& value);
  plVariant(const plColor& value);
  plVariant(const plVec2& value);
  plVariant(const plVec3& value);
  plVariant(const plVec4& value);
  plVariant(const plVec2I32& value);
  plVariant(const plVec3I32& value);
  plVariant(const plVec4I32& value);
  plVariant(const plVec2U32& value);
  plVariant(const plVec3U32& value);
  plVariant(const plVec4U32& value);
  plVariant(const plQuat& value);
  plVariant(const plMat3& value);
  plVariant(const plMat4& value);
  plVariant(const plTransform& value);
  plVariant(const char* value);
  plVariant(const plString& value);
  plVariant(const plUntrackedString& value);
  plVariant(const plStringView& value, bool bCopyString = true);
  plVariant(const plDataBuffer& value);
  plVariant(const plTime& value);
  plVariant(const plUuid& value);
  plVariant(const plAngle& value);
  plVariant(const plColorGammaUB& value);
  plVariant(const plHashedString& value);
  plVariant(const plTempHashedString& value);

  plVariant(const plVariantArray& value);
  plVariant(const plVariantDictionary& value);

  plVariant(const plTypedPointer& value);
  plVariant(const plTypedObject& value);

  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::CustomTypeCast, int> = 0>
  plVariant(const T& value);

  template <typename T>
  plVariant(const T* value);

  /// \brief Initializes to a TypedPointer of the given object and type.
  plVariant(void* value, const plRTTI* pType);

  /// \brief Initializes to a TypedObject by cloning the given object and type.
  void CopyTypedObject(const void* value, const plRTTI* pType); // [tested]

  /// \brief Initializes to a TypedObject by taking ownership of the given object and type.
  void MoveTypedObject(void* value, const plRTTI* pType); // [tested]

  /// \brief If necessary, this will deallocate any heap memory that is not in use any more.
  ~plVariant();

  /// \brief Copies the data from the \a other variant into this one.
  void operator=(const plVariant& other); // [tested]

  /// \brief Moves the data from the \a other variant into this one.
  void operator=(plVariant&& other) noexcept; // [tested]

  /// \brief Deduces the type of \a T and stores \a value.
  ///
  /// If the type to be stored in the variant is not supported, a compile time error will occur.
  template <typename T>
  void operator=(const T& value); // [tested]

  /// \brief Will compare the value of this variant to that of \a other.
  ///
  /// If both variants store 'numbers' (float, double, int types) the comparison will work, even if the types are not identical.
  ///
  /// \note If the two types are not numbers and not equal, an assert will occur. So be careful to only compare variants
  /// that can either both be converted to double (\see CanConvertTo()) or whose types are equal.
  bool operator==(const plVariant& other) const; // [tested]

  /// \brief Same as operator== (with a twist!)
  bool operator!=(const plVariant& other) const; // [tested]

  /// \brief See non-templated operator==
  template <typename T>
  bool operator==(const T& other) const; // [tested]

  /// \brief See non-templated operator!=
  template <typename T>
  bool operator!=(const T& other) const; // [tested]

  /// \brief Returns whether this variant stores any other type than 'Invalid'.
  bool IsValid() const; // [tested]

  /// \brief Returns whether the stored type is numerical type either integer or floating point.
  ///
  /// Bool counts as number.
  bool IsNumber() const; // [tested]

  /// \brief Returns whether the stored type is floating point (float or double).
  bool IsFloatingPoint() const; // [tested]

  /// \brief Returns whether the stored type is a string (plString or plStringView).
  bool IsString() const; // [tested]

  /// \brief Returns whether the stored type is a hashed string (plHashedString or plTempHashedString).
  bool IsHashedString() const;

  /// \brief Returns whether the stored type is exactly the given type.
  ///
  /// \note This explicitly also differentiates between the different integer types.
  /// So when the variant stores an Int32, IsA<Int64>() will return false, even though the types could be converted.
  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::DirectCast, int> = 0>
  bool IsA() const; // [tested]

  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::PointerCast, int> = 0>
  bool IsA() const; // [tested]

  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::TypedObject, int> = 0>
  bool IsA() const; // [tested]

  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::CustomTypeCast, int> = 0>
  bool IsA() const; // [tested]

  /// \brief Returns the exact plVariant::Type value.
  Type::Enum GetType() const; // [tested]

  /// \brief Returns the variants value as the provided type.
  ///
  /// \note This function does not do ANY type of conversion from the stored type to the given type. Not even integer conversions!
  /// If the types don't match, this function will assert!
  /// So be careful to use this function only when you know exactly that the stored type matches the expected type.
  ///
  /// Prefer to use ConvertTo() when you can instead.
  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::DirectCast, int> = 0>
  const T& Get() const; // [tested]

  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::PointerCast, int> = 0>
  T Get() const; // [tested]

  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::TypedObject, int> = 0>
  const T Get() const; // [tested]

  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::CustomTypeCast, int> = 0>
  const T& Get() const; // [tested]

  /// \brief Returns an writable plTypedPointer to the internal data.
  /// If the data is currently shared a clone will be made to ensure we hold the only reference.
  plTypedPointer GetWriteAccess(); // [tested]

  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::DirectCast, int> = 0>
  T& GetWritable(); // [tested]

  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::PointerCast, int> = 0>
  T GetWritable(); // [tested]

  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::CustomTypeCast, int> = 0>
  T& GetWritable(); // [tested]


  /// \brief Returns a const void* to the internal data.
  /// For TypedPointer and TypedObject this will return a pointer to the target object.
  const void* GetData() const; // [tested]

  /// \brief Returns the plRTTI type of the held value.
  /// For TypedPointer and TypedObject this will return the type of the target object.
  const plRTTI* GetReflectedType() const; // [tested]

  /// \brief Returns the sub value at iIndex. This could be an element in an array or a member property inside a reflected type.
  ///
  /// Out of bounds access is handled gracefully and will return an invalid variant.
  const plVariant operator[](plUInt32 uiIndex) const; // [tested]

  /// \brief Returns the sub value with szKey. This could be a value in a dictionary or a member property inside a reflected type.
  ///
  /// This function will return an invalid variant if no corresponding sub value is found.
  const plVariant operator[](StringWrapper key) const; // [tested]

  /// \brief Returns whether the stored type can generally be converted to the desired type.
  ///
  /// This function will return true for all number conversions, as float / double / int / etc. can generally be converted into each
  /// other. It will also return true for all conversion from string to number types, and from all 'simple' types (not array or dictionary)
  /// to string.
  ///
  /// \note This function only returns whether a conversion between the stored TYPE and the desired TYPE is generally possible. It does NOT
  /// return whether the stored VALUE is indeed convertible to the desired type. For example, a string is generally convertible to float, if
  /// it stores a string representation of a float value. If, however, it stores anything else, the conversion can still fail.
  ///
  /// The only way to figure out whether the stored data can be converted to some type, is to actually convert it, using ConvertTo(), and
  /// then to check the conversion status.
  template <typename T>
  bool CanConvertTo() const; // [tested]

  /// \brief Same as the templated CanConvertTo function.
  bool CanConvertTo(Type::Enum type) const; // [tested]

  /// \brief Tries to convert the stored value to the given type. The optional status parameter can be used to check whether the conversion
  /// succeeded.
  ///
  /// When CanConvertTo() returns false, ConvertTo() will also always fail. However, when CanConvertTo() returns true, this is no guarantee
  /// that ConvertTo() will succeed. Conversion between numbers and to strings will generally succeed. However, converting from a string to
  /// another type can fail or succeed, depending on the exact string value.
  template <typename T>
  T ConvertTo(plResult* out_pConversionStatus = nullptr) const; // [tested]

  /// \brief Same as the templated function.
  plVariant ConvertTo(Type::Enum type, plResult* out_pConversionStatus = nullptr) const; // [tested]

  /// \brief This will call the overloaded operator() (function call operator) of the provided functor.
  ///
  /// This allows to implement a functor that overloads operator() for different types and then call the proper version of that operator,
  /// depending on the provided runtime type. Note that the proper overload of operator() is selected by providing a dummy type, but it will
  /// contain no useful value. Instead, store the other necessary data inside the functor object, before calling this function. For example,
  /// store a pointer to a variant inside the functor object and then call DispatchTo to execute the function that will handle the given
  /// type of the variant.
  template <typename Functor, class... Args>
  static auto DispatchTo(Functor& ref_functor, Type::Enum type, Args&&... args); // [tested]

  /// \brief Computes the hash value of the stored data. Returns uiSeed (unchanged) for an invalid Variant.
  plUInt64 ComputeHash(plUInt64 uiSeed = 0) const;

private:
  friend class plVariantHelper;
  friend struct CompareFunc;
  friend struct GetTypeFromVariantFunc;

  struct SharedData
  {
    void* m_Ptr;
    const plRTTI* m_pType;
    plAtomicInteger32 m_uiRef = 1;
    PLASMA_ALWAYS_INLINE SharedData(void* pPtr, const plRTTI* pType)
      : m_Ptr(pPtr)
      , m_pType(pType)
    {
    }
    virtual ~SharedData() = default;
    virtual SharedData* Clone() const = 0;
  };

  template <typename T>
  class TypedSharedData : public SharedData
  {
  private:
    T m_t;

  public:
    PLASMA_ALWAYS_INLINE TypedSharedData(const T& value, const plRTTI* pType = nullptr)
      : SharedData(&m_t, pType)
      , m_t(value)
    {
    }

    virtual SharedData* Clone() const override
    {
      return PLASMA_DEFAULT_NEW(TypedSharedData<T>, m_t, m_pType);
    }
  };

  class RTTISharedData : public SharedData
  {
  public:
    RTTISharedData(void* pData, const plRTTI* pType);

    ~RTTISharedData();

    virtual SharedData* Clone() const override;
  };

  struct InlinedStruct
  {
    constexpr static int DataSize = 4 * sizeof(float) - sizeof(void*);
    plUInt8 m_Data[DataSize];
    const plRTTI* m_pType;
  };

  union Data
  {
    float f[4];
    SharedData* shared;
    InlinedStruct inlined;
  } m_Data;

  plUInt32 m_uiType : 31;
  plUInt32 m_bIsShared : 1; // NOLINT(pl*)

  template <typename T>
  void InitInplace(const T& value);

  template <typename T>
  void InitShared(const T& value);

  template <typename T>
  void InitTypedObject(const T& value, plTraitInt<0>);
  template <typename T>
  void InitTypedObject(const T& value, plTraitInt<1>);

  void InitTypedPointer(void* value, const plRTTI* pType);

  void Release();
  void CopyFrom(const plVariant& other);
  void MoveFrom(plVariant&& other);

  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::DirectCast, int> = 0>
  const T& Cast() const;
  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::PointerCast, int> = 0>
  T Cast() const;
  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::TypedObject, int> = 0>
  const T Cast() const;
  template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::CustomTypeCast, int> = 0>
  const T& Cast() const;

  static bool IsNumberStatic(plUInt32 type);
  static bool IsFloatingPointStatic(plUInt32 type);
  static bool IsStringStatic(plUInt32 type);
  static bool IsHashedStringStatic(plUInt32 type);
  static bool IsVector2Static(plUInt32 type);
  static bool IsVector3Static(plUInt32 type);
  static bool IsVector4Static(plUInt32 type);

  // Needed to prevent including plRTTI in plVariant.h
  static bool IsDerivedFrom(const plRTTI* pType1, const plRTTI* pType2);
  static plStringView GetTypeName(const plRTTI* pType);

  template <typename T>
  T ConvertNumber() const;
};

/// \brief An overload of plDynamicCast for dynamic casting a variant to a pointer type.
///
/// If the plVariant stores an plTypedPointer pointer, this pointer will be dynamically cast to T*.
/// If the plVariant stores any other type (or nothing), nullptr is returned.
template <typename T>
PLASMA_ALWAYS_INLINE T plDynamicCast(const plVariant& variant)
{
  if (variant.IsA<T>())
  {
    return variant.Get<T>();
  }

  return nullptr;
}

// Simple math operator overloads. An invalid variant is returned if the given variants have incompatible types.
PLASMA_FOUNDATION_DLL plVariant operator+(const plVariant& a, const plVariant& b);
PLASMA_FOUNDATION_DLL plVariant operator-(const plVariant& a, const plVariant& b);
PLASMA_FOUNDATION_DLL plVariant operator*(const plVariant& a, const plVariant& b);
PLASMA_FOUNDATION_DLL plVariant operator/(const plVariant& a, const plVariant& b);


namespace plMath
{
  /// \brief An overload of plMath::Lerp to interpolate variants. A and b must have the same type.
  ///
  /// If the type can't be interpolated like e.g. strings, a is returned for a fFactor less than 0.5, b is returned for a fFactor greater or equal to 0.5.
  PLASMA_FOUNDATION_DLL plVariant Lerp(const plVariant& a, const plVariant& b, double fFactor);
}

#include <Foundation/Types/Implementation/VariantHelper_inl.h>

#include <Foundation/Types/Implementation/Variant_inl.h>

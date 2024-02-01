#pragma once

#include <Foundation/Basics.h>

/// \brief A Unique ptr manages an object and destroys that object when it goes out of scope. It is ensure that only one unique ptr can
/// manage the same object.
template <typename T>
class plUniquePtr
{
  PL_DISALLOW_COPY_AND_ASSIGN(plUniquePtr);

public:
  PL_DECLARE_MEM_RELOCATABLE_TYPE();

  /// \brief Creates an empty unique ptr.
  plUniquePtr();

  /// \brief Creates a unique ptr from a freshly created instance through PL_NEW or PL_DEFAULT_NEW.
  template <typename U>
  plUniquePtr(const plInternal::NewInstance<U>& instance);

  /// \brief Creates a unique ptr from a pointer and an allocator. The passed allocator will be used to destroy the instance when the unique
  /// ptr goes out of scope.
  template <typename U>
  plUniquePtr(U* pInstance, plAllocator* pAllocator);

  /// \brief Move constructs a unique ptr from another. The other unique ptr will be empty afterwards to guarantee that there is only one
  /// unique ptr managing the same object.
  template <typename U>
  plUniquePtr(plUniquePtr<U>&& other);

  /// \brief Initialization with nullptr to be able to return nullptr in functions that return unique ptr.
  plUniquePtr(std::nullptr_t);

  /// \brief Destroys the managed object using the stored allocator.
  ~plUniquePtr();

  /// \brief Sets the unique ptr from a freshly created instance through PL_NEW or PL_DEFAULT_NEW.
  template <typename U>
  plUniquePtr<T>& operator=(const plInternal::NewInstance<U>& instance);

  /// \brief Move assigns a unique ptr from another. The other unique ptr will be empty afterwards to guarantee that there is only one
  /// unique ptr managing the same object.
  template <typename U>
  plUniquePtr<T>& operator=(plUniquePtr<U>&& other);

  /// \brief Same as calling 'Reset()'
  plUniquePtr<T>& operator=(std::nullptr_t);

  /// \brief Releases the managed object without destroying it. The unique ptr will be empty afterwards.
  T* Release();

  /// \brief Releases the managed object without destroying it. The unique ptr will be empty afterwards. Also returns the allocator that
  /// should be used to destroy the object.
  T* Release(plAllocator*& out_pAllocator);

  /// \brief Borrows the managed object. The unique ptr stays unmodified.
  T* Borrow() const;

  /// \brief Destroys the managed object and resets the unique ptr.
  void Clear();

  /// \brief Provides access to the managed object.
  T& operator*() const;

  /// \brief Provides access to the managed object.
  T* operator->() const;

  /// \brief Returns true if there is managed object and false if the unique ptr is empty.
  explicit operator bool() const;

  /// \brief Compares the unique ptr against another unique ptr.
  bool operator==(const plUniquePtr<T>& rhs) const;
  bool operator!=(const plUniquePtr<T>& rhs) const;
  bool operator<(const plUniquePtr<T>& rhs) const;
  bool operator<=(const plUniquePtr<T>& rhs) const;
  bool operator>(const plUniquePtr<T>& rhs) const;
  bool operator>=(const plUniquePtr<T>& rhs) const;

  /// \brief Compares the unique ptr against nullptr.
  bool operator==(std::nullptr_t) const;
  bool operator!=(std::nullptr_t) const;
  bool operator<(std::nullptr_t) const;
  bool operator<=(std::nullptr_t) const;
  bool operator>(std::nullptr_t) const;
  bool operator>=(std::nullptr_t) const;

private:
  template <typename U>
  friend class plUniquePtr;

  T* m_pInstance = nullptr;
  plAllocator* m_pAllocator = nullptr;
};

#include <Foundation/Types/Implementation/UniquePtr_inl.h>

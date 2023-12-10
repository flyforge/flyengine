#pragma once

/// \brief Operator to serialize plIAllocator::Stats objects.
PLASMA_FOUNDATION_DLL void operator<<(plStreamWriter& inout_stream, const plAllocatorBase::Stats& rhs);

/// \brief Operator to serialize plIAllocator::Stats objects.
PLASMA_FOUNDATION_DLL void operator>>(plStreamReader& inout_stream, plAllocatorBase::Stats& rhs);

struct plTime;

/// \brief Operator to serialize plTime objects.
PLASMA_FOUNDATION_DLL void operator<<(plStreamWriter& inout_stream, plTime value);

/// \brief Operator to serialize plTime objects.
PLASMA_FOUNDATION_DLL void operator>>(plStreamReader& inout_stream, plTime& ref_value);


class plUuid;

/// \brief Operator to serialize plUuid objects. [tested]
PLASMA_FOUNDATION_DLL void operator<<(plStreamWriter& inout_stream, const plUuid& value);

/// \brief Operator to serialize plUuid objects. [tested]
PLASMA_FOUNDATION_DLL void operator>>(plStreamReader& inout_stream, plUuid& ref_value);

class plHashedString;

/// \brief Operator to serialize plHashedString objects. [tested]
PLASMA_FOUNDATION_DLL void operator<<(plStreamWriter& inout_stream, const plHashedString& sValue);

/// \brief Operator to serialize plHashedString objects. [tested]
PLASMA_FOUNDATION_DLL void operator>>(plStreamReader& inout_stream, plHashedString& ref_sValue);

class plTempHashedString;

/// \brief Operator to serialize plHashedString objects.
PLASMA_FOUNDATION_DLL void operator<<(plStreamWriter& inout_stream, const plTempHashedString& sValue);

/// \brief Operator to serialize plHashedString objects.
PLASMA_FOUNDATION_DLL void operator>>(plStreamReader& inout_stream, plTempHashedString& ref_sValue);

class plVariant;

/// \brief Operator to serialize plVariant objects.
PLASMA_FOUNDATION_DLL void operator<<(plStreamWriter& inout_stream, const plVariant& value);

/// \brief Operator to serialize plVariant objects.
PLASMA_FOUNDATION_DLL void operator>>(plStreamReader& inout_stream, plVariant& ref_value);

class plTimestamp;

/// \brief Operator to serialize plTimestamp objects.
PLASMA_FOUNDATION_DLL void operator<<(plStreamWriter& inout_stream, plTimestamp value);

/// \brief Operator to serialize plTimestamp objects.
PLASMA_FOUNDATION_DLL void operator>>(plStreamReader& inout_stream, plTimestamp& ref_value);

struct plVarianceTypeFloat;

/// \brief Operator to serialize plTimestamp objects.
PLASMA_FOUNDATION_DLL void operator<<(plStreamWriter& inout_stream, const plVarianceTypeFloat& value);

/// \brief Operator to serialize plTimestamp objects.
PLASMA_FOUNDATION_DLL void operator>>(plStreamReader& inout_stream, plVarianceTypeFloat& ref_value);

struct plVarianceTypeTime;

/// \brief Operator to serialize plTimestamp objects.
PLASMA_FOUNDATION_DLL void operator<<(plStreamWriter& inout_stream, const plVarianceTypeTime& value);

/// \brief Operator to serialize plTimestamp objects.
PLASMA_FOUNDATION_DLL void operator>>(plStreamReader& inout_stream, plVarianceTypeTime& ref_value);

struct plVarianceTypeAngle;

/// \brief Operator to serialize plTimestamp objects.
PLASMA_FOUNDATION_DLL void operator<<(plStreamWriter& inout_stream, const plVarianceTypeAngle& value);

/// \brief Operator to serialize plTimestamp objects.
PLASMA_FOUNDATION_DLL void operator>>(plStreamReader& inout_stream, plVarianceTypeAngle& ref_value);

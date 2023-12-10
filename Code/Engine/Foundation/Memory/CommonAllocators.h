
#pragma once

/// \file

#include <Foundation/Memory/Allocator.h>

#include <Foundation/Memory/Policies/AlignedHeapAllocation.h>
#include <Foundation/Memory/Policies/GuardedAllocation.h>
#include <Foundation/Memory/Policies/HeapAllocation.h>
#include <Foundation/Memory/Policies/ProxyAllocation.h>


/// \brief Default heap allocator
using plAlignedHeapAllocator = plAllocator<plMemoryPolicies::plAlignedHeapAllocation>;

/// \brief Default heap allocator
using plHeapAllocator = plAllocator<plMemoryPolicies::plHeapAllocation>;

/// \brief Guarded allocator
using plGuardedAllocator = plAllocator<plMemoryPolicies::plGuardedAllocation>;

/// \brief Proxy allocator
using plProxyAllocator = plAllocator<plMemoryPolicies::plProxyAllocation>;

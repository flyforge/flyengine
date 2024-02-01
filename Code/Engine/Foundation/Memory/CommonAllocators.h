
#pragma once

/// \file

#include <Foundation/Memory/AllocatorWithPolicy.h>

#include <Foundation/Memory/Policies/AllocPolicyAlignedHeap.h>
#include <Foundation/Memory/Policies/AllocPolicyGuarding.h>
#include <Foundation/Memory/Policies/AllocPolicyHeap.h>
#include <Foundation/Memory/Policies/AllocPolicyProxy.h>


/// \brief Default heap allocator
using plAlignedHeapAllocator = plAllocatorWithPolicy<plAllocPolicyAlignedHeap>;

/// \brief Default heap allocator
using plHeapAllocator = plAllocatorWithPolicy<plAllocPolicyHeap>;

/// \brief Guarded allocator
using plGuardingAllocator = plAllocatorWithPolicy<plAllocPolicyGuarding>;

/// \brief Proxy allocator
using plProxyAllocator = plAllocatorWithPolicy<plAllocPolicyProxy>;

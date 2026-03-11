/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2026 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once

#include <unordered_map>
#include "api/replay/data_types.h"
#include "api/replay/rdcarray.h"
#include "api/replay/resourceid.h"

struct ResourceUsageEvent
{
  ResourceUsageEvent(ResourceId id, ResourceUsage u) : resId(id), usage(u) {}
  ResourceId resId;
  ResourceUsage usage;
};

struct ResourceUsageTracker
{
  typedef rdcarray<ResourceUsageEvent> ResourceUsages;

  void AddUsageAtEvent(uint32_t eid, const ResourceUsages &usages);
  void RemoveUsageFromEvent(uint32_t eid, const ResourceUsages &usages);
  rdcarray<EventUsage> GetUsage(ResourceId id) const;

private:
  typedef rdcarray<uint32_t> EventList;
  typedef rdcarray<uint32_t> IndexList;
  typedef rdcarray<ResourceId> ResourceIdList;
  typedef rdcarray<ResourceUsage> ResourceUsageList;

  // EID to indexes into usages storage
  std::unordered_map<uint32_t, IndexList> m_EIDs;
  // ResourceUsageEvent storage : the same usage pattern could be used by multiple different events
  // It is divided into two containers to remove the alignment padding storage
  // This could be a 4-byte index into a container that has ResourceId to save space
  rdcarray<ResourceIdList> m_Usages_ResIds;
  rdcarray<ResourceUsageList> m_Usages_ResUsages;

  // ResourceId to EIDs
  std::unordered_map<ResourceId, EventList> m_ResEvents;

  // Simple storage for testing verification
  std::unordered_map<ResourceId, rdcarray<EventUsage>> m_ResEventUsage;
};

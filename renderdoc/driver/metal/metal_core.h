/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2022-2024 Baldur Karlsson
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

#include "metal_common.h"

struct MetalInitParams
{
  MetalInitParams();
  void Set(MTL::Device *pDevice, ResourceId device);

  // update this when adding/removing members
  uint64_t GetSerialiseSize();

  // check if a frame capture section version is supported
  static bool IsSupportedVersion(uint64_t ver);
  static const uint64_t CurrentVersion = 0x1;

  ResourceId DeviceID;
};

struct MetalActionTreeNode
{
  MetalActionTreeNode() {}
  explicit MetalActionTreeNode(const ActionDescription &a) : action(a) {}
  ActionDescription action;
  rdcarray<MetalActionTreeNode> children;

  rdcarray<ResourceId> executedCmds;

  MetalActionTreeNode &operator=(const ActionDescription &a)
  {
    *this = MetalActionTreeNode(a);
    return *this;
  }

  void InsertAndUpdateIDs(const MetalActionTreeNode &child, uint32_t baseEventID, uint32_t baseDrawID)
  {
    children.reserve(child.children.size());
    for(size_t i = 0; i < child.children.size(); i++)
    {
      children.push_back(child.children[i]);
      children.back().UpdateIDs(baseEventID, baseDrawID);
    }
  }

  void UpdateIDs(uint32_t baseEventID, uint32_t baseDrawID)
  {
    action.eventId += baseEventID;
    action.actionId += baseDrawID;

    for(APIEvent &ev : action.events)
      ev.eventId += baseEventID;

    for(size_t i = 0; i < children.size(); i++)
      children[i].UpdateIDs(baseEventID, baseDrawID);
  }

  rdcarray<ActionDescription> Bake()
  {
    rdcarray<ActionDescription> ret;
    if(children.empty())
      return ret;

    ret.resize(children.size());
    for(size_t i = 0; i < children.size(); i++)
    {
      ret[i] = children[i].action;
      ret[i].children = children[i].Bake();
    }

    return ret;
  }
};

DECLARE_REFLECTION_STRUCT(MetalInitParams);

from typing import Dict, List, Tuple

import renderdoc as rd
import rdtest

class VK_Resource_Usage(rdtest.TestCase):
    demos_test_name = 'VK_Resource_Usage'
    resourceUsages: Dict[rd.ResourceId, List[rd.EventUsage]] = {}
    eids: List[int] = []
    markerStart = 0
    markerEnd = 0

    def add_action(self, action: rd.ActionDescription):
        self.eids.append(action.eventId)
        for c in action.children:
            self.add_action(c)
        for e in action.events:
            self.eids.append(e.eventId)

    def check_resource_usage(self, res: rd.ResourceDescription, expectedUsages: List[Tuple[int,rd.ResourceUsage]]):
        usages = self.resourceUsages[res.resourceId]
        if len(usages) == 1 and usages[0].eventId == 0 and usages[0].usage == rd.ResourceUsage.Unused:
            resUsages = usages
        else:
            resUsages = [u for u in usages if u.eventId >= self.markerStart and u.eventId <= self.markerEnd]
        rdtest.log.print(f"Resource '{res.name}' type:{res.type.name} {res.resourceId} usages:{len(resUsages)} expectedUsages:{len(expectedUsages)}")
        if len(resUsages) != len(expectedUsages):
            rdtest.log.print(f"Usages for Resource '{res.name}' {res.resourceId}")
            for u in resUsages:
                rdtest.log.print(f"EID:{u.eventId} usage:{u.usage.name}")
            rdtest.log.print(f"Expected Usages for Resource '{res.name}' {res.resourceId}")
            for u in expectedUsages:
                eid, usage = u
                rdtest.log.print(f"EID:{eid} usage:{usage.name}")
            raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} Incorrect resource usages count expected:{len(expectedUsages)} actual:{len(resUsages)}")
        for i, u in enumerate(resUsages):
            eid, usage = expectedUsages[i]
            if u.usage != usage:
                raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} EID:{u.eventId} Incorrect resource usage expected:{usage.name} actual:{u.usage.name}")
            if u.eventId != eid:
                raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} usage:{u.usage.name} Incorrect resource usage EID expected:{eid} actual:{u.eventId}")

    def check_capture(self):
        assert self.controller is not None
        # Cache the resource usage before running any replay i.e. without calling SetFrameEvent
        resources = self.controller.GetResources()
        for res in resources:
            self.resourceUsages[res.resourceId] = self.controller.GetUsage(res.resourceId)

        drawIndirectCount = self.find_action("Draw Indirect Count") is not None
        rdtest.log.print(f"Has Draw Indirect Count: {'Yes' if drawIndirectCount else 'No'}")

        nestedSecondaries = self.find_action("Nested Secondary Command Buffer") is not None
        rdtest.log.print(f"Has Nested Secondary Command Buffer: {'Yes' if nestedSecondaries else 'No'}")

        descBuffer = self.find_action("Descriptor Buffer") is not None
        rdtest.log.print(f"Has Descriptor Buffer: {'Yes' if descBuffer else 'No'}")

        meshShader = self.find_action("Mesh Shader") is not None
        rdtest.log.print(f"Has Mesh Shader: {'Yes' if meshShader else 'No'}")

        # Walk the actions finding all the Dispatch, Draw, DrawIndexed, DrawIndirect, DrawIndirectCount
        sdfile = self.controller.GetStructuredFile()
        actions = self.controller.GetRootActions().copy()

        drawEIDs: List[int] = []
        meshDispatchEIDs: List[int] = []
        indexedEIDs: List[int] = []
        indexedSpecialEIDs: List[int] = []
        dispatchEIDs: List[int] = []
        indirectEIDs: List[int] = []
        multiEIDs: List[int] = []
        indirectCountEIDs: List[int] = []
        submitEIDs: List[int] = []
        waitFencesEIDs: List[int] = []
        resetFencesEIDs: List[int] = []
        descSetDrawEIDs: List[int] = []
        descBufferDrawEIDs: List[int] = []
        cmdDrawIndirectEIDs: List[int] = []
        cmdMeshDispatchIndirectEIDs: List[int] = []
        descBufferDispatchEIDs : List[int] = []
        beginRenderPassEIDs : List[int] = []
        endRenderPassEIDs : List[int] = []
        fillBufferEIDs: List[int] = []
        bindPipelineEIDs : List[int] = []
        copyBufferEIDs: List[int] = []
        computeWriteDataEIDs : List[int] = []
        noDescriptorSetDrawEIDs : List[int] = []
        computeDescriptorSetDispatchEIDs : List[int] = []

        markerGraphics = 0
        markerGraphicsNoDescriptorSet = 0
        markerGraphicsDescriptorSet = 0
        markerGraphicsSecondaryCommandBuffer = 0
        markerIndirect = 0
        markerIndirectWriteIndirectDispatchData  = 0
        markerIndirectDispatchWriteIndirectDrawData = 0
        markerIndirectIndirectDraws = 0
        markerIndirectSecondaryCommandBuffer = 0
        markerIndirectSecondaryCommandBufferDispatchIndirect = 0
        markerIndirectSecondaryCommandBufferDrawIndirectSingle = 0
        markerIndirectDraws = 0
        markerIndirectEnd = 0
        markerNestedSecondaryCommandBuffer = 0
        markerNestedSecondaryCommandBufferDispatch = 0
        markerDescriptorBuffer = 0
        markerDescriptorBufferDraw = 0
        markerDescriptorBufferDispatch = 0
        markerBarrierCommandSubmitFence = 0
        markerCompute = 0
        markerComputeDescriptorSet = 0
        markerComputeSecondaryCommandBuffer = 0
        markerMeshShader = 0
        markerLooseEventsAfterIndirectDrawsDrawIndirectSingle = 0
        markerMultipleCommandBufferSubmits = 0
        markerMultipleSecondaryCommandBufferExecutes = 0

        self.markerStart = self.find_action("Resource Usage: Start").eventId
        self.markerEnd = self.find_action("Resource Usage: End").eventId
        markerIndirectWriteIndirectDispatchData = self.find_action("Indirect Write IndirectDispatch Data").eventId
        markerGraphics = self.find_action("Graphics").eventId
        markerGraphicsNoDescriptorSet = self.find_action("No Descriptor Set", markerGraphics).eventId
        markerGraphicsDescriptorSet = self.find_action("Descriptor Set", markerGraphicsNoDescriptorSet+1).eventId
        markerGraphicsSecondaryCommandBuffer = self.find_action("Secondary Command Buffer").eventId
        if nestedSecondaries:
            markerNestedSecondaryCommandBuffer = self.find_action("Nested Secondary Command Buffer").eventId
            markerNestedSecondaryCommandBufferDispatch = self.find_action("Dispatch", markerNestedSecondaryCommandBuffer).eventId
        if descBuffer:
            markerDescriptorBuffer = self.find_action("Descriptor Buffer").eventId
            markerDescriptorBufferDraw = self.find_action("Draw", markerDescriptorBuffer).eventId
            markerDescriptorBufferDispatch = self.find_action("Dispatch", markerDescriptorBuffer).eventId
        if meshShader:
            markerMeshShader = self.find_action("Mesh Shader").eventId
        markerBarrierCommandSubmitFence = self.find_action("Barrier Command Submit Fence").eventId
        markerIndirectDispatchWriteIndirectDrawData = self.find_action("Indirect Dispatch Write IndirectDraw Data").eventId
        markerIndirectSecondaryCommandBuffer = self.find_action("Secondary Command Buffer", markerIndirectDispatchWriteIndirectDrawData).eventId
        markerIndirectSecondaryCommandBufferDrawIndirectSingle = self.find_action("DrawIndirect: Single", markerIndirectSecondaryCommandBuffer).eventId
        markerIndirectDraws = self.find_action("Indirect Draws", markerIndirectDispatchWriteIndirectDrawData).eventId
        markerCompute = self.find_action("Compute").eventId
        markerComputeSecondaryCommandBuffer = self.find_action("Secondary Command Buffer", markerCompute).eventId
        markerLooseEventsAfterIndirectDraws = self.find_action("Loose Events After Indirect Draws").eventId
        markerLooseEventsAfterIndirectDrawsDrawIndirectSingle = self.find_action("DrawIndirect: Single", markerLooseEventsAfterIndirectDraws).eventId
        markerMultipleCommandBufferSubmits = self.find_action("Multiple Command Buffer Submits").eventId
        markerMultipleSecondaryCommandBufferExecutes = self.find_action("Multiple Secondary Command Buffer Executes").eventId

        while len(actions) > 0:
            action = actions.pop(0)
            actions += action.children
            flags = action.flags
            if flags & rd.ActionFlags.Drawcall:
                drawEIDs.append(action.eventId)
            if flags & rd.ActionFlags.Indexed:
                # Special case remove this from the indexed list
                # vkCmdDrawIndexedIndirect(1) => <3, 1>
                if action.customName.endswith("<3, 1>"):
                    indexedSpecialEIDs.append(action.eventId)
                else:
                    indexedEIDs.append(action.eventId)
            if flags & rd.ActionFlags.Dispatch:
                dispatchEIDs.append(action.eventId)
            if flags & rd.ActionFlags.Indirect:
                indirectEIDs.append(action.eventId)
                if "Count" in action.customName:
                    indirectCountEIDs.append(action.eventId)
            if flags & rd.ActionFlags.MeshDispatch:
                meshDispatchEIDs.append(action.eventId)
            if flags & rd.ActionFlags.MultiAction:
                multiEIDs.append(action.eventId)
                if "Count" in action.customName:
                    if not action.eventId in indirectCountEIDs:
                        indirectCountEIDs.append(action.eventId)
            if flags & rd.ActionFlags.PushMarker:
                parentName = action.parent.customName if action.parent is not None else None
                markerPath = parentName + "." + action.customName if parentName is not None else action.customName
                if not markerGraphicsNoDescriptorSet and markerPath == "Graphics.No Descriptor Set":
                    markerGraphicsNoDescriptorSet = action.eventId
                if not markerGraphicsDescriptorSet and markerPath == "Graphics.Descriptor Set":
                    markerGraphicsDescriptorSet = action.eventId
                if not markerIndirect and markerPath == "Indirect":
                    markerIndirect = action.eventId
                    markerIndirectEnd = action.children[-1].eventId
                if not markerComputeDescriptorSet and markerPath == "Compute.Descriptor Set":
                    markerComputeDescriptorSet = action.eventId
            if "vkCmdDrawIndirect(" in action.customName:
                cmdDrawIndirectEIDs.append(action.eventId)
            if "vkCmdDrawIndexedIndirect(" in action.customName:
                cmdDrawIndirectEIDs.append(action.eventId)
            if "vkCmdDrawMeshTasksIndirectEXT(" in action.customName:
                cmdMeshDispatchIndirectEIDs.append(action.eventId)
            for e in action.events:
                eventName = sdfile.chunks[e.chunkIndex].name
                if "vkQueueSubmit" in eventName:
                    submitEIDs.append(e.eventId)
                if "vkWaitForFences" in eventName:
                    waitFencesEIDs.append(e.eventId)
                if "vkResetFences" in eventName:
                    resetFencesEIDs.append(e.eventId)
                if "vkCmdCopyBuffer" in eventName:
                    copyBufferEIDs.append(e.eventId)
                if "vkCmdFillBuffer" in eventName:
                    fillBufferEIDs.append(e.eventId)
                if "vkCmdBeginRenderPass" in eventName:
                    beginRenderPassEIDs.append(e.eventId)
                if "vkCmdEndRenderPass" in eventName:
                    endRenderPassEIDs.append(e.eventId)
                if "vkCmdBindPipeline" in eventName:
                    bindPipelineEIDs.append(e.eventId)

        markerIndirectDispatchWriteIndirectDrawData = self.find_action("Indirect Dispatch Write IndirectDraw Data", markerIndirect).eventId
        markerIndirectIndirectDraws = self.find_action("Indirect Draws", markerIndirectDispatchWriteIndirectDrawData).eventId
        markerIndirectSecondaryCommandBuffer = self.find_action("Secondary Command Buffer", markerIndirect).eventId
        markerIndirectSecondaryCommandBufferDispatchIndirect = self.find_action("DispatchIndirect", markerIndirectSecondaryCommandBuffer).eventId

        descSetDrawEIDs += [eid for eid in drawEIDs 
                            if (eid > markerGraphicsDescriptorSet and eid < markerGraphicsSecondaryCommandBuffer) or 
                               (eid > markerIndirect and (not nestedSecondaries or eid < markerNestedSecondaryCommandBuffer))]
        descBufferDrawEIDs += [eid for eid in drawEIDs if descBuffer and eid > markerDescriptorBuffer]

        for eid in dispatchEIDs:
            if descBuffer and eid > markerDescriptorBuffer:
                descBufferDispatchEIDs.append(eid)

        # Dispatch before Compute region
        # All the dispatch and indirect 
        # Dipatch within "Indirect" region
        for eid in dispatchEIDs:
            if eid < markerCompute:
                computeWriteDataEIDs.append(eid)
            elif eid in indirectEIDs:
                computeWriteDataEIDs.append(eid)
            elif eid > markerIndirect and eid < markerIndirectEnd:
                computeWriteDataEIDs.append(eid)

        # Graphics->No Descriptor Set : Graphics -> Desscriptor Set
        # Graphics->Secondary Command Buffer -> Compute
        # Nested Secondary Command Buffer : Descriptor Buffer
        for eid in drawEIDs:
            if eid > markerGraphicsNoDescriptorSet and eid < markerGraphicsDescriptorSet:
                noDescriptorSetDrawEIDs.append(eid)
            if eid > markerGraphicsSecondaryCommandBuffer and eid < markerCompute:
                noDescriptorSetDrawEIDs.append(eid)
            if nestedSecondaries and eid > markerNestedSecondaryCommandBuffer and (not descBuffer or eid < markerDescriptorBuffer):
                noDescriptorSetDrawEIDs.append(eid)

        # Dispatches in Compute -> Indirect 
        # Dispatches in Nested Secondary Command Buffer -> Descriptor Buffer
        for eid in dispatchEIDs:
            if eid > markerComputeDescriptorSet and eid < markerIndirect:
                computeDescriptorSetDispatchEIDs.append(eid)
            if nestedSecondaries and eid > markerNestedSecondaryCommandBufferDispatch and (not descBuffer or eid < markerDescriptorBuffer):
                computeDescriptorSetDispatchEIDs.append(eid)

        drawEIDs.sort()
        indexedEIDs.sort()
        indexedSpecialEIDs.sort()
        dispatchEIDs.sort()
        multiEIDs.sort()
        indirectEIDs.sort()
        meshDispatchEIDs.sort()
        indirectCountEIDs.sort()
        submitEIDs.sort()
        waitFencesEIDs.sort()
        resetFencesEIDs.sort()
        descSetDrawEIDs.sort()
        descBufferDrawEIDs.sort()
        fillBufferEIDs.sort()
        copyBufferEIDs.sort()
        descBufferDispatchEIDs.sort()
        beginRenderPassEIDs.sort()
        endRenderPassEIDs.sort()
        bindPipelineEIDs.sort()
        computeWriteDataEIDs.sort()
        cmdDrawIndirectEIDs.sort()
        cmdMeshDispatchIndirectEIDs.sort()
        noDescriptorSetDrawEIDs.sort()
        computeDescriptorSetDispatchEIDs.sort()

        rdtest.log.print(f"markerStart: {self.markerStart}")
        rdtest.log.print(f"markerEnd: {self.markerEnd}")

        action = self.find_action("Draw")
        self.set_event(action.eventId, False)
        swapImage = self.controller.GetPipelineState().GetOutputTargets()[0].resource

        with rdtest.log.auto_section("Checking Resource Usage"):
            for res in resources:
                expectedUsage: List[Tuple[int,rd.ResourceUsage]] = []
                if res.type == rd.ResourceType.Device:
                    expectedUsage = []
                elif res.type == rd.ResourceType.Queue:
                    expectedUsage += [(eid,rd.ResourceUsage.Submit) for eid in submitEIDs if eid >= self.markerStart]
                elif res.type == rd.ResourceType.Pool:
                    expectedUsage = []
                elif res.type == rd.ResourceType.SwapchainImage:
                    # the swap chain image has usage, anything else does not
                    if res.resourceId == swapImage:
                        expectedUsage = [
                                        (self.markerStart + 1,rd.ResourceUsage.Barrier), 
                                        (self.markerStart + 1,rd.ResourceUsage.Discard), 
                                        (self.markerStart + 2,rd.ResourceUsage.Clear), 
                                        (self.markerStart + 3,rd.ResourceUsage.Barrier)] 
                        expectedUsage += [(eid,rd.ResourceUsage.ColorTarget) for eid in drawEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.ColorTarget) for eid in meshDispatchEIDs]
                        expectedUsage += [(markerBarrierCommandSubmitFence - 1, rd.ResourceUsage.Barrier)]
                    else:
                        expectedUsage = []
                elif res.type == rd.ResourceType.RenderPass:
                    if res.name == "Main Framebuffer":
                        expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in beginRenderPassEIDs]
                        for eid in endRenderPassEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.UnBind))
                        for eid in drawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.Framebuffer))
                        for eid in meshDispatchEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.Framebuffer))
                    if res.name == "Main Render Pass":
                        for eid in beginRenderPassEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.Bind))
                        for eid in endRenderPassEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.UnBind))
                        for eid in drawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.RenderPass))
                        for eid in meshDispatchEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.RenderPass))
                elif res.type == rd.ResourceType.Sync:
                    expectedUsage = []
                    if res.name.startswith("Autotesting renderEndSemaphore"):
                        expectedUsage = [(344+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Wait)]
                    elif res.name.startswith("Autotesting fence"):
                        expectedUsage = [(2,rd.ResourceUsage.Wait), 
                                         (3,rd.ResourceUsage.Reset)]
                    elif res.name == "Barrier Command Submit Fence":
                        for eid in waitFencesEIDs:
                            if eid > markerBarrierCommandSubmitFence:
                                expectedUsage.append((eid,rd.ResourceUsage.Wait))
                        for eid in resetFencesEIDs:
                            if eid > markerBarrierCommandSubmitFence:
                                expectedUsage.append((eid,rd.ResourceUsage.Reset))
                        for eid in submitEIDs:
                            if eid > markerBarrierCommandSubmitFence and eid < waitFencesEIDs[-1]:
                                expectedUsage.append((eid,rd.ResourceUsage.Wait))
                    elif res.name.startswith("Fence "):
                        # Ignore anonymous fences
                        continue
                    else:
                        expectedUsage = []
                elif res.type == rd.ResourceType.View:
                    # Ignore swapchain image views
                    if res.name.startswith("Main Swapchain ImageView"):
                        continue
                    if res.name == "Offscreen Image RTV":
                        for eid in descSetDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.ImageView))
                        for eid in descBufferDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.ImageView))
                elif res.type == rd.ResourceType.Memory:
                    expectedUsage = []
                elif res.type == rd.ResourceType.ShaderBinding:
                    if res.name == "Descriptor Set Pipeline Layout":
                        for eid in descSetDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.PipelineLayout))
                    elif res.name == "Compute Descriptor Set Pipeline Layout":
                        for eid in computeDescriptorSetDispatchEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.PipelineLayout))
                    elif res.name == "No Descriptor Set Pipeline Layout":
                        for eid in noDescriptorSetDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.PipelineLayout))
                    elif res.name == "Compute WriteData Pipeline Layout":
                        for eid in computeWriteDataEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.PipelineLayout))
                    elif res.name == "Compute Descriptor Buffer Pipeline Layout":
                        for eid in descBufferDispatchEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.PipelineLayout))
                    elif res.name == "Descriptor Buffer Pipeline Layout":
                        for eid in descBufferDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.PipelineLayout))
                    elif res.name == "Mesh Shader Pipeline Layout":
                        if meshShader:
                            for eid in meshDispatchEIDs:
                                expectedUsage.append((eid,rd.ResourceUsage.PipelineLayout))
                    elif res.name == "Descriptor Buffer Layout":
                        for eid in descBufferDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.DescriptorSetLayout))
                        for eid in descBufferDispatchEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.DescriptorSetLayout))
                    elif res.name == "Descriptor Set Layout":
                        for eid in descSetDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.DescriptorSetLayout))
                    elif res.name == "Compute Descriptor Set Layout":
                        for eid in computeDescriptorSetDispatchEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.DescriptorSetLayout))
                    elif res.name == "Compute WriteData Descriptor Set Layout":
                        for eid in computeWriteDataEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.DescriptorSetLayout))
                elif res.type == rd.ResourceType.Shader:
                    if res.name == "Descriptor Vertex Shader":
                        for eid in descSetDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.VS_Shader))
                        for eid in descBufferDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.VS_Shader))
                    elif res.name == "Descriptor Pixel Shader":
                        for eid in descSetDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.PS_Shader))
                        for eid in descBufferDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.PS_Shader))
                    elif res.name == "Descriptor Buffer Vertex Shader":
                        for eid in drawEIDs:
                            if descBuffer and eid > markerDescriptorBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.VS_Shader))
                    elif res.name == "Descriptor Buffer Pixel Shader":
                        for eid in drawEIDs:
                            if descBuffer and eid > markerDescriptorBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.PS_Shader))
                    elif res.name == "Mesh Mesh Shader":
                        for eid in meshDispatchEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.MS_Shader))
                    elif res.name == "Mesh Pixel Shader":
                        for eid in meshDispatchEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.PS_Shader))
                    elif res.name == "Descriptor Set Compute Shader":
                        for eid in computeDescriptorSetDispatchEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.CS_Shader))
                    elif res.name == "Descriptor Buffer Compute Shader":
                        for eid in dispatchEIDs:
                            if descBuffer and eid > markerDescriptorBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.CS_Shader))
                    elif res.name == "Default Vertex Shader":
                        for eid in noDescriptorSetDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.VS_Shader))
                    elif res.name == "Default Pixel Shader":
                        for eid in noDescriptorSetDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.PS_Shader))
                    elif res.name == "WriteData Compute Shader":
                        for eid in computeWriteDataEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.CS_Shader))
                elif res.type == rd.ResourceType.PipelineState:
                    if res.name == "Compute Descriptor Set Pipeline":
                        for eid in computeDescriptorSetDispatchEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.Pipeline))
                        for eid in bindPipelineEIDs:
                            if eid > markerComputeDescriptorSet and eid < markerIndirect:
                                expectedUsage.append((eid,rd.ResourceUsage.Bind))
                            elif eid > markerIndirectDispatchWriteIndirectDrawData and eid < markerIndirectDraws:
                                expectedUsage.append((eid,rd.ResourceUsage.UnBind))
                            elif nestedSecondaries and eid > markerNestedSecondaryCommandBufferDispatch and (not descBuffer or eid < markerDescriptorBuffer):
                                expectedUsage.append((eid,rd.ResourceUsage.Bind))
                    elif res.name == "Compute Descriptor Buffer Pipeline":
                        if descBuffer:
                            for eid in dispatchEIDs:
                                if eid > markerDescriptorBuffer:
                                    expectedUsage.append((eid,rd.ResourceUsage.Pipeline))
                            for eid in bindPipelineEIDs:
                                if eid > markerDescriptorBufferDispatch and meshShader and eid < markerMeshShader:
                                    expectedUsage.append((eid,rd.ResourceUsage.Bind))
                    elif res.name == "Descriptor Buffer Pipeline":
                        if descBuffer:
                            for eid in drawEIDs:
                                if eid > markerDescriptorBuffer:
                                    expectedUsage.append((eid,rd.ResourceUsage.Pipeline))
                            for eid in bindPipelineEIDs:
                                if eid > markerDescriptorBufferDraw and eid < markerDescriptorBufferDispatch:
                                    expectedUsage.append((eid,rd.ResourceUsage.Bind))
                                if meshShader and eid > markerMeshShader:
                                    expectedUsage.append((eid,rd.ResourceUsage.UnBind))
                    elif res.name == "Mesh Shader Pipeline":
                        if meshShader:
                            for eid in bindPipelineEIDs:
                                if eid > markerMeshShader:
                                    expectedUsage.append((eid,rd.ResourceUsage.Bind))
                            for eid in meshDispatchEIDs:
                                expectedUsage.append((eid,rd.ResourceUsage.Pipeline))
                    elif res.name == "Descriptor Set Pipeline":
                        # Bind: First pipeline bind in Graphics -> Descriptor Set
                        for eid in bindPipelineEIDs:
                            if eid > markerGraphicsDescriptorSet:
                                expectedUsage.append((eid,rd.ResourceUsage.Bind))
                                break
                        # UnBind: First pipeline bind in Indirect -> Indirect Draws
                        # Bind: First pipeline bind in Indirect -> Indirect Draws
                        for eid in bindPipelineEIDs:
                            if eid > markerIndirectDraws:
                                expectedUsage.append((eid,rd.ResourceUsage.UnBind))
                                expectedUsage.append((eid,rd.ResourceUsage.Bind))
                                break
                        # Bind : First pipeline bind in Indirect -> Secondary Command Buffer -> DrawIndirect: Single
                        for eid in bindPipelineEIDs:
                            if eid > markerIndirectSecondaryCommandBufferDrawIndirectSingle:
                                expectedUsage.append((eid,rd.ResourceUsage.Bind))
                                break
                        # This is an RD bug the RenderState should be reset in the primary when the secondary command buffer finishes
                        # UnBind : First pipeline bind in Loose Events After Indirect Draws -> DrawIndirect: Single
                        # Bind : First pipeline bind in Loose Events After Indirect Draws -> DrawIndirect: Single
                        for eid in bindPipelineEIDs:
                            if eid > markerLooseEventsAfterIndirectDrawsDrawIndirectSingle:
                                expectedUsage.append((eid,rd.ResourceUsage.UnBind))
                                expectedUsage.append((eid,rd.ResourceUsage.Bind))
                                break
                        for eid in descSetDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.Pipeline))
                    elif res.name == "No Descriptor Set Pipeline":
                        # Bind: First pipeline bind in Graphics 
                        for eid in bindPipelineEIDs:
                            if eid > markerGraphicsNoDescriptorSet:
                                expectedUsage.append((eid,rd.ResourceUsage.Bind))
                                break
                        # UnBind: First pipeline bind in Graphics -> Descriptor Set
                        for eid in bindPipelineEIDs:
                            if eid > markerGraphicsDescriptorSet:
                                expectedUsage.append((eid,rd.ResourceUsage.UnBind))
                                break
                        # Bind: all pipeline bind in Graphics -> Secondary Command Buffer : Compute
                        for eid in bindPipelineEIDs:
                            if eid > markerGraphicsSecondaryCommandBuffer and eid < markerCompute:
                                expectedUsage.append((eid,rd.ResourceUsage.Bind))
                        # Bind: First pipeline bind in Nested Secondary Command Buffer
                        for eid in bindPipelineEIDs:
                            if nestedSecondaries and eid > markerNestedSecondaryCommandBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.Bind))
                                break
                        for eid in noDescriptorSetDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.Pipeline))
                    elif res.name == "Compute WriteData Pipeline":
                        # Bind: First pipeline bind 
                        expectedUsage.append((bindPipelineEIDs[0],rd.ResourceUsage.Bind))
                        # UnBind : First bind after Compute->Descriptor Set
                        for eid in bindPipelineEIDs:
                            if eid > markerComputeDescriptorSet:
                                expectedUsage.append((eid,rd.ResourceUsage.UnBind))
                                break
                        # Bind : First bind after Indirect->Indirect Dispatch Write IndirectDraw Data
                        for eid in bindPipelineEIDs:
                            if eid > markerIndirectDispatchWriteIndirectDrawData:
                                expectedUsage.append((eid,rd.ResourceUsage.Bind))
                                break
                        # Bind : First bind after Indirect->Secondary Command Buffer
                        for eid in bindPipelineEIDs:
                            if eid > markerIndirectSecondaryCommandBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.Bind))
                                break
                        for eid in computeWriteDataEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.Pipeline))
                elif res.type == rd.ResourceType.Buffer:
                    if res.name == "Vertex Buffer":
                        expectedUsage += [(eid,rd.ResourceUsage.VertexBuffer) for eid in drawEIDs]
                    if res.name == "Index Buffer":
                        expectedUsage += [(eid,rd.ResourceUsage.IndexBuffer) for eid in indexedEIDs]
                    if res.name == "Compute Buffer In":
                        # All dispatches within "Compute" region
                        # All dispatches within "Nested Secondary Command Buffer" region
                        expectedUsage += [(eid,rd.ResourceUsage.CS_Constants) for eid in dispatchEIDs 
                                          if (eid > markerComputeDescriptorSet and eid < markerIndirect) or 
                                             (nestedSecondaries and eid > markerNestedSecondaryCommandBuffer)]
                    if res.name == "Compute Buffer Out":
                        # All dispatches within "Compute" region
                        # All dispatches within "Nested Secondary Command Buffer" region
                        expectedUsage += [(eid,rd.ResourceUsage.CS_RWResource) for eid in dispatchEIDs 
                                          if (eid > markerComputeDescriptorSet and eid < markerIndirect) or 
                                             (nestedSecondaries and eid > markerNestedSecondaryCommandBuffer)]
                    if res.name == "Indirect Data":
                        expectedUsage += [(markerIndirectWriteIndirectDispatchData + 1,rd.ResourceUsage.Barrier),
                                        (markerIndirectWriteIndirectDispatchData + 2,rd.ResourceUsage.Clear),
                                        (markerIndirectWriteIndirectDispatchData + 3,rd.ResourceUsage.Barrier)]

                        expectedUsage += [(markerIndirectSecondaryCommandBuffer - 2,rd.ResourceUsage.Barrier),
                                        (markerIndirectSecondaryCommandBuffer + 3,rd.ResourceUsage.Barrier),
                                        (markerIndirectSecondaryCommandBuffer + 5,rd.ResourceUsage.Barrier)]
                        expectedUsage += [(markerIndirectIndirectDraws - 2,rd.ResourceUsage.Barrier)]

                        expectedUsage += [(markerIndirectSecondaryCommandBufferDispatchIndirect - 1,rd.ResourceUsage.Barrier),
                                        (markerIndirectSecondaryCommandBufferDispatchIndirect + 3,rd.ResourceUsage.Barrier),
                                        (markerIndirectSecondaryCommandBufferDispatchIndirect + 5,rd.ResourceUsage.Barrier)]

                        for eid in dispatchEIDs:
                            if eid in indirectEIDs:
                                continue
                            if eid in indirectCountEIDs:
                                continue
                            if eid in multiEIDs:
                                continue
                            if eid in indexedSpecialEIDs:
                                continue
                            if eid > markerIndirectWriteIndirectDispatchData and eid < markerGraphics:
                                expectedUsage.append((eid,rd.ResourceUsage.CS_RWResource))
                                expectedUsage.append((eid+1,rd.ResourceUsage.Barrier))
                            if eid > markerComputeSecondaryCommandBuffer and eid < markerIndirect:
                                expectedUsage.append((eid+1,rd.ResourceUsage.Barrier))
                            if eid > markerIndirectSecondaryCommandBuffer and eid < markerIndirectEnd:
                                    expectedUsage.append((eid,rd.ResourceUsage.CS_RWResource))
                        for eid in cmdDrawIndirectEIDs:
                            if eid in indirectCountEIDs:
                                continue
                            if eid in multiEIDs:
                                continue
                            if eid in indexedSpecialEIDs:
                                continue
                            if eid > markerIndirectIndirectDraws and eid < markerIndirectEnd:
                                expectedUsage.append((eid,rd.ResourceUsage.Indirect))
                            if eid > markerLooseEventsAfterIndirectDrawsDrawIndirectSingle and eid < markerDescriptorBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.Indirect))
                        expectedUsage += [(eid,rd.ResourceUsage.Clear) for eid in fillBufferEIDs 
                                          if eid > markerIndirect and eid < markerIndirectEnd]
                        expectedUsage += [(eid,rd.ResourceUsage.CS_RWResource) for eid in indirectEIDs if eid in dispatchEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.Indirect) for eid in indirectEIDs if eid in dispatchEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.Indirect) for eid in multiEIDs if not eid in indirectCountEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.Indirect) for eid in indirectCountEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.Indirect) for eid in indirectCountEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.IndexBuffer) for eid in indexedSpecialEIDs if not eid in indirectCountEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.Indirect) for eid in indexedSpecialEIDs if not eid in indirectCountEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.Indirect) for eid in cmdMeshDispatchIndirectEIDs if not eid in multiEIDs]
                        for eid in endRenderPassEIDs:
                            if eid > markerLooseEventsAfterIndirectDraws:
                                expectedUsage.append((eid+2,rd.ResourceUsage.Barrier))
                                break
                        for eid in dispatchEIDs:
                            if nestedSecondaries and eid > markerNestedSecondaryCommandBufferDispatch:
                                expectedUsage.append((eid + 1,rd.ResourceUsage.Barrier))
                                break
                    if res.name == "Barrier Buffer":
                        expectedUsage += [(eid,rd.ResourceUsage.Barrier) 
                                for eid in range(markerMultipleCommandBufferSubmits + 1, markerMultipleCommandBufferSubmits + 1 + 10*8, 8)]
                    if res.name == "Barrier2 Buffer":
                        expectedUsage += [(eid,rd.ResourceUsage.Barrier) 
                                for eid in range(markerMultipleSecondaryCommandBufferExecutes + 3, markerMultipleSecondaryCommandBufferExecutes + 3 + 4*5, 5)]
                    if res.name == "Descriptor Buffer":
                        first = True
                        for eid in copyBufferEIDs:
                            if eid > markerLooseEventsAfterIndirectDraws:
                                expectedUsage.append((eid-1,rd.ResourceUsage.Barrier))
                                if first:
                                    expectedUsage.append((eid,rd.ResourceUsage.CopySrc))
                                    first = False
                                else:
                                    expectedUsage.append((eid,rd.ResourceUsage.CopyDst))
                        for eid in fillBufferEIDs:
                            if eid > markerLooseEventsAfterIndirectDraws:
                                expectedUsage.append((eid-1,rd.ResourceUsage.Barrier))
                                expectedUsage.append((eid,rd.ResourceUsage.Clear))
                                break
                        for eid in drawEIDs:
                            if eid > markerDescriptorBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.DescriptorBuffer))
                        for eid in dispatchEIDs:
                            if eid > markerDescriptorBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.DescriptorBuffer))
                        for eid in meshDispatchEIDs:
                            if eid > markerDescriptorBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.DescriptorBuffer))
                    if res.name == "Descriptor Backup Buffer":
                        first = True
                        for eid in copyBufferEIDs:
                            if eid > markerLooseEventsAfterIndirectDraws:
                                expectedUsage.append((eid-1,rd.ResourceUsage.Barrier))
                                if first:
                                    expectedUsage.append((eid,rd.ResourceUsage.CopyDst))
                                    first = False
                                else:
                                    expectedUsage.append((eid,rd.ResourceUsage.CopySrc))
                                    break
                elif res.type == rd.ResourceType.Texture:
                    if res.name == "Offscreen MSAA Image":
                        expectedUsage = [(self.markerStart + 6,rd.ResourceUsage.Barrier), 
                                        (self.markerStart + 6,rd.ResourceUsage.Discard), 
                                        (self.markerStart + 7,rd.ResourceUsage.Clear)]
                    if res.name == "Offscreen Image":
                        expectedUsage = [(self.markerStart + 4,rd.ResourceUsage.Barrier), 
                                        (self.markerStart + 4,rd.ResourceUsage.Discard), 
                                        (self.markerStart + 5,rd.ResourceUsage.Clear)] 
                        expectedUsage += [(eid,rd.ResourceUsage.PS_Resource) for eid in descSetDrawEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.PS_Resource) for eid in descBufferDrawEIDs]
                elif res.type == rd.ResourceType.CommandBuffer:
                    expectedUsage = [(0,rd.ResourceUsage.Unused)]
                    # TODO: need to pick carefully which command buffer to validate
                    # TODO: check Baked Command Buffer and Command Buffer
                    continue
                elif res.type == rd.ResourceType.DescriptorStore:
                    if res.name == "Descriptor Set":
                        for eid in descSetDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.DescriptorSet))
                    if res.name == "Compute Descriptor Set":
                        # All dispatches within "Compute" region
                        # All dispatches within "Nested Secondary Command Buffer" region
                        for eid in dispatchEIDs:
                            if eid > markerComputeDescriptorSet and eid < markerIndirect:
                                expectedUsage.append((eid,rd.ResourceUsage.DescriptorSet))
                            elif nestedSecondaries and eid > markerNestedSecondaryCommandBuffer and (not descBuffer or eid < markerDescriptorBuffer):
                                expectedUsage.append((eid,rd.ResourceUsage.DescriptorSet))
                    if res.name == "Compute WriteData Descriptor Set":
                        for eid in computeWriteDataEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.DescriptorSet))
                elif res.type == rd.ResourceType.Sampler:
                    for eid in descSetDrawEIDs:
                        expectedUsage.append((eid,rd.ResourceUsage.Sampler))
                    for eid in descBufferDrawEIDs:
                        expectedUsage.append((eid,rd.ResourceUsage.Sampler))
                else:
                    raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} Unexpected resource type {res.type.name}")
                expectedUsage.sort(key=lambda x: x[0])
                self.check_resource_usage(res, expectedUsage)

        actions = self.controller.GetRootActions()
        for a in actions:
            self.add_action(a)

        # Select every event of the resource usage to ensure the EID is valid
        with rdtest.log.auto_section("Checking Resource Usage Events can be replayed"):
            for res in resources:
                rdtest.log.print(f"Resource '{res.name}' type:{res.type.name} {res.resourceId}")
                usages = self.resourceUsages[res.resourceId]
                for u in usages:
                    eid = u.eventId
                    if eid == 0:
                        continue
                    self.set_event(eid, True)
                    if eid not in self.eids:
                        raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} Missing EID:{eid}")
        
        with rdtest.log.auto_section("Checking Indirect Action Names"):
            if not self.check_indirect_action_name_consistency(self.controller):
                raise rdtest.TestFailureException("Indirect action parameters do not match its event parameters")

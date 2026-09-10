import renderdoc as rd
import rdtest

class VK_Resource_Usage(rdtest.TestCase):
    demos_test_name = 'VK_Resource_Usage'
    resourceUsages = {}
    eids = []

    def add_action(self, action: rd.ActionDescription):
        self.eids.append(action.eventId)
        for c in action.children:
            self.add_action(c)
        for e in action.events:
            self.eids.append(e.eventId)

    def check_resource_usage(self, res: rd.ResourceDescription, expectedUsages=[]):
        usages = self.resourceUsages[res.resourceId]
        if len(usages) != len(expectedUsages):
            for u in usages:
                rdtest.log.print(f"Resource '{res.name}' {res.resourceId} usage EID:{u.eventId} usage:{u.usage.name}")
            for u in expectedUsages:
                eid, usage = u
                rdtest.log.print(f"Resource '{res.name}' {res.resourceId} usage EID:{eid} usage:{usage.name}")
            raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} Incorrect resource usages count expected:{len(expectedUsages)} actual:{len(usages)}")
        for i, u in enumerate(usages):
            eid, usage = expectedUsages[i]
            if u.usage != usage:
                raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} EID:{u.eventId} Incorrect resource usage expected:{usage.name} actual:{u.usage.name}")
            if u.eventId != eid:
                raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} usage:{u.usage.name} Incorrect resource usage EID expected:{eid} actual:{u.eventId}")

    def check_capture(self):
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

        countDrawIndirectCount = 30 if drawIndirectCount else 0
        countNested = 39 if nestedSecondaries else 0
        countDescBufferCopy = 10 if descBuffer else 0
        countDescBuffer = 21 if descBuffer else 0
        countDescBuffer += countDescBufferCopy
        countMeshShader = 33 if meshShader else 0

        # Walk the actions finding all the Dispatch, Draw, DrawIndexed, DrawIndirect, DrawIndirectCount
        sdfile = self.controller.GetStructuredFile()
        actions = self.controller.GetRootActions().copy()

        drawEIDs = []
        meshDispatchEIDs = []
        indexedEIDs = []
        indexedSpecialEIDs = []
        dispatchEIDs = []
        indirectEIDs = []
        multiEIDs = []
        indirectCountEIDs = []
        submitEIDs = []
        waitFencesEIDs = []
        resetFencesEIDs = []
        descSetDrawEIDs = []
        descBufferDrawEIDs = []
        beginRenderPassEIDs = []
        endRenderPassEIDs = []

        markerGraphicsDescriptorSet = 0
        markerGraphicsSecondaryCommandBuffer = 0
        markerIndirect = 0
        markerIndirectEnd = 0
        markerNestedSecondaryCommandBuffer = 0
        markerDescriptorBuffer = 0
        markerBarrierCommandSubmitFence = 0
        markerCompute = 0
        markerComputeDescriptorSet = 0

        markerGraphicsSecondaryCommandBuffer = self.find_action("Secondary Command Buffer").eventId
        if nestedSecondaries:
            markerNestedSecondaryCommandBuffer = self.find_action("Nested Secondary Command Buffer").eventId
        if descBuffer:
            markerDescriptorBuffer = self.find_action("Descriptor Buffer").eventId
        markerBarrierCommandSubmitFence = self.find_action("Barrier Command Submit Fence").eventId

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
            if flags & rd.ActionFlags.PushMarker:
                parentName = action.parent.customName if action.parent is not None else None
                markerPath = parentName + "." + action.customName if parentName is not None else action.customName
                if not markerGraphicsDescriptorSet and markerPath == "Graphics.Descriptor Set":
                    rdtest.log.print(f"{action.parent.customName} {action.parent.eventId} {action.customName} {action.eventId}")
                    markerGraphicsDescriptorSet = action.eventId
                if not markerIndirect and markerPath == "Indirect":
                    markerIndirect = action.eventId
                    markerIndirectEnd = action.children[-1].eventId
                if not markerCompute and markerPath == "Compute":
                    markerCompute = action.eventId
                if not markerComputeDescriptorSet and markerPath == "Compute.Descriptor Set":
                    markerComputeDescriptorSet = action.eventId
            for e in action.events:
                eventName = sdfile.chunks[e.chunkIndex].name
                if "vkQueueSubmit" in eventName:
                    submitEIDs.append(e.eventId)
                if "vkWaitForFences" in eventName:
                    waitFencesEIDs.append(e.eventId)
                if "vkResetFences" in eventName:
                    resetFencesEIDs.append(e.eventId)
                if "vkCmdBeginRenderPass" in eventName:
                    beginRenderPassEIDs.append(e.eventId)
                if "vkCmdEndRenderPass" in eventName:
                    endRenderPassEIDs.append(e.eventId)

        for eid in drawEIDs:
            if eid > markerGraphicsDescriptorSet and eid < markerGraphicsSecondaryCommandBuffer:
                descSetDrawEIDs.append(eid)
            elif eid > markerIndirect and (not nestedSecondaries or eid < markerNestedSecondaryCommandBuffer):
                descSetDrawEIDs.append(eid)
            elif descBuffer and eid > markerDescriptorBuffer:
                descBufferDrawEIDs.append(eid)

        drawEIDs.sort()
        indexedEIDs.sort()
        indexedSpecialEIDs.sort()
        dispatchEIDs.sort()
        indirectEIDs.sort()
        meshDispatchEIDs.sort()
        multiEIDs.sort()
        indirectCountEIDs.sort()
        submitEIDs.sort()
        waitFencesEIDs.sort()
        resetFencesEIDs.sort()
        descSetDrawEIDs.sort()
        descBufferDrawEIDs.sort()
        beginRenderPassEIDs.sort()
        endRenderPassEIDs.sort()

        action = self.find_action("Draw")
        self.controller.SetFrameEvent(action.eventId, False)
        swapImage = self.controller.GetPipelineState().GetOutputTargets()[0].resource

        with rdtest.log.auto_section("Checking Resource Usage"):
            for res in resources:
                expectedUsage = []
                if res.type == rd.ResourceType.Device:
                    expectedUsage = []
                elif res.type == rd.ResourceType.Queue:
                    for eid in submitEIDs:
                        expectedUsage.append((eid,rd.ResourceUsage.Submit))
                elif res.type == rd.ResourceType.Pool:
                    expectedUsage = []
                elif res.type == rd.ResourceType.SwapchainImage:
                    # the swap chain image has usage, anything else does not
                    if res.resourceId == swapImage:
                        expectedUsage = [
                                        (7,rd.ResourceUsage.Barrier), 
                                        (7,rd.ResourceUsage.Discard), 
                                        (8,rd.ResourceUsage.Clear), 
                                        (9,rd.ResourceUsage.Barrier)] 
                        for eid in drawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.ColorTarget))
                        for eid in meshDispatchEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.ColorTarget))
                        expectedUsage += [
                                    (236+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                    (344+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.CopyDst)]
                    else:
                        expectedUsage = []
                elif res.type == rd.ResourceType.RenderPass:
                    if res.name == "Main Framebuffer":
                        for eid in beginRenderPassEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.Bind))
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
                        for eid in dispatchEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.PipelineLayout))
                    else:
                        continue
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
                        # All dispatches within "Compute" region
                        # All dispatches within "Nested Secondary Command Buffer" region
                        for eid in dispatchEIDs:
                            if eid > markerComputeDescriptorSet and eid < markerIndirect:
                                expectedUsage.append((eid,rd.ResourceUsage.CS_Shader))
                            elif nestedSecondaries and eid > markerNestedSecondaryCommandBuffer and (not descBuffer or eid < markerDescriptorBuffer):
                                expectedUsage.append((eid,rd.ResourceUsage.CS_Shader))
                    elif res.name == "Descriptor Buffer Compute Shader":
                        for eid in dispatchEIDs:
                            if descBuffer and eid > markerDescriptorBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.CS_Shader))
                    else:
                        continue
                elif res.type == rd.ResourceType.PipelineState:
                    if res.name == "Compute Descriptor Set Pipeline":
                        # All dispatches within "Compute" region
                        for eid in dispatchEIDs:
                            if eid > markerComputeDescriptorSet and eid < markerIndirect:
                                expectedUsage.append((eid,rd.ResourceUsage.Pipeline))
                            elif nestedSecondaries and eid > markerNestedSecondaryCommandBuffer and (not descBuffer or eid < markerDescriptorBuffer):
                                expectedUsage.append((eid,rd.ResourceUsage.Pipeline))
                    elif res.name == "Compute Descriptor Buffer Pipeline":
                        for eid in dispatchEIDs:
                            if descBuffer and eid > markerDescriptorBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.Pipeline))
                    elif res.name == "Descriptor Buffer Pipeline":
                        for eid in drawEIDs:
                            if descBuffer and eid > markerDescriptorBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.Pipeline))
                    elif res.name == "Descriptor Set":
                        for eid in descSetDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.Pipeline))
                    else:
                        continue
                elif res.type == rd.ResourceType.Buffer:
                    if res.name == "Vertex Buffer":
                        for eid in drawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.VertexBuffer))
                    if res.name == "Index Buffer":
                        for eid in indexedEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.IndexBuffer))
                    if res.name == "Compute Buffer In":
                        # All dispatches within "Compute" region
                        # All dispatches within "Nested Secondary Command Buffer" region
                        for eid in dispatchEIDs:
                            if eid > markerComputeDescriptorSet and eid < markerIndirect:
                                expectedUsage.append((eid,rd.ResourceUsage.CS_Constants))
                            elif nestedSecondaries and eid > markerNestedSecondaryCommandBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.CS_Constants))
                    if res.name == "Compute Buffer Out":
                        # All dispatches within "Compute" region
                        # All dispatches within "Nested Secondary Command Buffer" region
                        for eid in dispatchEIDs:
                            if eid > markerComputeDescriptorSet and eid < markerIndirect:
                                expectedUsage.append((eid,rd.ResourceUsage.CS_RWResource))
                            elif nestedSecondaries and eid > markerNestedSecondaryCommandBuffer:
                                expectedUsage.append((eid,rd.ResourceUsage.CS_RWResource))
                    if res.name == "Indirect Data":
                        expectedUsage += [(15,rd.ResourceUsage.Barrier),
                                        (16,rd.ResourceUsage.Clear),
                                        (17,rd.ResourceUsage.Barrier),
                                        (21,rd.ResourceUsage.CS_RWResource),
                                        (22,rd.ResourceUsage.Barrier),
                                        (98,rd.ResourceUsage.Barrier),
                                        (110,rd.ResourceUsage.Barrier),
                                        (121,rd.ResourceUsage.Indirect),
                                        (125,rd.ResourceUsage.Indirect),
                                        (143,rd.ResourceUsage.Indirect),
                                        (145,rd.ResourceUsage.Barrier),
                                        (150,rd.ResourceUsage.Barrier),
                                        (151,rd.ResourceUsage.Clear),
                                        (152,rd.ResourceUsage.Barrier),
                                        (156,rd.ResourceUsage.CS_RWResource),
                                        (158,rd.ResourceUsage.Barrier),
                                        (162,rd.ResourceUsage.Barrier),
                                        (164,rd.ResourceUsage.Barrier),
                                        (178,rd.ResourceUsage.Indirect),
                                        (209,rd.ResourceUsage.Indirect)]
                        if drawIndirectCount:
                            expectedUsage += [
                                        (248,rd.ResourceUsage.Indirect),
                                        (255,rd.ResourceUsage.Indirect)]
                        expectedUsage += [(232+countDrawIndirectCount,rd.ResourceUsage.Barrier)]
                        if nestedSecondaries:
                            expectedUsage += [
                                        (269+countDrawIndirectCount+countDescBufferCopy,rd.ResourceUsage.Barrier)]
                        if meshShader:
                            expectedUsage += [
                                        (250+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.Indirect),
                                        (253+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.Indirect),
                                        (256+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.Indirect)]
                        for eid in indirectEIDs:
                            # Dispatch + Indirect
                            if eid in dispatchEIDs:
                                expectedUsage.append((eid,rd.ResourceUsage.CS_RWResource))
                                expectedUsage.append((eid,rd.ResourceUsage.Indirect))
                        for eid in multiEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.Indirect))
                        for eid in indirectCountEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.Indirect))
                        for eid in indexedSpecialEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.IndexBuffer))
                            expectedUsage.append((eid,rd.ResourceUsage.Indirect))
                    if res.name == "Barrier Buffer":
                        expectedUsage = [(244+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (252+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (260+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (268+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (276+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (284+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (292+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (300+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (308+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (316+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier)]
                    if res.name == "Barrier2 Buffer":
                        expectedUsage = [(324+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (329+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (334+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (339+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier)]
                    if res.name == "Descriptor Buffer":
                        if descBuffer:
                            expectedUsage = [(236+countDrawIndirectCount,rd.ResourceUsage.Barrier), 
                                        (237+countDrawIndirectCount,rd.ResourceUsage.CopySrc),
                                        (238+countDrawIndirectCount,rd.ResourceUsage.Barrier),
                                        (239+countDrawIndirectCount,rd.ResourceUsage.Clear),
                                        (242+countDrawIndirectCount,rd.ResourceUsage.Barrier),
                                        (243+countDrawIndirectCount,rd.ResourceUsage.CopyDst)]
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
                        if descBuffer:
                            expectedUsage = [(236+countDrawIndirectCount,rd.ResourceUsage.Barrier), 
                                        (237+countDrawIndirectCount,rd.ResourceUsage.CopyDst),
                                        (242+countDrawIndirectCount,rd.ResourceUsage.Barrier),
                                        (243+countDrawIndirectCount,rd.ResourceUsage.CopySrc)]
                elif res.type == rd.ResourceType.Texture:
                    if res.name == "Offscreen MSAA Image":
                        expectedUsage = [(12,rd.ResourceUsage.Barrier), 
                                        (12,rd.ResourceUsage.Discard), 
                                        (13,rd.ResourceUsage.Clear)]
                    if res.name == "Offscreen Image":
                        expectedUsage = [(10,rd.ResourceUsage.Barrier), 
                                        (10,rd.ResourceUsage.Discard), 
                                        (11,rd.ResourceUsage.Clear)] 
                        for eid in descSetDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.PS_Resource))
                        for eid in descBufferDrawEIDs:
                            expectedUsage.append((eid,rd.ResourceUsage.PS_Resource))
                elif res.type == rd.ResourceType.CommandBuffer:
                    expectedUsage = [(0,rd.ResourceUsage.Unused)]
                    # TODO: need to pick carefully which command buffer to validate
                    # TODO: check Baked Command Buffer and Command Buffer
                    continue
                elif res.type == rd.ResourceType.DescriptorStore:
                    expectedUsage = [(4,rd.ResourceUsage.CPUWrite)]
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
                        # Dispatch before Compute region
                        # All the dispatch and indirect 
                        # Dipatch within "Indirect" region
                        for eid in dispatchEIDs:
                            if eid < markerCompute:
                                expectedUsage.append((eid,rd.ResourceUsage.DescriptorSet))
                            elif eid in indirectEIDs:
                                expectedUsage.append((eid,rd.ResourceUsage.DescriptorSet))
                            elif eid > markerIndirect and eid < markerIndirectEnd:
                                expectedUsage.append((eid,rd.ResourceUsage.DescriptorSet))
                elif res.type == rd.ResourceType.Sampler:
                    for eid in descSetDrawEIDs:
                        expectedUsage.append((eid,rd.ResourceUsage.Sampler))
                    for eid in descBufferDrawEIDs:
                        expectedUsage.append((eid,rd.ResourceUsage.Sampler))
                else:
                    raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} Unexpected resource type {res.type.name}")
                rdtest.log.print(f"Resource '{res.name}' type:{res.type.name} {res.resourceId} usages:{len(self.resourceUsages[res.resourceId])} expectedUsages:{len(expectedUsage)}")
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
                    self.controller.SetFrameEvent(eid, True)
                    if eid not in self.eids:
                        raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} Missing EID:{eid}")
        
        with rdtest.log.auto_section("Checking Indirect Action Names"):
            if not self.check_indirect_action_name_consistency(self.controller):
                raise rdtest.TestFailureException("Indirect action parameters do not match its event parameters")
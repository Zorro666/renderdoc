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

        action = self.find_action("Draw")
        self.controller.SetFrameEvent(action.eventId, False)
        swapImage = self.controller.GetPipelineState().GetOutputTargets()[0].resource

        with rdtest.log.auto_section("Checking Resource Usage"):
            for res in self.controller.GetResources():
                expectedUsage = []
                if res.type == rd.ResourceType.Device:
                    expectedUsage = []
                elif res.type == rd.ResourceType.Queue:
                    expectedUsage = [(163+countNested+countDescBuffer, rd.ResourceUsage.Submit)]
                    expectedUsage = []
                elif res.type == rd.ResourceType.Pool:
                    expectedUsage = []
                elif res.type == rd.ResourceType.SwapchainImage:
                    # the swap chain image has usage, anything else does not
                    if res.resourceId == swapImage:
                        expectedUsage = [(6,rd.ResourceUsage.Barrier), 
                                        (6,rd.ResourceUsage.Discard), 
                                        (7,rd.ResourceUsage.Clear), 
                                        (8,rd.ResourceUsage.Barrier), 
                                        (32,rd.ResourceUsage.ColorTarget), 
                                        (35,rd.ResourceUsage.ColorTarget), 
                                        (42,rd.ResourceUsage.ColorTarget), 
                                        (45,rd.ResourceUsage.ColorTarget), 
                                        (59,rd.ResourceUsage.ColorTarget), 
                                        (62,rd.ResourceUsage.ColorTarget), 
                                        (73,rd.ResourceUsage.ColorTarget), 
                                        (76,rd.ResourceUsage.ColorTarget), 
                                        (118,rd.ResourceUsage.ColorTarget), 
                                        (122,rd.ResourceUsage.ColorTarget), 
                                        (126,rd.ResourceUsage.ColorTarget), 
                                        (127,rd.ResourceUsage.ColorTarget), 
                                        (128,rd.ResourceUsage.ColorTarget), 
                                        (129,rd.ResourceUsage.ColorTarget), 
                                        (134,rd.ResourceUsage.ColorTarget), 
                                        (135,rd.ResourceUsage.ColorTarget), 
                                        (136,rd.ResourceUsage.ColorTarget), 
                                        (140,rd.ResourceUsage.ColorTarget), 
                                        (175,rd.ResourceUsage.ColorTarget), 
                                        (179,rd.ResourceUsage.ColorTarget), 
                                        (180,rd.ResourceUsage.ColorTarget), 
                                        (181,rd.ResourceUsage.ColorTarget), 
                                        (182,rd.ResourceUsage.ColorTarget), 
                                        (187,rd.ResourceUsage.ColorTarget), 
                                        (188,rd.ResourceUsage.ColorTarget), 
                                        (189,rd.ResourceUsage.ColorTarget), 
                                        (206,rd.ResourceUsage.ColorTarget), 
                                        (210,rd.ResourceUsage.ColorTarget), 
                                        (211,rd.ResourceUsage.ColorTarget), 
                                        (212,rd.ResourceUsage.ColorTarget), 
                                        (213,rd.ResourceUsage.ColorTarget), 
                                        (218,rd.ResourceUsage.ColorTarget), 
                                        (219,rd.ResourceUsage.ColorTarget), 
                                        (220,rd.ResourceUsage.ColorTarget), 
                                        (225,rd.ResourceUsage.ColorTarget)] 
                        if drawIndirectCount:
                            expectedUsage += [
                                        (230,rd.ResourceUsage.ColorTarget), 
                                        (233,rd.ResourceUsage.ColorTarget), 
                                        (237,rd.ResourceUsage.ColorTarget), 
                                        (241,rd.ResourceUsage.ColorTarget), 
                                        (246,rd.ResourceUsage.ColorTarget), 
                                        (247,rd.ResourceUsage.ColorTarget), 
                                        (248,rd.ResourceUsage.ColorTarget), 
                                        (253,rd.ResourceUsage.ColorTarget), 
                                        (254,rd.ResourceUsage.ColorTarget)]
                        if nestedSecondaries:
                            expectedUsage += [
                                        (249+countDrawIndirectCount+countDescBufferCopy,rd.ResourceUsage.ColorTarget), 
                                        (252+countDrawIndirectCount+countDescBufferCopy,rd.ResourceUsage.ColorTarget)] 
                        if descBuffer:
                            expectedUsage += [
                                        (254+countDrawIndirectCount+countNested,rd.ResourceUsage.ColorTarget), 
                                        (257+countDrawIndirectCount+countNested,rd.ResourceUsage.ColorTarget)] 
                        if meshShader:
                            expectedUsage += [
                                        (239+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.ColorTarget), 
                                        (243+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.ColorTarget), 
                                        (244+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.ColorTarget), 
                                        (247+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.ColorTarget), 
                                        (250+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.ColorTarget), 
                                        (254+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.ColorTarget), 
                                        (258+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.ColorTarget), 
                                        (262+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.ColorTarget)] 

                        expectedUsage += [(233+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier)]
                    else:
                        expectedUsage = []
                elif res.type == rd.ResourceType.RenderPass:
                    expectedUsage = []
                elif res.type == rd.ResourceType.Sync:
                    expectedUsage = []
                elif res.type == rd.ResourceType.View:
                    expectedUsage = []
                elif res.type == rd.ResourceType.Memory:
                    expectedUsage = []
                elif res.type == rd.ResourceType.ShaderBinding:
                    expectedUsage = []
                elif res.type == rd.ResourceType.Shader:
                    expectedUsage = []
                elif res.type == rd.ResourceType.PipelineState:
                    expectedUsage = []
                elif res.type == rd.ResourceType.Buffer:
                    if (res.name == "Vertex Buffer"):
                        expectedUsage = [(32,rd.ResourceUsage.VertexBuffer), 
                                        (35,rd.ResourceUsage.VertexBuffer),
                                        (42,rd.ResourceUsage.VertexBuffer),
                                        (45,rd.ResourceUsage.VertexBuffer),
                                        (59,rd.ResourceUsage.VertexBuffer), 
                                        (62,rd.ResourceUsage.VertexBuffer), 
                                        (73,rd.ResourceUsage.VertexBuffer), 
                                        (76,rd.ResourceUsage.VertexBuffer), 
                                        (118,rd.ResourceUsage.VertexBuffer), 
                                        (122,rd.ResourceUsage.VertexBuffer), 
                                        (126,rd.ResourceUsage.VertexBuffer), 
                                        (127,rd.ResourceUsage.VertexBuffer), 
                                        (128,rd.ResourceUsage.VertexBuffer), 
                                        (129,rd.ResourceUsage.VertexBuffer), 
                                        (134,rd.ResourceUsage.VertexBuffer), 
                                        (135,rd.ResourceUsage.VertexBuffer), 
                                        (136,rd.ResourceUsage.VertexBuffer), 
                                        (140,rd.ResourceUsage.VertexBuffer), 
                                        (175,rd.ResourceUsage.VertexBuffer), 
                                        (179,rd.ResourceUsage.VertexBuffer), 
                                        (180,rd.ResourceUsage.VertexBuffer), 
                                        (181,rd.ResourceUsage.VertexBuffer), 
                                        (182,rd.ResourceUsage.VertexBuffer), 
                                        (187,rd.ResourceUsage.VertexBuffer), 
                                        (188,rd.ResourceUsage.VertexBuffer), 
                                        (189,rd.ResourceUsage.VertexBuffer), 
                                        (206,rd.ResourceUsage.VertexBuffer), 
                                        (210,rd.ResourceUsage.VertexBuffer), 
                                        (211,rd.ResourceUsage.VertexBuffer), 
                                        (212,rd.ResourceUsage.VertexBuffer), 
                                        (213,rd.ResourceUsage.VertexBuffer), 
                                        (218,rd.ResourceUsage.VertexBuffer), 
                                        (219,rd.ResourceUsage.VertexBuffer), 
                                        (220,rd.ResourceUsage.VertexBuffer),
                                        (225,rd.ResourceUsage.VertexBuffer)] 
                        if drawIndirectCount:
                            expectedUsage += [
                                        (230,rd.ResourceUsage.VertexBuffer), 
                                        (233,rd.ResourceUsage.VertexBuffer), 
                                        (237,rd.ResourceUsage.VertexBuffer), 
                                        (241,rd.ResourceUsage.VertexBuffer), 
                                        (246,rd.ResourceUsage.VertexBuffer), 
                                        (247,rd.ResourceUsage.VertexBuffer), 
                                        (248,rd.ResourceUsage.VertexBuffer), 
                                        (253,rd.ResourceUsage.VertexBuffer), 
                                        (254,rd.ResourceUsage.VertexBuffer)]
                        if nestedSecondaries:
                            expectedUsage += [
                                        (249+countDrawIndirectCount+countDescBufferCopy,rd.ResourceUsage.VertexBuffer), 
                                        (252+countDrawIndirectCount+countDescBufferCopy,rd.ResourceUsage.VertexBuffer)]
                        if descBuffer:
                            expectedUsage += [
                                        (254+countDrawIndirectCount+countNested,rd.ResourceUsage.VertexBuffer), 
                                        (257+countDrawIndirectCount+countNested,rd.ResourceUsage.VertexBuffer)]
                    if (res.name == "Index Buffer"):
                        expectedUsage = [(35,rd.ResourceUsage.IndexBuffer),
                                        (45,rd.ResourceUsage.IndexBuffer),
                                        (62,rd.ResourceUsage.IndexBuffer),
                                        (76,rd.ResourceUsage.IndexBuffer),
                                        (134,rd.ResourceUsage.IndexBuffer),
                                        (135,rd.ResourceUsage.IndexBuffer),
                                        (136,rd.ResourceUsage.IndexBuffer),
                                        (140,rd.ResourceUsage.IndexBuffer),
                                        (187,rd.ResourceUsage.IndexBuffer),
                                        (188,rd.ResourceUsage.IndexBuffer),
                                        (189,rd.ResourceUsage.IndexBuffer),
                                        (218,rd.ResourceUsage.IndexBuffer),
                                        (219,rd.ResourceUsage.IndexBuffer),
                                        (220,rd.ResourceUsage.IndexBuffer)]
                        if drawIndirectCount:
                            expectedUsage += [
                                        (233,rd.ResourceUsage.IndexBuffer), 
                                        (241,rd.ResourceUsage.IndexBuffer), 
                                        (253,rd.ResourceUsage.IndexBuffer),
                                        (254,rd.ResourceUsage.IndexBuffer)]
                        if nestedSecondaries:
                            expectedUsage += [
                                        (252+countDrawIndirectCount+countDescBufferCopy,rd.ResourceUsage.IndexBuffer)]
                        if descBuffer:
                            expectedUsage += [
                                        (257+countDrawIndirectCount+countNested,rd.ResourceUsage.IndexBuffer)]
                    if (res.name == "Compute Buffer In"):
                        expectedUsage += [(87,rd.ResourceUsage.CS_Constants),
                                        (94,rd.ResourceUsage.CS_Constants)]
                        if nestedSecondaries:
                            expectedUsage += [(265+countDrawIndirectCount+countDescBufferCopy,rd.ResourceUsage.CS_Constants)]
                        if descBuffer:
                            expectedUsage += [(262+countDrawIndirectCount+countNested,rd.ResourceUsage.CS_Constants)]
                    if (res.name == "Compute Buffer Out"):
                        expectedUsage += [(87,rd.ResourceUsage.CS_RWResource),
                                        (94,rd.ResourceUsage.CS_RWResource)]
                        if nestedSecondaries:
                            expectedUsage += [(265+countDrawIndirectCount+countDescBufferCopy,rd.ResourceUsage.CS_RWResource)]
                        if descBuffer:
                            expectedUsage += [(262+countDrawIndirectCount+countNested,rd.ResourceUsage.CS_RWResource)]
                    if (res.name == "Indirect Data"):
                        expectedUsage += [(14,rd.ResourceUsage.Barrier),
                                        (15,rd.ResourceUsage.Clear),
                                        (16,rd.ResourceUsage.Barrier),
                                        (20,rd.ResourceUsage.CS_RWResource),
                                        (21,rd.ResourceUsage.Barrier),
                                        (95,rd.ResourceUsage.Barrier),
                                        (106,rd.ResourceUsage.CS_RWResource),
                                        (106,rd.ResourceUsage.Indirect),
                                        (107,rd.ResourceUsage.Barrier),
                                        (118,rd.ResourceUsage.Indirect),
                                        (122,rd.ResourceUsage.Indirect),
                                        (125,rd.ResourceUsage.Indirect),
                                        (133,rd.ResourceUsage.Indirect),
                                        (140,rd.ResourceUsage.Indirect),
                                        (142,rd.ResourceUsage.Barrier),
                                        (147,rd.ResourceUsage.Barrier),
                                        (148,rd.ResourceUsage.Clear),
                                        (149,rd.ResourceUsage.Barrier),
                                        (153,rd.ResourceUsage.CS_RWResource),
                                        (155,rd.ResourceUsage.Barrier),
                                        (157,rd.ResourceUsage.CS_RWResource),
                                        (157,rd.ResourceUsage.Indirect),
                                        (158,rd.ResourceUsage.CS_RWResource),
                                        (158,rd.ResourceUsage.Indirect),
                                        (159,rd.ResourceUsage.Barrier),
                                        (160,rd.ResourceUsage.CS_RWResource),
                                        (160,rd.ResourceUsage.Indirect),
                                        (161,rd.ResourceUsage.Barrier),
                                        (175,rd.ResourceUsage.Indirect),
                                        (178,rd.ResourceUsage.Indirect),
                                        (186,rd.ResourceUsage.Indirect),
                                        (206,rd.ResourceUsage.Indirect),
                                        (209,rd.ResourceUsage.Indirect),
                                        (217,rd.ResourceUsage.Indirect),
                                        (225,rd.ResourceUsage.IndexBuffer),
                                        (225,rd.ResourceUsage.Indirect)]
                        if drawIndirectCount:
                            expectedUsage += [
                                        (230,rd.ResourceUsage.Indirect),
                                        (230,rd.ResourceUsage.Indirect),
                                        (233,rd.ResourceUsage.Indirect),
                                        (233,rd.ResourceUsage.Indirect),
                                        (237,rd.ResourceUsage.Indirect),
                                        (237,rd.ResourceUsage.Indirect),
                                        (241,rd.ResourceUsage.Indirect),
                                        (241,rd.ResourceUsage.Indirect),
                                        (245,rd.ResourceUsage.Indirect),
                                        (245,rd.ResourceUsage.Indirect),
                                        (252,rd.ResourceUsage.Indirect),
                                        (252,rd.ResourceUsage.Indirect)]
                        expectedUsage += [(229+countDrawIndirectCount,rd.ResourceUsage.Barrier)]
                        if nestedSecondaries:
                            expectedUsage += [
                                        (266+countDrawIndirectCount+countDescBufferCopy,rd.ResourceUsage.Barrier)]
                        if meshShader:
                            expectedUsage += [
                                        (242+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.Indirect),
                                        (247+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.Indirect),
                                        (250+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.Indirect),
                                        (253+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.Indirect),
                                        (253+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.Indirect),
                                        (258+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.Indirect),
                                        (258+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.Indirect),
                                        (262+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.Indirect),
                                        (262+countDrawIndirectCount+countNested+countDescBuffer,rd.ResourceUsage.Indirect)]
                    if (res.name == "Barrier Buffer"):
                        expectedUsage = [(240+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (248+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (256+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (264+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (272+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (280+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (288+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (296+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (304+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (312+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier)]
                    if (res.name == "Barrier2 Buffer"):
                        expectedUsage = [(320+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (325+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (330+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier),
                                        (335+countDrawIndirectCount+countNested+countDescBuffer+countMeshShader,rd.ResourceUsage.Barrier)]
                    if (res.name == "Descriptor Buffer"):
                        if descBuffer:
                            expectedUsage = [(233+countDrawIndirectCount,rd.ResourceUsage.Barrier), 
                                        (234+countDrawIndirectCount,rd.ResourceUsage.CopySrc),
                                        (235+countDrawIndirectCount,rd.ResourceUsage.Barrier),
                                        (236+countDrawIndirectCount,rd.ResourceUsage.Clear),
                                        (239+countDrawIndirectCount,rd.ResourceUsage.Barrier),
                                        (240+countDrawIndirectCount,rd.ResourceUsage.CopyDst)]
                    if (res.name == "Descriptor Backup Buffer"):
                        if descBuffer:
                            expectedUsage = [(233+countDrawIndirectCount,rd.ResourceUsage.Barrier), 
                                        (234+countDrawIndirectCount,rd.ResourceUsage.CopyDst),
                                        (239+countDrawIndirectCount,rd.ResourceUsage.Barrier),
                                        (240+countDrawIndirectCount,rd.ResourceUsage.CopySrc)]
                elif res.type == rd.ResourceType.Texture:
                    if (res.name == "Offscreen MSAA Image"):
                        expectedUsage = [(11,rd.ResourceUsage.Barrier), 
                                        (11,rd.ResourceUsage.Discard), 
                                        (12,rd.ResourceUsage.Clear)]
                    if (res.name == "Offscreen Image"):
                        expectedUsage = [(9,rd.ResourceUsage.Barrier), 
                                        (9,rd.ResourceUsage.Discard), 
                                        (10,rd.ResourceUsage.Clear), 
                                        (42,rd.ResourceUsage.PS_Resource), 
                                        (45,rd.ResourceUsage.PS_Resource), 
                                        (118,rd.ResourceUsage.PS_Resource), 
                                        (122,rd.ResourceUsage.PS_Resource), 
                                        (126,rd.ResourceUsage.PS_Resource), 
                                        (127,rd.ResourceUsage.PS_Resource), 
                                        (128,rd.ResourceUsage.PS_Resource), 
                                        (129,rd.ResourceUsage.PS_Resource), 
                                        (134,rd.ResourceUsage.PS_Resource), 
                                        (135,rd.ResourceUsage.PS_Resource), 
                                        (136,rd.ResourceUsage.PS_Resource), 
                                        (140,rd.ResourceUsage.PS_Resource), 
                                        (175,rd.ResourceUsage.PS_Resource), 
                                        (179,rd.ResourceUsage.PS_Resource), 
                                        (180,rd.ResourceUsage.PS_Resource), 
                                        (181,rd.ResourceUsage.PS_Resource), 
                                        (182,rd.ResourceUsage.PS_Resource), 
                                        (187,rd.ResourceUsage.PS_Resource), 
                                        (188,rd.ResourceUsage.PS_Resource), 
                                        (189,rd.ResourceUsage.PS_Resource), 
                                        (206,rd.ResourceUsage.PS_Resource), 
                                        (210,rd.ResourceUsage.PS_Resource), 
                                        (211,rd.ResourceUsage.PS_Resource), 
                                        (212,rd.ResourceUsage.PS_Resource), 
                                        (213,rd.ResourceUsage.PS_Resource), 
                                        (218,rd.ResourceUsage.PS_Resource), 
                                        (219,rd.ResourceUsage.PS_Resource), 
                                        (220,rd.ResourceUsage.PS_Resource),
                                        (225,rd.ResourceUsage.PS_Resource)]
                        if drawIndirectCount:
                            expectedUsage += [
                                        (230,rd.ResourceUsage.PS_Resource), 
                                        (233,rd.ResourceUsage.PS_Resource), 
                                        (237,rd.ResourceUsage.PS_Resource), 
                                        (241,rd.ResourceUsage.PS_Resource), 
                                        (246,rd.ResourceUsage.PS_Resource), 
                                        (247,rd.ResourceUsage.PS_Resource), 
                                        (248,rd.ResourceUsage.PS_Resource), 
                                        (253,rd.ResourceUsage.PS_Resource), 
                                        (254,rd.ResourceUsage.PS_Resource)]
                        if descBuffer:
                            expectedUsage += [
                                        (254+countDrawIndirectCount+countNested,rd.ResourceUsage.PS_Resource), 
                                        (257+countDrawIndirectCount+countNested,rd.ResourceUsage.PS_Resource)]
                elif res.type == rd.ResourceType.CommandBuffer:
                    expectedUsage = []
                elif res.type == rd.ResourceType.DescriptorStore:
                    expectedUsage = []
                elif res.type == rd.ResourceType.Sampler:
                    expectedUsage = []
                else:
                    raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} Unexpected resource type {res.type.name}")
                rdtest.log.print(f"Resource '{res.name}' type:{res.type.name} {res.resourceId} usages:{len(self.controller.GetUsage(res.resourceId))} expectedUsages:{len(expectedUsage)}")
                self.check_resource_usage(res, expectedUsage)

        actions = self.controller.GetRootActions()
        for a in actions:
            self.add_action(a)

        # Select every event of the resource usage to ensure the EID is valid
        with rdtest.log.auto_section("Checking Resource Usage Events can be replayed"):
            for res in self.controller.GetResources():
                rdtest.log.print(f"Resource '{res.name}' type:{res.type.name} {res.resourceId}")
                usages = self.resourceUsages[res.resourceId]
                for u in usages:
                    eid = u.eventId
                    if eid == 0:
                        continue
                    self.controller.SetFrameEvent(eid, True)
                    if eid not in self.eids:
                        raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} Missing EID:{eid}")
        

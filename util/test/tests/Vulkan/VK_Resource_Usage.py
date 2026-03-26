import renderdoc as rd
import rdtest

class VK_Resource_Usage(rdtest.TestCase):
    demos_test_name = 'VK_Resource_Usage'
    resourceUsages = {}

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

        descBuffer = self.find_action("Descriptor Buffer") is not None
        rdtest.log.print(f"Has Descriptor Buffer: {'Yes' if descBuffer else 'No'}")

        action = self.find_action("Draw")
        self.controller.SetFrameEvent(action.eventId, False)
        swapImage = self.controller.GetPipelineState().GetOutputTargets()[0].resource

        for res in self.controller.GetResources():
            expectedUsage = []
            if res.type == rd.ResourceType.Device:
                expectedUsage = [(0,rd.ResourceUsage.Unused)]
            elif res.type == rd.ResourceType.Queue:
                expectedUsage = [(0,rd.ResourceUsage.Unused)]
            elif res.type == rd.ResourceType.Pool:
                expectedUsage = [(0,rd.ResourceUsage.Unused)]
            elif res.type == rd.ResourceType.SwapchainImage:
                # the swap chain image has usage, anything else does not
                if res.resourceId == swapImage:
                    expectedUsage = [(6,rd.ResourceUsage.Barrier), 
                                     (6,rd.ResourceUsage.Discard), 
                                     (7,rd.ResourceUsage.Clear), 
                                     (8,rd.ResourceUsage.Barrier), 
                                     (22,rd.ResourceUsage.ColorTarget), 
                                     (25,rd.ResourceUsage.ColorTarget), 
                                     (32,rd.ResourceUsage.ColorTarget), 
                                     (35,rd.ResourceUsage.ColorTarget)] 
                    if descBuffer:
                        expectedUsage += [
                                     (43,rd.ResourceUsage.ColorTarget), 
                                     (46,rd.ResourceUsage.ColorTarget), 
                                     (60,rd.ResourceUsage.ColorTarget), 
                                     (63,rd.ResourceUsage.ColorTarget), 
                                     (80,rd.ResourceUsage.ColorTarget), 
                                     (83,rd.ResourceUsage.ColorTarget), 
                                     (125,rd.ResourceUsage.Barrier)]
                    else:
                        expectedUsage += [
                                     (49,rd.ResourceUsage.ColorTarget), 
                                     (52,rd.ResourceUsage.ColorTarget), 
                                     (69,rd.ResourceUsage.ColorTarget), 
                                     (72,rd.ResourceUsage.ColorTarget), 
                                     (108,rd.ResourceUsage.Barrier)]
                else:
                    expectedUsage = []
            elif res.type == rd.ResourceType.RenderPass:
                expectedUsage = [(0,rd.ResourceUsage.Unused)]
            elif res.type == rd.ResourceType.Sync:
                expectedUsage = [(0,rd.ResourceUsage.Unused)]
            elif res.type == rd.ResourceType.View:
                expectedUsage = [(0,rd.ResourceUsage.Unused)]
            elif res.type == rd.ResourceType.Memory:
                expectedUsage = [(0,rd.ResourceUsage.Unused)]
            elif res.type == rd.ResourceType.ShaderBinding:
                expectedUsage = [(0,rd.ResourceUsage.Unused)]
            elif res.type == rd.ResourceType.Shader:
                expectedUsage = [(0,rd.ResourceUsage.Unused)]
            elif res.type == rd.ResourceType.PipelineState:
                expectedUsage = [(0,rd.ResourceUsage.Unused)]
            elif res.type == rd.ResourceType.Buffer:
                if (res.name == "Vertex Buffer"):
                    expectedUsage = [(22,rd.ResourceUsage.VertexBuffer), 
                                     (25,rd.ResourceUsage.VertexBuffer),
                                     (32,rd.ResourceUsage.VertexBuffer),
                                     (35,rd.ResourceUsage.VertexBuffer)]
                    if descBuffer:
                        expectedUsage += [
                                     (43,rd.ResourceUsage.VertexBuffer), 
                                     (46,rd.ResourceUsage.VertexBuffer),
                                     (60,rd.ResourceUsage.VertexBuffer), 
                                     (63,rd.ResourceUsage.VertexBuffer),
                                     (80,rd.ResourceUsage.VertexBuffer), 
                                     (83,rd.ResourceUsage.VertexBuffer)]
                    else:
                        expectedUsage += [
                                     (49,rd.ResourceUsage.VertexBuffer), 
                                     (52,rd.ResourceUsage.VertexBuffer),
                                     (69,rd.ResourceUsage.VertexBuffer), 
                                     (72,rd.ResourceUsage.VertexBuffer)]
                if (res.name == "Index Buffer"):
                    expectedUsage = [(25,rd.ResourceUsage.IndexBuffer),
                                     (35,rd.ResourceUsage.IndexBuffer)]
                    if descBuffer:
                        expectedUsage += [
                                     (46,rd.ResourceUsage.IndexBuffer),
                                     (63,rd.ResourceUsage.IndexBuffer),
                                     (83,rd.ResourceUsage.IndexBuffer)]
                    else:
                        expectedUsage += [
                                     (52,rd.ResourceUsage.IndexBuffer),
                                     (72,rd.ResourceUsage.IndexBuffer)]
                if (res.name == "Compute Buffer In"):
                    if descBuffer:
                        expectedUsage = [(95,rd.ResourceUsage.CS_Constants),
                                     (101,rd.ResourceUsage.CS_Constants),
                                     (108,rd.ResourceUsage.CS_Constants),
                                     (118,rd.ResourceUsage.CS_Constants)]
                    else:
                        expectedUsage = [(84,rd.ResourceUsage.CS_Constants),
                                     (91,rd.ResourceUsage.CS_Constants),
                                     (101,rd.ResourceUsage.CS_Constants)]
                if (res.name == "Compute Buffer Out"):
                    if descBuffer:
                        expectedUsage = [(95,rd.ResourceUsage.CS_RWResource),
                                     (101,rd.ResourceUsage.CS_RWResource),
                                     (108,rd.ResourceUsage.CS_RWResource),
                                     (118,rd.ResourceUsage.CS_RWResource)]
                    else:
                        expectedUsage = [(84,rd.ResourceUsage.CS_RWResource),
                                     (91,rd.ResourceUsage.CS_RWResource),
                                     (101,rd.ResourceUsage.CS_RWResource)]
            elif res.type == rd.ResourceType.Texture:
                if (res.name == "Offscreen MSAA Image"):
                    expectedUsage = [(11,rd.ResourceUsage.Barrier), 
                                     (11,rd.ResourceUsage.Discard), 
                                     (12,rd.ResourceUsage.Clear)]
                if (res.name == "Offscreen Image"):
                    expectedUsage = [(9,rd.ResourceUsage.Barrier), 
                                     (9,rd.ResourceUsage.Discard), 
                                     (10,rd.ResourceUsage.Clear), 
                                     (32,rd.ResourceUsage.PS_Resource), 
                                     (35,rd.ResourceUsage.PS_Resource)]
                    if descBuffer:
                        expectedUsage += [
                                     (43,rd.ResourceUsage.PS_Resource), 
                                     (46,rd.ResourceUsage.PS_Resource)]
            elif res.type == rd.ResourceType.CommandBuffer:
                expectedUsage = [(0,rd.ResourceUsage.Unused)]
            elif res.type == rd.ResourceType.DescriptorStore:
                expectedUsage = [(0,rd.ResourceUsage.Unused)]
            elif res.type == rd.ResourceType.Sampler:
                expectedUsage = [(0,rd.ResourceUsage.Unused)]
            else:
                raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} Unexpected resource type {res.type.name}")
            rdtest.log.print(f"Resource '{res.name}' type:{res.type.name} {res.resourceId} usages:{len(self.controller.GetUsage(res.resourceId))} expectedUsages:{len(expectedUsage)}")
            self.check_resource_usage(res, expectedUsage)


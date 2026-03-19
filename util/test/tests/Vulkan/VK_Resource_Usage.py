import renderdoc as rd
import rdtest

class VK_Resource_Usage(rdtest.TestCase):
    demos_test_name = 'VK_Simple_Triangle'
    resourceUsages = {}

    def check_resource_usage(self, res: rd.ResourceDescription, expectedUsages=[]):
        usages = self.resourceUsages[res.resourceId]
        if len(usages) != len(expectedUsages):
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

        action = self.find_action("Draw")
        self.controller.SetFrameEvent(action.eventId, False)
        swapImage = self.controller.GetPipelineState().GetOutputTargets()[0].resource
        textures = self.controller.GetTextures()

        for res in self.controller.GetResources():
            expectedUsage = []
            if res.type == rd.ResourceType.Device:
                expectedUsage = []
            elif res.type == rd.ResourceType.Queue:
                expectedUsage = [(20, rd.ResourceUsage.Submit)]
            elif res.type == rd.ResourceType.Pool:
                expectedUsage = []
            elif res.type == rd.ResourceType.SwapchainImage:
                # the swap chain image has usage, anything else does not
                if res.resourceId == swapImage:
                    expectedUsage = [(6,rd.ResourceUsage.Barrier), (6,rd.ResourceUsage.Discard), (7,rd.ResourceUsage.Clear), (17,rd.ResourceUsage.ColorTarget), (19,rd.ResourceUsage.Barrier)]
                else:
                    expectedUsage = []
            elif res.type == rd.ResourceType.RenderPass:
#.. Resource Framebuffer 126 type:RenderPass id:ResourceId::126 usages:0
#.. Resource Render Pass 117 type:RenderPass id:ResourceId::117 usages:0
                #expectedUsage = [rd.ResourceUsage.Start, rd.ResourceUsage.RenderPass, rd.ResourceUsage.End]
                expectedUsage = []
            elif res.type == rd.ResourceType.Sync:
# .. Resource Autotesting renderStartSemaphore0 type:Sync 0 usages
# .. Resource Autotesting renderEndSemaphore0 type:Sync 0 usages
# .. Resource Autotesting fence0 type:Sync 0 usages
# .. Resource Fence 162 type:Sync 0 usages
# .. Resource Fence 233 type:Sync 0 usages
                expectedUsage = []
            elif res.type == rd.ResourceType.View:
                #expectedUsage = [rd.ResourceUsage.Bound]
                expectedUsage = []
# .. Resource Image View 124 type:View 0 usages
            elif res.type == rd.ResourceType.Memory:
# .. Resource Memory 134 type:Memory 0 usages
# .. Resource Memory 142 type:Memory 0 usages
                expectedUsage = []
            elif res.type == rd.ResourceType.ShaderBinding:
                expectedUsage = []
            elif res.type == rd.ResourceType.Shader:
                # Check if it is the vertex or pixel shader
                #expectedUsage = [rd.ResourceUsage.VertexShader]
                #expectedUsage = [rd.ResourceUsage.PixelShader]
                expectedUsage = []
            elif res.type == rd.ResourceType.PipelineState:
                #expectedUsage = [rd.ResourceUsage.Bound, rd.ResourceUsage.Pipeline]
                expectedUsage = []
# .. Resource Graphics Pipeline 139 type:PipelineState 0 usages
            elif res.type == rd.ResourceType.Buffer:
                #expectedUsage = [rd.ResourceUsage.Bound, rd.ResourceUsage.VertexBuffer]
                expectedUsage = [(17,rd.ResourceUsage.VertexBuffer)]
            elif res.type == rd.ResourceType.Texture:
                desc = [x for x in textures if x.resourceId == res.resourceId][0]
                # Hard coded distinguish by the format of the texture
                if desc.format.compByteWidth == 2 and desc.format.compCount == 4 and desc.format.compType == rd.CompType.Float:
                    expectedUsage = [(10,rd.ResourceUsage.Barrier), (10,rd.ResourceUsage.Discard), (11,rd.ResourceUsage.Clear)]
                elif desc.format.compByteWidth == 4 and desc.format.compCount == 4 and desc.format.compType == rd.CompType.Float:
                    expectedUsage = [(8,rd.ResourceUsage.Barrier), (8,rd.ResourceUsage.Discard), (9,rd.ResourceUsage.Clear)]
            elif res.type == rd.ResourceType.CommandBuffer:
# .. Resource Command Buffer 149 type:CommandBuffer 0 usages
# .. Resource Baked Command Buffer 232 type:CommandBuffer 0 usages
                expectedUsage = []
            else:
                raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} Unexpected resource type {res.type.name}")
            rdtest.log.print(f"Resource '{res.name}' type:{res.type.name} {res.resourceId} usages:{len(self.controller.GetUsage(res.resourceId))} expectedUsages:{len(expectedUsage)}")
            self.check_resource_usage(res, expectedUsage)


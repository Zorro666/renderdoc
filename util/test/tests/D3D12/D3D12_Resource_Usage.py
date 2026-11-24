from pickletools import dis
from typing import List, Tuple

import renderdoc as rd
import rdtest

class D3D12_Resource_Usage(rdtest.Resource_Usage):
    internal = False
    demos_test_name = 'D3D12_Resource_Usage'

    def check_capture(self):
        super(D3D12_Resource_Usage, self).check_capture()

        # Walk the actions finding all the Dispatch, Draw, DrawIndexed, DrawIndirect, DrawIndirectCount
        drawEIDs: List[int] = []
        meshDispatchEIDs: List[int] = []
        indexedEIDs: List[int] = []
        indexedSpecialEIDs: List[int] = []
        dispatchEIDs: List[int] = []
        indirectEIDs: List[int] = []
        multiEIDs: List[int] = []
        indirectCountEIDs: List[int] = []
        submitEIDs: List[int] = []
        setDescHeaps: List[int] = []
        setGraphicsRootDescTables: List[int] = []
        setGraphicsRootSRVs: List[int] = []
        setGraphicsRootUAVs: List[int] = []
        setGraphicsRootCBVs: List[int] = []
        indirectSetRootSRVs: List[int] = []
        indirectSetRootUAVs: List[int] = []
        indirectSetRootCBVs: List[int] = []
        setComputeRootDescTables: List[int] = []
        omSetRenderTargets: List[int] = []
        iaSetVertexBuffers: List[int] = []
        iaSetIndexBuffers: List[int] = []
        indirectIASetVertexBuffers: List[int] = []
        indirectIASetIndexBuffers: List[int] = []
        setGraphicsRootSigs: List[int] = []
        setComputeRootSigs: List[int] = []
        setPipelineStates: List[int] = []
        setMarkers: List[int] = []
        createRenderTargetViews: List[int] = []
        resourceBarriers: List[int] = []
        clearRenderTargetViews: List[int] = []
        signalEIDs: List[int] = []

        self.markerStart = self.find_action("Resource Usage: Start").eventId
        self.markerEnd = self.find_action("Resource Usage: End").eventId

        while len(self.actions) > 0:
            action = self.actions.pop(0)
            self.actions += action.children
            flags = action.flags
            if flags & rd.ActionFlags.Drawcall:
                drawEIDs.append(action.eventId)
            if flags & rd.ActionFlags.Indexed:
                indexedEIDs.append(action.eventId)
            if flags & rd.ActionFlags.Dispatch:
                dispatchEIDs.append(action.eventId)
            if flags & rd.ActionFlags.Indirect:
                indirectEIDs.append(action.eventId)
            if flags & rd.ActionFlags.MeshDispatch:
                meshDispatchEIDs.append(action.eventId)
            if flags & rd.ActionFlags.MultiAction:
                multiEIDs.append(action.eventId)
            for e in action.events:
                if e.eventId > self.markerEnd:
                    continue
                eventName = self.sdfile.chunks[e.chunkIndex].name
                if "SetDescriptorHeaps" in eventName:
                    setDescHeaps.append(e.eventId)
                if "SetGraphicsRootDescriptorTable" in eventName:
                    setGraphicsRootDescTables.append(e.eventId)
                if "SetComputeRootDescriptorTable" in eventName:
                    setComputeRootDescTables.append(e.eventId)
                if "OMSetRenderTargets" in eventName:
                    omSetRenderTargets.append(e.eventId)
                if "::IASetVertexBuffers" in eventName:
                    iaSetVertexBuffers.append(e.eventId)
                if "::IASetIndexBuffer" in eventName:
                    iaSetIndexBuffers.append(e.eventId)
                if "IndirectIASetVertexBuffer" in eventName:
                    indirectIASetVertexBuffers.append(e.eventId)
                if "IndirectIASetIndexBuffer" in eventName:
                    indirectIASetIndexBuffers.append(e.eventId)
                if "SetGraphicsRootSignature" in eventName:
                    setGraphicsRootSigs.append(e.eventId)
                if "SetComputeRootSignature" in eventName:
                    setComputeRootSigs.append(e.eventId)
                if "SetPipelineState" in eventName:
                    setPipelineStates.append(e.eventId)
                if "SetMarker" in eventName:
                    setMarkers.append(e.eventId)
                if "SetGraphicsRootShaderResourceView" in eventName:
                    setGraphicsRootSRVs.append(e.eventId)
                if "SetGraphicsRootUnorderedAccessView" in eventName:
                    setGraphicsRootUAVs.append(e.eventId)
                if "SetGraphicsRootConstantBufferView" in eventName:
                    setGraphicsRootCBVs.append(e.eventId)
                if "IndirectSetRootUnorderedAccessView" in eventName:
                    indirectSetRootUAVs.append(e.eventId)
                if "IndirectSetRootShaderResourceView" in eventName:
                    indirectSetRootSRVs.append(e.eventId)
                if "IndirectSetRootConstantBufferView" in eventName:
                    indirectSetRootCBVs.append(e.eventId)
                if "ExecuteCommandLists" in eventName:
                    submitEIDs.append(e.eventId)
                if "CreateRenderTargetView" in eventName:
                    createRenderTargetViews.append(e.eventId)
                if "ResourceBarrier" in eventName:
                    resourceBarriers.append(e.eventId)
                if "ClearRenderTargetView" in eventName:
                    clearRenderTargetViews.append(e.eventId)
                if "Signal" in eventName:
                    signalEIDs.append(e.eventId)

        drawEIDs = self.prune_and_sort(drawEIDs)
        indexedEIDs = self.prune_and_sort(indexedEIDs)
        indexedSpecialEIDs = self.prune_and_sort(indexedSpecialEIDs)
        dispatchEIDs = self.prune_and_sort(dispatchEIDs)
        multiEIDs = self.prune_and_sort(multiEIDs)
        indirectEIDs = self.prune_and_sort(indirectEIDs)
        meshDispatchEIDs = self.prune_and_sort(meshDispatchEIDs)           
        indirectCountEIDs = self.prune_and_sort(indirectCountEIDs)
        submitEIDs = self.prune_and_sort(submitEIDs)
        setDescHeaps = self.prune_and_sort(setDescHeaps)
        setGraphicsRootDescTables = self.prune_and_sort(setGraphicsRootDescTables)
        setComputeRootDescTables = self.prune_and_sort(setComputeRootDescTables)
        setGraphicsRootSigs = self.prune_and_sort(setGraphicsRootSigs)
        setGraphicsRootSRVs = self.prune_and_sort(setGraphicsRootSRVs)
        setGraphicsRootUAVs = self.prune_and_sort(setGraphicsRootUAVs)
        setGraphicsRootCBVs = self.prune_and_sort(setGraphicsRootCBVs)
        indirectSetRootSRVs = self.prune_and_sort(indirectSetRootSRVs)
        indirectSetRootUAVs = self.prune_and_sort(indirectSetRootUAVs)
        indirectSetRootCBVs = self.prune_and_sort(indirectSetRootCBVs)
        setComputeRootSigs = self.prune_and_sort(setComputeRootSigs)
        setPipelineStates = self.prune_and_sort(setPipelineStates)
        setMarkers = self.prune_and_sort(setMarkers)
        omSetRenderTargets = self.prune_and_sort(omSetRenderTargets)
        iaSetVertexBuffers = self.prune_and_sort(iaSetVertexBuffers)
        iaSetIndexBuffers = self.prune_and_sort(iaSetIndexBuffers)
        indirectIASetVertexBuffers = self.prune_and_sort(indirectIASetVertexBuffers)
        indirectIASetIndexBuffers = self.prune_and_sort(indirectIASetIndexBuffers)
        createRenderTargetViews = self.prune_and_sort(createRenderTargetViews)
        resourceBarriers = self.prune_and_sort(resourceBarriers)
        clearRenderTargetViews = self.prune_and_sort(clearRenderTargetViews)
        signalEIDs = self.prune_and_sort(signalEIDs)

        action = self.find_action("Draw")
        self.set_event(action.eventId, False)
        swapImage = self.controller.GetPipelineState().GetOutputTargets()[0].resource
        defaultVS = self.controller.GetPipelineState().GetShader(rd.ShaderStage.Vertex)
        defaultPS = self.controller.GetPipelineState().GetShader(rd.ShaderStage.Pixel)

        action = self.find_action("Dispatch")
        self.set_event(action.eventId, False)
        defaultCS = self.controller.GetPipelineState().GetShader(rd.ShaderStage.Compute)

        with rdtest.log.auto_section("Checking Resource Usage"):
            for res in self.resources:
                expectedUsage: List[Tuple[int,rd.ResourceUsage]] = []
                if res.type == rd.ResourceType.Device:
                    expectedUsage = []
                elif res.type == rd.ResourceType.Queue:
                    expectedUsage += [(eid,rd.ResourceUsage.Submit) for eid in submitEIDs]
                elif res.type == rd.ResourceType.Pool:
                    expectedUsage = []
                elif res.type == rd.ResourceType.SwapchainImage:
                    expectedUsage += [(eid, rd.ResourceUsage.Barrier) for eid in resourceBarriers]
                    expectedUsage += [(eid, rd.ResourceUsage.Clear) for eid in clearRenderTargetViews]
                    expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in omSetRenderTargets]
                    expectedUsage += [(eid,rd.ResourceUsage.ColorTarget) for eid in drawEIDs]
                    expectedUsage += [(eid,rd.ResourceUsage.ColorTarget) for eid in meshDispatchEIDs]
                elif res.type == rd.ResourceType.RenderPass:
                    expectedUsage = []
                elif res.type == rd.ResourceType.Sync:
                    if res.name == "Default Fence":
                        expectedUsage += [(signalEIDs[1], rd.ResourceUsage.Signal)]
                    elif res.name.startswith("GPUSync"):
                        # Ignore anonymous fences
                        continue
                elif res.type == rd.ResourceType.View:
                    expectedUsage = []
                elif res.type == rd.ResourceType.Memory:
                    expectedUsage = []
                elif res.type == rd.ResourceType.ShaderBinding:
                    if res.name == "Compute Root Signature":
                        expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in setComputeRootSigs]
                        expectedUsage += [(eid,rd.ResourceUsage.PipelineLayout) for eid in dispatchEIDs]
                    elif res.name == "Default Root Signature":
                        expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in setGraphicsRootSigs]
                        expectedUsage += [(eid,rd.ResourceUsage.PipelineLayout) for eid in drawEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.Sampler) for eid in drawEIDs]
                elif res.type == rd.ResourceType.Shader:
                    if res.resourceId == defaultVS:
                        expectedUsage += [(eid,rd.ResourceUsage.VS_Shader) for eid in drawEIDs]
                    elif res.resourceId == defaultPS:
                        expectedUsage += [(eid,rd.ResourceUsage.PS_Shader) for eid in drawEIDs]
                    elif res.resourceId == defaultCS:
                        expectedUsage += [(eid,rd.ResourceUsage.CS_Shader) for eid in dispatchEIDs]
                elif res.type == rd.ResourceType.PipelineState:
                    if res.name == "Default PSO":
                        expectedUsage += [(setPipelineStates[0],rd.ResourceUsage.Bind)]
                        expectedUsage += [(eid,rd.ResourceUsage.Pipeline) for eid in drawEIDs]
                        expectedUsage += [(setPipelineStates[1],rd.ResourceUsage.UnBind)]
                    elif res.name == "Compute PSO":
                        expectedUsage += [(setPipelineStates[1],rd.ResourceUsage.Bind)]
                        expectedUsage += [(eid,rd.ResourceUsage.Pipeline) for eid in dispatchEIDs]
                elif res.type == rd.ResourceType.Buffer:
                    if res.name == "Vertex Buffer":
                        expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in iaSetVertexBuffers]
                        expectedUsage += [(eid,rd.ResourceUsage.VertexBuffer) for eid in drawEIDs if eid not in indirectEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.UnBind) for eid in indirectIASetVertexBuffers]
                    elif res.name == "EI Vertex Buffer":
                        expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in indirectIASetVertexBuffers]
                        expectedUsage += [(eid,rd.ResourceUsage.VertexBuffer) for eid in indirectEIDs]
                    elif res.name == "Unused Vertex Buffer":
                        expectedUsage += [(iaSetVertexBuffers[1],rd.ResourceUsage.UnBind)]
                        expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in iaSetVertexBuffers]
                    elif res.name == "Index Buffer":
                        expectedUsage += [(eid,rd.ResourceUsage.IndexBuffer) for eid in indexedEIDs if eid not in indirectEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.UnBind) for eid in indirectIASetIndexBuffers]
                        expectedUsage += [(iaSetIndexBuffers[1],rd.ResourceUsage.Bind)]
                        expectedUsage += [(iaSetIndexBuffers[2],rd.ResourceUsage.Bind)]
                    elif res.name == "EI Index Buffer":
                        expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in indirectIASetIndexBuffers]
                        expectedUsage += [(eid,rd.ResourceUsage.IndexBuffer) for eid in indirectEIDs]
                    elif res.name == "Unused Index Buffer":
                        expectedUsage += [(iaSetIndexBuffers[0],rd.ResourceUsage.Bind)]
                        expectedUsage += [(iaSetIndexBuffers[1],rd.ResourceUsage.UnBind)]
                    elif res.name == "UAV":
                        expectedUsage += [(eid,rd.ResourceUsage.PS_RWResource) for eid in drawEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.CS_RWResource) for eid in dispatchEIDs]
                    elif res.name == "CBV":
                        expectedUsage += [(eid,rd.ResourceUsage.PS_Constants) for eid in drawEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.CS_Constants) for eid in dispatchEIDs]
                    elif res.name == "Root SRV":
                        expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in setGraphicsRootSRVs]
                        expectedUsage += [(eid,rd.ResourceUsage.VS_Resource) for eid in drawEIDs if eid not in indirectEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.UnBind) for eid in indirectSetRootSRVs]
                    elif res.name == "Root UAV":
                        expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in setGraphicsRootUAVs]
                        expectedUsage += [(eid,rd.ResourceUsage.VS_RWResource) for eid in drawEIDs if eid not in indirectEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.UnBind) for eid in indirectSetRootUAVs]
                    elif res.name == "Root CBV":
                        expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in setGraphicsRootCBVs]
                        expectedUsage += [(eid,rd.ResourceUsage.VS_Constants) for eid in drawEIDs if eid not in indirectEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.UnBind) for eid in indirectSetRootCBVs]
                    elif res.name == "EI Root SRV":
                        expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in indirectSetRootSRVs]
                        expectedUsage += [(eid,rd.ResourceUsage.VS_Resource) for eid in indirectEIDs]
                    elif res.name == "EI Root UAV":
                        expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in indirectSetRootUAVs]
                        expectedUsage += [(eid,rd.ResourceUsage.VS_RWResource) for eid in indirectEIDs]
                    elif res.name == "EI Root CBV":
                        expectedUsage += [(eid,rd.ResourceUsage.Bind) for eid in indirectSetRootCBVs]
                        expectedUsage += [(eid,rd.ResourceUsage.VS_Constants) for eid in indirectEIDs]
                    elif res.name == "EI Argument Buffer":
                        expectedUsage += [(eid,rd.ResourceUsage.Indirect) for eid in multiEIDs]
                elif res.type == rd.ResourceType.Texture:
                    if res.name == "Smiley Texture":
                        expectedUsage += [(eid,rd.ResourceUsage.PS_Resource) for eid in drawEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.CS_Resource) for eid in dispatchEIDs]
                elif res.type == rd.ResourceType.CommandBuffer:
                    if res.name == "Main Command List (Baked)":
                        expectedUsage += [(submitEIDs[0]+1,rd.ResourceUsage.Reset)]
                        for i in range(submitEIDs[0]+2, submitEIDs[1]-2):
                            expectedUsage += [(i,rd.ResourceUsage.Recorded)]
                        for i in range(1, 5):
                            expectedUsage += [(setMarkers[i],rd.ResourceUsage.Marker)]
                        expectedUsage += [(submitEIDs[1]-2,rd.ResourceUsage.Close)]
                    elif res.name == "Main Command List":
                        expectedUsage += [(submitEIDs[0]+1,rd.ResourceUsage.Reset)]
                        expectedUsage += [(submitEIDs[1]-2,rd.ResourceUsage.Close)]
                    elif res.name.startswith("Helper Command List"):
                        # Ignore helper command lists
                        continue;
                elif res.type == rd.ResourceType.DescriptorStore:
                    if res.name == "Sampler heap":
                        expectedUsage += [(setDescHeaps[0],rd.ResourceUsage.Bind)]
                        expectedUsage += [(setGraphicsRootDescTables[3],rd.ResourceUsage.Bind)]
                        expectedUsage += [(eid,rd.ResourceUsage.DescriptorStore) for eid in drawEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.Sampler) for eid in drawEIDs]
                        expectedUsage += [(setDescHeaps[-1],rd.ResourceUsage.UnBind)]
                    elif res.name == "CBV/UAV/SRV heap":
                        expectedUsage += [(setDescHeaps[0],rd.ResourceUsage.Bind)]
                        expectedUsage += [(setGraphicsRootDescTables[0],rd.ResourceUsage.Bind)]
                        expectedUsage += [(setGraphicsRootDescTables[1],rd.ResourceUsage.Bind)]
                        expectedUsage += [(setGraphicsRootDescTables[2],rd.ResourceUsage.Bind)]
                        expectedUsage += [(eid,rd.ResourceUsage.DescriptorStore) for eid in drawEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.DescriptorStore) for eid in dispatchEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.PS_Constants) for eid in drawEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.PS_Resource) for eid in drawEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.PS_RWResource) for eid in drawEIDs]
                        expectedUsage += [(setDescHeaps[-1],rd.ResourceUsage.UnBind)]
                        expectedUsage += [(setDescHeaps[-1],rd.ResourceUsage.Bind)]
                        expectedUsage += [(setComputeRootDescTables[0],rd.ResourceUsage.Bind)]
                        expectedUsage += [(setComputeRootDescTables[1],rd.ResourceUsage.Bind)]
                        expectedUsage += [(setComputeRootDescTables[2],rd.ResourceUsage.Bind)]
                        expectedUsage += [(eid,rd.ResourceUsage.CS_Constants) for eid in dispatchEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.CS_Resource) for eid in dispatchEIDs]
                        expectedUsage += [(eid,rd.ResourceUsage.CS_RWResource) for eid in dispatchEIDs]
                    elif res.name == "RTV heap":
                        expectedUsage += [(eid,rd.ResourceUsage.DescriptorStoreWrite) for eid in createRenderTargetViews]
                elif res.type == rd.ResourceType.Sampler:
                    expectedUsage = []
                else:
                    raise rdtest.TestFailureException(f"'{res.name}' {res.resourceId} Unexpected resource type {res.type.name}")
                expectedUsage.sort(key=lambda x: x[0])
                self.check_resource_usage(res, expectedUsage)

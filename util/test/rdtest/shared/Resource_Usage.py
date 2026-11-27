from typing import Dict, List, Tuple

import renderdoc as rd
import rdtest

class Resource_Usage(rdtest.TestCase):
    internal = True
    slow_test = True
    demos_test_name = None
    resources: List[rd.ResourceDescription] = []
    resourceUsages: Dict[rd.ResourceId, List[rd.EventUsage]] = {}
    eids: List[int] = []
    markerStart = 0
    markerEnd = 0
    actions: List[rd.ActionDescription] = []

    def prune_and_sort(self, eids: List[int]) -> List[int]:
        return sorted([x for x in eids if x >= self.markerStart and x <= self.markerEnd])

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
        self.resources = self.controller.GetResources()
        for res in self.resources:
            self.resourceUsages[res.resourceId] = self.controller.GetUsage(res.resourceId)

        self.actions = self.controller.GetRootActions().copy()
        for a in self.actions:
            self.add_action(a)

        # Select every event of the resource usage to ensure the EID is valid
        with rdtest.log.auto_section("Checking Resource Usage Events can be replayed"):
            for res in self.resources:
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
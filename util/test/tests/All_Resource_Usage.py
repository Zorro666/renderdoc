import rdtest
import os
import renderdoc as rd


class All_Resource_Usage(rdtest.TestCase):

    def resource_usage(self, path):
        try:
            replay: rd.ReplayController = rdtest.open_capture(path)
        except RuntimeError as err:
            rdtest.log.print(f"Skipping. Can't open {path}: {err}")
            return

        rdtest.log.print("Loaded");
        with rdtest.log.auto_section("Checking Indirect Action Names"):
            if not self.check_indirect_action_name_consistency(replay):
                raise rdtest.TestFailureException("Indirect action parameters do not match its event parameters")

        resources = replay.GetResources()
        resourceUsages = {}
        for res in resources:
            rdtest.log.print(f"Checking {res.resourceId} ({res.name})")
            resourceUsages[res.resourceId] = replay.GetUsage(res.resourceId)
        # Special case id = 0 means check event flags
        replay.GetUsage(rd.ResourceId.Null())
        replay.Shutdown()
        rdtest.log.success("Checked")

    def run(self):
        dir_path = self.get_ref_path('', extra=True)

        for file in os.scandir(dir_path):
            with rdtest.log.auto_section(f"Checking:{file.name}"):
                self.resource_usage(file.path)

        rdtest.log.success("Checked all files")


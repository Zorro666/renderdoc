import rdtest
import renderdoc as rd

class VK_Workgroup_Zoo(rdtest.Workgroup_Zoo):
    demos_test_name = 'VK_Workgroup_Zoo'
    internal = False
    mtOption = rd.SetConfigSetting("Vulkan_Debug_EnableShaderDebugMT")

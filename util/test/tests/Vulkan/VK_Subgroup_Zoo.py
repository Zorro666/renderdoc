import rdtest
import renderdoc as rd

class VK_Subgroup_Zoo(rdtest.Subgroup_Zoo):
    demos_test_name = 'VK_Subgroup_Zoo'
    internal = False
    mtOption = rd.SetConfigSetting("Vulkan_Debug_EnableShaderDebugMT")

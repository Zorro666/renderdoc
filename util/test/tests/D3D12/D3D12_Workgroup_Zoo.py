import rdtest
import renderdoc as rd

class D3D12_Workgroup_Zoo(rdtest.Workgroup_Zoo):
    demos_test_name = 'D3D12_Workgroup_Zoo'
    internal = False
    mtOption = rd.SetConfigSetting("D3D12_DXILShaderDebugger_EnableMT")
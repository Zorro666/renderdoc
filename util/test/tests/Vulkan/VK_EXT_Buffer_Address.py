from .VK_Buffer_Address import *

class VK_EXT_Buffer_Address(VK_Buffer_Address):
    demos_test_name = 'VK_EXT_Buffer_Address'
    internal = False

    def check_capture(self):
        super(VK_EXT_Buffer_Address, self).check_capture()

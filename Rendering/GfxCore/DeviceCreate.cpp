#include "D3D11/DeviceD3D11.h"

namespace RBX
{
namespace Graphics
{

Device* Device::create(API api, void* windowHandle)
{
    if (api == API_Direct3D11)
        return new DeviceD3D11(windowHandle);

	throw RBX::runtime_error("Unsupported API: %d", api);
}

}
}

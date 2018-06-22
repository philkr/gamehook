#include "dxgi.h"
#include "dxgiswapchain.h"
#include "dx11/d3d11.h"
#include "util.h"
#pragma comment(lib, "dxgi.lib")

bool wrapSwapChain(IDXGISwapChain ** pp_s, D3D11Device * device) {
	if (pp_s && device) {
		ASSERT(*pp_s != nullptr);

		DXGI_SWAP_CHAIN_DESC desc;
		(*pp_s)->GetDesc(&desc);

		// Update the hook
		std::shared_ptr<D3D11Hook> hook = std::dynamic_pointer_cast<D3D11Hook>(device->h_);
		ASSERT(hook);
		if (hook->s_ != nullptr) {
			LOG(WARN) << "Multiple swapchains for device '"<<device<<"'!";
			hook->s_->Release();
			hook->s_ = nullptr;
		}
		*pp_s = new DXGISwapChain(device, *pp_s, hook);
		return true;
	}
	return false;
}

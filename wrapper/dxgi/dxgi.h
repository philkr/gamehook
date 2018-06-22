#pragma once
#include "dxgiswapchain.h"

struct DXGIHook: public DXGISwapChainHook {
	DXGIHook() = default;
};

bool wrapSwapChain(IDXGISwapChain ** pp_s, D3D11Device * device);

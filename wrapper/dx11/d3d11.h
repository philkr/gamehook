#pragma once
#include "dxgi/dxgi.h"
#include "dx11/d3d11device.h"
#include "dx11/d3d11devicecontext.h"


struct D3D11Hook: public DXGIHook, public D3D11DeviceHook, public D3D11DeviceContextHook {
	D3D11Hook() :DXGIHook(), D3D11DeviceHook(), D3D11DeviceContextHook() {}
};

bool wrapDeviceAndContext(ID3D11Device ** pp_d, ID3D11DeviceContext ** pp_c, std::shared_ptr<D3D11Hook> hook);


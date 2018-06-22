#include "d3d11.h"
#include "util.h"
#include "d3d11device.h"
#include "d3d11devicecontext.h"
#include <d3d11_3.h>
#pragma comment(lib, "d3d11.lib")

bool wrapDeviceAndContext(ID3D11Device ** pp_d, ID3D11DeviceContext ** pp_c, std::shared_ptr<D3D11Hook> hook) {
	if (pp_d) {
		ID3D11Device * d = *pp_d;
		ASSERT(d != nullptr);

		ID3D11DeviceContext * c = nullptr;
		d->GetImmediateContext(&c);
		ASSERT(c != nullptr);

		auto wrapped_d = new D3D11Device(d, hook);
		auto wrapped_c = new D3D11DeviceContext(wrapped_d, c, hook);
		wrapped_d->immediate_context_ = wrapped_c;
		wrapped_c->AddRef();

		*pp_d = wrapped_d;
		if (pp_c)
			*pp_c = wrapped_c;
		return true;
	}
	return false;
}

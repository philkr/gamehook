#include "cbuffer.h"
#include "util.h"

CBufferImp::CBufferImp(const std::string & name, size_t size, D3D11Hook * h):buf_(h, { (size+15) & ~15, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE }), name(name), memory_(size) {
}

void CBufferImp::write(const void * data, size_t offset, size_t size) {
	memcpy(memory_.data() + offset, data, size);
	changed_ = true;
}

ID3D11Buffer * CBufferImp::get() {
	if (changed_) {
		buf_.write(memory_.data(), memory_.size());
		changed_ = false;
	}
	return buf_;
}


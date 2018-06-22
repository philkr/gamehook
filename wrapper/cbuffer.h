#pragma once
#include "api.h"
#include "resource.h"

class CBufferImp: public CBuffer {
protected:
	D3DBuffer buf_;
	std::vector<uint8_t> memory_;
	bool changed_ = false;
public:
	std::string name;

	CBufferImp(const std::string & name, size_t size, D3D11Hook * h);

	virtual void write(const void * data, size_t offset, size_t size);
	ID3D11Buffer * get();
};


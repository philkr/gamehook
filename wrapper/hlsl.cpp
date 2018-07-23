#include "hlsl.h"
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include "util.h"
#include "config/config.h"
#include "opcodes.h"
#include <string>
#include <algorithm>
#include <iomanip>

/* MD5 magic */
#define F(x, y, z)			((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z)			((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z)			(((x) ^ (y)) ^ (z))
#define H2(x, y, z)			((x) ^ ((y) ^ (z)))
#define I(x, y, z)			((y) ^ ((x) | ~(z)))
#define STEP(f, a, b, c, d, x, t, s) \
	(a) += f((b), (c), (d)) + (x) + (t); \
	(a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
	(a) += (b)

#define SET(n) (*(uint32_t *)&block[(n) * 4])
#define GET(n) SET(n)
static void updateMD5(const uint8_t * block, uint32_t h[4]) {
	uint32_t a = h[0], b = h[1], c = h[2], d = h[3];

	/* Round 1 */
	STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7);
	STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12);
	STEP(F, c, d, a, b, SET(2), 0x242070db, 17);
	STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22);
	STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7);
	STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12);
	STEP(F, c, d, a, b, SET(6), 0xa8304613, 17);
	STEP(F, b, c, d, a, SET(7), 0xfd469501, 22);
	STEP(F, a, b, c, d, SET(8), 0x698098d8, 7);
	STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12);
	STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17);
	STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22);
	STEP(F, a, b, c, d, SET(12), 0x6b901122, 7);
	STEP(F, d, a, b, c, SET(13), 0xfd987193, 12);
	STEP(F, c, d, a, b, SET(14), 0xa679438e, 17);
	STEP(F, b, c, d, a, SET(15), 0x49b40821, 22);

	/* Round 2 */
	STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5);
	STEP(G, d, a, b, c, GET(6), 0xc040b340, 9);
	STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14);
	STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20);
	STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5);
	STEP(G, d, a, b, c, GET(10), 0x02441453, 9);
	STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14);
	STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20);
	STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5);
	STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9);
	STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14);
	STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20);
	STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5);
	STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9);
	STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14);
	STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20);

	/* Round 3 */
	STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4);
	STEP(H2, d, a, b, c, GET(8), 0x8771f681, 11);
	STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16);
	STEP(H2, b, c, d, a, GET(14), 0xfde5380c, 23);
	STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4);
	STEP(H2, d, a, b, c, GET(4), 0x4bdecfa9, 11);
	STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16);
	STEP(H2, b, c, d, a, GET(10), 0xbebfbc70, 23);
	STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4);
	STEP(H2, d, a, b, c, GET(0), 0xeaa127fa, 11);
	STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16);
	STEP(H2, b, c, d, a, GET(6), 0x04881d05, 23);
	STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4);
	STEP(H2, d, a, b, c, GET(12), 0xe6db99e5, 11);
	STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16);
	STEP(H2, b, c, d, a, GET(2), 0xc4ac5665, 23);

	/* Round 4 */
	STEP(I, a, b, c, d, GET(0), 0xf4292244, 6);
	STEP(I, d, a, b, c, GET(7), 0x432aff97, 10);
	STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15);
	STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21);
	STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6);
	STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10);
	STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15);
	STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21);
	STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6);
	STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10);
	STEP(I, c, d, a, b, GET(6), 0xa3014314, 15);
	STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21);
	STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6);
	STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10);
	STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15);
	STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21);

	h[0] += a;
	h[1] += b;
	h[2] += c;
	h[3] += d;
}

std::vector<uint8_t> hlslHash(const uint8_t * s, int n) {
	uint8_t block[64] = { 0 };
	uint32_t h[4] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };

	// Compute the md5
	int i = 0;
	for (i = 0; i + 64 <= n; i += 64) {
		memcpy(block, s + i, 64);
		updateMD5(block, h);
	}
	// Compute the last block
	int remain = n - i;
	memset(block, 0, sizeof(block));
	if (remain < 56) {
		// Write one block
		SET(0) = 8 * n;
		SET(15) = (n << 1) | 1;
		memcpy(block + 4, s + i, remain);
		block[remain + 4] = 0x80;
		updateMD5(block, h);
	}
	else {
		// Write two block
		memcpy(block, s + i, remain);
		block[remain] = 0x80;
		updateMD5(block, h);

		memset(block, 0, sizeof(block));
		SET(0) = 8 * n;
		SET(15) = (n << 1) | 1;
		updateMD5(block, h);
	}
	return std::vector<uint8_t>((uint8_t*)h, (uint8_t*)(h + 4));
}
uint32_t writeString(std::vector<uint8_t> & buf, const std::string & s) {
	if (!s.size()) return 0;
	uint32_t r = (uint32_t)buf.size();
	buf.insert(buf.end(), s.begin(), s.end());
	buf.push_back(0);
	return r;
}

template<int V> struct RDEF_ {};
template<> struct RDEF_<0x400> {
	static const uint32_t HeaderSize = 7 * sizeof(uint32_t);
	static const uint32_t CBufferSize = 6 * sizeof(uint32_t);
	static const uint32_t VariableSize = 6 * sizeof(uint32_t);
	static const uint32_t TypeSize = 4 * sizeof(uint32_t);
	static const uint32_t MemberSize = 3 * sizeof(uint32_t);
	static const uint32_t BindingSize = 8 * sizeof(uint32_t);
};
template<> struct RDEF_<0x500> {
	static const uint32_t HeaderSize = 15 * sizeof(uint32_t);
	static const uint32_t CBufferSize = 6 * sizeof(uint32_t);
	static const uint32_t VariableSize = 10 * sizeof(uint32_t);
	static const uint32_t TypeSize = 9 * sizeof(uint32_t);
	static const uint32_t MemberSize = 3 * sizeof(uint32_t);
	static const uint32_t BindingSize = 8 * sizeof(uint32_t);
};
template<> struct RDEF_<0x501> { // Note HLSL 5.1 is currently NOT supported, the RDEF can be read, but instructions changed too much to merge with 5.0 or 4.x
	static const uint32_t HeaderSize = 15 * sizeof(uint32_t);
	static const uint32_t CBufferSize = 6 * sizeof(uint32_t);
	static const uint32_t VariableSize = 10 * sizeof(uint32_t);
	static const uint32_t TypeSize = 9 * sizeof(uint32_t);
	static const uint32_t MemberSize = 3 * sizeof(uint32_t);
	static const uint32_t BindingSize = 10 * sizeof(uint32_t);
};

struct RDEF {
	struct CBuffer {
		struct Member;
		struct Type {
			uint16_t vclass, type, rows, cols, els;
			uint16_t n_var;
			uint32_t o_var;
			uint32_t o_parent_type;
			uint32_t unknown[3];
			uint32_t o_parent_name;

			std::vector<Member> variable;
			std::string parent_type, parent_name;
			template<int V>
			void parse(uint32_t o, const uint8_t * buf, size_t n) {
				unknown[0] = unknown[1] = unknown[2] = 0;
				memcpy(this, buf + o, RDEF_<V>::TypeSize);

				// Read the nested variables
				for (uint32_t i = 0; i < n_var; i++) {
					Member v;
					v.parse<V>(o_var + RDEF_<V>::MemberSize * i, buf, n);
					variable.push_back(v);
				}
				if (V >= 0x500) {
					if (o_parent_type) parent_type = std::string((const char*)buf + o_parent_type);
					if (o_parent_name) parent_name = std::string((const char*)buf + o_parent_name);
				}
			}
			template<int V>
			void write(uint32_t o, std::vector<uint8_t> & buf) {
				// Write the nested variables
				o_var = n_var = 0;
				if (variable.size()) {
					o_var = (uint32_t)buf.size();
					n_var = (uint16_t)variable.size();
					buf.resize(o_var + n_var*RDEF_<V>::MemberSize);
					for (uint32_t i = 0; i < n_var; i++)
						variable[i].write<V>(o_var + i*RDEF_<V>::MemberSize, buf);
				}
				if (V >= 0x500) {
					o_parent_name = writeString(buf, parent_name);
					o_parent_type = writeString(buf, parent_type);
				}
				memcpy(&buf[o], this, RDEF_<V>::TypeSize);
			}
			bool operator==(const Type & o) const {
				return vclass == o.vclass && type == o.type && rows == o.rows && cols == o.cols && els == o.els && variable == o.variable;
			}

			operator std::string() const {
				std::string tpe = "UNKNOWN";
				if (type == 0) tpe = "void";
				if (type == 1) tpe = "bool";
				if (type == 2) tpe = "int";
				if (type == 3) tpe = "float";

				if (vclass == 0) return tpe; // Scalar
				if (vclass == 1) return tpe+std::to_string(rows*cols); // Vector
				if (vclass == 2) return tpe + std::to_string(rows) + "x" + std::to_string(cols); // RowMatrix
				if (vclass == 3) return tpe + std::to_string(rows) + "x" + std::to_string(cols); // ColMatrix
				if (vclass == 4) return "Object";
				if (vclass == 5) return "Struct";
				if (vclass == 6) return "InterfaceClass";
				if (vclass == 7) return "InterfacePointer";
				return "UNKNOWN";
			}
		};
		struct Member {
			uint32_t o_name;
			uint32_t o_type;
			uint32_t o_buf;
			std::string name;
			Type type;
			template<int V>
			void parse(uint32_t o, const uint8_t * buf, size_t n) {
				memcpy(this, buf + o, RDEF_<V>::MemberSize);

				// Read all members
				name = std::string((const char*)buf + o_name);

				type.parse<V>(o_type, buf, n);
			}
			template<int V>
			void write(uint32_t o, std::vector<uint8_t> & buf) {
				// Write the name string
				o_name = writeString(buf, name);

				// Write the type struct
				o_type = (uint32_t)buf.size();
				buf.resize(o_type + RDEF_<V>::TypeSize);
				type.write<V>(o_type, buf);

				// Write the cbuffer data
				memcpy(&buf[o], this, RDEF_<V>::MemberSize);
			}
			bool operator==(const Member & o) const {
				return o_buf == o.o_buf && type == o.type && name == o.name;
			}
		};
		struct Variable {
			uint32_t o_name;
			uint32_t o_buf;
			uint32_t size;
			uint32_t flags;
			uint32_t o_type;
			uint32_t o_def;
			int32_t tex_start, tex_size, sampler_start, sampler_size;
			std::string name;
			Type type;
			std::vector<uint8_t> def;
			template<int V>
			void parse(uint32_t o, const uint8_t * buf, size_t n) {
				tex_start = sampler_start = -1;
				tex_size = sampler_size = 0;
				memcpy(this, buf + o, RDEF_<V>::VariableSize);
				// Read all members
				name = std::string((const char*)buf + o_name);
				type.parse<V>(o_type, buf, n);
				if (o_def)
					def = std::vector<uint8_t>(buf + o_def, buf + o_def + size);
			}
			template<int V>
			void write(uint32_t o, std::vector<uint8_t> & buf) {
				// Write the name string
				o_name = writeString(buf, name);

				// Write the type struct
				o_type = (uint32_t)buf.size();
				buf.resize(o_type + RDEF_<V>::TypeSize);
				type.write<V>(o_type, buf);

				// Write the default value
				o_def = 0;
				if (def.size()) {
					o_def = (uint32_t)buf.size();
					buf.insert(buf.end(), def.begin(), def.end());
				}

				// Write the cbuffer data
				memcpy(&buf[o], this, RDEF_<V>::VariableSize);
			}
			bool operator==(const Variable & o) const {
				return o_buf == o.o_buf && type == o.type && name == o.name && size == o.size && flags == o.flags;
			}
		};
		uint32_t o_name;
		uint32_t n_var;
		uint32_t o_var;
		uint32_t size;
		uint32_t flags;
		uint32_t type;
		std::string name;
		std::vector<Variable> variable;
		template<int V>
		void parse(uint32_t o, const uint8_t * buf, size_t n) {
			memcpy(this, buf + o, RDEF_<V>::CBufferSize);
			name = std::string((const char*)buf + o_name);
			for (uint32_t i = 0; i < n_var; i++) {
				Variable v;
				v.parse<V>(o_var + RDEF_<V>::VariableSize * i, buf, n);
				variable.push_back(v);
			}
		}
		template<int V>
		void write(uint32_t o, std::vector<uint8_t> & buf) {
			// Write all the variables
			o_var = (uint32_t)buf.size();
			n_var = (uint32_t)variable.size();
			buf.resize(o_var + n_var*RDEF_<V>::VariableSize);
			for (uint32_t i = 0; i < n_var; i++)
				variable[i].write<V>(o_var + i*RDEF_<V>::VariableSize, buf);

			// Write the name string
			o_name = writeString(buf, name);

			// Write the cbuffer data
			memcpy(&buf[o], this, RDEF_<V>::CBufferSize);
		}
		// Is the current buffer a subset of o
		bool operator<=(const CBuffer & o) const {
			if (type != o.type && name != o.name && flags != o.flags)
				return false;
			for (const auto & v : variable) {
				bool match = false;
				for (const auto & vv: o.variable)
					if (v == vv) {
						match = true;
						break;
					}
				if (!match)
					return false;
			}
			return true;
		}
	};
	struct Binding {
		uint32_t o_name;
		uint32_t input_type;
		uint32_t return_type;
		uint32_t view_dim;
		uint32_t n_sample;
		uint32_t bind_point;
		uint32_t bind_count;
		uint32_t shader_input;
		uint32_t other1;
		uint32_t other2;

		std::string name;
		template<int V>
		void parse(uint32_t o, const uint8_t * buf, size_t n) {
			memcpy(this, buf + o, RDEF_<V>::BindingSize);
			name = std::string((const char*)buf + o_name);
		}
		template<int V>
		void write(uint32_t o, std::vector<uint8_t> & buf) {
			// Write the name string
			o_name = writeString(buf, name);

			// Write the binding data
			memcpy(&buf[o], this, RDEF_<V>::BindingSize);
		}
		bool collide(const Binding & o) const {
			return (bind_point <= o.bind_point && o.bind_point < bind_point + bind_count) || (o.bind_point <= bind_point && bind_point < o.bind_point + o.bind_count);
		}
		bool operator==(const Binding & o) const {
			return input_type == o.input_type && return_type == o.return_type && name == o.name && view_dim == o.view_dim && n_sample == o.n_sample && shader_input == o.shader_input && bind_count == o.bind_count;
		}
	};
	uint32_t n_const, o_const;
	uint32_t n_bind, o_bind;
	uint16_t version, type;
	uint32_t flags;
	uint32_t o_creator;
	uint32_t RD11;
	uint32_t unknown[7] = { 0 }; // 7 other uint32_t, no clue what they do
	std::string creator;
	std::vector<CBuffer> cbuffer;
	std::vector<Binding> binding;
template<int V>
	void parseV(const uint8_t * buf, size_t n) {
		memcpy(this, buf, RDEF_<V>::HeaderSize);

		for (uint32_t i = 0; i < n_const; i++) {
			CBuffer cb;
			cb.parse<V>(o_const + i * RDEF_<V>::CBufferSize, buf, n);
			cbuffer.push_back(cb);
		}
		for (uint32_t i = 0; i < n_bind; i++) {
			Binding b;
			b.parse<V>(o_bind + i * RDEF_<V>::BindingSize, buf, n);
			binding.push_back(b);
		}
		creator = std::string((const char*)buf + o_creator);
	}
	void parse(const uint8_t * buf, size_t n) {
		memcpy(this, buf, RDEF_<0x400>::HeaderSize);
		//if (version == 0x501)
			//return parseV<0x501>(buf, n);
		if (version == 0x500)
			return parseV<0x500>(buf, n);
		if (version == 0x400)
			return parseV<0x400>(buf, n);
		if (version == 0x401)
			return parseV<0x400>(buf, n);
		LOG(ERR) << "Unknown shader version " << std::hex << version << " using 400";
		return parseV<0x400>(buf, n);
	}
	template<int V>
	std::vector<uint8_t> writeV() {
		std::vector<uint8_t> buf;
		// Allocate the memory the header
		buf.resize(RDEF_<V>::HeaderSize);

		// Write all the constant buffers
		o_const = (uint32_t)buf.size();
		n_const = (uint32_t)cbuffer.size();
		buf.resize(o_const + n_const*RDEF_<V>::CBufferSize);
		for (uint32_t i = 0; i < n_const; i++)
			cbuffer[i].write<V>(o_const + i*RDEF_<V>::CBufferSize, buf);

		// Write all the binding buffers
		o_bind = (uint32_t)buf.size();
		n_bind = (uint32_t)binding.size();
		buf.resize(o_bind + n_bind*RDEF_<V>::BindingSize);
		for (uint32_t i = 0; i < n_bind; i++)
			binding[i].write<V>(o_bind + i*RDEF_<V>::BindingSize, buf);

		// Write the creator string
		o_creator = writeString(buf, creator);

		// Write the RDEF data
		memcpy(&buf[0], this, RDEF_<V>::HeaderSize);

		return buf;
	}
	std::vector<uint8_t> write() {
		//if (version == 0x501)
			//return writeV<0x501>();
		if (version == 0x500)
			return writeV<0x500>();
		if (version == 0x400)
			return writeV<0x400>();
		LOG(ERR) << "Unknown shader version " << std::hex << version << " using 400";
		return writeV<0x400>();
	}
	RDEF() {}
	RDEF(const HLSL::ByteCode & code) {
		parse(code.data(), code.size());
	}
};

static const uint32_t SGNHeaderSize = 2 * sizeof(uint32_t);
static const uint32_t SGNDefinitionSize = 6 * sizeof(uint32_t);

struct SGN {
	struct Definition {
		uint32_t o_name;
		uint32_t sem_id;
		uint32_t sys_id;
		uint32_t ctype;
		uint32_t reg;
		uint8_t mask, read_mask;
		uint16_t unused;

		std::string name;
		void parse(uint32_t o, const uint8_t * buf, size_t n) {
			memcpy(this, buf + o, SGNDefinitionSize);
			// Read all members
			name = std::string((const char*)buf + o_name);
		}
		void write(uint32_t o, std::vector<uint8_t> & buf) {
			// Write the name string
			o_name = writeString(buf, name);

			// Write the cbuffer data
			memcpy(&buf[o], this, SGNDefinitionSize);
		}
		bool collide(const Definition & o) const {
			return (sem_id == o.sem_id && name == o.name) || reg == o.reg;
		}
		bool operator==(const Definition & o) const {
			return sem_id == o.sem_id && sys_id == o.sys_id && ctype == o.ctype && reg == o.reg && name == o.name;
		}
	};

	uint32_t n_definitions;
	uint32_t eight;
	std::vector<Definition> definition;
	void parse(const uint8_t * buf, size_t n) {
		memcpy(this, buf, SGNHeaderSize);

		// Read all definitions
		for (uint32_t i = 0; i < n_definitions; i++) {
			Definition d;
			d.parse(SGNHeaderSize + i*SGNDefinitionSize, buf, n);
			definition.push_back(d);
		}
	}
	std::vector<uint8_t> write() {
		std::vector<uint8_t> buf;
		// Allocate the memory the header
		buf.resize(SGNHeaderSize);

		// Write the name string
		n_definitions = (uint32_t)definition.size();
		buf.resize(SGNHeaderSize + n_definitions*SGNDefinitionSize);
		for (uint32_t i = 0; i < n_definitions; i++)
			definition[i].write(SGNHeaderSize + i*SGNDefinitionSize, buf);

		// Write the cbuffer data
		memcpy(&buf[0], this, SGNHeaderSize);

		return buf;
	}
	SGN() {}
	SGN(const HLSL::ByteCode & c) {
		parse(c.data(), c.size());
	}
};

#define OPCODE_TYPE_MASK 0x00007ff
#define OPCODE_EXTENDED_MASK 0x80000000
#define DECODE_OPCODE_TYPE(opcode) ((opcode)&OPCODE_TYPE_MASK)
#define INSTRUCTION_LENGTH_MASK 0x7f000000
#define INSTRUCTION_LENGTH_SHIFT 24
#define DECODE_INSTRUCTION_LENGTH(opcode) (((opcode)&INSTRUCTION_LENGTH_MASK)>>INSTRUCTION_LENGTH_SHIFT)

struct SHDR {
	struct Operation {
		struct Operand {
			enum Component {
				X = 1,
				Y = 2,
				Z = 4,
				W = 8,
			};
			uint8_t component_mask = 0; // Bitmask of components used
			uint8_t type = 0; // Operand type (0: Temp, 1: input, 2: output, 3: indextemp, ..., 7: Resource, 8: CB, ...
			uint64_t index[3] = { 0,0,0 };
			uint32_t op_no = 0;
		};
	protected:
		template<bool can_edit>
		uint32_t traverseOp(uint32_t & o, uint32_t n_op, std::function<void(Operand &)> f) {
			uint32_t op = args[o++];
			if (!op) return 0;

			Operand operand = { 0, (op >> 12) & 0xff, {0,0,0}, n_op };

			// Parse the selection mask
			uint32_t nc = (op & 3);
			if (nc == 2) { // Four compoments
				nc = 4;
				uint8_t selection_mode = ((op >> 2) & 3);
				if (selection_mode == 0 /* MASK */)
					operand.component_mask = ((op >> 4) & 0xf);
				else if (selection_mode == 1 /*SWIZZLE*/)
					operand.component_mask = (1 << ((op >> 4) & 3)) | (1 << ((op >> 6) & 3)) | (1 << ((op >> 8) & 3)) | (1 << ((op >> 10) & 3));
				else if (selection_mode == 2 /*SELECT_1*/)
					operand.component_mask = 1 << (((op >> 4) & 3));
			}
			
			// Read the indices
			uint32_t op_o[3] = { 0 };
			uint8_t index_dim = (op >> 20) & 3;
			uint32_t r = 1;
			for (uint8_t i = 0; i < index_dim; i++) {
				uint8_t index_rep = (op >> (22 + 3 * i)) & 7;
				if (index_rep == 0 /*IMMEDIATE32*/) {
					op_o[i] = o;
					operand.index[i] = args[o++];
				}
				else if (index_rep == 1 /*IMMEDIATE64*/) {
					op_o[i] = o;
					operand.index[i] = *(uint64_t*)(&args[o]);
					o += 2;
				}
				else if (index_rep == 2 /*RELATIVE*/) {
					r += traverseOp<can_edit>(o, n_op + r, f);
					operand.index[i] = -1;
				}
				else if (index_rep == 3 /*IMMEDIATE32+RELATIVE*/) {
					o += 1;
					r += traverseOp<can_edit>(o, n_op + r, f);
					operand.index[i] = -1;
				}
				else if (index_rep == 4 /*IMMEDIATE64+RELATIVE*/) {
					o += 2;
					r += traverseOp<can_edit>(o, n_op + r, f);
					operand.index[i] = -1;
				}
			}
			if (operand.type == OperandType::IMMEDIATE32)
				o += nc;
			if (operand.type == OperandType::IMMEDIATE64)
				o += 2*nc;
			// Call the tranverse op
			f(operand);

			if (can_edit) {
				// Update any indices we might need
				for (uint8_t i = 0; i < index_dim; i++) {
					uint8_t index_rep = (op >> (22 + 3 * i)) & 7;
					if (index_rep == 0 /*IMMEDIATE32*/)
						args[op_o[i]] = (uint32_t)operand.index[i];
					else if (index_rep == 1 /*IMMEDIATE64*/)
						*(uint64_t*)(&args[op_o[i]]) = operand.index[i];
				}
			}
			return r;
		}
		template<bool can_edit>
		uint32_t traverseOps(std::function<void(Operand & op)> f) {
			uint32_t n_op = 0, o = !!(OPCODE_EXTENDED_MASK & opcode);
			// Parse the current op
			while (o < args.size())
				n_op += traverseOp<can_edit>(o, n_op, f);
			return n_op;
		}
	public:
		uint32_t opcode;
		std::vector<uint32_t> args;
		uint32_t parse(uint32_t o, const uint32_t * buf, size_t n) {
			opcode = buf[o];
			uint32_t size = DECODE_INSTRUCTION_LENGTH(opcode);
			if (DECODE_OPCODE_TYPE(opcode) == OpCode::CUSTOMDATA)
				size = buf[o + 1];
			if (size)
				args = std::vector<uint32_t>(buf + o + 1, buf + o + size);
			return size;
		}
		void write(std::vector<uint32_t> & buf) {
			if (DECODE_OPCODE_TYPE(opcode) != OpCode::CUSTOMDATA && !(opcode & INSTRUCTION_LENGTH_MASK))
				opcode = (opcode & 0x80ffffff) | ((((uint32_t)args.size() + 1) << INSTRUCTION_LENGTH_SHIFT) & INSTRUCTION_LENGTH_MASK);

			buf.push_back(opcode);
			if (args.size())
				buf.insert(buf.end(), args.begin(), args.end());
		}
		uint32_t traverseOperands(std::function<void(Operand & op)> f) {
			return traverseOps<true>(f);
		}
		uint32_t traverseOperands(std::function<void(const Operand & op)> f) const {
			return const_cast<Operation*>(this)->traverseOps<false>(f);
		}
	};
	uint16_t version;
	uint16_t type;
	uint32_t n_word;
	std::vector<Operation> operation;
	void parse(const uint8_t * buf, size_t n) {
		const uint32_t * buf32 = (const uint32_t*)buf;
		memcpy(this, buf, 2 * sizeof(uint32_t));
		for (uint32_t i = 2; i < n_word;) {
			Operation o;
			i += o.parse(i, buf32, n);
			operation.push_back(o);
		}
	}
	std::vector<uint8_t> write() {
		std::vector<uint32_t> buf;
		buf.resize(2);
		for (auto & o : operation)
			o.write(buf);
		n_word = (uint32_t)buf.size();
		memcpy(&buf[0], this, 2 * sizeof(uint32_t));
		return std::vector<uint8_t>((uint8_t*)&buf[0], (uint8_t*)(&buf[0] + buf.size()));
	}
	SHDR() {}
	SHDR(const HLSL::ByteCode & c) {
		parse(c.data(), c.size());
	}
};

// Chunk names
const ChunkName DXBCName = 0x43425844;
const ChunkName RDEFName = 0x46454452;
const ChunkName ISGNName = 0x4e475349;
const ChunkName OSGNName = 0x4e47534f;
const ChunkName SHDRName = 0x52444853;
const ChunkName SHEXName = 0x58454853;

#pragma warning( push )
#pragma warning( disable : 4200 )
struct DXBCHeader {
	ChunkName magic;
	uint32_t checksum[4];
	uint32_t one;
	uint32_t total_size;
	uint32_t total_chunk;
	uint32_t chunk_offset[];
};
#pragma warning( pop )

struct Chunk {
	ChunkName name;
	uint32_t size;
};

void HLSL::parse(const ByteCode & code) {
	const DXBCHeader * header = (const DXBCHeader*)&code[0];
	for (uint32_t i = 0; i < header->total_chunk; i++) {
		size_t o = header->chunk_offset[i];
		const Chunk * chunk = (const Chunk*)(code.data() + o);
		chunks_.push_back(std::make_pair(chunk->name, ByteCode(code.begin()+o+sizeof(Chunk), code.begin()+o+sizeof(Chunk)+chunk->size)));
	}
}
HLSL::ByteCode HLSL::write() const {
	ByteCode r(sizeof(DXBCHeader) + chunks_.size() * sizeof(uint32_t));

	// Create the header
	DXBCHeader * header = (DXBCHeader*)&r[0];
	header->magic = DXBCName;
	header->one = 1;

	// Write all chunks
	for (uint32_t i = 0; i < chunks_.size(); i++) {
		// Write the file header
		header = (DXBCHeader*)&r[0];
		header->chunk_offset[i] = (uint32_t)r.size();
			
		// Write the chunk header
		Chunk c{chunks_[i].first, (uint32_t)chunks_[i].second.size()};
		r.insert(r.end(), (uint8_t*)&c, (uint8_t*)(&c+1));
			
		// Write the chunk
		r.insert(r.end(), chunks_[i].second.begin(), chunks_[i].second.end());
	}

	// Create the checksum and call it a day
	header = (DXBCHeader*)&r[0];
	header->total_size = (uint32_t)r.size();
	header->total_chunk = (uint32_t)chunks_.size();

	// Recompute the hash
	std::vector<uint8_t> hash = hlslHash(r.data() + 20, (uint32_t)r.size() - 20);
	memcpy(header->checksum, hash.data(), hash.size());

	return r;
}
bool HLSL::empty() const {
	// Is the HLSL shader body empty?
	for (const auto & c : chunks_) {
		if (c.first == SHDRName || c.first == SHEXName) {
			SHDR s(c.second);
			for (const auto & i : s.operation)
				if (DECODE_OPCODE_TYPE(i.opcode) != OpCode::DCL_GLOBAL_FLAGS && DECODE_OPCODE_TYPE(i.opcode) != OpCode::RET)
					return false;
		}
	}
	return true;
}

bool HLSL::usesDiscard() const {
	for (auto & c : chunks_)
		if (c.first == SHDRName || c.first == SHEXName) {
			SHDR s(c.second);
			for (const auto & op : s.operation)
				if (DECODE_OPCODE_TYPE(op.opcode) == OpCode::DISCARD)
					return true;
		}
	return false;
}
bool HLSL::writesDepth() const {
	for (auto & c : chunks_)
		if (c.first == OSGNName) {
			SGN osgn(c.second);
			for (const auto & d : osgn.definition)
				if (d.name == "SV_depth")
					return true;
		}
	return false;
}
void HLSL::setEarlyDepthStencil(bool v) {
	for (auto & c : chunks_)
		if (c.first == SHDRName || c.first == SHEXName) {
			SHDR s(c.second);
			uint32_t eds_mask = 0x00002000;
			for (auto & op : s.operation)
				if (DECODE_OPCODE_TYPE(op.opcode) == OpCode::DCL_GLOBAL_FLAGS)
					op.opcode = (op.opcode & (~eds_mask)) | (v ? eds_mask : 0);

			c.second = s.write();
		}
}
void HLSL::addRetAfterDiscard() {
	for (auto & c : chunks_)
		if (c.first == SHDRName || c.first == SHEXName) {
			SHDR s(c.second);
			uint32_t eds_mask = 0x00002000;
			std::vector<SHDR::Operation> new_ops;
			for (auto op : s.operation) {
				new_ops.push_back(op);
				if (DECODE_OPCODE_TYPE(op.opcode) == OpCode::DISCARD) {
					op.opcode = (op.opcode & (~OPCODE_TYPE_MASK)) | OpCode::RETC;
					new_ops.push_back(op);
				}
			}
			s.operation = new_ops;
			c.second = s.write();
		}
}
HLSL::ShaderType HLSL::type() const {
	for (const auto & c : chunks_)
		if (c.first == SHDRName || c.first == SHEXName) {
			SHDR s(c.second);
			return (ShaderType)s.type;
		}
	return INVALID_SHADER;
}
std::vector<HLSL::Binding> HLSL::listInput() const {
	std::vector<Binding> r;
	for (const auto & ck : chunks_)
		if (ck.first == ISGNName) {
			SGN isgn(ck.second);
			for (const auto & d : isgn.definition) {
				//if (d.sem_id == 0)
					//r.push_back(d.name);
				r.push_back({ d.name + std::to_string(d.sem_id), d.reg });
			}
		}
	return r;
}
std::vector<HLSL::Binding> HLSL::listOutput() const {
	std::vector<Binding> r;
	for (const auto & ck : chunks_)
		if (ck.first == OSGNName) {
			SGN osgn(ck.second);
			for (const auto & d : osgn.definition) {
				//if (d.sem_id == 0)
					//r.push_back(d.name);
				r.push_back({ d.name + std::to_string(d.sem_id), d.reg });
			}
		}
	return r;
}
std::vector<HLSL::Buffer> HLSL::listBuffers(uint32_t type_mask) const {
	std::vector<Buffer> r;
	for (const auto & ck : chunks_)
		if (ck.first == RDEFName) {
			RDEF rdef(ck.second);
			for (const auto & b : rdef.binding)
				if (type_mask & (1<<b.input_type)) {
					Buffer rb = { b.name, b.bind_point,{} };

					for (const auto & cb : rdef.cbuffer)
						if (cb.name == b.name) {
							for (const auto & v : cb.variable)
								rb.variables.push_back({ v.name, v.type, v.o_buf });
						}
					r.push_back(rb);
				}
		}
		return r;
}
std::vector<HLSL::Buffer> HLSL::listCBuffers() const {
	return listBuffers(1 /* cbuffer=0*/);
}
std::vector<HLSL::Buffer> HLSL::listSBuffers() const {
	return listBuffers(0x32 /* tbuffer=1  RWTexture=4  structured=5*/);
}
std::vector<HLSL::Binding> HLSL::listTextures() const {
	std::vector<Binding> r;
	for (const auto & ck : chunks_)
		if (ck.first == RDEFName) {
			RDEF rdef(ck.second);
			for (const auto & b : rdef.binding)
				if (b.input_type == 2)
					r.push_back({ b.name, b.bind_point });
		}
	return r;
}

static uint32_t OP_ID(uint8_t type, uint8_t comp, uint32_t index) {
	return (((uint32_t)type) << 24) | ((comp & 3) << 22) | (index & 0xffff);
}

HLSL HLSL::subset(const std::vector<std::string> & named_outputs) {
	std::unordered_map<std::string, uint32_t> remap;
	for (const Binding & b : listOutput())
		remap[b.name] = b.bind_point;

	std::vector<uint32_t> outputs;
	for (const std::string & n : named_outputs)
		outputs.push_back(remap[n]);

	return subset(outputs);
}

void HLSL::renameOutput(const std::string & old_name, const std::string & new_name, int sys_id) {
	int old_binding = -1;
	for (auto & ck : chunks_)
		if (ck.first == OSGNName) {
			SGN osgn(ck.second);
			for (auto & d : osgn.definition)
				if (d.name == old_name) {
					d.name = new_name;
					if (sys_id>=0)
						d.sys_id = sys_id;
				}
			ck.second = osgn.write();
		}
}

void HLSL::renameCBuffer(const std::string & old_name, const std::string & new_name, int new_slot) {
	int old_binding = -1;
	for (auto & ck : chunks_)
		if (ck.first == RDEFName) {
			RDEF rdef(ck.second);
			if (new_slot == -1) {
				// Find an empty slot
				std::unordered_set<uint32_t> used_bindings;
				for (const auto & b : rdef.binding)
					if (b.input_type == InputType::CBUFFER)
						used_bindings.insert(b.bind_point);
				for (new_slot = 0; new_slot <= 14; new_slot++)
					if (!used_bindings.count(new_slot))
						break;
			}
			for (auto & b : rdef.binding)
				if (b.input_type == InputType::CBUFFER && b.name == old_name) {
					old_binding = b.bind_point;
					b.bind_point = new_slot;
					b.name = new_name;
				}
			for (auto & b : rdef.cbuffer)
				if (b.name == old_name)
					b.name = new_name;
			ck.second = rdef.write();
		}
	for (auto & ck : chunks_)
		if (ck.first == SHDRName || ck.first == SHEXName) {
			SHDR shdr(ck.second);
			typedef SHDR::Operation::Operand Operand;
			for (auto & o : shdr.operation)
				o.traverseOperands([&](Operand & op) {if (op.type == OperandType::CONSTANT_BUFFER && op.index[0] == old_binding) op.index[0] = new_slot; });
			ck.second = shdr.write();
		}
}

HLSL HLSL::subset(const std::vector<uint32_t>& outputs) {
	// Just copy all the code and let the compiler figure out which parts to remove
	HLSL result = *this;
	std::unordered_set<uint32_t> output_set(outputs.begin(), outputs.end());
	for (auto & c : result.chunks_)
		if (c.first == SHDRName || c.first == SHEXName) {
			SHDR s(c.second);
			typedef SHDR::Operation::Operand Operand;
			const auto & ops = s.operation;
			std::vector<SHDR::Operation> new_ops;
			for (size_t i = 0; i < ops.size(); i++) {
				// Parse the input and outputs
				Operand ret;
				// For all inputs figure out where they came from
				uint32_t n_op = ops[i].traverseOperands([&](const Operand & op) {
					if (op.op_no == 0) // Output
						ret = op;
				});
				// Update last_written
				if (!n_op || (ret.type != OperandType::OUTPUT || output_set.count((uint32_t)ret.index[0]) ) ) {
					new_ops.push_back(ops[i]);
				}
			}
			s.operation = new_ops;
			c.second = s.write();
		}

	for (auto & c : result.chunks_) {
		if (c.first == OSGNName) {
			uint8_t op_type = OperandType::OUTPUT;
			SGN sgn(c.second);
			// Filter the input/output definitions
			std::vector<SGN::Definition> new_def;
			for (auto & d : sgn.definition)
				if (output_set.count(d.reg))
					new_def.push_back(d);
			sgn.definition = new_def;
			c.second = sgn.write();
		}
	}

	return result;
}
//HLSL HLSL::subset(const std::vector<uint32_t>& outputs) {
//	HLSL result = *this;
//	std::unordered_set<uint32_t> used_resources;
//
//	for (auto & c : result.chunks_)
//		if (c.first == SHDRName || c.first == SHEXName) {
//			SHDR s(c.second);
//			typedef SHDR::Operation::Operand Operand;
//			const auto & ops = s.operation;
//			std::unordered_map<uint32_t, uint32_t> last_written;
//			std::vector<std::vector<uint32_t> > dep_graph(ops.size()); // What code lines does a certain line of code depend on?
//			for (size_t i = 0; i < ops.size(); i++) {
//				// TODO: Handle blocks
//
//				// Parse the input and outputs
//				Operand ret;
//				// For all inputs figure out where they came from
//				std::unordered_set<uint32_t> depends_on_line;
//				uint32_t n_op = ops[i].traverseOperands([&](const Operand & op) {
//					if (op.op_no == 0) // Output
//						ret = op;
//					else { // Input
//						uint8_t mask = 1;
//						if (op.component_mask) mask = op.component_mask;
//						for (uint32_t k = 0; k < 4; k++)
//							if ((1 << k) & mask) {
//								uint32_t id = OP_ID(op.type, k, (uint32_t)op.index[0]);
//								auto j = last_written.find(id);
//								if (j != last_written.end())
//									depends_on_line.insert(j->second);
//							}
//					}
//				});
//				// Update last_written
//				if (n_op) {
//					// Update the write info
//					uint8_t mask = 1;
//					if (ret.component_mask) mask = ret.component_mask;
//					for (uint32_t k = 0; k < 4; k++)
//						if ((1 << k) & mask) {
//							uint32_t id = OP_ID(ret.type, k, (uint32_t)ret.index[0]);
//							last_written[id] = (uint32_t)i;
//						}
//				}
//				// Build the dependency graph
//				dep_graph[i] = std::vector<uint32_t>(depends_on_line.begin(), depends_on_line.end());
//			}
//
//			// Figure out what output lines were used
//			std::unordered_set<uint32_t> used_lines;
//			std::unordered_set<uint32_t> output_set( outputs.begin(), outputs.end() );
//			for (uint32_t o : outputs)
//				for (uint32_t k = 0; k < 4; k++) {
//					uint32_t id = OP_ID(OperandType::OUTPUT, k, o);
//					auto i = last_written.find(id);
//					if (i != last_written.end())
//						used_lines.insert(i->second);
//				}
//
//			// Add all sort of dcls
//			for (uint32_t i = 0; i < ops.size(); i++) {
//				uint32_t op = DECODE_OPCODE_TYPE( ops[i].opcode );
//				switch (op) {
//				case OpCode::DCL_GLOBAL_FLAGS:
//				case OpCode::DCL_TEMPS:
//				case OpCode::DCL_INDEXABLE_TEMP:
//					used_lines.insert(i);
//					break;
//				case OpCode::DCL_OUTPUT:
//				case OpCode::DCL_OUTPUT_SGV:
//				case OpCode::DCL_OUTPUT_SIV:
//					{
//						bool use = false;
//						ops[i].traverseOperands([&](const Operand & op) { if (op.type == OperandType::OUTPUT && output_set.count((uint32_t)op.index[0])) use = true; });
//						if (use)
//							used_lines.insert(i);
//						break;
//					}
//				}
//			}
//
//			// Traverse the dependency graph to figure out which lines we should keep (plus some DCLs)
//			for (int i = (int)ops.size() - 1; i >= 0; i--)
//				if (used_lines.count(i))
//					for (uint32_t j : dep_graph[i])
//						used_lines.insert(j);
//
//			// Copy all the used ops
//			std::vector<SHDR::Operation> new_ops;
//			for (uint32_t i = 0; i < ops.size(); i++)
//				if (used_lines.count(i)) {
//					new_ops.push_back(ops[i]);
//					// Update all used resources
//					ops[i].traverseOperands([&used_resources](const Operand & op) {
//						switch (op.type) {
//						case OperandType::INPUT:
//						case OperandType::OUTPUT:
//						case OperandType::SAMPLER:
//						case OperandType::RESOURCE:
//						case OperandType::CONSTANT_BUFFER:
//						case OperandType::UNORDERED_ACCESS_VIEW:
//							used_resources.insert(OP_ID(op.type, 0, (uint32_t)op.index[0]));
//						}
//					});
//					;
//				}
//
//			s.operation = new_ops;
//			c.second = s.write();
//		}
//
//	for (auto & c : result.chunks_) {
//		if (c.first == RDEFName) {
//			RDEF rdef(c.second);
//			std::unordered_set<std::string> used_bindings;
//			// Filter the bindings
//			std::vector<RDEF::Binding> new_bindings;
//			for (const auto & i : rdef.binding) {
//				uint8_t op_type = 0xff;
//				switch (i.input_type) {
//				case 0: op_type = OperandType::CONSTANT_BUFFER; break; // cbuffer
//				case 1:                                                // tbuffer
//				case 2: op_type = OperandType::RESOURCE; break;        // texture
//				case 3: op_type = OperandType::SAMPLER; break;         // sampler
//				case 4:                                                // uavrw
//				case 6:                                                // uavrwstruct
//				case 8:                                                // uavrwaddr
//				case 9:                                                // uavappend
//				case 10:                                               // uavconst
//				case 11: op_type = OperandType::UNORDERED_ACCESS_VIEW; break; // uavrwstructwithcount
//				default: LOG(INFO) << "Unknown binding type  " << i.input_type;
//				}
//				if (used_resources.count(OP_ID(op_type, 0, i.bind_point))) {
//					new_bindings.push_back(i);
//					used_bindings.insert(i.name);
//				}
//			}
//			rdef.binding = new_bindings;
//
//			// Filter the buffers
//			std::vector<RDEF::CBuffer> new_cbuffer;
//			for (const auto & i : rdef.cbuffer)
//				if (used_bindings.count(i.name))
//					new_cbuffer.push_back(i);
//			rdef.cbuffer = new_cbuffer;
//
//			c.second = rdef.write();
//		} else if (c.first == ISGNName || c.first == OSGNName) {
//			uint8_t op_type = (c.first == OSGNName) ? OperandType::OUTPUT : OperandType::INPUT;
//			SGN sgn(c.second);
//			// Filter the input/output definitions
//			std::vector<SGN::Definition> new_def;
//			for (auto & d : sgn.definition)
//				if (used_resources.count(OP_ID(op_type, 0, d.reg)))
//					new_def.push_back(d);
//			sgn.definition = new_def;
//			c.second = sgn.write();
//		}
//	}
//
//	return result;
//}

struct MergeData {
	std::unordered_map<uint32_t, uint32_t> input_reg_remap_source_, input_reg_remap_target_;
	std::unordered_map<uint32_t, uint32_t> output_reg_remap_;
	std::unordered_map<uint32_t, uint32_t> cbuffer_reg_remap_;
};
typedef HLSL::ByteCode(*MergeFunc)(const HLSL::ByteCode & a, const HLSL::ByteCode & b, MergeData * data, bool inject_after);
static std::unordered_map<ChunkName, MergeFunc> chunk_merger;
static MergeFunc RegisterMergeFunc(ChunkName n, MergeFunc f) { return chunk_merger[n] = f; }
#define MERGE(n) HLSL::ByteCode merge ## n(const HLSL::ByteCode &, const HLSL::ByteCode &, MergeData *, bool); static MergeFunc init_##n = RegisterMergeFunc(n ## Name, merge ## n); HLSL::ByteCode merge ## n(const HLSL::ByteCode & a, const HLSL::ByteCode & b, MergeData * data, bool inject_after)
#define DEF_MERGE(n, nn) static MergeFunc init_##n = RegisterMergeFunc(n ## Name, merge ## nn);
MERGE(RDEF) {
	RDEF r(a), rb(b);
	if (rb.version > r.version) {
		r.RD11 = rb.RD11;
		r.version = rb.version;
		memcpy(r.unknown, rb.unknown, sizeof(r.unknown));
	}
	// Merge the cbuffers
	for (auto & b : rb.cbuffer) {
		bool insert = true;
		for (auto & bb : r.cbuffer) {
			if (bb <= b) {
				insert = false;
			}
			else if (b <= bb) {
				insert = false;
				bb = b;
			}
		}
		if (insert)
			r.cbuffer.push_back(b);
	}
	// Merge the bindings and hope for the best
	for (auto & b : rb.binding) {
		bool insert = true;
		// Let's see if there is already a matching binding
		for (auto & bb : r.binding)
			if (b.input_type == bb.input_type && b.name == bb.name) {
				// Let's use the existing binding (should probably do some more checks here, but this works for now)
				data->cbuffer_reg_remap_[b.bind_point] = bb.bind_point;
				insert = false;
			}
		if (insert)
			for (auto & bb : r.binding) 
				if (b.input_type == bb.input_type) {
					if (b.collide(bb)) {
						// We have a problem
						LOG(WARN) << "Colliding resource bindings " << b.name << " : " << bb.name << " @ " + std::to_string(b.bind_point) + "! giving up ...";
						return HLSL::ByteCode();
					}
				}
		if (insert)
			r.binding.push_back(b);
	}

	return r.write();
}

MERGE(ISGN) {
	SGN r(a), ib(b);
	uint32_t insert_reg = 0, insert_location = 0;
	for (uint32_t i = 0; i<r.definition.size(); i++ ) {
		const auto & d = r.definition[i];
		// The "insert_reg == d.reg + 1" special case prevents merged indices from being overwritten
		if (insert_reg == d.reg + 1 || (insert_reg < d.reg+1 && d.sys_id <= 6)) { /*D3D10_SB_NAME_VERTEX_ID = 6*/
			insert_reg = d.reg + 1;
			insert_location = i+1;
		}
	}

	std::unordered_map<uint32_t, uint32_t> tmp_map;
	std::vector<SGN::Definition> new_def;
	for (auto i : ib.definition) {
		bool insert = true;
		for (auto & ii : r.definition)
			if (i.collide(ii) && i == ii) {
				insert = false;
				ii.mask |= i.mask;
				ii.read_mask |= i.read_mask;
			}
		if (insert)
			for (auto & ii : r.definition)
				if (i.collide(ii) && i.reg != ii.reg) {
					// Resolve the register collision
					ii.mask |= i.mask;
					ii.read_mask |= i.read_mask;
					i.reg = tmp_map[i.reg] = ii.reg;
					insert = false;
				}
		if (insert)
			for (auto & ii : r.definition)
				if (i.collide(ii) && i.sem_id == ii.sem_id && i.name == ii.name) {
					LOG(WARN) << "Unresolvable input collision";
					return HLSL::ByteCode();
				}
		if (insert)
			new_def.push_back(i);
	}

	// Assign the new register ids
	uint32_t ni_reg = 0;
	for (auto & i : new_def) {
		if (data) {
			if (!data->input_reg_remap_source_.count(i.reg))
				data->input_reg_remap_source_[i.reg] = insert_reg + ni_reg++;
			i.reg = data->input_reg_remap_source_[i.reg];
		}
	}

	// Merge the two SGNs
	r.definition.insert(r.definition.begin() + insert_location, new_def.begin(), new_def.end());

	// Update all the old sysvals registers
	if (ni_reg > 0)
		for (uint32_t i = insert_location + (uint32_t)new_def.size(); i < r.definition.size(); i++)
			if (data)
				r.definition[i].reg = data->input_reg_remap_target_[r.definition[i].reg] = r.definition[i].reg + ni_reg;
	
	// Update any remap_source that might have changed its target
	for (const auto & i : tmp_map)
		if (data->input_reg_remap_target_.count(i.second))
			data->input_reg_remap_source_[i.first] = data->input_reg_remap_target_[i.second];
		else
			data->input_reg_remap_source_[i.first] = i.second;

	return r.write();
}
MERGE(OSGN) {
	SGN r(a), ob(b);
	uint32_t n_reg = 0;
	for (const auto & i : r.definition)
		if (n_reg <= i.reg)
			n_reg = i.reg + 1;
	std::unordered_map<std::string, int> order;
	int l = 0;
	for (const auto & d : r.definition)
		if (!order.count(d.name))
			order[d.name] = 100 * l++;
	for (const auto & d : ob.definition)
		if (order.count(d.name))
			l = order[d.name];
		else
			order[d.name] = l++;
	order["SV_Coverage"] = 10000;

	for (auto o : ob.definition) {
		bool insert = true;
		for (const auto & oo : r.definition) {
			if (o.collide(oo) && o == oo)
				insert = false;
			else if (o.collide(oo) && o.reg != oo.reg) {
				LOG(WARN) << "Writing identical outputs";
				return HLSL::ByteCode();
			}
			else if (o.collide(oo) && (o.sem_id != oo.sem_id || o.name != oo.name)) {
				// add the new input to its own register and insert it
				if (data) o.reg = data->output_reg_remap_[o.reg] = n_reg++;
			}
			else if (o.collide(oo)) {
				LOG(WARN) << "Unresolvable output collision";
				return HLSL::ByteCode();
			}
		}
		if (insert)
			r.definition.push_back(o);
	}

//	std::stable_sort(r.definition.begin(), r.definition.end(), [&](const SGN::Definition & a, const SGN::Definition & b) {return order[a.name] < order[b.name] || (order[a.name] == order[b.name] && a.reg < b.reg); });
	std::stable_sort(r.definition.begin(), r.definition.end(), [&](const SGN::Definition & a, const SGN::Definition & b) {return a.reg < b.reg; });
	return r.write();
}

SHDR::Operation translateSource(SHDR::Operation o, MergeData * data) {
	if (data) {
		typedef SHDR::Operation::Operand Operand;
		o.traverseOperands([&](Operand & op) {
			if (op.type == OperandType::INPUT && data->input_reg_remap_source_.count((uint32_t)op.index[0]))
				op.index[0] = data->input_reg_remap_source_[(uint32_t)op.index[0]];
			if (op.type == OperandType::OUTPUT && data->output_reg_remap_.count((uint32_t)op.index[0]))
				op.index[0] = data->output_reg_remap_[(uint32_t)op.index[0]];
			if (op.type == OperandType::CONSTANT_BUFFER && data->cbuffer_reg_remap_.count((uint32_t)op.index[0]))
				op.index[0] = data->cbuffer_reg_remap_[(uint32_t)op.index[0]];
		} );
	}
	return o;
}
SHDR::Operation translateTarget(SHDR::Operation o, MergeData * data) {
	if (data) {
		typedef SHDR::Operation::Operand Operand;
		o.traverseOperands([&](Operand & op) {if (op.type == OperandType::INPUT && data->input_reg_remap_target_.count((uint32_t)op.index[0])) op.index[0] = data->input_reg_remap_target_[(uint32_t)op.index[0]]; });
	}
	return o;
}
MERGE(SHDR) {
	SHDR sa(a), sb(b);
	SHDR r = sa;
	if (r.type != sb.type) {
		LOG(WARN) << "Trying to merge different shader types: " << r.type << " vs " << sb.type;
		return HLSL::ByteCode();
	}
	if (sb.version > r.version)
		r.version = sb.version;

	r.operation.clear();
	// Read the DCL
	size_t i = 0, j = 0;
	std::vector<SHDR::Operation> dcl_a, dcl_b;
	for (i = 0; i < sa.operation.size() && (isDCL(DECODE_OPCODE_TYPE(sa.operation[i].opcode)) || DECODE_OPCODE_TYPE(sa.operation[i].opcode) == OpCode::CUSTOMDATA); i++)
		dcl_a.push_back(translateTarget(sa.operation[i], data));
	for (j = 0; j < sb.operation.size() && (isDCL(DECODE_OPCODE_TYPE(sb.operation[j].opcode)) || DECODE_OPCODE_TYPE(sb.operation[j].opcode) == OpCode::CUSTOMDATA); j++)
		dcl_b.push_back(translateSource(sb.operation[j], data));

	std::unordered_map<uint32_t, int> order;
	int l = 0;
	for (auto & d : dcl_a) {
		if (!order.count(DECODE_OPCODE_TYPE(d.opcode)))
			order[DECODE_OPCODE_TYPE(d.opcode)] = 100 * ++l;
	}
	l = 0;
	for (auto & d : dcl_b) {
		if (order.count(DECODE_OPCODE_TYPE(d.opcode)))
			l = order[order[DECODE_OPCODE_TYPE(d.opcode)]];
		else
			order[DECODE_OPCODE_TYPE(d.opcode)] = ++l;
	}

	// Merge all dcls
	for (auto & o : dcl_b) {
		bool insert = true;
		for (auto & d : dcl_a) {
			if (DECODE_OPCODE_TYPE(o.opcode) == DECODE_OPCODE_TYPE(d.opcode)) {
				// Duplicate op
				insert = !(o.args == d.args);
				// Merge some ops
				if (DECODE_OPCODE_TYPE(o.opcode) == OpCode::DCL_TEMPS) {
					if (o.args[0] > d.args[0])
						d.args[0] = o.args[0];
					insert = false;
				}
				if (DECODE_OPCODE_TYPE(o.opcode) == OpCode::DCL_CONSTANT_BUFFER && o.args[1] == d.args[1]) {
					if (o.args[2] > d.args[2])
						d.args[2] = o.args[2];
					insert = false;
				}
				if (insert && o.args.size() == d.args.size() && o.args.size() > 0) {
					// Lets see if we need to correct the masks for the opcode (first element)
					insert = false;
					for (size_t i = 1; i < d.args.size() && !insert; i++)
						insert = (d.args[i] != o.args[i]);
					if (!insert) // or the masks (other things might go wrong here, let's just see)
						insert = (d.args[0] != o.args[0]);
				}
				if (!insert) break;
			}
		}
		if (insert)
			dcl_a.push_back(o);
	}
	// Sort the ops
	r.operation.insert(r.operation.end(), dcl_a.begin(), dcl_a.end());
	std::sort(r.operation.begin(), r.operation.end(), [&](const SHDR::Operation & a, const SHDR::Operation & b) { return order[DECODE_OPCODE_TYPE(a.opcode)] < order[DECODE_OPCODE_TYPE(b.opcode)] || (order[DECODE_OPCODE_TYPE(a.opcode)] == order[DECODE_OPCODE_TYPE(b.opcode)] && a.args < b.args); });

	if (inject_after) {
		// Add all the other ops (injected code first)
		for (; i < sa.operation.size(); i++)
			if (DECODE_OPCODE_TYPE(sa.operation[i].opcode) != OpCode::RET)
				r.operation.push_back(translateTarget(sa.operation[i], data));
		for (; j < sb.operation.size(); j++)
			r.operation.push_back(translateSource(sb.operation[j], data));
	}  else {
		// Add all the other ops (injected code first)
		for (; j < sb.operation.size(); j++)
			if (DECODE_OPCODE_TYPE(sb.operation[j].opcode) != OpCode::RET)
				r.operation.push_back(translateSource(sb.operation[j], data));
		for (; i < sa.operation.size(); i++)
			r.operation.push_back(translateTarget(sa.operation[i], data));
	}

	return r.write();
}
DEF_MERGE(SHEX, SHDR);

bool merge(const HLSL & base, const HLSL & inject, HLSL * out, bool inject_after) {
	HLSL r;
	MergeData merge_data;
	for (const auto & c : base.chunks_) {
		auto mc = c;
		for (const auto & oc : inject.chunks_) {
			// Deal with "SHEX" and "SHDR" (choose SHDR for both)
			// Thank you MS! Great idea redefining min as a macro, now I have to use the ugly (std::min)(..)
			uint32_t T0 = (std::min)(c.first, oc.first), T1 = (std::max)(c.first, oc.first);
			if ((T0 == T1 || (T0 == SHDRName && T1 == SHEXName) ) && chunk_merger.count(T0)) {
				mc.first = T1;
				mc.second = chunk_merger[T0](c.second, oc.second, &merge_data, inject_after);
				if (!mc.second.size())
					return false;
				break;
			}
		}
		r.chunks_.push_back(mc);
	}
	if (out) *out = r;
	return true;
}

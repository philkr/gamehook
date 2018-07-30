#define NOMINMAX
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <chrono>
#include <mutex>
#include <queue>
#include "log.h"
#include "sdk.h"
#include "json.h"
#include "viewer.h"
//#include "mongoose.h"
#define ASIO_STANDALONE
#define USE_STANDALONE_ASIO
#pragma warning( push )
#pragma warning( disable : 4996)
#include "external/simple-web-crypto/crypto.hpp" // UGLY hack around openssl dependency
#include "server_http.hpp"
#include "server_ws.hpp"
#pragma warning( pop )

#pragma comment(lib,"ws2_32")

const uint16_t DEFAULT_PORT = 8766;

double time() {
	auto now = std::chrono::system_clock::now();
	return std::chrono::duration<double>(now.time_since_epoch()).count();
}
struct CaptureSettings {
	int W = 800, H = 600;
	float fps = 0;
	int frame_buffer_size = 10;
	int info_buffer_size = 1000;
	std::vector<std::string> targets;
};
TOJSON(CaptureSettings, W, H, fps, frame_buffer_size, info_buffer_size, targets)

// Helper struct for BMP
#pragma pack(2)
struct BMHead {
	char magic[2] = { 'B', 'M' };
	int size;
	int _reserved = 0;
	int offset = 0x36;
	int hsize = 40;
	int width, height;
	short planes = 1;
	short depth = 24;
	int compression = 0;
	int img_size = 0;
	int rx = 0x03C3, ry = 0x03C3;
	int ncol = 0, nimpcol = 0;
	BMHead(int W, int H, int C) :width(W), height(-H), depth(C*8) {
		img_size = 4 * ((C * W - 1) / 4 + 1) * H;
		size = img_size + sizeof(BMHead);
	}
};
#pragma pack()

// helper functions
template<typename T> void S(uint32_t * v, T s, T e) { if (v) *v = std::stoul(s->second); }
template<typename T> void S(int * v, T s, T e) { if (v) *v = std::stoi(s->second); }
template<typename T> void S(float * v, T s, T e) { if (v) *v = std::stof(s->second); }
template<typename T> void S(std::vector<std::string> * v, T s, T e) { if (v) { v->clear(); for (auto i = s; i != e; i++) v->push_back(i->second); } }
std::string jsonify(const std::string & s) {
	if (s.empty() || s.find_first_not_of(" 0123456789.-") != s.npos)
		return "\"" + s + "\"";
	return s;
}


struct Frame {
	virtual int W() const = 0;
	virtual int H() const = 0;
	virtual int C() const = 0;
	virtual DataType type() const = 0;
	virtual void * data() = 0;
	virtual ~Frame() {}
};
template<typename T> struct TypedFrame : public Frame {
	std::vector<T> d;
	uint32_t w, h, c;
	bool used = false;
	TypedFrame(uint32_t W, uint32_t H, uint32_t C) :w(W), h(H), c(C), d(W*H*C) {}
	void resize(uint32_t W, uint32_t H, uint32_t C) {
		w = W;
		h = H;
		c = C;
		d.resize(W*H*C);
	}
	virtual int W() const final { return w; }
	virtual int H() const final { return h; }
	virtual int C() const final { return c; }
	void * data() final {
		return &d[0];
	}
	virtual DataType type() const final {
		return DT<T>::type;
	}
};
// Wrapper for TypedFrame, setting used=true for the lifetime of this object.
template<typename F> struct UsingFrame : public Frame {
	std::shared_ptr<F> f;
	UsingFrame(std::shared_ptr<F> f) :f(f) { f->used = true; }
	UsingFrame(const UsingFrame &) = delete;
	UsingFrame& operator=(const UsingFrame &) = delete;
	~UsingFrame() { f->used = false; }
	virtual int W() const { return f->W(); }
	virtual int H() const { return f->H(); }
	virtual int C() const { return f->C(); }
	virtual DataType type() const { return f->type(); }
	virtual void * data() { return f->data(); }
};
struct FrameBuffer {
	virtual std::shared_ptr<Frame> get(uint32_t id) const = 0;
	// Erase all but n_to_keep frames
	virtual void erase(uint32_t n_to_keep) = 0;
	std::shared_ptr<Frame> operator[](uint32_t id) const { return get(id); }

	// You ABSOLUTELY NEED TO add every frame you request back into the FrameBuffer
	virtual std::shared_ptr<Frame> requestFrame(int W, int H, int C) = 0;
	virtual void add(uint32_t id, std::shared_ptr<Frame> f) = 0;
	virtual std::vector<uint32_t> list() const = 0;
	virtual DataType type() const = 0;
	virtual ~FrameBuffer() {}
};
template<typename T> struct TypedFrameBuffer : public FrameBuffer {
	typedef TypedFrame<T> F;
	std::vector< std::shared_ptr<F> > empty_frames;
	mutable std::mutex mtx;
	std::map<uint32_t, std::shared_ptr<F> > stored_frames;

	virtual std::shared_ptr<Frame> get(uint32_t id) const final {
		// Return a stored typed frame
		std::lock_guard<std::mutex> lock(mtx);
		auto i = stored_frames.find(id);
		if (i == stored_frames.end()) return std::shared_ptr<Frame>();
		return std::make_shared<UsingFrame<F> >(i->second);
	}
	virtual void erase(uint32_t n_to_keep) final {
		// let's not actually delete them, but just move them to emtpy_frame_buffer
		std::lock_guard<std::mutex> lock(mtx);
		while (stored_frames.size() > n_to_keep) {
			auto i = stored_frames.begin();
			empty_frames.push_back(i->second);
			stored_frames.erase(i);
		}
	}
	// You ABSOLUTELY NEED TO add every frame you request back into the FrameBuffer
	virtual std::shared_ptr<Frame> requestFrame(int W, int H, int C) final {
		std::lock_guard<std::mutex> lock(mtx);
		for (auto i = empty_frames.begin(); i != empty_frames.end(); ++i)
			if (!(*i)->used) {
				auto r = *i;
				r->resize(W, H, C);
				// TODO: This would be faster with a std::list or equivalent!
				empty_frames.erase(i);
				return r;
			}
		return std::make_shared<F>(W, H, C);
	}
	virtual void add(uint32_t id, std::shared_ptr<Frame> f) final {
		auto ff = std::dynamic_pointer_cast<F>(f);
		if (ff) {
			std::lock_guard<std::mutex> lock(mtx);
			stored_frames[id] = ff;
		} else {
			LOG(WARN) << "TypeFrameBuffer::add type mismatch!";
		}
	}
	virtual std::vector<uint32_t> list() const {
		std::vector<uint32_t> r;
		for (auto i : stored_frames)
			r.push_back(i.first);
		return r;
	}
	virtual DataType type() const final {
		return DT<T>::type;
	}
};
std::shared_ptr<FrameBuffer> newFrameBuffer(DataType d) {
#define D(T) if (DT<T>::type == d) return std::make_shared<TypedFrameBuffer<T> >()
	DO_ALL_TYPE(D);
	LOG(WARN) << "Type " << d << " not supported in FrameBuffer!";
	return std::shared_ptr<FrameBuffer>();
}


struct HTTPServer {
	HTTPServer() = default;
	HTTPServer(const HTTPServer&) = delete;
	HTTPServer& operator=(const HTTPServer&) = delete;

	enum Status {
		STOPPED,
		STARTED,
		RUNNING,
	};
	Status status = STOPPED;
	std::mutex available_targets_mtx;
	std::vector<std::string> available_targets;
	std::mutex settings_mtx;
	CaptureSettings settings;
	uint32_t current_frame_id = 0, last_capture_frame_id = 0;
	std::mutex game_info_mtx;
	std::unordered_map<uint32_t, std::string> game_info;
	std::mutex frame_buffer_mtx;
	std::unordered_map<std::string, std::shared_ptr<FrameBuffer> > frame_buffer;
	std::mutex send_queue_mtx;
	std::queue<std::string> send_queue;

	typedef SimpleWeb::Server<SimpleWeb::HTTP> Server;
	typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;

	Server server;
	WsServer ws_server;
	HANDLE server_thread;
template<typename T>
	void updateSettings(T q) {
		std::lock_guard<std::mutex> lock(settings_mtx);
#define SET(x) {auto r = q.equal_range(#x); if (r.first != r.second) S(&settings.x, r.first, r.second);}
		SET(W); SET(H); SET(fps); SET(frame_buffer_size); SET(info_buffer_size); SET(targets);
	}
	template<typename T> const T * visPix(const T * d, int C, uint8_t * o, double max_norm) {
		return 0;
	}
	const uint8_t * visPix(const uint8_t * d, int C, uint8_t * o, double max_norm) {
		for (int i = 0; i < 3; i++)
			o[i] = d[i % C];
		return d + C;
	}
	static uint8_t * colorMap() {
		static std::vector<uint8_t> color_map;
		if (!color_map.size()) {
			color_map.resize(10010);
			srand(0);
			for (int i = 0; i < color_map.size(); i++)
				color_map[i] = rand();
			color_map[0] = color_map[1] = color_map[2] = 0;
		}
		return color_map.data();
	}
	const uint16_t * visPix(const uint16_t * d, int C, uint8_t * o, double max_norm) {
		if (C == 1) {
			const uint8_t * bc = colorMap() + (*d % 10000);
			for (int i = 0; i < 3; i++) o[i] = bc[i];
			return d + C;
		}
		return 0;
	}
	const uint32_t * visPix(const uint32_t * d, int C, uint8_t * o, double max_norm) {
		if (C == 1) {
			const uint8_t * bc = colorMap() + (*d % 10000);
			for (int i = 0; i < 3; i++) o[i] = bc[i];
			return d + C;
		}
		return 0;
	}
	const float * visPix(const float * d, int C, uint8_t * o, double max_norm) {
		if (C == 1) { // Disparity
			for (int i = 0; i < 3; i++) o[i] = (uint8_t)(255 * (*d / max_norm));
			return d + C;
		}
		if (C == 2) { // Flow
			float s = (float)(sqrt((double)d[0] * d[0] + (double)d[1] * d[1]) / max_norm);
			if (std::isnan(s)) {
				o[0] = o[1] = o[2] = 0;
			} else {
				float h = (atan2(d[1], d[0]) / 3.14159265359f + 0.5f);
				// HSV to RGB
				int hi = (int)(6 * h + 6) % 6;
				float f = 6 * h - hi;
				float p = 1.f - s, q = 1.f - s * f, t = 1.f - s * (1.f - f), v = 1.f;
				float r = 0, g = 0, b = 0;
				switch (hi) {
				case 0: r = v, g = t, b = p; break;
				case 1: r = q, g = v, b = p; break;
				case 2: r = p, g = v, b = t; break;
				case 3: r = p, g = q, b = v; break;
				case 4: r = t, g = p, b = v; break;
				case 5: r = v, g = p, b = q; break;
				}
				o[0] = (uint8_t)(255 * r);
				o[1] = (uint8_t)(255 * g);
				o[2] = (uint8_t)(255 * b);
			}
			return d + C;
		}
		return 0;
	}
	template<typename T>
	double maxNorm(const T* d, int N, int C) {
		double mx = 0;
		for (int i = 0, k = 0; i < N; i++) {
			double nm = 0;
			for (int j = 0; j < C; j++, k++)
				nm += (double)d[k] * (double)d[k];
			if (nm > mx) mx = nm;
		}
		return sqrt(mx);
	}
	template<typename T>
	bool sendBMPT(std::shared_ptr<Server::Response> response, std::shared_ptr<Frame> f) {
		const uint32_t C = f->C(), H = f->H(), W = f->W();
		const T * d = (const T *)f->data();
		{
			uint8_t tmp[10];
			if (!visPix(d, C, tmp, 1.f)) return false;
		}
		double max_norm = maxNorm(d, W*H, C);
		// TODO: This is slow (50-100ms)
		BMHead header(W, H, 3);
		SimpleWeb::CaseInsensitiveMultimap http_header;
		http_header.insert(std::pair<std::string, std::string>("content-type", "image/bmp"));
		http_header.insert(std::pair<std::string, std::string>("content-length", std::to_string(header.img_size + sizeof(BMHead))));
		response->write(http_header);
		response->write((const char *)&header, sizeof(BMHead));
		uint8_t * row = new uint8_t[3 * W + 10];
		memset(row, 0, 3 * W + 10);
		for (uint32_t j = 0; j < H; j++) {
			for (uint32_t i = 0; i < W; i++)
				d = visPix(d, C, row + 3 * i, max_norm);
			response->write((const char *)row, 4 * ((3 * W - 1) / 4 + 1));
		}
		response->flush();
		delete[] row;
		return true;
	}

	bool sendBMP(std::shared_ptr<Server::Response> response, std::shared_ptr<Frame> f) {
#define F(t) if (DT<t>::type == f->type()) return sendBMPT<t>(response, f);
			DO_ALL_BUT_HALF_TYPE(F);
#undef F
		return false;
	}
	bool sendRAW(std::shared_ptr<Server::Response> response, std::shared_ptr<Frame> f) {
		DataType dt = f->type();
		const uint32_t C = f->C(), H = f->H(), W = f->W(), DS = (uint32_t)dataSize(dt);

		SimpleWeb::CaseInsensitiveMultimap http_header;
		http_header.insert(std::pair<std::string, std::string>("content-type", "image/raw"));
		http_header.insert(std::pair<std::string, std::string>("content-length", std::to_string(W*H*C*DS)));
		response->write(http_header);

		const char * format_str[] = { "=u1\0", "=u2\0", "=u4\0", "=f4\0", "=f2\0" };
		response->write(format_str[(int)dt], 4);
		uint32_t hdr[3] = { H, W, C };
		response->write((const char*)hdr, sizeof(hdr));
		response->write((const char*)f->data(), W*H*C*DS);
		return true;
	}
	const std::string PING_ENDPOINT = "^/ping/?$";
	void ping(size_t id, const std::string & msg) {
		if (ws_server.endpoint.count(PING_ENDPOINT)) {
			auto &ping_endpoint = ws_server.endpoint[PING_ENDPOINT];
			for (auto c : ping_endpoint.get_connections()) {
				c->send("{\"id\": "+std::to_string(id)+", \"message\": \""+msg+"\"}");
			}
		}
	}
	void start() {
		server.config.port = DEFAULT_PORT;
		{
			char tmp[256];
			if (GetEnvironmentVariableA("SERVER_PORT", tmp, sizeof(tmp)))
				server.config.port = atoi(tmp);
		}

		// Setup a websock server  
		server.on_upgrade = [&](std::unique_ptr<SimpleWeb::HTTP> &socket, std::shared_ptr<Server::Request> request) {
			auto connection = std::make_shared<WsServer::Connection>(std::move(socket));
			connection->method = std::move(request->method);
			connection->path = std::move(request->path);
			connection->http_version = std::move(request->http_version);
			connection->header = std::move(request->header);
			connection->remote_endpoint = std::move(*request->remote_endpoint);
			ws_server.upgrade(connection);
		};
		auto &ping_endpoint = ws_server.endpoint[PING_ENDPOINT];
		ping_endpoint.on_open = [&](std::shared_ptr<WsServer::Connection> connection) {
			LOG(INFO) << "New ping from " << connection;
		};
		ping_endpoint.on_close = [&](std::shared_ptr<WsServer::Connection> connection, int status, const std::string &reason) {
			LOG(INFO) << "Closed ping from " << connection << " reason: " << reason;
		};


		// Setup the HTTP server
		server.resource["^/view$"]["GET"] = [this](std::shared_ptr<Server::Response> response, std::shared_ptr<Server::Request> request) {
			updateSettings(request->parse_query_string());
			response->write(viewer("localhost:"+std::to_string(server.config.port), settings.targets));
		};
		server.resource["^/status$"]["GET"] = [this](std::shared_ptr<Server::Response> response, std::shared_ptr<Server::Request> request) {
			if (this->status == STARTED) response->write(std::string("started"));
			if (this->status == RUNNING) response->write(std::string("running"));
			if (this->status == STOPPED) response->write(std::string("stopped"));
		};
		server.resource["^/targets$"]["GET"] = [this](std::shared_ptr<Server::Response> response, std::shared_ptr<Server::Request> request) {
			std::lock_guard<std::mutex> lock(available_targets_mtx);
			response->write(toJSON(available_targets));
		};
		server.resource["^/settings$"]["GET"] = [this](std::shared_ptr<Server::Response> response, std::shared_ptr<Server::Request> request) {
			updateSettings(request->parse_query_string());
			std::lock_guard<std::mutex> lock(settings_mtx);
			response->write(toJSON(settings));
		};
		server.resource["^/current$"]["GET"] = [this](std::shared_ptr<Server::Response> response, std::shared_ptr<Server::Request> request) {
			response->write(std::to_string(current_frame_id));
		};
		server.resource["^/frames$"]["GET"] = [this](std::shared_ptr<Server::Response> response, std::shared_ptr<Server::Request> request) {
			std::shared_ptr<FrameBuffer> fb;
			{
				std::lock_guard<std::mutex> lock(frame_buffer_mtx);
				auto it = frame_buffer.begin();
				if (it != frame_buffer.end())
					fb = it->second;
			}
			if (fb) {
				std::vector<uint32_t> ids = fb->list();
				std::string r = "[";
				for (uint32_t i : ids)
					r += (r.size()>1? ",":"") + std::to_string(i);
				r += "]";
				response->write(r);
			} else response->write("[]");
		};
		server.resource["^/info$"]["GET"] = [this](std::shared_ptr<Server::Response> response, std::shared_ptr<Server::Request> request) {
			uint32_t f = current_frame_id;
			auto q = request->parse_query_string();
			auto id = q.find("id");
			if (id != q.end()) f = std::stoul(id->second);
			std::lock_guard<std::mutex> lock(game_info_mtx);
			if (game_info.count(f))
				response->write(game_info[f]);
			else
				response->write(SimpleWeb::StatusCode::success_no_content);
		};
		server.resource["^/send$"]["GET"] = [this](std::shared_ptr<Server::Response> response, std::shared_ptr<Server::Request> request) {
			auto q = request->parse_query_string();
			std::string json = "{";
			for (auto i = q.begin(); i != q.end(); ) {
				auto c = i++;
				if (i != q.end() && c->first == i->first) { // We are deadling with an array
					std::string e = "[" + jsonify(c->second) + ",";
					for (c = i++; i != q.end() && c->first == i->first; c = i++)
						e += jsonify(c->second) + ",";
					e += jsonify(c->second) + "]";
					json += "\"" + c->first + "\":" + e;
				}
				else
					json += "\"" + c->first + "\":" + jsonify(c->second);
				if (i != q.end()) json += ",";
			}
			json += "}";
			{
				std::lock_guard<std::mutex> lock(send_queue_mtx);
				send_queue.push(json);
			}

			std::lock_guard<std::mutex> lock(game_info_mtx);
			if (game_info.count(current_frame_id))
				response->write(game_info[current_frame_id]);
			else
				response->write(SimpleWeb::StatusCode::success_no_content);
		};
		server.resource["^/send$"]["POST"] = [this](std::shared_ptr<Server::Response> response, std::shared_ptr<Server::Request> request) {
			{
				std::lock_guard<std::mutex> lock(send_queue_mtx);
				send_queue.push(request->content.string());
			}

			std::lock_guard<std::mutex> lock(game_info_mtx);
			if (game_info.count(current_frame_id))
				response->write(game_info[current_frame_id]);
			else
				response->write(SimpleWeb::StatusCode::success_no_content);
		};
		server.resource["^/bmp$"]["GET"] = [this](std::shared_ptr<Server::Response> response, std::shared_ptr<Server::Request> request) {
			auto q = request->parse_query_string();
			auto it = q.find("t");
			if (it != q.end()) {
				std::string t = it->second;
				uint32_t id = last_capture_frame_id;
				it = q.find("id");
				if (it != q.end()) id = std::stoul(it->second);

				std::shared_ptr<FrameBuffer> fb;
				{
					std::lock_guard<std::mutex> lock(frame_buffer_mtx);
					auto it = frame_buffer.find(t);
					if (it != frame_buffer.end())
						fb = it->second;
				}
				if (fb) {
					auto f = fb->get(id);
					if (f && sendBMP(response, f))
						return;
				}
			}
			response->write(SimpleWeb::StatusCode::success_ok);
		};
		server.resource["^/raw$"]["GET"] = [this](std::shared_ptr<Server::Response> response, std::shared_ptr<Server::Request> request) {
			auto q = request->parse_query_string();
			auto it = q.find("t");
			if (it != q.end()) {
				std::string t = it->second;
				uint32_t id = last_capture_frame_id;
				it = q.find("id");
				if (it != q.end()) id = std::stoul(it->second);

				std::shared_ptr<FrameBuffer> fb;
				{
					std::lock_guard<std::mutex> lock(frame_buffer_mtx);
					auto it = frame_buffer.find(t);
					if (it != frame_buffer.end())
						fb = it->second;
				}
				if (fb) {
					auto f = fb->get(id);
					if (f && sendRAW(response, f))
						return;
				}
			}
			response->write(SimpleWeb::StatusCode::success_ok);
		};
		server.default_resource["GET"] = [](std::shared_ptr<Server::Response> response, std::shared_ptr<Server::Request> request) {
			response->write(std::string("gamehook server."));
		};
		status = STARTED;
		server_thread = CreateThread(NULL, 0, HTTPServer::_serve, this, 0, NULL);
	}
	void stop() {
		status = STOPPED;
		server.stop();
		if (WaitForSingleObject(server_thread, 2000) == WAIT_TIMEOUT)
			LOG(WARN) << "Failed to close server thread";
		CloseHandle(server_thread);
	}
	static DWORD WINAPI _serve(LPVOID that) {
		return reinterpret_cast<HTTPServer*>(that)->serve();
	}
	int serve() {
		server.start();
		return 0;
	}
};
HTTPServer server;



struct Server : public GameController {
	double last_recorded_frame = 0, frame_timestamp = 0;
	bool recording_current_frame = false;
	CaptureSettings current_settings;
	virtual void onPresent(uint32_t frame_id) override {
		if (recording_current_frame)
			server.ping(frame_id, "frame_recorded");
		{
			std::lock_guard<std::mutex> lock(server.settings_mtx);
			current_settings = server.settings;
		}
		frame_timestamp = time();
		if ((frame_timestamp - last_recorded_frame) * current_settings.fps >= 1) {
			last_recorded_frame = frame_timestamp;
			recording_current_frame = true;
			recordNextFrame(RecordingType::DRAW);
		}
		else {
			// Recording disabled, let's make sure we capture the first frame once we enable it.
			if (current_settings.fps <= 0)
				last_recorded_frame = 0;
			recording_current_frame = false;
		}
	}

	virtual void onBeginFrame(uint32_t frame_id) override {
		recording_current_frame = currentRecordingType() != RecordingType::NONE;
		if (recording_current_frame) {
			for (const auto & t : current_settings.targets)
				requestOutput(t, current_settings.W, current_settings.H);
		}

		if (server.status)
			server.status = HTTPServer::RUNNING;
		if (!server.available_targets.size()) {
			std::lock_guard<std::mutex> lock(server.available_targets_mtx);
			server.available_targets = listTargets();
		}

		// Process the send queue
		{
			std::lock_guard<std::mutex> lock(server.send_queue_mtx);
			while (!server.send_queue.empty()) {
				command(server.send_queue.front());
				server.send_queue.pop();
			}
		}
	}
	virtual void onEndFrame(uint32_t frame_id) override {
		server.current_frame_id = frame_id;
		{
			std::lock_guard<std::mutex> lock(server.game_info_mtx);
			server.game_info[frame_id] = gameState();
			// Purge old game states
			while (server.game_info.size() > server.settings.info_buffer_size)
				server.game_info.erase(server.game_info.begin());
		}
		if (recording_current_frame) {
			for (const auto & t : current_settings.targets)
				if (hasTarget(t)) {
					if (!targetAvailable(t)) {
						LOG(INFO) << "Target '" << t << "' exists, but was not written. Check your plugins to make sure all targets are written properly!";
						continue;
					}
					DataType dt = outputType(t);
					std::shared_ptr<FrameBuffer> fb;
					{
						std::lock_guard<std::mutex> lock(server.frame_buffer_mtx);
						auto i = server.frame_buffer.find(t);
						if (i != server.frame_buffer.end())
							fb = i->second;
						else
							fb = server.frame_buffer[t] = newFrameBuffer(dt);
					}
					ASSERT(fb);
					if (fb) {
						uint32_t W = current_settings.W, H = current_settings.H, C = outputChannels(t);
						// Request the frame
						auto f = fb->requestFrame(W, H, C);
						// Write the data
						readTarget(t, W, H, C, f->type(), f->data());
						// Color correction
						TargetType tt = targetType(t);
						if ((B8G8R8A8_UNORM <= tt && tt <= B8G8R8X8_UNORM_SRGB && tt != R10G10B10_XR_BIAS_A2_UNORM) && C == 4) {
							// Flip RB
							uint8_t * d = (uint8_t*)f->data();
							// TODO: SSE This if it's too slow _mm_shuffle_epi8
							for (size_t i = 0; i < W*H; i++)
								std::swap(d[C*i + 0], d[C*i + 2]);
						}

						// Add the frame back into the buffer
						fb->add(frame_id, f);
					}
				}
			server.last_capture_frame_id = frame_id;
			// Clean up the frame buffer
			std::lock_guard<std::mutex> lock(server.frame_buffer_mtx);
			for (auto i : server.frame_buffer)
				i.second->erase(server.settings.frame_buffer_size);
		}
	}
};
REGISTER_CONTROLLER(Server);


BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		LOG(INFO) << "Server turned on";
		server.start();
	}

	if (reason == DLL_PROCESS_DETACH) {
		server.stop();
		LOG(INFO) << "Server turned off";
	}
	return TRUE;
}

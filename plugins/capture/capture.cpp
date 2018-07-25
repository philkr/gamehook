#include <windows.h>
#include <unordered_map>
#include <ctime>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <fstream>
#include "log.h"
#include "sdk.h"

double time() {
	auto now = std::chrono::system_clock::now();
	return std::chrono::duration<double>(now.time_since_epoch()).count();
}

// You can set 3 env variables to control the capture process
// CAPTURE_DIR  specified the base capture directory (a unique directly for all files will be created inside CAPTURE_DIR)
// CAPTURE_FPS  specified how many FPS we should capture at (a double/float value)
// CAPTURE_TARGETS  a comma separated list of targets to capture

// This plugin does implement frame skipping, if your hard drive is too slow you will start to lose frames!
// Use F12 to start and stop recording

const std::string DEFAULT_CAPTURE_DIR = "cap";
const int MAX_IMAGE_BUFFER = 32;

static std::string strip(const std::string & i) {
	size_t a = 0, b = i.size();
	while (a < b && isblank(i[a])) a++;
	while (b > 1 && isblank(i[b - 1])) b--;
	return i.substr(a, b);
}
static std::vector<std::string> splitStrip(const std::string & s, char c) {
	std::vector<std::string> r;
	for (size_t i = 0; i < s.size(); i++) {
		size_t j = s.find(c, i);
		r.push_back(strip(s.substr(i, j)));
		if (j == s.npos) break;
		i = j;
	}
	return r;
}


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
	BMHead(int W, int H) :width(W), height(-H) {
		int E = 4 - ((W * 3) % 4);
		img_size = (3 * W + E) * H;
		size = img_size + sizeof(BMHead);
	}
};
#pragma pack()

static void saveBMP(int W, int H, int C, const uint8_t * d, const char * fn, bool flip_rb = false) {
	BMHead header(W, H);
	FILE * fp;
	fopen_s(&fp, fn, "wb");
	fwrite(&header, 1, sizeof(header), fp);

	uint8_t * row = new uint8_t[3 * W + 10];
	for (int j = 0; j < H; j++, d += W*C) {
		if (flip_rb) {
			for (int i = 0; i < W; i++)
				for (int k = 0; k < 3; k++)
					row[3 * i + k] = d[4 * i + 2 - k];
		} else {
			for (int i = 0; i < W; i++)
				for (int k = 0; k < 3; k++)
					row[3 * i + k] = d[4 * i + k];
		}
		fwrite(row, 4, ((3 * W - 1) / 4 + 1), fp);
	}
	delete[] row;
	fclose(fp);
}

// Concurrent queue
template <typename T>
class CQueue {
public:
	bool pop(T* item) {
		std::unique_lock<std::mutex> mlock(mutex_);
		if (queue_.empty())
			cond_.wait(mlock);
		if (queue_.empty()) return false;
		*item = queue_.front();
		queue_.pop();
		return true;
	}
	void push(const T& item) {
		std::unique_lock<std::mutex> mlock(mutex_);
		queue_.push(item);
		mlock.unlock();
		cond_.notify_one();
	}
	void push(T&& item) {
		std::unique_lock<std::mutex> mlock(mutex_);
		queue_.push(std::move(item));
		mlock.unlock();
		cond_.notify_one();
	}
	bool empty() {
		std::unique_lock<std::mutex> mlock(mutex_);
		return queue_.empty();
	}
	void wake() {
		cond_.notify_all();
	}
private:
	std::queue<T> queue_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

struct Image {
	std::string filename;
	std::vector<uint8_t> data_;
	int W, H, C;
	DataType dt;
	TargetType tt;

	Image(int W, int H, int C, DataType dt, TargetType tt) :W(W), H(H), C(C), dt(dt), tt(tt), data_(W*H*C*dataSize(dt)) {}
	uint8_t * data() { return &data_[0]; }
	const uint8_t * data() const { return &data_[0]; }

	void save() {
		if (R8G8B8A8_TYPELESS <= tt && tt <= R8G8B8A8_SINT) {
			saveBMP(W, H, C, data(), (filename + ".bmp").c_str(), false);
		} else if (B8G8R8A8_UNORM <= tt && tt <= B8G8R8X8_UNORM_SRGB && tt != R10G10B10_XR_BIAS_A2_UNORM) {
			saveBMP(W, H, C, data(), (filename + ".bmp").c_str(), true);
		} else if (dt != UNKNOWN){
			FILE * fp;
			fopen_s(&fp, (filename + ".raw").c_str(), "wb");
			const char * format_str[] = { "=u1\0", "=u2\0", "=u4\0", "=f4\0", "=f2\0" };
			fwrite(format_str[(int)dt], 4, 1, fp);
			uint32_t hdr[3] = { H, W, C };
			fwrite(hdr, sizeof(hdr), 1, fp);
			fwrite(data_.data(), sizeof(uint8_t), data_.size(), fp);
			fclose(fp);
		} else {
			LOG(WARN) << "Failed to save file '" << filename << "'! Unknown format " << std::hex << tt;
		}
	}

};

struct Capture: public GameController {
	bool is_init = false;
	std::string capture_dir;
	std::vector<std::string> capture_targets;

	Capture() : GameController(), running_(false) {
	}
	~Capture() {
		if (running_) {
			running_ = false;
			save_queue_.wake();
			thread_.join();
		}
	}
	void init() {
		if (!is_init) {
			is_init = true;

			char tmp[256];
			std::string base_capture_dir = DEFAULT_CAPTURE_DIR;
			if (GetEnvironmentVariableA("CAPTURE_DIR", tmp, sizeof(tmp)))
				base_capture_dir = tmp;

			capture_targets = listTargets();
			if (GetEnvironmentVariableA("CAPTURE_TARGETS", tmp, sizeof(tmp))) {
				capture_targets.clear();
				for (const auto & t : splitStrip(tmp, ','))
					if (hasTarget(t))
						capture_targets.push_back(t);
					else
						LOG(WARN) << "Target '" << t << "' does not exist!";
			}

			time_t mytime = time(NULL);
			tm mytm;
			localtime_s(&mytm, &mytime);
			char stamp[128] = { 0 };
			strftime(stamp, sizeof(stamp), "/%Y_%m_%d_%H_%M_%S/", &mytm);
			capture_dir = base_capture_dir + stamp;
			CreateDirectory(base_capture_dir.c_str(), NULL);
			CreateDirectory(capture_dir.c_str(), NULL);

			// Start the capture thread
			for (int i = 0; i < MAX_IMAGE_BUFFER; i++)
				ready_queue_.push(i);
			running_ = true;
			thread_ = std::thread(&Capture::captureThread, this);

			LOG(INFO) << "Capture in '" << capture_dir << "'";
		}
	}

	std::unordered_map<uint8_t, bool> key_state;
	virtual bool keyUp(unsigned char key, unsigned char special_status) {
		key_state[key] = 0;
		return false;
	}
	bool capture = false;
	virtual void onBeginFrame(uint32_t frame_id) override {
		init();
		capture = currentRecordingType() != NONE;
		if (capture) {
			for (const auto & t : capture_targets)
				requestOutput(t);
		}
	}
	virtual void onEndFrame(uint32_t frame_id) override {
		std::string frame_name = std::to_string(frame_id);
		frame_name = std::string(8 - frame_name.size(), '0') + frame_name;
		if (capture) {
			for (const auto & t : capture_targets)
				if (!targetAvailable(t))
					capture = false;
			if (capture)
				for (const auto & t : capture_targets) {
					TargetType tt = targetType(t);
					DataType tp = outputType(t);
					int C = outputChannels(t), W = defaultWidth(), H = defaultHeight();
					std::shared_ptr<Image> im = std::make_shared<Image>(W, H, C, tp, tt);
					readTarget(t, W, H, C, tp, im->data());
					im->filename = capture_dir + frame_name + "_" + t;
					saveImage(im);
				}
		}
		std::string gs = gameState();
		if (gs.size() > 5) {
			gs[gs.size() - 1] = ',';
			gs = gs + "\"timestamp\": " + std::to_string(time());
			gs = gs + "}";

			std::ofstream fout(capture_dir + frame_name + "_state.json");
			fout << gs;
			fout.close();
		}
	}
	CQueue<int> ready_queue_;
	CQueue<int> save_queue_;
	std::shared_ptr<Image> cap[MAX_IMAGE_BUFFER];
	std::vector< std::shared_ptr<Image> > save_queue;
	void saveImage(std::shared_ptr<Image> im) {
		// Find a slot that's empty
		int tid = -1;
		for (int it = 0; !ready_queue_.pop(&tid) && it < 100; it++);
		if (tid == -1) {
			LOG(WARN) << "Failed to find storage for frame '" << im->filename << "' dropping it!";
		} else {
			cap[tid] = im;
			save_queue_.push(tid);
		}
	}

	bool running_;
	std::thread thread_;
	void captureThread() {
		while (running_ || !save_queue_.empty()) {
			int tid;
			// Fetch an element from the queue
			if (save_queue_.pop(&tid)) {
				cap[tid]->save();
				ready_queue_.push(tid);
			}
		}
	}
};
REGISTER_CONTROLLER(Capture);


BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		LOG(INFO) << "Capture turned on";
	}

	if (reason == DLL_PROCESS_DETACH) {
		LOG(INFO) << "Capture turned off";
	}
	return TRUE;
}

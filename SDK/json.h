#pragma once
#include "log.h"
#include <unordered_map>

// Standard toJSON conversions
inline std::string toJSON(const std::string & s) { return "\"" + s + "\""; }
inline std::string toJSON(int v) { return std::to_string(v); }
inline std::string toJSON(float v) { return std::to_string(v); }
template<typename T> std::string toJSON(const std::vector<T> & v) {
	std::string r = "[";
	for (auto i : v)
		r += (r.size() == 1 ? "" : ", ") + toJSON(i);
	return r + "]";
}
#define STR(s) #s
#define _TOJSON(x) {if (r.size()>1) r += std::string(","); r += "\"" + std::string(STR(x)) + "\":" + toJSON(t.x); }
// Helper function to do a JSON conversion of a game state (list the class and all variables you want to export)
#define TOJSON(T, ...) inline std::string toJSON(const T & t) { std::string r = "{"; FOR_EACH(_TOJSON, __VA_ARGS__); return r + "}";};

// Simple JSON parser (it's a bit longer than I wanted)
struct JSONValue {
	enum Type { STRING, NUMBER, BOOL, ARRAY, OBJECT, NULL_, };
	Type type = NULL_;
	float number;
	std::string string;
	bool boolean;
	std::vector<JSONValue> array;
	std::unordered_map<std::string, JSONValue> object;
	static void skipSpace(const char *& s, const char * e) {
		while (s < e && isspace(*s)) s++;
	}
	std::string parseNextString(const char *& s, const char * e) {
		skipSpace(s, e);
		if (s >= e || *s != '"') { LOG(ERR) << "JSON String parsing failed." << std::string(s, e); return ""; }
		++s;
		bool escape = false;
		std::string r;
		while (s < e) {
			char c = *(s++);
			if (escape) {
				if (c == 'n') r.push_back('\n');
				else if (c == 'r') r.push_back('\r');
				else if (c == 't') r.push_back('\t');
				else if (c == 'b') r.push_back('\b');
				else if (c == '0') r.push_back('\0');
				else if (c == 'f') r.push_back('\f');
				else if (c == 'v') r.push_back('\v');
				else r.push_back(c);
				escape = false;
			} else if (c == '\\') escape = true;
			else if (c == '"') return r;
			else r.push_back(c);
		}
		LOG(ERR) << "JSON Failed to parse string. No closing quotes: " << std::string(s, e);
		return "";
	}
	void parseArray(const char *& s, const char * e) {
		type = ARRAY;
		skipSpace(s, e);
		if (s >= e || *s != '[') { LOG(ERR) << "JSON Array parsing failed."; return; }
		++s;
		while (1) {
			skipSpace(s, e);
			if (*s == ']') { ++s; break; }
			array.push_back(JSONValue::parse(s, e));
			skipSpace(s, e);
			if (s >= e || (*s != ',' || *s != ']')) { LOG(ERR) << "JSON Array ',' or ']' expected '" << *s << "' found!"; return; }
			if (s < e && *s == ',') ++s;
		}
	}
	void parseObject(const char *& s, const char * e) {
		type = OBJECT;
		skipSpace(s, e);
		if (s >= e || *s != '{') { LOG(ERR) << "JSON Object parsing failed."; return; }
		++s;
		while (1) {
			skipSpace(s, e);
			if (*s == '}') { ++s; break; }
			std::string name = parseNextString(s, e);
			skipSpace(s, e);
			if (s >= e || *s != ':') { LOG(ERR) << "JSON Object ':' expected '" << *s << "' found!"; return; }
			++s;
			skipSpace(s, e);
			object[name] = JSONValue::parse(s, e);
			skipSpace(s, e);
			if (s >= e || (*s != ',' && *s != '}')) { LOG(ERR) << "JSON Object ',' or '}' expected '" << *s << "' found!"; return; }
			if (s < e && *s == ',') ++s;
		}
	}
	void parseString(const char *& s, const char * e) {
		type = STRING;
		string = parseNextString(s, e);
	}
	void parseNumber(const char *& s, const char * e) {
		type = NUMBER;
		skipSpace(s, e);
		const char * i = s;
		for (; s < e && (isalnum(*s) || *s == '.' || *s == '-');++s);
		try {
			number = std::stof(std::string(i, s - i));
		}
		catch (std::exception) {
			LOG(ERR) << "JSON Number Failed to convert value '" << std::string(i, s - i) << "' to number for string '" << s << "'";
		}
	}
	void parseNull(const char *& s, const char * e) {
		type = NULL_;
		skipSpace(s, e);
		if (s + 4 >= e || std::string(s, s + 4) != "null") LOG(ERR) << "JSON NULL Expected string 'null', got '" << s << "'!";
		else s += 4;
	}
	void parseBoolean(const char *& s, const char * e) {
		type = BOOL;
		skipSpace(s, e);
		if (s + 4 < e && std::string(s, s + 4) == "true") { boolean = true; s += 4; }
		else if (s + 5 < e && std::string(s, s + 5) == "false") { boolean = false; s += 5; }
		else LOG(ERR) << "JSON Boolean expected string 'true' or 'false', got '" << s << "'!";
	}
	static JSONValue parse(const char *& s, const char * e) {
		JSONValue r;
		skipSpace(s, e);
		if (*s == '{') r.parseObject(s, e);
		else if (*s == '[') r.parseArray(s, e);
		else if (*s == '"') r.parseString(s, e);
		else if (isdigit(*s) || *s == '.' || *s == '-') r.parseNumber(s, e);
		else if (*s == 't' || *s == 'f') r.parseBoolean(s, e);
		else if (*s == 'n') r.parseNull(s, e);
		else LOG(ERR) << "Unknown JSON value";
		return r;
	}
	static JSONValue parse(const std::string & str) {
		const char * s = str.c_str();
		const char * e = s + str.size();
		return parse(s, e);
	}
};
inline void fromJSON(std::string * r, const JSONValue & v) { if (v.type == JSONValue::STRING) *r = v.string; }
inline void fromJSON(int * r, const JSONValue & v) { if (v.type == JSONValue::NUMBER) *r = static_cast<int>(v.number); }
inline void fromJSON(float * r, const JSONValue & v) { if (v.type == JSONValue::NUMBER) *r = v.number; }
inline void fromJSON(double * r, const JSONValue & v) { if (v.type == JSONValue::NUMBER) *r = v.number; }
template<typename T> void fromJSON(std::vector<T> * r, const JSONValue & v) {
	if (v.type == JSONValue::ARRAY) {
		r->resize(v.array.size());
		for (size_t i = 0; i < r->size(); i++)
			fromJSON(&(*r[i]), v.array[i]);
	}
}
#define _FROMJSON(x) { auto it = v.object.find(STR(x)); if (it != v.object.end()) fromJSON(&r->x, it->second); }
// Helper function to do a JSON conversion of a game state (list the class and all variables you want to export)
#define FROMJSON(T, ...) inline void fromJSON(T * r, const JSONValue & v) { if (v.type == JSONValue::OBJECT) { FOR_EACH(_FROMJSON, __VA_ARGS__); } };
template<typename T> void fromJSON(T * r, const std::string & s) {
	fromJSON(r, JSONValue::parse(s));
}

#define JSON(T, ...) E( FROMJSON(T, __VA_ARGS__); TOJSON(T, __VA_ARGS__) )

/* Only the brave continue past this point! If there is a bug in the code below, good luck finding it (you might want to enable the preprocessor output of your compiler and read that file) */
// Note to future self: I'm sorry!

// Macro foreach
// Thank you MSVC! This expansion macro makes soo much more sense than the gcc interpretation of __VA__ARGS__
#define E(x) x
#define VA_COUNT(...) E(VA_COUNT_H(__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,))
#define VA_COUNT_H(e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10, e11, e12, e13, e14, e15, e16, e17, e18, e19, e20, e21, e22, e23, e24, e25, e26, e27, e28, e29, e30, e31, e32, e33, e34, e35, e36, e37, e38, e39, e40, e41, e42, e43, e44, e45, e46, e47, e48, e49, e50, e51, e52, e53, e54, e55, e56, e57, e58, e59, e60, e61, e62, e63, size, ...) size

#define __FE_1(f, x) f(x);
#define __FE_2(f, x, ...) f(x); E(__FE_1(f, __VA_ARGS__))
#define __FE_3(f, x, ...) f(x); E(__FE_2(f, __VA_ARGS__))
#define __FE_4(f, x, ...) f(x); E(__FE_3(f, __VA_ARGS__))
#define __FE_5(f, x, ...) f(x); E(__FE_4(f, __VA_ARGS__))
#define __FE_6(f, x, ...) f(x); E(__FE_5(f, __VA_ARGS__))
#define __FE_7(f, x, ...) f(x); E(__FE_6(f, __VA_ARGS__))
#define __FE_8(f, x, ...) f(x); E(__FE_7(f, __VA_ARGS__))
#define __FE_9(f, x, ...) f(x); E(__FE_8(f, __VA_ARGS__))
#define __FE_10(f, x, ...) f(x); E(__FE_9(f, __VA_ARGS__))
#define __FE_11(f, x, ...) f(x); E(__FE_10(f, __VA_ARGS__))
#define __FE_12(f, x, ...) f(x); E(__FE_11(f, __VA_ARGS__))
#define __FE_13(f, x, ...) f(x); E(__FE_12(f, __VA_ARGS__))
#define __FE_14(f, x, ...) f(x); E(__FE_13(f, __VA_ARGS__))
#define __FE_15(f, x, ...) f(x); E(__FE_14(f, __VA_ARGS__))
#define __FE_16(f, x, ...) f(x); E(__FE_15(f, __VA_ARGS__))
#define __FE_17(f, x, ...) f(x); E(__FE_16(f, __VA_ARGS__))
#define __FE_18(f, x, ...) f(x); E(__FE_17(f, __VA_ARGS__))
#define __FE_19(f, x, ...) f(x); E(__FE_18(f, __VA_ARGS__))
#define __FE_20(f, x, ...) f(x); E(__FE_19(f, __VA_ARGS__))
#define __FE_21(f, x, ...) f(x); E(__FE_20(f, __VA_ARGS__))
#define __FE_22(f, x, ...) f(x); E(__FE_21(f, __VA_ARGS__))
#define __FE_23(f, x, ...) f(x); E(__FE_22(f, __VA_ARGS__))
#define __FE_24(f, x, ...) f(x); E(__FE_23(f, __VA_ARGS__))
#define __FE_25(f, x, ...) f(x); E(__FE_24(f, __VA_ARGS__))
#define __FE_26(f, x, ...) f(x); E(__FE_25(f, __VA_ARGS__))
#define __FE_27(f, x, ...) f(x); E(__FE_26(f, __VA_ARGS__))
#define __FE_28(f, x, ...) f(x); E(__FE_27(f, __VA_ARGS__))
#define __FE_29(f, x, ...) f(x); E(__FE_28(f, __VA_ARGS__))
#define __FE_30(f, x, ...) f(x); E(__FE_29(f, __VA_ARGS__))
#define __FE_31(f, x, ...) f(x); E(__FE_30(f, __VA_ARGS__))
#define __FE_32(f, x, ...) f(x); E(__FE_31(f, __VA_ARGS__))
#define __FE_33(f, x, ...) f(x); E(__FE_32(f, __VA_ARGS__))
#define __FE_34(f, x, ...) f(x); E(__FE_33(f, __VA_ARGS__))
#define __FE_35(f, x, ...) f(x); E(__FE_34(f, __VA_ARGS__))
#define __FE_36(f, x, ...) f(x); E(__FE_35(f, __VA_ARGS__))
#define __FE_37(f, x, ...) f(x); E(__FE_36(f, __VA_ARGS__))
#define __FE_38(f, x, ...) f(x); E(__FE_37(f, __VA_ARGS__))
#define __FE_39(f, x, ...) f(x); E(__FE_38(f, __VA_ARGS__))
#define __FE_40(f, x, ...) f(x); E(__FE_39(f, __VA_ARGS__))
#define __FE_41(f, x, ...) f(x); E(__FE_40(f, __VA_ARGS__))
#define __FE_42(f, x, ...) f(x); E(__FE_41(f, __VA_ARGS__))
#define __FE_43(f, x, ...) f(x); E(__FE_42(f, __VA_ARGS__))
#define __FE_44(f, x, ...) f(x); E(__FE_43(f, __VA_ARGS__))
#define __FE_45(f, x, ...) f(x); E(__FE_44(f, __VA_ARGS__))
#define __FE_46(f, x, ...) f(x); E(__FE_45(f, __VA_ARGS__))
#define __FE_47(f, x, ...) f(x); E(__FE_46(f, __VA_ARGS__))
#define __FE_48(f, x, ...) f(x); E(__FE_47(f, __VA_ARGS__))
#define __FE_49(f, x, ...) f(x); E(__FE_48(f, __VA_ARGS__))
#define __FE_50(f, x, ...) f(x); E(__FE_49(f, __VA_ARGS__))
#define __FE_51(f, x, ...) f(x); E(__FE_50(f, __VA_ARGS__))
#define __FE_52(f, x, ...) f(x); E(__FE_51(f, __VA_ARGS__))
#define __FE_53(f, x, ...) f(x); E(__FE_52(f, __VA_ARGS__))
#define __FE_54(f, x, ...) f(x); E(__FE_53(f, __VA_ARGS__))
#define __FE_55(f, x, ...) f(x); E(__FE_54(f, __VA_ARGS__))
#define __FE_56(f, x, ...) f(x); E(__FE_55(f, __VA_ARGS__))
#define __FE_57(f, x, ...) f(x); E(__FE_56(f, __VA_ARGS__))
#define __FE_58(f, x, ...) f(x); E(__FE_57(f, __VA_ARGS__))
#define __FE_59(f, x, ...) f(x); E(__FE_58(f, __VA_ARGS__))
#define __FE_60(f, x, ...) f(x); E(__FE_59(f, __VA_ARGS__))
#define __FE_61(f, x, ...) f(x); E(__FE_60(f, __VA_ARGS__))
#define __FE_62(f, x, ...) f(x); E(__FE_61(f, __VA_ARGS__))
#define __FE_63(f, x, ...) f(x); E(__FE_62(f, __VA_ARGS__))
#define __FE_64(f, x, ...) f(x); E(__FE_63(f, __VA_ARGS__))

#define CONCATENATE(arg1, arg2)   CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2)  CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2)  arg1##arg2
#define __FE_N(N, f, ...) E(CONCATENATE(__FE_, N)(f, __VA_ARGS__))
#define FOR_EACH(f, ...) E(__FE_N(VA_COUNT(__VA_ARGS__), f, __VA_ARGS__))

#ifndef SIMPLE_WEB_CRYPTO_HPP
#define SIMPLE_WEB_CRYPTO_HPP

#include <cmath>
#include <iomanip>
#include <istream>
#include <sstream>
#include <string>
#include <vector>
#include "sha1.hpp"
#include "base64.h"

namespace SimpleWeb {
// TODO 2017: remove workaround for MSVS 2012
#if _MSC_VER == 1700                       // MSVS 2012 has no definition for round()
  inline double round(double x) noexcept { // Custom definition of round() for positive numbers
    return floor(x + 0.5);
  }
#endif

  class Crypto {
    const static std::size_t buffer_size = 131072;

  public:
    class Base64 {
    public:
      static std::string encode(const std::string &ascii) noexcept {
		  std::string r;
		  ::Base64::Encode(ascii, &r);
		  return r;
      }

      static std::string decode(const std::string &base64) noexcept {
		  std::string r;
		  ::Base64::Decode(base64, &r);
		  return r;
      }
    };

    static std::string sha1(const std::string &input) noexcept {
	  SHA1 sha;
	  sha.update(input);
	  return sha.final();
    }

    static std::string sha1(std::istream &stream) noexcept {
	  SHA1 sha;
	  sha.update(stream);
	  return sha.final();
    }

  };
}
#endif /* SIMPLE_WEB_CRYPTO_HPP */

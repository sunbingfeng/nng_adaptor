#ifndef NNG_ADAPTOR_MD5_TRAITS_H_
#define NNG_ADAPTOR_MD5_TRAITS_H_

#include <google/protobuf/message.h>
#include <hash-library/md5.h>
#include <cstdlib>
#include <optional>

namespace nng_adaptor {
constexpr size_t k_MD5_SIZE = 32;

template <class M>
struct Md5Traits {
  static std::string md5_value() {
    static_assert(std::is_base_of<::google::protobuf::Message, M>::value,
                  "non-supported message type!");
    static std::optional<std::string> md5_str;
    if (!md5_str.has_value()) {
      MD5 md5;
      md5_str =
          md5(M::descriptor()
                  ->DebugString());  // Calling `DebugString` will obtain an
                                     // simple topology definition of this
                                     // protobuf message. This md5 calculation
                                     // method may be not robust enough under
                                     // some corner cases, and we will implement
                                     // it in a more elegant way in the future.
    }
    return md5_str.value();
  }
};

}  // namespace nng_adaptor
#endif

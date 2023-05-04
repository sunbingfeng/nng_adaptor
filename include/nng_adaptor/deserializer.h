#ifndef NNG_ADAPTOR_DESERIALIZER_H_
#define NNG_ADAPTOR_DESERIALIZER_H_

#include <google/protobuf/message.h>
#include <cstdlib>

namespace nng_adaptor {
template <class M>
M FromString(const std::string& str) {
  static_assert(std::is_base_of<::google::protobuf::Message, M>::value,
		"non-supported message type!");

  M m;
  m.ParseFromString(str);
  return m;
}
}  // namespace nng_adaptor
#endif

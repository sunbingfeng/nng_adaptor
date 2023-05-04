#ifndef NNG_ADAPTOR_SERIALIZER_H_
#define NNG_ADAPTOR_SERIALIZER_H_

#include <google/protobuf/message.h>
#include <cstdlib>

namespace nng_adaptor {
template <class M>
std::string ToString(const M& m) {
  static_assert(std::is_base_of<::google::protobuf::Message, M>::value,
                "non-supported message type!");

  std::string data;
  m.SerializeToString(&data);
  return data;
}
}  // namespace nng_adaptor
#endif

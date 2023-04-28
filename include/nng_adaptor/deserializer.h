#ifndef NNG_ADAPTOR_DESERIALIZER_H_
#define NNG_ADAPTOR_DESERIALIZER_H_

#include <cstdlib>

template <class M>
M FromString(const std::string& str) {
  return M();
}

template <>
float FromString<float>(const std::string& str){
  return atof(str.c_str());
}

template <>
int FromString<int>(const std::string& str){
  return atoi(str.c_str());
}

#endif

#ifndef NNG_ADAPTOR_H_
#define NNG_ADAPTOR_H_

#include <nng/nng.h>
#include <nng/supplemental/util/platform.h>
#include <nng/protocol/pubsub0/pub.h>
#include <functional>
#include <iostream>
#include <string>

namespace nng_adaptor {
template <typename M>
class Publisher {
 public:
  using complete_callback = std::function<void(int)>;
  Publisher(nng_socket socket, const std::string topic)
      : socket_(socket), topic_(topic) {}

  // Async interface
  void Publish(const M& msg_entity, const complete_callback& comp_cb) {}

  void Publish(const M& msg_entity) {
    int rv;
    nng_msg* msg;
    if ((rv = nng_msg_alloc(&msg, 0)) != 0) {
      std::cout << __func__ << "==>"
                << "nng_msg_alloc failed, " << nng_strerror(rv) << std::endl;
    }

    // Add topic header
    if ((rv = nng_msg_append(msg, topic_.c_str(), topic_.size() + 1)) != 0) {
      std::cout << __func__ << "==>"
                << "nng_msg_append failed, " << nng_strerror(rv) << std::endl;
    }

    std::string body = std::to_string(msg_entity);
    size_t size = body.size();
    nng_msg_append_u32(msg,size);
    nng_msg_append(msg, body.c_str(), size);

    if ((rv = nng_sendmsg(socket_, msg, 0)) != 0) {
      std::cout << __func__ << "==>"
                << "nng_sendmsg failed, " << nng_strerror(rv) << std::endl;
    }
  }

 private:
  const std::string url_;
  const std::string topic_;

  nng_socket socket_;
};

class NodeHandler {
 private:
  nng_socket socket_;
  nng_listener listener_;

 public:
  NodeHandler() = default;
  ~NodeHandler() = default;

  NodeHandler(const std::string& url) {
    /*  Create the socket. */
    int rv = nng_pub0_open(&socket_);
    if (rv != 0) {
      // fatal("nng_pub0_open", rv);
      std::cout << __func__ << "==>"
                << "nng_pub0_open failed, " << nng_strerror(rv) << std::endl;
    }

    if ((rv = nng_listener_create(&listener_, socket_, url.c_str())) != 0) {
      // fatal("nng_listener_create", rv);
      std::cout << __func__ << "==>"
                << "nng_listener_create failed, " << nng_strerror(rv) << std::endl;
    }

    if ((rv = nng_listener_start(listener_, 0)) != 0) {
      // fatal("nng_listener_start", rv);
      std::cout << __func__ << "==>"
                << "nng_listener_start failed, " << nng_strerror(rv) << std::endl;
    }
  }

  template <typename M>
  Publisher<M> CreatePublisher(std::string topic) {
    return Publisher<M>(socket_, topic);
  }
};

}  // namespace nng_adaptor
#endif

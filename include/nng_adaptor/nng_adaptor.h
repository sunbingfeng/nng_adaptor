#ifndef NNG_ADAPTOR_H_
#define NNG_ADAPTOR_H_
#include <nng_adaptor/deserializer.h>
#include <nng_adaptor/md5_traits.h>
#include <nng_adaptor/serializer.h>

#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/protocol/pubsub0/sub.h>
#include <nng/supplemental/util/platform.h>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <utility>

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

    // Add the md5 value
    std::string md5 = Md5Traits<M>::md5_value();
    nng_msg_append(msg, md5.c_str(), k_MD5_SIZE);

    std::string body = ToString(msg_entity);
    size_t size = body.size();
    nng_msg_append_u32(msg, size);
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

  struct SubscribeCallHelper {
    virtual void call(const std::string&, const std::string&) = 0;
  };

  template <class P>
  class SubscribeCallHelperT : public SubscribeCallHelper {
   public:
    typedef typename std::remove_reference<
        typename std::remove_const<P>::type>::type Message;
    typedef const std::shared_ptr<Message const> Parameter;

    typedef std::function<void(Parameter)> callback_t;

    SubscribeCallHelperT(const callback_t& callback) : callback_(callback){};

    void call(const std::string& msg_md5, const std::string& msg_str) override {
      if (msg_md5 != md5) {
        std::cout << __func__ << "==>non matched msg received!"
                  << "recved md5: " << msg_md5 << ", expected md5: " << md5
                  << std::endl;
        return;
      }

      Parameter param = std::make_shared<Message const>(FromString<P>(msg_str));
      callback_(param);
    }
    // callback_t GetCallback() const { return callback_; }

   private:
    callback_t callback_;

    const std::string md5 = Md5Traits<Message>::md5_value();
  };

  struct SubsriberOptions {
    std::string topic;
    std::shared_ptr<SubscribeCallHelper> ptr_sub_call_helper;

    nng_socket sock;
    nng_aio* recv_aio;
    nng_dialer dialer;

    SubsriberOptions(const std::string& _topic) : topic(_topic) {}

    void InitComm(const std::string& url) {
      int rv;
      if ((rv = nng_sub0_open(&sock)) != 0) {
        std::cout << __func__ << "==>"
                  << "nng_sub0_open failed, " << nng_strerror(rv) << std::endl;
      }

      if ((rv = nng_socket_set(sock, NNG_OPT_SUB_SUBSCRIBE, topic.c_str(),
                               topic.size())) != 0) {
        // fatal("nng_socket_set", rv);
      }

      if ((rv = nng_dialer_create(&dialer, sock, url.c_str())) != 0) {
        // fatal("nng_dialer_create", rv);
        std::cout << __func__ << "==>"
                  << "nng_dialer_create failed, " << nng_strerror(rv)
                  << std::endl;
      }

      if ((rv = nng_dialer_start(dialer, 0)) != 0) {
        // fatal("nng_dialer_start", rv);
        std::cout << __func__ << "==>"
                  << "nng_dialer_start failed, " << nng_strerror(rv)
                  << std::endl;
      }
    }

    template <class P>
    void InitCallback(
        const typename SubscribeCallHelperT<P>::callback_t& callback) {
      ptr_sub_call_helper =
          std::make_shared<SubscribeCallHelperT<P> >(callback);
    }

    size_t topic_size() const { return topic.size() + 1 /*\0 terminator*/; }
  };

  std::map<std::string, SubsriberOptions> topic_to_ops_;

 public:
  NodeHandler() = default;
  ~NodeHandler() {
    std::vector<std::string> all_topics;
    for (auto& sub : topic_to_ops_) {
      all_topics.push_back(sub.first);
    }

    for (auto& topic : all_topics) {
      Unsubscribe(topic);
    }
  }

  struct Package {
    std::string topic;
    size_t body_size;
    std::string body;

    bool is_valid() const { return topic != "" && (body.size() == body_size); }
  };

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
                << "nng_listener_create failed, " << nng_strerror(rv)
                << std::endl;
    }

    if ((rv = nng_listener_start(listener_, 0)) != 0) {
      // fatal("nng_listener_start", rv);
      std::cout << __func__ << "==>"
                << "nng_listener_start failed, " << nng_strerror(rv)
                << std::endl;
    }
  }

  static void default_callback(void* args) {
    SubsriberOptions* ops = static_cast<SubsriberOptions*>(args);

    int rv;
    if ((rv = nng_aio_result(ops->recv_aio)) != 0) {
      // fatal("nng_aio_result", rv);
      nng_recv_aio(ops->sock, ops->recv_aio);
      return;
    }

    nng_msg* msg = nng_aio_get_msg(ops->recv_aio);
    size_t real_len = nng_msg_len(msg);
    nng_msg_trim(msg, ops->topic.size() + 1);

    char* msg_body = (char*)nng_msg_body(msg);
    std::string md5(msg_body, k_MD5_SIZE);
    nng_msg_trim(msg, k_MD5_SIZE);

    uint32_t msg_content_sz;
    nng_msg_trim_u32(msg, &msg_content_sz);
    size_t expected_len = ops->topic_size() + k_MD5_SIZE +
                          4 /*len of 'size element'*/ + msg_content_sz;
    if (real_len != expected_len) {
      std::cout << __func__ << "==>"
                << "broken msg received!" << std::endl;
      nng_recv_aio(ops->sock, ops->recv_aio);
      return;
    }

    char* msg_content = (char*)nng_msg_body(msg);
    std::string content_str(msg_content, msg_content_sz);

    ops->ptr_sub_call_helper->call(md5, content_str);

    nng_recv_aio(ops->sock, ops->recv_aio);
  }

  template <typename M>
  Publisher<M> CreatePublisher(std::string topic) {
    return Publisher<M>(socket_, topic);
  }

  template <typename M>
  void Subscribe(
      const std::string& url, std::string topic,
      const std::function<void(const std::shared_ptr<M const>)> callback) {
    auto result = topic_to_ops_.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(topic),
                                        std::forward_as_tuple(topic));
    if (!result.second) {
      std::cerr << __func__ << "==>"
                << "Only one subscriber is allowed for one topic!" << std::endl;
      return;
    }

    std::cout << __func__ << "==>"
              << "topic: " << topic << ", url: " << url << std::endl;
    int rv;
    SubsriberOptions& ops = result.first->second;
    ops.InitComm(url);

    ops.InitCallback<M>(callback);
    if ((rv = nng_aio_alloc(&(ops.recv_aio), default_callback, &ops)) != 0) {
      // fatal("nng_aio_alloc", rv);
      std::cout << __func__ << "==>"
                << "nng_aio_alloc failed, " << nng_strerror(rv) << std::endl;
    }

    nng_recv_aio(ops.sock, ops.recv_aio);
  }

  bool Unsubscribe(const std::string topic) {
    if (!topic_to_ops_.count(topic)) {
      std::cout << __func__ << "==>"
                << "Please subscribe first!" << std::endl;
      return false;
    }

    auto& ops = topic_to_ops_.at(topic);
    nng_socket_set(ops.sock, NNG_OPT_SUB_UNSUBSCRIBE, topic.c_str(),
                   topic.size());
    nng_aio_stop(ops.recv_aio);
    nng_dialer_close(ops.dialer);

    topic_to_ops_.erase(topic);

    return true;
  }
};

}  // namespace nng_adaptor
#endif

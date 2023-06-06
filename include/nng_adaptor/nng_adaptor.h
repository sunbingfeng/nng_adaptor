#ifndef NNG_ADAPTOR_H_
#define NNG_ADAPTOR_H_
#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/protocol/pubsub0/sub.h>
#include <nng/supplemental/util/platform.h>
#include <nng_adaptor/deserializer.h>
#include <nng_adaptor/md5_traits.h>
#include <nng_adaptor/serializer.h>

#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <utility>

#include "thread"

namespace nng_adaptor {
template <typename M>
class Publisher {
 public:
  using complete_callback = std::function<void(int)>;

  // Use external provided socket
  Publisher(const std::string& topic, nng_socket socket)
      : topic_(topic), socket_(socket) {}

  // Manage the socket independently, and bind it to the specified url.
  Publisher(const std::string& url, const std::string topic)
      : url_(url), topic_(topic) {
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
  nng_listener listener_;
};

class NodeHandler {
 private:
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
    std::vector<std::shared_ptr<SubscribeCallHelper>> callers;

    nng_socket sock;
    nng_aio* recv_aio;
    nng_dialer dialer;

    std::mutex cb_mutex;

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
    void AddCallback(
        const typename SubscribeCallHelperT<P>::callback_t& callback) {
      const std::lock_guard<std::mutex> lock(cb_mutex);
      callers.push_back(std::make_shared<SubscribeCallHelperT<P>>(callback));
    }

    size_t topic_size() const { return topic.size() + 1 /*\0 terminator*/; }
  };

  std::map<std::string, SubsriberOptions> topic_to_ops_;

  std::string publish_url_;
  nng_listener publish_listener_;
  nng_socket publish_socket_;

  void InitPublishSocket() {
    /*  Create the socket. */
    int rv = nng_pub0_open(&publish_socket_);
    if (rv != 0) {
      // fatal("nng_pub0_open", rv);
      std::cout << __func__ << "==>"
                << "nng_pub0_open failed, " << nng_strerror(rv) << std::endl;
    }

    if ((rv = nng_listener_create(&publish_listener_, publish_socket_,
                                  publish_url_.c_str())) != 0) {
      // fatal("nng_listener_create", rv);
      std::cout << __func__ << "==>"
                << "nng_listener_create failed, " << nng_strerror(rv)
                << std::endl;
    }

    if ((rv = nng_listener_start(publish_listener_, 0)) != 0) {
      // fatal("nng_listener_start", rv);
      std::cout << __func__ << "==>"
                << "nng_listener_start failed, " << nng_strerror(rv)
                << std::endl;
    }
  }

 public:
  NodeHandler() = default;
  NodeHandler(const std::string& pub_url) : publish_url_(pub_url) {
    InitPublishSocket();
  }

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

    {
      const std::lock_guard<std::mutex> lock(ops->cb_mutex);
      for (const auto cb : ops->callers) {
        cb->call(md5, content_str);
      }
    }

    nng_recv_aio(ops->sock, ops->recv_aio);
  }

  template <typename M>
  Publisher<M> Advertise(const std::string& url, const std::string& topic) {
    return Publisher<M>(url, topic);
  }

  template <typename M>
  Publisher<M> Advertise(const std::string& topic) {
    return Publisher<M>(topic, publish_socket_);
  }

  template <typename M>
  void Subscribe(
      const std::string& url, std::string topic,
      const std::function<void(const std::shared_ptr<M const>)> callback) {
    auto result = topic_to_ops_.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(topic),
                                        std::forward_as_tuple(topic));
    int rv;
    if (result.second) {
      // Initialize the comm for newly topic subscription
      std::cout << __func__ << "==>"
                << "topic: " << topic << ", url: " << url << std::endl;
      SubsriberOptions& ops = result.first->second;
      ops.InitComm(url);

      if ((rv = nng_aio_alloc(&(ops.recv_aio), default_callback, &ops)) != 0) {
        // fatal("nng_aio_alloc", rv);
        std::cout << __func__ << "==>"
                  << "nng_aio_alloc failed, " << nng_strerror(rv) << std::endl;
      }

      nng_recv_aio(ops.sock, ops.recv_aio);
    }

    SubsriberOptions& ops = result.first->second;
    ops.AddCallback<M>(callback);
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

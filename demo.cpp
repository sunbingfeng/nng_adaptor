#include <cstdio>
#include <memory>

#include <nng_adaptor/nng_adaptor.h>

int main(int argc, char** argv) {
  nng_adaptor::NodeHandler handler("ipc:///tmp/async_demo");

  auto pub = handler.CreatePublisher<int>("int_topic");
  auto pub2 = handler.CreatePublisher<float>("float_topic");

  for (;;) {
    pub.Publish(10);
    nng_msleep(1000);  // neither pause() nor sleep() portable
    pub2.Publish(0.3);
    nng_msleep(1000);  // neither pause() nor sleep() portable
  }

  return 0;
}

#include <cstdio>
#include <memory>

#include <nng_adaptor/nng_adaptor.h>

int main(int argc, char** argv) {
  nng_adaptor::NodeHandler handler("ipc:///tmp/async_demo");

  auto pub = handler.CreatePublisher<int>("int_topic");
  auto pub2 = handler.CreatePublisher<float>("float_topic");
  handler.Subscribe<int>(
      "ipc:///tmp/async_demo", "int_topic", [](auto value) {
        std::cout << "main==>received value: " << *value << std::endl;
      });

  int count = 0;
  for (;;) {
    pub.Publish(count++);
    nng_msleep(1000);  // neither pause() nor sleep() portable
    pub2.Publish(0.3);
    // nng_msleep(1000);  // neither pause() nor sleep() portable
  }

  return 0;
}

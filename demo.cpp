#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <memory>

#include <nng_adaptor/nng_adaptor.h>
#include <proto/TestMsg.pb.h>

int main(int argc, char** argv) {
  std::srand(
      std::time(nullptr));  // use current time as seed for random generator
  nng_adaptor::NodeHandler handler("ipc:///tmp/async_demo");

  auto pub3 = handler.CreatePublisher<nng_adaptor::test::Pose>("testmsg_topic");
  handler.Subscribe<nng_adaptor::test::Pose>(
      "ipc:///tmp/async_demo", "testmsg_topic", [](auto value) {
        std::cout << "main==>received pose: [" << value->x() << ", "
                  << value->y() << ", " << value->yaw() << "]" << std::endl;
      });

  int count = 0;
  for (;;) {
    nng_adaptor::test::Pose p;
    float x = 1e-3 * (std::rand() % 1000), y = 1e-3 * (std::rand() % 1000);
    float yaw = M_PI * (std::rand() % 180) / 180.0;

    std::cout << __func__ << "==>"
              << "Try to publish pose: [" << x << ", " << y << ", " << yaw
              << "]" << std::endl;
    p.set_x(x);
    p.set_y(y);
    p.set_yaw(yaw);
    pub3.Publish(p);
    nng_msleep(1000);  // neither pause() nor sleep() portable
  }

  return 0;
}

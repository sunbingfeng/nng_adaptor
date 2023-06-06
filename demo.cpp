#include <nng_adaptor/md5_traits.h>
#include <nng_adaptor/nng_adaptor.h>
#include <proto/TestMsg.pb.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <memory>

class TestClass {
public:
  TestClass() = default;
  ~TestClass() = default;

  void OnPoseRecved(const std::shared_ptr<nng_adaptor::test::Pose const>& p) {
    std::cout << __func__ << "==>"
              << "received pose: [" << p->x() << ", " << p->y() << ", "
              << p->yaw() << "]" << std::endl;
  }
};

int main(int argc, char** argv) {
  std::srand(
      std::time(nullptr));  // use current time as seed for random generator
  nng_adaptor::NodeHandler handler("ipc:///tmp/async_demo");

  // Use mannual specified pub url
  auto pub1 = handler.Advertise<nng_adaptor::test::IMU>("tcp://127.0.0.1:1999",
                                                        "imu_topic");

  // Use default pub url of handler
  auto pub3 = handler.Advertise<nng_adaptor::test::Pose>("pose_topic");

  // Use class member function as callback
  TestClass test_class;
  handler.Subscribe<nng_adaptor::test::Pose>(
      "ipc:///tmp/async_demo", "pose_topic",
      std::bind(&TestClass::OnPoseRecved, &test_class, std::placeholders::_1));

  // Use lambda function as callback
  handler.Subscribe<nng_adaptor::test::Pose>(
      "ipc:///tmp/async_demo", "pose_topic", [](auto value) {
        std::cout << "main==>received pose: [" << value->x() << ", "
                  << value->y() << ", " << value->yaw() << "]" << std::endl;
      });

  handler.Subscribe<nng_adaptor::test::IMU>(
      "tcp://127.0.0.1:1999", "imu_topic",
      [](auto value) { std::cerr << "imu: " << value->ax() << std::endl; });

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

    nng_adaptor::test::IMU imu;
    imu.set_wx(0.1);
    imu.set_wy(0.2);
    imu.set_wz(0.3);

    imu.set_ax(0.1);
    imu.set_ay(0.2);
    imu.set_az(0.3);

    pub1.Publish(imu);

    nng_msleep(1000);  // neither pause() nor sleep() portable
  }

  return 0;
}

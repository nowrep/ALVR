#include "display.hpp"

#include"layer/settings.h"

#include <chrono>

#include <poll.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>

wsi::display::display()
{
    m_socket = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
}

VkFence wsi::display::get_vsync_fence()
{
  if (not std::atomic_exchange(&m_thread_running, true))
  {
  vsync_fence = reinterpret_cast<VkFence>(this);
  m_vsync_thread = std::thread([this]()
      {
          std::string socket_path = getenv("XDG_RUNTIME_DIR");
          socket_path += "/alvr-ipc:vsync";

          struct sockaddr_un name;
          memset(&name, 0, sizeof(name));
          name.sun_family = AF_UNIX;
          strncpy(name.sun_path, socket_path.c_str(), sizeof(name.sun_path) - 1);

          while (connect(m_socket, (const struct sockaddr *)&name, sizeof(name)) == -1) {
              if (m_exiting) {
                  return;
              }
              using namespace std::chrono_literals;
              std::this_thread::sleep_for(1ms);
          }

          struct pollfd fd;
          fd.fd = m_socket;
          fd.events = POLLIN;
          while (!m_exiting) {
              int ret = poll(&fd, 1, 100);
              if (ret <= 0) {
                  continue;
              }
              uint8_t m;
              if (read(m_socket, &m, sizeof(m)) == sizeof(m)) {
                  m_signaled = true;
                  m_cond.notify_all();
                  m_vsync_count += 1;
              }
          }
      });
  }
  m_signaled = false;
  return vsync_fence;
}

wsi::display::~display() {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_exiting = true;
  if (m_vsync_thread.joinable())
    m_vsync_thread.join();
}

bool wsi::display::wait_for_vsync(uint64_t timeoutNs)
{
  if (!m_signaled) {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_cond.wait_for(lock, std::chrono::nanoseconds(timeoutNs)) == std::cv_status::no_timeout;
  }
  return true;
}

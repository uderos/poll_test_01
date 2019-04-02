#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream> // debug purpose only
#include <memory>
#include <sstream> // debug purpose only

#include <poll.h>
#include <unistd.h>

#include "poll_lib.h"

#define DBG_OUT if (m_dbgout_enabled) std::cout << __FUNCTION__ << ":" << __LINE__ << ' '

namespace udr {
namespace poll_lib {

static constexpr std::size_t DEFAULT_READ_BUFFER_SIZE = 4096;

static std::string f_poll_events_to_string(const int flags)
{
  std::ostringstream oss;
  if (flags &  POLLIN)            oss << " POLLIN";
  if (flags &  POLLPRI)           oss << " POLLPRI";
  if (flags &  POLLOUT)           oss << " POLLOUT";
  if (flags &  POLLERR)           oss << " POLLERR";
  if (flags &  POLLHUP)           oss << " POLLHUP";
  if (flags &  POLLNVAL)          oss << " POLLNVAL";
  if (flags &  POLLRDNORM)        oss << " POLLRDNORM";
  if (flags &  POLLRDBAND)        oss << " POLLRDBAND";
  if (flags &  POLLWRNORM)        oss << " POLLWRNORM";
  if (flags &  POLLWRBAND)        oss << " POLLWRBAND";

  return oss.str();
}

Utils::Utils() : 
  m_read_buffer_size(DEFAULT_READ_BUFFER_SIZE),
  m_dbgout_enabled(true)
{
}

std::size_t Utils::set_read_buffer_size(const std::size_t new_size)
{
  const auto old_size = m_read_buffer_size;
  m_read_buffer_size = new_size;
  return old_size;
}

bool Utils::enable_dbgout()
{
  const auto old_value = m_dbgout_enabled;
  m_dbgout_enabled = true;
  return old_value;
}

bool Utils::disable_dbgout()
{
  const auto old_value = m_dbgout_enabled;
  m_dbgout_enabled = false;
  return old_value;
}

eReadResult Utils::read_single_shot(
    const int fd, 
    const uint64_t timeout_ms, 
    std::string & out_buffer) const
{
  const auto end_time_abs = util_clock_t::now() + ms_t(timeout_ms);

  pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fd;
  pfd.events = POLLIN;

  for(;;)
  {
    const int poll_timeout_ms = m_calculate_poll_timeout_ms(end_time_abs);
    const int poll_rc = poll(&pfd, 1, poll_timeout_ms);
    DBG_OUT << " poll_rc=" << poll_rc << " revents=" 
            << f_poll_events_to_string(pfd.revents) << std::endl;
    
    if (poll_rc > 0)
    {
      if (pfd.revents & POLLIN)
      {
        const eReadResult read_rc = m_read_after_poll(fd, out_buffer);

        return read_rc;
      }
      else if (pfd.revents & POLLHUP)
      {
        return RR_EOF;
      }
      else // if (pfd.revents & (POLLERR | POLLNVAL))
      {
        return RR_ERROR_POLL;
      }
    }
    else if (poll_rc == 0) // timeout expired
    {
      return RR_TIMEOUT;
    }
    else if ((poll_rc == EINTR) || (poll_rc == EAGAIN))
    {
      ; // keep trying
    }
    else // poll_rc < 0
    {
      perror("poll() FAILURE: ");
      return RR_ERROR_POLL;
    }
  }

  return RR_SW_ERROR;
}


eReadResult Utils::read_by_size(
    const int fd, 
    const std::uint64_t data_size,
    const uint64_t timeout_ms, 
    std::string & out_buffer) const
{
  const auto end_time_abs = util_clock_t::now() + ms_t(timeout_ms);

  pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fd;
  pfd.events = POLLIN;

  while (out_buffer.size() < data_size)
  {
    const int poll_timeout_ms = m_calculate_poll_timeout_ms(end_time_abs);
    const int poll_rc = poll(&pfd, 1, poll_timeout_ms);
    DBG_OUT << " poll_rc=" << poll_rc << " revents=" 
            << f_poll_events_to_string(pfd.revents) << std::endl;
    
    if (poll_rc > 0)
    {
      if (pfd.revents & POLLIN)
      {
        const eReadResult read_rc = m_read_after_poll(fd, out_buffer);

        if (read_rc != RR_OKAY)
          return read_rc;
      }
      else if (pfd.revents & POLLHUP)
      {
        return RR_EOF;
      }
      else // if (pfd.revents & (POLLERR | POLLNVAL))
      {
        return RR_ERROR_POLL;
      }
    }
    else if (poll_rc == 0) // timeout expired
    {
      return RR_TIMEOUT;
    }
    else if ((poll_rc == EINTR) || (poll_rc == EAGAIN))
    {
      ; // keep trying
    }
    else // poll_rc < 0
    {
      perror("poll() FAILURE: ");
      return RR_ERROR_POLL;
    }
  }

  if (out_buffer.size() >= data_size)
    return RR_OKAY;

  return RR_SW_ERROR;
}

int Utils::m_calculate_poll_timeout_ms(const time_point_t & end_time_abs) const
{
  int timeout_ms = 0;

  const auto now = util_clock_t::now();
  if (now < end_time_abs)
  {
    const auto remaining_time = end_time_abs - now;
    const auto remaining_time_ms = std::chrono::duration_cast<ms_t>(remaining_time).count();
    timeout_ms = static_cast<int>(remaining_time_ms); // Needs check to ensure it fits
  }

  return timeout_ms;
}

eReadResult Utils::m_read_after_poll(const int fd, 
                                     std::string & out_buffer) const
{
  eReadResult result = RR_OKAY;

  auto read_buff_ptr = std::make_unique<char[]>(m_read_buffer_size);

  const int read_rc = read(fd, read_buff_ptr.get(), m_read_buffer_size);
  DBG_OUT << " read_rc=" << read_rc << std::endl;

  if (read_rc > 0)
  {
    out_buffer.append(read_buff_ptr.get(), read_rc); 
  }
  else if (read_rc == 0) // EOF
  {
    result = RR_EOF;
  }
  else if (read_rc == EINTR)
  {
    ; // keep trying
  }
  else // read_rc < 0
  {
    perror("read() FAILURE: ");
    result = RR_ERROR_READ;
  }

  return result;
}

} // namespace poll_lib
} // namespace udr


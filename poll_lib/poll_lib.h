#ifndef UDR_POLL_LIB_H
#define UDR_POLL_LIB_H

#include <chrono>
#include <cstdint>
#include <string>

namespace udr {
namespace poll_lib {

enum eReadResult
{
  RR_OKAY,
  RR_TIMEOUT,
  RR_EOF,
  RR_ERROR_POLL,
  RR_ERROR_READ,
  RR_SW_ERROR
};

class Utils
{

  public:

    Utils();
    virtual ~Utils() = default;

    eReadResult read_single_shot(const int fd, 
                                 const uint64_t timeout_ms, 
                                 std::string & out_buffer) const;

    eReadResult read_by_size(const int fd, 
                             const std::uint64_t data_size,
                             const uint64_t timeout_ms, 
                             std::string & out_buffer) const;

    std::size_t set_read_buffer_size(const std::size_t new_size);

    bool enable_dbgout();
    bool disable_dbgout();

  private:

    using util_clock_t = std::chrono::steady_clock;
    using time_point_t = std::chrono::time_point<util_clock_t>;
    using ms_t = std::chrono::milliseconds;

    std::size_t m_read_buffer_size;
    bool m_dbgout_enabled;

    int m_calculate_poll_timeout_ms(const time_point_t & end_time_abs) const;
    eReadResult m_read_after_poll(const int fd, std::string & out_buffer) const;

};

} // namespace poll_lib
} // namespace udr

#endif // UDR_POLL_LIB_H

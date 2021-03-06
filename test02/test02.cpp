#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

//#include <sys/types>
#include <unistd.h>

#include "poll_lib/poll_lib.h"

#define DBG_OUT std::cout << "T=" << std::chrono::duration_cast<ms_t>(cpp_clock_t::now() - START_TIME).count() \
                          << ' ' << __FUNCTION__ << ":" << __LINE__ \
                          << ' ' 

namespace pl = udr::poll_lib;

using cpp_clock_t = std::chrono::steady_clock;
using ms_t = std::chrono::milliseconds;

static const auto START_TIME = cpp_clock_t::now();

struct write_step_t
{
  ms_t delta_t_ms;
  std::string data;
};

using write_list_t = std::vector<write_step_t>;

static void f_writer_thread(const int fd, const write_list_t write_list)
{
  //DBG_OUT << "BEGIN" << std::endl;

  for (const auto & w : write_list)
  {
    std::this_thread::sleep_for(w.delta_t_ms);
    if (!w.data.empty())
    {
      //DBG_OUT << "Writing " << w.data.size() << " bytes ..." << std::endl;
      const int rc = write(fd, w.data.c_str(), w.data.size());
      if (rc < 0) 
      {
        perror("write() failure: ");
        return;
      }
    }
  }
  //DBG_OUT << "THE-END" << std::endl;
  close(fd);
}

static void f_test_rdbuffer(const std::size_t data_size, const std::size_t rd_buffer_size)
{
  int pipefd[2];
  const int pipe_rc = pipe(pipefd);
  if (pipe_rc < 0)
  {
    perror("pipe() failure: ");
    return;
  }

  std::string data;
  data.reserve(data_size);
  for (std::size_t i = 0; i < data_size; ++i)
    data.push_back('0' + (i % 10));

  const write_list_t write_list { { ms_t(200), data } };

  std::thread writer_thread(f_writer_thread, pipefd[1], write_list);
  std::this_thread::sleep_for(ms_t(300));

  pl::Utils utils;
  utils.set_read_buffer_size(rd_buffer_size);
  utils.disable_dbgout();
  const uint64_t timeout_ms = 1000;
  std::string out_buffer;

  const auto read_start_time = cpp_clock_t::now();
  pl::eReadResult rc = pl::eReadResult::RR_OKAY;
  while ((out_buffer.size() < data_size) && (rc == pl::eReadResult::RR_OKAY))
  {
    rc = utils.read_single_shot(pipefd[0], timeout_ms, out_buffer);
  }
  const auto read_end_time = cpp_clock_t::now();
  const auto read_time_ms = std::chrono::duration_cast<ms_t>(read_end_time - read_start_time).count();

  DBG_OUT << " ds=" << data_size
          << " bs=" << rd_buffer_size
          << " to=" << timeout_ms 
          << " rc=" << rc
          << " ms=" << read_time_ms
          << " DS=" << out_buffer.size()
          << ' ' << (out_buffer.size() < data_size ? " MISSING DATA" : " OK")
          << std::endl;

  // Cleanup before leaving
  close(pipefd[0]);
  // close(pipefd[1]);
  writer_thread.join();
}

static void f_test03()
{
  std::cout << __FUNCTION__ << " BEGIN " << std::endl;

  {
    const std::size_t data_size = 10;
    const std::size_t rd_buffer_size = 20;
    f_test_rdbuffer(data_size, rd_buffer_size);
  }

  {
    const std::size_t data_size = 50;
    const std::size_t rd_buffer_size = 20;
    f_test_rdbuffer(data_size, rd_buffer_size);
  }

  {
    const std::size_t data_size = 1024 * 1024;
    const std::size_t rd_buffer_size = 1 + data_size;
    f_test_rdbuffer(data_size, rd_buffer_size);
  }

  {
    const std::size_t data_size = 1024 * 1024;
    const std::size_t rd_buffer_size = 512;
    f_test_rdbuffer(data_size, rd_buffer_size);
  }

  {
    const std::size_t data_size = 1024 * 1024 * 1024;
    const std::size_t rd_buffer_size = 1 + data_size;
    f_test_rdbuffer(data_size, rd_buffer_size);
  }

  {
    const std::size_t data_size = 1024 * 1024 * 1024;
    const std::size_t rd_buffer_size = 1024 * 1024;
    f_test_rdbuffer(data_size, rd_buffer_size);
  }

  {
    const std::size_t data_size = 1024 * 1024 * 1024;
    const std::size_t rd_buffer_size = 4096;
    f_test_rdbuffer(data_size, rd_buffer_size);
  }

  {
    const std::size_t data_size = 1024 * 1024 * 1024;
    const std::size_t rd_buffer_size = 1024;
    f_test_rdbuffer(data_size, rd_buffer_size);
  }
}

static void f_test02()
{
  std::cout << __FUNCTION__ << " BEGIN " << std::endl;
  int pipefd[2];
  const int pipe_rc = pipe(pipefd);
  if (pipe_rc < 0)
  {
    perror("pipe() failure: ");
    return;
  }
  DBG_OUT << " pipefd: " << pipefd[0] << ", " << pipefd[1] << std::endl;

  const write_list_t write_list {
    { ms_t(1000), "" },
    { ms_t(100),  "01234" },
    { ms_t(100),  "56789" },
    { ms_t(1000), "" }
  };

  std::thread writer_thread(f_writer_thread, pipefd[1], write_list);

  pl::Utils utils;
  const std::uint64_t data_size = 10;
  const uint64_t timeout_ms = 4000;
  std::string out_buffer;

  DBG_OUT << "Reading data - single shot"
          << " to=" << timeout_ms << " ms" << std::endl;

  pl::eReadResult rc = utils.read_single_shot(pipefd[0], 
                                              timeout_ms, 
                                              out_buffer);
  std::cout << "reading_result = " << rc
            << " data=" << out_buffer
            << std::endl;

  // Cleanup before leaving
  writer_thread.join();
  close(pipefd[0]);
  close(pipefd[1]);

  DBG_OUT << __FUNCTION__ << " THE-END " << std::endl;
}

static void f_test01()
{
  std::cout << __FUNCTION__ << " BEGIN " << std::endl;
  int pipefd[2];
  const int pipe_rc = pipe(pipefd);
  if (pipe_rc < 0)
  {
    perror("pipe() failure: ");
    return;
  }
  DBG_OUT << " pipefd: " << pipefd[0] << ", " << pipefd[1] << std::endl;

  const write_list_t write_list {
    { ms_t(1000), "" },
    { ms_t(100),  "01234" },
    { ms_t(100),  "56789" },
    { ms_t(1000), "" }
  };

  std::thread writer_thread(f_writer_thread, pipefd[1], write_list);

  pl::Utils utils;
  const std::uint64_t data_size = 10;
  const uint64_t timeout_ms = 4000;
  std::string out_buffer;

  DBG_OUT << "Reading data - size=" << data_size 
          << " to=" << timeout_ms << " ms" << std::endl;

  pl::eReadResult rc = utils.read_by_size(pipefd[0], 
                                          data_size, 
                                          timeout_ms, 
                                          out_buffer);
  std::cout << "reading_result = " << rc
            << " data=" << out_buffer
            << std::endl;

  // Cleanup before leaving
  writer_thread.join();
  close(pipefd[0]);
  close(pipefd[1]);

  DBG_OUT << __FUNCTION__ << " THE-END " << std::endl;
}

int main()
{
  //f_test01();
  //f_test02();
  f_test03();

  return 0;
}

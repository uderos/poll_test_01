#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>

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

// Sends only 1/2 of the expected data
static void f_test01_writer_thread_03(const int fd)
{
  const char data[] = "0123456789";
  const std::size_t data_size = sizeof(data);

  DBG_OUT << __FUNCTION__ << " Writing thread sleeping ..." << std::endl;
  std::this_thread::sleep_for(ms_t(1000));

  const int write_data_size = 1;
  for (int i = 0; i < data_size / 2; i += write_data_size)
  { 
    std::this_thread::sleep_for(ms_t(100));
    DBG_OUT << "Sending data - size=" << write_data_size << " fd=" << fd << std::endl;
    const int rc = write(fd, data + i, write_data_size);  // one byte at the time
    if (rc < 0) perror("write() failure: ");
  }

  DBG_OUT << "Writing thread terminates" << std::endl;
}

// Sends all the data but slowly
static void f_test01_writer_thread_02(const int fd)
{
  const char data[] = "0123456789";
  const std::size_t data_size = sizeof(data);

  DBG_OUT << "Writing thread sleeping ..." << std::endl;
  std::this_thread::sleep_for(ms_t(1000));

  const int write_data_size = 2;
  for (int i = 0; i < data_size; i += write_data_size)
  { 
    std::this_thread::sleep_for(ms_t(100));
    DBG_OUT << "Sending data - size=" << write_data_size << " fd=" << fd << std::endl;
    const int rc = write(fd, data + i, write_data_size);  // one byte at the time
    if (rc < 0) perror("write() failure: ");
  }

  DBG_OUT << "Writing thread terminates" << std::endl;
}

// Sends all data after initial delay
static void f_test01_writer_thread_01(const int fd)
{
  const char data[] = "0123456789";
  const std::size_t data_size = sizeof(data);

  DBG_OUT << "Writing thread sleeping ..." << std::endl;
  std::this_thread::sleep_for(ms_t(1000));

  DBG_OUT << "Sending data - size=" << data_size << " fd=" << fd << std::endl;
  const int rc = write(fd, data, data_size);
  if (rc < 0) perror("write() failure: ");

  DBG_OUT << "THE END - rc=" << rc << std::endl;
}

static void f_test01_writer_thread(const int fd)
{
  f_test01_writer_thread_02(fd);
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

  std::thread writer_thread(f_test01_writer_thread, pipefd[1]);

  pl::Utils utils;
  const std::uint64_t data_size = 10;
  const uint64_t timeout_ms = 4000;
  std::string out_buffer;

  DBG_OUT << "Reading data - size=" << data_size << " to=" << timeout_ms << " ms" << std::endl;
  pl::eReadResult rc = utils.read_from_pipe(pipefd[0], 
                                            data_size, 
                                            timeout_ms, 
                                            out_buffer);
  std::cout << "reading_result = " << rc
            << " data=" << out_buffer
            << std::endl;

  writer_thread.join();
  DBG_OUT << __FUNCTION__ << " THE-END " << std::endl;
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

  {
    const int fd = pipefd[1];
    const char data[] = "0123456789";
    const std::size_t data_size = sizeof(data);
    DBG_OUT << "Sending data - size=" << data_size << " fd=" << fd << std::endl;
    const int rc = write(fd, data, data_size);
    if (rc < 0) perror("write() failure: ");
  }

  pl::Utils utils;
  const std::uint64_t data_size = 5;
  const uint64_t timeout_ms = 1000;
  std::string out_buffer;

  DBG_OUT << "Reading data - size=" << data_size << " to=" << timeout_ms << " ms" << std::endl;
  pl::eReadResult rc = utils.read_from_pipe(pipefd[0], 
                                            data_size, 
                                            timeout_ms, 
                                            out_buffer);
  std::cout << "reading_result = " << rc
            << " data=" << out_buffer
            << std::endl;
  DBG_OUT << __FUNCTION__ << " THE-END " << std::endl;
}

static void f_test03()
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

  const char data_out[] = "0123456789";
  const std::size_t data_size = sizeof(data_out);

  {
    const int fd = pipefd[1];
    DBG_OUT << "Sending data - size=" << data_size << " fd=" << fd << std::endl;
    const int rc = write(fd, data_out, data_size);
    DBG_OUT << "write() rc=" << rc << std::endl;
    if (rc < 0) perror("write() failure: ");
  }

  {
    const int fd = pipefd[0];
    char data_in[1 + data_size];
    memset(data_in, 0, sizeof(data_in));
    DBG_OUT << "Reading data - size=" << data_size << " fd=" << fd << std::endl;
    const int rc = read(fd, data_in, data_size);
    DBG_OUT << "read() rc=" << rc << std::endl;
    if (rc > 0) std::cout << "data_in='" << data_in << "'" << std::endl;
    else if (rc == 0) std::cout << "No data" << std::endl;
    else if (rc < 0) perror("read() failure: ");
  }
  DBG_OUT << __FUNCTION__ << " THE-END " << std::endl;
}

  


int main()
{
  f_test01();
  //f_test02();
  //f_test03();

  return 0;
}

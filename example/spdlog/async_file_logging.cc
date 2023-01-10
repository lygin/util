#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
int main()
{
  // default thread pool settings can be modified *before* creating the async logger:
  // spdlog::init_thread_pool(8192, 1); // queue with 8k items and 1 backing thread.
  auto logger = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", "../logs/async_log.txt");
  logger->info("hello");
  logger->debug("debug");
  logger->error("error");
  logger->warn("warn");
}
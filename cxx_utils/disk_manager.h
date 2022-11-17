#pragma once

#include <atomic>
#include <fstream>
#include <future> // NOLINT
#include <string>
#include "page.h"

/**
 * 负责Page的alloc和delete。负责page从磁盘的读写。
 * TODO: 并发控制
 */
class DiskManager
{
public:
    /**
     * 创建dm
     * @param db_file 存储Page的文件名
     */
    explicit DiskManager(const std::string &db_file);

    ~DiskManager() = default;

    void ShutDown();

    void WritePage(page_id_t page_id, const char *page_data);

    void ReadPage(page_id_t page_id, char *page_data);

    /**
     * Flush the entire log buffer into disk.
     * @param log_data raw log data
     * @param size size of log entry
     */
    void WriteLog(char *log_data, int size);

    /**
     * Read a log entry from the log file.
     * @param[out] log_data output buffer
     * @param size size of the log entry
     * @param offset offset of the log entry in the file
     * @return true if the read was successful, false otherwise
     */
    bool ReadLog(char *log_data, int size, int offset);

    /**
     * Allocate a page on disk.
     * @return the id of the allocated page
     */
    page_id_t AllocatePage();

    void DeallocatePage(page_id_t page_id);

    int GetNumFlushes() const;

    bool GetFlushState() const;

    int GetNumWrites() const;

    /**
     * Sets the future which is used to check for non-blocking flushes.
     * @param f the non-blocking flush check
     */
    inline void SetFlushLogFuture(std::future<void> *f) { flush_log_f_ = f; }

    /** Checks if the non-blocking flush future was set. */
    inline bool HasFlushLogFuture() { return flush_log_f_ != nullptr; }

private:
    int GetFileSize(const std::string &file_name);
    // stream to write log file
    std::fstream log_io_;
    std::string log_name_;
    // stream to write db file
    std::fstream db_io_;
    std::string file_name_;
    std::atomic<page_id_t> next_page_id_;
    int num_flushes_;
    int num_writes_;
    bool flush_log_;
    std::future<void> *flush_log_f_;
};

#pragma once
extern "C" {
#include <sys/stat.h>
}
#include <atomic>
#include <fstream>
#include <future>
#include <string>
#include "page.h"

/**
 * 负责以page粒度读写磁盘。
 */
class DiskManager
{
public:
    /**
     * Creates a new disk manager that writes to the specified database file.
     * @param db_file the file name of the database file to write to
     */
    explicit DiskManager(const std::string &db_file);
    ~DiskManager() = default;
    void ShutDown() {
        db_io_.close();
        log_io_.close();
    }
    void WritePage(uint32_t page_id, const char *page_data);
    void ReadPage(uint32_t page_id, char *page_data);

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
    uint32_t AllocatePage() { return next_page_id_++; }

    void DeallocatePage(uint32_t page_id) {}

    int GetNumFlushes() const { return num_flushes_; }

    bool GetFlushState() const { return flush_log_; }

    int GetNumWrites() { return num_writes_; }

    /**
     * Sets the future which is used to check for non-blocking flushes.
     * @param f the non-blocking flush check
     */
    inline void SetFlushLogFuture(std::future<void> *f) { flush_log_f_ = f; }

    /** Checks if the non-blocking flush future was set. */
    inline bool HasFlushLogFuture() { return flush_log_f_ != nullptr; }

private:
    int GetFileSize(const std::string &file_name) {
        struct stat stat_buf;
        int rc = stat(file_name.c_str(), &stat_buf);
        return rc == 0 ? static_cast<int>(stat_buf.st_size) : -1;
    }
    std::fstream log_io_;
    std::string log_name_;
    std::fstream db_io_;
    std::string file_name_;
    std::atomic<uint32_t> next_page_id_;
    int num_flushes_;
    int num_writes_;
    bool flush_log_;
    std::future<void> *flush_log_f_;
};

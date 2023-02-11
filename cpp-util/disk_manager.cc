#include <sys/stat.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <thread> // NOLINT

#include "disk_manager.h"

static char *buffer_used;

DiskManager::DiskManager(const std::string &db_file)
    : file_name_(db_file), next_page_id_(0), num_flushes_(0), num_writes_(0), flush_log_(false), flush_log_f_(nullptr)
{
    std::string::size_type n = file_name_.rfind('.');
    if (n == std::string::npos)
    {
        return;
    }
    log_name_ = file_name_.substr(0, n) + ".log";
    log_io_.open(log_name_, std::ios::binary | std::ios::in | std::ios::app | std::ios::out);
    if (!log_io_.is_open())
    {
        log_io_.clear();
        // create a new file
        log_io_.open(log_name_, std::ios::binary | std::ios::trunc | std::ios::app | std::ios::out);
        log_io_.close();
        log_io_.open(log_name_, std::ios::binary | std::ios::in | std::ios::app | std::ios::out);
        if (!log_io_.is_open())
        {
            throw "can't open dblog file";
        }
    }
    db_io_.open(db_file, std::ios::binary | std::ios::in | std::ios::out);
    if (!db_io_.is_open())
    {
        db_io_.clear();
        // create a new file
        db_io_.open(db_file, std::ios::binary | std::ios::trunc | std::ios::out);
        db_io_.close();
        db_io_.open(db_file, std::ios::binary | std::ios::in | std::ios::out);
        if (!db_io_.is_open())
        {
            throw "can't open db file";
        }
    }
    buffer_used = nullptr;
}


void DiskManager::WritePage(uint32_t page_id, const char *page_data)
{
    size_t offset = (size_t)page_id * PAGE_SIZE;
    num_writes_ += 1;
    db_io_.seekp(offset);
    db_io_.write(page_data, PAGE_SIZE);
    if (db_io_.bad())
    {
        std::cerr << "WritePage failed" << std::endl;
        return;
    }
    db_io_.flush();
}

void DiskManager::Append(const char *buf, size_t size)
{
    num_writes_ += 1;
    db_io_.seekp(GetFileSize(file_name_));
    db_io_.write(buf, size);
    if (db_io_.bad())
    {
        std::cerr << "Append fail" << std::endl;
        return;
    }
    db_io_.flush();
}

void DiskManager::ReadPage(uint32_t page_id, char *page_data)
{
    int offset = page_id * PAGE_SIZE;
    int file_size = GetFileSize(file_name_);
    if (offset > file_size)
    {
        printf("load File failed, read offset %d while filesize %d B\n", offset, file_size);
    }
    else
    {
        db_io_.seekp(offset);
        db_io_.read(page_data, PAGE_SIZE);
        if (db_io_.bad())
        {
            return;
        }
        // if file ends before reading PAGE_SIZE
        int read_count = db_io_.gcount();
        if (read_count < PAGE_SIZE)
        {
            db_io_.clear();
            std::cerr << "Read less than a page" << std::endl;
            memset(page_data + read_count, 0, PAGE_SIZE - read_count);
        }
    }
}

void DiskManager::Read(char *buf, int offset, size_t size)
{
    int file_size = GetFileSize(file_name_);
    if (offset > file_size)
    {
        printf("load File failed, read offset %d while filesize %d B\n", offset, file_size);
    }
    else
    {
        db_io_.seekp(offset);
        db_io_.read(buf, size);
        if (db_io_.bad())
        {
            return;
        }
    }
}

void DiskManager::WriteLog(char *log_data, int size)
{
    // enforce swap log buffer
    assert(log_data != buffer_used);
    buffer_used = log_data;

    if (size == 0)
    {
        return;
    }

    flush_log_ = true;

    if (flush_log_f_ != nullptr)
    {
        // used for checking non-blocking flushing
        assert(flush_log_f_->wait_for(std::chrono::seconds(10)) == std::future_status::ready);
    }

    num_flushes_ += 1;
    log_io_.write(log_data, size);

    if (log_io_.bad())
    {
        return;
    }
    // needs to flush to keep disk file in sync
    log_io_.flush();
    flush_log_ = false;
}

/**
 * @return: false means already reach the end
 */
bool DiskManager::ReadLog(char *log_data, int size, int offset)
{
    if (offset >= GetFileSize(log_name_))
    {
        return false;
    }
    log_io_.seekp(offset);
    log_io_.read(log_data, size);

    if (log_io_.bad())
    {
        return false;
    }
    // if log file ends before reading "size"
    int read_count = log_io_.gcount();
    if (read_count < size)
    {
        log_io_.clear();
        memset(log_data + read_count, 0, size - read_count);
    }

    return true;
}

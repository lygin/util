#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "slice.h"
#include "status.h"
#include <mutex>
#include <vector>
#include <cstdio>
#include <string>

constexpr const size_t kWritableFileBufferSize = 65536; // 64KB

// A file abstraction for randomly reading the contents of a file.
class RandomAccessFile{
  public:
  RandomAccessFile() = default;
  RandomAccessFile(const RandomAccessFile&) = delete;
  RandomAccessFile& operator=(const RandomAccessFile&) = delete;

  virtual ~RandomAccessFile();
  // Read up to "n" bytes from the file starting at "offset".
  // "scratch[0..n-1]" may be written by this routine.
  // Sets "*result" to the data that was read
  virtual Status Read(uint64_t offset, size_t n, Slice* result,
                      char* scratch) const = 0;
};

// A file abstraction for sequential writing.
class WritableFile {
  WritableFile() = default;

  WritableFile(const WritableFile&) = delete;
  WritableFile& operator=(const WritableFile&) = delete;

  virtual ~WritableFile();

  virtual Status Append(const Slice& data) = 0;
  virtual Status Close() = 0;
  virtual Status Flush() = 0;
  virtual Status Sync() = 0;
};

class MemFileState {
  public:
  MemFileState():refs_(0), size_(0) {}
  MemFileState(const MemFileState&) = delete;
  MemFileState& operator=(const MemFileState&) = delete;

  void Ref() {
    std::lock_guard<std::mutex> lock(refs_mutex_);
    ++refs_;
  }

  void Unref() {
    bool do_delete = false;
    {
      std::lock_guard<std::mutex> lock(refs_mutex_);
      --refs_;
      assert(refs_ >= 0);
      if (refs_ <= 0) {
        do_delete = true;
      }
    }

    if (do_delete) {
      delete this;
    }
  }

  uint64_t Size() const {
    std::lock_guard<std::mutex> lock(blocks_mutex_);
    return size_;
  }

  void Truncate() {
    std::lock_guard<std::mutex> lock(blocks_mutex_);
    //noncopy
    for (char* & block : blocks_) {
      delete[] block;
    }
    blocks_.clear();
    size_ = 0;
  }

  Status Append(const Slice& data) {
    const char* src = data.data();
    size_t src_len = data.size();

    std::lock_guard<std::mutex> lock(blocks_mutex_);
    while (src_len > 0) {
      size_t avail;
      size_t offset = size_ % kBlockSize;

      if (offset != 0) {
        // There is some room in the last block.
        avail = kBlockSize - offset;
      } else {
        // No room in the last block; push new one.
        blocks_.push_back(new char[kBlockSize]);
        avail = kBlockSize;
      }

      if (avail > src_len) {
        avail = src_len;
      }
      std::memcpy(blocks_.back() + offset, src, avail);
      src_len -= avail;
      src += avail;
      size_ += avail;
    }

    return Status::OK();
  }

  Status Read(uint64_t offset, size_t n, Slice* result, char* scratch) const {
    std::lock_guard<std::mutex> lock(blocks_mutex_);
    if (offset > size_) {
      return Status::IOError("Offset greater than file size.");
    }
    const uint64_t available = size_ - offset;
    if (n > available) {
      n = static_cast<size_t>(available);
    }
    if (n == 0) {
      *result = Slice();
      return Status::OK();
    }

    assert(offset / kBlockSize <= std::numeric_limits<size_t>::max());
    size_t block = static_cast<size_t>(offset / kBlockSize);
    size_t block_offset = offset % kBlockSize;
    size_t bytes_to_copy = n;
    char* dst = scratch;

    while (bytes_to_copy > 0) {
      size_t avail = kBlockSize - block_offset;
      if (avail > bytes_to_copy) {
        avail = bytes_to_copy;
      }
      std::memcpy(dst, blocks_[block] + block_offset, avail);

      bytes_to_copy -= avail;
      dst += avail;
      block++;
      block_offset = 0;
    }

    *result = Slice(scratch, n);
    return Status::OK();
  }

  private:
  enum { kBlockSize = 8 * 1024 };
  // Private since only Unref() should be used to delete it.
  ~MemFileState() { Truncate(); }
  std::mutex refs_mutex_;
  int refs_;
  mutable std::mutex blocks_mutex_;
  std::vector<char*> blocks_;
  uint64_t size_; //file size, guarded by blocks_mutex_
};

class RandomAccessFileImpl : public RandomAccessFile {
 public:
  explicit RandomAccessFileImpl(MemFileState* file) : file_(file) { file_->Ref(); }

  ~RandomAccessFileImpl() override { file_->Unref(); }

  Status Read(uint64_t offset, size_t n, Slice* result,
              char* scratch) const override {
    return file_->Read(offset, n, result, scratch);
  }

 private:
  MemFileState* file_;
};

class WritableFileImpl : public WritableFile {
 public:
  WritableFileImpl(MemFileState* file) : file_(file) { file_->Ref(); }

  ~WritableFileImpl() override { file_->Unref(); }

  Status Append(const Slice& data) override { return file_->Append(data); }

  Status Close() override { return Status::OK(); }
  Status Flush() override { return Status::OK(); }
  Status Sync() override { return Status::OK(); }

 private:
  MemFileState* file_;
};

// Implements random read access in a file using pread().
// Instances of this class are thread-safe
// Instances are immutable and Read() only calls thread-safe library
class PosixRandomAccessFile final : public RandomAccessFile {
  public:
  PosixRandomAccessFile(std::string filename) : fd_(-1),
      filename_(std::move(filename)) {
    fd_ = ::open(filename_.c_str(), O_RDONLY);
    if (fd_ < 0) {
      return Status::IOError(filename_, strerror(errno));
    }
    assert(fd != -1);
  }

  ~PosixRandomAccessFile() override {
    assert(fd_ != -1);
    ::close(fd_);
  }

  Status Read(uint64_t offset, size_t n, Slice* result,
              char* scratch) const override {
    assert(fd != -1);

    Status status;
    ssize_t read_size = ::pread(fd, scratch, n, static_cast<off_t>(offset));
    *result = Slice(scratch, (read_size < 0) ? 0 : read_size);
    if (read_size < 0) {
      // An error: return a non-ok status.
      status = Status::IsIOError(filename_, strerror(errno));
    }
    return status;
  }
  private:
  int fd_; 
  const std::string filename_;
};

// Implements random read access in a file using mmap().
// Instances of this class are thread-safe
// Instances are immutable and Read() only calls thread-safe library
class PosixMmapReadableFile final : public RandomAccessFile{
  public:
  PosixMmapReadableFile(std::string filename, char* mmap_base, size_t length)
    : mmap_base_(mmap_base),
      length_(length),
      filename_(std::move(filename)) {}

  ~PosixMmapReadableFile() override {
    ::munmap(static_cast<void*>(mmap_base_), length_);
  }

  Status Read(uint64_t offset, size_t n, Slice* result,
              char* scratch) const override {
    //off, off+1, off+n-1, off+ n <= length_ is fine
    if (offset + n > length_) {
      *result = Slice();
      return Status::IOError(filename_, strerror(EINVAL));
    }

    *result = Slice(mmap_base_ + offset, n);
    return Status::OK();
  }
  private:
  // mmap_base[0, length-1] points to the memory-mapped contents of the file.
  char* const mmap_base_;
  const size_t length_;
  const std::string filename_;
}

class PosixWritableFile final : public WritableFile {
 public:
  PosixWritableFile(std::string filename, int fd)
      : pos_(0),
        fd_(fd),
        is_manifest_(IsManifest(filename)),
        filename_(std::move(filename)),
        dirname_(Dirname(filename_)) {}

  ~PosixWritableFile() override {
    if (fd_ >= 0) {
      Close();
    }
  }

  Status Append(const Slice& data) override {
    size_t write_size = data.size();
    const char* write_data = data.data();
    // Fit as much as possible into buffer.
    size_t copy_size = std::min(write_size, kWritableFileBufferSize - pos_);
    std::memcpy(buf_ + pos_, write_data, copy_size);
    write_data += copy_size;
    write_size -= copy_size;
    pos_ += copy_size;
    if (write_size == 0) {
      return Status::OK();
    }
    // Can't fit in buffer, so need to do at least one write.
    Status status = FlushBuffer();
    if (!status.ok()) {
      return status;
    }
    // Small writes go to buffer
    if (write_size < kWritableFileBufferSize) {
      std::memcpy(buf_, write_data, write_size);
      pos_ = write_size;
      return Status::OK();
    }
    // large writes are written directly.
    return WriteUnbuffered(write_data, write_size);
  }

  Status Close() override {
    Status status = FlushBuffer();
    const int close_result = ::close(fd_);
    if (close_result < 0 && status.ok()) {
      status = Status::IOError(filename_, strerror(errno));
    }
    fd_ = -1;
    return status;
  }

  // OS may crash and lose data
  Status Flush() override { return FlushBuffer(); }
  // Ensure new files referred to by the manifest are in the filesystem.
  Status Sync() override {
    Status status = SyncDirIfManifest();
    if (!status.ok()) {
      return status;
    }

    status = FlushBuffer();
    if (!status.ok()) {
      return status;
    }

    return SyncFd(fd_, filename_);
  }

 private:
  Status FlushBuffer() {
    Status status = WriteUnbuffered(buf_, pos_);
    pos_ = 0;
    return status;
  }

  Status WriteUnbuffered(const char* data, size_t size) {
    while (size > 0) {
      ssize_t write_result = ::write(fd_, data, size);
      if (write_result < 0) {
        if (errno == EINTR) {
          continue;  // Retry
        }
        return Status::IOError(filename_, strerror(errno));
      }
      data += write_result;
      size -= write_result;
    }
    return Status::OK();
  }

  Status SyncDirIfManifest() {
    Status status;
    if (!is_manifest_) {
      return status;
    }

    int fd = ::open(dirname_.c_str(), O_RDONLY);
    if (fd < 0) {
      status = Status::IOError(dirname_, strerror(errno));
    } else {
      status = SyncFd(fd, dirname_);
      ::close(fd);
    }
    return status;
  }

  // Ensures that all the caches associated with the given file descriptor's
  // data are flushed all the way to durable media, and can withstand power
  // failures.
  static Status SyncFd(int fd, const std::string& fd_path) {

    bool sync_success = ::fsync(fd) == 0;

    if (sync_success) {
      return Status::OK();
    }
    return Status::IOError(fd_path, strerror(errno));
  }

  // Returns the directory name in a path pointing to a file.
  // e.g. dirname/foo/bar.txt will return /foo/bar.
  static std::string Dirname(const std::string& filename) {
    std::string::size_type separator_pos = filename.rfind('/');
    if (separator_pos == std::string::npos) {
      return std::string(".");
    }
    // The filename component should not contain a path separator. If it does,
    // the splitting was done incorrectly.
    assert(filename.find('/', separator_pos + 1) == std::string::npos);

    return filename.substr(0, separator_pos);
  }

  // Extracts the file name from a path pointing to a file.
  // e.g. dirname/foo/bar.txt will return bar.txt
  static Slice Basename(const std::string& filename) {
    std::string::size_type separator_pos = filename.rfind('/');
    if (separator_pos == std::string::npos) {
      return Slice(filename);
    }
    assert(filename.find('/', separator_pos + 1) == std::string::npos);

    return Slice(filename.data() + separator_pos + 1,
                 filename.length() - separator_pos - 1);
  }

  // True if the given file is a manifest file.
  static bool IsManifest(const std::string& filename) {
    return Basename(filename).starts_with("MANIFEST");
  }

  // buf_[0, pos_ - 1] contains data to be written to fd_.
  char buf_[kWritableFileBufferSize];
  size_t pos_;
  int fd_;

  const bool is_manifest_;  // True if the file's name starts with MANIFEST.
  const std::string filename_;
  const std::string dirname_;  // The directory of filename_.
};







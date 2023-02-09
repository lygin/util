#pragma once
#include <string>
#include "slice.h"
class Status {
  public:
  // Create a success status.
  Status() noexcept : state_(nullptr) {}
  ~Status() { delete[] state_; }

  Status(const Status& rhs);
  Status& operator=(const Status& rhs);

  Status(Status&& rhs) noexcept : state_(rhs.state_) { rhs.state_ = nullptr; }
  Status& operator=(Status&& rhs) noexcept;

  // Return a success status.
  static Status OK() { return Status(); }

  // Return error status of an appropriate type.
  static Status NotFound(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kNotFound, msg, msg2);
  }
  static Status Corruption(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kCorruption, msg, msg2);
  }
  static Status NotSupported(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kNotSupported, msg, msg2);
  }
  static Status InvalidArgument(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kInvalidArgument, msg, msg2);
  }
  static Status IOError(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kIOError, msg, msg2);
  }

  // Returns true iff the status indicates success.
  bool ok() const { return (state_ == nullptr); }

  // Returns true iff the status indicates a NotFound error.
  bool IsNotFound() const { return code() == kNotFound; }

  // Returns true iff the status indicates a Corruption error.
  bool IsCorruption() const { return code() == kCorruption; }

  // Returns true iff the status indicates an IOError.
  bool IsIOError() const { return code() == kIOError; }

  // Returns true iff the status indicates a NotSupportedError.
  bool IsNotSupportedError() const { return code() == kNotSupported; }

  // Returns true iff the status indicates an InvalidArgument.
  bool IsInvalidArgument() const { return code() == kInvalidArgument; }

  // Return a string representation of this status suitable for printing.
  // Returns the string "OK" for success.
  std::string ToString() const {
    if (state_ == nullptr) {
      return "OK";
    } else {
      char tmp[30];
      const char* type;
      switch (code()) {
        case kOk:
          type = "OK";
          break;
        case kNotFound:
          type = "NotFound: ";
          break;
        case kCorruption:
          type = "Corruption: ";
          break;
        case kNotSupported:
          type = "Not implemented: ";
          break;
        case kInvalidArgument:
          type = "Invalid argument: ";
          break;
        case kIOError:
          type = "IO error: ";
          break;
        default:
          std::snprintf(tmp, sizeof(tmp),
                        "Unknown code(%d): ", static_cast<int>(code()));
          type = tmp;
          break;
      }
      std::string result(type);
      uint32_t length;
      std::memcpy(&length, state_, sizeof(length));
      result.append(state_ + 5, length);
      return result;
    }
  }

private:
  enum Code {
    kOk = 0,
    kNotFound = 1,
    kCorruption = 2,
    kNotSupported = 3,
    kInvalidArgument = 4,
    kIOError = 5
  };
  Code code() const {
    return (state_ == nullptr) ? kOk : static_cast<Code>(state_[4]);
  }
  Status(Code code, const Slice& msg, const Slice& msg2) {
    assert(code != kOk);
    const uint32_t len1 = static_cast<uint32_t>(msg.size());
    const uint32_t len2 = static_cast<uint32_t>(msg2.size());
    const uint32_t size = len1 + (len2 ? (2 + len2) : 0);
    char* result = new char[size + 5];
    std::memcpy(result, &size, sizeof(size));
    result[4] = static_cast<char>(code);
    std::memcpy(result + 5, msg.data(), len1);
    if (len2) {
      result[5 + len1] = ':';
      result[6 + len1] = ' ';
      std::memcpy(result + 7 + len1, msg2.data(), len2);
    }
    state_ = result;
  }
  static const char* CopyState(const char* state) {
    uint32_t size;
    std::memcpy(&size, state, sizeof(size));
    char* result = new char[size + 5];
    std::memcpy(result, state, size + 5);
    return result;
  }
  const char* state_;
};
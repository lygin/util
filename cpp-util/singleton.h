#pragma once

template <typename T>
class Singleton {
 public:
  Singleton() {}

  static T* GetInstance() {
    static T instance;
    return &instance;
  }

 private:
  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;
};
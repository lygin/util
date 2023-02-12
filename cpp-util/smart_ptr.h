/**
 * 模仿shared_ptr实现一个智能指针
*/
#ifndef _SMART_PTR_H_
#define _SMART_PTR_H_

#include <functional>
#include "logging.h"

template <typename T>
class smart_ptr {
public:
  smart_ptr(): m_pobject(nullptr), m_p_use_count(new unsigned(1)) {}
  explicit smart_ptr(T *) : m_pobject(p), m_p_use_count(new unsigned(1)) {}
  smart_ptr(T *, std::function<void(T *)>) : m_pobject(p), 
      m_p_use_count(new unsigned(1)), m_del(del) {}

  smart_ptr(const smart_ptr &rhs) : m_pobject(rhs.m_pobject), 
      m_p_use_count(rhs.m_p_use_count), m_del(rhs.m_del) {
    (*m_p_use_count)++;
    LOG_INFO("copy construct %u", *m_p_use_count);
  }
  smart_ptr &operator=(const smart_ptr &rhs) {
    m_del = rhs.m_del;
    ++(*rhs.m_p_use_count);
    // 递减本对象的引用计数
    if (--(*m_p_use_count) == 0) {
      m_del(m_pobject);
      delete m_p_use_count;
    }

    m_p_use_count = rhs.m_p_use_count;
    m_pobject = rhs.m_pobject;
    LOG_INFO("copy assignment %u", *m_p_use_count);
    return *this;
  }

  T &operator*() const { return *m_pobject; }
  T *operator->() const {
    return &this->operator*();
  }

  ~smart_ptr() {
    LOG_INFO("deconstruct %u", *m_p_use_count-1);
    if (--(*m_p_use_count) == 0) {
      m_del(m_pobject);
      m_pobject = nullptr;

      delete m_p_use_count;
      m_p_use_count = nullptr;
    }
  }
  // 向bool的类型转换
  explicit operator bool() const { return m_pobject != nullptr; }

  bool unique() { return *m_p_use_count == 1; }

  void reset() {
    (*m_p_use_count)--;

    if (*m_p_use_count == 0) {
      m_del(m_pobject);
    }

    m_pobject = nullptr;
    *m_p_use_count = 1;
    m_del = default_del;
  }
  
  void reset(T *) {
    (*m_p_use_count)--;

    if (*m_p_use_count == 0) {
      m_del(m_pobject);
    }

    m_pobject = p;
    *m_p_use_count = 1;
    m_del = default_del;
  }

  void reset(T *, std::function<void(T *)>) {
    reset(p);
    m_del = del;
  }

  T *release() {
    (*m_p_use_count)--;

    if (*m_p_use_count == 0) {
      *m_p_use_count = 1;
    }

    auto p = m_pobject;
    m_pobject = nullptr;

    return p;
  }

  T *get() const { return m_pobject; }

private:
  static std::function<void(T *)> default_del;

private:
  unsigned *m_p_use_count = nullptr;
  T *m_pobject = nullptr;
  std::function<void(T *)> m_del = default_del;
};

template <typename T>
std::function<void(T *)> smart_ptr<T>::default_del = [](T *p) {
  delete p;
  p = nullptr;
};

template <typename T, typename... Args>
smart_ptr<T> make_smart(Args &&... args) {
  smart_ptr<T> sp(new T(std::forward<Args>(args)...));
  return sp;
}

#endif
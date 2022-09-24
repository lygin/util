#ifndef _SMART_PTR_H_
#define _SMART_PTR_H_
#include <functional>

// 模仿shared_ptr实现一个智能指针
template <typename T> 
class smart_ptr {
public:
  smart_ptr();
  explicit smart_ptr(T *);
  //带deleter的构造
  smart_ptr(T *, std::function<void(T *)>);
  //拷贝构造
  smart_ptr(const smart_ptr &);
  //拷贝赋值
  smart_ptr &operator=(const smart_ptr &);
  T &operator*() const;
  T *operator->() const;

  ~smart_ptr();
  // 向bool的类型转换
  explicit operator bool() const;

  bool unique();
  void reset();
  void reset(T *);
  void reset(T *, std::function<void(T *)>);
  T *release();

  T *get() const;

private:
  // 默认的deleter
  static std::function<void(T *)> default_del;

private:
  unsigned *m_p_use_count = nullptr;
  T *m_pobject = nullptr;
  std::function<void(T *)> m_del = default_del;
};

template <typename T, typename... Args>
smart_ptr<T> make_smart(Args&&... args);


#endif
#pragma once

#include "thread.h"

#include <ucontext.h>
#include <memory>

namespace sylar {

class Fiber : public std::enable_shared_from_this<Fiber>{
public:
  using ptr = std::shared_ptr<Fiber>;

  enum State {
    INIT,
    HOLD,
    EXEC,
    TERM,
    EXCEPT,
    READY
  };
private:
  Fiber();

public:
  Fiber(std::function<void()> cb, size_t stacksize = 0);
  ~Fiber();
  //重置协程状态
  //INIT, TERM
  void reset(std::function<void()> cb);
  //切换到当前协程执行
  void swapIn();
  //把当前协程切换到后台
  void swapOut();

  uint64_t getId() const {return m_id;}
public:
  //设置当前程
  static void SetThis(Fiber* f);
  //获取当前程
  static Fiber::ptr GetThis();
  //协程切换到后台，并设置为Ready状态
  static void YieldToReady();
  //协程切换到后台，并设置为Hold状态
  static void YieldToHold();
  //总协程数
  static uint64_t TotalFibers();

  static void MainFunc();
  static uint64_t GetFiberId();
private:
  uint64_t m_id = 0;
  uint32_t m_stacksize = 0;
  State m_state = INIT;

  ucontext_t m_ctx;
  void* m_stack = nullptr;

  std::function<void()> m_cb;
};

}
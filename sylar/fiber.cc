#include "fiber.h"
#include "config.h"
#include "macro.h"

#include <atomic>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size = 
  Config::Lookup<uint32_t> ("fiber.stack_size", 1024 * 1024, "fiber stack size");

class MallocStackAllocator {
public:
  static void* Alloc(size_t size) {
    return malloc(size);
  }

  static void Dealloc(void* vp, size_t size) {
    free(vp);
  }
};

typedef MallocStackAllocator StackAllocator;
uint64_t Fiber::GetFiberId() {
  if(t_fiber) {
    return t_fiber->getId();
  }
  return 0;
}

//only for main fiber
Fiber::Fiber(){
  m_state = EXEC;
  SetThis(this);

  if(getcontext(&m_ctx)) {
    SYLAR_ASSERT2(false, "getcontext");
  }

  ++s_fiber_count;
  SYLAR_LOG_DEBUG(g_logger) << " Fiber::Fiber, id=" << m_id;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize) 
  : m_id(++s_fiber_id)
  , m_cb(cb) {
  m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

  m_stack = StackAllocator::Alloc(m_stacksize);

  if(getcontext(&m_ctx)) {
    SYLAR_ASSERT2(false, "getcontext");
  }
  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  makecontext(&m_ctx, &Fiber::MainFunc, 0);
  SYLAR_LOG_DEBUG(g_logger) << " Fiber::Fiber, id=" << m_id;
}

Fiber::~Fiber() {
  --s_fiber_count;
  if(m_stack){
    //sub_fiber
    SYLAR_ASSERT(m_state == INIT 
        || m_state == TERM
        || m_state == EXCEPT);

    StackAllocator::Dealloc(m_stack, m_stacksize);
  } else {
    //main fiber
    SYLAR_ASSERT(!m_cb);
    SYLAR_ASSERT(m_state == EXEC);

    Fiber* cur = t_fiber;
    if(cur == this) {
      SetThis(nullptr);
    }
  }
  SYLAR_LOG_DEBUG(g_logger) << " Fiber::~Fiber, id=" << m_id;
}
//重置协程状态
//INIT, TERM，利用结束但未释放的内存，重新建立一个协程
//可以看作再次分配任务?
void Fiber::reset(std::function<void()> cb) {
  SYLAR_ASSERT(m_stack);
  SYLAR_ASSERT(m_state == INIT || m_state == TERM);

  m_cb = cb;
  if(getcontext(&m_ctx)) {
    SYLAR_ASSERT2(false, "getcontext");
  }
  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  makecontext(&m_ctx, &Fiber::MainFunc, 0);
  m_state = INIT;
}
//切换到当前协程执行
void Fiber::swapIn() {
  //把当前协程加到
  SetThis(this);
  SYLAR_ASSERT(m_state != EXEC);

  m_state = EXEC;
  //from main_fiber to cur_fiber
  if(swapcontext(&(t_threadFiber->m_ctx), &m_ctx)) {
    SYLAR_ASSERT2(false, "swapcontext");
  }

}
//把当前协程切换到后台
void Fiber::swapOut() {
  SetThis(t_threadFiber.get());

  //from cur_fiber to main_fiber 
  if(swapcontext(&m_ctx, &(t_threadFiber->m_ctx))) {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

void Fiber::SetThis(Fiber* f) {
  t_fiber = f;
}

Fiber::ptr Fiber::GetThis() {
  if(t_fiber) {
    return t_fiber->shared_from_this();
  }
  Fiber::ptr main_fiber(new Fiber);
  SYLAR_ASSERT(t_fiber == main_fiber.get());
  t_threadFiber = main_fiber;
  return t_fiber->shared_from_this();
}
//协程切换到后台，并设置为Ready状态
void Fiber::YieldToReady() {
  Fiber::ptr cur = GetThis();
  cur->m_state = READY;
  cur->swapOut();
}
//协程切换到后台，并设置为Hold状态
void Fiber::YieldToHold() {
  Fiber::ptr cur = GetThis();
  cur->m_state = HOLD;
  cur->swapOut();
}
//总协程数
uint64_t Fiber::TotalFibers() {
  return s_fiber_count;
}

void Fiber::MainFunc() {
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur);
  try {
    cur->m_cb();
    cur->m_cb = nullptr;
    cur->m_state = TERM;
  } catch (std::exception& e) {
    cur->m_state = EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Exception: " << e.what();
  } catch (...) {
    cur->m_state = EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Exception";
  }

  auto raw_ptr = cur.get();
  cur.reset();
  raw_ptr->swapOut();

  SYLAR_ASSERT2(false, "never reach");
}

}
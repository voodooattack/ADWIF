//  scheduler.hpp
//
//  Copyright (C) 2013 Abdullah Ali
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  This file is part of the boost concurrent graph library (work in progress)

// note: async result implementation is unfinished

#ifndef BOOST_CGL_SCHEDULER_HPP
#define BOOST_CGL_SCHEDULER_HPP

#include <memory>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/asio.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/coroutine/all.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/assert.hpp>

namespace boost {
  namespace cgl {
    class scheduler;
    namespace detail
    {
      template<typename T>
      static void null_deleter(T *) { }

      struct task_base: noncopyable
      {
        friend class boost::cgl::scheduler;
        friend class boost::thread_specific_ptr<task_base>;
      protected:
        virtual ~task_base() { }
      protected:
        virtual void create() = 0;
        virtual void * enter() = 0;
        virtual bool finished() const = 0;
        virtual bool created() const = 0;
        virtual void yield(void * v) = 0;
        virtual void yield(const void * v) = 0;
      };

      template<typename Handler, class E>
      struct task: public task_base
      {
        friend class boost::cgl::scheduler;
      private:
        typedef coroutines::coroutine<E(void)> coro_t;
        typedef E type;
        cgl::scheduler * scheduler;
        Handler handler;
        optional<E> result;
        std::auto_ptr<coro_t> coro;
        typename coro_t::caller_type * ca;
        task(Handler handler, cgl::scheduler * scheduler);
        virtual bool finished() const { return !created() || !*coro; }
        virtual bool created() const { return coro.get(); }
        virtual void create();
        virtual void * enter();
        virtual void yield(void *);
        virtual void yield(const void *);
        E coroutine(typename coro_t::caller_type & ca);
      };

      template<typename Handler>
      struct task<Handler, void>: public task_base
      {
        friend class boost::cgl::scheduler;
      private:
        typedef coroutines::coroutine<void(void)> coro_t;
        typedef void type;
        Handler handler;
        cgl::scheduler * scheduler;
        std::auto_ptr<coro_t> coro;
        typename coro_t::caller_type * ca;
        task(Handler handler, cgl::scheduler * scheduler);
        virtual bool finished() const { return !created() || !*coro; }
        virtual bool created() const { return coro.get(); }
        virtual void create();
        virtual void * enter();
        virtual void yield(void *);
        virtual void yield(const void *);
        void coroutine(typename coro_t::caller_type & ca);
      };
    }

    class scheduler
    : private noncopyable
    {
    public:
      typedef asio::deadline_timer timer_type;
      typedef timer_type::duration_type duration;

      template<typename T, typename E>
      friend struct detail::task;

      scheduler(): service(), thread_group_(),
      current(), tasks(256)
      {
        service.reset(new asio::io_service(thread::hardware_concurrency()));
      }

      scheduler(asio::io_service * service_) : service(), thread_group_(),
      current(), tasks(256)
      {
        service.reset(service_, &detail::null_deleter<asio::io_service>);
      }

      template<typename CompletionHandler>
      void schedule(CompletionHandler task)
      {
        tasks.push(new detail::task<CompletionHandler, typename result_of<CompletionHandler(void)>::type>(task, this));
      }

      template<typename CompletionHandler>
      void schedule(const duration & time, CompletionHandler task)
      {
        shared_ptr<timer_type> timer(new timer_type(*service));
        timer->expires_from_now(time);
        timer->async_wait(bind(bind(&scheduler::schedule<CompletionHandler>, this, task), timer));
      }

      void add_thread()
      {
        thread_group_->create_thread(bind(&scheduler::dispatcher, this));
      }

      void run(unsigned int nthreads = 0)
      {
        if (service->stopped())
          service->reset();
        thread_group_.reset(new boost::thread_group);
        work.reset(new asio::io_service::work(*service));
        unsigned int threads = nthreads ? nthreads : thread::hardware_concurrency();
        while (threads--)
          add_thread();
      }

      void yield()
      {
        BOOST_ASSERT_MSG(current.get(), "call to yield outside of a scheduler task");
        current->yield((const void *)0);
      }

      template<typename T>
      void yield(T& t)
      {
        BOOST_ASSERT_MSG(current.get(), "call to yield outside of a scheduler task");
        current->yield(&t);
      }

      template<typename T>
      void yield(const T& t)
      {
        BOOST_ASSERT_MSG(current.get(), "call to yield outside of a scheduler task");
        current->yield(&t);
      }

      void stop()
      {
        work.reset();
      }

      void abort()
      {
        work.reset();
        service->stop();
      }

      void join()
      {
        thread_group_->join_all();
      }

      ~scheduler() {
        service->stop();
        flush();
        join();
      }

      asio::io_service & io_service() { return *service; }
      class thread_group & thread_group() { return *thread_group_; }
      asio::io_service const & io_service() const { return *service; }
      class thread_group const & thread_group() const { return *thread_group_; }
      unsigned int threads() const { return thread_group_->size(); }
      bool stopped() const { return service->stopped(); }

    private:
      bool process()
      {
        detail::task_base * task = 0;
        if (tasks.pop(task))
        {
          current.reset(task);
          if (current.get() && !current->created())
            current->create();
          if (current.get() && !current->finished())
            current->enter();
          if (current.get() && !current->finished())
            tasks.push(current.release());
          else
            current.reset();
          return true;
        }
        else
          return false;
      }

      void dispatcher()
      {
        while (!service->stopped())
        {
          if (process())
            boost::this_thread::yield();
          else if (!service->poll_one())
            boost::this_thread::sleep_for(boost::chrono::microseconds(200));
        }
      }

      void make_current(detail::task_base * task)
      {
        current.reset(task);
      }

      void flush()
      {
        while (!tasks.empty())
        {
          detail::task_base * task;
          if (tasks.pop(task))
            delete task;
        }
      }

    private:
      shared_ptr<asio::io_service> service;
      std::auto_ptr<asio::io_service::work> work;
      std::auto_ptr<boost::thread_group> thread_group_;
      thread_specific_ptr<detail::task_base> current;
      lockfree::queue<detail::task_base*> tasks;
    };

    namespace detail
    {
      template<typename Handler, class E>
      task<Handler,E>::task(Handler handler, cgl::scheduler * scheduler):
      handler(handler), scheduler(scheduler), coro(), ca(0) { }
      template<typename Handler, class E>
      void task<Handler,E>::create() { scheduler->make_current(this); coro.reset(new coro_t(bind(&task::coroutine, this, _1))); }
      template<typename Handler, class E>
      void * task<Handler,E>::enter() { scheduler->make_current(this); coro->operator()();
        if(coro->has_result()) { result = coro->get(); return result.get_ptr(); } else return 0; }
      template<typename Handler, class E>
      void task<Handler,E>::yield(void * value) { assert(value); ca->operator()(*reinterpret_cast<typename remove_reference<E>::type*>(value));
        scheduler->make_current(this); }
      template<typename Handler, class E>
      void task<Handler,E>::yield(const void * value) { assert(value); ca->operator()(*reinterpret_cast<
        const typename remove_reference<E>::type*>(value)); scheduler->make_current(this); }
      template<typename Handler, class E>
      E task<Handler,E>::coroutine(typename coro_t::caller_type & ca) { this->ca = &ca; scheduler->make_current(this); return handler(); }
      // ----
      template<typename Handler>
      task<Handler,void>::task(Handler handler, cgl::scheduler * scheduler):
      handler(handler), scheduler(scheduler), coro(), ca(0) { }
      template<typename Handler>
      void task<Handler, void>::create() { scheduler->make_current(this); coro.reset(new coro_t(bind(&task::coroutine, this, _1))); }
      template<typename Handler>
      void * task<Handler, void>::enter() { scheduler->make_current(this); coro->operator()(); return 0; }
      template<typename Handler>
      void task<Handler, void>::yield(void *) { ca->operator()(); scheduler->make_current(this); }
      template<typename Handler>
      void task<Handler, void>::yield(const void *) { ca->operator()(); scheduler->make_current(this); }
      template<typename Handler>
      void task<Handler, void>::coroutine(typename coro_t::caller_type & ca) { this->ca = &ca; scheduler->make_current(this); handler(); }
    }
  }
}

#endif

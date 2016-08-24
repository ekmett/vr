#include "framework/stdafx.h"
#include "framework/error.h"
#include "framework/cds.h"
#include "framework/grammar.h"
#include "framework/worker.h"
#include <chrono>

using namespace std;
using namespace std::chrono;
using namespace framework;

namespace framework {

  void worker::main() {
#ifdef FRAMEWORK_SUPPORTS_CDS
    cds_thread_attachment attach_thread;
#endif

    uniform_int_distribution<int> random_peer(0, p.N - 2);
    exponential_distribution<double> random_delay_us(100.0); // 0.1ms expected task size
    string name = fmt::format("worker {}", i);
    shared_ptr<logger> diary = log(name.c_str());
    auto d = high_resolution_clock::now();
    diary->info("starting: {} {} in queue", q.size(), plural(q.size(), "item", "items"));
    for (;;) {
      task t;
      if (p.shutdown.load(std::memory_order_relaxed)) {
        diary->info("pool shutdown");
        return; // check for pool shutdown
      }
      if (q.empty()) {
        diary->info("looking for work");
        // acquire
        p.s[i].data.store(nullptr, memory_order_relaxed);
        // TODO: introduce exponential backoff
        task * tp = p.s[i].data.load(memory_order_relaxed);
        while (t == nullptr) {
          //diary->info("unemployed");
          this_thread::yield();
          if (p.shutdown.load(std::memory_order_relaxed)) return; // check for pool shutdown
          tp = p.s[i].data.load(memory_order_relaxed);
        }
        diary->info("employed");
        t = *tp;
        delete tp;
      } else {
        diary->info("have work");
        t = q.back();
        q.pop_back();
      }
      if (p.N > 1) { // we have peers, so see if we should hand off work
        auto then = high_resolution_clock::now();
        // communicate if we should deal and we have something to deal out
        if (then > d && !q.empty()) {
          // deal_attempt
          int j = random_peer(rng);
          if (j >= i) j = j + 1; // make sure it isn't us. can't wrap: j < N-1 before.

          task * expected = nullptr;
          // weak should be fine, we're already in an outer loop, we'll come back
          // on excessively weak architectures, this might mean that the effective delay is much higher though
          if (p.s[j].data.load(memory_order_relaxed) == nullptr
            && p.s[j].data.compare_exchange_weak(expected, &q.front(), memory_order_seq_cst)) {
            q.pop_front(); // we gave the front of the deque away
            diary->info("sent work to {}", j);
          }

          // don't resample time and round down to err on the side of too much sharing.
          d = then - chrono::floor<high_resolution_clock::duration>(
            duration<double, micro>(random_delay_us(rng))
          );
        }
      }
      try {
        t(*this);
      } catch (std::exception & e) {
        p.shutdown.store(true, std::memory_order_release);
        diary->critical("exception: {}, shutting down pool", e.what());
        throw;
      } catch (...) {
        p.shutdown.store(true, std::memory_order_release);
        diary->critical("non std::exception caught, shutting down pool");
        throw;
      }
    }
    diary->info("stopping work");
  } // worker::main

  pool::~pool() {
    shutdown.store(true, std::memory_order_release);
    for (auto && thread : threads)
      thread.join();
  }

  detail::dummy_task detail::dummy_task::instance;
}
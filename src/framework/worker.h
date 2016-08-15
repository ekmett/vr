#pragma once
/*
#include "framework/cds.h"
#include "framework/chase_lev_deque.h"

namespace framework {
  static const int max_workers = 16;
  static const int epochs = 3;

  typedef uint16_t worker_id;
  typedef worker * pworker;

  struct task {
    virtual void work(worker & me) = 0;
  };

  struct worker;

  typedef std::function<void(worker&)> * task;

  struct task {
    
  };

  struct worker {
    worker_id id, N;
    chase_lev_deque<task*> work[epochs];
    worker ** peers; // shuffled as we steal work from the peers for fairness, we don't own our peers, just the storage for the array.
    //static concurrent_list idlers; // 
    // template <typename Guard> friend struct pool<Guard>;
    static std::atomic<int> epoch;
    static thread_local worker * self;
  };

  template <typename Guard>
  struct pool {
    pool(size_t N = min(max(cds::OS::topology::processor_count() - 2, 1), 16)) // a number between 1 and 16, minus one for our gl thread and one for openvr/os.
      : N(N) {
      workers = new worker[N];
      workers.reserve(
      for (int i = 0;i < N;++i) {
        workers[i].id = i;
        workers[i].N = N;
        workers[i].peers = new worker*[N - 1];
        for (int j = 0;j<N-1;++k) {
          workers[i].peers[j] = workers[j];
        }
      }
      worker * last;
      for (int i = 0;i<N-1;++i) {
        workers[i].peers[i] = last;
        threads.push_back(std::thread(schedule(workers[i])); // this should be worked on a given processor for better affinity.
      }
      threads.push_back(std::thread(schedule(workers[N - 1]));
      make_unique<thread[]>(n)
      std::thread(
    }

    size_t size() const { return N }

    const size_t N;
    worker * workers;
    std::vector<thread> threads;

    struct managed_thread {
    }
  }
}
*/
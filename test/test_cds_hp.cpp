#include "stdafx.h"
#include "CppUnitTest.h"

#include <cds/init.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace test {		
  // tests directly using the cds hazard pointer implementation
	TEST_CLASS(test_cds_hp) {
	public:
    TEST_METHOD_INITIALIZE(init) {
      cds::Initialize();
      cds::gc::hp::GarbageCollector::Construct();
      cds::threading::Manager::attachThread();
    }
    TEST_METHOD_CLEANUP(fini) {
      cds::threading::Manager::detachThread();
      cds::gc::hp::GarbageCollector::Destruct();
      cds::Terminate();
    }
    TEST_METHOD(main_attachment) {      
      Assert::IsTrue(cds::threading::Manager::isThreadAttached());
    }
    TEST_METHOD(thread_attachment) {      
      std::thread worker([&] {
        Assert::IsFalse(cds::threading::Manager::isThreadAttached());
        cds::threading::Manager::attachThread();
        Assert::IsTrue(cds::threading::Manager::isThreadAttached());
        cds::threading::Manager::detachThread();
      });
      worker.join();
    }
	};
}
/*******************************************************************************
//
//  SYCL 2020 Conformance Test Suite
//
//  Provides tests for sycl::aligned_alloc_device
//
*******************************************************************************/

#include "../common/common.h"
#include "usm_allocate_free.h"

#define TEST_NAME usm_aligned_alloc_device

namespace TEST_NAMESPACE {
using namespace sycl_cts;

class TEST_NAME : public sycl_cts::util::test_base {
 public:
  /** return information about this test
   */
  void get_info(test_base::info &out) const override {
    set_test_info(out, TOSTRING(TEST_NAME), TEST_FILE);
  }

  /** execute the test
   */
  void run(util::logger &log) override {
    using namespace usm_allocate_free;
    using TestType = int;
    using op_aligned_alloc_device =
        usm_operation<usm_op_name::malloc_device, usm_op_form::aligned>;
    run_test<TestType, op_aligned_alloc_device>(log);
  }
};

// construction of this proxy will register the above test
util::test_proxy<TEST_NAME> proxy;

}  // namespace TEST_NAMESPACE

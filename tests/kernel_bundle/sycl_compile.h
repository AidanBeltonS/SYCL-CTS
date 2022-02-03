/*******************************************************************************
//
//  SYCL 2020 Conformance Test Suite
//
//  Provides tools for sycl::compile tests
//
*******************************************************************************/

#include "../../util/aspect_set.h"
#include "../../util/device_set.h"
#include "../../util/kernel_restrictions.h"
#include "../common/assertions.h"
#include "../common/common.h"
#include "kernel_bundle.h"
#include "kernels.h"

#include <algorithm>

#ifndef __SYCLCTS_TESTS_SYCL_COMPILE_H
#define __SYCLCTS_TESTS_SYCL_COMPILE_H

namespace sycl_cts::tests::sycl_compile {

class TestCaseDescription
    : public kernel_bundle::TestCaseDescriptionBase<sycl::bundle_state::input> {
 public:
  TestCaseDescription(std::string_view functionOverload)
      : TestCaseDescriptionBase<sycl::bundle_state::input>("sycl::compile",
                                                           functionOverload){};
};

/** @brief Used to select sycl::compile overload
 */
enum class CompileOverload { bundle_and_devs, bundle_only };

using input_bundle_t = sycl::kernel_bundle<sycl::bundle_state::input>;

/** @brief Call sycl::compile overload selected by Overload tparam
 *  @tparam Overload Selected overload of sycl::compile
 */
template <CompileOverload Overload>
static auto compile_bundle(const input_bundle_t &inKb,
                           const std::vector<sycl::device> &devices) {
  if constexpr (Overload == CompileOverload::bundle_and_devs) {
    return sycl::compile(inKb, devices);
  } else if constexpr (Overload == CompileOverload::bundle_only) {
    return sycl::compile(inKb);
  } else {
    static_assert(Overload != Overload, "Wrong Overload passed");
  }
}

/** @brief Check that sycl::compile result bundle contains kernel given
 *  @tparam DescriptorT Selected kernel descriptor
 *  @tparam Overload Selected overload of sycl::compile
 */
template <typename DescriptorT, CompileOverload Overload>
static void check_bundle_kernels(util::logger &log, const std::string &kName) {
  using kernel_t = typename DescriptorT::type;

  auto queue = util::get_cts_object::queue();
  auto ctx = queue.get_context();
  auto dev = queue.get_device();

  auto k_id = sycl::get_kernel_id<kernel_t>();
  // Test can be skipped if all devices do not support online compilation
  // since this is not sycl::compile's fault
  if (!sycl::has_kernel_bundle<sycl::bundle_state::input>(ctx, {dev}, {k_id})) {
    log.note("No kernel bundle with input state with kernel: " + kName +
             " (skipped).");
    return;
  }

  auto input_kb =
      sycl::get_kernel_bundle<sycl::bundle_state::input>(ctx, {dev}, {k_id});
  auto input_ids = input_kb.get_kernel_ids();

  auto obj_kb = compile_bundle<Overload>(input_kb, {dev});
  kernel_bundle::define_kernel<DescriptorT, sycl::bundle_state::input>(queue);

  auto restrictions{
      kernel_bundle::get_restrictions<DescriptorT,
                                      sycl::bundle_state::input>()};
  bool dev_is_compat = restrictions.is_compatible(dev);

  if (obj_kb.has_kernel(k_id) != dev_is_compat) {
    FAIL(log, "Device does not support kernel " + kName +
                  " but compiled bundle"
                  " contains it");
  }

  // Check that result object bundle has the same kernels as input bundle
  if (!std::any_of(input_ids.cbegin(), input_ids.cend(),
                   [&](auto &in_id) { return obj_kb.has_kernel(in_id); })) {
    FAIL(log,
         "Result bundle does not contain all kernels from input bundle "
         "(kernel: " +
             kName + ")");
  }

  // Check that input and result kernel bundles have the same context
  if (input_kb.get_context() != obj_kb.get_context()) {
    FAIL(log,
         "Input bundle and result bundle have different contexts "
         "(kernel: " +
             kName + ")");
  }
}

/** @brief Check that sycl::compile result bundle has the same associated
 *         devices as presented in input device vector
 *  @tparam Overload Selected overload of sycl::compile
 */
template <CompileOverload Overload>
static void check_associated_devices(util::logger &log) {
  auto queue = util::get_cts_object::queue();
  const auto ctx = queue.get_context();
  std::vector<sycl::device> devices{queue.get_device()};
  // Force duplicates
  devices.push_back(devices[0]);

  // Test can be skipped if all devices do not support online compilation
  // since this is not sycl::compile's fault
  if (!sycl::has_kernel_bundle<sycl::bundle_state::input>(ctx)) {
    log.note("No kernel bundle with input state for test (skipped).");
    return;
  }

  const auto input_kb =
      sycl::get_kernel_bundle<sycl::bundle_state::input>(ctx, devices);
  const auto obj_kb = compile_bundle<Overload>(input_kb, devices);
  const auto kb_devs = obj_kb.get_devices();

  // Check that result kernel bundle contains all devices from passed vector
  if (!std::equal(kb_devs.begin(), kb_devs.end(), devices.begin())) {
    FAIL(log,
         "Set of associated to obj_kb devices is not equal to list of "
         "devices passed.");
  }

  // TODO: we can't use std::set<sycl::device> due to the fact that sycl::device
  // has no comparison operators.
  // Once this issue is resolved, the following statement can be used instead of
  // cycling with a std::count call:
  //  std::sort(kb_devs.begin(), kb_devs.end());
  //  if (std::adjacent_find(kb_devs.begin(), kb_devs.end()) != kb_devs.end()) {

  // Check that result kernel bundle does not have duplicates
  bool unique_devs = true;
  for (const auto &dev : kb_devs) {
    unique_devs &= (std::count(kb_devs.begin(), kb_devs.end(), dev) == 1);
  }
  if (!unique_devs) {
    FAIL(log, "Set of associated to obj_kb devices has duplicates.");
  }

  using Descr = kernels::simple_kernel_descriptor;
  kernel_bundle::define_kernel<Descr, sycl::bundle_state::input>(queue);
}

}  // namespace sycl_cts::tests::sycl_compile

#endif  // __SYCLCTS_TESTS_SYCL_COMPILE_H

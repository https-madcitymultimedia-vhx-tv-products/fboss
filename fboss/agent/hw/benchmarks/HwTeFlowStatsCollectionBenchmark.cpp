/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/Platform.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTeFlowTestUtils.h"
#include "fboss/agent/hw/test/HwTestTeFlowUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/FunctionCallTimeReporter.h"

#include "fboss/agent/benchmarks/AgentBenchmarks.h"

#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>

DEFINE_int32(teflow_scale_entries, 9216, "Teflow scale entries");

namespace facebook::fboss {

/*
 * Collect Teflow stats with 9K entries and benchmark that.
 */

BENCHMARK(HwTeFlowStatsCollection) {
  folly::BenchmarkSuspender suspender;

  static std::string nextHopAddr("1::1");
  static std::string ifName("fboss2000");
  static std::string dstIpStart("100");
  static int prefixLength(61);
  uint32_t numEntries = FLAGS_teflow_scale_entries;
  // @lint-ignore CLANGTIDY
  FLAGS_enable_exact_match = true;
  std::unique_ptr<AgentEnsemble> ensemble{};

  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](SwSwitch* swSwitch, const std::vector<PortID>& ports) {
        CHECK_GT(ports.size(), 0);

        // Before m-mpu agent test, use first Asic for initialization.
        auto switchIds = swSwitch->getHwAsicTable()->getSwitchIDs();
        CHECK_GE(switchIds.size(), 1);
        auto asic = swSwitch->getHwAsicTable()->getHwAsic(*switchIds.cbegin());
        return utility::onePortPerInterfaceConfig(
            swSwitch->getPlatformMapping(),
            asic,
            ports,
            asic->desiredLoopbackModes());
      };

  AgentEnsemblePlatformConfigFn platformConfigFn =
      [](cfg::PlatformConfig& config) {
        if (!(config.chip()->getType() == config.chip()->bcm)) {
          return;
        }
        auto& bcm = *(config.chip()->bcm_ref());
        // enable exact match in platform config
        AgentEnsemble::enableExactMatch(bcm);
      };

  ensemble = createAgentEnsemble(initialConfigFn, platformConfigFn);
  const auto& ports = ensemble->masterLogicalPortIds();
  auto hwSwitch = ensemble->getHw();
  auto ecmpHelper =
      utility::EcmpSetupAnyNPorts6(ensemble->getSw()->getState(), RouterID(0));
  // Setup EM Config
  auto state = ensemble->getSw()->getState();
  utility::setExactMatchCfg(&state, prefixLength);
  ensemble->applyNewState(state);
  // Resolve nextHops
  CHECK_GE(ports.size(), 2);
  ensemble->applyNewState(ecmpHelper.resolveNextHops(
      ensemble->getSw()->getState(), {PortDescriptor(ports[0])}));
  ensemble->applyNewState(ecmpHelper.resolveNextHops(
      ensemble->getSw()->getState(), {PortDescriptor(ports[1])}));
  // Add Entries
  auto flowEntries = utility::makeFlowEntries(
      dstIpStart, nextHopAddr, ifName, ports[0], numEntries);
  state = ensemble->getSw()->getState();
  utility::addFlowEntries(&state, flowEntries, ensemble->scopeResolver());
  ensemble->applyNewState(state, true /* rollback on fail */);
  CHECK_EQ(utility::getNumTeFlowEntries(hwSwitch), numEntries);
  // Measure stats collection time for 9K entries
  SwitchStats dummy;
  suspender.dismiss();
  hwSwitch->updateStats(&dummy);
  suspender.rehire();
}

} // namespace facebook::fboss

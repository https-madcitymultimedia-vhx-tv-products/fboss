/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/fan_service/Bsp.h"
#include "fboss/platform/fan_service/SensorData.h"
#include "fboss/platform/helpers/Init.h"

using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::fan_service;

namespace {

class FanServiceTest : public ::testing::Test {};

FanServiceConfig getConfig() {
  auto config = FanServiceConfig{};
  auto fanServiceConfJson = ConfigLib().getFanServiceConfig();
  apache::thrift::SimpleJSONSerializer::deserialize<FanServiceConfig>(
      fanServiceConfJson, config);
  return config;
}

} // namespace

namespace facebook::fboss::platform::fan_service {

/*
 * Group 1. Configuration related tests
 */

TEST_F(FanServiceTest, configParse) {
  // Check if the config file for this platform can be
  // parsed without an error.
  EXPECT_NO_THROW(getConfig());
}

TEST_F(FanServiceTest, configShutdownCommand) {
  // Make sure that shutdown command is not empty
  EXPECT_NE(*getConfig().shutdownCmd(), "");
}

TEST_F(FanServiceTest, configTransitionValue) {
  // Make sure transitional fan speed value (until the first sensor read)
  // is configured correctly.
  EXPECT_GT(*getConfig().pwmTransitionValue(), 0.0);
}

/*
 * Group 2. BSP related tests
 */

TEST_F(FanServiceTest, bspInvalidWrite) {
  // Check BSP system write features by giving invalid parameter and
  // make sure the write fails
  Bsp myBsp(FanServiceConfig{});
  EXPECT_EQ(myBsp.setFanPwmSysfs("/dev/null/fake", 1), false);
}

TEST_F(FanServiceTest, bspTestSystemCommandFunction) {
  // Make sure BSP is capable of running shell command.
  // This feature is important for pwm configuration and
  // emergency command execution.
  Bsp myBsp(FanServiceConfig{});
  EXPECT_EQ(myBsp.setFanPwmShell("ls", "", 0), true);
}

TEST_F(FanServiceTest, bspTestSystemTime) {
  // Make sure BSP's system time checking is working
  Bsp myBsp(FanServiceConfig{});
  EXPECT_NE(myBsp.getCurrentTime(), 0);
}

/*
 * Group 3. Sensor Data Storage Class
 */

TEST_F(FanServiceTest, sensorDataEntryCheck) {
  // Make sure BSP's system time checking is working
  SensorData mySensorData;
  mySensorData.updateEntryInt("TEST1", 0, 1);
  EXPECT_EQ(mySensorData.checkIfEntryExists("TEST1"), true);
}

TEST_F(FanServiceTest, sensorDataValue) {
  // Make sure BSP's system time checking is working
  SensorData mySensorData;
  mySensorData.updateEntryInt("TEST2", 2, 3);
  EXPECT_EQ(mySensorData.getSensorDataInt("TEST2"), 2);
}

TEST_F(FanServiceTest, sensorDataUpdateTime) {
  // Make sure BSP's system time checking is working
  SensorData mySensorData;
  mySensorData.updateEntryInt("TEST3", 12, 30);
  EXPECT_EQ(mySensorData.getLastUpdated("TEST3"), 30);
}

} // namespace facebook::fboss::platform::fan_service

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(argc, argv);
  return RUN_ALL_TESTS();
}


#include "googletest/googletest/include/gtest/gtest.h"
#include "googletest/googlemock/include/gmock/gmock.h"

#include "system/scanner/OpenwaterScanningSystem_Pulsed/ConexStage.h"

using testing::Return;
using testing::_;

// Mock for RS232
class MockRS232 : public RS232 {
 public:
  MOCK_METHOD3(Open, int(int port, int baud_rate, const char* mode));
  MOCK_CONST_METHOD0(Port, int());
  MOCK_METHOD1(SendString, int(const std::string&));
  MOCK_METHOD2(Poll, int(unsigned char* buffer, int max_size));
};

// Test subclass using MockRS2332
class ConexStageTest : public ConexStage {
 public:
  ConexStageTest(MockRS232 *mock) { rs232_ = mock; }
  ~ConexStageTest() { rs232_ = NULL; } // keep parent class from deleting stack object
};

class StagesTest : public ::testing::Test {
 public:
  StagesTest() {}
};

static int FakePoll(unsigned char* s, int sz) {
  const unsigned char readyFromHoming = '2';
  s[8] = readyFromHoming;
  return 10;
}

static int FakePollForLocation(unsigned char* s, int sz) {
  strcpy((char*)s, "1TP4.2500");
  return 10;
}

static int FakePollForHardwareError_disFromReady(unsigned char* s, int sz) {
  strcpy((char*)s, "1TS00003C");
  return 10;
}

static int FakePollForHardwareError_disFromMoving(unsigned char* s, int sz) {
  strcpy((char*)s, "1TS00003D");
  return 10;
}

static int FakePollForHardwareError_nrFromMoving(unsigned char* s, int sz) {
  strcpy((char*)s, "1TS00000F");
  return 10;
}

static int FakePollForHardwareError_nr(unsigned char* s, int sz) {
  strcpy((char*)s, "1TS000010");
  return 10;
}

static int FakePollForHardwareError_nrFromReady(unsigned char* s, int sz) {
  strcpy((char*)s, "1TS00000E");
  return 10;
}

TEST(StagesTest, initFailsWhenRs232InitFails) {
  testing::NiceMock<MockRS232> mockRS232;
  ConexStageTest test(&mockRS232);
  EXPECT_CALL(mockRS232, Open(6 - 1, 921600, _)).Times(1).WillOnce(Return(1));
  ASSERT_FALSE(test.init(6));
}

TEST(StagesTest, initSucceedsWhenRs232InitSucceeds) {
  testing::NiceMock<MockRS232> mockRS232;
  ConexStageTest test(&mockRS232);
  EXPECT_CALL(mockRS232, Open(6 - 1, 921600, _)).Times(1).WillOnce(Return(0));
  ASSERT_TRUE(test.init(6));
}

TEST(StagesTest, stageMoving) {
  testing::NiceMock<MockRS232> mockRS232;
  ConexStageTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString("1TS\r\n")).Times(1).WillOnce(Return(1));
  ON_CALL(mockRS232, Poll).WillByDefault(FakePoll);
  ASSERT_EQ(0, test.stageMoving());

  EXPECT_CALL(mockRS232, SendString("1TS\r\n")).Times(1).WillOnce(Return(1));
  ON_CALL(mockRS232, Poll).WillByDefault(FakePollForHardwareError_disFromReady);
  ASSERT_EQ(-1, test.stageMoving());

  EXPECT_CALL(mockRS232, SendString("1TS\r\n")).Times(1).WillOnce(Return(1));
  ON_CALL(mockRS232, Poll).WillByDefault(FakePollForHardwareError_disFromMoving);
  ASSERT_EQ(-1, test.stageMoving());

  EXPECT_CALL(mockRS232, SendString("1TS\r\n")).Times(1).WillOnce(Return(1));
  ON_CALL(mockRS232, Poll).WillByDefault(FakePollForHardwareError_nrFromMoving);
  ASSERT_EQ(-1, test.stageMoving());

  EXPECT_CALL(mockRS232, SendString("1TS\r\n")).Times(1).WillOnce(Return(1));
  ON_CALL(mockRS232, Poll).WillByDefault(FakePollForHardwareError_nr);
  ASSERT_EQ(-1, test.stageMoving());

  EXPECT_CALL(mockRS232, SendString("1TS\r\n")).Times(1).WillOnce(Return(1));
  ON_CALL(mockRS232, Poll).WillByDefault(FakePollForHardwareError_nrFromReady);
  ASSERT_EQ(-1, test.stageMoving());
}

TEST(StagesTest, getStageLocation) {
  testing::NiceMock<MockRS232> mockRS232;
  ConexStageTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString("1TP\r\n")).Times(1).WillOnce(Return(1));
  ON_CALL(mockRS232, Poll).WillByDefault(FakePollForLocation);
  ASSERT_EQ(4.25, test.getStageLocation());
}

TEST(StagesTest, resetController) {
  testing::NiceMock<MockRS232> mockRS232;
  ConexStageTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString("1RS\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString("1TS\r\n")).Times(1).WillOnce(Return(1));
  ON_CALL(mockRS232, Poll).WillByDefault(FakePoll);
  ASSERT_TRUE(test.resetController());
}

TEST(StagesTest, moveHome) {
  testing::NiceMock<MockRS232> mockRS232;
  ConexStageTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString("1OR\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString("1TS\r\n")).Times(1).WillOnce(Return(1));
  ON_CALL(mockRS232, Poll).WillByDefault(FakePoll);
  ASSERT_TRUE(test.moveHome());
}

TEST(StagesTest, disableController) {
  testing::NiceMock<MockRS232> mockRS232;
  ConexStageTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString("1MM0\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.disableController());
}

TEST(StagesTest, moveRelative) {
  testing::NiceMock<MockRS232> mockRS232;
  ConexStageTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString("1PR3.2\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString("1TS\r\n")).Times(1).WillOnce(Return(1));
  ON_CALL(mockRS232, Poll).WillByDefault(FakePoll);
  ASSERT_TRUE(test.moveRelative(3.2));
}

TEST(StagesTest, moveAbsolute) {
  testing::NiceMock<MockRS232> mockRS232;
  ConexStageTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString("1PA2.9\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString("1TS\r\n")).Times(1).WillOnce(Return(1));
  ON_CALL(mockRS232, Poll).WillByDefault(FakePoll);
  ASSERT_TRUE(test.moveAbsolute(2.9));
}

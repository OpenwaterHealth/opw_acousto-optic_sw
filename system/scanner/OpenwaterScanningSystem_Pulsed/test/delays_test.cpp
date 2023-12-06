#include "googletest/googletest/include/gtest/gtest.h"
#include "googletest/googlemock/include/gmock/gmock.h"

#include "system/scanner/OpenwaterScanningSystem_Pulsed/delays.h"

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
class BerkeleyNucleonicsTest : public BerkeleyNucleonics {
 public:
  BerkeleyNucleonicsTest(MockRS232 *mock) { rs232_ = mock; }
  ~BerkeleyNucleonicsTest() { rs232_ = NULL; } // keep parent class from deleting stack object

  void TestSetModelNumber(int modelNum) { modelNumber_ = modelNum; }
};

class DelaysTest : public ::testing::Test {
 public:
  DelaysTest() {}
};

static int FakePollForSetModelNumber(unsigned char* s, int sz) {
  strcpy((char*)s, "BNC,525");
  return 7;
}

static int FakePollForSetModelNumberFails(unsigned char* s, int sz) {
  strcpy((char*)s, "ok");  // Believe it or not, this is what we are getting in some cases.
  return 2;
}

TEST(Delays, initFailsIfOpenFails) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, Open(2, 115200, _)).Times(1).WillOnce(Return(1));  // 0: ok; 1: error
  ASSERT_FALSE(test.init(3));
}

TEST(Delays, errorsCounted) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  ON_CALL(mockRS232, SendString(_)).WillByDefault(Return(0));
  ASSERT_FALSE(test.setUnitPulseMode(CONTINUOUS));
  ASSERT_EQ(test.errors(), 1);
}

TEST(Delays, setUnitPulseMode) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS0:MOD NORM\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:MOD SING\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:MOD BURS\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:MOD DCYC\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setUnitPulseMode(CONTINUOUS));
  ASSERT_TRUE(test.setUnitPulseMode(SINGLE));
  ASSERT_TRUE(test.setUnitPulseMode(BURST));
  ASSERT_TRUE(test.setUnitPulseMode(DUTY_CYCLE));
}

TEST(Delays, setUnitExternalTriggerMode) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  test.TestSetModelNumber(int(525));
  EXPECT_CALL(mockRS232, SendString(":PULS0:EXT:MOD TRIG\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:EXT:MOD DIS\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setUnitExternalTriggerMode(true));
  ASSERT_TRUE(test.setUnitExternalTriggerMode(false));
  test.TestSetModelNumber(int(577));
  EXPECT_CALL(mockRS232, SendString(":PULS0:TRIG:MOD TRIG\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:TRIG:MOD DIS\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setUnitExternalTriggerMode(true));
  ASSERT_TRUE(test.setUnitExternalTriggerMode(false));
}

TEST(Delays, setPulseDelay) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS5:DEL foo\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setPulseDelay(5, "foo"));
}

TEST(Delays, setPulseWidth) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS9:WIDT foo\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setPulseWidth(9, "foo"));
}

TEST(Delays, setPulseAmplitude) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS2:OUTP:MOD ADJ\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS2:OUTP:AMPL 4.200000\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setPulseAmplitude(2, 4.2));
}

TEST(Delays, muxChannels) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS1:MUX 2\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.muxChannels(1, 2));
}

TEST(Delays, enableChannel) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS3:STAT ON\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS3:POL NORM\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS7:STAT OFF\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS7:POL NORM\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.enableChannel(3, true));
  ASSERT_TRUE(test.enableChannel(7, false));
}

TEST(Delays, enableUnit) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS0:STAT ON\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:STAT OFF\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.enableUnit(true));
  ASSERT_TRUE(test.enableUnit(false));
}

TEST(Delays, displayUpdate) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":DISP:UPD?\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.displayUpdate());
}

TEST(Delays, setChannelActiveHigh) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS1:POL NORM\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS6:POL INV\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setChannelActiveHigh(1, true));
  ASSERT_TRUE(test.setChannelActiveHigh(6, false));
}

TEST(Delays, setUnitPulseRate) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  // N.B.: This sends 1/freq = period.
  EXPECT_CALL(mockRS232, SendString(":PULS0:PER 0.200000\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setUnitPulseRate(5.0));
}

TEST(Delays, setUnitExternalClock) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS0:ICL 10\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:ICL S\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setUnitExternalClock(10, true));
  ASSERT_TRUE(test.setUnitExternalClock(0, false));
}

TEST(Delays, resetUnit) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString("*RST\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.resetUnit());
}

TEST(Delays, setUnitChannelGateMode) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS0:GAT:MOD ENABLE\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:GAT:MOD CHPU\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:GAT:LOG HIGH\r\n")).Times(2).WillRepeatedly(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:GAT:LEV 0.5\r\n")).Times(2).WillRepeatedly(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:GAT:MOD DIS\r\n")).Times(2).WillRepeatedly(Return(1));
  ASSERT_TRUE(test.setUnitChannelGateMode(true));
  ASSERT_TRUE(test.setUnitChannelGateMode(false));
}

TEST(Delays, gateChannel) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS2:CGAT HIGH\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS1:CGAT DIS\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.gateChannel(2, true));
  ASSERT_TRUE(test.gateChannel(1, false));
}

TEST(Delays, setNumberBurstPulses) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS0:BCO 27\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setNumberBurstPulses(27));
}

TEST(Delays, setModelNumber) {
  testing::NiceMock<MockRS232> mockRS232;
  BerkeleyNucleonicsTest test(&mockRS232);
  ON_CALL(mockRS232, Poll).WillByDefault(FakePollForSetModelNumber);
  EXPECT_CALL(mockRS232, SendString("*IDN?\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setModelNumber());
  ASSERT_EQ(test.getModelNumber(), 525);
  EXPECT_CALL(mockRS232, SendString("*IDN?\r\n")).Times(1).WillOnce(Return(1));
  ON_CALL(mockRS232, Poll).WillByDefault(FakePollForSetModelNumberFails);
  ASSERT_FALSE(test.setModelNumber());
}

#include "googletest/googletest/include/gtest/gtest.h"
#include "googletest/googlemock/include/gmock/gmock.h"

#include "system/scanner/OpenwaterScanningSystem_Pulsed/QuantumComposers.h"
#include "system/scanner/OpenwaterScanningSystem_Pulsed/Verdi.h"

using testing::Return;
using testing::_;

// Mock for RS232
class MockRS232 : public RS232 {
 public:
  MOCK_METHOD3(Open, int(int port, int baud_rate, const char* mode));
  MOCK_CONST_METHOD0(Port, int());
  MOCK_METHOD1(SendString, int(const std::string&));
};

// Test subclass using MockRS2332
class QuantumComposersTest : public QuantumComposers {
 public:
  QuantumComposersTest(MockRS232 *mock) { rs232_ = mock; }
  ~QuantumComposersTest() { rs232_ = NULL; } // keep parent class from deleting stack object
};

// Test subclass using MockRS2332
class VerdiTest : public Verdi {
 public:
  VerdiTest(MockRS232* mock) { rs232_ = mock; }
  ~VerdiTest() { rs232_ = NULL; } // keep parent class from deleting stack object
};

class LaserTest : public ::testing::Test {
 public:
  LaserTest() {}
};

TEST(LaserTest, initFailsIfRs232Fails) {
  testing::NiceMock<MockRS232> mockRS232;
  QuantumComposersTest test(&mockRS232);
  EXPECT_CALL(mockRS232, Open(6, 38400, _)).Times(1).WillOnce(Return(-1));
  ASSERT_FALSE(test.init(7));
}

TEST(LaserTest, initFailsIfRs232Fails_Verdi) {
  testing::NiceMock<MockRS232> mockRS232;
  VerdiTest test(&mockRS232);
  EXPECT_CALL(mockRS232, Open(3, 19200, _)).Times(1).WillOnce(Return(-1));
  ASSERT_FALSE(test.init(4));
}

TEST(LaserTest, initSucceedsIfRs232Succeeds) {
  testing::NiceMock<MockRS232> mockRS232;
  QuantumComposersTest test(&mockRS232);
  EXPECT_CALL(mockRS232, Open(6, 38400, _)).Times(1).WillOnce(Return(0));
  ASSERT_TRUE(test.init(7));
}

TEST(LaserTest, initSucceedsIfRs232Succeeds_Verdi) {
  testing::NiceMock<MockRS232> mockRS232;
  VerdiTest test(&mockRS232);
  EXPECT_CALL(mockRS232, Open(3, 19200, _)).Times(1).WillOnce(Return(0));
  ASSERT_TRUE(test.init(4));
}

TEST(LaserTest, setUnitChannelGateMode) {
  testing::NiceMock<MockRS232> mockRS232;
  QuantumComposersTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS0:GAT:MOD CHAN\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:GAT:LEV 1.200000\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:GAT:MOD DIS\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:GAT:LEV 3.400000\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setUnitChannelGateMode(true, 1.2));
  ASSERT_TRUE(test.setUnitChannelGateMode(false, 3.4));
}

TEST(LaserTest, setPulseWidth) {
  testing::NiceMock<MockRS232> mockRS232;
  QuantumComposersTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS9:WIDT foo\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setPulseWidth(9, "foo"));
}

TEST(LaserTest, setPulseDelay) {
  testing::NiceMock<MockRS232> mockRS232;
  QuantumComposersTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS5:DEL foo\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.setPulseDelay(5, "foo"));
}

TEST(LaserTest, gateChannel) {
  testing::NiceMock<MockRS232> mockRS232;
  QuantumComposersTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS3:CGAT PULS\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS3:CLOG HIGH\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS5:CGAT DIS\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.gateChannel(3, true));
  ASSERT_TRUE(test.gateChannel(5, false));
}

TEST(LaserTest, enableChannel) {
  testing::NiceMock<MockRS232> mockRS232;
  QuantumComposersTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS7:STAT ON\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS7:STAT OFF\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.enableChannel(7, true));
  ASSERT_TRUE(test.enableChannel(7, false));
}

TEST(LaserTest, enableChannel_Verdi) {
  testing::NiceMock<MockRS232> mockRS232;
  VerdiTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString("S:1\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString("S:0\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.enableChannel(1, true));
  ASSERT_TRUE(test.enableChannel(1, false));
}

TEST(LaserTest, enableUnit) {
  testing::NiceMock<MockRS232> mockRS232;
  QuantumComposersTest test(&mockRS232);
  EXPECT_CALL(mockRS232, SendString(":PULS0:STAT ON\r\n")).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockRS232, SendString(":PULS0:STAT OFF\r\n")).Times(1).WillOnce(Return(1));
  ASSERT_TRUE(test.enableUnit(true));
  ASSERT_TRUE(test.enableUnit(false));
}

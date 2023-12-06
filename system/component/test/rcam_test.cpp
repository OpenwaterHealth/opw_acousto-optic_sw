#include "googletest/googlemock/include/gmock/gmock.h"
#include "googletest/googletest/include/gtest/gtest.h"
#include "system/component/inc/rcam.h"

using testing::Return;
using testing::_;

class MockFX3 : public FX3 {
 public:
  MOCK_METHOD2(Open, int(uint16_t pid, int n));
  MOCK_METHOD2(Flash, int(int len, uint8_t* data));
  MOCK_METHOD5(CmdRead, int(uint8_t req, uint16_t wValue, uint16_t wIndex, uint16_t len, uint8_t* buf));
};

// Test subclass using MockFX3
class RcamTest : public Rcam {
 public:
  RcamTest(MockFX3* fx3): Rcam(fx3) {}
};

class TestRcam : public ::testing::Test {
 public:
  TestRcam() {}
};

// All of FX3 impl is commented out off-Win.
#ifndef _MSC_VER
int FX3::NumDevices(uint16_t) { return 0; }
int FX3::Open(uint16_t pid, int n) { return 0; }
void FX3::Close() {}
int FX3::Flash(int len, uint8_t* data) { return -1; }
int FX3::CmdWrite(uint8_t req, uint16_t wValue, uint16_t wIndex, uint16_t len, uint8_t* buf) { return 0; }
int FX3::CmdRead(uint8_t req, uint16_t wValue, uint16_t wIndex, uint16_t len, uint8_t* buf) { return 0; }
int FX3::SerialNumber() { return -1; }
int FX3::Reset() { return 0; }
void FX3::Reattach() {}
int FX3::DataIn(int len, uint8_t* data) { return 0; }
int FX3::DataOut(int len, uint8_t* data) { return 0; }
int FX3::BulkInData(int&, unsigned char*&) { return 0; }
void FX3::BulkInStop() {}
int FX3::BulkInStart() { return -1; }
int FX3::BulkInBuffers(int, int, int) { return -1; }
#endif

TEST(TestFrame, OpenFailsIfFX3OpenFails) {
  testing::NiceMock<MockFX3> mockFX3;
  RcamTest test(&mockFX3);
  EXPECT_CALL(mockFX3, Open(Rcam::pid(), 0)).Times(1).WillOnce(Return(-1));
  ASSERT_EQ(-1, test.Open(0));
}

TEST(TestFrame, OpenFailsIfFlashFails) {
  testing::NiceMock<MockFX3> mockFX3;
  RcamTest test(&mockFX3);
  EXPECT_CALL(mockFX3, Open(Rcam::pid(), 0)).Times(1).WillOnce(Return(0));
  EXPECT_CALL(mockFX3, Flash(_, _)).Times(1).WillOnce(Return(-1));
  ASSERT_EQ(-1, test.Open(0));
}

TEST(TestFrame, OpenFailsIfParamReadFails) {
  testing::NiceMock<MockFX3> mockFX3;
  RcamTest test(&mockFX3);
  EXPECT_CALL(mockFX3, Open(Rcam::pid(), 0)).Times(1).WillOnce(Return(0));
  EXPECT_CALL(mockFX3, Flash(_, _)).Times(1).WillOnce(Return(0));
  EXPECT_CALL(mockFX3, CmdRead(_, _, _, _, _)).Times(3).WillOnce(Return(0));
  ASSERT_EQ(-1, test.Open(0));
}

// TODO: more ...

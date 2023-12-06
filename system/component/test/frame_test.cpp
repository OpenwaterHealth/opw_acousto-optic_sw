#include "googletest/googlemock/include/gmock/gmock.h"
#include "googletest/googletest/include/gtest/gtest.h"
#include "system/component/inc/frame.h"
#include "system/component/inc/TiffInterface.h"
#include "system/third_party/inc/tiffio.h"
#include "system/third_party/inc/tiff.h"

using testing::Return;
using testing::_;

class MockTiff : public TiffInterface {
 public:
  MOCK_METHOD2(Open, bool(const char*, const char*));
  MOCK_METHOD0(Close, void());
  MOCK_METHOD2(GetField, int(ttag_t tag, int* vp));
  MOCK_METHOD2(GetField, int(ttag_t tag, uint16_t* vp));
  MOCK_METHOD2(SetField, int(ttag_t tag, int value));
  MOCK_METHOD2(ReadScanline, int(tdata_t buf, uint32 row));
  MOCK_METHOD2(WriteScanline, int(tdata_t buf, uint32 row));
  MOCK_METHOD0(WriteDirectory, int());
};

// Test subclass using MockTiff
class FrameTest : public Frame {
 public:
  FrameTest(MockTiff* tiff): Frame(20, 10) { tiff_ = tiff; /* happens after Init() */ }
  FrameTest() {}
  ~FrameTest() { tiff_ = NULL; }  // keep parent class from deleting stack object
  bool hasOutput() { return tiff_ != NULL; }
  TiffInterface* getTiff() { return tiff_; }
  void DoInitTiff() { InitTiff(); }
};

class TestFrame : public ::testing::Test {
 public:
  TestFrame() {}
};

TEST(TestFrame, FrameBitsIs16) {
  FrameTest test(NULL);
  ASSERT_EQ(16, test.bits);
}

TEST(TestFrame, FrameCreatesDefaultOutput) {
  FrameTest test;
  test.DoInitTiff();
  ASSERT_TRUE(test.hasOutput());
}

TEST(TestFrame, FramesCreateSeparateOutputs) {  // Necessary for tiff i/o to be reentrant.
  FrameTest test1;
  FrameTest test2;
  test1.DoInitTiff();  // These call InitTiff, and should create two TiffInterfaces.
  test2.DoInitTiff();  // (Previously, tiff_ was static.)
  ASSERT_FALSE(test1.getTiff() == test2.getTiff());
}

TEST(TestFrame, FrameDoesNotCopyOutput) {
  testing::NiceMock<MockTiff> mockTiff;
  FrameTest test1(&mockTiff);
  FrameTest test2(test1);
  ASSERT_TRUE(test1.getTiff() == &mockTiff);
  ASSERT_TRUE(test2.getTiff() == NULL);
}

TEST(TestFrame, FramesGetDistinctTimestamps) {
  Frame f1, f2;
  f1.SetTimestamp();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  f2.SetTimestamp();
  ASSERT_TRUE(f2.timestamp_ms_ > f1.timestamp_ms_);
}

TEST(TestFrame, WriteFailsWhenOpenFails) {
  testing::NiceMock<MockTiff> mockTiff;
  FrameTest test(&mockTiff);
  EXPECT_CALL(mockTiff, Open(_, _)).Times(1).WillOnce(Return(false));
  ASSERT_EQ(-1, test.Write("foo"));
}

#if 0  // not worth it; WriteScanline will fail if disk is full.
TEST(TestFrame, WriteFailsWhenSetFieldFails) {
  testing::NiceMock<MockTiff> mockTiff;
  FrameTest test(&mockTiff);
  EXPECT_CALL(mockTiff, Open(_, _)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(mockTiff, SetField(_, _)).Times(1).WillOnce(Return(-1));
  ASSERT_EQ(-1, test.Write("foo"));
}
#endif

TEST(TestFrame, WriteFailsWhenWriteScanlineFails) {
  testing::NiceMock<MockTiff> mockTiff;
  FrameTest test(&mockTiff);
  EXPECT_CALL(mockTiff, Open(_, _)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(mockTiff, SetField(_, _)).Times(12).WillRepeatedly(Return(1));
  EXPECT_CALL(mockTiff, WriteScanline(_, 0)).Times(1).WillOnce(Return(-1));
  ASSERT_EQ(-1, test.Write("foo"));
}

TEST(TestFrame, WriteFailsWhenWriteDirectoryFails) {
  testing::NiceMock<MockTiff> mockTiff;
  FrameTest test(&mockTiff);
  EXPECT_CALL(mockTiff, Open(_, _)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(mockTiff, SetField(_, _)).Times(12).WillRepeatedly(Return(1));
  EXPECT_CALL(mockTiff, WriteScanline(_, _)).Times(10).WillRepeatedly(Return(1));
  EXPECT_CALL(mockTiff, WriteDirectory()).Times(1).WillOnce(Return(-1));
  ASSERT_EQ(-1, test.Write("foo"));
}

TEST(TestFrame, AddAndGetTag) {
  struct TestTag : Frame::Tag {
    int i;
  } tag;
  Frame fr;
  fr.AddTag(&tag);
  ASSERT_EQ(fr.GetTag<TestTag>(), &tag);
}

TEST(TestFrame, TagNullWhenNoTag) {
  struct TestTag : Frame::Tag {
    int i;
  };
  Frame fr;
  ASSERT_EQ(fr.GetTag<TestTag>(), nullptr);
}

TEST(TestFrame, GetCorrectTag) {
  struct TestTag1 : Frame::Tag {
    int i;
  } tag1;
  struct TestTag2 : Frame::Tag {
    double i;
  } tag2;
  Frame fr;
  fr.AddTag(&tag1);
  EXPECT_EQ(fr.GetTag<TestTag1>(), &tag1);
  EXPECT_EQ(fr.GetTag<TestTag2>(), nullptr);
  fr.AddTag(&tag2);
  EXPECT_EQ(fr.GetTag<TestTag2>(), &tag2);
}

TEST(TestFrame, ClearTagRemovesTags) {
  struct TestTag : Frame::Tag {
    int i;
  } tag;
  Frame fr;
  fr.AddTag(&tag);
  EXPECT_EQ(fr.GetTag<TestTag>(), &tag);
  fr.ClearTags();
  EXPECT_EQ(fr.GetTag<TestTag>(), nullptr);
}

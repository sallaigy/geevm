#include "common/Encoding.h"

#include <gmock/internal/gmock-internal-utils.h>
#include <gtest/gtest.h>

TEST(EncodingTest, decode_modified_utf8_ascii)
{
  // 'foo'
  uint8_t bytes[] = {0x66, 0x6f, 0x6f};
  auto str = geevm::decodeJvmUtf8(bytes);

  EXPECT_EQ(str.size(), 3);
  EXPECT_EQ(str, "foo");
}

TEST(EncodingTest, decode_modified_utf8_ascii_null_code_point)
{
  // 'fo\0o
  uint8_t bytes[] = {0x66, 0x6f, 0xc0, 0x80, 0x6f};
  auto str = geevm::decodeJvmUtf8(bytes);

  ASSERT_EQ(str.size(), 4);

  char expectedBytes[] = {'f', 'o', '\0', 'o'};
  EXPECT_TRUE(std::equal(str.data(), str.data() + str.size(), expectedBytes));
}

TEST(EncodingTest, decode_modified_utf8_two_byte)
{
  // 'naïve'
  uint8_t bytes[] = {0x6e, 0x61, 0xc3, 0xaf, 0x76, 0x65};
  auto str = geevm::decodeJvmUtf8(bytes);

  EXPECT_EQ(str.size(), 6);
  EXPECT_EQ(str, "naïve");
}

TEST(EncodingTest, decode_modified_utf8_three_bytes)
{
  // 'ༀကⲊ'
  uint8_t bytes[] = {0xe0, 0xbc, 0x80, 0xe1, 0x80, 0x80, 0xe2, 0xb2, 0x8a};
  auto str = geevm::decodeJvmUtf8(bytes);

  EXPECT_EQ(str, "ༀကⲊ");
}

TEST(EncodingTest, decode_modified_utf8_supplementary_characters)
{
  // '𐐀' - U+10437
  uint8_t bytes[] = {0xed, 0xa0, 0x81, 0xed, 0xb0, 0x80};
  auto str = geevm::decodeJvmUtf8(bytes);

  uint8_t expectedUtf8[] = {0xF0, 0x90, 0x90, 0x80, 0x00};
  EXPECT_EQ(str, reinterpret_cast<const char*>(expectedUtf8));
}

TEST(EncodingTest, utf8_to_utf16)
{
  EXPECT_EQ(geevm::utf8ToUtf16("foo"), u"foo");
  EXPECT_EQ(geevm::utf8ToUtf16("\xC3\xB1"), u"ñ");
  EXPECT_EQ(geevm::utf8ToUtf16("\xE2\x82\xAC"), u"€");
  // U+10437
  EXPECT_EQ(geevm::utf8ToUtf16("\xF0\x90\x90\xB7"), u"\xD801\xDC37");

  // Empty string
  EXPECT_EQ(geevm::utf8ToUtf16(""), u"");
}

TEST(EncodingTest, utf16_to_utf8)
{
  EXPECT_EQ(geevm::utf16ToUtf8(u"foo"), "foo");
  EXPECT_EQ(geevm::utf16ToUtf8(u"ñ"), "\xC3\xB1");
  EXPECT_EQ(geevm::utf16ToUtf8(u"€"), "\xE2\x82\xAC");
  // U+10437
  EXPECT_EQ(geevm::utf16ToUtf8(u"\xD801\xDC37"), "\xF0\x90\x90\xB7");

  // Empty string
  EXPECT_EQ(geevm::utf16ToUtf8(u""), "");
}

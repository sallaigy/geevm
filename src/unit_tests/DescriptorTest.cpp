#include "BaseTest.h"

#include "class_file/Descriptor.h"

using namespace geevm;

TEST(DescriptorTest, primitive_types)
{
  EXPECT_EQ(FieldType{PrimitiveType::Byte}, FieldType::parse(u"B").value());
  EXPECT_EQ(FieldType{PrimitiveType::Char}, FieldType::parse(u"C").value());
  EXPECT_EQ(FieldType{PrimitiveType::Double}, FieldType::parse(u"D").value());
  EXPECT_EQ(FieldType{PrimitiveType::Float}, FieldType::parse(u"F").value());
  EXPECT_EQ(FieldType{PrimitiveType::Int}, FieldType::parse(u"I").value());
  EXPECT_EQ(FieldType{PrimitiveType::Long}, FieldType::parse(u"J").value());
  EXPECT_EQ(FieldType{PrimitiveType::Short}, FieldType::parse(u"S").value());
  EXPECT_EQ(FieldType{PrimitiveType::Boolean}, FieldType::parse(u"Z").value());
}

TEST(DescriptorTest, object_type)
{
  EXPECT_EQ(FieldType{u"java/lang/Object"}, FieldType::parse(u"Ljava/lang/Object;").value());
}

TEST(DescriptorTest, array_type)
{
  EXPECT_EQ(FieldType(PrimitiveType::Byte, 1), FieldType::parse(u"[B").value());
  EXPECT_EQ(FieldType(PrimitiveType::Long, 3), FieldType::parse(u"[[[J").value());
  EXPECT_EQ(FieldType(u"java/lang/Object", 1), FieldType::parse(u"[Ljava/lang/Object;").value());
  EXPECT_EQ(FieldType(u"java/lang/Object", 3), FieldType::parse(u"[[[Ljava/lang/Object;").value());
}

TEST(DescriptorTest, invalid_field_descriptor)
{
  EXPECT_EQ(std::nullopt, FieldType::parse(u"Ljava/lang/String"));
  EXPECT_EQ(std::nullopt, FieldType::parse(u""));
  EXPECT_EQ(std::nullopt, FieldType::parse(u"["));
  EXPECT_EQ(std::nullopt, FieldType::parse(u"V"));
  EXPECT_EQ(std::nullopt, FieldType::parse(u"[[["));
  EXPECT_EQ(std::nullopt, FieldType::parse(u"Bjava/lang/String;"));
}

TEST(MethodDescriptorTest, method_type)
{
  FieldType byteTy{PrimitiveType::Byte};
  FieldType floatTy{PrimitiveType::Float};
  FieldType float2Ty{PrimitiveType::Float, 2};
  FieldType javaStringTy{u"java/lang/String"};

  EXPECT_EQ(MethodDescriptor(ReturnType::VoidType, {byteTy, floatTy}), MethodDescriptor::parse(u"(BF)V").value());
  EXPECT_EQ(MethodDescriptor(ReturnType::VoidType, {}), MethodDescriptor::parse(u"()V").value());
  EXPECT_EQ(MethodDescriptor(ReturnType{floatTy}, {}), MethodDescriptor::parse(u"()F").value());
  EXPECT_EQ(MethodDescriptor(ReturnType{float2Ty}, {}), MethodDescriptor::parse(u"()[[F").value());
  EXPECT_EQ(MethodDescriptor(ReturnType{javaStringTy}, {javaStringTy}), MethodDescriptor::parse(u"(Ljava/lang/String;)Ljava/lang/String;").value());
}

TEST(MethodDescriptorTest, invalid_descriptor)
{
  EXPECT_EQ(std::nullopt, MethodDescriptor::parse(u"("));
  EXPECT_EQ(std::nullopt, MethodDescriptor::parse(u"()"));
  EXPECT_EQ(std::nullopt, MethodDescriptor::parse(u"(V)"));
  EXPECT_EQ(std::nullopt, MethodDescriptor::parse(u"(V"));
  EXPECT_EQ(std::nullopt, MethodDescriptor::parse(u"(Ljava/lang/String)Ljava/lang/String;"));
}

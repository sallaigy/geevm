#include "BaseTest.h"
#include "class_file/ClassFile.h"

using namespace geevm;

class ClassFileReaderTest : public geevm::testing::BaseTest
{
public:
  void readClassFile(const std::string& path)
  {
    auto file = getResource(path);
    classFile = ClassFile::fromFile(file.string());
  }

  void checkField(types::u2 index, const std::u16string& name, const std::u16string& descriptor, FieldAccessFlags accessFlags) const
  {
    const FieldInfo& field = classFile->fields()[index];

    EXPECT_EQ(field.accessFlags(), accessFlags);
    EXPECT_EQ(classFile->constantPool().getString(field.nameIndex()), name);
    EXPECT_EQ(classFile->constantPool().getString(field.descriptorIndex()), descriptor);
  }

  ::testing::AssertionResult checkMethod(types::u2 index, const std::u16string& name, const std::u16string& descriptor, MethodAccessFlags accessFlags) const
  {
    const MethodInfo& method = classFile->methods()[index];

    if (method.accessFlags() != accessFlags) {
      return ::testing::AssertionFailure() << "Method access flags mismatch:" << ::testing::PrintToString(method.accessFlags())
                                           << " expected: " << ::testing::PrintToString(accessFlags);
    }

    auto actualName = classFile->constantPool().getString(method.nameIndex());
    if (actualName != name) {
      return ::testing::AssertionFailure() << "Method name mismatch:" << ::testing::PrintToString(actualName)
                                           << " expected: " << ::testing::PrintToString(name);
    }

    auto actualDescriptor = classFile->constantPool().getString(method.descriptorIndex());
    if (actualDescriptor != descriptor) {
      return ::testing::AssertionFailure() << "Method descriptor mismatch:" << ::testing::PrintToString(actualDescriptor)
                                           << " expected: " << ::testing::PrintToString(descriptor);
    }

    return ::testing::AssertionSuccess();
  }

  std::unique_ptr<geevm::ClassFile> classFile;
};

TEST_F(ClassFileReaderTest, read_hello_world)
{
  this->readClassFile("class_file/org/geevm/tests/classfile/HelloWorld.class");
  ASSERT_NE(classFile, nullptr);

  EXPECT_EQ(classFile->minorVersion(), 0);
  EXPECT_EQ(classFile->majorVersion(), 61);
  EXPECT_EQ(classFile->accessFlags(), ClassAccessFlags::ACC_PUBLIC | ClassAccessFlags::ACC_SUPER);

  EXPECT_EQ(classFile->thisClass(), 21);
  EXPECT_EQ(classFile->constantPool().getClassName(classFile->thisClass()), u"org/geevm/tests/classfile/HelloWorld");

  EXPECT_EQ(classFile->superClass(), 2);
  EXPECT_EQ(classFile->constantPool().getClassName(classFile->superClass()), u"java/lang/Object");

  EXPECT_EQ(classFile->interfaces().size(), 0);
  EXPECT_EQ(classFile->fields().size(), 0);

  EXPECT_EQ(classFile->methods().size(), 2);

  auto& init = classFile->methods()[0];
  EXPECT_EQ(init.accessFlags(), MethodAccessFlags::ACC_PUBLIC);
  EXPECT_EQ(classFile->constantPool().getString(init.nameIndex()), u"<init>");
  EXPECT_EQ(classFile->constantPool().getString(init.descriptorIndex()), u"()V");

  auto& main = classFile->methods()[1];
  EXPECT_EQ(main.accessFlags(), MethodAccessFlags::ACC_PUBLIC | MethodAccessFlags::ACC_STATIC);
  EXPECT_EQ(classFile->constantPool().getString(main.nameIndex()), u"main");
  EXPECT_EQ(classFile->constantPool().getString(main.descriptorIndex()), u"([Ljava/lang/String;)V");

  ASSERT_TRUE(main.hasCode());
  EXPECT_EQ(main.code().maxStack(), 2);
  EXPECT_EQ(main.code().maxLocals(), 1);
  EXPECT_EQ(main.code().bytes(), std::vector<types::u1>({
                                     0xB2, 0x00, 0x07, // getstatic #7
                                     0x12, 0x0D,       // ldc #13
                                     0xB6, 0x00, 0x0F, // invokestatic #15
                                     0xB1              // return
                                 }));
  EXPECT_TRUE(main.code().exceptionTable().empty());
}

TEST_F(ClassFileReaderTest, read_fields)
{
  this->readClassFile("class_file/org/geevm/tests/classfile/Fields.class");

  EXPECT_EQ(classFile->minorVersion(), 0);
  EXPECT_EQ(classFile->majorVersion(), 61);
  EXPECT_EQ(classFile->accessFlags(), ClassAccessFlags::ACC_PUBLIC | ClassAccessFlags::ACC_SUPER);

  EXPECT_EQ(classFile->constantPool().getClassName(classFile->thisClass()), u"org/geevm/tests/classfile/Fields");
  EXPECT_EQ(classFile->constantPool().getClassName(classFile->superClass()), u"java/lang/Object");

  EXPECT_EQ(classFile->fields().size(), 18);

  checkField(0, u"charField", u"C", FieldAccessFlags::ACC_PRIVATE);
  checkField(1, u"byteField", u"B", FieldAccessFlags::ACC_PRIVATE);
  checkField(2, u"shortField", u"S", FieldAccessFlags::ACC_PRIVATE);
  checkField(3, u"intField", u"I", FieldAccessFlags::ACC_PRIVATE);
  checkField(4, u"longField", u"J", FieldAccessFlags::ACC_PRIVATE);
  checkField(5, u"floatField", u"F", FieldAccessFlags::ACC_PRIVATE);
  checkField(6, u"doubleField", u"D", FieldAccessFlags::ACC_PRIVATE);
  checkField(7, u"stringField", u"Ljava/lang/String;", FieldAccessFlags::ACC_PRIVATE);
  checkField(8, u"privField", u"I", FieldAccessFlags::ACC_PRIVATE);
  checkField(9, u"publicField", u"I", FieldAccessFlags::ACC_PUBLIC);
  checkField(10, u"protectedField", u"I", FieldAccessFlags::ACC_PROTECTED);
  checkField(11, u"internalField", u"I", static_cast<FieldAccessFlags>(0x00));
  checkField(12, u"privateStaticField", u"I", FieldAccessFlags::ACC_PRIVATE | FieldAccessFlags::ACC_STATIC);
  checkField(13, u"publicStaticField", u"I", FieldAccessFlags::ACC_PUBLIC | FieldAccessFlags::ACC_STATIC);
  checkField(14, u"privateStaticFinalField", u"I", FieldAccessFlags::ACC_PRIVATE | FieldAccessFlags::ACC_STATIC | FieldAccessFlags::ACC_FINAL);
  checkField(15, u"publicFinalField", u"I", FieldAccessFlags::ACC_PUBLIC | FieldAccessFlags::ACC_FINAL);
  checkField(16, u"publicVolatileField", u"I", FieldAccessFlags::ACC_PUBLIC | FieldAccessFlags::ACC_VOLATILE);
  checkField(17, u"publicTransientField", u"I", FieldAccessFlags::ACC_PUBLIC | FieldAccessFlags::ACC_TRANSIENT);
}

TEST_F(ClassFileReaderTest, read_inheritance)
{
  this->readClassFile("class_file/org/geevm/tests/classfile/Inheritance.class");

  EXPECT_EQ(classFile->minorVersion(), 0);
  EXPECT_EQ(classFile->majorVersion(), 61);
  EXPECT_EQ(classFile->accessFlags(), ClassAccessFlags::ACC_PUBLIC | ClassAccessFlags::ACC_SUPER);

  EXPECT_EQ(classFile->constantPool().getClassName(classFile->thisClass()), u"org/geevm/tests/classfile/Inheritance");
  EXPECT_EQ(classFile->constantPool().getClassName(classFile->superClass()), u"org/geevm/tests/classfile/Fields");

  EXPECT_EQ(classFile->interfaces().size(), 2);
  EXPECT_EQ(classFile->constantPool().getClassName(classFile->interfaces()[0]), u"java/lang/Comparable");
  EXPECT_EQ(classFile->constantPool().getClassName(classFile->interfaces()[1]), u"java/lang/Iterable");

  EXPECT_EQ(classFile->fields().size(), 1);
  checkField(0, u"x", u"Ljava/lang/String;", FieldAccessFlags::ACC_PRIVATE | FieldAccessFlags::ACC_FINAL);

  EXPECT_EQ(classFile->methods().size(), 4);
  checkMethod(0, u"<init>", u"(Ljava/lang/String;)V", MethodAccessFlags::ACC_PUBLIC);
  checkMethod(1, u"compareTo", u"(Ljava/lang/String;)I", MethodAccessFlags::ACC_PUBLIC);
  checkMethod(2, u"iterator", u"()Ljava/util/Iterator;", MethodAccessFlags::ACC_PUBLIC);
  checkMethod(3, u"compareTo", u"(Ljava/lang/Object;)I", MethodAccessFlags::ACC_PUBLIC | MethodAccessFlags::ACC_BRIDGE | MethodAccessFlags::ACC_SYNTHETIC);
}

TEST_F(ClassFileReaderTest, read_methods)
{
  this->readClassFile("class_file/org/geevm/tests/classfile/Methods.class");

  EXPECT_EQ(classFile->minorVersion(), 0);
  EXPECT_EQ(classFile->majorVersion(), 61);
  EXPECT_EQ(classFile->accessFlags(), ClassAccessFlags::ACC_PUBLIC | ClassAccessFlags::ACC_SUPER | ClassAccessFlags::ACC_ABSTRACT);

  EXPECT_EQ(classFile->constantPool().getClassName(classFile->thisClass()), u"org/geevm/tests/classfile/Methods");
  EXPECT_EQ(classFile->constantPool().getClassName(classFile->superClass()), u"java/lang/Object");

  EXPECT_EQ(classFile->methods().size(), 14);
  EXPECT_TRUE(checkMethod(0, u"<init>", u"()V", MethodAccessFlags::ACC_PUBLIC));
  EXPECT_TRUE(checkMethod(1, u"publicMethod", u"()V", MethodAccessFlags::ACC_PUBLIC));
  EXPECT_TRUE(checkMethod(2, u"protectedMethod", u"()V", MethodAccessFlags::ACC_PROTECTED));
  EXPECT_TRUE(checkMethod(3, u"privateMethod", u"()V", MethodAccessFlags::ACC_PRIVATE));
  EXPECT_TRUE(checkMethod(4, u"packageInternalMethod", u"()V", static_cast<MethodAccessFlags>(0x00)));
  EXPECT_TRUE(checkMethod(5, u"staticMethod", u"()V", MethodAccessFlags::ACC_PUBLIC | MethodAccessFlags::ACC_STATIC));
  EXPECT_TRUE(checkMethod(6, u"finalMethod", u"()V", MethodAccessFlags::ACC_PUBLIC | MethodAccessFlags::ACC_FINAL));
  EXPECT_TRUE(checkMethod(7, u"synchronizedMethod", u"()V", MethodAccessFlags::ACC_PUBLIC | MethodAccessFlags::ACC_SYNCHRONIZED));
  EXPECT_TRUE(checkMethod(8, u"simpleMethod", u"(I)I", MethodAccessFlags::ACC_PUBLIC));
  EXPECT_TRUE(checkMethod(9, u"varArgsMethod", u"([Ljava/lang/String;)Ljava/lang/String;", MethodAccessFlags::ACC_PUBLIC | MethodAccessFlags::ACC_VARARGS));
  EXPECT_TRUE(checkMethod(10, u"abstractMethod", u"()V", MethodAccessFlags::ACC_PUBLIC | MethodAccessFlags::ACC_ABSTRACT));
  EXPECT_TRUE(checkMethod(11, u"nativeMethod", u"()V", MethodAccessFlags::ACC_PUBLIC | MethodAccessFlags::ACC_NATIVE));
  EXPECT_TRUE(checkMethod(12, u"methodWithExceptions", u"()V", MethodAccessFlags::ACC_PUBLIC | MethodAccessFlags::ACC_ABSTRACT));
  EXPECT_TRUE(checkMethod(13, u"genericMethod", u"(Ljava/lang/Object;)Ljava/lang/Object;", MethodAccessFlags::ACC_PUBLIC));

  EXPECT_EQ(classFile->methods()[12].exceptions().size(), 2);
  EXPECT_EQ(classFile->constantPool().getClassName(classFile->methods()[12].exceptions()[0]), u"java/io/IOException");
  EXPECT_EQ(classFile->constantPool().getClassName(classFile->methods()[12].exceptions()[1]), u"org/geevm/tests/classfile/Methods$MyException");
}

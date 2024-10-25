#include "ClassFile.h"

#include <format>
#include <fstream>
#include <span>

using namespace geevm;

namespace
{

class ClassFileStream
{
public:
  explicit ClassFileStream(std::istream& stream)
    : mStream(stream)
  {
  }

  types::u1 readU1()
  {
    char result;
    mStream.read(&result, 1);

    return static_cast<types::u1>(result);
  }

  types::u2 readU2()
  {
    auto chunks = readArray<2>();
    return (chunks[0] << 8u) | chunks[1];
  }

  types::u4 readU4()
  {
    auto chunks = readArray<4>();
    return (chunks[0] << 24u) | (chunks[1] << 16u) | (chunks[2] << 8u) | chunks[3];
  }

  template<size_t N>
  std::array<types::u1, N> readArray()
  {
    std::array<types::u1, N> result;
    mStream.read(reinterpret_cast<char*>(result.data()), N);

    return result;
  }

  std::vector<types::u1> readVector(unsigned size)
  {
    std::vector<types::u1> result;

    unsigned i = 0;
    while (mStream && i < size) {
      char byte;
      mStream.read(&byte, 1);
      result.push_back(static_cast<types::u1>(byte));
      i += 1;
    }

    if (i != size) {
      throw ClassFileReadError("not enough bytes in the stream " + i);
    }
    // Check if stream has enough bytes
    // mStream.read(reinterpret_cast<char*>(result.data()), size);

    return result;
  }

private:
  std::istream& mStream;
};

class ClassFileReader
{
public:
  explicit ClassFileReader(ClassFileStream& stream)
    : mStream(stream)
  {
  }

  std::unique_ptr<ClassFile> parse()
  {
    if (types::u4 magic = mStream.readU4(); magic != 0xCAFEBABE) {
      throw ClassFileReadError("not a Java class file");
    }

    types::u2 minorVersion = mStream.readU2();
    types::u2 majorVersion = mStream.readU2();

    auto constantPool = this->readConstantPool();

    auto classAccessFlags = static_cast<ClassAccessFlags>(mStream.readU2());
    types::u2 thisClass = mStream.readU2();
    types::u2 superClass = mStream.readU2();

    // Read interfaces
    types::u2 interfacesCount = mStream.readU2();
    std::vector<types::u2> interfaces;
    for (types::u2 i = 0; i < interfacesCount; i++) {
      interfaces.push_back(mStream.readU2());
    }

    std::vector<FieldInfo> fields = this->readFields(*constantPool);
    std::vector<MethodInfo> methods = this->readMethods(*constantPool);

    return std::make_unique<ClassFile>(minorVersion, majorVersion, std::move(constantPool), classAccessFlags, thisClass, superClass, interfaces, fields,
                                       std::move(methods));
  }

  std::unique_ptr<ConstantPool> readConstantPool();

  std::vector<FieldInfo> readFields(const ConstantPool& constantPool);
  std::vector<MethodInfo> readMethods(const ConstantPool& constantPool);

private:
  ClassFileStream& mStream;
};

} // namespace

std::unique_ptr<ClassFile> ClassFile::fromFile(const std::string& path)
{
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return nullptr;
  }

  auto stream = ClassFileStream(file);

  return ClassFileReader(stream).parse();
}

struct MemoryBuffer : std::streambuf
{
  MemoryBuffer(char* begin, char* end)
  {
    this->setg(begin, begin, end);
  }
};

std::unique_ptr<ClassFile> ClassFile::fromBytes(char* bytes, size_t size)
{
  MemoryBuffer buffer(bytes, bytes + size);
  std::istream in{&buffer};
  ClassFileStream stream{in};

  return ClassFileReader(stream).parse();
}

std::unique_ptr<ConstantPool> ClassFileReader::readConstantPool()
{
  types::u2 count = mStream.readU2();

  std::vector<ConstantPool::Entry> entries;
  std::vector<types::JString> strings;
  entries.reserve(count);

  int i = 1;
  while (i < count) {
    auto tag = static_cast<ConstantPool::Tag>(mStream.readU1());
    auto entry = ConstantPool::Entry{tag};

    switch (tag) {
      using enum geevm::ConstantPool::Tag;
      case CONSTANT_Class: {
        types::u2 nameIndex = mStream.readU2();
        entry.data.classInfo.nameIndex = nameIndex;
        break;
      }
      case CONSTANT_Fieldref:
      case CONSTANT_Methodref:
      case CONSTANT_InterfaceMethodref: {
        types::u2 classIndex = mStream.readU2();
        types::u2 nameAndTypeIndex = mStream.readU2();
        entry.data.classAndNameRef.classIndex = classIndex;
        entry.data.classAndNameRef.nameAndTypeIndex = nameAndTypeIndex;
        break;
      }
      case CONSTANT_String: {
        types::u2 stringIndex = mStream.readU2();
        entry.data.stringInfo.stringIndex = stringIndex;
        break;
      }
      case CONSTANT_Integer: {
        types::u4 value = mStream.readU4();
        entry.data.singleInteger = std::bit_cast<int32_t>(value);
        break;
      }
      case CONSTANT_Float: {
        types::u4 value = mStream.readU4();
        entry.data.singleFloat = std::bit_cast<float>(value);
        break;
      }
      case CONSTANT_Long: {
        types::u4 high = mStream.readU4();
        types::u4 low = mStream.readU4();
        entry.data.doubleInteger = std::bit_cast<int64_t>((static_cast<types::u8>(high) << 32u) | low);
        break;
      }
      case CONSTANT_Double: {
        types::u4 high = mStream.readU4();
        types::u4 low = mStream.readU4();
        entry.data.doubleFloat = std::bit_cast<double>((static_cast<types::u8>(high) << 32u) | low);
        break;
      }
      case CONSTANT_NameAndType: {
        entry.data.nameAndType.nameIndex = mStream.readU2();
        entry.data.nameAndType.descriptorIndex = mStream.readU2();
        break;
      }
      case CONSTANT_Utf8: {
        types::u2 length = mStream.readU2();
        auto bytes = mStream.readVector(length);
        types::JString utf8String(bytes.begin(), bytes.end());

        strings.push_back(utf8String);
        entry.data.utf8String = static_cast<types::u2>(strings.size() - 1);

        break;
      }
      case CONSTANT_MethodHandle: {
        // TODO(sallaigy): Create a dedicated enum
        entry.data.methodHandle.referenceKind = mStream.readU1();
        entry.data.methodHandle.referenceIndex = mStream.readU2();
        break;
      }
      case CONSTANT_InvokeDynamic: {
        entry.data.invokeDynamicInfo.bootstrapAddrIndex = mStream.readU2();
        entry.data.invokeDynamicInfo.nameAndTypeIndex = mStream.readU2();
        break;
      }
      case CONSTANT_MethodType: {
        entry.data.methodType.descriptorIndex = mStream.readU2();
        break;
      }
      default: throw ClassFileReadError(std::format("Invalid constant pool tag at position {}", i));
    }

    entries.push_back(entry);
    i += 1;
    if (tag == ConstantPool::Tag::CONSTANT_Long || tag == ConstantPool::Tag::CONSTANT_Double) {
      // Long and double entries take two slots in the constant pool
      entries.push_back(ConstantPool::Entry{ConstantPool::Tag::Empty});
      i += 1;
    }
  }

  return std::make_unique<ConstantPool>(entries, strings);
}

std::vector<FieldInfo> ClassFileReader::readFields(const ConstantPool& constantPool)
{
  types::u2 fieldsCount = mStream.readU2();
  std::vector<FieldInfo> fields;

  for (types::u2 i = 0; i < fieldsCount; ++i) {
    auto accessFlags = static_cast<FieldAccessFlags>(mStream.readU2());
    types::u2 nameIndex = mStream.readU2();
    types::u2 descriptorIndex = mStream.readU2();
    std::optional<types::u2> constantValueIndex;

    types::u2 attributesCount = mStream.readU2();

    for (types::u2 j = 0; j < attributesCount; ++j) {
      types::u2 attrNameIndex = mStream.readU2();
      types::u4 attributeLength = mStream.readU4();
      auto attrName = constantPool.getString(attrNameIndex);

      if (attrName == u"ConstantValue") {
        constantValueIndex = mStream.readU2();
      } else {
        // Skip other attributes
        mStream.readVector(attributeLength);
      }
    }

    fields.emplace_back(accessFlags, nameIndex, descriptorIndex, constantValueIndex);
  }

  return fields;
}

std::vector<MethodInfo> ClassFileReader::readMethods(const ConstantPool& constantPool)
{
  types::u2 methodsCount = mStream.readU2();
  std::vector<MethodInfo> methods;

  for (types::u2 i = 0; i < methodsCount; ++i) {
    auto accessFlags = static_cast<MethodAccessFlags>(mStream.readU2());
    types::u2 nameIndex = mStream.readU2();
    types::u2 descriptorIndex = mStream.readU2();
    types::u2 attributesCount = mStream.readU2();

    std::optional<Code> code = std::nullopt;
    std::vector<types::u2> exceptions;

    for (types::u2 j = 0; j < attributesCount; ++j) {
      types::u2 attrNameIndex = mStream.readU2();
      types::u4 attributeLength = mStream.readU4();
      auto attrName = constantPool.getString(attrNameIndex);

      if (attrName == u"Code") {
        types::u2 maxStack = mStream.readU2();
        types::u2 maxLocals = mStream.readU2();

        types::u4 codeLength = mStream.readU4();
        std::vector<types::u1> bytes = mStream.readVector(codeLength);

        types::u2 exceptionTableLength = mStream.readU2();
        std::vector<Code::ExceptionTableEntry> exceptionTable;
        for (types::u2 j = 0; j < exceptionTableLength; ++j) {
          types::u2 startPc = mStream.readU2();
          types::u2 endPc = mStream.readU2();
          types::u2 handlerPc = mStream.readU2();
          types::u2 catchType = mStream.readU2();
          exceptionTable.emplace_back(startPc, endPc, handlerPc, catchType);
        }

        // Code attributes
        types::u2 codeAttributesCount = mStream.readU2();
        for (types::u2 j = 0; j < codeAttributesCount; ++j) {
          types::u2 codeAttrNameIndex = mStream.readU2();
          types::u2 codeAttrLength = mStream.readU4();
          mStream.readVector(codeAttrLength);
        }

        code.emplace(maxStack, maxLocals, bytes, exceptionTable, std::vector<Code::LocalVariableTableEntry>{}, std::vector<Code::LocalVariableTableEntry>{});
      } else if (attrName == u"Exceptions") {
        types::u2 numberOfExceptions = mStream.readU2();
        for (types::u2 j = 0; j < numberOfExceptions; ++j) {
          exceptions.push_back(mStream.readU2());
        }
      } else {
        // Skip unknown attributes
        mStream.readVector(attributeLength);
      }
    }

    methods.emplace_back(accessFlags, nameIndex, descriptorIndex, std::move(code), exceptions);
  }

  return methods;
}

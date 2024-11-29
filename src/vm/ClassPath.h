#ifndef GEEVM_VM_CLASSPATH_H
#define GEEVM_VM_CLASSPATH_H

#include "common/JvmError.h"
#include "common/Zip.h"

#include <optional>
#include <variant>
#include <vector>

namespace geevm
{

class InstanceClass;

class ClassLocation
{
  using StorageTy = std::variant<std::string, std::pair<ZipArchive*, std::string>>;

  explicit ClassLocation(StorageTy storage)
    : mStorage(std::move(storage))
  {
  }

public:
  static ClassLocation createFileLocation(std::string fileName);
  static ClassLocation createJarLocation(ZipArchive* archive, const std::string& fileName);

  JvmExpected<std::unique_ptr<InstanceClass>> resolve();

private:
  StorageTy mStorage;
};

class ClassPath
{
public:
  class Entry
  {
    using StorageTy = std::variant<std::string, std::unique_ptr<ZipArchive>>;

  public:
    explicit Entry(StorageTy storage)
      : mStorage(std::move(storage))
    {
    }

    std::optional<ClassLocation> search(const types::JString& name);

    std::optional<std::string> asDirectory();
    std::optional<ZipArchive*> asJar();

  private:
    StorageTy mStorage;
  };

  void addDirectory(const std::string& path);
  void addJar(const std::string& path);

  std::optional<ClassLocation> search(const types::JString& name);

private:
  std::vector<Entry> mEntries;
};

} // namespace geevm

#endif // GEEVM_VM_CLASSPATH_H

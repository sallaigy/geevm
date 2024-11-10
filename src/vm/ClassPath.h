#ifndef GEEVM_VM_CLASSPATH_H
#define GEEVM_VM_CLASSPATH_H

#include "common/Zip.h"

#include <vector>

namespace geevm
{

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
    std::optional<ClassLocation> search(const std::string& name);

    bool isDirectory() const;
    bool isJar() const;

  private:
    StorageTy mStorage;
  };

private:
  std::vector<Entry> mEntries;
};

} // namespace geevm

#endif // GEEVM_VM_CLASSPATH_H

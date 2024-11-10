#ifndef GEEVM_COMMON_ZIP_H
#define GEEVM_COMMON_ZIP_H

#include <memory>
#include <string>

namespace geevm
{

class ZipArchive
{
public:
  static std::unique_ptr<ZipArchive> open(const std::string& name);

  ZipArchive& operator=(const ZipArchive&) = delete;

  /// Looks up the given file name in the archive, and puts its contents into \p buffer.
  /// The buffer is allocated inside the function call, and is up to the caller to free it.
  /// \returns true if the read is successful, false otherwise.
  virtual bool readAsBinary(const std::string& fileName, char** buffer, size_t* size) = 0;

  virtual ~ZipArchive() = default;
};

} // namespace geevm

#endif // GEEVM_COMMON_ZIP_H

#include "common/Zip.h"

#include <cassert>
#include <zip.h>

using namespace geevm;

namespace
{

class ZipArchiveImpl : public ZipArchive
{
public:
  explicit ZipArchiveImpl(zip_t* zip)
    : mZip(zip)
  {
    assert(mZip != nullptr);
  }

  ZipArchiveImpl(const ZipArchiveImpl&) = delete;
  ZipArchiveImpl& operator=(const ZipArchiveImpl&) = delete;

  bool readAsBinary(const std::string& fileName, char** buffer, size_t* size) override;

  bool containsFile(const std::string& fileName) override;

  ~ZipArchiveImpl() override;

private:
  zip_t* mZip;
};

} // namespace

std::unique_ptr<ZipArchive> ZipArchive::open(const std::string& name)
{
  int ec;
  zip_t* archive = zip_open(name.c_str(), ZIP_RDONLY, &ec);
  if (archive == nullptr) {
    return nullptr;
  }

  return std::make_unique<ZipArchiveImpl>(archive);
}

bool ZipArchiveImpl::containsFile(const std::string& fileName)
{
  int64_t fileIndex = zip_name_locate(mZip, fileName.c_str(), ZIP_FL_ENC_GUESS);
  if (fileIndex == -1) {
    return false;
  }

  return true;
}

bool ZipArchiveImpl::readAsBinary(const std::string& fileName, char** buffer, size_t* size)
{
  int64_t fileIndex = zip_name_locate(mZip, fileName.c_str(), ZIP_FL_ENC_GUESS);
  if (fileIndex == -1) {
    return false;
  }

  zip_stat_t stat;
  if (zip_stat_index(mZip, fileIndex, ZIP_STAT_SIZE, &stat) != 0) {
    return false;
  }

  zip_file_t* zipFile = zip_fopen_index(mZip, fileIndex, 0);
  if (zipFile == nullptr) {
    return false;
  }

  *buffer = new char[stat.size];
  *size = stat.size;
  if (zip_fread(zipFile, *buffer, stat.size) == -1) {
    zip_fclose(zipFile);
    delete[] *buffer;
    return false;
  }

  return true;
}

ZipArchiveImpl::~ZipArchiveImpl()
{
  assert(mZip != nullptr);
  zip_close(mZip);
}

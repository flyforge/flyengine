#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/UniquePtr.h>

#include <Foundation/Containers/IdTable.h>
#include <RmlUi/Core/FileInterface.h>

namespace plRmlUiInternal
{
  struct FileId : public plGenericId<24, 8>
  {
    using plGenericId::plGenericId;

    static FileId FromRml(Rml::FileHandle hFile) { return FileId(static_cast<plUInt32>(hFile)); }

    Rml::FileHandle ToRml() const { return m_Data; }
  };

  //////////////////////////////////////////////////////////////////////////

  class FileInterface final : public Rml::FileInterface
  {
  public:
    FileInterface();
    virtual ~FileInterface();

    virtual Rml::FileHandle Open(const Rml::String& path) override;
    virtual void Close(Rml::FileHandle file) override;

    virtual size_t Read(void* buffer, size_t size, Rml::FileHandle file) override;

    virtual bool Seek(Rml::FileHandle file, long offset, int origin) override;
    virtual size_t Tell(Rml::FileHandle file) override;

    virtual size_t Length(Rml::FileHandle file) override;

  private:
    struct OpenFile
    {
      plDefaultMemoryStreamStorage m_Storage;
      plMemoryStreamReader m_Reader;
    };

    plIdTable<FileId, plUniquePtr<OpenFile>> m_OpenFiles;
  };
} // namespace plRmlUiInternal

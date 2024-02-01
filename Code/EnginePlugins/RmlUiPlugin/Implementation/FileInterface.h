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

    virtual Rml::FileHandle Open(const Rml::String& sPath) override;
    virtual void Close(Rml::FileHandle hFile) override;

    virtual size_t Read(void* pBuffer, size_t uiSize, Rml::FileHandle hFile) override;

    virtual bool Seek(Rml::FileHandle hFile, long iOffset, int iOrigin) override;
    virtual size_t Tell(Rml::FileHandle hFile) override;

    virtual size_t Length(Rml::FileHandle hFile) override;

  private:
    struct OpenFile
    {
      plDefaultMemoryStreamStorage m_Storage;
      plMemoryStreamReader m_Reader;
    };

    plIdTable<FileId, plUniquePtr<OpenFile>> m_OpenFiles;
  };
} // namespace plRmlUiInternal

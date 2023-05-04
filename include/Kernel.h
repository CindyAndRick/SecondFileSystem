#ifndef KERNEL_H
#define KERNEL_H

#include <string>

#include "Img.h"
#include "FileManager.h"
#include "FileSystem.h"
#include "BufferManager.h"
#include "User.h"
// #include "UserManager.h"

#define SYS_SEEK_SET 0 // 文件开头位置
#define SYS_SEEK_CUR 1 // 当前位置
#define SYS_SEEK_END 2 // 文件结尾位置

// 文件描述符类型
typedef unsigned int FD;

class Kernel
{
public:
    Kernel();
    ~Kernel();

// 工具函数
    void Initialize();
    void Quit();
    static Kernel& Instance();

// 子组件
    Img& GetImg();
    BufferManager& GetBufferManager();
    FileSystem& GetFileSystem();
    FileManager& GetFileManager();
    User& GetUser();
    // UserManager& GetUserManager();

// 文件系统API
    FD Sys_Open(std::string& fpath, int mode=File::FWRITE);
    int Sys_Close(FD fd);
    int Sys_Mkdir(std::string& fpath);
    int Sys_Creat(std::string& fpath, int mode=File::FWRITE);
    int Sys_Delete(std::string& fpath);
    int Sys_Read(FD fd, size_t size, size_t nmemb, void* ptr);
    int Sys_Write(FD fd, size_t size, size_t nmemb, void* ptr);
    int Sys_Seek(FD fd, long int offset, int whence); // whence: 0:文件开头 1:当前位置 2:文件结尾

private:
// 子组件初始化函数
    void InitImg();
    void InitFileSystem();
    void InitBuffer();
    void InitUser();

    static Kernel instance; // 单体实例

// 子组件指针
    Img* m_Img;
    BufferManager* m_BufferManager;
    FileSystem* m_FileSystem;
    FileManager* m_FileManager;
    User* m_User;
    // UserManager* m_UserManager;
};

#endif
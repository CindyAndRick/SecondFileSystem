#include <string.h>

#include "../include/Kernel.h"

// ======================= 全局单体实例 ======================= //

Kernel Kernel::instance;

Img g_Img;
BufferManager g_BufferManager;
FileSystem g_FileSystem;
FileManager g_FileManager;
User g_User;
// UserManager g_UserManager;

// ======================= 全局单体实例 ======================= //

// ======================= Kernel ======================= //

Kernel::Kernel()
{
    // nothing to do here
}

Kernel::~Kernel()
{
    // nothing to do here
}

Kernel &Kernel::Instance()
{
    return Kernel::instance;
}

// ======================= Kernel ======================= //

// ======================= 子组件初始化函数 ======================= //

void Kernel::InitImg()
{
    this->m_Img = &g_Img;
    this->m_Img->Initialize();
}

void Kernel::InitFileSystem()
{
    this->m_FileSystem = &g_FileSystem;
    this->m_FileSystem->Initialize();
    this->m_FileManager = &g_FileManager;
    this->m_FileManager->Initialize();
}

void Kernel::InitBuffer()
{
    this->m_BufferManager = &g_BufferManager;
    this->m_BufferManager->Initialize();
}

void Kernel::InitUser()
{
    this->m_User = &g_User;
    // this->m_UserManager = &g_UserManager;
}

// ======================= 子组件初始化函数 ======================= //

// ======================= 工具函数 ======================= //

void Kernel::Initialize()
{
    this->InitBuffer();
    this->InitImg();
    this->InitFileSystem();
    this->InitUser();

    FileManager &fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.rootDirInode = g_InodeTable.IGet(FileSystem::ROOTINO);
    fileMgr.rootDirInode->i_flag &= (~Inode::ILOCK);

    Kernel::Instance().GetFileSystem().LoadSuperBlock();
    User &us = Kernel::Instance().GetUser();
    us.u_cdir = g_InodeTable.IGet(FileSystem::ROOTINO);
    us.u_cdir->i_flag &= (~Inode::ILOCK);
    strcpy(us.u_curdir, "/");

    printf("[info] Init Kernel info: SecondFIleSystem Init Finish.\n");
}

void Kernel::Quit()
{
    this->m_BufferManager->Bflush();
    this->m_FileManager->m_InodeTable->UpdateInodeTable();
    this->m_FileSystem->Update();
    this->m_Img->Quit();
}

// ======================= 工具函数 ======================= //

// ======================= 子组件获取函数 ======================= //

Img &Kernel::GetImg()
{
    return *(this->m_Img);
}

BufferManager &Kernel::GetBufferManager()
{
    return *(this->m_BufferManager);
}

FileSystem &Kernel::GetFileSystem()
{
    return *(this->m_FileSystem);
}

FileManager &Kernel::GetFileManager()
{
    return *(this->m_FileManager);
}

User &Kernel::GetUser()
{
    return *(this->m_User);
}

// UserManager& Kernel::GetUserManager()
// {
//     return *(this->m_UserManager);
// }

// ======================= 子组件获取函数 ======================= //

// ======================= 文件系统API ======================= //

FD Kernel::Sys_Open(std::string &fpath, int mode)
{
    // 模仿系统调用，将参数放入user结构中
    User &u = Kernel::Instance().GetUser();
    char path[256];
    strcpy(path, fpath.c_str());
    u.u_dirp = path;
    // u.u_arg[0] = (int)path;
    u.u_arg[1] = mode;

    FileManager &fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.Open();

    // 从user结构取出返回值
    return u.u_ar0[User::EAX];
}

int Kernel::Sys_Close(FD fd)
{
    User &u = Kernel::Instance().GetUser();
    u.u_arg[0] = fd;

    FileManager &fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.Close();

    return u.u_ar0[User::EAX];
}

int Kernel::Sys_Mkdir(std::string &fpath)
{
    int default_mode = 040755;
    User &u = Kernel::Instance().GetUser();
    u.u_error = NOERROR;
    char filename_char[300] = {0};
    strcpy(filename_char, fpath.c_str());
    u.u_dirp = filename_char;
    u.u_arg[1] = default_mode;
    u.u_arg[2] = 0;
    FileManager &fimanag = Kernel::Instance().GetFileManager();
    fimanag.MkNod();
    return u.u_ar0[User::EAX];
}

int Kernel::Sys_Creat(std::string &fpath, int mode)
{
    // 模仿系统调用，将参数放入user结构中
    User &u = Kernel::Instance().GetUser();
    char path[256];
    strcpy(path, fpath.c_str());
    u.u_dirp = path;
    u.u_arg[0] = (long long)&path;
    u.u_arg[1] = mode;

    FileManager &fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.Creat();

    // 从user结构取出返回值
    return u.u_ar0[User::EAX];
}

int Kernel::Sys_Delete(std::string &fpath)
{
    // 模仿系统调用，将参数放入user结构中
    User &u = Kernel::Instance().GetUser();
    char path[256];
    strcpy(path, fpath.c_str());
    u.u_dirp = path;
    u.u_arg[0] = (long long)&path;

    FileManager &fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.UnLink();

    // 从user结构取出返回值
    return u.u_ar0[User::EAX];
}

int Kernel::Sys_Read(FD fd, size_t size, size_t nmemb, void *ptr)
{
    if (size > nmemb)
        return -1;
    // 模仿系统调用，将参数放入user结构中
    User &u = Kernel::Instance().GetUser();

    u.u_arg[0] = fd;
    u.u_arg[1] = (long long)ptr;
    u.u_arg[2] = size;

    FileManager &fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.Read();

    // 从user结构取出返回值
    return u.u_ar0[User::EAX];
}

int Kernel::Sys_Write(FD fd, size_t size, size_t nmemb, void *ptr)
{
    if (size > nmemb)
        return -1;
    // 模仿系统调用，将参数放入user结构中
    User &u = Kernel::Instance().GetUser();

    u.u_arg[0] = fd;
    u.u_arg[1] = (unsigned long long)ptr;
    u.u_arg[2] = size;

    FileManager &fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.Write();

    // 从user结构取出返回值
    return u.u_ar0[User::EAX];
}

int Kernel::Sys_Seek(FD fd, long int offset, int whence)
{
    // 模仿系统调用，将参数放入user结构中
    User &u = Kernel::Instance().GetUser();

    u.u_arg[0] = fd;
    u.u_arg[1] = offset;
    u.u_arg[2] = whence;

    FileManager &fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.Seek();

    // 从user结构取出返回值
    return u.u_ar0[User::EAX];
}

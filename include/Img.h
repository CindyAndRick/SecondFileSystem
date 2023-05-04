#ifndef IMG_H
#define IMG_H
#include "FileSystem.h"
#include "BufferManager.h"

/* 为了更便利地使用mmap因而构建img类 */
class Img
{
public:
    Img();
    ~Img();

    void Initialize();
    void Quit();

private:
    void InitImg();
    void ReadImg();
    void InitSb(SuperBlock* sbp);
    void InitDb(char* db);
private:
    const char* devpath = "c.img";
    int fd_img; // dev打开后的文件号
    BufferManager *m_BufferManager; // 用于保存指针
};

#endif
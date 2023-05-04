#include <string.h>
#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "../include/Img.h"
#include "../include/Kernel.h"

Img::Img()
{
    // nothing to do here
}

Img::~Img()
{
    // nothing to do here
}

void Img::Initialize()
{
    printf("[info] Img Init Info: 初始化img\n");
    // 取得BufferManger
    this->m_BufferManager = &Kernel::Instance().GetBufferManager();

    /* 分两种情况:若存在c.img,说明并非第一次打开,故调用ReadImg,若不存在c.img,则创建c.img并对其初始化 */
    int fd = open(devpath, O_RDWR);
    if (fd == -1)
    {
        fd = open(devpath, O_RDWR | O_CREAT, 0666);
        if (fd == -1)
        {
            printf("[error] Img Init Error: 创建 %s 失败\n", devpath);
            exit(-1);
        }
        printf("[info] Img Init Info: 创建 %s 成功\n", devpath);
        this->fd_img = fd;
        // 对磁盘进行初始化
        this->InitImg();
    }
    else
    {
        this->fd_img = fd;
        this->ReadImg();
    }
}

void Img::ReadImg()
{
    /* ReadImg的本质为建立映射,但出于安全性考虑,先检验文件大小是否正确 */
    struct stat st;
    fstat(this->fd_img, &st);
    printf("size of c.img:%ld", st.st_size);

    char *p = (char *)mmap(NULL, sizeof(SuperBlock) + sizeof(DiskInode) * FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR + FileSystem::DATA_ZONE_SIZE * 512, PROT_READ | PROT_WRITE, MAP_SHARED, fd_img, 0);

    /* 让BufferManager拥有映射指针 */
    this->m_BufferManager->SetP(p);

    return;
}

void Img::Quit()
{
    char *p = this->m_BufferManager->GetP();
    // 使用msync将mmap过程中的改动全部写回
    msync((void *)p, sizeof(SuperBlock) + sizeof(DiskInode) * FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR + FileSystem::DATA_ZONE_SIZE * 512, MS_SYNC);
    // 解除映射
    munmap(p, sizeof(SuperBlock) + sizeof(DiskInode) * FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR + FileSystem::DATA_ZONE_SIZE * 512);
}

void Img::InitImg()
{
    // printf("[info] Img Init Info: 开始初始化 %s\n", devpath);

    /* 构造写入结构 */
    /* 超级块 */
    // 必须要new,否则指针指向null，后续修改会报segement fault
    SuperBlock *sbp = new SuperBlock;
    this->InitSb(sbp);

    /* di_table */
    /* 设置rootDiskInode的初始值 */
    DiskInode *di_table = new DiskInode[FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR];
    di_table[0].d_mode = Inode::IFDIR;  // 目录文件
    di_table[0].d_mode |= Inode::IEXEC; // 可执行

    /* db */
    char *db = new char[FileSystem::DATA_ZONE_SIZE * 512];
    memset(db, 0, FileSystem::DATA_ZONE_SIZE * 512);
    this->InitDb(db);

    // printf("[info] Img Init Info: 开始映射 %s\n", devpath);
    /* 修改文件大小,否则mmap超出文件大小会报segement fault */
    ftruncate(fd_img, ((sizeof(SuperBlock) + sizeof(DiskInode) * FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR + FileSystem::DATA_ZONE_SIZE * 512) / 4096 + 1) * 4096);
    /* mmap建立映射 */
    char *p = (char *)mmap(NULL, sizeof(SuperBlock) + sizeof(DiskInode) * FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR + FileSystem::DATA_ZONE_SIZE * 512, PROT_READ | PROT_WRITE, MAP_SHARED, fd_img, 0);

    // printf("[info] Img Init Info: 开始写入 %s\n", devpath);
    /* 拷贝 */
    memcpy(p, sbp, sizeof(SuperBlock));
    memcpy(p + sizeof(SuperBlock), di_table, sizeof(DiskInode) * FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR);
    memcpy(p + sizeof(SuperBlock) + sizeof(DiskInode) * FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR, db, FileSystem::DATA_ZONE_SIZE * 512);

    /* 删除动态申请内存 */
    delete sbp;
    delete[] di_table;
    delete[] db;

    /* 让BufferManager拥有映射指针 */
    this->m_BufferManager->SetP(p);

    return;
}

void Img::InitSb(SuperBlock *sbp)
{
    sbp->s_isize = FileSystem::INODE_ZONE_SIZE;
    sbp->s_fsize = FileSystem::DATA_ZONE_END_SECTOR + 1;
    /* 第一组99块，其余每组100块，余下部分被超级块直接管理 */
    sbp->s_nfree = (FileSystem::DATA_ZONE_SIZE - 99) % 100;

    /* 从倒数第s_nfree个盘块装入s_free中 */
    /* 获取偏置 */
    int offset = FileSystem::DATA_ZONE_START_SECTOR;
    while (offset + 99 < FileSystem::DATA_ZONE_END_SECTOR)
        offset += 100;
    offset--;
    /* 装入超级块 */
    for (int i = 0; i < sbp->s_nfree; ++i)
        sbp->s_free[i] = offset + i;

    sbp->s_ninode = 100;
    for (int i = 0; i < sbp->s_ninode; ++i)
        sbp->s_inode[i] = i;

    // sbp->s_flock = 0;
    // sbp->s_ilock = 0;
    sbp->s_fmod = 0;
    sbp->s_ronly = 0;

    return;
}

void Img::InitDb(char *db)
{
    struct
    {
        int nfree;
        int free[100];
    } tmp_table;

    /* 未加入索引的盘块数量 */
    int num = FileSystem::DATA_ZONE_SIZE;
    /* 初始化组长盘块 */
    for (int i = 0;; ++i)
    {
        if (num >= 100)
            tmp_table.nfree = 100;
        else
            // 不满100,说明直接被SuperBlock管理
            break;
        num -= tmp_table.nfree;
        for (int j = 0; j < tmp_table.nfree; ++j)
        {
            if (i == 0 && j == 0)
                tmp_table.free[j] = 0;
            else
                tmp_table.free[j] = i * 100 + j + FileSystem::DATA_ZONE_START_SECTOR - 1;
            // printf("i:%d,j:%d,tmp_table.free[j]:%d\n", i, j, tmp_table.free[j]);
        }
        // printf("i:%d,tmp_table.nfree:%d\n", i, tmp_table.nfree);
        // 99 * 512 是因为第一组的空闲块索引表被放在第二组第一块中
        memcpy(&db[99 * 512 + i * 100 * 512], (void *)&tmp_table, sizeof(tmp_table));
        if (num <= 0)
            break;
    }

    return;
}
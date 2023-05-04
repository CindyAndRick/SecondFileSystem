#include <iostream>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <sstream>

#include "../include/Kernel.h"

using std::cin;
using std::cout;
using std::endl;
using std::string;

bool isNumber(const string &str)
{
    for (char const &c : str)
    {
        if (std::isdigit(c) == 0)
            return false;
    }
    return true;
}

std::stringstream print_head()
{
    std::stringstream send_str;
    send_str << "===============================================" << endl;
    send_str << "||请在一行中依次输入需要调用的函数名称及其参数||" << endl;
    send_str << "||open(char *name, int mode)                 ||" << endl;
    send_str << "||close(int fd)                              ||" << endl;
    send_str << "||read(int fd, int length)                   ||" << endl;
    send_str << "||write(int fd, char *buffer, int length)    ||" << endl;
    send_str << "||seek(int fd, int position, int ptrname)    ||" << endl;
    send_str << "||mkfile(char *name, int mode=0777)          ||" << endl;
    send_str << "||rm(char *name)                             ||" << endl;
    send_str << "||ls()                                       ||" << endl;
    send_str << "||mkdir(char* dirname)                       ||" << endl;
    send_str << "||cd(char* dirname)                          ||" << endl;
    send_str << "||cat(char* dirname)                         ||" << endl;
    send_str << "||copyin(char* ofpath, char *  ifpath)       ||" << endl;
    send_str << "||copyout(char* ifpath, char *  ofpath)      ||" << endl;
    send_str << "||q/Q 退出文件系统                           ||" << endl;
    return send_str;
}

void start_routine()
{
    cout << print_head().str();

    string username = "test";

    string tipswords;

    while (true)
    {
        tipswords = "||SecondFileSystem@" + username + ":" + Kernel::Instance().GetUser().u_curdir + "$ ";
        cout << tipswords;
        string buf_recv;

        getline(cin, buf_recv);

        // 解析命令名称
        std::stringstream ss(buf_recv);
        // cout<<"buf_recv : "<< buf_recv << endl;
        string api;
        ss >> api;
        std::stringstream send_str;

        // cout<<"api : "<< api << endl;
        if (api == "cd")
        {
            string param1;
            ss >> param1;
            if (param1 == "")
            {
                send_str << "cd [fpath]";
                send_str << "参数个数错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            // 调用
            User &u = Kernel::Instance().GetUser();
            u.u_error = NOERROR;
            char dirname[300] = {0};
            strcpy(dirname, param1.c_str());
            u.u_dirp = dirname;
            u.u_arg[0] = (unsigned long long)(dirname);
            FileManager &fimanag = Kernel::Instance().GetFileManager();
            fimanag.ChDir();
            // 打印结果
            // send_str << "[result]:\n"
            //          << "now dir=" << u.u_curdir << endl;
            // cout << send_str.str() << endl;
            continue;
        }
        if (api == "ls")
        {
            User &u = Kernel::Instance().GetUser();
            u.u_error = NOERROR;
            string cur_path = u.u_curdir;
            FD fd = Kernel::Instance().Sys_Open(cur_path, (File::FREAD));
            // send_str << " cur_path:" << cur_path << endl;
            char buf[33] = {0};
            while (1)
            {
                if (Kernel::Instance().Sys_Read(fd, 32, 33, buf) == 0)
                    break;
                else
                {
                    // send_str << "cur_path:" << cur_path << endl << "buf:" << buf;
                    DirectoryEntry *mm = (DirectoryEntry *)buf;
                    if (mm->m_ino == 0)
                        continue;
                    send_str << mm->m_name << " ";
                    memset(buf, 0, 32);
                }
            }
            Kernel::Instance().Sys_Close(fd);
            cout << send_str.str() << endl;
            continue;
        }
        if (api == "mkdir")
        {
            string path;
            ss >> path;
            if (path == "")
            {
                send_str << "mkdir [dirpath]";
                send_str << "参数个数错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            int ret = Kernel::Instance().Sys_Mkdir(path);
            // send_str << "mkdir success (ret=" << ret << ")" << endl;
            // cout << send_str.str() << endl;
            continue;
        }
        if (api == "mkfile")
        {
            string filename;
            ss >> filename;
            if (filename == "")
            {
                send_str << "mkfile [filepath]";
                send_str << "参数个数错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            User &u = Kernel::Instance().GetUser();
            u.u_error = NOERROR;
            u.u_ar0[0] = 0;
            u.u_ar0[1] = 0;
            char filename_char[512];
            strcpy(filename_char, filename.c_str());
            u.u_dirp = filename_char;
            u.u_arg[1] = Inode::IRWXU;
            FileManager &fimanag = Kernel::Instance().GetFileManager();
            fimanag.Creat();
            // send_str << "mkfile sucess" << endl;
            // cout << send_str.str() << endl;
            continue;
        }
        if (api == "rm")
        {
            string filename;
            ss >> filename;
            if (filename == "")
            {
                send_str << "rm [filepath]";
                send_str << "参数个数错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            User &u = Kernel::Instance().GetUser();
            u.u_error = NOERROR;
            u.u_ar0[0] = 0;
            u.u_ar0[1] = 0;
            char filename_char[512];
            strcpy(filename_char, filename.c_str());
            u.u_dirp = filename_char;
            FileManager &fimanag = Kernel::Instance().GetFileManager();
            fimanag.UnLink();
            continue;
        }
        if (api == "seek")
        {
            string fd, position, ptrname;
            ss >> fd >> position >> ptrname;
            if (fd == "" || position == "" || ptrname == "")
            {
                send_str << "seek [fd] [position] [ptrname]";
                send_str << "参数个数错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            if (!isNumber(fd))
            {
                send_str << "[error] 参数fd错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            int fd_int = atoi(fd.c_str());
            if (!isNumber(position))
            {
                send_str << "[error] 参数position错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            int position_int = atoi(position.c_str());
            if (!isNumber(ptrname))
            {
                send_str << "[error] 参数ptrname错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            int ptrname_int = atoi(ptrname.c_str());
            User &u = Kernel::Instance().GetUser();
            u.u_error = NOERROR;
            u.u_ar0[0] = 0;
            u.u_ar0[1] = 0;
            u.u_arg[0] = fd_int;
            u.u_arg[1] = position_int;
            u.u_arg[2] = ptrname_int;
            FileManager &fimanag = Kernel::Instance().GetFileManager();
            fimanag.Seek();
            send_str << "[result] u.u_ar0=" << u.u_ar0 << endl;
            cout << send_str.str();
            continue;
        }
        if (api == "open")
        {
            string param1;
            string param2;
            ss >> param1 >> param2;
            if (param1 == "")
            {
                send_str << "open [fpath] [mode]\n";
                send_str << "参数个数错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            string fpath = param1;
            if (!isNumber(param2))
            {
                send_str << "[error] 参数mode错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            int mode;
            if (param2 == "")
            {
                mode = 0777;
            }
            else
            {
                int tmp = atoi(param2.c_str());
                // 进制转换
                mode = tmp / 100 * 64 + tmp % 100 / 10 * 8 + tmp % 10;
            }
            // 调用
            FD fd = Kernel::Instance().Sys_Open(fpath, mode);
            // 打印结果
            send_str << "[result] fd=" << fd << endl;
            cout << send_str.str();
            continue;
        }
        if (api == "read")
        {
            string p1_fd;
            string p2_size;
            ss >> p1_fd >> p2_size;
            if (p1_fd == "" || p2_size == "")
            {
                send_str << "read [fd] [size]\n";
                send_str << "参数个数错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            if (!isNumber(p1_fd))
            {
                send_str << "[error] 参数fd错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            if (!isNumber(p2_size))
            {
                send_str << "[error] 参数size错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            int fd = atoi(p1_fd.c_str());
            if (fd < 0)
            {
                send_str << "[error] 参数fd应当为正整数" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            int size = atoi(p2_size.c_str());
            if (size <= 0 || size > 1024)
            {
                send_str << "[error] size 的取值范围是(0,1024]." << endl;
                cout << send_str.str() << endl;
                continue;
            }
            // 调用 API
            char buf[1025];
            memset(buf, 0, sizeof(buf));
            int ret = Kernel::Instance().Sys_Read(fd, size, 1025, buf);
            // 结果返回
            send_str << "[result] read " << ret << " letters" << endl
                     << buf << endl;
            cout << send_str.str();
            continue;
        }
        if (api == "write")
        {
            string p1_fd = "";
            string p2_content = "";
            ss >> p1_fd >> p2_content;
            if (p1_fd == "")
            {
                send_str << "write [fd] [content]\n";
                send_str << "参数个数错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            if (!isNumber(p1_fd))
            {
                send_str << "[error] 参数fd错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            int fd = atoi(p1_fd.c_str());
            if (fd < 0)
            {
                send_str << "[error] 参数fd应当为正整数" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            if (p2_content.length() > 1024)
            {
                send_str << "[error] 内容content过长（不超过1024字节）" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            char buf[1025];
            memset(buf, 0, sizeof(buf));
            strcpy(buf, p2_content.c_str());
            int size = p2_content.length();
            // 调用 API
            int ret = Kernel::Instance().Sys_Write(fd, size, 1024, buf);
            // 打印结果
            send_str << "[result] written " << ret << " letters" << endl;
            cout << send_str.str();
            continue;
        }
        if (api == "close")
        {
            string p1_fd;
            ss >> p1_fd;
            if (p1_fd == "")
            {
                send_str << "close [fd]\n";
                send_str << "参数个数错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            if (!isNumber(p1_fd))
            {
                send_str << "[error] 参数fd错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            int fd = atoi(p1_fd.c_str());
            if (fd < 0)
            {
                send_str << "[error] 参数fd应当为正整数" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            // 调用 API
            int ret = Kernel::Instance().Sys_Close(fd);
            // ret = 0 success
            continue;
        }
        if (api == "cat")
        {
            string p1_fpath;
            ss >> p1_fpath;
            if (p1_fpath == "")
            {
                send_str << "cat [fpath]\n";
                send_str << "参数个数错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            string fpath = p1_fpath;
            // Open
            FD fd = Kernel::Instance().Sys_Open(fpath, 0x1);
            if (fd < 0)
            {
                send_str << "[error] 打开文件出错." << endl;
                cout << send_str.str() << endl;
                continue;
            }
            // Read
            char buf[257];
            while (true)
            {
                memset(buf, 0, sizeof(buf));
                int ret = Kernel::Instance().Sys_Read(fd, 256, 256, buf);
                if (ret <= 0)
                {
                    break;
                }
                send_str << buf;
            }
            // Close
            Kernel::Instance().Sys_Close(fd);
            cout << send_str.str() << endl;
            continue;
        }
        // TODO
        if (api == "copyin")
        {
            string p1_ofpath;
            string p2_ifpath;
            ss >> p1_ofpath >> p2_ifpath;
            if (p1_ofpath == "" || p2_ifpath == "")
            {
                send_str << "copyin ofpath ifpath\n";
                send_str << "参数个数错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            // 打开外部文件
            int ofd = open(p1_ofpath.c_str(), O_RDONLY); // 只读方式打开外部文件
            if (ofd < 0)
            {
                send_str << "[error] 打开文件失败：" << p1_ofpath << endl;
                cout << send_str.str() << endl;
                continue;
            }
            // 创建内部文件
            Kernel::Instance().Sys_Creat(p2_ifpath, 0x1 | 0x2);
            int ifd = Kernel::Instance().Sys_Open(p2_ifpath, 0x1 | 0x2);
            if (ifd < 0)
            {
                close(ofd);
                send_str << "[error] 打开文件失败：" << p2_ifpath << endl;
                cout << send_str.str() << endl;
                continue;
            }
            // 开始拷贝，一次 256 字节
            char buf[256];
            int all_read_num = 0;
            int all_write_num = 0;
            while (true)
            {
                memset(buf, 0, sizeof(buf));
                int read_num = read(ofd, buf, 256);
                if (read_num <= 0)
                {
                    break;
                }
                all_read_num += read_num;
                int write_num =
                    Kernel::Instance().Sys_Write(ifd, read_num, 256, buf);
                if (write_num <= 0)
                {
                    send_str << "[error] 写入文件失败：" << p2_ifpath;
                    break;
                }
                all_write_num += write_num;
            }
            send_str << "共读取字节：" << all_read_num
                     << " 共写入字节：" << all_write_num << endl;
            close(ofd);
            Kernel::Instance().Sys_Close(ifd);
            cout << send_str.str() << endl;
            continue;
        }
        if (api == "copyout")
        {
            string p1_ifpath;
            string p2_ofpath;
            ss >> p1_ifpath >> p2_ofpath;
            if (p1_ifpath == "" || p2_ofpath == "")
            {
                send_str << "copyout [ifpath] [ofpath]\n";
                send_str << "参数个数错误" << endl;
                cout << send_str.str() << endl;
                continue;
            }
            // 创建外部文件
            int ofd = open(p2_ofpath.c_str(), O_WRONLY | O_TRUNC | O_CREAT); // 截断写入方式打开外部文件
            if (ofd < 0)
            {
                send_str << "[error] 创建文件失败：" << p2_ofpath << endl;
                cout << send_str.str() << endl;
                continue;
            }
            // 打开内部文件
            int ifd = Kernel::Instance().Sys_Open(p1_ifpath, 0x1 | 0x2);
            if (ifd < 0)
            {
                close(ofd);
                send_str << "[error] 打开文件失败：" << p1_ifpath << endl;
                cout << send_str.str() << endl;
                continue;
            }
            // 开始拷贝，一次 256 字节
            char buf[256];
            int all_read_num = 0;
            int all_write_num = 0;
            while (true)
            {
                memset(buf, 0, sizeof(buf));
                int read_num =
                    Kernel::Instance().Sys_Read(ifd, 256, 256, buf);
                if (read_num <= 0)
                {
                    break;
                }
                all_read_num += read_num;
                int write_num = write(ofd, buf, read_num);
                if (write_num <= 0)
                {
                    send_str << "[error] 写入文件失败：" << p1_ifpath;
                    break;
                }
                all_write_num += write_num;
            }
            send_str << "共读取字节：" << all_read_num
                     << " 共写入字节：" << all_write_num << endl;
            close(ofd);
            Kernel::Instance().Sys_Close(ifd);
            cout << send_str.str() << endl;
            continue;
        }
        if (api == "q" || api == "quit")
        {
            // Kernel::Instance().GetUserManager().Logout();
            send_str << "用户登出\n";
            cout << send_str.str() << endl;
            break;
        }
        if (api != "" && api != " ")
        {
            std::stringstream tishi;
            tishi = print_head();
            tishi << "\n"
                  << "温馨提示：您的指令错误！\n";
            cout << tishi.str() << endl;
        }
    }
}

int main(int argc, char const *argv[])
{
    Kernel::Instance().Initialize();
    // 1. 关联根目录
    User &u = Kernel::Instance().GetUser();
    u.u_cdir = g_InodeTable.IGet(FileSystem::ROOTINO);
    strcpy(u.u_curdir, "/");
    // 2. 尝试创建家目录
    std::string bin = "bin";
    Kernel::Instance().Sys_Mkdir(bin);
    std::string etc = "etc";
    Kernel::Instance().Sys_Mkdir(etc);
    std::string home = "home";
    Kernel::Instance().Sys_Mkdir(home);
    std::string dev = "dev";
    Kernel::Instance().Sys_Mkdir(dev);
    // 3. 跳转
    u.u_error = NOERROR;
    // char dirname[512] = {0};
    // strcpy(dirname, home.c_str());
    // u.u_dirp = dirname;
    // u.u_arg[0] = (unsigned long long)(dirname);
    // printf("u.u_arg[0]:%llu\n", (unsigned long long)u.u_arg[0]);
    FileManager &fimanag = Kernel::Instance().GetFileManager();
    // fimanag.ChDir();
    // printf("[info] 请登陆\n");
    start_routine();
    Kernel::Instance().Quit();
    return 0;
}

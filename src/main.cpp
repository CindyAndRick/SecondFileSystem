#include <iostream>
#include <string>
#include <string.h>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#define PORT 1235
#define BACKLOG 128

#include "../include/Kernel.h"

using std::cin;
using std::cout;
using std::endl;
using std::string;

void handle_pipe(int sig)
{
    // 不做任何处理即可
}

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
    send_str << "||open(char *name, int mode=0777)            ||" << endl;
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
    send_str << "||help()                                     ||" << endl;
    send_str << "||q/Q 退出文件系统                           ||" << endl;
    return send_str;
}

class sendU
{
private:
    int fd;
    string username;

public:
    int send_(const stringstream &send_str)
    {
        // cout<<send_str.str()<<endl;
        int numbytes = send(fd, send_str.str().c_str(), send_str.str().length(), 0);
        cout << "[[ " << username << " ]] send " << numbytes << " bytes" << endl;
        return numbytes;
    };
    sendU(int fd, string username)
    {
        this->fd = fd;
        this->username = username;
    };
};

void *start_routine(void *ptr)
{
    int fd = *(int *)ptr;
    char buf[1024];
    int numbytes;
    numbytes = send(fd, "请输入用户名：", sizeof("请输入用户名："), 0);
    cout << "[info] send函数返回值：" << numbytes << endl;

    printf("进入用户线程，fd=%d\n", fd);

    memset(buf, 0, sizeof(buf));
    if ((numbytes = recv(fd, buf, 1024, 0)) == -1)
    {
        cout << ("recv() error\n");
        return (void *)NULL;
    }

    string username = buf;
    cout << "[info] 用户输入用户名：" << username << endl;

    // 读取密码
    memset(buf, 0, sizeof(buf));
    numbytes = recv(fd, buf, 1024, 0);
    if (numbytes == -1)
    {
        cout << ("recv() error\n");
        return (void *)NULL;
    }
    string password = buf;
    cout << "[info] 用户输入密码：" << password << endl;

    // 验证用户名和密码
    if (Kernel::Instance().GetUserManager().CheckUser(username, password) == false)
    {
        cout << "[info] 用户 " << username << " 登录失败." << endl;
        send(fd, "[info] 用户名或密码错误，登录失败.", sizeof("[info] 用户名或密码错误，登录失败."), 0);
        return (void *)NULL;
    }
    cout << "[info] 用户 " << username << " 登录成功." << endl;

    sendU sd(fd, username);
    stringstream greeting;
    greeting << "[info] 登录成功，欢迎使用Rick's SecondFileSystem，" << username << "!" << endl;
    sd.send_(greeting);
    stringstream head = print_head();
    sd.send_(head);

    // 初始化用户User结构和目录
    Kernel::Instance().GetUserManager().Login(username);

    string tipswords;

    while (true)
    {
        tipswords = "||SecondFileSystem@" + username + ":" + Kernel::Instance().GetUser().u_curdir + "$ ";
        // cout << tipswords;
        // string buf_recv;

        // getline(cin, buf_recv);

        char buf_recv[1024] = {0};

        // 发送提示
        numbytes = send(fd, tipswords.c_str(), tipswords.length(), 0);
        if (numbytes <= 0)
        {
            cout << "[info] 用户 " << username << " 断开连接." << endl;
            Kernel::Instance().GetUserManager().Logout();
            return (void *)NULL;
        }
        printf("[INFO] send %d bytes\n", numbytes);

        // 读取用户输入的命令行
        if ((numbytes = recv(fd, buf_recv, 1024, 0)) == -1)
        {
            cout << "recv() error" << endl;
            Kernel::Instance().GetUserManager().Logout();
            return (void *)NULL;
        }

        // 解析命令名称
        std::stringstream ss(buf_recv);
        cout << "buf_recv : " << buf_recv << endl;
        string api;
        ss >> api;
        std::stringstream send_str;

        cout << "api : " << api << endl;
        if (api == "help")
        {
            sd.send_(head);
        }
        if (api == "cd")
        {
            string param1;
            ss >> param1;
            if (param1 == "")
            {
                send_str << "cd [fpath]";
                send_str << "参数个数错误" << endl
                         << endl;
                sd.send_(send_str);
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
            send_str << endl;
            sd.send_(send_str);
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
                send_str << endl;
                sd.send_(send_str);
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
                send_str << endl;
                sd.send_(send_str);
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
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            if (!isNumber(fd))
            {
                send_str << "[error] 参数fd错误" << endl;
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            int fd_int = atoi(fd.c_str());
            if (!isNumber(position))
            {
                send_str << "[error] 参数position错误" << endl;
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            int position_int = atoi(position.c_str());
            if (!isNumber(ptrname))
            {
                send_str << "[error] 参数ptrname错误" << endl;
                send_str << endl;
                sd.send_(send_str);
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
            send_str << endl;
            sd.send_(send_str);
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
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            string fpath = param1;
            if (!isNumber(param2))
            {
                send_str << "[error] 参数mode错误" << endl;
                send_str << endl;
                sd.send_(send_str);
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
            send_str << endl;
            sd.send_(send_str);
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
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            if (!isNumber(p1_fd))
            {
                send_str << "[error] 参数fd错误" << endl;
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            if (!isNumber(p2_size))
            {
                send_str << "[error] 参数size错误" << endl;
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            int fd = atoi(p1_fd.c_str());
            if (fd < 0)
            {
                send_str << "[error] 参数fd应当为正整数" << endl;
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            int size = atoi(p2_size.c_str());
            if (size <= 0 || size > 1024)
            {
                send_str << "[error] size 的取值范围是(0,1024]." << endl;
                send_str << endl;
                sd.send_(send_str);
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
            send_str << endl;
            sd.send_(send_str);
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
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            if (!isNumber(p1_fd))
            {
                send_str << "[error] 参数fd错误" << endl;
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            int fd = atoi(p1_fd.c_str());
            if (fd < 0)
            {
                send_str << "[error] 参数fd应当为正整数" << endl;
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            if (p2_content.length() > 1024)
            {
                send_str << "[error] 内容content过长（不超过1024字节）" << endl;
                send_str << endl;
                sd.send_(send_str);
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
            send_str << endl;
            sd.send_(send_str);
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
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            if (!isNumber(p1_fd))
            {
                send_str << "[error] 参数fd错误" << endl;
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            int fd = atoi(p1_fd.c_str());
            if (fd < 0)
            {
                send_str << "[error] 参数fd应当为正整数" << endl;
                send_str << endl;
                sd.send_(send_str);
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
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            string fpath = p1_fpath;
            // Open
            FD fd = Kernel::Instance().Sys_Open(fpath, 0x1);
            if (fd < 0)
            {
                send_str << "[error] 打开文件出错." << endl;
                send_str << endl;
                sd.send_(send_str);
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
            send_str << endl;
            sd.send_(send_str);
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
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            // 打开外部文件
            int ofd = open(p1_ofpath.c_str(), O_RDONLY); // 只读方式打开外部文件
            if (ofd < 0)
            {
                send_str << "[error] 打开文件失败：" << p1_ofpath << endl;
                send_str << endl;
                sd.send_(send_str);
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
                send_str << endl;
                sd.send_(send_str);
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
            send_str << endl;
            sd.send_(send_str);
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
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            // 创建外部文件
            int ofd = open(p2_ofpath.c_str(), O_WRONLY | O_TRUNC | O_CREAT); // 截断写入方式打开外部文件
            if (ofd < 0)
            {
                send_str << "[error] 创建文件失败：" << p2_ofpath << endl;
                send_str << endl;
                sd.send_(send_str);
                cout << send_str.str() << endl;
                continue;
            }
            // 打开内部文件
            int ifd = Kernel::Instance().Sys_Open(p1_ifpath, 0x1 | 0x2);
            if (ifd < 0)
            {
                close(ofd);
                send_str << "[error] 打开文件失败：" << p1_ifpath << endl;
                send_str << endl;
                sd.send_(send_str);
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
            send_str << endl;
            sd.send_(send_str);
            cout << send_str.str() << endl;
            continue;
        }
        if (api == "q" || api == "quit")
        {
            // Kernel::Instance().GetUserManager().Logout();
            send_str << "用户登出\n";
            send_str << endl;
            sd.send_(send_str);
            cout << send_str.str() << endl;
            break;
        }
        if (api != "" && api != " " && api != "help")
        {
            std::stringstream tishi;
            tishi = print_head();
            tishi << "\n"
                  << "温馨提示：您的指令错误！\n"
                  << endl;
            sd.send_(tishi);
            cout << tishi.str() << endl;
        }
    }
}

int main(int argc, char const *argv[])
{
    // 进行信号处理
    struct sigaction action;
    action.sa_handler = handle_pipe;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGPIPE, &action, NULL);

    int listenfd, connectfd;
    struct sockaddr_in server;
    struct sockaddr_in client;
    int sin_size;
    sin_size = sizeof(struct sockaddr_in);

    // 创建监听fd
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Creating socket failed.");
        exit(1);
    }

    int opt = SO_REUSEADDR;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // 使得端口释放后立马被复用
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    // 绑定
    if (bind(listenfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("Bind error.");
        exit(1);
    }
    // 监听
    if (listen(listenfd, BACKLOG) == -1)
    { /* calls listen() */
        perror("listen() error\n");
        exit(1);
    }
    // 初始化文件系统
    Kernel::Instance().Initialize();
    string etc = "etc";
    Kernel::Instance().Sys_Mkdir(etc);
    string bin = "bin";
    Kernel::Instance().Sys_Mkdir(bin);
    string home = "home";
    Kernel::Instance().Sys_Mkdir(home);
    string dev = "dev";
    Kernel::Instance().Sys_Mkdir(dev);
    cout << "[info] 等待用户接入..." << endl;
    // 进入通信循环
    while (1)
    {
        // accept
        if ((connectfd = accept(listenfd, (struct sockaddr *)&client, (socklen_t *)&sin_size)) == -1)
        {
            perror("accept() error\n");
            continue;
        }
        printf("客户端接入：%s\n", inet_ntoa(client.sin_addr));
        string str = "hello";
        // send(connectfd,str.c_str(),6,0);
        pthread_t thread; // 定义一个线程号
        pthread_create(&thread, NULL, start_routine, (void *)&connectfd);
    }
    close(listenfd);
    return 0;
}

#include "../include/Kernel.h"
#include "../include/UserManager.h"

UserManager::UserManager()
{
    // 清空
    for (int i = 0; i < USER_N; ++i)
    {
        (this->pusers)[i] = NULL;
    }
    this->user_addr.clear();
    // 分配一个给系统主进程
    pthread_t pid = pthread_self();
    pusers[0] = (User *)malloc(sizeof(User));
    user_addr[pid] = 0;
}

UserManager::~UserManager()
{
    for (int i = 0; i < USER_N; ++i)
    {
        if ((this->pusers)[i] != NULL)
            free((this->pusers)[i]);
    }
}

// 用户登录
bool UserManager::Login(string uname)
{
    // 取得线程 id
    pthread_t pthread_id = pthread_self();
    // 检查该线程是否已登录
    if (user_addr.find(pthread_id) != user_addr.end())
    {
        printf("[ERROR] 线程 %lu 重复登录\n", pthread_id);
        return false;
    }
    // 寻找空闲的pusers指针
    int i;
    for (i = 0; i < USER_N; ++i)
    {
        if (pusers[i] == NULL)
        {
            break;
        }
    }
    if (i == USER_N)
    {
        printf("[ERROR] UserManager无空闲资源可用，用户并发数量达到上限\n");
        return false;
    }
    // i 为空闲索引
    pusers[i] = (User *)malloc(sizeof(User));
    if (pusers[i] == NULL)
    {
        printf("[ERROR] UserManager申请堆空间失败\n");
        return false;
    }
    // 建立pid与addr的关联
    user_addr[pthread_id] = i;
    pusers[i]->u_uid = 0;
    printf("[INFO] 线程 %lu 登录成功.\n", pthread_id);
    // 设置 User 结构的初始值
    // 1. 关联根目录
    pusers[i]->u_cdir = g_InodeTable.IGet(FileSystem::ROOTINO);
    pusers[i]->u_cdir->NFrele();
    strcpy(pusers[i]->u_curdir, "/");
    // 2. 转到home目录
    pusers[i]->u_error = NOERROR;
    char dirname1[512] = {0};
    string tmp = "/home";
    strcpy(dirname1, tmp.c_str());
    pusers[i]->u_dirp = dirname1;
    pusers[i]->u_arg[0] = (unsigned long long)(dirname1);
    FileManager &fimanag = Kernel::Instance().GetFileManager();
    fimanag.ChDir();
    // 2. 尝试创建家目录
    Kernel::Instance().Sys_Mkdir(uname);
    printf("[info] home dic created\n");
    // 3. 转到家目录
    pusers[i]->u_error = NOERROR;
    char dirname2[512] = {0};
    tmp = "/home/" + uname;
    strcpy(dirname2, tmp.c_str());
    pusers[i]->u_dirp = dirname2;
    pusers[i]->u_arg[0] = (unsigned long long)(dirname2);
    fimanag.ChDir();
    return true;
}
// 用户登出
bool UserManager::Logout()
{
    // 将系统更新至磁盘
    Kernel::Instance().Quit();

    // 取得线程 id
    pthread_t pthread_id = pthread_self();
    // 检查该线程是否已登录
    if (user_addr.find(pthread_id) == user_addr.end())
    {
        printf("[ERROR] 线程 %ld 未登录，无需登出\n", pthread_id);
        return false;
    }
    int i = user_addr[pthread_id];
    free(pusers[i]);
    user_addr.erase(pthread_id);
    printf("[INFO] 线程 %ld 登出成功.\n", pthread_id);
    return true;
}

// 得到当前线程的User结构
User *UserManager::GetUser()
{
    pthread_t pthread_id = pthread_self();
    if (user_addr.find(pthread_id) == user_addr.end())
    {
        printf("[ERROR] 线程 %ld 的 User 结构无法得到，系统错误.\n", pthread_id);
        exit(1);
    }
    return pusers[user_addr[pthread_id]];
}

bool UserManager::CheckUser(string username, string passwd)
{
    ifstream file("password.txt");
    bool found = false;
    string correct_passwd;
    if(file.is_open()){
        string line;
        while (getline(file, line)) {
            size_t pos = line.find(":");
            if (pos != string::npos) {
                string name = line.substr(0, pos);
                if (name == username) {
                    correct_passwd = line.substr(pos + 1);
                    found = true;
                    break;
                }
            }
        }
        file.close();
    }
    else{
        cout << "[error] Unable to open password.txt file" << endl;
    }
    if(found){
        if (correct_passwd == passwd) {
            return true;
        }
        else{
            return false;
        }
    }
    else{
        return false;
    }
}
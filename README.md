# SecondFileSystem

同济大学操作系统课设 二级文件系统

## Quick Start

### server端

1. 进入SecondFileSystem文件夹
2. 运行 `make clean`
3. 运行 `make all`
4. 运行 `./SecondFileSystem`

### client端

1. 进入SecondFileSystem/client文件夹
2. 运行 `make all`
3. 运行 `./client 127.0.0.1 1235`，其中端口号需要对应server端代码中PORT

## 指令

1. `open [name] [mode]` 打开名为`name`的文件，`mode` 默认值为`0777`，返回打开文件`fd`
2. `close [fd]` 关闭`fd`号文件
3. `read [fd] [length]` 读取`fd` 号文件从当前指针位置开始的`length`长度个字符
4. `write [fd] [buffer] [length]` 向`fd`号文件当前指针位置写入`buffer`， `length`默认值为`buffer`长度
5. `seek [fd] [position] [ptrname]` 将`fd`号文件的指针调整到某位置，其中`position`为偏置，`ptrname`：0 读写位置设置为`position`，1 读写位置加`position`，2 读写位置设置为文件长度加`position`
6. `mkfile [name] [mode]` 创建名称为`name`的文件，`mode`默认值为`0777`
7. `rm [name]` 删除名称为`[name]`的文件/文件夹（非递归删除）
8. `ls` 展示当前目录下的所有目录项
9. `mkdir [dirname]` 创建名称为`dirname`的文件夹
10. `cd [dirname]` 前往名称为`dirname`的文件夹，若`dirname`以`/`开头则从根目录开始寻找，否则从当前目录开始
11. `cat [dirname]`  查看当前目录下名称为`dirname`的文件
12. `copyin [ofpath] [ifpath]` 将二级文件系统外`ofpath`路径的文件复制到系统内`ifpath`处
13. `copyout [ifpath] [ofpath]` 将二级文件系统内`ifpath`处路径复制到系统外`ofpath`处
14. `help` 查看提示信息

代码参考

https://github.com/fffqh/MultiUser-secondFileSystem

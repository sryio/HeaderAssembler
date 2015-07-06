# HeaderAssembler

头文件合并工具

# Windows 下编译

直接使用Visual Studio 2015打开 sln文件，开始构建即可。


# Mac 下编译

首先来到这里 http://brew.sh/，安装Homebrew Package Manager，之后运行：

```
brew install boost
```

等待boost安装完成后，使用xcode打开项目，编译即可。

# 使用方法

HeaderAssembler "等待组装的头文件全路径.h" "输入头文件全路径.h" ["头文件搜索路径1;头文件搜索路径2;头文件搜索路径3;...."]


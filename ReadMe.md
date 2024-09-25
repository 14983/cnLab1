# 计算机网络 实验一

---

本项目由我和 [@BottleFish](https://github.com/BottleFish326) 共同完成。

## 编译运行

```bash
$ make
$ ./client.out
$ ./server.out
```

> 如果您的系统中有 `gnome-terminal`，可以直接使用 `make run` 命令运行。您也可以修改 `Makefile` 中的 `ENV` 变量来指定运行环境。

如果无法编译，请确保满足以下条件之一：

- 安装了 `libnotify-dev`，且 `./common.h` 中定义 `USE_NOTIFY_SEND`，如果仍然不能编译，请考虑去掉 `Makefile` 中的 `GTKFLAG`
- 安装了 `libgtk-3-dev`，且 `./common.h` 中定义 `USE_GTK`

63S-GRUB 是一个极度精简的 GRUB。

精简的目的是将体积控制在 32256 字节（63个扇区）之内，从而可以装入硬盘0磁道，不受重新分区和格式化的影响。

体积减小了，功能也大大缩水，严格来讲，已经算不上引导器，只能算是 “引导器的引导器”。它的使命就是加载其它 boot loader ，再在新的 loader 中做你想做的事。

63S-GRUB 由 UBUNTU 9.04 的 GRUB 改造而来，主要改动如下：

1.底层函数基本未动；
2.功能部分进行了大幅删减，gzip 支持、网络启动、图形模式等均被去除；
3.保留了命令行（包括“自动补全”特性）；
4.重新实现了一个简单的菜单系统；
5.大部分命令被去除，仅保留 root、rootnoverify、chainloader、boot、find 命令；
6.新增 ntldr 命令，用来引导 ntldr/peldr/bootmgr/grldr；
7.新增 loadgrub 命令，可加载 grub1 的 stage2 和 grub2 的 core.img；
8.默认支持 fat、ntfs、ext2/3/4 文件系统（还有 reiserfs 可选）。

其中，ntldr 命令和 ntfs 文件系统代码取自 GRUB4DOS 。

图形界面的安装配置程序（By Pauly，For windows）：  http://bbs.wuyou.com/viewthread.php?tid=160886

参与讨论： http://bbs.wuyou.com/viewthread.php?tid=159898
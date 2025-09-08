# MonitorGGWrite
监控 GameGuardian 内存写入

For English instructions please click [here](README.md)

## 说明
1. 将 GameGuardian 目录中的 lib5.so 重命名为 lib5.so.orig
2. 将 lib5.so 和 编译后的libHookWrite.so 从项目仓库移动到 /data/user/0/com.XXXX/files/GG-XXXX/ 目录
3. 重新打开 GameGuardian 以永久使用 Hook
## 编译项目
- 使用 camke 编译
```cpp
mkdir build \&\& cd build
cmake ..
make -j$(nproc)
```





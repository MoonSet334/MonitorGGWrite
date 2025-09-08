# MonitorGGWrite
Monitors GameGuardian memory writes.

中文说明请点击[此处](README.zh-CN.md)

## Instructions
1. Rename lib5.so in the GameGuardian directory to lib5.so.orig
2. Move lib5.so and The compiled libHookWrite.so from the project repository to the /data/user/0/XXXX/files/GG-XXXX/ directory
3. Reopen GameGuardian to permanently use the Hook
## Compile the project
- Compile using camke
```cpp
mkdir build && cd build
cmake ..
make -j$(nproc)
```

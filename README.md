# Chinese Official Mahjong Helper
国标麻将助手

基于cocos2dx引擎写的国标麻将助手，可用于算番和面麻计分

##VS2013/VS2015配置
1. 下载cocos2dx v3.11
2. clone项目
3. 随便创建一个c++工程
4. 将生成的工程下的cocos2d目录拷贝到ChineseOfficialMahjongHelper目录下
5. 用VS2013/VS2015打开proj.win32目录下的ChineseOfficialMahjongHelper.sln
6. 编译完成后可以将另外五个项目都卸载了

##xcode配置
1. 前4步同VS2013
2. 用xcode打开proj.ios_mac目录下的ChineseOfficialMahjongHelper.xcodeproj
PS: 如果编译ios项目报找不到Prefix.pch，则新建一个，复制粘贴如下代码
```Objective-C
#ifdef __OBJC__
    #import <Foundation/Foundation.h>
    #import <UIKit/UIKit.h>
#endif
```

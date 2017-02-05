# Chinese Official Mahjong Helper
国标麻将助手

本程序以cocos2dx引擎为UI写成，内容包含算番器、计分器、番种详细说明等
算番逻辑可提取出来单独使用

##VS2013/VS2015配置
1. 下载cocos2dx v3.14.1
2. clone项目
3. 随便创建一个c++工程
4. 将生成的工程下的cocos2d目录拷贝到Classes的同一级目录下
5. 用VS2013/VS2015打开proj.win32目录下的ChineseOfficialMahjongHelper.sln
6. 当第一次编译完成后，可以将引擎相关的项目都卸载了，以节约下次编译的时间

##xcode配置
1. 前4步同VS2013
2. 用xcode打开proj.ios_mac目录下的ChineseOfficialMahjongHelper.xcodeproj

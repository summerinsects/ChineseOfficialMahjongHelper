# Chinese Official Mahjong Helper
**国标小助手**

1. 本程序以cocos2dx引擎为UI写成，包含算番器、计分器、番种详细说明、牌理等内容
2. 算番逻辑在mahjong-algorithm文件夹下，可提取出来单独使用
3. 由于cocos2dx引擎自身的源码较为庞大，这里就不上传了，请按如下步骤配置

## 配置步骤 ##
1. 下载cocos2dx v3.14.1
2. clone本项目
3. 随便创建一个c++工程（命令行：cocos new -l cpp --portrait 工程名）
4. 将生成的工程下的cocos2d目录拷贝到Classes的同一级目录下

**Win32版**
* 用VS2013/VS2015打开proj.win32目录下的ChineseOfficialMahjongHelper.sln
* PS: 当第一次编译完成后，可以将引擎相关的项目都卸载了，以节约下次编译的时间

**iOS版**
* 用xcode打开proj.ios_mac目录下的ChineseOfficialMahjongHelper.xcodeproj

**Android版(使用Eclipse ADT)**
1. shift+右键“在此处打开命令行窗口”
2. cocos compile -p android（如果要编译release版本，请加参数-m release)

**Android版(使用Android Studio)**
* 暂未打理，需要的可参考Eclipse ADT的相关文件

Chinese Official Mahjong Helper 国标小助手
=========
- 这是与国标麻将相关的应用，包含算番器、线下实麻计分器、番种详细说明、牌理等内容
- 本程序以cocos2dx引擎为UI写成
- 算番、判断听牌、听牌计算、上听数计算、有效牌计算等相关算法在mahjong-algorithm文件夹下，可提取出来单独使用
- 由于cocos2dx引擎自身的源码较为庞大，这里就不上传了，请按如下步骤配置

## 配置步骤

1. 下载[cocos2dx v3.17](http://www.cocos2d-x.org/download)
2. 下载(download)本项目（clone随意）
3. 随便创建一个c++工程，工程名可以随便取（命令行：cocos new -l cpp --portrait 工程名）
4. 将在上一步创建的工程目录下的cocos2d目录拷贝到本项目Classes的同一级目录下
5. cocos2dx引擎源码修改：将文件夹cocos2d_improvements里面的内容复制到cocos2d

- 配置步骤的第3、4步也可改为如下两步：
3. 将在本项目Classes的同一级目录下新建一个cocos2d目录
4. 将解压cocos2dx引擎后的文件，除templates、tests、web外，其他全部复制到cocos2d目录

- 配置完成后项目的目录结构为：
   - attachment/
   - Classes/
   - cocos2d/
       - build/
       - cmake/
       - cocos/
       - docs/
       - extensions/
       - external/
       - licenses/
       - tools/
       - AUTHORS
       - CHANGELOG
       - CMakeLists.txt
       - CONTRIBUTING.md
       - download-deps.py
       - issue_template.md
       - README.md
   - cocos2d_improvements/
   - proj.android/
   - proj.ios_mac/
   - proj.linux/
   - proj.win32/
   - Resources/
   - CMakeLists.txt
   - LICENSE
   - README.md

---

#### Win32版
- 用VS2015/VS2017打开proj.win32目录下的ChineseOfficialMahjongHelper.sln
- PS: 当第一次编译完成后，可以将引擎相关的项目都卸载了，以节约下次编译的时间

#### iOS版
- 用xcode打开proj.ios_mac目录下的ChineseOfficialMahjongHelper.xcodeproj

#### Android版
- 用Android Studio导入proj.android

---

## 特别鸣谢
1. 牌画来源：[日语维基——中国麻雀](https://ja.wikipedia.org/wiki/%E4%B8%AD%E5%9B%BD%E9%BA%BB%E9%9B%80)
2. 其他图片来源：CrossApp
3. Android自带素材

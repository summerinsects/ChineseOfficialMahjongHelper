Chinese Official Mahjong Helper 国标小助手
=========
- 这是与国标麻将相关的应用，包含算番器、线下实麻计分器、番种详细说明、牌理等内容
- 本项目主要在[GitHub](https://github.com/summerinsects/ChineseOfficialMahjongHelper)上提交，码云（gitee）这边不定期手动同步代码
- 本程序以cocos2dx引擎为UI写成
- Classes/mahjong-algorithm文件夹下为算番、判断听牌、听牌计算、上听数计算、有效牌计算等相关算法，可独立使用。
- 由于cocos2dx引擎自身的源码较为庞大，这里就不上传了，请按如下步骤配置

## 配置步骤

1. 下载[cocos2dx v4.0](http://www.cocos2d-x.org/download)
2. 下载(download)本项目（clone随意）
3. 随便创建一个c++工程，工程名任意（命令行：cocos new -l cpp --portrait 工程名）
4. 将在上一步创建的工程目录下的"cocos2d目录"和".cocos-project.json"文件拷贝到本项目Classes的同一级目录下
5. cocos2dx引擎源码修改：将文件夹cocos2d_improvements里面的内容复制到cocos2d

#### 配置步骤的第3、4步也可改为如下两步：（但推荐使用上面一种方式，上一种方式cocos2d目录体积更小）
3. 将在本项目Classes的同一级目录下新建一个cocos2d目录
4. 将解压cocos2dx引擎后的文件，除templates、tests外，其他全部复制到cocos2d目录

#### 配置完成后项目的目录结构为：
```
   - attachment/
   - Classes/
   - cocos2d/
       - cmake/
       - cocos/
       - docs/
       - extensions/
       - external/
       - licenses/
       - tools/
       - .appveyor.yml
       - AUTHORS
       - CHANGELOG
       - CMakeLists.txt
       - CONTRIBUTING.md
       - download-deps.py
       - install-deps-linux.sh
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
```
---

- 命令行依次执行
```
    $ mkdir build
    $ cd build
    $ cocos run --proj-dir .. -p [mac|windows|android|linux|ios]
```

---
### 仅针对本项目缩小包体的方法
- 打开cocos/base/ccConfig.h
- 将CC_USE_PHYSICS、CC_USE_3D_PHYSICS、CC_USE_NAVMESH、CC_USE_CULLING、CC_USE_WEBP、CC_ENABLE_SCRIPT_BINDING这几个宏定义值改为0

---

## 图片来源
1. Ant Design 官方图标库
2. Android自带素材

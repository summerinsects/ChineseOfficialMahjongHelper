Chinese Official Mahjong Helper 国标小助手
=========
- 这是与国标麻将相关的应用，包含算番器、线下实麻计分器、番种详细说明、牌理等内容
- 本程序以cocos2dx引擎为UI写成
- 算番、判断听牌、听牌计算、上听数计算、有效牌计算等相关算法在mahjong-algorithm文件夹下，可提取出来单独使用
- 由于cocos2dx引擎自身的源码较为庞大，这里就不上传了，请按如下步骤配置

## 配置步骤

- 下载cocos2dx v3.16
- clone本项目
- 随便创建一个c++工程，工程名可以随便取（命令行：cocos new -l cpp --portrait 工程名）
- 将在上一步创建的工程目录下的cocos2d目录拷贝到本项目Classes的同一级目录下

#### Win32版
- 用VS2013/VS2015/VS2017打开proj.win32目录下的ChineseOfficialMahjongHelper.sln
- PS: 当第一次编译完成后，可以将引擎相关的项目都卸载了，以节约下次编译的时间

#### iOS版
- 用xcode打开proj.ios_mac目录下的ChineseOfficialMahjongHelper.xcodeproj

#### Android版(使用Eclipse ADT)
- shift+右键“在此处打开命令行窗口”
- cocos compile -p android（如果要编译release版本，请加参数-m release）

---

## 其他说明

### 如何使界面非全屏
cocos2dx为游戏引擎，默认是全屏，用来写应用一般需要非全屏
- Android的实现需要修改cocos2dx的源码：
  - 打开cocos2d/cocos/platform/android/java/src/org/cocos2dx/lib/Cocos2dxActivity.java
  - 删除Cocos2dxActivity.hideVirtualButton方法
  - 删除所有对Cocos2dxActivity.hideVirtualButton方法的调用
- iOS的非全屏功能主体部分已经在本项目的代码实现，但需要改一处cocos2dx的源码：
  - 打开cocos2d/cocos/platform/ios/CCEAGLView-ios.mm
  - 找到函数`- (void)onUIKeyboardNotification:(NSNotification *)notif`（759行）
  - 在`auto glview = cocos2d::Director::getInstance()->getOpenGLView();`一句前、switch语句后，增加如下代码：
```obj-c
    CGPoint viewOrigin = self.frame.origin;
    begin.origin.x += viewOrigin.x;
    begin.origin.y += viewOrigin.y;
    end.origin.x += viewOrigin.x;
    end.origin.y += viewOrigin.y;
```

### iOS的WebView不能透明（cocos2dx v3.16 BUG）
- 打开cocos2d/cocos/ui/UIWebViewImpl-ios.mm
- 找到函数`-(void)setBackgroundTransparent`（170行），将函数体改为如下代码：
```obj-c
    if (!self.uiWebView) {[self setupWebView];}
    [self.uiWebView setOpaque:NO];
    [self.uiWebView setBackgroundColor:[UIColor clearColor]];
```

### iOS的WebView如何禁止数字链接
- 同样是cocos2d/cocos/ui/UIWebViewImpl-ios.mm
- 找到函数`-(void)setupWebView`（141行），在`self.uiWebView.delegate = self;`后增加如下代码
```obj-c
    self.uiWebView.dataDetectorTypes = UIDataDetectorTypeNone;
```

### 扩展cocos2d::StringUtils::StringUTF8（必须。项目中部分功能的实现基于此扩展）
https://github.com/cocos2d/cocos2d-x/pull/18356


## 特别鸣谢
1. 牌画来源：[日语维基——中国麻雀](https://ja.wikipedia.org/wiki/%E4%B8%AD%E5%9B%BD%E9%BA%BB%E9%9B%80)
2. 其他图片来源：CrossApp
3. Android自带素材

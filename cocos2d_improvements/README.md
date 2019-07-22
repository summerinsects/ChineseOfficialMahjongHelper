cocos2dx引擎源码修改
=========

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

### iOS的WebView如何禁止数字链接
- cocos2d/cocos/ui/UIWebViewImpl-ios.mm
- 找到函数`-(void)setupWebView`（141行），在`self.uiWebView.delegate = self;`后增加如下代码
```obj-c
    self.uiWebView.dataDetectorTypes = UIDataDetectorTypeNone;
```

#include "CheckBoxScale9.h"

USING_NS_CC;

static const int BACKGROUNDBOX_RENDERER_Z = (-1);
static const int BACKGROUNDSELECTEDBOX_RENDERER_Z = (-1);
static const int FRONTCROSS_RENDERER_Z = (-1);
static const int BACKGROUNDBOXDISABLED_RENDERER_Z = (-1);
static const int FRONTCROSSDISABLED_RENDERER_Z = (-1);

void CheckBoxScale9::initRenderer() {
    _backGroundBoxRenderer = ui::Scale9Sprite::create();
    _backGroundSelectedBoxRenderer = ui::Scale9Sprite::create();
    _frontCrossRenderer = ui::Scale9Sprite::create();
    _backGroundBoxDisabledRenderer = ui::Scale9Sprite::create();
    _frontCrossDisabledRenderer = ui::Scale9Sprite::create();

    addProtectedChild(_backGroundBoxRenderer, BACKGROUNDBOX_RENDERER_Z, -1);
    addProtectedChild(_backGroundSelectedBoxRenderer, BACKGROUNDSELECTEDBOX_RENDERER_Z, -1);
    addProtectedChild(_frontCrossRenderer, FRONTCROSS_RENDERER_Z, -1);
    addProtectedChild(_backGroundBoxDisabledRenderer, BACKGROUNDBOXDISABLED_RENDERER_Z, -1);
    addProtectedChild(_frontCrossDisabledRenderer, FRONTCROSSDISABLED_RENDERER_Z, -1);
}

void CheckBoxScale9::adaptRenderers() {
    if (_backGroundBoxRendererAdaptDirty) {
        ((ui::Scale9Sprite *)_backGroundBoxRenderer)->setCapInsets(Rect::ZERO);
        _backGroundBoxRenderer->setContentSize(_contentSize);
        _backGroundBoxRenderer->setPosition(_contentSize.width * 0.5f, _contentSize.height * 0.5f);
        _backGroundBoxRendererAdaptDirty = false;
    }
    if (_backGroundSelectedBoxRendererAdaptDirty) {
        ((ui::Scale9Sprite *)_backGroundSelectedBoxRenderer)->setCapInsets(Rect::ZERO);
        _backGroundSelectedBoxRenderer->setContentSize(_contentSize);
        _backGroundSelectedBoxRenderer->setPosition(_contentSize.width * 0.5f, _contentSize.height * 0.5f);
        _backGroundSelectedBoxRendererAdaptDirty = false;
    }
    if (_frontCrossRendererAdaptDirty) {
        ((ui::Scale9Sprite *)_frontCrossRenderer)->setCapInsets(Rect::ZERO);
        _frontCrossRenderer->setContentSize(_contentSize);
        _frontCrossRenderer->setPosition(_contentSize.width * 0.5f, _contentSize.height * 0.5f);
        _frontCrossRendererAdaptDirty = false;
    }
    if (_backGroundBoxDisabledRendererAdaptDirty) {
        ((ui::Scale9Sprite *)_backGroundBoxDisabledRenderer)->setCapInsets(Rect::ZERO);
        _backGroundBoxDisabledRenderer->setContentSize(_contentSize);
        _backGroundBoxDisabledRenderer->setPosition(_contentSize.width * 0.5f, _contentSize.height * 0.5f);
        _backGroundBoxDisabledRendererAdaptDirty = false;
    }
    if (_frontCrossDisabledRendererAdaptDirty) {
        ((ui::Scale9Sprite *)_frontCrossDisabledRenderer)->setCapInsets(Rect::ZERO);
        _frontCrossDisabledRenderer->setContentSize(_contentSize);
        _frontCrossDisabledRenderer->setPosition(_contentSize.width * 0.5f, _contentSize.height * 0.5f);
        _frontCrossDisabledRendererAdaptDirty = false;
    }
}

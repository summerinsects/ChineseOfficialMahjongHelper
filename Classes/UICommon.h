#ifndef __UI_COMMON_H__
#define __UI_COMMON_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

namespace UICommon {

    static inline cocos2d::ui::Button *createButton() {
        return cocos2d::ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    }

    static inline cocos2d::ui::CheckBox *createCheckBox() {
        return cocos2d::ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_check.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    }

    static inline cocos2d::ui::RadioButton *createRadioButton() {
        return cocos2d::ui::RadioButton::create("source_material/btn_radio_normal.png", "", "source_material/btn_radio_highlighted.png", "source_material/btn_radio_disabled.png", "source_material/btn_radio_disabled.png");
    }

    static inline cocos2d::ui::EditBox *createEditBox(const cocos2d::Size &size) {
        return cocos2d::ui::EditBox::create(size, cocos2d::ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    }

}

#endif

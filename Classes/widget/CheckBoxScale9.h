#ifndef __CHECK_BOX_SCALE_9_H__
#define __CHECK_BOX_SCALE_9_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class CheckBoxScale9 : public cocos2d::ui::CheckBox {
public:
    CREATE_FUNC(CheckBoxScale9);

protected:
    virtual void initRenderer() override;
    virtual void adaptRenderers() override;
};

#endif

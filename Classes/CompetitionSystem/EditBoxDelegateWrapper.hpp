
namespace {
    class EditBoxDelegateWrapper : public cocos2d::ui::EditBoxDelegate {
        const std::vector<cocos2d::ui::EditBox *> _editBoxes;

    public:
        EditBoxDelegateWrapper(const std::vector<cocos2d::ui::EditBox *> &editBoxes) : _editBoxes(editBoxes) { }

        virtual ~EditBoxDelegateWrapper() { CCLOG("%s", __FUNCTION__); }

        virtual void editBoxEditingDidBegin(cocos2d::ui::EditBox *editBox) override {
            // 实现点击后清除内容
            editBox->setPlaceHolder(editBox->getText());
            editBox->setText("");
        }

        virtual void editBoxReturn(cocos2d::ui::EditBox *editBox) override {
            // 如果没内容，则恢复成占位符
            const char *text = editBox->getText();
            if (Common::isCStringEmpty(text)) {
                editBox->setText(editBox->getPlaceHolder());
            }
        }

        virtual void editBoxEditingDidEndWithAction(cocos2d::ui::EditBox *editBox, EditBoxEndAction action) override {
            // 使得能连续输入
            if (action == EditBoxEndAction::TAB_TO_NEXT) {
                auto it = std::find(_editBoxes.begin(), _editBoxes.end(), editBox);
                if (it != _editBoxes.end() && ++it != _editBoxes.end()) {
                    editBox = *it;
                    editBox->scheduleOnce([editBox](float) {
                        editBox->openKeyboard();
                    }, 0.0f, "open_keyboard");
                }
            }
        }
    };
}

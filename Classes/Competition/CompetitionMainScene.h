#ifndef __COMPETITION_MAIN_SCENE__
#define __COMPETITION_MAIN_SCENE__

#include "../BaseScene.h"

class CompetitionMainScene : public BaseScene {
public:
    virtual bool init() override;

    CREATE_FUNC(CompetitionMainScene);

private:
    void showCompetitionCreatingAlert(const std::string &name, unsigned num, unsigned round);
};

#endif

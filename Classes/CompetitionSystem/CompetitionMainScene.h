#ifndef __COMPETITION_MAIN_SCENE_H__
#define __COMPETITION_MAIN_SCENE_H__

#include "../BaseScene.h"

class CompetitionData;

class CompetitionMainScene : public BaseScene {
public:
    virtual bool init() override;

    CREATE_FUNC(CompetitionMainScene);

private:
    void showNewCompetitionAlert(const std::string &name, size_t player, size_t round);

    std::shared_ptr<CompetitionData> _competitionData;
};

#endif

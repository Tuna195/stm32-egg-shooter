#include <gui/gameover_screen/GameOverView.hpp>
#include <gui/gameover_screen/GameOverPresenter.hpp>

GameOverPresenter::GameOverPresenter(GameOverView& v)
    : view(v)
{

}

void GameOverPresenter::activate()
{
	view.showScore(model->getFinalScore());
}

void GameOverPresenter::deactivate()
{

}

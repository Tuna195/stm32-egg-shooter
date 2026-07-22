#include <gui/gameover_screen/GameOverView.hpp>

GameOverView::GameOverView()
{

}

void GameOverView::setupScreen()
{
    GameOverViewBase::setupScreen();
    finalScore.setWildcard(scoreBuffer);
    Unicode::snprintf(scoreBuffer, 10, "%d", 0);
    finalScore.invalidate();
}

void GameOverView::tearDownScreen()
{
    GameOverViewBase::tearDownScreen();
}
void GameOverView::showScore(int s)
{
	Unicode::snprintf(scoreBuffer, 10, "%d", s);
    finalScore.invalidate();
}

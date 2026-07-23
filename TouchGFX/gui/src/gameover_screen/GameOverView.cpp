#include <gui/gameover_screen/GameOverView.hpp>

GameOverView::GameOverView()
{

}

void GameOverView::setupScreen()
{
    GameOverViewBase::setupScreen();
    Unicode::snprintf(scoreTextBuffer, SCORETEXT_SIZE, "%d", 0);
    finalScore.invalidate();
}

void GameOverView::tearDownScreen()
{
    GameOverViewBase::tearDownScreen();
}
void GameOverView::showScore(int s)
{
    Unicode::snprintf(scoreTextBuffer, SCORETEXT_SIZE, "%d", s);
    finalScore.invalidate();
}

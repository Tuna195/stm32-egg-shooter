#include <gui/gameover_screen/GameOverView.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Color.hpp>

GameOverView::GameOverView()
{
    highScoreText.setPosition(20, 174, 220, 24);
    highScoreText.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
    highScoreText.setLinespacing(0);
    Unicode::snprintf(highScoreTextBuffer, HIGHSCORETEXT_SIZE, "%d", 0);
    highScoreText.setWildcard(highScoreTextBuffer);
    highScoreText.setTypedText(touchgfx::TypedText(T_HIGHSCORETEXT));
    add(highScoreText);
}

void GameOverView::setupScreen()
{
    GameOverViewBase::setupScreen();
    finalScore.setPosition(20, 148, 210, 24);
    highScoreText.setPosition(20, 174, 220, 24);

    Unicode::snprintf(scoreTextBuffer, SCORETEXT_SIZE, "%d", 0);
    finalScore.invalidate();

    Unicode::snprintf(highScoreTextBuffer, HIGHSCORETEXT_SIZE, "%d", 0);
    highScoreText.invalidate();
}

void GameOverView::tearDownScreen()
{
    GameOverViewBase::tearDownScreen();
}
void GameOverView::showScore(int s, int highScore)
{
    Unicode::snprintf(scoreTextBuffer, SCORETEXT_SIZE, "%d", s);
    finalScore.invalidate();

    Unicode::snprintf(highScoreTextBuffer, HIGHSCORETEXT_SIZE, "%d", highScore);
    highScoreText.invalidate();
}

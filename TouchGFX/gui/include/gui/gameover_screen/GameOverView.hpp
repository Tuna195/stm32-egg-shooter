#ifndef GAMEOVERVIEW_HPP
#define GAMEOVERVIEW_HPP

#include <gui_generated/gameover_screen/GameOverViewBase.hpp>
#include <gui/gameover_screen/GameOverPresenter.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>

class GameOverView : public GameOverViewBase
{
public:
    GameOverView();
    virtual ~GameOverView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    void showScore(int s, int highScore);

private:
    static const uint16_t HIGHSCORETEXT_SIZE = 12;
    touchgfx::TextAreaWithOneWildcard highScoreText;
    touchgfx::Unicode::UnicodeChar highScoreTextBuffer[HIGHSCORETEXT_SIZE];
};

#endif // GAMEOVERVIEW_HPP

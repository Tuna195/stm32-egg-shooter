#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/common/FrontendApplication.hpp>
Screen2Presenter::Screen2Presenter(Screen2View& v)
    : view(v)
{

}

void Screen2Presenter::activate()
{

}

void Screen2Presenter::deactivate()
{

}
void Screen2Presenter::handleGameOver()
{
	model->setFinalScore(view.getScore());
    application().gotoGameOverScreenNoTransition();
}

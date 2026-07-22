#include <gui/common/FrontendApplication.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap)
{

}
FrontendApplication& application()
{
    return *static_cast<FrontendApplication*>(Application::getInstance());
}

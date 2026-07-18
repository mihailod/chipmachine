#include "LinkerView.h"

#include "../controller/LinkerController.h"
#include "../../../controllers/SongController.h"

namespace arkostracker 
{

LinkerView::LinkerView(LinkerController& pLinkerController) noexcept :
        linkerController(pLinkerController)
{
}

}   // namespace arkostracker

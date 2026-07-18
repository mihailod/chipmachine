#include "LinkerController.h"

#include "../../../controllers/SongController.h"

namespace arkostracker 
{

LinkerController::LinkerController(SongController& pSongController) noexcept :
        songController(pSongController)
{
}


}   // namespace arkostracker


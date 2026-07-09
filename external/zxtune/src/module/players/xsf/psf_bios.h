/**
 *
 * @file
 *
 * @brief  HighlyExperimental BIOS access interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/data.h"

namespace Module::PSF
{
  Binary::Data::Ptr GetSCPH10000HeBios();
}  // namespace Module::PSF

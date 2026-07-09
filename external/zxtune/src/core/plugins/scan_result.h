/**
 *
 * @file
 *
 * @brief  Helper for scanning result
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "analysis/result.h"

namespace Parameters
{
  class Accessor;
}

namespace ZXTune
{
  Analysis::Result::Ptr CreateUnmatchedResult(const Parameters::Accessor& params, Binary::Format::Ptr format,
                                              Binary::Container::Ptr data);
}  // namespace ZXTune

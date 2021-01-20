/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CLOCK_UTC_CLOCK_HPP
#define CPP_FILECOIN_CORE_CLOCK_UTC_CLOCK_HPP

#include "clock/time.hpp"

namespace fc::clock {
  /**
   * Provides current UTC time
   */
  class UTCClock {
   public:
    inline auto nowUTC() const {
      return std::chrono::duration_cast<UnixTime>(nowMicro());
    }
    virtual microseconds nowMicro() const = 0;
    virtual ~UTCClock() = default;
  };
}  // namespace fc::clock

#endif  // CPP_FILECOIN_CORE_CLOCK_UTC_CLOCK_HPP

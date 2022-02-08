/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/miner/power_pair.hpp"
#include "vm/actor/builtin/types/miner/quantize.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"
#include "vm/actor/builtin/types/universal/universal.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using primitives::SectorSize;
  using primitives::TokenAmount;

  struct ExpirationSet {
    RleBitset on_time_sectors;
    RleBitset early_sectors;
    TokenAmount on_time_pledge{};
    PowerPair active_power;
    PowerPair faulty_power;

    outcome::result<void> add(const RleBitset &on_time_sectors,
                              const RleBitset &early_sectors,
                              const TokenAmount &on_time_pledge,
                              const PowerPair &active_power,
                              const PowerPair &faulty_power);

    outcome::result<void> remove(const RleBitset &on_time_sectors,
                                 const RleBitset &early_sectors,
                                 const TokenAmount &on_time_pledge,
                                 const PowerPair &active_power,
                                 const PowerPair &faulty_power);

    bool isEmpty() const;
    uint64_t count() const;
    outcome::result<void> validateState() const;
  };
  CBOR_TUPLE(ExpirationSet,
             on_time_sectors,
             early_sectors,
             on_time_pledge,
             active_power,
             faulty_power)

  struct SectorEpochSet {
    ChainEpoch epoch{};
    RleBitset sectors;
    PowerPair power;
    TokenAmount pledge;
  };

  using PartitionExpirationsArray = adt::Array<ExpirationSet, 4>;

  struct ExpirationQueue {
    PartitionExpirationsArray queue;
    QuantSpec quant;

    using MutateFunction =
        std::function<outcome::result<std::tuple<bool, bool>>(ChainEpoch,
                                                              ExpirationSet &)>;

    virtual ~ExpirationQueue() = default;

    outcome::result<std::tuple<RleBitset, PowerPair, TokenAmount>>
    addActiveSectors(const std::vector<Universal<SectorOnChainInfo>> &sectors,
                     SectorSize ssize);

    outcome::result<void> rescheduleExpirations(
        ChainEpoch new_expiration,
        const std::vector<Universal<SectorOnChainInfo>> &sectors,
        SectorSize ssize);

    virtual outcome::result<PowerPair> rescheduleAsFaults(
        ChainEpoch new_expiration,
        const std::vector<Universal<SectorOnChainInfo>> &sectors,
        SectorSize ssize) = 0;

    virtual outcome::result<void> rescheduleAllAsFaults(
        ChainEpoch fault_expiration) = 0;

    outcome::result<PowerPair> rescheduleRecovered(
        const std::vector<Universal<SectorOnChainInfo>> &sectors,
        SectorSize ssize);

    outcome::result<std::tuple<RleBitset, RleBitset, PowerPair, TokenAmount>>
    replaceSectors(const std::vector<Universal<SectorOnChainInfo>> &old_sectors,
                   const std::vector<Universal<SectorOnChainInfo>> &new_sectors,
                   SectorSize ssize);

    outcome::result<std::tuple<ExpirationSet, PowerPair>> removeSectors(
        const std::vector<Universal<SectorOnChainInfo>> &sectors,
        const RleBitset &faults,
        const RleBitset &recovering,
        SectorSize ssize);

    outcome::result<ExpirationSet> popUntil(ChainEpoch until);

    outcome::result<void> add(ChainEpoch raw_epoch,
                              const RleBitset &on_time_sectors,
                              const RleBitset &early_sectors,
                              const PowerPair &active_power,
                              const PowerPair &faulty_power,
                              const TokenAmount &pledge);

    outcome::result<void> remove(ChainEpoch raw_epoch,
                                 const RleBitset &on_time_sectors,
                                 const RleBitset &early_sectors,
                                 const PowerPair &active_power,
                                 const PowerPair &faulty_power,
                                 const TokenAmount &pledge);

    virtual outcome::result<std::tuple<RleBitset, PowerPair, TokenAmount>>
    removeActiveSectors(
        const std::vector<Universal<SectorOnChainInfo>> &sectors,
        SectorSize ssize) = 0;

    outcome::result<void> traverseMutate(MutateFunction f);

    outcome::result<void> mustUpdateOrDelete(ChainEpoch epoch,
                                             const ExpirationSet &es);

    std::vector<SectorEpochSet> groupNewSectorsByDeclaredExpiration(
        SectorSize sector_size,
        const std::vector<Universal<SectorOnChainInfo>> &sectors) const;
  };

  Universal<ExpirationQueue> loadExpirationQueue(
      const PartitionExpirationsArray &expirations_epochs,
      const QuantSpec &quant);

  struct SectorExpirationSet {
    SectorEpochSet sector_epoch_set;
    ExpirationSet es;
  };

}  // namespace fc::vm::actor::builtin::types::miner

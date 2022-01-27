/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/block_validator/validator.hpp"

#include <gtest/gtest.h>

#include "cbor_blake/ipld_any.hpp"
#include "cbor_blake/memory.hpp"
#include "const.hpp"
#include "primitives/tipset/chain.hpp"
#include "proofs/proof_param_provider.hpp"
#include "storage/car/car.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/resources/resources.hpp"
#include "vm/actor/cgo/actors.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::blockchain::block_validator {
  using proofs::ProofParamProvider;
  using storage::InMemoryStorage;

  TEST(BlockValidator, Interopnet) {
    const auto params{ProofParamProvider::readJson(
                          "/var/tmp/filecoin-proof-parameters/parameters.json")
                          .value()};
    ProofParamProvider::getParams(params, 0).value();

    // Works on network version 13
    setParamsInteropnet();

    vm::actor::cgo::configParams();

    const auto cbipld{std::make_shared<MemoryCbIpld>()};
    EnvironmentContext envx;
    envx.ipld = std::make_shared<CbAsAnyIpld>(cbipld);
    const auto car{
        storage::car::loadCar(*envx.ipld, resourcePath("block_validator.car"))
            .value()};
    envx.ts_load = std::make_shared<primitives::tipset::TsLoadIpld>(envx.ipld);
    envx.interpreter_cache = std::make_shared<InterpreterCache>(
        std::make_shared<InMemoryStorage>(), cbipld);
    envx.ts_branches_mutex = std::make_shared<std::shared_mutex>();
    BlockValidator validator{std::make_shared<InMemoryStorage>(), envx};

    const auto head_tsk{*TipsetKey::make(car)};
    const auto ts_main{std::make_shared<TsBranch>()};
    const auto head{envx.ts_load->load(head_tsk).value()};
    auto ts{head};
    while (true) {
      ts_main->chain[ts->height()].key = ts->key;
      envx.interpreter_cache->set(ts->getParents(),
                                  {
                                      ts->getParentStateRoot(),
                                      ts->getParentMessageReceipts(),
                                      ts->getParentWeight(),
                                  });
      if (ts->height() == 0) {
        break;
      }
      ts = envx.ts_load->load(ts->getParents()).value();
    }
    ts = head;
    while (ts->height() != 0) {
      for (const auto &block : ts->blks) {
        validator.validate(ts_main, block).value();
      }
      ts = envx.ts_load->load(ts->getParents()).value();
    }
  }
}  // namespace fc::blockchain::block_validator

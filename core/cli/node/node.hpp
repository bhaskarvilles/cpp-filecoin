/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/format.h>

#include "api/rpc/client_setup.hpp"
#include "api/rpc/info.hpp"
#include "cli/cli.hpp"
#include "cli/try.hpp"
#include "common/libp2p/multi/multiaddress_fmt.hpp"
#include "node/node_version.hpp"

namespace fc::cli::_node {
  struct Node {
    struct Args {
      bool version{};
      boost::optional<boost::filesystem::path> repo;

      CLI_OPTS() {
        Opts opts;
        auto opt{opts.add_options()};
        opt("version,v", po::bool_switch(&version));
        opt("repo", po::value(&repo));
        return opts;
      }
    };
    CLI_RUN() {
      if (args.version) {
        fmt::print("{}\n", node::kNodeVersion);
        return;
      }
      throw ShowHelp{};
    }

    struct Api {
      std::shared_ptr<api::FullNodeApi> _;
      IoThread thread;
      std::shared_ptr<api::rpc::Client> wsc;

      Api(const ArgsMap &argm) {
        const auto &args{argm.of<Node>()};
        const auto info{cliTry(
            api::rpc::loadInfo(args.repo.value_or(""), "FULLNODE_API_INFO").o,
            "api info is missing")};
        _ = std::make_shared<api::FullNodeApi>();
        wsc = std::make_shared<api::rpc::Client>(*thread.io);
        wsc->setup(*_);
        cliTry(wsc->connect(info.first, "/rpc/v1", info.second),
               "connecting to {}",
               info.first);
      }
    };
  };
}  // namespace fc::cli::_node

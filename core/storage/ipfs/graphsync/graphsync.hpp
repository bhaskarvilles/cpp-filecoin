/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>

#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/protocol/common/subscription.hpp>

#include "common/buffer.hpp"
#include "common/libp2p/peer/cbor_peer_id.hpp"
#include "common/outcome.hpp"
#include "storage/ipfs/graphsync/extension.hpp"

namespace fc::storage::ipfs::graphsync {
  using libp2p::peer::PeerId;
  using libp2p::peer::PeerInfo;

  /// Subscription to any data stream, borrowed from libp2p
  using libp2p::protocol::Subscription;

  /// Response status codes. Positive values are received from wire,
  /// negative are internal. Terminal codes end request-response session
  enum ResponseStatusCode {
    // internal codes - terminal
    RS_NO_PEERS = -1,          // no peers: cannot find peer to connect to
    RS_CANNOT_CONNECT = -2,    // error during outbound connection establishment
    RS_TIMEOUT = -3,           // timeout occured in p2p communication
    RS_CONNECTION_ERROR = -4,  // network error (due to connection)
    RS_INTERNAL_ERROR = -5,    // internal error (due to local components)
    RS_REJECTED_LOCALLY = -6,  // request was rejected by local side
    RS_SLOW_STREAM = -7,       // slow stream: outbound buffers overflow

    // Other response codes are received from the network

    // info - partial
    RS_REQUEST_ACKNOWLEDGED = 10,  //   Request Acknowledged. Working on it.
    RS_ADDITIONAL_PEERS = 11,      //   Additional Peers. PeerIDs in extra.
    RS_NOT_ENOUGH_GAS = 12,        //   Not enough vespene gas ($)
    RS_OTHER_PROTOCOL = 13,        //   Other Protocol - info in extra.
    RS_PARTIAL_RESPONSE = 14,      //   Partial Response w/metadata
    RS_PAUSE = 15,

    // success - terminal
    RS_FULL_CONTENT = 20,     //   Request Completed, full content.
    RS_PARTIAL_CONTENT = 21,  //   Request Completed, partial content.

    // error - terminal
    RS_REJECTED = 30,        //   Request Rejected. NOT working on it.
    RS_TRY_AGAIN = 31,       //   Request failed, busy, try again later
    RS_REQUEST_FAILED = 32,  //   Request failed, for unknown reason.
    RS_LEGAL_ISSUES = 33,    //   Request failed, for legal reasons.
    RS_NOT_FOUND = 34,       //   Request failed, content not found.
  };

  /// Returns true if the status code passed is terminal, i.e. no more data will
  /// be sent to the subscription. See ResponseStatusCode comments
  bool isTerminal(ResponseStatusCode code);

  /// Returns true if the status code is terminal and successful
  bool isSuccess(ResponseStatusCode code);

  /// Returns true if the status code is an error code - internal or network
  bool isError(ResponseStatusCode code);

  /// Converts status code to string repr
  std::string statusCodeToString(ResponseStatusCode code);

  using GsId = int32_t;

  /// Full ID of remote request
  struct FullRequestId {
    /// Remote peer
    libp2p::peer::PeerId peer{codec::cbor::kDefaultT<PeerId>()};

    /// Unique ID generated by remote peer
    GsId id;
  };

  struct Request {
    CID root_cid;
    common::Buffer selector;
    std::vector<Extension> extensions;
    bool cancels_previous_requests;
  };

  struct Data {
    CID cid;
    common::Buffer content;
  };

  struct Response {
    ResponseStatusCode status;
    std::vector<Extension> extensions;
    std::vector<Data> data;
  };

  using Responder = std::function<boost::optional<Response>(bool)>;

  /// Graphsync protocol interface
  class Graphsync {
   public:
    virtual ~Graphsync() = default;

    using DataConnection = boost::signals2::connection;

    /// New data go through this callback
    using OnDataReceived = void(const libp2p::peer::PeerId &from,
                                const Data &data);

    /// Subscribes to data
    virtual DataConnection subscribe(std::function<OnDataReceived> handler) = 0;

    using RequestHandler = void(FullRequestId id, Request request);

    virtual void setDefaultRequestHandler(
        std::function<RequestHandler> handler) = 0;

    virtual void setRequestHandler(std::function<RequestHandler> handler,
                                   std::string extension_name) = 0;

    virtual void postResponse(const FullRequestId &id,
                              const Response &response) = 0;

    virtual void postBlocks(const FullRequestId &id, Responder responder) = 0;

    /// Starts instance and subscribes to blocks
    virtual void start() = 0;

    /// Stops the instance. Active requests will be cancelled and return
    /// RS_REJECTED_LOCALLY to their callbacks
    virtual void stop() = 0;

    /// Request progress subscription data
    using RequestProgressCallback = std::function<void(
        ResponseStatusCode code, std::vector<Extension> extensions)>;

    /// Initiates a new request to graphsync network
    /// \param peer Peer ID
    /// \param root_cid Root CID of the request
    /// \param selector IPLD selector
    /// \param extensions - extension data
    /// \param callback A callback which keeps track of request progress
    /// \return Subscription object. Request is cancelled as soon as
    /// this subscription is cancelled or goes out of scope
    virtual Subscription makeRequest(const libp2p::peer::PeerInfo &peer,
                                     const CID &root_cid,
                                     gsl::span<const uint8_t> selector,
                                     const std::vector<Extension> &extensions,
                                     RequestProgressCallback callback) = 0;
  };

}  // namespace fc::storage::ipfs::graphsync

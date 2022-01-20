#pragma once

#include "envoy/common/scope_tracker.h"
#include "envoy/common/time.h"
#include "envoy/config/core/v3/http_uri.pb.h"
#include "envoy/config/core/v3/resolver.pb.h"
#include "envoy/config/core/v3/udp_socket_config.pb.h"
#include "envoy/event/dispatcher_thread_deletable.h"
#include "envoy/event/file_event.h"
#include "envoy/event/scaled_timer.h"
#include "envoy/event/schedulable_cb.h"
#include "envoy/event/signal.h"
#include "envoy/event/timer.h"
#include "envoy/filesystem/watcher.h"
#include "envoy/network/connection.h"
#include "envoy/network/connection_handler.h"
#include "envoy/network/dns.h"
#include "envoy/network/listen_socket.h"
#include "envoy/network/listener.h"
#include "envoy/network/transport_socket.h"
#include "envoy/server/watchdog.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"
#include "envoy/stream_info/stream_info.h"
#include "envoy/thread/thread.h"

#include "source/common/http/message_impl.h"
#include "source/extensions/filters/http/common/jwks_fetcher.h"

#include "test/mocks/upstream/mocks.h"

#include "gmock/gmock.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Common {

class MockJwksFetcher : public JwksFetcher {
public:
  MOCK_METHOD(void, cancel, ());
  MOCK_METHOD(void, fetch,
              (const envoy::config::core::v3::HttpUri& uri, Tracing::Span& parent_span,
               JwksReceiver& receiver));
};

// A mock HTTP upstream.
class MockUpstream {
public:
  /**
   * Mock upstream which returns a given response body.
   */
  MockUpstream(Upstream::MockClusterManager& mock_cm, const std::string& status,
               const std::string& response_body);
  /**
   * Mock upstream which returns a given failure.
   */
  MockUpstream(Upstream::MockClusterManager& mock_cm, Http::AsyncClient::FailureReason reason);
  /**
   * Mock upstream which returns the given request.
   */
  MockUpstream(Upstream::MockClusterManager& mock_cm, Http::MockAsyncClientRequest* request);

private:
  Http::MockAsyncClientRequest request_;
  std::string status_;
  std::string response_body_;
};

class MockJwksReceiver : public JwksFetcher::JwksReceiver {
public:
  /* GoogleMock does handle r-value references hence the below construction.
   * Expectations and assertions should be made on onJwksSuccessImpl in place
   * of onJwksSuccess.
   */
  void onJwksSuccess(google::jwt_verify::JwksPtr&& jwks) override {
    ASSERT(jwks);
    onJwksSuccessImpl(*jwks.get());
  }
  MOCK_METHOD(void, onJwksSuccessImpl, (const google::jwt_verify::Jwks& jwks));
  MOCK_METHOD(void, onJwksError, (JwksFetcher::JwksReceiver::Failure reason));
};

class MockDispatcher : public Envoy::Event::Dispatcher {
public:
  MOCK_METHOD(const std::string&, name, ());
  MOCK_METHOD(Envoy::Event::FileEventPtr, createFileEvent, (os_fd_t fd, Envoy::Event::FileReadyCb cb, Envoy::Event::FileTriggerType trigger, uint32_t events));
  MOCK_METHOD(Event::TimerPtr, createTimer, (Envoy::Event::TimerCb cb));
  MOCK_METHOD(Event::TimerPtr, createScaledTimer, (Event::ScaledTimerType timer_type, Envoy::Event::TimerCb cb));
  MOCK_METHOD(Event::TimerPtr, createScaledTimer, (Event::ScaledTimerMinimum timer_min, Envoy::Event::TimerCb cb));
  MOCK_METHOD(Event::SchedulableCallbackPtr, createSchedulableCallback, (std::function<void()> cb));
  MOCK_METHOD(void, registerWatchdog, (const Server::WatchDogSharedPtr& watchdog, std::chrono::milliseconds mti));
  MOCK_METHOD(TimeSource&, timeSource, ());
  //MOCK_METHOD(MonotonicTime, approximateMonotonicTime, ());
  MOCK_METHOD(void, initializeStats, (Stats::Scope& s, const absl::optional<std::string>& prefix));
  MOCK_METHOD(void, clearDefferedDeleteList, ());
  MOCK_METHOD(Network::ServerConnectionPtr, createServerConnection,
              (Network::ConnectionSocketPtr&& s, Network::TransportSocketPtr&& t, StreamInfo::StreamInfo& si));
  MOCK_METHOD(Network::ClientConnectionPtr, createClientConnection,
              (Network::Address::InstanceConstSharedPtr a,
               Network::Address::InstanceConstSharedPtr s,
               Network::TransportSocketPtr&& t,
               const Network::ConnectionSocket::OptionsSharedPtr& options));
  MOCK_METHOD(Filesystem::WatcherPtr, createFilesystemWatcher, ());
  MOCK_METHOD(Network::ListenerPtr, createListener,
              (Network::SocketSharedPtr&& s, Network::TcpListenerCallbacks& cb, bool b, bool i));
  MOCK_METHOD(Network::UdpListenerPtr, createUdpListener,
              (Network::SocketSharedPtr s, Network::UdpListenerCallbacks& c, const envoy::config::core::v3::UdpSocketConfig& config));
  MOCK_METHOD(void, deferredDelete, (Envoy::Event::DeferredDeletablePtr&& t));
  MOCK_METHOD(void, exit, ());
  MOCK_METHOD(Envoy::Event::SignalEventPtr, listenForSignal, (signal_t s, Envoy::Event::SignalCb sa));
  MOCK_METHOD(void, deleteInDispatcherThread, (Envoy::Event::DispatcherThreadDeletableConstPtr d));
  MOCK_METHOD(void, run, (Envoy::Event::Dispatcher::RunType t));
  MOCK_METHOD(Buffer::WatermarkFactory&, getWatermarkFactory, ());
  MOCK_METHOD(void, updateApproximateMonotonicTime, ());
  MOCK_METHOD(void, shutdown, ());
};

} // namespace Common
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy

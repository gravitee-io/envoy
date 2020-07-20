1.17.3 (May 11, 2021)
=======================

Incompatible Behavior Changes
-----------------------------
*Changes that are expected to cause an incompatibility if applicable; deployment changes are likely required*

Minor Behavior Changes
----------------------
*Changes that may cause incompatibilities for some users, but should not for most*

Bug Fixes
---------
*Changes expected to improve the state of the world and are unlikely to have negative effects*

Removed Config or Runtime
-------------------------
*Normally occurs at the end of the* :ref:`deprecation period <deprecated>`

New Features
------------
* http: added the ability to :ref:`unescape slash sequences<envoy_v3_api_field_extensions.filters.network.http_connection_manager.v3.HttpConnectionManager.path_with_escaped_slashes_action>` in the path. Requests with unescaped slashes can be proxied, rejected or redirected to the new unescaped path. By default this feature is disabled. The default behavior can be overridden through :ref:`http_connection_manager.path_with_escaped_slashes_action<config_http_conn_man_runtime_path_with_escaped_slashes_action>` runtime variable. This action can be selectively enabled for a portion of requests by setting the :ref:`http_connection_manager.path_with_escaped_slashes_action_sampling<config_http_conn_man_runtime_path_with_escaped_slashes_action_enabled>` runtime variable.

Deprecated
----------
* cluster: HTTP configuration for upstream clusters has been reworked. HTTP-specific configuration is now done in the new :ref:`http_protocol_options <envoy_v3_api_msg_extensions.upstreams.http.v3.HttpProtocolOptions>` message, configured via the cluster's :ref:`extension_protocol_options<envoy_v3_api_field_config.cluster.v3.Cluster.typed_extension_protocol_options>`. This replaces explicit HTTP configuration in cluster config, including :ref:`upstream_http_protocol_options<envoy_v3_api_field_config.cluster.v3.Cluster.upstream_http_protocol_options>` :ref:`common_http_protocol_options<envoy_v3_api_field_config.cluster.v3.Cluster.common_http_protocol_options>` :ref:`http_protocol_options<envoy_v3_api_field_config.cluster.v3.Cluster.http_protocol_options>` :ref:`http2_protocol_options<envoy_v3_api_field_config.cluster.v3.Cluster.http2_protocol_options>` and :ref:`protocol_selection<envoy_v3_api_field_config.cluster.v3.Cluster.protocol_selection>`. Examples of before-and-after configuration can be found in the :ref:`http_protocol_options docs <envoy_v3_api_msg_extensions.upstreams.http.v3.HttpProtocolOptions>` and all of Envoy's example configurations have been updated to the new style of config.
* compression: the fields :ref:`content_length <envoy_v3_api_field_extensions.filters.http.compressor.v3.Compressor.content_length>`, :ref:`content_type <envoy_v3_api_field_extensions.filters.http.compressor.v3.Compressor.content_type>`, :ref:`disable_on_etag_header <envoy_v3_api_field_extensions.filters.http.compressor.v3.Compressor.disable_on_etag_header>`, :ref:`remove_accept_encoding_header <envoy_v3_api_field_extensions.filters.http.compressor.v3.Compressor.remove_accept_encoding_header>` and :ref:`runtime_enabled <envoy_v3_api_field_extensions.filters.http.compressor.v3.Compressor.runtime_enabled>` of the :ref:`Compressor <envoy_v3_api_msg_extensions.filters.http.compressor.v3.Compressor>` message have been deprecated in favor of :ref:`response_direction_config <envoy_v3_api_field_extensions.filters.http.compressor.v3.Compressor.response_direction_config>`.
* formatter: :ref:`text_format <envoy_v3_api_field_config.core.v3.SubstitutionFormatString.text_format>` is now deprecated in favor of :ref:`text_format_source <envoy_v3_api_field_config.core.v3.SubstitutionFormatString.text_format_source>`. To migrate existing text format strings, use the :ref:`inline_string <envoy_v3_api_field_config.core.v3.DataSource.inline_string>` field.
* gzip: :ref:`HTTP Gzip filter <config_http_filters_gzip>` is rejected now unless explicitly allowed with :ref:`runtime override <config_runtime_deprecation>` `envoy.deprecated_features.allow_deprecated_gzip_http_filter` set to `true`. Use the :ref:`compressor filter <config_http_filters_compressor>`.
* listener: :ref:`use_proxy_proto <envoy_v3_api_field_config.listener.v3.FilterChain.use_proxy_proto>` has been deprecated in favor of adding a :ref:`PROXY protocol listener filter <config_listener_filters_proxy_protocol>` explicitly.
* logging: the `--log-format-prefix-with-location` option is removed.
* ratelimit: the :ref:`dynamic metadata <envoy_v3_api_field_config.route.v3.RateLimit.Action.dynamic_metadata>` action is deprecated in favor of the more generic :ref:`metadata <envoy_v3_api_field_config.route.v3.RateLimit.Action.metadata>` action.
* stats: the `--use-fake-symbol-table` option is removed.
* tracing: OpenCensus :ref:`Zipkin configuration <envoy_api_field_config.trace.v2.OpenCensusConfig.zipkin_exporter_enabled>` is now deprecated, the preferred Zipkin export is via Envoy's :ref:`native Zipkin tracer <envoy_v3_api_msg_config.trace.v3.ZipkinConfig>`.

* build: alpine based debug image is deprecated in favor of :ref:`Ubuntu based debug image <install_binaries>`.
* cluster: the :ref:`track_timeout_budgets <envoy_v3_api_field_config.cluster.v3.Cluster.track_timeout_budgets>`
  field has been deprecated in favor of `timeout_budgets` part of an :ref:`Optional Configuration <envoy_v3_api_field_config.cluster.v3.Cluster.track_cluster_stats>`.
* ext_authz: the :ref:`dynamic metadata <envoy_v3_api_field_service.auth.v3.OkHttpResponse.dynamic_metadata>` field in :ref:`OkHttpResponse <envoy_v3_api_msg_service.auth.v3.OkHttpResponse>` has been deprecated in favor of :ref:`dynamic metadata <envoy_v3_api_field_service.auth.v3.CheckResponse.dynamic_metadata>` field in :ref:`CheckResponse <envoy_v3_api_msg_service.auth.v3.CheckResponse>`.
* hds: the :ref:`endpoints_health <envoy_v3_api_field_service.health.v3.EndpointHealthResponse.endpoints_health>`
  field has been deprecated in favor of :ref:`cluster_endpoints_health <envoy_v3_api_field_service.health.v3.EndpointHealthResponse.cluster_endpoints_health>` to maintain
  grouping by cluster and locality.
* router: the :ref:`include_vh_rate_limits <envoy_v3_api_field_config.route.v3.RouteAction.include_vh_rate_limits>` field has been deprecated in favor of :ref:`vh_rate_limits <envoy_v3_api_field_extensions.filters.http.ratelimit.v3.RateLimitPerRoute.vh_rate_limits>`.
* router: the :ref:`max_grpc_timeout <envoy_v3_api_field_config.route.v3.RouteAction.max_grpc_timeout>` field has been deprecated in favor of :ref:`grpc_timeout_header_max <envoy_v3_api_field_config.route.v3.RouteAction.MaxStreamDuration.grpc_timeout_header_max>`.
* router: the :ref:`grpc_timeout_offset <envoy_v3_api_field_config.route.v3.RouteAction.grpc_timeout_offset>` field has been deprecated in favor of :ref:`grpc_timeout_header_offset <envoy_v3_api_field_config.route.v3.RouteAction.MaxStreamDuration.grpc_timeout_header_offset>`.
* tap: the :ref:`match_config <envoy_v3_api_field_config.tap.v3.TapConfig.match_config>` field has been deprecated in favor of
  :ref:`match <envoy_v3_api_field_config.tap.v3.TapConfig.match>` field.
* router_check_tool: `request_header_fields`, `response_header_fields` config deprecated in favor of `request_header_matches`, `response_header_matches`.
* watchdog: :ref:`watchdog <envoy_v3_api_field_config.bootstrap.v3.Bootstrap.watchdog>` deprecated in favor of :ref:`watchdogs <envoy_v3_api_field_config.bootstrap.v3.Bootstrap.watchdogs>`.
* listener: fix crash when disabling or re-enabling listeners due to overload while processing LDS updates.
* proxy_proto: fixed a bug where network filters would not have the correct downstreamRemoteAddress() when accessed from the StreamInfo. This could result in incorrect enforcement of RBAC rules in the RBAC network filter (but not in the RBAC HTTP filter), or incorrect access log addresses from tcp_proxy.
* tls: fix read resumption after triggering buffer high-watermark and all remaining request/response bytes are stored in the SSL connection's internal buffers.
* udp: fixed issue in which receiving truncated UDP datagrams would cause Envoy to crash.
* vrp: allow supervisord to open its log file.
* docs: fix docs for v1.15.1.
* http: fixed CVE-2020-25017. Previously header matching did not match on all headers for non-inline
  headers. This patch changes the default behavior to always logically match on all headers.
  Multiple individual headers will be logically concatenated with ',' similar to what is done with
  inline headers. This makes the behavior effectively consistent. This behavior can be temporary
  reverted by setting the runtime value `envoy.reloadable_features.header_match_on_all_headers` to
  "false".

  Targeted fixes have been additionally performed on the following extensions which make them
  consider all duplicate headers by default as a comma concatenated list:
    1. Any extension using CEL matching on headers.
    2. The header to metadata filter.
    3. The JWT filter.
    4. The Lua filter.

  Like primary header matching used in routing, RBAC, etc. this behavior can be disabled by setting
  the runtime value `envoy.reloadable_features.header_match_on_all_headers` to false.
* http: the setCopy() header map API previously only set the first header in the case of duplicate
  non-inline headers. setCopy() now behaves similarly to the other set*() APIs and replaces all found
  headers with a single value. This may have had security implications in the extauth filter which
  uses this API. This behavior can be disabled by setting the runtime value
  `envoy.reloadable_features.http_set_copy_replace_all_headers` to false.
1.15.1 (Pending)
================

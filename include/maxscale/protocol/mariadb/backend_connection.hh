/*
 * Copyright (c) 2019 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2025-01-25
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */
#pragma once

#include <maxscale/ccdefs.hh>
#include <maxscale/protocol/mariadb/protocol_classes.hh>

#include <queue>

class MariaDBBackendConnection : public mxs::BackendConnection
{
public:
    using Iter = mxs::Buffer::iterator;

    static std::unique_ptr<MariaDBBackendConnection>
    create(MXS_SESSION* session, mxs::Component* component, SERVER& server);

    ~MariaDBBackendConnection() override;

    void ready_for_reading(DCB* dcb) override;
    void write_ready(DCB* dcb) override;
    void error(DCB* dcb) override;
    void hangup(DCB* dcb) override;

    int32_t write(GWBUF* buffer) override;

    void    finish_connection() override;
    bool    reuse_connection(BackendDCB* dcb, mxs::Component* upstream) override;
    bool    established() override;
    void    ping() override;
    bool    can_close() const override;
    bool    is_idle() const override;
    int64_t seconds_idle() const override;
    json_t* diagnostics() const override;

    void              set_dcb(DCB* dcb) override;
    const BackendDCB* dcb() const override;
    BackendDCB*       dcb() override;

    uint64_t thread_id() const;
    uint32_t server_capabilities {0};   /**< Server capabilities TODO: private */

private:
    enum class State
    {
        HANDSHAKING,        /**< Handshaking with backend */
        AUTHENTICATING,     /**< Authenticating with backend */
        CONNECTION_INIT,    /**< Sending connection init file contents */
        SEND_DELAYQ,        /**< Sending contents of delay queue */
        ROUTING,            /**< Ready to route queries */
        CHANGING_USER,      /**< Processing a COM_CHANGE_USER sent by the user */
        RESET_CONNECTION,   /**< Reset the connection with a COM_CHANGE_USER */
        PINGING,            /**< Pinging backend server */
        SEND_HISTORY,       /**< Sending stored session command history */
        READ_HISTORY,       /**< Reading results of history execution */
        PREPARE_PS,         /**< Executing a COM_STMT_PREPARE */
        FAILED,             /**< Handshake/authentication failed */
    };

    enum class HandShakeState
    {
        SEND_PROHY_HDR, /**< Send proxy protocol header */
        EXPECT_HS,      /**< Expecting initial server handshake */
        START_SSL,      /**< Send SSLRequest and start SSL */
        SSL_NEG,        /**< Negotiating SSL */
        SEND_HS_RESP,   /**< Send handshake response */
        COMPLETE,       /**< Handshake complete */
        FAIL,           /**< Handshake failed */
    };

    enum class StateMachineRes
    {
        IN_PROGRESS,// The SM should be called again once more data is available.
        DONE,       // The SM is complete for now, the protocol may advance to next state.
        ERROR,      // The SM encountered an error. The connection should be closed.
    };

    State          m_state {State::HANDSHAKING};                /**< Connection state */
    HandShakeState m_hs_state {HandShakeState::SEND_PROHY_HDR}; /**< Handshake state */

    SERVER&                  m_server;          /**< Connected backend server */
    mariadb::SBackendAuth    m_authenticator;   /**< Authentication plugin */
    mariadb::BackendAuthData m_auth_data;       /**< Data shared with auth plugin */

    /**
     * Packets received from router while the connection was busy handshaking/authenticating.
     * Sent to server once connection is ready. */
    std::vector<mxs::Buffer> m_delayed_packets;

    /**
     * Contains information about custom connection initialization queries.
     */
    struct InitQueryStatus
    {
        enum class State
        {
            SENDING,
            RECEIVING,
        };
        State state {State::SENDING};

        int ok_packets_expected {0};    /**< OK packets expected in total */
        int ok_packets_received {0};    /**< OK packets received so far */
    };
    InitQueryStatus m_init_query_status;

    MariaDBBackendConnection(SERVER& server);

    StateMachineRes handshake();
    StateMachineRes authenticate();
    StateMachineRes send_connection_init_queries();
    bool            send_delayed_packets();
    void            normal_read();

    void            send_history();
    StateMachineRes read_history_response();
    void            compare_response();

    bool backend_write_delayqueue(GWBUF* buffer);

    bool   change_user(GWBUF* queue);
    bool   send_change_user_to_backend();
    int    read_change_user();
    bool   send_proxy_protocol_header();
    int    handle_persistent_connection(GWBUF* queue);
    GWBUF* create_change_user_packet();
    void   read_com_ping_response();
    void   do_handle_error(DCB* dcb, const std::string& errmsg,
                           mxs::ErrorType type = mxs::ErrorType::TRANSIENT);
    void prepare_for_write(GWBUF* buffer);

    GWBUF* track_response(GWBUF** buffer);
    bool   mxs_mysql_is_result_set(GWBUF* buffer);
    bool   read_backend_handshake(mxs::Buffer&& buffer);
    void   handle_error_response(DCB* plain_dcb, GWBUF* buffer);
    bool   session_ok_to_route(DCB* dcb);
    bool   complete_ps_response(GWBUF* buffer);
    bool   handle_auth_change_response(GWBUF* reply, DCB* dcb);
    int    send_mysql_native_password_response(DCB* dcb, GWBUF* reply);
    bool   expecting_text_result();
    bool   expecting_ps_response();
    void   mxs_mysql_parse_ok_packet(GWBUF* buff, size_t packet_offset, size_t packet_len);
    int    gw_decode_mysql_server_handshake(uint8_t* payload);
    GWBUF* gw_generate_auth_response(bool with_ssl, bool ssl_established, uint64_t service_capabilities);

    std::string create_response_mismatch_error();

    uint32_t create_capabilities(bool with_ssl, bool db_specified, uint64_t capabilities);
    GWBUF*   process_packets(GWBUF** result);
    void     process_one_packet(Iter it, Iter end, uint32_t len);
    void     process_reply_start(Iter it, Iter end);
    void     process_result_start(Iter it, Iter end);
    void     process_ps_response(Iter it, Iter end);
    void     process_ok_packet(Iter it, Iter end);
    void     update_error(mxs::Buffer::iterator it, mxs::Buffer::iterator end);
    bool     consume_fetched_rows(GWBUF* buffer);
    void     set_reply_state(mxs::ReplyState state);

    // Contains the necessary information required to track queries
    struct TrackedQuery
    {
        explicit TrackedQuery(GWBUF* buffer);

        uint32_t payload_len = 0;
        uint8_t  command = 0;
        bool     opening_cursor = false;
        uint32_t id = 0;
    };

    void track_query(const TrackedQuery& query);

    /**
     * Set associated client protocol session and upstream. Should be called after creation or when swapping
     * sessions. Also initializes authenticator plugin.
     *
     * @param session The new session to read client data from
     * @param upstream The new upstream to send server replies to
     */
    void assign_session(MXS_SESSION* session, mxs::Component* upstream);

    static std::string to_string(State auth_state);

    uint64_t    m_thread_id {0};                    /**< Backend thread id, received in backend handshake */
    bool        m_collect_result {false};           /**< Collect the next result set as one buffer */
    bool        m_track_state {false};              /**< Track session state */
    bool        m_skip_next {false};
    uint64_t    m_num_coldefs {0};
    uint32_t    m_num_eof_packets {0};  /**< Encountered eof packet number, used for check packet type */
    mxs::Buffer m_collectq;             /**< Used to collect results when resultset collection is requested */
    int64_t     m_ps_packets {0};
    bool        m_opening_cursor = false;   /**< Whether we are opening a cursor */
    uint32_t    m_expected_rows = 0;        /**< Number of rows a COM_STMT_FETCH is retrieving */
    bool        m_large_query = false;
    mxs::Reply  m_reply;

    std::queue<TrackedQuery> m_track_queue;

    // The mapping of COM_STMT_PREPARE IDs we sent upstream to the actual IDs that the backend sent us
    std::unordered_map<uint32_t, uint32_t> m_ps_map;

    // The internal ID of the current query
    uint32_t m_current_id {0};

    // The ID and response to the command that will be added to the history. This is stored in a separate
    // variable in case the correct response that is delivered to the client isn't available when this backend
    // receive the response.
    std::pair<uint32_t, bool> m_id_to_check {0, false};

    // The responses to the history that's being replayed. The IDs are not needed as we know any future
    // commands will be queued until we complete the history replay.
    std::deque<bool> m_history_responses;

    mxs::Component* m_upstream {nullptr};       /**< Upstream component, typically a router */
    MXS_SESSION*    m_session {nullptr};        /**< Generic session */
    BackendDCB*     m_dcb {nullptr};            /**< Dcb used by this protocol connection */
};

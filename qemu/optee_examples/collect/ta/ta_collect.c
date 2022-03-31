#include <tee_internal_api.h>
#include <introspection_ta.h>
#include <tee_tcpsocket.h>
#include <tee_isocket.h>
#include <string.h>
#include <stdlib.h>

#define TCP_SERVER "receiver"

struct sock_handle
{
    TEE_iSocketHandle ctx;
    TEE_iSocket *socket;
};

TEE_Result TA_CreateEntryPoint(void)
{
    DMSG("TA_CreateEntryPoint has been called");

    return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
    DMSG("TA_DestroyEntryPoint has been called");
}

struct sock_handle h = {0};

TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
                                    TEE_Param __maybe_unused params[4],
                                    void __maybe_unused **sess_ctx)
{
    uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
                                               TEE_PARAM_TYPE_NONE,
                                               TEE_PARAM_TYPE_NONE,
                                               TEE_PARAM_TYPE_NONE);

    DMSG("TA_OpenSessionEntryPoint has been called");

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    /* Unused parameters */
    (void)&params;
    (void)&sess_ctx;

    TEE_Result res = TEE_ERROR_GENERIC;
    TEE_tcpSocket_Setup setup = {0};
    uint32_t timeout = 10;
    uint32_t proto_error;

    setup.ipVersion = TEE_IP_VERSION_4;
    setup.server_port = 8080;
    setup.server_addr = TCP_SERVER;

    // tcp socket open
    h.socket = TEE_tcpSocket;
    res = h.socket->open(&h.ctx, &setup, &proto_error);
    if (res != TEE_SUCCESS)
        return TEE_ISOCKET_ERROR_REMOTE_CLOSED;

    return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
    (void)&sess_ctx; /* Unused parameter */
}

//外部のTCPサーバにデータを送信
static TEE_Result send_event_log(char *ab, size_t *length)
{
    TEE_Result res = TEE_ERROR_GENERIC;
    uint32_t timeout = TEE_TIMEOUT_INFINITE;
    struct sock_handle *h_control1 = NULL;

    IMSG("send_event_log called");

    TEE_tcpSocket_Setup setup = {0};
    uint32_t proto_error;

    setup.ipVersion = TEE_IP_VERSION_4;
    setup.server_port = 8080;
    setup.server_addr = TCP_SERVER;

    // tcp socket open
    h.socket = TEE_tcpSocket;
    res = h.socket->open(&h.ctx, &setup, &proto_error);
    if (res != TEE_SUCCESS)
        return TEE_ISOCKET_ERROR_REMOTE_CLOSED;

    // tcp send data
    h_control1 = &h;

    res = h_control1->socket->send(h_control1->ctx, ab, &length, timeout);
    if (res != TEE_SUCCESS)
        return TEE_ISOCKET_ERROR_TIMEOUT;

    h_control1->socket->close(h_control1->ctx);

    return TEE_SUCCESS;
}

// CAが作成したshared_memoryからデータを読み込み，表示する．
static TEE_Result show_recv_msg(uint32_t param_types,
                                TEE_Param params[4])
{
    char *ab;
    size_t msg_len;
    uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                                               TEE_PARAM_TYPE_NONE,
                                               TEE_PARAM_TYPE_NONE,
                                               TEE_PARAM_TYPE_NONE);
    TEE_Result res;
    // DMSG("show recv msg has been called");

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    msg_len = params[0].memref.size;
    ab = TEE_Malloc(msg_len, TEE_MALLOC_FILL_ZERO);

    TEE_MemMove(ab, params[0].memref.buffer, msg_len);

    // IMSG("len: %d", msg_len);
    res = send_event_log(ab, msg_len);

    if (res != TEE_SUCCESS)
    {
        return TEE_ISOCKET_ERROR_REMOTE_CLOSED;
    }

    TEE_Free(ab);
    return TEE_SUCCESS;
}

// tcp open, connectのテスト関数
// server_addr:server_portにTCP接続を行い，bufを送信する
static TEE_Result test_tcp_connect(uint32_t paramt_types, TEE_Param params[4])
{
    TEE_Result res = TEE_ERROR_GENERIC;
    struct sock_handle h = {};
    TEE_tcpSocket_Setup setup = {};
    uint32_t *proto_error;
    uint32_t timeout = 10;
    char buf[] = "auditctl -a exit,always  -F success=0 -S kill";
    uint32_t *length = sizeof(buf);
    struct sock_handle *h_control1 = NULL;

    IMSG("test_tcp_connect called!");
    IMSG("%s len: %d", buf, &length);

    setup.ipVersion = TEE_IP_VERSION_4;
    setup.server_port = 8080;
    setup.server_addr = TCP_SERVER;

    // tcp socket open
    h.socket = TEE_tcpSocket;
    res = h.socket->open(&h.ctx, &setup, &proto_error);

    // res = linuxaudit_plugin_ping(buf, length);

    if (res != TEE_SUCCESS)
        return TEE_ISOCKET_ERROR_REMOTE_CLOSED;

    // tcp send data
    h_control1 = &h;

    h_control1->socket->send(h_control1->ctx, &buf, &length, timeout);
    h_control1->socket->close(h_control1->ctx);
    return res;
}

/*
 * Called when a TA is invoked. sess_ctx hold that value that was
 * assigned by TA_OpenSessionEntryPoint(). The rest of the paramters
 * comes from normal world.
 */
TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
                                      uint32_t cmd_id,
                                      uint32_t param_types, TEE_Param params[4])
{
    (void)&sess_ctx; /* Unused parameter */

    switch (cmd_id)
    {
    case TA_COLLECT_TCP_TEST:
        return test_tcp_connect(param_types, params); // default
    case TA_COLLECT_SEND_MSG:
        return show_recv_msg(param_types, params);
    default:
        return TEE_ERROR_BAD_PARAMETERS;
    }
}

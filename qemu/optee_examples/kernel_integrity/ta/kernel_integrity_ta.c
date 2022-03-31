/*
 * Copyright (c) 2016, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <pta_memory_introspection.h>
#include <kernel_integrity_ta.h>
#include <tee_tcpsocket.h>
#include <tee_isocket.h>
#include <string.h>

#define TCP_SERVER "receiver"

struct sock_handle
{
	    TEE_iSocketHandle ctx;
	    TEE_iSocket *socket;
};

struct sock_handle h = {0}; 
/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{
	DMSG("has been called");

	return TEE_SUCCESS;
}

/*
 * Called when the instance of the TA is destroyed if the TA has not
 * crashed or panicked. This is the last call in the TA.
 */
void TA_DestroyEntryPoint(void)
{
	DMSG("has been called");
}

/*
 * Called when a new session is opened to the TA. *sess_ctx can be updated
 * with a value to be able to identify this session in subsequent calls to the
 * TA. In this function you will normally do the global initialization for the
 * TA.
 */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param __maybe_unused params[4],
		void __maybe_unused **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Unused parameters */
	(void)&params;
	(void)&sess_ctx;

	/*
	 * The DMSG() macro is non-standard, TEE Internal API doesn't
	 * specify any means to logging from a TA.
	 */

	/* If return value != TEE_SUCCESS the session will not be created. */
	return TEE_SUCCESS;
}

/*
 * Called when a session is closed, sess_ctx hold the value that was
 * assigned by TA_OpenSessionEntryPoint().
 */
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */
	IMSG("Goodbye!\n");
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

    //tcp socket open
    h.socket = TEE_tcpSocket;
    res = h.socket->open(&h.ctx, &setup, &proto_error);
    if (res != TEE_SUCCESS)
        return TEE_ISOCKET_ERROR_REMOTE_CLOSED;

    //tcp send data
    h_control1 = &h;

    res = h_control1->socket->send(h_control1->ctx, ab, &length, timeout);
    if (res != TEE_SUCCESS)
        return TEE_ISOCKET_ERROR_TIMEOUT;

    h_control1->socket->close(h_control1->ctx);

    return TEE_SUCCESS;
}

void pta_call(void *buf, uint32_t *len){
    TEE_Result res;
    TEE_TASessionHandle sess;
    TEE_UUID uuid = PTA_MEMORY_INTROSPECTION_UUID;
    unsigned char *hash_result;
    uint32_t hash_size;

    TEE_Param params[TEE_NUM_PARAMS];
    uint32_t param_types;

    param_types = TEE_PARAM_TYPES(
        TEE_PARAM_TYPE_VALUE_INOUT, TEE_PARAM_TYPE_MEMREF_OUTPUT,
        TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);

    memset(params, 0, sizeof(params));

    //params[1].memref.buffer = (void *)"hogeeee";
    params[1].memref.buffer = buf;
    params[1].memref.size = *len;

    DMSG("PTA_Inovke has been called");
    
    res = TEE_OpenTASession(&uuid, 
                            TEE_TIMEOUT_INFINITE, 
                            0, NULL, &sess, NULL);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_Open TA Session failed");
        return res;
    }

    TEE_InvokeTACommand(sess, TEE_TIMEOUT_INFINITE, PTA_CMD_GET_DEVICES_SUPP,
                              param_types, params, NULL);
    DMSG("PTA invoke success");
    
    TEE_CloseTASession(sess);
}


TEE_Result pta_invoke(uint32_t parama_types, TEE_Param params[4]) {
    TEE_Result res;
    TEE_TASessionHandle sess;
    TEE_UUID uuid = PTA_MEMORY_INTROSPECTION_UUID;
 
    uint32_t a = 100;
    char *buf = "hogeeee";
    char *hash_result = NULL;
    size_t hash_length = 0;
    uint32_t exp_parama_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
                                                TEE_PARAM_TYPE_MEMREF_OUTPUT,
                                                TEE_PARAM_TYPE_NONE,
                                                TEE_PARAM_TYPE_NONE);

    if (parama_types != exp_parama_types){
        DMSG("host tee parameter is diffrent");
        return TEE_ERROR_BAD_PARAMETERS;
    }

    //pta_call(buf, &a);

    // TEE_MemMove(hash_result, params[0].memref.buffer, params[0].memref.size);
    res = TEE_OpenTASession(&uuid, 
                            TEE_TIMEOUT_INFINITE, 
                            0, NULL, &sess, NULL);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_Open TA Session failed");
        return res;
    }

    TEE_InvokeTACommand(sess, TEE_TIMEOUT_INFINITE, PTA_CMD_GET_DEVICES_SUPP,
                              parama_types, params, NULL);
    DMSG("PTA invoke success");

    TEE_CloseTASession(sess);
    
    DMSG("buffffer: %s", params[1].memref.buffer);
    hash_result = params[1].memref.buffer;
    hash_length = strlen(hash_result);

    send_event_log(hash_result, hash_length);

    return TEE_SUCCESS;
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

	switch (cmd_id) {
	case TA_KERNEL_INTEGRITY_CMD_INC_VALUE:
		return pta_invoke(param_types, params);
	case TA_KERNEL_INTEGRITY_CMD_DEC_VALUE:
		//return dec_value(param_types, params);
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}



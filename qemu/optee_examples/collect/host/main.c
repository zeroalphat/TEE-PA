#include <err.h>
#include <stdio.h>
#include <string.h>

#include <tee_client_api.h>
#include <introspection_ta.h>

int main(int argc, char **argv)
{
    TEEC_Result res;
    TEEC_Context ctx;
    TEEC_Session sess;
    TEEC_Operation op;
    TEEC_UUID uuid = TA_INTROSPECTION_UUID;
    uint32_t err_origin;
    volatile uintptr_t vaddr, paddr = 0;
    uintptr_t srt, end;
    int i, count;

    res = TEEC_InitializeContext(NULL, &ctx);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

    res = TEEC_OpenSession(&ctx, &sess, &uuid,
                           TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_OpenSession failed with code 0x%x", res);

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE,
                                     TEEC_NONE, TEEC_NONE);

    res = TEEC_InvokeCommand(&sess, TA_COLLECT_TCP_TEST, &op, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_Invokecommand failed with code 0x%x", res, err_origin);

    TEEC_CloseSession(&sess);
    TEEC_FinalizeContext(&ctx);

    return 0;
}

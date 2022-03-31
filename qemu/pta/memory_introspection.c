#include <pta_memory_introspection.h>
#include <config.h>
#include <kernel/early_ta.h>
#include <kernel/linker.h>
#include <kernel/pseudo_ta.h>
#include <kernel/tee_ta_manager.h>
#include <pta_device.h>
#include <string.h>
#include <stdio.h>
#include <tee/uuid.h>
#include <user_ta_header.h>
// sha256
#include <tee/tee_cryp_utl.h>
#include <tee/tee_svc.h>

#include <mbedtls/sha256.h>
#include <mbedtls/md5.h>
#include <mbedtls/md.h>

#include <kernel/msg_param.h>
#include <kernel/user_ta.h>
#include <kernel/tee_ta_manager.h>
#include <mm/core_mmu.h>
#include <tee/tee_svc.h>
#include <io.h>

#define PTA_NAME "memory_introspection.pta"

#define STEXT_PA 0x77610000
#define ETEXT_PA 0x784A0000

//refer to https://stackoverflow.com/a/57723897
char HexLookUp[] = "0123456789abcdef";
void bytes2hex(unsigned char *src, char *out, int len)
{
    while (len--)
    {
        *out++ = HexLookUp[*src >> 4];
        *out++ = HexLookUp[*src & 0x0F];
        src++;
    }
    *out = 0;
}

// PAをVAに変換
vaddr_t nsec_periph_base(paddr_t pa, size_t len)
{
    if (cpu_mmu_enabled())
        return (vaddr_t)phys_to_virt(pa, MEM_AREA_RAM_NSEC);
    return (vaddr_t)pa;
}

static void mem_hash(void *payload, size_t payloadLength)
{
    int res;
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    uint8_t hashResult[32];
    char buffer[sizeof(hashResult) * 2 + 1];

    DMSG("payload: %ld length:%ld", payload, payloadLength);

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    res = mbedtls_md_update(&ctx, (const unsigned char *)payload, payloadLength);
    if (res != 0)
        EMSG("mbedtls_md_update failed");
    mbedtls_md_finish(&ctx, hashResult);
    mbedtls_md_free(&ctx);

    DMSG("uint8_t %u", hashResult);

    bytes2hex(hashResult, buffer, sizeof(hashResult));

    DMSG("%s's sha256 sum %s", payload, buffer);
}

static TEE_Result get_memory_address(uint32_t types,
                                     TEE_Param params[TEE_NUM_PARAMS],
                                     uint32_t rflags)
{
    TEE_Result res;
    TEE_UUID uuid;
    const size_t nslen = 5;
    char *buf = NULL;
    uint32_t blen = 0;
    DMSG("get_memory_address invoked!");

    char *payload = "Hello SHA 256!";
    const size_t payloadLength = strlen(payload);
    mem_hash(payload, payloadLength);
    DMSG("buffer: %s", buf);

    // check param types
    uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT,
                                               TEE_PARAM_TYPE_NONE,
                                               TEE_PARAM_TYPE_NONE,
                                               TEE_PARAM_TYPE_NONE);

    if (types != exp_param_types)
    {
        EMSG("pta: param:%ld exp: %ld", types, exp_param_types);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    vaddr_t stext_addr = nsec_periph_base(STEXT_PA, 1);
    DMSG("va stext address: %lx", stext_addr);
    vaddr_t etext_addr = nsec_periph_base(ETEXT_PA, 1);
    DMSG("va etext address: %lx", etext_addr);

    uint8_t hashResult[32];
    size_t text_length;
    text_length = etext_addr - stext_addr;

    mem_hash(stext_addr, text_length);
    DMSG("buffer: %s", buf);

    DMSG("memory hash success");
    // io_write32(stext_addr + 10, 0);

    return TEE_SUCCESS;
}

static TEE_Result communication_test_ta_to_pta(uint32_t types, TEE_Param params[TEE_NUM_PARAMS])
{
    char *payload = "Hello SHA 256!";
    const size_t payloadLength = strlen(payload);
    char *buf;
    uint32_t test[32];
    uint32_t a = 50;
    char test_value[5] = "fuga";
    int res;
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    uint8_t hashResult[32];
    char buffer[sizeof(hashResult) * 2 + 1];

    // check param types
    uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
                                               TEE_PARAM_TYPE_MEMREF_OUTPUT,
                                               TEE_PARAM_TYPE_NONE,
                                               TEE_PARAM_TYPE_NONE);

    if (types != exp_param_types)
    {
        EMSG("pta: param:%ld exp: %ld different", types, exp_param_types);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    DMSG("params[1] %s size: %d", params[1].memref.buffer, params[1].memref.size);

    // caluculate hash
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    res = mbedtls_md_update(&ctx, (const unsigned char *)payload, payloadLength);
    if (res != 0)
        EMSG("mbedtls_md_update failed");
    mbedtls_md_finish(&ctx, hashResult);
    mbedtls_md_free(&ctx);

    bytes2hex(hashResult, buffer, sizeof(hashResult));

    // DMSG("%s's sha256 sum %s", payload, buffer);

    memcpy(params[1].memref.buffer, buffer, 64);
    DMSG("PTA progress done!");

    return TEE_SUCCESS;
}

static TEE_Result invoke_command(void *pSessionContext __unused,
                                 uint32_t nCommandID, uint32_t nParamTypes,
                                 TEE_Param pParams[TEE_NUM_PARAMS])
{
    switch (nCommandID)
    {
    case PTA_CMD_GET_ADDRESS:
        return get_memory_address(nParamTypes, pParams,
                                  TA_FLAG_DEVICE_ENUM);
    case PTA_CMD_GET_HASH:
        return communication_test_ta_to_pta(nParamTypes, pParams);
    default:
        break;
    }

    return TEE_ERROR_NOT_IMPLEMENTED;
}

pseudo_ta_register(.uuid = PTA_MEMORY_INTROSPECTION_UUID, .name = PTA_NAME,
                   .flags = PTA_DEFAULT_FLAGS,
                   .invoke_command_entry_point = invoke_command);

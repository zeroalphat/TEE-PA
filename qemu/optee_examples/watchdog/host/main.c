#include <err.h>
#include <stdio.h>
#include <string.h>

#include <tee_client_api.h>

#define WDT_UUID \
		{ 0x44c9884e, 0x9e5f, 0x48d9, \
			{ 0x9b, 0x78, 0x4d, 0x14, 0x1a, 0x2f, 0xec, 0xab } }

#define GPIO_ON             0
#define GPIO_OFF            1
#define TEST_PSEUDO_TA      2

int main(void)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = WDT_UUID;
	uint32_t err_origin;

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT,
                                                   TEEC_NONE,
                                                   TEEC_NONE,
                                                   TEEC_NONE);


	res = TEEC_InvokeCommand(&sess, TEST_PSEUDO_TA, &op,
				 &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);
	
	
	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                                   TEEC_NONE,
                                                   TEEC_NONE,
                                                   TEEC_NONE);


	res = TEEC_InvokeCommand(&sess, GPIO_ON, &op,
				 &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);

	TEEC_CloseSession(&sess);

	TEEC_FinalizeContext(&ctx);

	return 0;
}
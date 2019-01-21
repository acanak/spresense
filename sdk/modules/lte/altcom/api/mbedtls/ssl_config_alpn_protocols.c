/****************************************************************************
 * modules/lte/altcom/api/mbedtls/ssl_config_alpn_protocols.c
 *
 *   Copyright 2018 Sony Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor Sony nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>
#include "dbg_if.h"
#include "altcom_errno.h"
#include "altcom_seterrno.h"
#include "apicmd_config_alpn_protocols.h"
#include "apiutil.h"
#include "mbedtls/ssl.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONFIG_ALPN_PROTOCOLS_REQ_DATALEN (sizeof(struct apicmd_config_alpn_protocols_s))
#define CONFIG_ALPN_PROTOCOLS_RES_DATALEN (sizeof(struct apicmd_config_alpn_protocolsres_s))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct config_alpn_protocols_req_s
{
  uint32_t id;
  char     **protos;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int32_t config_alpn_protocols_request(FAR struct config_alpn_protocols_req_s *req)
{
  int32_t                                      ret;
  uint16_t                                     reslen = 0;
  char                                         **p;
  FAR struct apicmd_config_alpn_protocols_s    *cmd = NULL;
  FAR struct apicmd_config_alpn_protocolsres_s *res = NULL;

  /* Allocate send and response command buffer */

  if (!altcom_mbedtls_alloc_cmdandresbuff(
    (FAR void **)&cmd, APICMDID_TLS_CONFIG_ALPN_PROTOCOLS,
    CONFIG_ALPN_PROTOCOLS_REQ_DATALEN,
    (FAR void **)&res, CONFIG_ALPN_PROTOCOLS_RES_DATALEN))
    {
      return MBEDTLS_ERR_SSL_ALLOC_FAILED;
    }

  /* Fill the data */

  cmd->conf = htonl(req->id);
  p = req->protos;
  memset(cmd->protos1, 0, APICMD_CONFIG_ALPN_PROTOCOLS_PROTOS_LEN);
  if (*p != NULL)
    {
      strncpy((char*)cmd->protos1, *p, APICMD_CONFIG_ALPN_PROTOCOLS_PROTOS_LEN-1);
      p++;
    }
  memset(cmd->protos2, 0, APICMD_CONFIG_ALPN_PROTOCOLS_PROTOS_LEN);
  if (*p != NULL)
    {
      strncpy((char*)cmd->protos2, *p, APICMD_CONFIG_ALPN_PROTOCOLS_PROTOS_LEN-1);
      p++;
    }
  memset(cmd->protos3, 0, APICMD_CONFIG_ALPN_PROTOCOLS_PROTOS_LEN);
  if (*p != NULL)
    {
      strncpy((char*)cmd->protos3, *p, APICMD_CONFIG_ALPN_PROTOCOLS_PROTOS_LEN-1);
      p++;
    }
  memset(cmd->protos4, 0, APICMD_CONFIG_ALPN_PROTOCOLS_PROTOS_LEN);
  if (*p != NULL)
    {
      strncpy((char*)cmd->protos4, *p, APICMD_CONFIG_ALPN_PROTOCOLS_PROTOS_LEN-1);
      p++;
    }

  DBGIF_LOG1_DEBUG("[config_alpn_protocols]config id: %d\n", req->id);
  DBGIF_LOG1_DEBUG("[config_alpn_protocols]protos1: %s\n", cmd->protos1);
  DBGIF_LOG1_DEBUG("[config_alpn_protocols]protos2: %s\n", cmd->protos2);
  DBGIF_LOG1_DEBUG("[config_alpn_protocols]protos3: %s\n", cmd->protos3);
  DBGIF_LOG1_DEBUG("[config_alpn_protocols]protos4: %s\n", cmd->protos4);

  /* Send command and block until receive a response */

  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res,
                      CONFIG_ALPN_PROTOCOLS_RES_DATALEN, &reslen,
                      SYS_TIMEO_FEVR);

  if (ret < 0)
    {
      DBGIF_LOG1_ERROR("apicmdgw_send error: %d\n", ret);
      goto errout_with_cmdfree;
    }

  if (reslen != CONFIG_ALPN_PROTOCOLS_RES_DATALEN)
    {
      DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
      goto errout_with_cmdfree;
    }

  ret = ntohl(res->ret_code);

  DBGIF_LOG1_DEBUG("[config_alpn_protocols res]ret: %d\n", ret);

  altcom_mbedtls_free_cmdandresbuff(cmd, res);

  return ret;

errout_with_cmdfree:
  altcom_mbedtls_free_cmdandresbuff(cmd, res);
  return MBEDTLS_ERR_SSL_ALLOC_FAILED;
}



/****************************************************************************
 * Public Functions
 ****************************************************************************/

int mbedtls_ssl_conf_alpn_protocols(mbedtls_ssl_config *conf, const char **protos)
{
  int32_t                            result;
  struct config_alpn_protocols_req_s req;

  if (!altcom_isinit())
    {
      DBGIF_LOG_ERROR("Not intialized\n");
      altcom_seterrno(ALTCOM_ENETDOWN);
      return MBEDTLS_ERR_SSL_ALLOC_FAILED;
    }

  req.id = conf->id;
  req.protos = (char**)protos;

  result = config_alpn_protocols_request(&req);

  return result;
}

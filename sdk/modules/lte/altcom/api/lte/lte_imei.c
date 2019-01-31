/****************************************************************************
 * modules/lte/altcom/api/lte/lte_imei.c
 *
 *   Copyright 2018 Sony Semiconductor Solutions Corporation
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
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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

#include <stdint.h>
#include <errno.h>

#include "lte/lte_api.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "apicmd_imei.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GETIMEI_DATA_LEN (0)

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern get_imei_cb_t g_getimei_callback;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: getimei_job
 *
 * Description:
 *   This function is an API callback for get IMEI.
 *
 * Input Parameters:
 *  arg    Pointer to received event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void getimei_job(FAR void *arg)
{
  int32_t                               ret;
  int32_t                               result;
  FAR struct apicmd_cmddat_getimeires_s *data;
  get_imei_cb_t                         callback;

  data = (FAR struct apicmd_cmddat_getimeires_s *)arg;
  ALTCOM_GET_AND_CLR_CALLBACK(ret, g_getimei_callback, callback);

  if ((ret == 0) && (callback))
    {
      result = (int32_t)data->result;

      callback(result, (int8_t*)data->imei);
    }
  else
    {
      DBGIF_LOG_ERROR("Unexpected!! callback is NULL.\n");
    }

  /* In order to reduce the number of copies of the receive buffer,
   * bring a pointer to the receive buffer to the worker thread.
   * Therefore, the receive buffer needs to be released here. */

  altcom_free_cmd((FAR uint8_t *)arg);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lte_get_imei
 *
 * Description:
 *   Get International Mobile Equipment Identifier.
 *
 * Input Parameters:
 *   callback Callback function to notify that get of IMEI is completed.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

int32_t lte_get_imei(get_imei_cb_t callback)
{
  int32_t     ret;
  FAR uint8_t *cmdbuff;

  /* Return error if callback is NULL */

  if (!callback)
    {
      DBGIF_LOG_ERROR("Input argument is NULL.\n");
      return -EINVAL;
    }

  /* Check if the library is initialized */

  if (!altcom_isinit())
    {
      DBGIF_LOG_ERROR("Not intialized\n");
      ret = -EPERM;
    }
  else
    {
      /* Register API callback */

      ALTCOM_REG_CALLBACK(ret, g_getimei_callback, callback);
      if (ret < 0)
        {
          DBGIF_LOG_ERROR("Currently API is busy.\n");
        }
    }

  /* Accept the API */

  if (ret == 0)
    {
      /* Allocate API command buffer to send */

      cmdbuff = apicmdgw_cmd_allocbuff(APICMDID_GET_IMEI,
        GETIMEI_DATA_LEN);
      if (!cmdbuff)
        {
          DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
          ret = -ENOMEM;
        }
      else
        {
          /* Send API command to modem */

          ret = altcom_send_and_free(cmdbuff);
        }

      /* If fail, there is no opportunity to execute the callback,
       * so clear it here. */

      if (ret < 0)
        {
          /* Clear registered callback */

          ALTCOM_CLR_CALLBACK(g_getimei_callback);
        }
      else
        {
          ret = 0;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: apicmdhdlr_imei
 *
 * Description:
 *   This function is an API command handler for IMEI get result.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_GET_IMEI_RES,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_imei(FAR uint8_t *evt, uint32_t evlen)
{
  return apicmdhdlrbs_do_runjob(evt, APICMDID_CONVERT_RES(APICMDID_GET_IMEI),
    getimei_job);
}

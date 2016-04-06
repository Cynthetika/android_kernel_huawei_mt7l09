/******************************************************************************

                  ��Ȩ���� (C), 2001-2011, ��Ϊ�������޹�˾

 ******************************************************************************
  �� �� ��   : Omapprl.c
  �� �� ��   : ����
  ��    ��   : ���� 46160
  ��������   : 2008��6��11��
  ����޸�   :
  ��������   : R�ӿ�����ת��ģ��
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2008��6��11��
    ��    ��   : ���� 46160
    �޸�����   : �����ļ�

******************************************************************************/

/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "omrl.h"
#include "omringbuffer.h"
#include "omprivate.h"
#include "cpm.h"
#include "OmHdlcInterface.h"
#include "OmAppRl.h"
#include "OmApp.h"
#include "omsdlog.h"
#include "OmToolInterface.h"
#include "SCMProc.h"
#include "ombufmngr.h"
#include "AtAppVcomInterface.h"

#if (FEATURE_ON == FEATURE_MERGE_OM_CHAN)
#include "CbtPpm.h"
#include "SCMSoftDecode.h"
#include "SOCPInterface.h"
#endif
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#define    THIS_FILE_ID        PS_FILE_ID_ACPU_OMRL_C

#if (FEATURE_OFF == FEATURE_MERGE_OM_CHAN)
/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
OMRL_MSG_COMBINE_INFO_STRU              g_stMsgCombineInfo;

/*���ƻ��������ʵĻ����ź���*/
VOS_SEM                                 g_AcpuOmTxBuffSem;

/* ��¼��pc�ൽue��ʱ��������������Ϣ */
OM_ACPU_PC_UE_SUC_STRU                  g_stAcpuPcToUeSucRecord;

/* ��¼���ݴ�pc�ൽue��ʱ�����ĸ������ */
OM_ACPU_PC_UE_FAIL_STRU                 g_stAcpuPcToUeErrRecord;

/*��¼��PC�෢�����ݵ�������Ϣ */
OM_UE_PC_SUC_STRU                       g_stAcpuUeToPcSucRecord;

/*��¼��PC�෢������ʱ�����ĸ������*/
OM_UE_PC_FAIL_STRU                      g_stAcpuUeToPcErrRecord;

#if (FEATURE_ON == FEATURE_CBT_LOG)
OMRL_MSG_COMBINE_INFO_STRU              g_stCBTMsgCombineInfo;

/* ��¼��pc�ൽue��ʱ��������������Ϣ */
OM_ACPU_PC_UE_SUC_STRU                  g_stCBTAcpuPcToUeSucRecord;

/* ��¼���ݴ�pc�ൽue��ʱ�����ĸ������ */
OM_ACPU_PC_UE_FAIL_STRU                 g_stCBTAcpuPcToUeErrRecord;
#endif

/*��¼��ʼʱ��*/
VOS_UINT32                              g_ulAcpuLastTick = 0;

/*��¼�ֽڳ���*/
VOS_UINT32                              g_ulAcpuLastByteCnt = 0;

/*ָ��OM�ְ�ͷ���ݵ�ָ��*/
OM_APP_MSG_SEG_EX_STRU                 *g_pstAcpuSegMsgEx = VOS_NULL_PTR;

/* HDLC���ƽṹ */
OM_HDLC_STRU                            g_stOmHdlcEntity;

OM_HDLC_STRU                            g_stNVHdlcEntity;

#if (FEATURE_ON == FEATURE_CBT_LOG)
OM_HDLC_STRU                            g_stCBTHdlcEntity;
#endif

/* ��������OM�Ĵ�ӡ��� */
#ifdef OAM_UT_DEBUG
VOS_UINT32                              g_ulOmAcpuDbgFlag = VOS_NULL_DWORD;
#else
VOS_UINT32                              g_ulOmAcpuDbgFlag = VOS_FALSE;
#endif

/* OM���������ƿ� */
OM_BUF_CTRL_STRU                        g_stAcpuOmBufCtrl;

OM_NV_MNTN_RECORD_STRU                 *g_pstRecordOmToApp = VOS_NULL_PTR;
VOS_UINT32                              g_ulNvOmToAppCount = 0;

OM_NV_MNTN_RECORD_STRU                 *g_pstRecordAppToOm = VOS_NULL_PTR;
VOS_UINT32                              g_ulNvAppToOmCount = 0;

#if (VOS_LINUX == VOS_OS_VER)
struct semaphore                        g_stOmTxBuffSem;
#endif

#if (FEATURE_ON == FEATURE_CBT_LOG)
VOS_UINT32                              g_ulCBTStatus = OM_NOT_CBT_MSG;
VOS_UINT32                              g_ulCBTEstablishStatus = 0;
VOS_UINT8                              *g_pucCBTHdlcEncBuf;

#if (VOS_LINUX == VOS_OS_VER)
struct semaphore                        g_stOmRxCBTBuffSem;
#endif

#endif

VOS_UINT32                              g_ulCBTLogEnable = 0;

/* ����SD����Debug ��Ϣ */
extern OM_SD_DEBUG_INFO                 g_stOmSDDebugInfo;
extern OM_VCOM_DEBUG_INFO               g_stVComDebugInfo;

#if (FEATURE_ON == FEATURE_CBT_LOG)
extern OM_VCOM_CBT_LOG_DEBUG_INFO     g_stVComCBTDebugInfo;
#endif

extern OM_RECORD_BUF_STRU               g_astAcpuRecordInfo[VOS_EXC_DUMP_MEM_NUM_BUTT];

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/


VOS_UINT32 OMRL_AcpuCpuIdToPid(VOS_UINT8 ucCpuId, VOS_UINT32 *pulPid)
{

      switch(ucCpuId)
      {
        case OM_TOOL_RSV_ID:
        case OM_TOOL_CCPU_ID:
            *pulPid = WUEPS_PID_OM;/* ccpu om pid */
            break;
        case OM_TOOL_ACPU_ID:
            *pulPid = ACPU_PID_OM; /* acpu om pid */
            break;
        case OM_TOOL_HIFI_ID:
            *pulPid = DSP_PID_HIFI_OM;/* HIFI om pid */
            break;
        case OM_TOOL_MCU_ID:
            *pulPid = ACPU_PID_MCU_OM;/* MCU om pid */
            break;
        default:
            /* ��ӡ���� */
            return VOS_ERR;
      }

      return VOS_OK;
}
VOS_VOID OMRL_AcpuRecordNvInfo(VOS_VOID)
{
    APP_OM_MSG_EX_STRU                 *pstAppToOmMsg;
    OM_SW_VER_FLAG_STRU                 stCustomVersion;
    VOS_UINT32                          ulLen;

    if ( VOS_OK != NV_Read(en_NV_Item_SW_VERSION_FLAG, &stCustomVersion, sizeof(stCustomVersion)) )
    {
        stCustomVersion.ulNvSwVerFlag = VOS_TRUE;
    }

    /* ���ulCustomVersion == VOS_TRUE��ʾ������Ƭ�汾����ֱ�ӷ��� */
    if ( (VOS_TRUE == stCustomVersion.ulNvSwVerFlag))
    {
        return;
    }

    /* ��Ƭ�汾��FTMģʽ�ż�¼ */
    if(VOS_TRUE != g_ulAcpuFTMFlag)
    {
        return;
    }

    pstAppToOmMsg = (APP_OM_MSG_EX_STRU*)g_stMsgCombineInfo.pstWholeMsg->aucValue;

    /* �ж����� */
    if ( (OM_WRF_FUNC == ((pstAppToOmMsg->ucFuncType) & OM_FUNCID_VALUE_BITS))
         ||(OM_GRF_FUNC == ((pstAppToOmMsg->ucFuncType) & OM_FUNCID_VALUE_BITS))
         ||(OM_QUERY_FUNC == ((pstAppToOmMsg->ucFuncType) & OM_FUNCID_VALUE_BITS))
         ||(OM_NV_FUNC == ((pstAppToOmMsg->ucFuncType) & OM_FUNCID_VALUE_BITS)))
    {
        g_pstRecordAppToOm[g_ulNvAppToOmCount].ulSlice = VOS_GetSlice();
        g_pstRecordAppToOm[g_ulNvAppToOmCount].ulFlag  = 0x55555555;

        /* ��Ϣ���ȼ����ֽڵ�functype��cpuid */
        ulLen = pstAppToOmMsg->usLength + 2U;

        if (ulLen > NV_MNTN_SAVE_MAX_MSG_LEN)
        {
            ulLen = NV_MNTN_SAVE_MAX_MSG_LEN;
        }

        /*lint -e670*/
        VOS_MemCpy(g_pstRecordAppToOm[g_ulNvAppToOmCount].aucVaule, g_stMsgCombineInfo.pstWholeMsg->aucValue, ulLen);
        /*lint +e670*/

        g_ulNvAppToOmCount++;

        if(SAVE_MAX_SEND_INFO == g_ulNvAppToOmCount)
        {
             g_ulNvAppToOmCount = 0;
        }
    }

    return;
}


VOS_UINT32 OMRL_AcpuMsgDispatch(VOS_VOID)
{
    VOS_UINT32                       ulRslt;

    /*lint -e40*/
    OM_ACPU_DEBUG_TRACE(g_stMsgCombineInfo.pstWholeMsg->aucValue, g_stMsgCombineInfo.pstWholeMsg->ulLength, OM_ACPU_DISPATCH_MSG);
    /*lint +e40*/

    /* CCPU����Ϣͨ��ICCͨ������ */
    if (WUEPS_PID_OM == g_stMsgCombineInfo.pstWholeMsg->ulReceiverPid)
    {
        ulRslt = GU_OamSndPcMsgToCcpu(g_stMsgCombineInfo.pstWholeMsg->aucValue, g_stMsgCombineInfo.pstWholeMsg->ulLength);

        g_stAcpuPcToUeSucRecord.stCcpuData.ulDataLen += g_stMsgCombineInfo.pstWholeMsg->ulLength;
        g_stAcpuPcToUeSucRecord.stCcpuData.ulNum++;

        OMRL_AcpuRecordNvInfo();

        /* �ͷ���Ϣ�ռ� */
        VOS_FreeMsg(ACPU_PID_OM, g_stMsgCombineInfo.pstWholeMsg);
    }
    else if (ACPU_PID_OM == g_stMsgCombineInfo.pstWholeMsg->ulReceiverPid)
    {
        /* ����cpu����Ϣͨ��VOS_SendMsg���� */
        g_stAcpuPcToUeSucRecord.stAcpuData.ulDataLen += g_stMsgCombineInfo.pstWholeMsg->ulLength;
        g_stAcpuPcToUeSucRecord.stAcpuData.ulNum++;

        ulRslt = VOS_SendMsg(ACPU_PID_OM, g_stMsgCombineInfo.pstWholeMsg);
    }
    else if (DSP_PID_HIFI_OM == g_stMsgCombineInfo.pstWholeMsg->ulReceiverPid)
    {
        /* ����cpu����Ϣͨ��VOS_SendMsg���� */
        g_stAcpuPcToUeSucRecord.stHifiData.ulDataLen += g_stMsgCombineInfo.pstWholeMsg->ulLength;
        g_stAcpuPcToUeSucRecord.stHifiData.ulNum++;

        ulRslt = VOS_SendMsg(ACPU_PID_OM, g_stMsgCombineInfo.pstWholeMsg);
    }
    else
    {
        /* ����cpu����Ϣͨ��VOS_SendMsg���� */
        g_stAcpuPcToUeSucRecord.stMcuData.ulDataLen += g_stMsgCombineInfo.pstWholeMsg->ulLength;
        g_stAcpuPcToUeSucRecord.stMcuData.ulNum++;

        ulRslt = VOS_SendMsg(ACPU_PID_OM, g_stMsgCombineInfo.pstWholeMsg);
    }

    g_stMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

    if (VOS_OK != ulRslt )
    {
        return VOS_ERR;
    }

    return VOS_OK;

}
VOS_UINT32 OMRL_AcpuMsgCombine(VOS_UINT8 *pucData, VOS_UINT16 usLen)
{
    VOS_UINT32                       ulRslt = VOS_OK;/*Get the return value of PS_SEND_MSG.*/
    VOS_UINT8                        ucCurrentSegSn;/*Reserve the current packet's SN.*/
    OM_APP_MSG_SEGMENT_STRU          *pstOmAppSeg;
    APP_OM_GU_MSG_STRU               *pstAppOmMsg;

    /*Check the input parameter's validity.Make sure empty segment can't pass*/
    if ( (VOS_NULL_PTR == pucData) ||
        (usLen <= sizeof(OM_APP_MSG_SEGMENT_STRU)) )
    {
        LogPrint("OMRL_AcpuMsgCombine: The input is wrong.");

        g_stAcpuPcToUeErrRecord.usLenTooShortErr++;

        return VOS_ERR;
    }

    pstOmAppSeg = (OM_APP_MSG_SEGMENT_STRU *)pucData;

    /*Check the input parameter's validity.*/
    if (usLen != ((pstOmAppSeg->usSegLen + 3) & ~0x3))
    {
        LogPrint("OMRL_AcpuMsgCombine: The input length is wrong.");

        g_stAcpuPcToUeErrRecord.usSegLenErr++;

        return VOS_ERR;
    }

    usLen = pstOmAppSeg->usSegLen;

    ucCurrentSegSn = pstOmAppSeg->ucSegSn;

    /*When the MSG packet is new, we need initialize the static variable.*/
    if (1 == ucCurrentSegSn)
    {
        if (VOS_NULL_PTR != g_stMsgCombineInfo.pstWholeMsg)
        {
            VOS_FreeMsg(ACPU_PID_OM, g_stMsgCombineInfo.pstWholeMsg);
            g_stMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

            g_stAcpuPcToUeErrRecord.usPacketLostErr++;
        }

        /*The new MSG packet is coming.*/
        g_stMsgCombineInfo.ucExpectedSegSn = 1;
        g_stMsgCombineInfo.ulMoveLen       = 0;

        if (usLen < (sizeof(APP_OM_GU_MSG_STRU) + OM_APP_MSG_SEGMENT_HEADER_LENGTH))
        {
            LogPrint("OMRL_AcpuMsgCombine: The input parameter is wrong.");

            g_stAcpuPcToUeErrRecord.usMsgLenErr++;

            return VOS_ERR;
        }

        pstAppOmMsg = (APP_OM_GU_MSG_STRU *)(pucData + OM_APP_MSG_SEGMENT_HEADER_LENGTH);

        g_stMsgCombineInfo.ulTotalMsgLen = pstAppOmMsg->usLength + APP_OM_HEADER_LENGTH;

        /* �����ɺ��ж��Ƿ������ֵ */
        if (OM_RL_TOTAL_MSG_MAX_LEN < g_stMsgCombineInfo.ulTotalMsgLen)
        {
            g_stAcpuPcToUeErrRecord.usMsgTooLongErr++;

            LogPrint1("OMRL_AcpuMsgCombine: receive first msg pack is too long %d;\r\n",
                        (VOS_INT32)g_stMsgCombineInfo.ulTotalMsgLen);

            /* �������֧�ֱ���1024�ֽڵ����ݣ�����û���ṩ�꣬����ֱ��ʹ������ */
            DRV_SYSTEM_ERROR(OAM_PC_LENGTH_TOO_BIG, (VOS_INT)g_stMsgCombineInfo.ulTotalMsgLen, 0, (VOS_CHAR *)pucData, 1024);

            return VOS_ERR;
        }

        /*Allocate the memory space.*/
        g_stMsgCombineInfo.pstWholeMsg = VOS_AllocMsg(ACPU_PID_OM,  g_stMsgCombineInfo.ulTotalMsgLen);
        if (VOS_NULL_PTR == g_stMsgCombineInfo.pstWholeMsg)
        {
            LogPrint("OMRL_AcpuMsgCombine: VOS_AllocMsg fail.");

            g_stAcpuPcToUeErrRecord.usAllocMsg++;

            return VOS_ERR;
        }

        /* ����CPU ID���ҵ���Ӧ��PID�����ڹ����·�����֡����VOS_OM_HEADER�ṹ�б����ֶα�ʾCPUID */
        ulRslt = OMRL_AcpuCpuIdToPid(pstAppOmMsg->ucCpuId, &g_stMsgCombineInfo.pstWholeMsg->ulReceiverPid);
        if (VOS_OK != ulRslt)
        {
            /* �ͷ���Ϣ�ռ� */
            VOS_FreeMsg(ACPU_PID_OM, g_stMsgCombineInfo.pstWholeMsg);

            g_stMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

            g_stAcpuPcToUeErrRecord.usCpuIdErr++;

            /* �˴�����ֱ�ӷ��أ����ü�¼�����������Ѿ���¼��CPU ID�������� */
            return VOS_ERR;
        }

        g_stMsgCombineInfo.ucTotalSegNum = pstOmAppSeg->ucSegNum;
    }

    if ( g_stMsgCombineInfo.ucTotalSegNum != pstOmAppSeg->ucSegNum )
    {
        LogPrint("OMRL_AcpuMsgCombine: The length of the packet is biger than the size of allocated memory.");

        /* �ͷ���Ϣ�ռ� */
        VOS_FreeMsg(ACPU_PID_OM, g_stMsgCombineInfo.pstWholeMsg);

        g_stMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

        g_stAcpuPcToUeErrRecord.usSegNumErr++;

        return VOS_ERR;
    }

    /*The expected MSG pakcet is lost.*/
    if (g_stMsgCombineInfo.ucExpectedSegSn != ucCurrentSegSn)
    {
        /*Print the error info.*/
        LogPrint2("OMRL_AcpuMsgCombine: expected SN is %d, current SN is %d.", g_stMsgCombineInfo.ucExpectedSegSn, ucCurrentSegSn);

        /* �ͷ���Ϣ�ռ� */
        VOS_FreeMsg(ACPU_PID_OM, g_stMsgCombineInfo.pstWholeMsg);

        g_stMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

        g_stAcpuPcToUeErrRecord.usMsgSnErr++;

        return VOS_ERR;
    }

    /*We make sure that the memory can't be violated.*/
    if (g_stMsgCombineInfo.ulTotalMsgLen < (g_stMsgCombineInfo.ulMoveLen + (usLen - OM_APP_MSG_SEGMENT_HEADER_LENGTH)))
    {
        LogPrint("OMRL_AcpuMsgCombine: The length of the packet is biger than the size of allocated memory.\n");

        /* �ͷ���Ϣ�ռ� */
        VOS_FreeMsg(ACPU_PID_OM, g_stMsgCombineInfo.pstWholeMsg);

        g_stMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

        g_stAcpuPcToUeErrRecord.usNoMemErr++;

        return VOS_ERR;
    }

    VOS_MemCpy((VOS_UINT8*)(g_stMsgCombineInfo.pstWholeMsg->aucValue) + g_stMsgCombineInfo.ulMoveLen,
               pucData + OM_APP_MSG_SEGMENT_HEADER_LENGTH, usLen - OM_APP_MSG_SEGMENT_HEADER_LENGTH);
    g_stMsgCombineInfo.ulMoveLen += usLen - OM_APP_MSG_SEGMENT_HEADER_LENGTH;
    g_stMsgCombineInfo.ucExpectedSegSn++;

    /*If the current MSG packet is an complete packet.*/
    if (g_stMsgCombineInfo.ucTotalSegNum == ucCurrentSegSn)
    {
        /* �����ɺ��ж��Ƿ������ֵ */
        if (OM_RL_TOTAL_MSG_MAX_LEN < g_stMsgCombineInfo.ulTotalMsgLen)
        {
            g_stAcpuPcToUeErrRecord.usMsgTooLongErr++;

            /* �ͷ���Ϣ�ռ� */
            VOS_FreeMsg(ACPU_PID_OM, g_stMsgCombineInfo.pstWholeMsg);

            g_stMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

            return VOS_ERR;
        }

        /*It will send the MSG to OM module.*/
        ulRslt = OMRL_AcpuMsgDispatch();

        return ulRslt;
    }

    return VOS_OK;
}


VOS_UINT32 OMRL_AcpuSendMsg(VOS_UINT8 *pucData, VOS_UINT32 ulDataLen, VOS_UINT32 ulSenderPid, VOS_UINT32 ulReceiverPid)
{

    MsgBlock           *pstAppOmMsg = VOS_NULL_PTR;

    pstAppOmMsg = VOS_AllocMsg(ulSenderPid, ulDataLen);
    if (VOS_NULL_PTR == pstAppOmMsg)
    {
        return VOS_ERR;
    }

    pstAppOmMsg->ulReceiverPid = ulReceiverPid;

    /* copy ��Ϣ���� */
    VOS_MemCpy((VOS_UINT8 *)pstAppOmMsg->aucValue, pucData, ulDataLen);

    return VOS_SendMsg(ulSenderPid, pstAppOmMsg);

}
VOS_UINT32 OMRL_AcpuHeadProc(VOS_UINT8 *pucData, VOS_UINT32 ulSize)
{
    VOS_UINT32          ulRslt;
    VOS_UINT32          ulValue;
    VOS_UINT32          ulTmpLen;

    if ((OM_RL_DATATYPE_LEN + OM_USB_FRAME_LEN) >= ulSize)
    {
        /* ��ӡ���� */
        LogPrint("OMRL_AcpuHeadProc: The data length is error.");
        g_stAcpuPcToUeErrRecord.usPacketLenErr++;
        return VOS_ERR;
    }

    VOS_MemCpy(&ulValue, (pucData+OM_RL_DATATYPE_LEN), sizeof(VOS_UINT32));

    /* ������ͨ��Ϣ����·��Ϣ */
    /* ���ݰ�����һ���ֽ��ж��ǲ���0xaaaa5555 */
    if (OM_USB_FRAME_HEAD != ulValue)
    {
        /* �˷�֧��Ϊ��·��Ϣ */
        ulTmpLen =  ulSize - (VOS_UINT32)sizeof(OM_APP_MSG_SEGMENT_STRU);

        /* ��·��Ϣ�����ж� */
        if (sizeof(APP_OM_MSG_EX_STRU) != ulTmpLen)
        {
            g_stAcpuPcToUeErrRecord.usLinkDataLenErr++;

            return VOS_ERR;
        }

        /* �������ֽ�sn header */
        ulRslt = OMRL_AcpuSendMsg(pucData + sizeof(OM_APP_MSG_SEGMENT_STRU),
                       ulTmpLen, ACPU_PID_OM, ACPU_PID_OM);

        g_stAcpuPcToUeSucRecord.stLinkData.ulDataLen += ulTmpLen;
        g_stAcpuPcToUeSucRecord.stLinkData.ulNum++;

        return ulRslt;
    }

    /* �յ���ͨ��Ϣʱ���жϵ�ǰ��·״̬ */
    if (OM_STATE_IDLE == g_ulAcpuOMSwitchOnOff)
    {
         g_stAcpuPcToUeErrRecord.usLinkStatusErr++;
         return VOS_ERR;
    }


    /* ��ͨ��Ϣ���ж�datatype�ֶ��ǲ���GU���� */
    if (OM_RL_DATATYPE_GU != *pucData)
    {
         g_stAcpuPcToUeErrRecord.usDatatypeErr++;
         return VOS_ERR;
    }

    VOS_MemCpy(&ulValue, (pucData + ulSize - OM_USB_TAIL_LEN), sizeof(VOS_UINT32));

    /* �ж����ݰ�β�ǲ���0x5555aaaa */
    if (OM_USB_FRAME_TAIL != ulValue)
    {
        /* ��¼���� */
        g_stAcpuPcToUeErrRecord.usDataHeadErr++;
        return VOS_ERR;
    }

    /* ����ָ��ͳ��ȸ���*/
    pucData += (OM_RL_DATATYPE_LEN + OM_USB_HEAD_LEN);
    ulSize  -= (OM_RL_DATATYPE_LEN + OM_USB_FRAME_LEN);

    /* ��� */
    return OMRL_AcpuMsgCombine(pucData, (VOS_UINT16)ulSize);
}


VOS_UINT32 OMRL_AcpuRcvData(VOS_UINT8 *pucData, VOS_UINT32 ulLen)
{
    VOS_UINT8                           ucChar;
    VOS_UINT32                          i;
    VOS_UINT32                          ulResult;

    ulResult = VOS_ERR;

    g_stAcpuPcToUeSucRecord.stTotalData.ulDataLen += ulLen;
    g_stAcpuPcToUeSucRecord.stTotalData.ulNum++;

    /*lint -e40*/
    OM_ACPU_DEBUG_TRACE(pucData, ulLen, OM_ACPU_RECV_USB);
    /*lint +e40*/

    /* �������ͷ���Ϣ����ʱֹͣ�������� */
    if (0 < g_stAcpuUeToPcErrRecord.usRlsMemErr)
    {
        return VOS_OK;
    }

    for( i = 0; i < ulLen; i++ )
    {
        ucChar = (VOS_UINT8)pucData[i];

        ulResult = Om_HdlcDecap(&g_stOmHdlcEntity, ucChar);

        if ( HDLC_SUCC == ulResult )
        {
            /*lint -e40*/
            OM_ACPU_DEBUG_TRACE(g_stOmHdlcEntity.pucDecapBuff,
                g_stOmHdlcEntity.ulInfoLen, OM_ACPU_RECV_USB);
            /*lint +e40*/

            g_stAcpuPcToUeSucRecord.stHdlcDecapData.ulDataLen += g_stOmHdlcEntity.ulInfoLen;
            g_stAcpuPcToUeSucRecord.stHdlcDecapData.ulNum++;

            if (VOS_OK != OMRL_AcpuHeadProc(g_stOmHdlcEntity.pucDecapBuff, g_stOmHdlcEntity.ulInfoLen))
            {
                g_stAcpuPcToUeErrRecord.stLostData.ulDataLen += g_stOmHdlcEntity.ulInfoLen;
                g_stAcpuPcToUeErrRecord.stLostData.ulNum++;
            }
        }
        else if (HDLC_NOT_HDLC_FRAME == ulResult)
        {
            /*����������֡,����HDLC���װ*/
        }
        else
        {
            g_stAcpuPcToUeErrRecord.ulFrameDecapErr++;
        }
    }

    return VOS_OK;
}

#if (FEATURE_ON == FEATURE_CBT_LOG)


VOS_UINT32 OMRL_AcpuCBTMsgDispatch(VOS_VOID)
{
    VOS_UINT32                       ulRslt;

    /*lint -e40*/
    OM_ACPU_DEBUG_TRACE(g_stCBTMsgCombineInfo.pstWholeMsg->aucValue, g_stCBTMsgCombineInfo.pstWholeMsg->ulLength, OM_ACPU_DISPATCH_MSG);
    /*lint +e40*/

    /* CCPU����Ϣͨ��ICCͨ������ */
    if (WUEPS_PID_OM == g_stCBTMsgCombineInfo.pstWholeMsg->ulReceiverPid)
    {
        ulRslt = GU_OamSndPcMsgToCcpu(g_stCBTMsgCombineInfo.pstWholeMsg->aucValue, g_stCBTMsgCombineInfo.pstWholeMsg->ulLength);

        g_stCBTAcpuPcToUeSucRecord.stCcpuData.ulDataLen += g_stCBTMsgCombineInfo.pstWholeMsg->ulLength;
        g_stCBTAcpuPcToUeSucRecord.stCcpuData.ulNum++;

        /* �ͷ���Ϣ�ռ� */
        VOS_FreeMsg(ACPU_PID_OM, g_stCBTMsgCombineInfo.pstWholeMsg);
    }
    else if (ACPU_PID_OM == g_stCBTMsgCombineInfo.pstWholeMsg->ulReceiverPid)
    {
        /* ����cpu����Ϣͨ��VOS_SendMsg���� */
        g_stCBTAcpuPcToUeSucRecord.stAcpuData.ulDataLen += g_stCBTMsgCombineInfo.pstWholeMsg->ulLength;
        g_stCBTAcpuPcToUeSucRecord.stAcpuData.ulNum++;

        ulRslt = VOS_SendMsg(ACPU_PID_OM, g_stCBTMsgCombineInfo.pstWholeMsg);
    }
    else if (DSP_PID_HIFI_OM == g_stCBTMsgCombineInfo.pstWholeMsg->ulReceiverPid)
    {
        /* ����cpu����Ϣͨ��VOS_SendMsg���� */
        g_stCBTAcpuPcToUeSucRecord.stHifiData.ulDataLen += g_stCBTMsgCombineInfo.pstWholeMsg->ulLength;
        g_stCBTAcpuPcToUeSucRecord.stHifiData.ulNum++;

        ulRslt = VOS_SendMsg(ACPU_PID_OM, g_stCBTMsgCombineInfo.pstWholeMsg);
    }
    else
    {
        /* ����cpu����Ϣͨ��VOS_SendMsg���� */
        g_stCBTAcpuPcToUeSucRecord.stMcuData.ulDataLen += g_stCBTMsgCombineInfo.pstWholeMsg->ulLength;
        g_stCBTAcpuPcToUeSucRecord.stMcuData.ulNum++;

        ulRslt = VOS_SendMsg(ACPU_PID_OM, g_stCBTMsgCombineInfo.pstWholeMsg);
    }

    g_stCBTMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

    if (VOS_OK != ulRslt )
    {
        return VOS_ERR;
    }

    return VOS_OK;

}
VOS_UINT32 OMRL_AcpuCBTMsgCombine(VOS_UINT8 *pucData, VOS_UINT16 usLen)
{
    VOS_UINT32                       ulRslt = VOS_OK;/*Get the return value of PS_SEND_MSG.*/
    VOS_UINT8                        ucCurrentSegSn;/*Reserve the current packet's SN.*/
    OM_APP_MSG_SEGMENT_STRU          *pstOmAppSeg;
    APP_OM_GU_MSG_STRU               *pstAppOmMsg;

    /*Check the input parameter's validity.Make sure empty segment can't pass*/
    if ( (VOS_NULL_PTR == pucData) ||
        (usLen <= sizeof(OM_APP_MSG_SEGMENT_STRU)) )
    {
        LogPrint("OMRL_AcpuCBTMsgCombine: The input is wrong.");

        g_stCBTAcpuPcToUeErrRecord.usLenTooShortErr++;

        return VOS_ERR;
    }

    pstOmAppSeg = (OM_APP_MSG_SEGMENT_STRU *)pucData;

    /*Check the input parameter's validity.*/
    if (usLen != ((pstOmAppSeg->usSegLen + 3) & ~0x3))
    {
        LogPrint("OMRL_AcpuCBTMsgCombine: The input length is wrong.");

        g_stCBTAcpuPcToUeErrRecord.usSegLenErr++;

        return VOS_ERR;
    }

    usLen = pstOmAppSeg->usSegLen;

    ucCurrentSegSn = pstOmAppSeg->ucSegSn;

    /*When the MSG packet is new, we need initialize the static variable.*/
    if (1 == ucCurrentSegSn)
    {
        if (VOS_NULL_PTR != g_stCBTMsgCombineInfo.pstWholeMsg)
        {
            VOS_FreeMsg(ACPU_PID_OM, g_stCBTMsgCombineInfo.pstWholeMsg);
            g_stCBTMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

            g_stCBTAcpuPcToUeErrRecord.usPacketLostErr++;
        }

        /*The new MSG packet is coming.*/
        g_stCBTMsgCombineInfo.ucExpectedSegSn = 1;
        g_stCBTMsgCombineInfo.ulMoveLen       = 0;

        if (usLen < (sizeof(APP_OM_GU_MSG_STRU) + OM_APP_MSG_SEGMENT_HEADER_LENGTH))
        {
            LogPrint("OMRL_AcpuCBTMsgCombine: The input parameter is wrong.");

            g_stCBTAcpuPcToUeErrRecord.usMsgLenErr++;

            return VOS_ERR;
        }

        pstAppOmMsg = (APP_OM_GU_MSG_STRU *)(pucData + OM_APP_MSG_SEGMENT_HEADER_LENGTH);

        g_stCBTMsgCombineInfo.ulTotalMsgLen = pstAppOmMsg->usLength + APP_OM_HEADER_LENGTH;

        /* �����ɺ��ж��Ƿ������ֵ */
        if (OM_RL_TOTAL_MSG_MAX_LEN < g_stCBTMsgCombineInfo.ulTotalMsgLen)
        {
            g_stCBTAcpuPcToUeErrRecord.usMsgTooLongErr++;

            LogPrint1("OMRL_AcpuCBTMsgCombine: receive first msg pack is too long %d;\r\n",
                        (VOS_INT32)g_stCBTMsgCombineInfo.ulTotalMsgLen);

            /* �������֧�ֱ���1024�ֽڵ����ݣ�����û���ṩ�꣬����ֱ��ʹ������ */
            DRV_SYSTEM_ERROR(OAM_PC_LENGTH_TOO_BIG, (VOS_INT)g_stCBTMsgCombineInfo.ulTotalMsgLen, 0, (VOS_CHAR *)pucData, 1024);

            return VOS_ERR;
        }

        /*Allocate the memory space.*/
        g_stCBTMsgCombineInfo.pstWholeMsg = VOS_AllocMsg(ACPU_PID_OM,  g_stCBTMsgCombineInfo.ulTotalMsgLen);
        if (VOS_NULL_PTR == g_stCBTMsgCombineInfo.pstWholeMsg)
        {
            LogPrint("OMRL_AcpuCBTMsgCombine: VOS_AllocMsg fail.");

            g_stCBTAcpuPcToUeErrRecord.usAllocMsg++;

            return VOS_ERR;
        }

        /* ����CPU ID���ҵ���Ӧ��PID�����ڹ����·�����֡����VOS_OM_HEADER�ṹ�б����ֶα�ʾCPUID */
        ulRslt = OMRL_AcpuCpuIdToPid(pstAppOmMsg->ucCpuId, &g_stCBTMsgCombineInfo.pstWholeMsg->ulReceiverPid);
        if (VOS_OK != ulRslt)
        {
            /* �ͷ���Ϣ�ռ� */
            VOS_FreeMsg(ACPU_PID_OM, g_stCBTMsgCombineInfo.pstWholeMsg);

            g_stCBTMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

            g_stCBTAcpuPcToUeErrRecord.usCpuIdErr++;

            /* �˴�����ֱ�ӷ��أ����ü�¼�����������Ѿ���¼��CPU ID�������� */
            return VOS_ERR;
        }

        g_stCBTMsgCombineInfo.ucTotalSegNum = pstOmAppSeg->ucSegNum;
    }

    if ( g_stCBTMsgCombineInfo.ucTotalSegNum != pstOmAppSeg->ucSegNum )
    {
        LogPrint("OMRL_AcpuCBTMsgCombine: The length of the packet is biger than the size of allocated memory.");

        /* �ͷ���Ϣ�ռ� */
        VOS_FreeMsg(ACPU_PID_OM, g_stCBTMsgCombineInfo.pstWholeMsg);

        g_stCBTMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

        g_stCBTAcpuPcToUeErrRecord.usSegNumErr++;

        return VOS_ERR;
    }

    /*The expected MSG pakcet is lost.*/
    if (g_stCBTMsgCombineInfo.ucExpectedSegSn != ucCurrentSegSn)
    {
        /*Print the error info.*/
        LogPrint2("OMRL_AcpuCBTMsgCombine: expected SN is %d, current SN is %d.", g_stCBTMsgCombineInfo.ucExpectedSegSn, ucCurrentSegSn);

        /* �ͷ���Ϣ�ռ� */
        VOS_FreeMsg(ACPU_PID_OM, g_stCBTMsgCombineInfo.pstWholeMsg);

        g_stCBTMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

        g_stCBTAcpuPcToUeErrRecord.usMsgSnErr++;

        return VOS_ERR;
    }

    /*We make sure that the memory can't be violated.*/
    if (g_stCBTMsgCombineInfo.ulTotalMsgLen < (g_stCBTMsgCombineInfo.ulMoveLen + (usLen - OM_APP_MSG_SEGMENT_HEADER_LENGTH)))
    {
        LogPrint("OMRL_AcpuCBTMsgCombine: The length of the packet is biger than the size of allocated memory.\n");

        /* �ͷ���Ϣ�ռ� */
        VOS_FreeMsg(ACPU_PID_OM, g_stCBTMsgCombineInfo.pstWholeMsg);

        g_stCBTMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

        g_stCBTAcpuPcToUeErrRecord.usNoMemErr++;

        return VOS_ERR;
    }

    VOS_MemCpy((VOS_UINT8*)(g_stCBTMsgCombineInfo.pstWholeMsg->aucValue) + g_stCBTMsgCombineInfo.ulMoveLen,
               pucData + OM_APP_MSG_SEGMENT_HEADER_LENGTH, usLen - OM_APP_MSG_SEGMENT_HEADER_LENGTH);
    g_stCBTMsgCombineInfo.ulMoveLen += usLen - OM_APP_MSG_SEGMENT_HEADER_LENGTH;
    g_stCBTMsgCombineInfo.ucExpectedSegSn++;

    /*If the current MSG packet is an complete packet.*/
    if (g_stCBTMsgCombineInfo.ucTotalSegNum == ucCurrentSegSn)
    {
        /* �����ɺ��ж��Ƿ������ֵ */
        if (OM_RL_TOTAL_MSG_MAX_LEN < g_stCBTMsgCombineInfo.ulTotalMsgLen)
        {
            g_stCBTAcpuPcToUeErrRecord.usMsgTooLongErr++;

            /* �ͷ���Ϣ�ռ� */
            VOS_FreeMsg(ACPU_PID_OM, g_stCBTMsgCombineInfo.pstWholeMsg);

            g_stCBTMsgCombineInfo.pstWholeMsg = VOS_NULL_PTR;

            return VOS_ERR;
        }

        /*It will send the MSG to OM module.*/
        ulRslt = OMRL_AcpuCBTMsgDispatch();

        return ulRslt;
    }

    return VOS_OK;
}


VOS_UINT32 OMRL_AcpuCBTHeadProc(VOS_UINT8 *pucData, VOS_UINT32 ulSize)
{
    VOS_UINT32          ulRslt;
    VOS_UINT32          ulValue;
    VOS_UINT32          ulTmpLen;
    APP_OM_MSG_EX_STRU  stOmMsg;

    if ((OM_RL_DATATYPE_LEN + OM_USB_FRAME_LEN) >= ulSize)
    {
        /* ��ӡ���� */
        LogPrint("OMRL_AcpuCBTHeadProc: The data length is error.");
        g_stCBTAcpuPcToUeErrRecord.usPacketLenErr++;
        return VOS_ERR;
    }

    VOS_MemCpy(&ulValue, (pucData+OM_RL_DATATYPE_LEN), sizeof(VOS_UINT32));

    /* ������ͨ��Ϣ����·��Ϣ */
    /* ���ݰ�����һ���ֽ��ж��ǲ���0xaaaa5555 */
    if (OM_USB_FRAME_HEAD != ulValue)
    {
        /* �˷�֧��Ϊ��·��Ϣ */
        ulTmpLen =  ulSize - sizeof(OM_APP_MSG_SEGMENT_STRU);

        /* ��·��Ϣ�����ж� */
        if (sizeof(APP_OM_MSG_EX_STRU) != ulTmpLen)
        {
            g_stCBTAcpuPcToUeErrRecord.usLinkDataLenErr++;

            return VOS_ERR;
        }

        VOS_MemCpy((VOS_UINT8 *)(&stOmMsg), (pucData + sizeof(OM_APP_MSG_SEGMENT_STRU)), sizeof(APP_OM_MSG_EX_STRU));

        if (APP_OM_ESTABLISH_REQ == stOmMsg.usPrimId)
        {
            g_ulCBTEstablishStatus = OM_CBT_LOG_ENABLE_FLAG;   
        }
        else if (APP_OM_RELEASE_REQ == stOmMsg.usPrimId )
        {
            g_ulCBTEstablishStatus = VOS_NULL;
        }
        else
        {
            /* blank */
        }
        
        /* �������ֽ�sn header */
        ulRslt = OMRL_AcpuSendMsg(pucData + sizeof(OM_APP_MSG_SEGMENT_STRU),
                       ulTmpLen, ACPU_PID_OM, ACPU_PID_OM);

        g_stCBTAcpuPcToUeSucRecord.stLinkData.ulDataLen += ulTmpLen;
        g_stCBTAcpuPcToUeSucRecord.stLinkData.ulNum++;

        return ulRslt;
    }

    /* �յ���ͨ��Ϣʱ���жϵ�ǰ��·״̬ */
    if (OM_STATE_IDLE == g_ulAcpuOMSwitchOnOff)
    {
         g_stCBTAcpuPcToUeErrRecord.usLinkStatusErr++;
         return VOS_ERR;
    }


    /* ��ͨ��Ϣ���ж�datatype�ֶ��ǲ���GU���� */
    if (OM_RL_DATATYPE_GU != *pucData)
    {
         g_stCBTAcpuPcToUeErrRecord.usDatatypeErr++;
         return VOS_ERR;
    }

    VOS_MemCpy(&ulValue, (pucData + ulSize - OM_USB_TAIL_LEN), sizeof(VOS_UINT32));

    /* �ж����ݰ�β�ǲ���0x5555aaaa */
    if (OM_USB_FRAME_TAIL != ulValue)
    {
        /* ��¼���� */
        g_stCBTAcpuPcToUeErrRecord.usDataHeadErr++;
        return VOS_ERR;
    }

    /* ����ָ��ͳ��ȸ���*/
    pucData += (OM_RL_DATATYPE_LEN + OM_USB_HEAD_LEN);
    ulSize  -= (OM_RL_DATATYPE_LEN + OM_USB_FRAME_LEN);

    /* ��� */
    return OMRL_AcpuCBTMsgCombine(pucData, (VOS_UINT16)ulSize);
}
VOS_INT OMRL_AcpuRcvCBTData(VOS_UINT8 ucDevIndex, VOS_UINT8 *pucData, VOS_UINT32 ulLen)
{
    VOS_UINT8                           ucChar;
    VOS_UINT32                          i;
    VOS_UINT32                          ulResult;
    
    if(ucDevIndex != APP_VCOM_DEV_INDEX_CBT)
    {
        return VOS_ERR;
    }

    if ((VOS_NULL_PTR == pucData) || (0 == ulLen))
    {
        return VOS_ERR;
    }

#if (VOS_LINUX == VOS_OS_VER)
    down(&g_stOmRxCBTBuffSem);
#endif

    if ( OM_CBT_LOG_ENABLE_FLAG != g_ulCBTLogEnable )
    {
#if (VOS_LINUX == VOS_OS_VER)
        up(&g_stOmRxCBTBuffSem);
#endif
        return VOS_OK;
    }

    ulResult = VOS_ERR;

    g_stCBTAcpuPcToUeSucRecord.stTotalData.ulDataLen += ulLen;
    g_stCBTAcpuPcToUeSucRecord.stTotalData.ulNum++;

    /*lint -e40*/
    OM_ACPU_DEBUG_TRACE(pucData, ulLen, OM_ACPU_RECV_CBT);
    /*lint +e40*/

    /* �������ͷ���Ϣ����ʱֹͣ�������� �������쳣��������Ҫ����ȫ�ֱ�������ͨ���ı��� */
    if (0 < g_stAcpuUeToPcErrRecord.usRlsMemErr)
    {
#if (VOS_LINUX == VOS_OS_VER)
        up(&g_stOmRxCBTBuffSem);
#endif
        return VOS_OK;
    }

    for( i = 0; i < ulLen; i++ )
    {
        ucChar = (VOS_UINT8)pucData[i];

        ulResult = Om_HdlcDecap(&g_stCBTHdlcEntity, ucChar);

        if ( HDLC_SUCC == ulResult )
        {
            /*lint -e40*/
            OM_ACPU_DEBUG_TRACE(g_stCBTHdlcEntity.pucDecapBuff,
                g_stCBTHdlcEntity.ulInfoLen, OM_ACPU_RECV_CBT);
            /*lint +e40*/

            g_stCBTAcpuPcToUeSucRecord.stHdlcDecapData.ulDataLen += g_stCBTHdlcEntity.ulInfoLen;
            g_stCBTAcpuPcToUeSucRecord.stHdlcDecapData.ulNum++;

            if (VOS_OK != OMRL_AcpuCBTHeadProc(g_stCBTHdlcEntity.pucDecapBuff, g_stCBTHdlcEntity.ulInfoLen))
            {
                g_stCBTAcpuPcToUeErrRecord.stLostData.ulDataLen += g_stCBTHdlcEntity.ulInfoLen;
                g_stCBTAcpuPcToUeErrRecord.stLostData.ulNum++;
            }
        }
        else if (HDLC_NOT_HDLC_FRAME == ulResult)
        {
            /*����������֡,����HDLC���װ*/
        }
        else
        {
            g_stCBTAcpuPcToUeErrRecord.ulFrameDecapErr++;
        }
    }

#if (VOS_LINUX == VOS_OS_VER)
    up(&g_stOmRxCBTBuffSem);
#endif

    return VOS_OK;
}


VOS_BOOL OMRL_JudgeCbtMsg(VOS_UINT8   ucFuncType)
{
    if ( (OM_FS_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
                 ||(OM_WRF_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
                 ||(OM_GRF_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
                 ||(OM_QUERY_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
                 ||(OM_NV_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
                 ||(OM_NOSIG_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
                 ||(OM_CBT_TRANS_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS)) )
    {
        return VOS_TRUE;
    }

    return VOS_FALSE;
}


VOS_VOID OMRL_EncCBTData(OM_HDLC_STRU *pstNVHdlcEntity)
{
    VOS_UINT16                         usHdlcEncLen;

    g_stVComCBTDebugInfo.ulVCOMHdlcEnNum++;
    
    if ( VOS_OK != Om_HdlcEncap(pstNVHdlcEntity->pucDecapBuff,
                                (VOS_UINT16)pstNVHdlcEntity->ulInfoLen,
                                g_pucCBTHdlcEncBuf,
                                (2*OM_HDLC_BUF_MAX_LEN), &usHdlcEncLen) )
    {
        g_stVComCBTDebugInfo.ulVCOMHdlcEnFailNum++;
        return;
    }

    g_stVComCBTDebugInfo.ulVCOMSendNum++;
    g_stVComCBTDebugInfo.ulVCOMSendLen += usHdlcEncLen;

    if ( OM_CBT_LOG_ENABLE_FLAG != g_ulCBTEstablishStatus )
    {
        return;
    }

    if ( VOS_OK != APP_VCOM_Send(APP_VCOM_DEV_INDEX_CBT, g_pucCBTHdlcEncBuf, usHdlcEncLen) )
    {
        g_stVComCBTDebugInfo.ulVCOMSendErrNum++;
        g_stVComCBTDebugInfo.ulVCOMSendErrLen += usHdlcEncLen;

    }

    return;
}
#endif


VOS_VOID OMRL_AcpuRecordUeToAPPNvInfo(VOS_UINT8 ucFuncType, VOS_UINT8 *pucData, VOS_UINT32 ulLen)
{
    if ( (OM_WRF_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
         ||(OM_GRF_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
         ||(OM_QUERY_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
         ||(OM_NV_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS)))
    {
        g_pstRecordOmToApp[g_ulNvOmToAppCount].ulSlice = VOS_GetSlice();
        g_pstRecordOmToApp[g_ulNvOmToAppCount].ulFlag  = 0xaaaaaaaa;

        if (ulLen > NV_MNTN_SAVE_MAX_MSG_LEN)
        {
            ulLen = NV_MNTN_SAVE_MAX_MSG_LEN;
        }

        VOS_MemCpy(g_pstRecordOmToApp[g_ulNvOmToAppCount].aucVaule, pucData, ulLen);

        g_ulNvOmToAppCount++;

        if (SAVE_MAX_SEND_INFO == g_ulNvOmToAppCount)
        {
             g_ulNvOmToAppCount = 0;
        }
    }
    return;
}


VOS_VOID OMRL_AcpuHdlcData(VOS_UINT8 *pucData, VOS_UINT32 ulLen)
{
    VOS_UINT8                           ucChar;
    VOS_UINT32                          i;
    VOS_UINT32                          ulResult = VOS_ERR;
    OM_APP_MSG_EX_STRU                 *pstOmToAppMsg;
    VOS_UINT8                          *pucTem;
    VOS_UINT8                           ucSegSn;

    if ( OM_CBT_LOG_ENABLE_FLAG != g_ulCBTLogEnable )
    {
        return;
    }

    for( i = 0; i < ulLen; i++ )
    {
        ucChar = (VOS_UINT8)pucData[i];

        ulResult = Om_HdlcDecap(&g_stNVHdlcEntity, ucChar);

        if ( HDLC_SUCC == ulResult )
        {
            /* �ж����� */
            pucTem = (VOS_UINT8*)g_stNVHdlcEntity.pucDecapBuff;
            ucSegSn = *(pucTem + OM_RL_DATATYPE_LEN + offsetof(OM_APP_MSG_SEG_EX_STRU, stMsgSeg.ucSegSn));
            pstOmToAppMsg = (OM_APP_MSG_EX_STRU*)(pucTem + OM_RL_DATATYPE_LEN + sizeof(OM_APP_MSG_SEG_EX_STRU));

#if (FEATURE_ON == FEATURE_CBT_LOG)
            /* FuncTypeֻ�����ڵ�һ֡������ */
            if (1 != ucSegSn )
            {
                if  ( OM_CBT_MSG == g_ulCBTStatus )
                {
                     OMRL_EncCBTData(&g_stNVHdlcEntity);
                }

                continue;
            }

            if ( VOS_TRUE == OMRL_JudgeCbtMsg(pstOmToAppMsg->ucFuncType) )
            {
                 g_ulCBTStatus = OM_CBT_MSG;

                 OMRL_EncCBTData(&g_stNVHdlcEntity);
            }
            else
            {
                g_ulCBTStatus = OM_NOT_CBT_MSG;
            }
#else
            if (1 != ucSegSn )
            {
                continue;
            }

#endif
            OMRL_AcpuRecordUeToAPPNvInfo(pstOmToAppMsg->ucFuncType, pucData, ulLen);
        }
        else if (HDLC_NOT_HDLC_FRAME == ulResult)
        {
            /*����������֡,����HDLC���װ*/
        }
        else
        {
        }
    }

    return;
}
VOS_UINT32 OMRL_AcpuHdlcInit(VOS_VOID)
{
    g_stOmHdlcEntity.pucDecapBuff =
        (VOS_UINT8 *)VOS_MemAlloc(ACPU_PID_OM, STATIC_MEM_PT, OM_HDLC_BUF_MAX_LEN);

    if (VOS_NULL_PTR == g_stOmHdlcEntity.pucDecapBuff)
    {
        LogPrint("\r\nOMRL_AcpuHdlcInit:Alloc Memory Failed!\r\n");

        g_stOmHdlcEntity.ulDecapBuffSize = 0;

        return VOS_ERR;
    }

    Om_HdlcInit(&g_stOmHdlcEntity);

    g_stOmHdlcEntity.ulDecapBuffSize = OM_HDLC_BUF_MAX_LEN;

    return VOS_OK;
}


VOS_UINT32 OMRL_AcpuNVHdlcInit(VOS_VOID)
{
    OM_SW_VER_FLAG_STRU         stCustomVersion;

#if (FEATURE_ON == FEATURE_CBT_LOG)    
    OM_CHANNLE_PORT_CFG_STRU    stPortCfg;
    NV_OM_CBT_LOG_ENABLE_STRU   stNVCBTLogEnable;
#endif    

    if ( VOS_OK != NV_Read(en_NV_Item_SW_VERSION_FLAG, &stCustomVersion, sizeof(stCustomVersion)) )
    {
        stCustomVersion.ulNvSwVerFlag = VOS_TRUE;
    }

    /* ���ulCustomVersion == VOS_TRUE��ʾ������Ƭ�汾����ֱ�ӷ��أ����÷����¼NV��ά�ɲ����Դ */
    if ( VOS_TRUE == stCustomVersion.ulNvSwVerFlag)
    {
        return VOS_OK;
    }

    g_pstRecordOmToApp = (OM_NV_MNTN_RECORD_STRU *)VOS_MemAlloc(ACPU_PID_OM,
                         STATIC_MEM_PT, SAVE_MAX_SEND_INFO*sizeof(OM_NV_MNTN_RECORD_STRU));

    if (VOS_NULL_PTR == g_pstRecordOmToApp)
    {
        LogPrint("\r\nOMRL_AcpuNVHdlcInit:Alloc Memory Failed!\r\n");

        return VOS_ERR;
    }

    g_pstRecordAppToOm = (OM_NV_MNTN_RECORD_STRU *)VOS_MemAlloc(ACPU_PID_OM,
                   STATIC_MEM_PT, SAVE_MAX_SEND_INFO*sizeof(OM_NV_MNTN_RECORD_STRU));

    if (VOS_NULL_PTR == g_pstRecordAppToOm)
    {
        LogPrint("\r\nOMRL_AcpuNVHdlcInit:Alloc Memory Failed!\r\n");

        return VOS_ERR;
    }

#if (FEATURE_ON == FEATURE_CBT_LOG)
    /* ��ȡOM���������ͨ�� */
    if (NV_OK != NV_Read(en_NV_Item_Om_Port_Type, &stPortCfg, sizeof(OM_CHANNLE_PORT_CFG_STRU)))
    {
        return VOS_OK;
    }

    stPortCfg.enPortNum += CPM_APP_PORT;
    if ( CPM_VCOM_PORT != stPortCfg.enPortNum )
    {
        return VOS_OK;
    }

    /* ��ȡ������NV ����Ҫ������ʹ�� Nv Ҳ�Ǵ򿪵� ����ֱ���˳� */
    if (NV_OK != NV_Read(en_NV_Item_CBT_LOG_ENABLE, &stNVCBTLogEnable, sizeof(NV_OM_CBT_LOG_ENABLE_STRU)))
    {
        return VOS_OK;
    }

    if (VOS_TRUE != stNVCBTLogEnable.bCBTLogEnable)
    {
        return VOS_OK;
    }
#endif

    g_ulCBTLogEnable = OM_CBT_LOG_ENABLE_FLAG;

    g_stNVHdlcEntity.pucDecapBuff = (VOS_UINT8 *)VOS_MemAlloc(ACPU_PID_OM, STATIC_MEM_PT, 2*OM_HDLC_BUF_MAX_LEN);

    if (VOS_NULL_PTR == g_stNVHdlcEntity.pucDecapBuff)
    {
        LogPrint("\r\nOMRL_AcpuNVHdlcInit:Alloc Memory Failed!\r\n");

        g_stNVHdlcEntity.ulDecapBuffSize = 0;

        return VOS_ERR;
    }

    Om_HdlcInit(&g_stNVHdlcEntity);

    g_stNVHdlcEntity.ulDecapBuffSize = 2*OM_HDLC_BUF_MAX_LEN;

#if (FEATURE_ON == FEATURE_CBT_LOG)
    g_stCBTHdlcEntity.pucDecapBuff = (VOS_UINT8 *)VOS_MemAlloc(ACPU_PID_OM, STATIC_MEM_PT, OM_HDLC_BUF_MAX_LEN);

    if (VOS_NULL_PTR == g_stCBTHdlcEntity.pucDecapBuff)
    {
        LogPrint("\r\n OMRL_AcpuNVHdlcInit:Alloc Memory Failed!\r\n");

        g_stCBTHdlcEntity.ulDecapBuffSize = 0;

        return VOS_ERR;
    }

    Om_HdlcInit(&g_stCBTHdlcEntity);

    g_stCBTHdlcEntity.ulDecapBuffSize = OM_HDLC_BUF_MAX_LEN;

    g_pucCBTHdlcEncBuf = (VOS_UINT8 *)VOS_MemAlloc(ACPU_PID_OM, STATIC_MEM_PT, 2*OM_HDLC_BUF_MAX_LEN);
    if (VOS_NULL_PTR == g_pucCBTHdlcEncBuf)
    {
        return VOS_ERR;
    }
#endif

    return VOS_OK;
}
VOS_UINT32 OMRL_AcpuReportEvent(VOS_VOID)
{
    PS_OM_EVENT_IND_STRU    *pstEventInd;
    VOS_UINT32               ulEventLenth;
    VOS_UINT32               ulTemp = 0x5a5a5a5a;
    VOS_UINT32               ulIndex;
    VOS_UINT32               ulSendLen;
    OM_SEND_DEBUG_INFO_STRU  astAcpuDebuginfoTab2[3];
    VOS_UINT_PTR             ulAddr;
    VOS_UINT16               usDebugEventID;

    /* ���A�˵�����Ϣ�� ��С�͵�ַ,�ְ��ϱ���Ϣ */
    OM_SEND_DEBUG_INFO_STRU  astAcpuDbuginfoTab1[] = {
        {sizeof(g_stAcpuPcToUeSucRecord),                 (VOS_UINT8*)&g_stAcpuPcToUeSucRecord},
        {sizeof(g_stAcpuPcToUeErrRecord),                 (VOS_UINT8*)&g_stAcpuPcToUeErrRecord},
        {sizeof(g_stAcpuUeToPcSucRecord),                 (VOS_UINT8*)&g_stAcpuUeToPcSucRecord},
        {sizeof(g_stAcpuUeToPcErrRecord),                 (VOS_UINT8*)&g_stAcpuUeToPcErrRecord},
        {sizeof(g_stAcpuDebugInfo),                       (VOS_UINT8*)&g_stAcpuDebugInfo      },
        {sizeof(g_stOmSDDebugInfo),                       (VOS_UINT8*)&g_stOmSDDebugInfo      },
        {sizeof(g_stVComDebugInfo),                       (VOS_UINT8*)&g_stVComDebugInfo      },
        {sizeof(g_astAcpuRecordInfo),                     (VOS_UINT8*)g_astAcpuRecordInfo    },
        {sizeof(g_ulAcpuOMSwitchOnOff),                   (VOS_UINT8*)&g_ulAcpuOMSwitchOnOff  },
    };

    astAcpuDebuginfoTab2[0].pucAddr = (VOS_UINT8*)SCM_GetDebugLogInfo();
    astAcpuDebuginfoTab2[0].ulLenth = SCM_GetDebugLogInfoLen();

    astAcpuDebuginfoTab2[1].pucAddr = (VOS_UINT8*)SOCP_GetDebugLogInfo(OM_GET_SOCP_STATUS);
    astAcpuDebuginfoTab2[1].ulLenth = SOCP_GetDebugLogInfoLen(OM_GET_SOCP_STATUS);

    astAcpuDebuginfoTab2[2].pucAddr = (VOS_UINT8*)SOCP_GetDebugLogInfo(OM_GET_SOCP_DEBUG_INFO);
    astAcpuDebuginfoTab2[2].ulLenth = SOCP_GetDebugLogInfoLen(OM_GET_SOCP_DEBUG_INFO);

    /* ---------------��һ������ astAcpuDbuginfoTab1[]  begin----------- */

    ulEventLenth = (sizeof(PS_OM_EVENT_IND_STRU)-(4 * sizeof(VOS_UINT8)));
    for (ulIndex=0; ulIndex<(sizeof(astAcpuDbuginfoTab1)/sizeof(astAcpuDbuginfoTab1[0])); ulIndex ++)
    {
        ulEventLenth += astAcpuDbuginfoTab1[ulIndex].ulLenth;
    }

    /* ���Ϸָ����ų��� */
    ulEventLenth += (ulIndex - 1) * (VOS_UINT32)sizeof(VOS_UINT32);
    pstEventInd = (PS_OM_EVENT_IND_STRU *)VOS_MemAlloc(ACPU_PID_OM, DYNAMIC_MEM_PT, ulEventLenth);
    if (VOS_NULL_PTR == pstEventInd)
    {
        LogPrint("OMRL_AcpuReportEvent:Alloc Memory Failed!\n");
        return VOS_ERR;
    }

    pstEventInd->ulLength   = ulEventLenth;
    pstEventInd->usEventId  = OAM_DEGUG_EVENT_ID1;
    pstEventInd->ulModuleId = ACPU_PID_OM;

    /*lint -e419 -e416 */
    /* pc�ൽue��ʱ��������������Ϣ */
    VOS_MemCpy((VOS_UINT8 *)pstEventInd->aucData,
        (VOS_UINT8*)&g_stAcpuPcToUeSucRecord,
        sizeof(OM_ACPU_PC_UE_SUC_STRU));
     /*lint +e419 +e416 */

    ulEventLenth = 0;
    for (ulIndex=1;ulIndex<(sizeof(astAcpuDbuginfoTab1)/sizeof(astAcpuDbuginfoTab1[0])); ulIndex ++)
    {
        /*lint -e419 -e416 */
        /* ���Ϸָ����� */
        ulEventLenth += astAcpuDbuginfoTab1[ulIndex-1].ulLenth;
        VOS_MemCpy((VOS_UINT8 *)pstEventInd->aucData + ulEventLenth,
                   (VOS_UINT8 *)&ulTemp, sizeof(VOS_UINT32));

        ulEventLenth += (VOS_UINT32)sizeof(VOS_UINT32);
        VOS_MemCpy((VOS_UINT8 *)pstEventInd->aucData + ulEventLenth,
            astAcpuDbuginfoTab1[ulIndex].pucAddr, astAcpuDbuginfoTab1[ulIndex].ulLenth);
         /*lint +e419 +e416 */
    }
    /*lint +e419 +e416 */

    if (VOS_OK != OM_AcpuEvent(pstEventInd))
    {
        PS_LOG(ACPU_PID_OM, VOS_NULL, PS_PRINT_WARNING, "OMRL_AcpuReportEvent: send event fail.");
    }
    /* �ͷ���Ϣ���ݿռ� */
    VOS_MemFree(ACPU_PID_OM, pstEventInd);

    /* ---------------��һ������ astAcpuDbuginfoTab1[]  end----------- */

    /* ---------------astAcpuDbuginfoTab2[] ÿ�����ݽϴ������ͣ�ÿ�������2k----------- */
    /* Event Id */
    usDebugEventID  = OAM_DEGUG_EVENT_ID2;
    /* һ�������� 2k �ռ䣬����ͷ� */
    pstEventInd = (PS_OM_EVENT_IND_STRU *)VOS_MemAlloc(ACPU_PID_OM, DYNAMIC_MEM_PT,
                                          OM_SEND_DEBUG_INFO_MAX_LEN + sizeof(PS_OM_EVENT_IND_STRU)-(4 * sizeof(VOS_UINT8)));
    if (VOS_NULL_PTR == pstEventInd)
    {
        LogPrint("OMRL_AcpuReportEvent:Alloc Memory Failed!\n");
        return VOS_ERR;
    }

    /* ���ݲ�ַ��� */
    for(ulIndex =0; ulIndex<(sizeof(astAcpuDebuginfoTab2)/sizeof(astAcpuDebuginfoTab2[0])); ulIndex++)
    {
        /* �������ݿռ� ���Ϊ 2k*/
        ulSendLen = 0;

        while(ulSendLen < astAcpuDebuginfoTab2[ulIndex].ulLenth)
        {
            if((astAcpuDebuginfoTab2[ulIndex].ulLenth - ulSendLen) > OM_SEND_DEBUG_INFO_MAX_LEN)
            {
                ulEventLenth = OM_SEND_DEBUG_INFO_MAX_LEN;
            }
            else
            {
                ulEventLenth = astAcpuDebuginfoTab2[ulIndex].ulLenth - ulSendLen;
            }

            pstEventInd->ulLength   = ulEventLenth;
            pstEventInd->usEventId  = usDebugEventID++;
            pstEventInd->ulModuleId = ACPU_PID_OM;

            /* make pclint happy  begin */
            ulAddr = (VOS_UINT_PTR)pstEventInd->aucData;
            VOS_MemCpy((VOS_UINT8 *)ulAddr, astAcpuDebuginfoTab2[ulIndex].pucAddr + ulSendLen, ulEventLenth);
            /* make pclint happy  end */

            ulSendLen += ulEventLenth;

            if (VOS_OK != OM_AcpuEvent(pstEventInd))
            {
                PS_LOG(ACPU_PID_OM, VOS_NULL, PS_PRINT_WARNING, "OMRL_AcpuReportEvent: send event fail.");
            }
        }
    }

    /* �ͷ���Ϣ���ݿռ� */
    VOS_MemFree(ACPU_PID_OM, pstEventInd);

    return VOS_OK;
}


VOS_UINT32 OMRL_AcpuReleaseSocpData(VOS_VOID)
{
    VOS_UINT_PTR        DataAddr;
    VOS_UINT32          ulDataLen;
    VOS_UINT8           *pucRealAddr;

#if 0

    VOS_BOOL            bLoopFlag = VOS_TRUE;

    /* Ϊ�˲��Գ�֣������Ȱ�һ��һ��RD�ͷ� */
    do
    {
        /* ��SCM����ѷ�����Ϣ�ܳ��� */
        if (VOS_OK != SCM_RlsSrcRDFirst(SOCP_CODER_SRC_GU_OM1, &ulDataAddr, &ulDataLen))
        {
            g_stAcpuUeToPcErrRecord.usRlsRdErr++;

            return VOS_ERR;
        }

        /* �Ѿ�û��RD */
        if ((0 == ulDataAddr) || (0 == ulDataLen))
        {
            break;
        }

        g_stAcpuUeToPcSucRecord.stRealRlsData.ulDataLen += ulDataLen;
        g_stAcpuUeToPcSucRecord.stRealRlsData.ulNum++;

        pucRealAddr = (VOS_UINT8*)OM_AddrRealToVirt(&g_stAcpuOmBufCtrl, (VOS_UINT8*)ulDataAddr);

        if (VOS_NULL_PTR == pucRealAddr)
        {
            g_stAcpuUeToPcErrRecord.usRlsAddrErr++;

            return VOS_ERR;
        }

        /* �ͷ�OM�������ڴ� */
        if (VOS_OK != OM_ReleaseTraceMem(&g_stAcpuOmBufCtrl, pucRealAddr, (VOS_INT32)ulDataLen))
        {
            g_stAcpuUeToPcErrRecord.usRlsMemErr++;

            return VOS_ERR;
        }

    }while(bLoopFlag);

#else
    /* ��������ѷ���RD */
    if (VOS_OK != SCM_RlsSrcRDAll(SOCP_CODER_SRC_GU_OM1, &DataAddr, &ulDataLen))
    {
        g_stAcpuUeToPcErrRecord.usRlsRdErr++;

        return VOS_ERR;
    }

    /* �Ѿ�û��RD */
    if ((0 == DataAddr) || (0 == ulDataLen))
    {
        return VOS_OK;
    }

    g_stAcpuUeToPcSucRecord.stRealRlsData.ulDataLen += ulDataLen;
    g_stAcpuUeToPcSucRecord.stRealRlsData.ulNum++;

    pucRealAddr = (VOS_UINT8*)OM_AddrRealToVirt(&g_stAcpuOmBufCtrl, (VOS_UINT8*)DataAddr);

    if (VOS_NULL_PTR == pucRealAddr)
    {
        g_stAcpuUeToPcErrRecord.usRlsAddrErr++;

        return VOS_ERR;
    }

    /* �����ѷ���RD�ܳ����ͷ��ڴ� */
    if (VOS_OK != OM_ReleaseTraceMem(&g_stAcpuOmBufCtrl, pucRealAddr, (VOS_INT32)ulDataLen))
    {
        g_stAcpuUeToPcErrRecord.usRlsMemErr++;

        g_ulAcpuOMSwitchOnOff = OM_STATE_IDLE;

        return VOS_ERR;
    }
#endif

    return VOS_OK;
}
VOS_UINT32 OMRL_AcpuSendSocpData(VOS_UINT8 *pucHead, VOS_UINT32 ulHeadLen,
                            VOS_UINT8 *pucData, VOS_UINT32 ulDataLen)
{
    VOS_UINT8                          *pucMemAddr;
    VOS_INT32                           lUsedBytes;
    VOS_UINT8                          *pucRealMemAddr;

    /*lint -e40*/
    OM_ACPU_DEBUG_TRACE(pucHead, ulHeadLen, OM_ACPU_SEND_SOCP);
    OM_ACPU_DEBUG_TRACE(pucData, ulDataLen, OM_ACPU_SEND_SOCP);
    /*lint +e40*/

    pucMemAddr = (VOS_UINT8*)OM_AllocTraceMem(&g_stAcpuOmBufCtrl, (VOS_INT32)(ulHeadLen + ulDataLen + sizeof(VOS_UINT32)));

    if (VOS_NULL_PTR == pucMemAddr)
    {
        g_stAcpuUeToPcErrRecord.usNoMemErr++;

        return VOS_ERR;
    }

    /* ��������ͷ */
    VOS_MemCpy(pucMemAddr, pucHead, ulHeadLen);

    /* ������������ */
    VOS_MemCpy(pucMemAddr + ulHeadLen, pucData, ulDataLen);

    /* ��д��β */
    *(VOS_UINT32*)(pucMemAddr + ulHeadLen + ulDataLen) = OM_USB_FRAME_TAIL;

    VOS_FlushCpuWriteBuf();

    pucRealMemAddr = (VOS_UINT8*)OM_AddrVirtToReal(&g_stAcpuOmBufCtrl, pucMemAddr);

    if (VOS_NULL_PTR == pucRealMemAddr)
    {
        g_stAcpuUeToPcErrRecord.usSendAddrErr++;

        return VOS_ERR;
    }

    /* ����SCM�ṩ�Ľӿڽ����ݷ��͸�SOCP */
    if (VOS_OK != SCM_SendCoderSrc(SOCP_CODER_SRC_GU_OM1, pucRealMemAddr, ulHeadLen + ulDataLen + (VOS_UINT32)sizeof(VOS_UINT32)))
    {
        g_stAcpuUeToPcErrRecord.usSendSocpDataErr++;

        return VOS_ERR;
    }

    g_stAcpuUeToPcSucRecord.stRealSendData.ulDataLen += (ulHeadLen + ulDataLen + (VOS_UINT32)sizeof(VOS_UINT32));
    g_stAcpuUeToPcSucRecord.stRealSendData.ulNum++;

    lUsedBytes = OM_TraceMemNBytes(&g_stAcpuOmBufCtrl);

    if (g_stAcpuUeToPcSucRecord.ulMaxUsedBufSize < (VOS_UINT32)lUsedBytes)
    {
        g_stAcpuUeToPcSucRecord.ulMaxUsedBufSize = (VOS_UINT32)lUsedBytes;
    }

    return VOS_OK;
}


VOS_UINT32 OMRL_AcpuIsBDEnough(VOS_UINT8 ucMsgCnt)
{
    VOS_UINT32                          ulFreeBdNum = 0;

    /* ��ȡOMԴͨ��ʣ��BD���� */
    if (VOS_OK != SCM_GetBDFreeNum(SOCP_CODER_SRC_GU_OM1, &ulFreeBdNum))
    {
        g_stAcpuUeToPcErrRecord.usGetBDErr++;

        return VOS_ERR;
    }

    /* �ж�ʣ��BD�Ƿ�����ְ����� */
    if (ucMsgCnt > ulFreeBdNum)
    {
        g_stAcpuUeToPcErrRecord.usBDLackErr++;

        return VOS_ERR;
    }

    return VOS_OK;
}

/*****************************************************************************
 �� �� ��  : OMRL_ProcessSendData
 ��������  : ����ά�ɲ����ݷ��͵�SOCPģ��
 �������  : pucMsg    -- ָ�������ݵ�ָ��
             usMsgLen  -- �������ݵĳ���
             ucMsgCnt  -- ���������ݰ�����
 �������  : ��
 �� �� ֵ  : VOS_UINT32

 �޸���ʷ      :
  1.��    ��   : 2009��3��28��
    ��    ��   : ���� 47350
    �޸�����   : �����ɺ���

*****************************************************************************/
VOS_UINT32 OMRL_AcpuProcessSendData(VOS_UINT8 *pucMsg, VOS_UINT8 ucMsgCnt, VOS_UINT16 usMsgLen)
{
    VOS_UINT8                           ucCurSegNum = 1; /*��ǰ�����*/
    VOS_UINT_PTR                        ulTempAddress;
    VOS_BOOL                            bIsHead     = VOS_FALSE;
    OM_APP_TRACE_STRU_EX               *pstAppTrace;

    pstAppTrace = (OM_APP_TRACE_STRU_EX*)pucMsg;

    ulTempAddress = (VOS_UINT_PTR)pucMsg;

    if ((OM_TRACE_FUNC == pstAppTrace->stAppHeader.ucFuncType)
        &&(OM_APP_TRACE_IND == pstAppTrace->usPrimId))
    {
        ulTempAddress = *(VOS_UINT_PTR*)(pstAppTrace->aucPara);
        VOS_MemCpy(g_pstAcpuSegMsgEx + 1, pucMsg, OM_APP_TRACE_LEN);
        bIsHead = VOS_TRUE;
    }

    /* ���SOCPͷ�е�RTCֵ */
    g_pstAcpuSegMsgEx->stSocpHdr.ulRTC = (VOS_UINT32)DRV_GET_RTC_VALUE();
    g_pstAcpuSegMsgEx->stMsgSeg.ucSegNum = ucMsgCnt;
    g_pstAcpuSegMsgEx->stMsgSeg.usSegLen = OM_APP_SPLIT_MSG_LEN;

    /* �������ְ���С�����ݣ��������ְ���С�������ݷ��͵Ĵ��� */
    for (ucCurSegNum = 1; ucCurSegNum < ucMsgCnt; ucCurSegNum++)
    {
        g_pstAcpuSegMsgEx->stMsgSeg.ucSegSn = ucCurSegNum;

        if (VOS_TRUE == bIsHead)
        {
            /* ��Ϣͷǰ���ϳ����Ų����뻷�λ�����*/
            if (VOS_OK != OMRL_AcpuSendSocpData((VOS_UINT8 *)g_pstAcpuSegMsgEx,
                (sizeof(OM_APP_MSG_SEG_EX_STRU) + OM_APP_TRACE_LEN),
                (VOS_UINT8*)ulTempAddress, OM_APP_MSG_SEGMENT_LEN - OM_APP_TRACE_LEN))
            {
                return VOS_ERR;
            }

            bIsHead       =  VOS_FALSE;
            ulTempAddress += (OM_APP_MSG_SEGMENT_LEN - OM_APP_TRACE_LEN);
        }
        else
        {
            /* ��Ϣͷǰ���ϳ����Ų����뻷�λ�����*/
            if (VOS_OK != OMRL_AcpuSendSocpData((VOS_UINT8 *)g_pstAcpuSegMsgEx,
                 sizeof(OM_APP_MSG_SEG_EX_STRU),
                (VOS_UINT8*)ulTempAddress, OM_APP_MSG_SEGMENT_LEN))
            {
                return VOS_ERR;
            }

            ulTempAddress += OM_APP_MSG_SEGMENT_LEN;
        }

        /* ����ʣ�����ݰ���С */
        usMsgLen -= OM_APP_MSG_SEGMENT_LEN;
    }

    /*������Ϣͷ����*/
    g_pstAcpuSegMsgEx->stMsgSeg.usSegLen = (VOS_UINT16)(usMsgLen + OM_APP_MSG_SEGMENT_HEADER_LENGTH);
    g_pstAcpuSegMsgEx->stMsgSeg.ucSegSn  = ucCurSegNum;

    /*Ϊ�˱���Ringbuffer�д�ŵ����ݰ�4�ֽڶ���*/
    usMsgLen = (usMsgLen + 3U) & ~0x03U;

    if (VOS_TRUE == bIsHead)
    {
        /* ��Ϣͷǰ���ϳ��Ȳ����뻷�λ����� */
        if ( VOS_OK != OMRL_AcpuSendSocpData((VOS_UINT8 *)g_pstAcpuSegMsgEx,
                            (sizeof(OM_APP_MSG_SEG_EX_STRU) + OM_APP_TRACE_LEN),
                            (VOS_UINT8*)ulTempAddress,
                            usMsgLen - OM_APP_TRACE_LEN))
        {
            return VOS_ERR;
        }
    }
    else
    {
        /* ��Ϣͷǰ���ϳ��Ȳ����뻷�λ����� */
        if ( VOS_OK != OMRL_AcpuSendSocpData((VOS_UINT8 *)g_pstAcpuSegMsgEx,
                                        sizeof(OM_APP_MSG_SEG_EX_STRU),
                                        (VOS_UINT8*)ulTempAddress,
                                        usMsgLen))
        {
            return VOS_ERR;
        }
    }

    return VOS_OK;
}

/*****************************************************************************
 �� �� ��  : OMRL_AcpuMsgSplit
 ��������  : ����USB�Ͻ���ͨ��ʱ��ʹ�õ���չ�ְ�����
 �������  : pucMsg    -- ָ�������ݵ�ָ��
             usMsgLen  -- �������ݵĳ���
 �������  : ��
 �� �� ֵ  : VOS_UINT32

 �޸���ʷ      :
  1.��    ��   : 2009��3��28��
    ��    ��   : ���� 47350
    �޸�����   : �����ɺ���

*****************************************************************************/
VOS_UINT32 OMRL_AcpuMsgSplit(VOS_UINT8 *pucMsg, VOS_UINT16 usMsgLen)
{
    VOS_UINT8                           ucMsgCnt    = 1; /*�ֶε�����*/
    VOS_UINT32                          ulRet;

    /*���л������*/
#if (VOS_LINUX == VOS_OS_VER)
    down(&g_stOmTxBuffSem);
#else
    if ( VOS_OK != VOS_SmP(g_AcpuOmTxBuffSem, OM_PV_TIMEOUT) )
    {
        LogPrint("OMRL_AcpuMsgSplit, Error, g_AcpuOmTxBuffSem VOS_SmP Failed.\n");

        g_stAcpuUeToPcErrRecord.usSemErr++;
        return VOS_ERR;
    }
#endif

    /* �ͷ��ѷ��������ڴ� */
    if (VOS_OK != OMRL_AcpuReleaseSocpData())
    {
#if (VOS_LINUX == VOS_OS_VER)
        up(&g_stOmTxBuffSem);
#else
        VOS_SmV(g_AcpuOmTxBuffSem);
#endif
        return VOS_ERR;
    }

    /* ����ְ����� */
    ucMsgCnt = (VOS_UINT8)(((usMsgLen + OM_APP_MSG_SEGMENT_LEN) - 1)/OM_APP_MSG_SEGMENT_LEN);

    /* ��ȡOMԴͨ��ʣ��BD���� */
    if (VOS_OK != OMRL_AcpuIsBDEnough(ucMsgCnt))
    {
#if (VOS_LINUX == VOS_OS_VER)
        up(&g_stOmTxBuffSem);
#else
        VOS_SmV(g_AcpuOmTxBuffSem);
#endif
        return VOS_ERR;
    }

    ulRet = OMRL_AcpuProcessSendData(pucMsg, ucMsgCnt, usMsgLen);

#if (VOS_LINUX == VOS_OS_VER)
    up(&g_stOmTxBuffSem);
#else
    VOS_SmV(g_AcpuOmTxBuffSem);
#endif

    return ulRet;
}
VOS_UINT32 OMRL_AcpuUsbFrameInit(VOS_VOID)
{
    /* ����ְ��ṹ���ڴ�ռ� */
    g_pstAcpuSegMsgEx = (OM_APP_MSG_SEG_EX_STRU*)VOS_MemAlloc(ACPU_PID_OM, STATIC_MEM_PT,
                        sizeof(OM_APP_MSG_SEG_EX_STRU) + OM_APP_TRACE_LEN);

    if (VOS_NULL_PTR == g_pstAcpuSegMsgEx)
    {
        LogPrint("OMRL_AcpuUsbFrameInit:VOS_MemAlloc g_pstSegMsgEx Failed!\n");
        return VOS_ERR;
    }

    g_pstAcpuSegMsgEx->stSocpHdr.ucCPUID   = OM_TOOL_ACPU_ID;
    g_pstAcpuSegMsgEx->ulFrameHead         = OM_USB_FRAME_HEAD;

    /* ����OM���ͻ����� */
    if (VOS_OK != OM_CreateTraceBuffer(&g_stAcpuOmBufCtrl, OM_UPLINK_BUFFER_SIZE))
    {
        VOS_MemFree(ACPU_PID_OM, g_pstAcpuSegMsgEx);

        LogPrint("OMRL_AcpuUsbFrameInit:OM_CreateTraceBuffer Failed!\n");

        return VOS_ERR;
    }

    return VOS_OK;
}


VOS_VOID OmAcpuInitVar(VOS_VOID)
{
    VOS_MemSet(&g_stAcpuPcToUeErrRecord, 0, sizeof(OM_ACPU_PC_UE_FAIL_STRU));
    VOS_MemSet(&g_stAcpuPcToUeSucRecord, 0, sizeof(OM_ACPU_PC_UE_SUC_STRU));
    VOS_MemSet(&g_stAcpuUeToPcErrRecord, 0, sizeof(OM_UE_PC_FAIL_STRU));
    VOS_MemSet(&g_stAcpuUeToPcSucRecord, 0, sizeof(OM_UE_PC_SUC_STRU));

#if (FEATURE_ON == FEATURE_CBT_LOG)
    VOS_MemSet(&g_stCBTAcpuPcToUeErrRecord, 0, sizeof(OM_ACPU_PC_UE_FAIL_STRU));
    VOS_MemSet(&g_stCBTAcpuPcToUeSucRecord, 0, sizeof(OM_ACPU_PC_UE_SUC_STRU));

#if (VOS_LINUX == VOS_OS_VER)
    sema_init(&g_stOmRxCBTBuffSem, 1);
#endif
#endif
}


VOS_UINT32 OMRL_AcpuInit(VOS_VOID)
{
    OmAcpuInitVar();

    VOS_MemSet(&g_stMsgCombineInfo, 0, sizeof(g_stMsgCombineInfo));

    /* �װ����Ϊ1 */
    g_stMsgCombineInfo.ucExpectedSegSn = 1;

#if (FEATURE_ON == FEATURE_CBT_LOG)
    VOS_MemSet(&g_stCBTMsgCombineInfo, 0, sizeof(g_stCBTMsgCombineInfo));

    /* �װ����Ϊ1 */
    g_stCBTMsgCombineInfo.ucExpectedSegSn = 1;
#endif

#if (VOS_LINUX == VOS_OS_VER)
    sema_init(&g_stOmTxBuffSem, 1);
#else
    if (VOS_OK != VOS_SmMCreate("TXRB", VOS_SEMA4_FIFO, &g_AcpuOmTxBuffSem))
    {
        LogPrint("OMRL_AcpuInit: Error, VOS_SmMCreate Fail\n");
        return VOS_ERR;
    }
#endif

    if (VOS_OK != OMRL_AcpuHdlcInit())
    {
        LogPrint("OMRL_AcpuInit:OMRL_AcpuHdlcInit Fail.\n");
        return VOS_ERR;
    }

    /* ��36��NV��Ϊ��Ƭ�汾 */
    if (VOS_OK != OMRL_AcpuNVHdlcInit())
    {
        LogPrint("OMRL_AcpuInit:OMRL_AcpuNVHdlcInit Fail.\n");
        return VOS_ERR;
    }

    if (VOS_OK != OMRL_AcpuUsbFrameInit())
    {
        LogPrint("OMRL_AcpuInit:OMRL_AcpuUsbFrameInit Fail.\n");
        return VOS_ERR;
    }

    /* ��OM SOCPĿ��ͨ����������ע���SCM */
    if (VOS_OK != SCM_RegCoderDestProc(SOCP_CODER_DST_GU_OM, (SCM_CODERDESTFUCN)GU_OamSocpSendDataToUDI))
    {
        LogPrint("OMRL_AcpuInit:SCM_RegCoderDestProc Fail.\n");

        return VOS_ERR;
    }

    CPM_LogicRcvReg(CPM_OM_COMM, OMRL_AcpuRcvData);

    return VOS_OK;
}


VOS_VOID OmAcpuSendShow(VOS_VOID)
{
    vos_printf("Total Data Send by Ps: num:%6u, len:%6u\n",
                g_stAcpuUeToPcSucRecord.stTotalData.ulNum, g_stAcpuUeToPcSucRecord.stTotalData.ulDataLen);

    vos_printf("Real Data send to Socp: num:%6u, len:%6u\n",
                g_stAcpuUeToPcSucRecord.stRealSendData.ulNum, g_stAcpuUeToPcSucRecord.stRealSendData.ulDataLen);

    vos_printf("Real Data release by Socp: num:%6u, len:%6u\n",
                g_stAcpuUeToPcSucRecord.stRealRlsData.ulNum, g_stAcpuUeToPcSucRecord.stRealRlsData.ulDataLen);

    vos_printf("Max Used Buffer Size: size:%6u\n", g_stAcpuUeToPcSucRecord.ulMaxUsedBufSize);

    return;
}


VOS_VOID OmAcpuSendErrShow(VOS_VOID)
{
    vos_printf("Interrput Lost Data: num:%6u, len:%6u\n",
                g_stAcpuUeToPcErrRecord.stIntData.ulNum, g_stAcpuUeToPcErrRecord.stIntData.ulDataLen);

    vos_printf("Total Lost Data: num:%6u, len:%6u\n",
                g_stAcpuUeToPcErrRecord.stLostData.ulNum, g_stAcpuUeToPcErrRecord.stLostData.ulDataLen);

    vos_printf("Link Err Data: num:%6u, len:%6u\n",
                g_stAcpuUeToPcErrRecord.stLinkErrData.ulNum, g_stAcpuUeToPcErrRecord.stLinkErrData.ulDataLen);

    vos_printf("Para Err Data: num:%6u, len:%6u\n",
                g_stAcpuUeToPcErrRecord.stParaErrData.ulNum, g_stAcpuUeToPcErrRecord.stParaErrData.ulDataLen);

    vos_printf("num of sem take err: num:%6u\n", g_stAcpuUeToPcErrRecord.usSemErr);

    vos_printf("num of release RD err: num:%6u\n", g_stAcpuUeToPcErrRecord.usRlsRdErr);

    vos_printf("num of release Mem err: num:%6u\n", g_stAcpuUeToPcErrRecord.usRlsMemErr);

    vos_printf("num of BD Lack err: num:%6u\n", g_stAcpuUeToPcErrRecord.usBDLackErr);

    vos_printf("num of get BD err: num:%6u\n", g_stAcpuUeToPcErrRecord.usGetBDErr);

    vos_printf("num of No Mem err: num:%6u\n", g_stAcpuUeToPcErrRecord.usNoMemErr);

    vos_printf("num of Send Socp Data err: num:%6u\n", g_stAcpuUeToPcErrRecord.usSendSocpDataErr);

    vos_printf("num of transfer sending addr err: num:%6u\n", g_stAcpuUeToPcErrRecord.usSendAddrErr);

    vos_printf("num of transfer Release addr err: num:%6u\n", g_stAcpuUeToPcErrRecord.usRlsAddrErr);

    return;
}



VOS_VOID OmAcpuRcvShow(VOS_VOID)
{
    vos_printf("Total Data Recv From PC: num:%6u, len:%6u\n",
                g_stAcpuPcToUeSucRecord.stTotalData.ulNum, g_stAcpuPcToUeSucRecord.stTotalData.ulDataLen);

    vos_printf("Data after Hdlc decap:   num:%6u, len:%6u\n",
                g_stAcpuPcToUeSucRecord.stHdlcDecapData.ulNum, g_stAcpuPcToUeSucRecord.stHdlcDecapData.ulDataLen);

    vos_printf("Link Data Recv From PC: num:%6u, len:%6u\n",
                g_stAcpuPcToUeSucRecord.stLinkData.ulNum, g_stAcpuPcToUeSucRecord.stLinkData.ulDataLen);

    vos_printf("CCPU Data Recv From PC: num:%6u, len:%6u\n",
                g_stAcpuPcToUeSucRecord.stCcpuData.ulNum, g_stAcpuPcToUeSucRecord.stCcpuData.ulDataLen);

    vos_printf("ACPU Data Recv From PC: num:%6u, len:%6u\n",
                g_stAcpuPcToUeSucRecord.stAcpuData.ulNum, g_stAcpuPcToUeSucRecord.stAcpuData.ulDataLen);

    vos_printf("HIFI Data Recv From PC: num:%6u, len:%6u\n",
                g_stAcpuPcToUeSucRecord.stHifiData.ulNum, g_stAcpuPcToUeSucRecord.stHifiData.ulDataLen);

    vos_printf("MCU Data Recv From PC: num:%6u, len:%6u\n",
                g_stAcpuPcToUeSucRecord.stMcuData.ulNum, g_stAcpuPcToUeSucRecord.stMcuData.ulDataLen);

    vos_printf("Data Send by ICC: num:%6u, len:%6u, slice:%6u\n",
                g_stAcpuPcToUeSucRecord.stICCSendSUCInfo.ulICCOmSendMsgNum,
                g_stAcpuPcToUeSucRecord.stICCSendSUCInfo.ulICCOmSendLen,
                g_stAcpuPcToUeSucRecord.stICCSendSUCInfo.ulICCOmSendSlice);

    vos_printf("Rls Data Recv From PC: num:%6u, slice:%6u\n",
                g_stAcpuPcToUeSucRecord.stRlsData.ulNum, g_stAcpuPcToUeSucRecord.stRlsData.ulDataLen);

#if (FEATURE_ON == FEATURE_CBT_LOG)
    vos_printf("CBT LOG Total Data Recv From PC: num:%6u, len:%6u\n",
                g_stCBTAcpuPcToUeSucRecord.stTotalData.ulNum, g_stCBTAcpuPcToUeSucRecord.stTotalData.ulDataLen);

    vos_printf("CBT LOG Data after Hdlc decap:   num:%6u, len:%6u\n",
                g_stCBTAcpuPcToUeSucRecord.stHdlcDecapData.ulNum, g_stCBTAcpuPcToUeSucRecord.stHdlcDecapData.ulDataLen);

    vos_printf("CBT LOG Link Data Recv From PC: num:%6u, len:%6u\n",
                g_stCBTAcpuPcToUeSucRecord.stLinkData.ulNum, g_stCBTAcpuPcToUeSucRecord.stLinkData.ulDataLen);

    vos_printf("CBT LOG CCPU Data Recv From PC: num:%6u, len:%6u\n",
                g_stCBTAcpuPcToUeSucRecord.stCcpuData.ulNum, g_stCBTAcpuPcToUeSucRecord.stCcpuData.ulDataLen);

    vos_printf("CBT LOG ACPU Data Recv From PC: num:%6u, len:%6u\n",
                g_stCBTAcpuPcToUeSucRecord.stAcpuData.ulNum, g_stCBTAcpuPcToUeSucRecord.stAcpuData.ulDataLen);

    vos_printf("CBT LOG HIFI Data Recv From PC: num:%6u, len:%6u\n",
                g_stCBTAcpuPcToUeSucRecord.stHifiData.ulNum, g_stCBTAcpuPcToUeSucRecord.stHifiData.ulDataLen);

    vos_printf("CBT LOG MCU Data Recv From PC: num:%6u, len:%6u\n",
                g_stCBTAcpuPcToUeSucRecord.stMcuData.ulNum, g_stCBTAcpuPcToUeSucRecord.stMcuData.ulDataLen);

    vos_printf("CBT LOG Data Send by ICC: num:%6u, len:%6u, slice:%6u\n",
                g_stCBTAcpuPcToUeSucRecord.stICCSendSUCInfo.ulICCOmSendMsgNum,
                g_stCBTAcpuPcToUeSucRecord.stICCSendSUCInfo.ulICCOmSendLen,
                g_stCBTAcpuPcToUeSucRecord.stICCSendSUCInfo.ulICCOmSendSlice);

    vos_printf("CBT LOG Rls Data Recv From PC: num:%6u, slice:%6u\n",
                g_stCBTAcpuPcToUeSucRecord.stRlsData.ulNum, g_stCBTAcpuPcToUeSucRecord.stRlsData.ulDataLen);
#endif
    return;
}


VOS_VOID OmAcpuRcvErrShow(VOS_VOID)
{
    vos_printf("Total Lost Data Recv From PC: num:%6u, len:%6u\n",
            g_stAcpuPcToUeErrRecord.stLostData.ulNum, g_stAcpuPcToUeErrRecord.stLostData.ulDataLen);

    vos_printf("Number of Frame Decap Err: num:%6u\n", g_stAcpuPcToUeErrRecord.ulFrameDecapErr);

    vos_printf("Number of Packet Len Err: num:%6u\n", g_stAcpuPcToUeErrRecord.usPacketLenErr);

    vos_printf("Number of Too Short Err: num:%6u\n", g_stAcpuPcToUeErrRecord.usLenTooShortErr);

    vos_printf("Number of Seg Len Err: num:%6u\n", g_stAcpuPcToUeErrRecord.usSegLenErr);

    vos_printf("Number of Seg Num Err: num:%6u\n", g_stAcpuPcToUeErrRecord.usSegNumErr);

    vos_printf("Number of DataType Err: num:%6u\n", g_stAcpuPcToUeErrRecord.usDatatypeErr);

    vos_printf("Number of Too Long Err: num:%6u\n", g_stAcpuPcToUeErrRecord.usMsgTooLongErr);

    vos_printf("Number of Cpu Id Err: num:%6u\n", g_stAcpuPcToUeErrRecord.usCpuIdErr);

    vos_printf("Number of No Mem Err: num:%6u\n", g_stAcpuPcToUeErrRecord.usNoMemErr);

    vos_printf("Number of Data Head Err: num:%6u\n", g_stAcpuPcToUeErrRecord.usDataHeadErr);

    vos_printf("Number of Msg Len Err: num:%6u\n", g_stAcpuPcToUeErrRecord.usMsgLenErr);

    vos_printf("Number of Msg Sn Err: num:%6u\n", g_stAcpuPcToUeErrRecord.usMsgSnErr);

    vos_printf("Number of Packet Lost Err: num:%6u\n", g_stAcpuPcToUeErrRecord.usPacketLostErr);

    vos_printf("Number of Link Data Len Err: num:%6u\n", g_stAcpuPcToUeErrRecord.usLinkDataLenErr);

    vos_printf("Number of Alloc msg Mem: num: %6u\n",g_stAcpuPcToUeErrRecord.usAllocMsg);

    vos_printf("Number of Lost Msg Because Switch Is IDLE: num: %6u\n",g_stAcpuPcToUeErrRecord.usLinkStatusErr);

    vos_printf("Data Send err by ICC: num:%6u, len:%6u, slice:%6u\n",
            g_stAcpuPcToUeErrRecord.stICCSendFailInfo.ulICCOmSendErrNum,
            g_stAcpuPcToUeErrRecord.stICCSendFailInfo.ulICCOmSendErrLen,
            g_stAcpuPcToUeErrRecord.stICCSendFailInfo.ulICCOmSendErrSlice);

#if (FEATURE_ON == FEATURE_CBT_LOG)
    vos_printf("CBT LOG Total Lost Data Recv From PC: num:%6u, len:%6u\n",
               g_stCBTAcpuPcToUeErrRecord.stLostData.ulNum, g_stCBTAcpuPcToUeErrRecord.stLostData.ulDataLen);

    vos_printf("CBT LOG Number of Frame Decap Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.ulFrameDecapErr);

    vos_printf("CBT LOG Number of Packet Len Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.usPacketLenErr);

    vos_printf("CBT LOG Number of Too Short Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.usLenTooShortErr);

    vos_printf("CBT LOG Number of Seg Len Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.usSegLenErr);

    vos_printf("CBT LOG Number of Seg Num Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.usSegNumErr);

    vos_printf("CBT LOG Number of DataType Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.usDatatypeErr);

    vos_printf("CBT LOG Number of Too Long Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.usMsgTooLongErr);

    vos_printf("CBT LOG Number of Cpu Id Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.usCpuIdErr);

    vos_printf("CBT LOG Number of No Mem Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.usNoMemErr);

    vos_printf("CBT LOG Number of Data Head Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.usDataHeadErr);

    vos_printf("CBT LOG Number of Msg Len Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.usMsgLenErr);

    vos_printf("CBT LOG Number of Msg Sn Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.usMsgSnErr);

    vos_printf("CBT LOG Number of Packet Lost Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.usPacketLostErr);

    vos_printf("CBT LOG Number of Link Data Len Err: num:%6u\n", g_stCBTAcpuPcToUeErrRecord.usLinkDataLenErr);

    vos_printf("CBT LOG Number of Alloc msg Mem: num: %6u\n",g_stCBTAcpuPcToUeErrRecord.usAllocMsg);

    vos_printf("CBT LOG Number of Lost Msg Because Switch Is IDLE: num: %6u\n",g_stCBTAcpuPcToUeErrRecord.usLinkStatusErr);

    vos_printf("CBT LOG Data Send err by ICC: num:%6u, len:%6u, slice:%6u\n",
            g_stCBTAcpuPcToUeErrRecord.stICCSendFailInfo.ulICCOmSendErrNum,
            g_stCBTAcpuPcToUeErrRecord.stICCSendFailInfo.ulICCOmSendErrLen,
            g_stCBTAcpuPcToUeErrRecord.stICCSendFailInfo.ulICCOmSendErrSlice);
#endif

    return;
}


VOS_VOID OmOpenLog(VOS_UINT32 ulFlag)
{
    g_ulOmAcpuDbgFlag = ulFlag;

    return;
}



VOS_VOID OmAcpuRate(VOS_VOID)
{
    VOS_UINT32            ulCurrTick;                    /*��¼����ͳ��ʱ��*/
    VOS_UINT32            ulDeltaTick;                   /*��¼ͳ��ʱ��*/
    VOS_UINT32            ulDelData;                     /*��¼ͳ�����ݴ�С*/
    VOS_UINT32            ulBandWidth;                   /*��¼ͳ�����ݴ�С*/

    vos_printf("OmAcpuRate Show(ALL Data To UDI):\n");

    ulCurrTick = VOS_GetTick();

    if (ulCurrTick < g_ulAcpuLastTick)
    {
        ulDeltaTick = (0xFFFFFFFF - g_ulAcpuLastTick) + ulCurrTick;
    }
    else
    {
        ulDeltaTick = ulCurrTick - g_ulAcpuLastTick;
    }

    ulDelData      = g_stAcpuDebugInfo.ulUSBSendLen - g_ulAcpuLastByteCnt;
    g_ulAcpuLastByteCnt  = g_stAcpuDebugInfo.ulUSBSendLen;

    ulBandWidth    = (ulDelData/ulDeltaTick)/10;

    /*ͳ��OM��ulDeltaTickʱ��(��λ10ms)�ڷ�����ulTotalLen�ֽ�����*/
    vos_printf("OmAcpuRate,Tick <%u> Byte <%u>, BandWidth <%u>.\n",
                 ulDeltaTick, ulDelData, ulBandWidth);

    g_ulAcpuLastTick = ulCurrTick;

    vos_printf("OmAcpuRate Show End\n");
}


VOS_VOID OmAcpuShowCombineInfo(VOS_VOID)
{
    vos_printf("pstWholeMsg:        0x%p\r\n",g_stMsgCombineInfo.pstWholeMsg);
    vos_printf("ucExpectedSegSn:    0x%x\r\n",g_stMsgCombineInfo.ucExpectedSegSn);
    vos_printf("ucTotalSegNum:      0x%x\r\n",g_stMsgCombineInfo.ucTotalSegNum);
    vos_printf("ulMoveLen:          0x%x\r\n",g_stMsgCombineInfo.ulMoveLen);
    vos_printf("ulTotalMsgLen:      0x%x\r\n",g_stMsgCombineInfo.ulTotalMsgLen);
}
#else
/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
/* CNFͨ��������Ϣȫ�ֱ��� */
OMRL_RCV_CHAN_CTRL_INFO_STRU            g_stAcpuCnfCtrlInfo;

/* CBTͨ��������Ϣȫ�ֱ��� */
OMRL_RCV_CHAN_CTRL_INFO_STRU            g_stAcpuCbtCtrlInfo;

/*��¼��ʼʱ��*/
VOS_UINT32                              g_ulAcpuLastTick = 0;

/*��¼�ֽڳ���*/
VOS_UINT32                              g_ulAcpuLastByteCnt = 0;

/* ��������OM�Ĵ�ӡ��� */
#ifdef OAM_UT_DEBUG
VOS_UINT32                              g_ulOmAcpuDbgFlag = VOS_NULL_DWORD;
#else
VOS_UINT32                              g_ulOmAcpuDbgFlag = VOS_FALSE;
#endif

OMRL_MNTN_TX_CHAN_CTRL_INFO_STRU        g_stAcpuTxCnfCtrlInfo;

OMRL_MNTN_TX_CHAN_CTRL_INFO_STRU        g_stAcpuTxIndCtrlInfo;

OMRL_CBT_HDLC_ENCODE_MEM_CTRL           g_stCbtHdlcEncBuf;

VOS_UINT32                              g_ulCBTLogEnable = 0;

/* ����SD����Debug ��Ϣ */
extern OM_SD_DEBUG_INFO                 g_stOmSDDebugInfo;
extern OM_VCOM_DEBUG_INFO               g_stVComDebugInfo[3];
extern OM_RECORD_BUF_STRU               g_astAcpuRecordInfo[VOS_EXC_DUMP_MEM_NUM_BUTT];

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/


VOS_UINT32 OMRL_AcpuCpuIdToPid(VOS_UINT8 ucCpuId, VOS_UINT32 *pulPid)
{

      switch(ucCpuId)
      {
        case OM_TOOL_RSV_ID:
        case OM_TOOL_CCPU_ID:
            *pulPid = WUEPS_PID_OM;/* ccpu om pid */
            break;
        case OM_TOOL_ACPU_ID:
            *pulPid = ACPU_PID_OM; /* acpu om pid */
            break;
        case OM_TOOL_HIFI_ID:
            *pulPid = DSP_PID_HIFI_OM;/* HIFI om pid */
            break;
        case OM_TOOL_MCU_ID:
            *pulPid = ACPU_PID_MCU_OM;/* MCU om pid */
            break;
        default:
            /* ��ӡ���� */
            return VOS_ERR;
      }

      return VOS_OK;
}
VOS_BOOL OMRL_JudgeCbtMsg(VOS_UINT8   ucFuncType)
{
    if ( (OM_FS_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
                 ||(OM_WRF_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
                 ||(OM_GRF_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
                 ||(OM_QUERY_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
                 ||(OM_NV_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
                 ||(OM_NOSIG_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS))
                 ||(OM_CBT_TRANS_FUNC == ((ucFuncType) & OM_FUNCID_VALUE_BITS)) )
    {
        return VOS_TRUE;
    }

    return VOS_FALSE;
}


VOS_UINT32 OMRL_AcpuMsgDispatch(OMRL_RCV_CHAN_CTRL_INFO_STRU *pstCtrlInfo)
{
    VOS_UINT32                          ulRslt;
    OMRL_MSG_COMBINE_INFO_STRU          *pstCombineInfo;
    APP_OM_MSG_EX_STRU                  *pstAppToOmMsg;

    pstCombineInfo = &(pstCtrlInfo->stMsgCombineInfo);

    pstAppToOmMsg = (APP_OM_MSG_EX_STRU*)(pstCombineInfo->pstWholeMsg)->aucValue;
    pstAppToOmMsg->ucCpuId = (VOS_UINT8)pstCtrlInfo->enChannel;

    /*lint -e40*/
    OM_ACPU_DEBUG_CHANNEL_TRACE(pstCtrlInfo->enChannel, (pstCombineInfo->pstWholeMsg)->aucValue, (pstCombineInfo->pstWholeMsg)->ulLength, OM_ACPU_DISPATCH_MSG);
    /*lint +e40*/

    if (OM_LOGIC_CHANNEL_CBT == pstCtrlInfo->enChannel)
    {
        /* ȡ��Ϣ�е�function type�ж��ǲ�������У׼���� */
        if (VOS_TRUE != OMRL_JudgeCbtMsg(pstAppToOmMsg->ucFuncType))
        {
            /* �������У׼ͨ����������ϵ�У׼ͨ������· */
            return PPM_DisconnectGUPort(OM_LOGIC_CHANNEL_CBT);
        }
    }

    /* CCPU����Ϣͨ��ICCͨ������ */
    if (WUEPS_PID_OM == (pstCombineInfo->pstWholeMsg)->ulReceiverPid)
    {
        pstCtrlInfo->stPcToUeSucRecord.stCcpuData.ulDataLen += (pstCombineInfo->pstWholeMsg)->ulLength;
        pstCtrlInfo->stPcToUeSucRecord.stCcpuData.ulNum++;

        ulRslt = GU_OamSndPcMsgToCcpu(pstCtrlInfo, (pstCombineInfo->pstWholeMsg)->aucValue, (pstCombineInfo->pstWholeMsg)->ulLength);

        /* �ͷ���Ϣ�ռ� */
        VOS_FreeMsg(ACPU_PID_OM, pstCombineInfo->pstWholeMsg);
    }
    else if (ACPU_PID_OM == (pstCombineInfo->pstWholeMsg)->ulReceiverPid)
    {
        /* ����cpu����Ϣͨ��VOS_SendMsg���� */
        pstCtrlInfo->stPcToUeSucRecord.stAcpuData.ulDataLen += (pstCombineInfo->pstWholeMsg)->ulLength;
        pstCtrlInfo->stPcToUeSucRecord.stAcpuData.ulNum++;

        ulRslt = VOS_SendMsg(ACPU_PID_OM, (pstCombineInfo->pstWholeMsg));
    }
    else if (DSP_PID_HIFI_OM == (pstCombineInfo->pstWholeMsg)->ulReceiverPid)
    {
        /* ����cpu����Ϣͨ��VOS_SendMsg���� */
        pstCtrlInfo->stPcToUeSucRecord.stHifiData.ulDataLen += (pstCombineInfo->pstWholeMsg)->ulLength;
        pstCtrlInfo->stPcToUeSucRecord.stHifiData.ulNum++;

        ulRslt = VOS_SendMsg(ACPU_PID_OM, (pstCombineInfo->pstWholeMsg));
    }
    else
    {
        /* ����cpu����Ϣͨ��VOS_SendMsg���� */
        pstCtrlInfo->stPcToUeSucRecord.stMcuData.ulDataLen += (pstCombineInfo->pstWholeMsg)->ulLength;
        pstCtrlInfo->stPcToUeSucRecord.stMcuData.ulNum++;

        ulRslt = VOS_SendMsg(ACPU_PID_OM, (pstCombineInfo->pstWholeMsg));
    }

    pstCombineInfo->pstWholeMsg = VOS_NULL_PTR;

    if (VOS_OK != ulRslt )
    {
        return VOS_ERR;
    }

    return VOS_OK;
}
VOS_UINT32 OMRL_AcpuMsgCombine(OMRL_RCV_CHAN_CTRL_INFO_STRU *pstCtrlInfo, VOS_UINT8 *pucData, VOS_UINT16 usLen)
{
    VOS_UINT32                       ulRslt = VOS_OK;/*Get the return value of PS_SEND_MSG.*/
    VOS_UINT8                        ucCurrentSegSn;/*Reserve the current packet's SN.*/
    OM_APP_MSG_SEGMENT_STRU          *pstOmAppSeg;
    APP_OM_GU_MSG_STRU               *pstAppOmMsg;
    OMRL_MSG_COMBINE_INFO_STRU       *pstCombineInfo;

    pstCombineInfo = &(pstCtrlInfo->stMsgCombineInfo);

    /*Check the input parameter's validity.Make sure empty segment can't pass*/
    if ( (VOS_NULL_PTR == pucData) ||
        (usLen <= sizeof(OM_APP_MSG_SEGMENT_STRU)) )
    {
        LogPrint("OMRL_AcpuMsgCombine: The input is wrong.");

        pstCtrlInfo->stPcToUeErrRecord.usLenTooShortErr++;

        return VOS_ERR;
    }

    pstOmAppSeg = (OM_APP_MSG_SEGMENT_STRU *)pucData;

    /*Check the input parameter's validity.*/
    if (usLen != ((pstOmAppSeg->usSegLen + 3) & ~0x3))
    {
        LogPrint("OMRL_AcpuMsgCombine: The input length is wrong.");

        pstCtrlInfo->stPcToUeErrRecord.usSegLenErr++;

        return VOS_ERR;
    }

    usLen = pstOmAppSeg->usSegLen;

    ucCurrentSegSn = pstOmAppSeg->ucSegSn;

    /*When the MSG packet is new, we need initialize the static variable.*/
    if (1 == ucCurrentSegSn)
    {
        if (VOS_NULL_PTR != pstCombineInfo->pstWholeMsg)
        {
            VOS_FreeMsg(ACPU_PID_OM, pstCombineInfo->pstWholeMsg);
            pstCombineInfo->pstWholeMsg = VOS_NULL_PTR;

            pstCtrlInfo->stPcToUeErrRecord.usPacketLostErr++;
        }

        /*The new MSG packet is coming.*/
        pstCombineInfo->ucExpectedSegSn = 1;
        pstCombineInfo->ulMoveLen       = 0;

        if (usLen < (sizeof(APP_OM_GU_MSG_STRU) + OM_APP_MSG_SEGMENT_HEADER_LENGTH))
        {
            LogPrint("OMRL_AcpuMsgCombine: The input parameter is wrong.");

            pstCtrlInfo->stPcToUeErrRecord.usMsgLenErr++;

            return VOS_ERR;
        }

        pstAppOmMsg = (APP_OM_GU_MSG_STRU *)(pucData + OM_APP_MSG_SEGMENT_HEADER_LENGTH);

        pstCombineInfo->ulTotalMsgLen = pstAppOmMsg->usLength + APP_OM_HEADER_LENGTH;

        /* �����ɺ��ж��Ƿ������ֵ */
        if (OM_RL_TOTAL_MSG_MAX_LEN < pstCombineInfo->ulTotalMsgLen)
        {
            pstCtrlInfo->stPcToUeErrRecord.usMsgTooLongErr++;

            LogPrint1("OMRL_AcpuMsgCombine: receive first msg pack is too long %d;\r\n",
                        (VOS_INT32)pstCombineInfo->ulTotalMsgLen);

            /* �������֧�ֱ���1024�ֽڵ����ݣ�����û���ṩ�꣬����ֱ��ʹ������ */
            DRV_SYSTEM_ERROR(OAM_PC_LENGTH_TOO_BIG, (VOS_INT)pstCombineInfo->ulTotalMsgLen, 0, (VOS_CHAR *)pucData, 1024);

            return VOS_ERR;
        }

        /*Allocate the memory space.*/
        pstCombineInfo->pstWholeMsg = VOS_AllocMsg(ACPU_PID_OM,  pstCombineInfo->ulTotalMsgLen);
        if (VOS_NULL_PTR == pstCombineInfo->pstWholeMsg)
        {
            LogPrint("OMRL_AcpuMsgCombine: VOS_AllocMsg fail.");

            pstCtrlInfo->stPcToUeErrRecord.usAllocMsg++;

            return VOS_ERR;
        }

        /* ����CPU ID���ҵ���Ӧ��PID�����ڹ����·�����֡����VOS_OM_HEADER�ṹ�б����ֶα�ʾCPUID */
        ulRslt = OMRL_AcpuCpuIdToPid(pstAppOmMsg->ucCpuId, &(pstCombineInfo->pstWholeMsg)->ulReceiverPid);
        if (VOS_OK != ulRslt)
        {
            /* �ͷ���Ϣ�ռ� */
            VOS_FreeMsg(ACPU_PID_OM, pstCombineInfo->pstWholeMsg);

            pstCombineInfo->pstWholeMsg = VOS_NULL_PTR;

            pstCtrlInfo->stPcToUeErrRecord.usCpuIdErr++;

            /* �˴�����ֱ�ӷ��أ����ü�¼�����������Ѿ���¼��CPU ID�������� */
            return VOS_ERR;
        }

        pstCombineInfo->ucTotalSegNum = pstOmAppSeg->ucSegNum;
    }

    if ( pstCombineInfo->ucTotalSegNum != pstOmAppSeg->ucSegNum )
    {
        LogPrint("OMRL_AcpuMsgCombine: The length of the packet is biger than the size of allocated memory.");

        /* �ͷ���Ϣ�ռ� */
        VOS_FreeMsg(ACPU_PID_OM, pstCombineInfo->pstWholeMsg);

        pstCombineInfo->pstWholeMsg = VOS_NULL_PTR;

        pstCtrlInfo->stPcToUeErrRecord.usSegNumErr++;

        return VOS_ERR;
    }

    /*The expected MSG pakcet is lost.*/
    if (pstCombineInfo->ucExpectedSegSn != ucCurrentSegSn)
    {
        /*Print the error info.*/
        LogPrint2("OMRL_AcpuMsgCombine: expected SN is %d, current SN is %d.", pstCombineInfo->ucExpectedSegSn, ucCurrentSegSn);

        /* �ͷ���Ϣ�ռ� */
        VOS_FreeMsg(ACPU_PID_OM, pstCombineInfo->pstWholeMsg);

        pstCombineInfo->pstWholeMsg = VOS_NULL_PTR;

        pstCtrlInfo->stPcToUeErrRecord.usMsgSnErr++;

        return VOS_ERR;
    }

    /*We make sure that the memory can't be violated.*/
    if (pstCombineInfo->ulTotalMsgLen < (pstCombineInfo->ulMoveLen + (usLen - OM_APP_MSG_SEGMENT_HEADER_LENGTH)))
    {
        LogPrint("OMRL_AcpuMsgCombine: The length of the packet is biger than the size of allocated memory.\n");

        /* �ͷ���Ϣ�ռ� */
        VOS_FreeMsg(ACPU_PID_OM, pstCombineInfo->pstWholeMsg);

        pstCombineInfo->pstWholeMsg = VOS_NULL_PTR;

        pstCtrlInfo->stPcToUeErrRecord.usNoMemErr++;

        return VOS_ERR;
    }

    VOS_MemCpy((VOS_UINT8*)((pstCombineInfo->pstWholeMsg)->aucValue) + pstCombineInfo->ulMoveLen,
               pucData + OM_APP_MSG_SEGMENT_HEADER_LENGTH, usLen - OM_APP_MSG_SEGMENT_HEADER_LENGTH);
    pstCombineInfo->ulMoveLen += usLen - OM_APP_MSG_SEGMENT_HEADER_LENGTH;
    pstCombineInfo->ucExpectedSegSn++;

    /*If the current MSG packet is an complete packet.*/
    if (pstCombineInfo->ucTotalSegNum == ucCurrentSegSn)
    {
        /* �����ɺ��ж��Ƿ������ֵ */
        if (OM_RL_TOTAL_MSG_MAX_LEN < pstCombineInfo->ulTotalMsgLen)
        {
            pstCtrlInfo->stPcToUeErrRecord.usMsgTooLongErr++;

            /* �ͷ���Ϣ�ռ� */
            VOS_FreeMsg(ACPU_PID_OM, pstCombineInfo->pstWholeMsg);

            pstCombineInfo->pstWholeMsg = VOS_NULL_PTR;

            return VOS_ERR;
        }

        /*It will send the MSG to OM module.*/
        ulRslt = OMRL_AcpuMsgDispatch(pstCtrlInfo);

        return ulRslt;
    }

    return VOS_OK;
}


VOS_UINT32 OMRL_AcpuSendMsg(VOS_UINT8 *pucData, VOS_UINT32 ulDataLen, VOS_UINT32 ulSenderPid, VOS_UINT32 ulReceiverPid)
{

    MsgBlock           *pstAppOmMsg = VOS_NULL_PTR;

    pstAppOmMsg = VOS_AllocMsg(ulSenderPid, ulDataLen);
    if (VOS_NULL_PTR == pstAppOmMsg)
    {
        return VOS_ERR;
    }

    pstAppOmMsg->ulReceiverPid = ulReceiverPid;

    /* copy ��Ϣ���� */
    VOS_MemCpy((VOS_UINT8 *)pstAppOmMsg->aucValue, pucData, ulDataLen);

    return VOS_SendMsg(ulSenderPid, pstAppOmMsg);

}
VOS_UINT32 OMRL_AcpuHeadProc(OMRL_RCV_CHAN_CTRL_INFO_STRU *pstCtrlInfo, VOS_UINT8 *pucData, VOS_UINT32 ulSize)
{
    VOS_UINT32                          ulValue;
    VOS_INT                             lRet;
    VOS_UINT8                           aucLinkData[] = {0x55, 0x55, 0xaa, 0xaa, 0x10, 0x00, 0x01, 0x01,
                                                         0x0a, 0x01, 0x08, 0x00, 0xb1, 0x80, 0x00, 0x00,
                                                         0x01, 0x00, 0x00, 0x00, 0xaa, 0xaa, 0x55, 0x55};

    pstCtrlInfo->stPcToUeSucRecord.stTotalData.ulDataLen += ulSize;
    pstCtrlInfo->stPcToUeSucRecord.stTotalData.ulNum++;

    /*lint -e40*/
    OM_ACPU_DEBUG_CHANNEL_TRACE(pstCtrlInfo->enChannel, pucData, ulSize, OM_ACPU_SEND_USB);
    /*lint +e40*/

    if (OM_USB_FRAME_LEN >= ulSize)
    {
        /* ��ӡ���� */
        LogPrint("OMRL_AcpuHeadProc: The data length is error.");
        pstCtrlInfo->stPcToUeErrRecord.usPacketLenErr++;
        return VOS_ERR;
    }

    VOS_MemCpy(&ulValue, pucData, sizeof(VOS_UINT32));

    /* �ж����ݰ�ͷ�ǲ���0xaaaa5555 */
    if (OM_USB_FRAME_HEAD != ulValue)
    {
        /* ��¼���� */
        pstCtrlInfo->stPcToUeErrRecord.usDataHeadErr++;
        return VOS_ERR;
    }

    VOS_MemCpy(&ulValue, (pucData + ulSize - OM_USB_TAIL_LEN), sizeof(VOS_UINT32));

    /* �ж����ݰ�β�ǲ���0x5555aaaa */
    if (OM_USB_FRAME_TAIL != ulValue)
    {
        /* ��¼���� */
        pstCtrlInfo->stPcToUeErrRecord.usDataHeadErr++;
        return VOS_ERR;
    }

    /* �յ���ͨ��Ϣʱ���жϵ�ǰ��·״̬ */
    lRet = VOS_MemCmp(pucData, aucLinkData, ulSize);

    if ((OM_STATE_IDLE == pstCtrlInfo->ulOMSwitchOnOff)
        && (VOS_OK != lRet))
    {
        pstCtrlInfo->stPcToUeErrRecord.usLinkStatusErr++;
        return VOS_ERR;
    }

    /* ����ָ��ͳ��ȸ���*/
    pucData += OM_USB_HEAD_LEN;
    ulSize  -= OM_USB_FRAME_LEN;

    /* ��� */
    return OMRL_AcpuMsgCombine(pstCtrlInfo, pucData, (VOS_UINT16)ulSize);
}



VOS_UINT32 OMRL_RcvChannelData(OMRL_RCV_CHAN_CTRL_INFO_STRU *pstCtrlInfo, VOS_UINT8 *pucData, VOS_UINT32 ulSize)
{
    return OMRL_AcpuHeadProc(pstCtrlInfo, pucData, ulSize);
}


VOS_UINT32 OMRL_RcvCbtChannel(SOCP_DECODER_DST_ENUM_U32 enChanlID,VOS_UINT8 *pucData, VOS_UINT32 ulSize, VOS_UINT8 *pucRBData, VOS_UINT32 ulRBSize)
{
    if ((VOS_NULL_PTR != pucRBData)||(0 != ulRBSize))
    {
        /* ��ӡ���� */
        LogPrint("OMRL_RcvCbtChannel:para error!\n");
    }

    return OMRL_RcvChannelData(&g_stAcpuCbtCtrlInfo, pucData, ulSize);
}
VOS_UINT32 OMRL_RcvCnfChannel(SOCP_DECODER_DST_ENUM_U32 enChanlID,VOS_UINT8 *pucData, VOS_UINT32 ulSize, VOS_UINT8 *pucRBData, VOS_UINT32 ulRBSize)
{
    if ((VOS_NULL_PTR != pucRBData)||(0 != ulRBSize))
    {
        /* ��ӡ���� */
        LogPrint("OMRL_RcvCnfChannel:para error!\n");
    }

    return OMRL_RcvChannelData(&g_stAcpuCnfCtrlInfo, pucData, ulSize);
}
VOS_UINT32 OMRL_AcpuReportEvent(VOS_VOID)
{
    PS_OM_EVENT_IND_STRU    *pstEventInd;
    VOS_UINT32               ulEventLenth;
    VOS_UINT32               ulTemp = 0x5a5a5a5a;
    VOS_UINT32               ulIndex;
    VOS_UINT32               ulSendLen;
    OM_SEND_DEBUG_INFO_STRU  astAcpuDebuginfoTab2[3];
    VOS_UINT_PTR             ulAddr;
    VOS_UINT16               usDebugEventID;

    /* ���A�˵�����Ϣ�� ��С�͵�ַ,�ְ��ϱ���Ϣ */
    OM_SEND_DEBUG_INFO_STRU  astAcpuDbuginfoTab1[] = {
        {sizeof(g_stAcpuCnfCtrlInfo.stPcToUeSucRecord),                 (VOS_UINT8*)&g_stAcpuCnfCtrlInfo.stPcToUeSucRecord},
        {sizeof(g_stAcpuCbtCtrlInfo.stPcToUeSucRecord),                 (VOS_UINT8*)&g_stAcpuCbtCtrlInfo.stPcToUeSucRecord},
        {sizeof(g_stAcpuTxCnfCtrlInfo.stUeToPcSucRecord),               (VOS_UINT8*)&g_stAcpuTxCnfCtrlInfo.stUeToPcSucRecord},
        {sizeof(g_stAcpuTxIndCtrlInfo.stUeToPcSucRecord),               (VOS_UINT8*)&g_stAcpuTxIndCtrlInfo.stUeToPcSucRecord},
        {sizeof(g_stAcpuDebugInfo),                       (VOS_UINT8*)&g_stAcpuDebugInfo      },
        {sizeof(g_stOmSDDebugInfo),                       (VOS_UINT8*)&g_stOmSDDebugInfo      },
        {sizeof(g_stVComDebugInfo),                       (VOS_UINT8*)&g_stVComDebugInfo[0]   },
        {sizeof(g_astAcpuRecordInfo),                     (VOS_UINT8*)&g_astAcpuRecordInfo[0]    },
        {sizeof(g_stAcpuCnfCtrlInfo.ulOMSwitchOnOff),     (VOS_UINT8*)&g_stAcpuCnfCtrlInfo.ulOMSwitchOnOff  },
#ifdef SCM_SNCHECK
        {sizeof(VOS_UINT32),                     (VOS_UINT8*)&g_ulSnErrCount    },
#endif
    };

    astAcpuDebuginfoTab2[0].pucAddr = (VOS_UINT8*)SCM_GetDebugLogInfo();
    astAcpuDebuginfoTab2[0].ulLenth = SCM_GetDebugLogInfoLen();

    astAcpuDebuginfoTab2[1].pucAddr = (VOS_UINT8*)SOCP_GetDebugLogInfo(OM_GET_SOCP_STATUS);
    astAcpuDebuginfoTab2[1].ulLenth = SOCP_GetDebugLogInfoLen(OM_GET_SOCP_STATUS);

    astAcpuDebuginfoTab2[2].pucAddr = (VOS_UINT8*)SOCP_GetDebugLogInfo(OM_GET_SOCP_DEBUG_INFO);
    astAcpuDebuginfoTab2[2].ulLenth = SOCP_GetDebugLogInfoLen(OM_GET_SOCP_DEBUG_INFO);

    /* ---------------��һ������ astAcpuDbuginfoTab1[]  begin----------- */

    ulEventLenth = (sizeof(PS_OM_EVENT_IND_STRU)-(4 * sizeof(VOS_UINT8)));
    for (ulIndex=0; ulIndex<(sizeof(astAcpuDbuginfoTab1)/sizeof(astAcpuDbuginfoTab1[0])); ulIndex ++)
    {
        ulEventLenth += astAcpuDbuginfoTab1[ulIndex].ulLenth;
    }

    /* ���Ϸָ����ų��� */
    ulEventLenth += (ulIndex - 1) * (VOS_UINT32)sizeof(VOS_UINT32);
    pstEventInd = (PS_OM_EVENT_IND_STRU *)VOS_MemAlloc(ACPU_PID_OM, DYNAMIC_MEM_PT, ulEventLenth);
    if (VOS_NULL_PTR == pstEventInd)
    {
        LogPrint("OMRL_AcpuReportEvent:Alloc Memory Failed!\n");
        return VOS_ERR;
    }

    pstEventInd->ulLength   = ulEventLenth;
    pstEventInd->usEventId  = OAM_DEGUG_EVENT_ID1;
    pstEventInd->ulModuleId = ACPU_PID_OM;

    /*lint -e419 -e416 */
    /* pc�ൽue��ʱ��������������Ϣ */
    VOS_MemCpy((VOS_UINT8 *)pstEventInd->aucData,
        (VOS_UINT8*)&g_stAcpuCnfCtrlInfo.stPcToUeSucRecord,
        sizeof(OM_ACPU_PC_UE_SUC_STRU));
     /*lint +e419 +e416 */

    ulEventLenth = 0;
    for (ulIndex=1;ulIndex<(sizeof(astAcpuDbuginfoTab1)/sizeof(astAcpuDbuginfoTab1[0])); ulIndex ++)
    {
        /*lint -e419 -e416 */
        /* ���Ϸָ����� */
        ulEventLenth += astAcpuDbuginfoTab1[ulIndex-1].ulLenth;
        VOS_MemCpy((VOS_UINT8 *)pstEventInd->aucData + ulEventLenth,
                   (VOS_UINT8 *)&ulTemp, sizeof(VOS_UINT32));

        ulEventLenth += (VOS_UINT32)sizeof(VOS_UINT32);
        VOS_MemCpy((VOS_UINT8 *)pstEventInd->aucData + ulEventLenth,
            astAcpuDbuginfoTab1[ulIndex].pucAddr, astAcpuDbuginfoTab1[ulIndex].ulLenth);
         /*lint +e419 +e416 */
    }
    /*lint +e419 +e416 */

    if (VOS_OK != OM_AcpuEvent(pstEventInd))
    {
        PS_LOG(ACPU_PID_OM, VOS_NULL, PS_PRINT_WARNING, "OMRL_AcpuReportEvent: send event fail.");
    }
    /* �ͷ���Ϣ���ݿռ� */
    VOS_MemFree(ACPU_PID_OM, pstEventInd);

    /* ---------------��һ������ astAcpuDbuginfoTab1[]  end----------- */

    /* ---------------astAcpuDbuginfoTab2[] ÿ�����ݽϴ������ͣ�ÿ�������2k----------- */
    /* Event Id */
    usDebugEventID  = OAM_DEGUG_EVENT_ID2;
    /* һ�������� 2k �ռ䣬����ͷ� */
    pstEventInd = (PS_OM_EVENT_IND_STRU *)VOS_MemAlloc(ACPU_PID_OM, DYNAMIC_MEM_PT,
                                          OM_SEND_DEBUG_INFO_MAX_LEN + sizeof(PS_OM_EVENT_IND_STRU)-(4 * sizeof(VOS_UINT8)));
    if (VOS_NULL_PTR == pstEventInd)
    {
        LogPrint("OMRL_AcpuReportEvent:Alloc Memory Failed!\n");
        return VOS_ERR;
    }

    /* ���ݲ�ַ��� */
    for(ulIndex =0; ulIndex<(sizeof(astAcpuDebuginfoTab2)/sizeof(astAcpuDebuginfoTab2[0])); ulIndex++)
    {
        /* �������ݿռ� ���Ϊ 2k*/
        ulSendLen = 0;

        while(ulSendLen < astAcpuDebuginfoTab2[ulIndex].ulLenth)
        {
            if((astAcpuDebuginfoTab2[ulIndex].ulLenth - ulSendLen) > OM_SEND_DEBUG_INFO_MAX_LEN)
            {
                ulEventLenth = OM_SEND_DEBUG_INFO_MAX_LEN;
            }
            else
            {
                ulEventLenth = astAcpuDebuginfoTab2[ulIndex].ulLenth - ulSendLen;
            }

            pstEventInd->ulLength   = ulEventLenth;
            pstEventInd->usEventId  = usDebugEventID++;
            pstEventInd->ulModuleId = ACPU_PID_OM;

            /* make pclint happy  begin */
            ulAddr = (VOS_UINT_PTR)pstEventInd->aucData;
            VOS_MemCpy((VOS_UINT8 *)ulAddr, astAcpuDebuginfoTab2[ulIndex].pucAddr + ulSendLen, ulEventLenth);
            /* make pclint happy  end */

            ulSendLen += ulEventLenth;

            if (VOS_OK != OM_AcpuEvent(pstEventInd))
            {
                PS_LOG(ACPU_PID_OM, VOS_NULL, PS_PRINT_WARNING, "OMRL_AcpuReportEvent: send event fail.");
            }
        }
    }

    /* �ͷ���Ϣ���ݿռ� */
    VOS_MemFree(ACPU_PID_OM, pstEventInd);

    return VOS_OK;
}


VOS_UINT32 OMRL_AcpuSendCbtData(VOS_UINT8  *pucSrc, VOS_UINT16  usSrcLen)
{
    VOS_UINT16                         usHdlcEncLen;
    VOS_UINT32                         ulResult;

    /*���л������*/
    if ( VOS_OK != VOS_SmP(g_ulOmTxCbtSem, OM_PV_TIMEOUT) )
    {
        LogPrint("OMRL_AcpuMsgSplit, Error, OmTxBuffSem VOS_SmP Failed.\n");

        return VOS_ERR;
    }

    /* ��HDLC���� */
    if ( VOS_OK != Om_HdlcEncap(pucSrc,
                                usSrcLen,
                                g_stCbtHdlcEncBuf.pucBuf,
                                (VOS_UINT16)g_stCbtHdlcEncBuf.ulBufSize,
                                &usHdlcEncLen) )
    {
        VOS_SmV(g_ulOmTxCbtSem);

        return VOS_ERR;
    }

    ulResult = CBTPPM_OamCbtPortDataSnd(g_stCbtHdlcEncBuf.pucBuf, g_stCbtHdlcEncBuf.pucRealBuf,usHdlcEncLen);

    VOS_SmV(g_ulOmTxCbtSem);

    return ulResult;
}


VOS_UINT32 OMRL_AcpuReleaseSocpData(OMRL_MNTN_TX_CHAN_CTRL_INFO_STRU *pstCtrlInfo)
{
    VOS_UINT_PTR                        DataAddr;
    VOS_UINT32                          ulDataLen;
    VOS_UINT8                           *pucRealAddr;
    SOCP_CODER_SRC_ENUM_U32             enChanlID;
    OM_BUF_CTRL_STRU                    *pstCtrlBuf = VOS_NULL_PTR;

    enChanlID = pstCtrlInfo->enChanlID;
    pstCtrlBuf = &(pstCtrlInfo->OmBufCtrl);

#if 0

    VOS_BOOL            bLoopFlag = VOS_TRUE;

    /* Ϊ�˲��Գ�֣������Ȱ�һ��һ��RD�ͷ� */
    do
    {
        /* ��SCM����ѷ�����Ϣ�ܳ��� */
        if (VOS_OK != SCM_RlsSrcRDFirst(SOCP_CODER_SRC_GU_OM1, &ulDataAddr, &ulDataLen))
        {
            g_stAcpuUeToPcErrRecord.usRlsRdErr++;

            return VOS_ERR;
        }

        /* �Ѿ�û��RD */
        if ((0 == ulDataAddr) || (0 == ulDataLen))
        {
            break;
        }

        pucRealAddr = (VOS_UINT8*)OM_AddrRealToVirt(&g_stAcpuOmBufCtrl, (VOS_UINT8*)ulDataAddr);

        if (VOS_NULL_PTR == pucRealAddr)
        {
            g_stAcpuUeToPcErrRecord.usRlsAddrErr++;

            return VOS_ERR;
        }

        /* �ͷ�OM�������ڴ� */
        if (VOS_OK != OM_ReleaseTraceMem(&g_stAcpuOmBufCtrl, pucRealAddr, (VOS_INT32)ulDataLen))
        {
            g_stAcpuUeToPcErrRecord.usRlsMemErr++;

            return VOS_ERR;
        }

    }while(bLoopFlag);

#else
    /* ��������ѷ���RD */
    if (VOS_OK != SCM_RlsSrcRDAll(enChanlID, &DataAddr, &ulDataLen))
    {
        pstCtrlInfo->stUeToPcErrRecord.usRlsRdErr++;

        return VOS_ERR;
    }

    /* �Ѿ�û��RD */
    if ((0 == DataAddr) || (0 == ulDataLen))
    {
        return VOS_OK;
    }

    pstCtrlInfo->stUeToPcSucRecord.stRealRlsData.ulDataLen += ulDataLen;
    pstCtrlInfo->stUeToPcSucRecord.stRealRlsData.ulNum++;

    pucRealAddr = (VOS_UINT8*)OM_AddrRealToVirt(pstCtrlBuf, (VOS_UINT8*)DataAddr);

    if (VOS_NULL_PTR == pucRealAddr)
    {
        pstCtrlInfo->stUeToPcErrRecord.usRlsAddrErr++;

        return VOS_ERR;
    }

    /* �����ѷ���RD�ܳ����ͷ��ڴ� */
    if (VOS_OK != OM_ReleaseTraceMem(pstCtrlBuf, pucRealAddr, (VOS_INT32)ulDataLen))
    {
        pstCtrlInfo->stUeToPcErrRecord.usRlsMemErr++;

        g_stAcpuCnfCtrlInfo.ulOMSwitchOnOff = OM_STATE_IDLE;

        return VOS_ERR;
    }
#endif

    return VOS_OK;
}
VOS_UINT32 OMRL_AcpuSendSocpData(OMRL_MNTN_TX_CHAN_CTRL_INFO_STRU *pstCtrlInfo, VOS_UINT8 *pucHead, VOS_UINT32 ulHeadLen,
                            VOS_UINT8 *pucData, VOS_UINT32 ulDataLen)
{
    VOS_UINT8                          *pucMemAddr;
    VOS_INT32                           lUsedBytes;
    VOS_UINT8                          *pucRealMemAddr;
    SOCP_CODER_SRC_ENUM_U32             enChanlID;
    OM_BUF_CTRL_STRU                   *pstCtrlBuf = VOS_NULL_PTR;

    enChanlID = pstCtrlInfo->enChanlID;
    pstCtrlBuf = &(pstCtrlInfo->OmBufCtrl);

    pucMemAddr = (VOS_UINT8*)OM_AllocTraceMem(pstCtrlBuf, (VOS_INT32)(ulHeadLen + ulDataLen + sizeof(VOS_UINT32)));

    if (VOS_NULL_PTR == pucMemAddr)
    {
        pstCtrlInfo->stUeToPcErrRecord.usNoMemErr++;

        return VOS_ERR;
    }

    /* ��������ͷ */
    VOS_MemCpy(pucMemAddr, pucHead, ulHeadLen);

    /* ������������ */
    VOS_MemCpy(pucMemAddr + ulHeadLen, pucData, ulDataLen);

    /* ��д��β */
    *(VOS_UINT32*)(pucMemAddr + ulHeadLen + ulDataLen) = OM_USB_FRAME_TAIL;

    VOS_FlushCpuWriteBuf();

    pucRealMemAddr = (VOS_UINT8*)OM_AddrVirtToReal(pstCtrlBuf, pucMemAddr);

    if (VOS_NULL_PTR == pucRealMemAddr)
    {
        pstCtrlInfo->stUeToPcErrRecord.usSendAddrErr++;

        return VOS_ERR;
    }

    /* ����SCM�ṩ�Ľӿڽ����ݷ��͸�SOCP */
    if (VOS_OK != SCM_SendCoderSrc(enChanlID, pucRealMemAddr, ulHeadLen + ulDataLen + (VOS_UINT32)sizeof(VOS_UINT32)))
    {
        pstCtrlInfo->stUeToPcErrRecord.usSendSocpDataErr++;

        return VOS_ERR;
    }

    pstCtrlInfo->stUeToPcSucRecord.stRealSendData.ulDataLen += (ulHeadLen + ulDataLen + (VOS_UINT32)sizeof(VOS_UINT32));
    pstCtrlInfo->stUeToPcSucRecord.stRealSendData.ulNum++;

    lUsedBytes = OM_TraceMemNBytes(pstCtrlBuf);

    if (pstCtrlInfo->stUeToPcSucRecord.ulMaxUsedBufSize < (VOS_UINT32)lUsedBytes)
    {
        pstCtrlInfo->stUeToPcSucRecord.ulMaxUsedBufSize = (VOS_UINT32)lUsedBytes;
    }

    return VOS_OK;
}



VOS_UINT32 OMRL_AcpuIsBDEnough(OMRL_MNTN_TX_CHAN_CTRL_INFO_STRU *pstCtrlInfo, VOS_UINT8 ucMsgCnt)
{
    VOS_UINT32                          ulFreeBdNum = 0;
    SOCP_CODER_SRC_ENUM_U32             enChanlID;

    enChanlID = pstCtrlInfo->enChanlID;

    /* ��ȡOMԴͨ��ʣ��BD���� */
    if (VOS_OK != SCM_GetBDFreeNum(enChanlID, &ulFreeBdNum))
    {
        pstCtrlInfo->stUeToPcErrRecord.usGetBDErr++;

        return VOS_ERR;
    }

    /* �ж�ʣ��BD�Ƿ�����ְ����� */
    if (ucMsgCnt > ulFreeBdNum)
    {
        pstCtrlInfo->stUeToPcErrRecord.usBDLackErr++;

        return VOS_ERR;
    }

    return VOS_OK;
}

/*****************************************************************************
 �� �� ��  : OMRL_ProcessSendData
 ��������  : ����ά�ɲ����ݷ��͵�SOCPģ��
 �������  : pucMsg    -- ָ�������ݵ�ָ��
             usMsgLen  -- �������ݵĳ���
             ucMsgCnt  -- ���������ݰ�����
 �������  : ��
 �� �� ֵ  : VOS_UINT32

 �޸���ʷ      :
  1.��    ��   : 2009��3��28��
    ��    ��   : ���� 47350
    �޸�����   : �����ɺ���

*****************************************************************************/
VOS_UINT32 OMRL_AcpuProcessSendData(OMRL_MNTN_TX_CHAN_CTRL_INFO_STRU *pstCtrlInfo, VOS_UINT8 *pucMsg, VOS_UINT8 ucMsgCnt, VOS_UINT16 usMsgLen)
{
    VOS_UINT8                           ucCurSegNum = 1; /*��ǰ�����*/
    VOS_UINT_PTR                        ulTempAddress;
    VOS_BOOL                            bIsHead     = VOS_FALSE;
    OM_APP_TRACE_STRU_EX               *pstAppTrace;
    /*ָ��OM�ְ�ͷ���ݵ�ָ��*/
    OM_APP_MSG_SEG_EX_STRU             *pstSegMsgEx = VOS_NULL_PTR;

    /*����ְ��ṹ���ڴ�ռ�*/
    pstSegMsgEx = (OM_APP_MSG_SEG_EX_STRU*)VOS_MemAlloc(WUEPS_PID_OM,
                DYNAMIC_MEM_PT, sizeof(OM_APP_MSG_SEG_EX_STRU) + OM_APP_TRACE_LEN);

    if (VOS_NULL_PTR == pstSegMsgEx)
    {
        LogPrint("OM_UsbFrameInit:VOS_MemAlloc g_pstSegMsgEx Failed!\n");
        return VOS_ERR;
    }

    pstSegMsgEx->stSocpHdr.ucCPUID   = OM_TOOL_ACPU_ID;
    pstSegMsgEx->ulFrameHead         = OM_USB_FRAME_HEAD;

    pstAppTrace = (OM_APP_TRACE_STRU_EX*)pucMsg;

    ulTempAddress = (VOS_UINT_PTR)pucMsg;

    if ((OM_TRACE_FUNC == pstAppTrace->stAppHeader.ucFuncType)
        &&(OM_APP_TRACE_IND == pstAppTrace->usPrimId))
    {
        ulTempAddress = *(VOS_UINT_PTR*)(pstAppTrace->aucPara);
        VOS_MemCpy(pstSegMsgEx + 1, pucMsg, OM_APP_TRACE_LEN);
        bIsHead = VOS_TRUE;
    }

    /* ���SOCPͷ�е�RTCֵ */
    pstSegMsgEx->stSocpHdr.ulRTC = (VOS_UINT32)DRV_GET_RTC_VALUE();
    pstSegMsgEx->stMsgSeg.ucSegNum = ucMsgCnt;
    pstSegMsgEx->stMsgSeg.usSegLen = OM_APP_SPLIT_MSG_LEN;

    /* �������ְ���С�����ݣ��������ְ���С�������ݷ��͵Ĵ��� */
    for (ucCurSegNum = 1; ucCurSegNum < ucMsgCnt; ucCurSegNum++)
    {
        pstSegMsgEx->stMsgSeg.ucSegSn = ucCurSegNum;

        if (VOS_TRUE == bIsHead)
        {
            /* ��Ϣͷǰ���ϳ����Ų����뻷�λ�����*/
            if (VOS_OK != OMRL_AcpuSendSocpData(pstCtrlInfo, (VOS_UINT8 *)pstSegMsgEx,
                (sizeof(OM_APP_MSG_SEG_EX_STRU) + OM_APP_TRACE_LEN),
                (VOS_UINT8*)ulTempAddress, OM_APP_MSG_SEGMENT_LEN - OM_APP_TRACE_LEN))
            {
                VOS_MemFree(WUEPS_PID_OM, pstSegMsgEx);

                return VOS_ERR;
            }

            bIsHead       =  VOS_FALSE;
            ulTempAddress += (OM_APP_MSG_SEGMENT_LEN - OM_APP_TRACE_LEN);
        }
        else
        {
            /* ��Ϣͷǰ���ϳ����Ų����뻷�λ�����*/
            if (VOS_OK != OMRL_AcpuSendSocpData(pstCtrlInfo, (VOS_UINT8 *)pstSegMsgEx,
                 sizeof(OM_APP_MSG_SEG_EX_STRU),
                (VOS_UINT8*)ulTempAddress, OM_APP_MSG_SEGMENT_LEN))
            {
                VOS_MemFree(WUEPS_PID_OM, pstSegMsgEx);

                return VOS_ERR;
            }

            ulTempAddress += OM_APP_MSG_SEGMENT_LEN;
        }

        /* ����ʣ�����ݰ���С */
        usMsgLen -= OM_APP_MSG_SEGMENT_LEN;
    }

    /*������Ϣͷ����*/
    pstSegMsgEx->stMsgSeg.usSegLen = (VOS_UINT16)(usMsgLen + OM_APP_MSG_SEGMENT_HEADER_LENGTH);
    pstSegMsgEx->stMsgSeg.ucSegSn  = ucCurSegNum;

    /*Ϊ�˱���Ringbuffer�д�ŵ����ݰ�4�ֽڶ���*/
    usMsgLen = (usMsgLen + 3U) & ~0x03U;

    if (VOS_TRUE == bIsHead)
    {
        /* ��Ϣͷǰ���ϳ��Ȳ����뻷�λ����� */
        if ( VOS_OK != OMRL_AcpuSendSocpData(pstCtrlInfo, (VOS_UINT8 *)pstSegMsgEx,
                            (sizeof(OM_APP_MSG_SEG_EX_STRU) + OM_APP_TRACE_LEN),
                            (VOS_UINT8*)ulTempAddress,
                            usMsgLen - OM_APP_TRACE_LEN))
        {
            VOS_MemFree(WUEPS_PID_OM, pstSegMsgEx);

            return VOS_ERR;
        }
    }
    else
    {
        /* ��Ϣͷǰ���ϳ��Ȳ����뻷�λ����� */
        if ( VOS_OK != OMRL_AcpuSendSocpData(pstCtrlInfo, (VOS_UINT8 *)pstSegMsgEx,
                                        sizeof(OM_APP_MSG_SEG_EX_STRU),
                                        (VOS_UINT8*)ulTempAddress,
                                        usMsgLen))
        {
            VOS_MemFree(WUEPS_PID_OM, pstSegMsgEx);

            return VOS_ERR;
        }
    }

    VOS_MemFree(WUEPS_PID_OM, pstSegMsgEx);

    return VOS_OK;
}


/*****************************************************************************
 �� �� ��  : OMRL_AcpuMsgSplit
 ��������  : ����USB�Ͻ���ͨ��ʱ��ʹ�õ���չ�ְ�����
 �������  : pucMsg    -- ָ�������ݵ�ָ��
             usMsgLen  -- �������ݵĳ���
 �������  : ��
 �� �� ֵ  : VOS_UINT32

 �޸���ʷ      :
  1.��    ��   : 2009��3��28��
    ��    ��   : ���� 47350
    �޸�����   : �����ɺ���

*****************************************************************************/
VOS_UINT32 OMRL_AcpuMsgSplit(OMRL_MNTN_TX_CHAN_CTRL_INFO_STRU *pstCtrlInfo, VOS_UINT8 *pucMsg, VOS_UINT16 usMsgLen)
{
    VOS_UINT8                           ucMsgCnt    = 1; /*�ֶε�����*/
    VOS_UINT32                          ulRet;

    /*���л������*/
    if ( VOS_OK != VOS_SmP(pstCtrlInfo->OmTxBuffSem, OM_PV_TIMEOUT) )
    {
        LogPrint("OMRL_AcpuMsgSplit, Error, OmTxBuffSem VOS_SmP Failed.\n");

        pstCtrlInfo->stUeToPcErrRecord.usSemErr++;
        return VOS_ERR;
    }

    /* �ͷ��ѷ��������ڴ� */
    if (VOS_OK != OMRL_AcpuReleaseSocpData(pstCtrlInfo))
    {
        VOS_SmV(pstCtrlInfo->OmTxBuffSem);
        return VOS_ERR;
    }

    /* ����ְ����� */
    ucMsgCnt = (VOS_UINT8)(((usMsgLen + OM_APP_MSG_SEGMENT_LEN) - 1)/OM_APP_MSG_SEGMENT_LEN);

    /* ��ȡOMԴͨ��ʣ��BD���� */
    if (VOS_OK != OMRL_AcpuIsBDEnough(pstCtrlInfo, ucMsgCnt))
    {
        VOS_SmV(pstCtrlInfo->OmTxBuffSem);
        return VOS_ERR;
    }

    ulRet = OMRL_AcpuProcessSendData(pstCtrlInfo, pucMsg, ucMsgCnt, usMsgLen);

    VOS_SmV(pstCtrlInfo->OmTxBuffSem);

    return ulRet;
}
VOS_UINT32 OMRL_AcpuUsbFrameInit(VOS_VOID)
{
    VOS_UINT_PTR    ulRealAddr;

    VOS_MemSet(&g_stAcpuTxCnfCtrlInfo, 0, sizeof(g_stAcpuTxCnfCtrlInfo));

    VOS_MemSet(&g_stAcpuTxIndCtrlInfo, 0, sizeof(g_stAcpuTxIndCtrlInfo));

    g_stAcpuTxCnfCtrlInfo.enChanlID = SOCP_CODER_SRC_GU_CNF1;

    g_stAcpuTxCnfCtrlInfo.enChannel = OM_LOGIC_CHANNEL_CNF;

    g_stAcpuTxIndCtrlInfo.enChanlID = SOCP_CODER_SRC_GU_IND1;

    g_stAcpuTxIndCtrlInfo.enChannel = OM_LOGIC_CHANNEL_IND;

    /* ����OM CNF���ͻ����� */
    if (VOS_OK != OM_CreateTraceBuffer(&(g_stAcpuTxCnfCtrlInfo.OmBufCtrl), OM_CNF_BUFFER_SIZE))
    {
        LogPrint("OMRL_AcpuUsbFrameInit:OM_CreateTraceBuffer Failed!\n");

        return VOS_ERR;
    }

    /* ����OM IND���ͻ����� */
    if (VOS_OK != OM_CreateTraceBuffer(&(g_stAcpuTxIndCtrlInfo.OmBufCtrl), OM_UPLINK_BUFFER_SIZE))
    {
        LogPrint("OMRL_AcpuUsbFrameInit:OM_CreateTraceBuffer Failed!\n");

        return VOS_ERR;
    }

    /* ���������ź��� */
    if (VOS_OK != VOS_SmMCreate("TXCNF", VOS_SEMA4_PRIOR | VOS_SEMA4_INVERSION_SAFE, &g_stAcpuTxCnfCtrlInfo.OmTxBuffSem))
    {
        LogPrint("OMRL_AcpuUsbFrameInit: Error, VOS_SmMCreate Fail\n");
        return VOS_ERR;
    }

    /* ���������ź��� */
    if (VOS_OK != VOS_SmMCreate("TXIND", VOS_SEMA4_PRIOR | VOS_SEMA4_INVERSION_SAFE, &g_stAcpuTxIndCtrlInfo.OmTxBuffSem))
    {
        LogPrint("OMRL_AcpuUsbFrameInit: Error, VOS_SmMCreate Fail\n");
        return VOS_ERR;
    }

    /* ����CBTͨ��HDLC����ʹ�õ�uncached memory */
    VOS_MemSet(&g_stCbtHdlcEncBuf, 0, sizeof(g_stCbtHdlcEncBuf));

    g_stCbtHdlcEncBuf.pucBuf = (VOS_UINT8 *)VOS_UnCacheMemAlloc(2 * OM_HDLC_BUF_MAX_LEN, &ulRealAddr);
    if (VOS_NULL_PTR == g_stCbtHdlcEncBuf.pucBuf)
    {
        LogPrint("OMRL_AcpuUsbFrameInit: VOS_UnCacheMemAlloc failed \n");
        return VOS_ERR;
    }

    /* ����bufʵ��ַ */
    g_stCbtHdlcEncBuf.pucRealBuf = (VOS_UINT8 *)ulRealAddr;
    g_stCbtHdlcEncBuf.ulBufSize  = 2 * OM_HDLC_BUF_MAX_LEN;

    return VOS_OK;
}


VOS_VOID OmAcpuInitVar(VOS_VOID)
{
    VOS_MemSet(&g_stAcpuCnfCtrlInfo, 0, sizeof(g_stAcpuCnfCtrlInfo));

    /* �װ����Ϊ1 */
    g_stAcpuCnfCtrlInfo.stMsgCombineInfo.ucExpectedSegSn = 1;
    g_stAcpuCnfCtrlInfo.ulOMSwitchOnOff = OM_STATE_IDLE;
    g_stAcpuCnfCtrlInfo.enChannel = OM_LOGIC_CHANNEL_CNF;

    VOS_MemSet(&g_stAcpuCbtCtrlInfo, 0, sizeof(g_stAcpuCbtCtrlInfo));

    /* �װ����Ϊ1 */
    g_stAcpuCbtCtrlInfo.stMsgCombineInfo.ucExpectedSegSn = 1;
    g_stAcpuCbtCtrlInfo.ulOMSwitchOnOff = OM_STATE_IDLE;
    g_stAcpuCbtCtrlInfo.enChannel = OM_LOGIC_CHANNEL_CBT;

    return;
}


VOS_UINT32 OMRL_AcpuInit(VOS_VOID)
{
    OmAcpuInitVar();

    if (VOS_OK != OMRL_AcpuUsbFrameInit())
    {
        LogPrint("OMRL_AcpuInit:OMRL_AcpuUsbFrameInit Fail.\n");
        return VOS_ERR;
    }

    return VOS_OK;
}


VOS_VOID OmAcpuSendIndSucShow(VOS_VOID)
{
    OM_UE_PC_SUC_STRU                  *pstUeToPcSucRecord = &(g_stAcpuTxIndCtrlInfo.stUeToPcSucRecord);

    vos_printf("\r\n *****IND channel success info show *******\r\n");

    vos_printf("Total Data Send by Ps: num:%6u, len:%6u\n",
                pstUeToPcSucRecord->stTotalData.ulNum, pstUeToPcSucRecord->stTotalData.ulDataLen);

    vos_printf("Real Data send to Socp: num:%6u, len:%6u\n",
                pstUeToPcSucRecord->stRealSendData.ulNum, pstUeToPcSucRecord->stRealSendData.ulDataLen);

    vos_printf("Real Data release by Socp: num:%6u, len:%6u\n",
                pstUeToPcSucRecord->stRealRlsData.ulNum, pstUeToPcSucRecord->stRealRlsData.ulDataLen);

    vos_printf("Max Used Buffer Size: size:%6u\n", pstUeToPcSucRecord->ulMaxUsedBufSize);

    return;
}


VOS_VOID OmAcpuSendCnfSucShow(VOS_VOID)
{
    OM_UE_PC_SUC_STRU                  *pstUeToPcSucRecord = &(g_stAcpuTxCnfCtrlInfo.stUeToPcSucRecord);

    vos_printf("\r\n *****CNF channel success info show *******\r\n");

    vos_printf("Total Data Send by Ps: num:%6u, len:%6u\n",
                pstUeToPcSucRecord->stTotalData.ulNum, pstUeToPcSucRecord->stTotalData.ulDataLen);

    vos_printf("Real Data send to Socp: num:%6u, len:%6u\n",
                pstUeToPcSucRecord->stRealSendData.ulNum, pstUeToPcSucRecord->stRealSendData.ulDataLen);

    vos_printf("Real Data release by Socp: num:%6u, len:%6u\n",
                pstUeToPcSucRecord->stRealRlsData.ulNum, pstUeToPcSucRecord->stRealRlsData.ulDataLen);

    vos_printf("Max Used Buffer Size: size:%6u\n", pstUeToPcSucRecord->ulMaxUsedBufSize);

    return;
}


VOS_VOID OmAcpuSendIndErrShow(VOS_VOID)
{
    OM_UE_PC_FAIL_STRU                  *pstUeToPcErrRecord = &(g_stAcpuTxIndCtrlInfo.stUeToPcErrRecord);

    vos_printf("\r\n *****IND channel error info show *******\r\n");

    vos_printf("Interrput Lost Data: num:%6u, len:%6u\n",
                pstUeToPcErrRecord->stIntData.ulNum, pstUeToPcErrRecord->stIntData.ulDataLen);

    vos_printf("Total Lost Data: num:%6u, len:%6u\n",
                pstUeToPcErrRecord->stLostData.ulNum, pstUeToPcErrRecord->stLostData.ulDataLen);

    vos_printf("Link Err Data: num:%6u, len:%6u\n",
                pstUeToPcErrRecord->stLinkErrData.ulNum, pstUeToPcErrRecord->stLinkErrData.ulDataLen);

    vos_printf("Para Err Data: num:%6u, len:%6u\n",
                pstUeToPcErrRecord->stParaErrData.ulNum, pstUeToPcErrRecord->stParaErrData.ulDataLen);

    vos_printf("num of sem take err: num:%6u\n", pstUeToPcErrRecord->usSemErr);

    vos_printf("num of release RD err: num:%6u\n", pstUeToPcErrRecord->usRlsRdErr);

    vos_printf("num of release Mem err: num:%6u\n", pstUeToPcErrRecord->usRlsMemErr);

    vos_printf("num of BD Lack err: num:%6u\n", pstUeToPcErrRecord->usBDLackErr);

    vos_printf("num of get BD err: num:%6u\n", pstUeToPcErrRecord->usGetBDErr);

    vos_printf("num of No Mem err: num:%6u\n", pstUeToPcErrRecord->usNoMemErr);

    vos_printf("num of Send Socp Data err: num:%6u\n", pstUeToPcErrRecord->usSendSocpDataErr);

    vos_printf("num of transfer sending addr err: num:%6u\n", pstUeToPcErrRecord->usSendAddrErr);

    vos_printf("num of transfer Release addr err: num:%6u\n", pstUeToPcErrRecord->usRlsAddrErr);

    return;
}


VOS_VOID OmAcpuSendCnfErrShow(VOS_VOID)
{
    OM_UE_PC_FAIL_STRU                  *pstUeToPcErrRecord = &(g_stAcpuTxCnfCtrlInfo.stUeToPcErrRecord);

    vos_printf("\r\n *****CNF channel error info show *******\r\n");

    vos_printf("Interrput Lost Data: num:%6u, len:%6u\n",
                pstUeToPcErrRecord->stIntData.ulNum, pstUeToPcErrRecord->stIntData.ulDataLen);

    vos_printf("Total Lost Data: num:%6u, len:%6u\n",
                pstUeToPcErrRecord->stLostData.ulNum, pstUeToPcErrRecord->stLostData.ulDataLen);

    vos_printf("Link Err Data: num:%6u, len:%6u\n",
                pstUeToPcErrRecord->stLinkErrData.ulNum, pstUeToPcErrRecord->stLinkErrData.ulDataLen);

    vos_printf("Para Err Data: num:%6u, len:%6u\n",
                pstUeToPcErrRecord->stParaErrData.ulNum, pstUeToPcErrRecord->stParaErrData.ulDataLen);

    vos_printf("num of sem take err: num:%6u\n", pstUeToPcErrRecord->usSemErr);

    vos_printf("num of release RD err: num:%6u\n", pstUeToPcErrRecord->usRlsRdErr);

    vos_printf("num of release Mem err: num:%6u\n", pstUeToPcErrRecord->usRlsMemErr);

    vos_printf("num of BD Lack err: num:%6u\n", pstUeToPcErrRecord->usBDLackErr);

    vos_printf("num of get BD err: num:%6u\n", pstUeToPcErrRecord->usGetBDErr);

    vos_printf("num of No Mem err: num:%6u\n", pstUeToPcErrRecord->usNoMemErr);

    vos_printf("num of Send Socp Data err: num:%6u\n", pstUeToPcErrRecord->usSendSocpDataErr);

    vos_printf("num of transfer sending addr err: num:%6u\n", pstUeToPcErrRecord->usSendAddrErr);

    vos_printf("num of transfer Release addr err: num:%6u\n", pstUeToPcErrRecord->usRlsAddrErr);

    return;
}


VOS_VOID OmAcpuRcvCbtSucShow(VOS_VOID)
{
    OM_ACPU_PC_UE_SUC_STRU              *pstPcToUeSucRecord = &(g_stAcpuCbtCtrlInfo.stPcToUeSucRecord);

    vos_printf("\r\n *****CBT receive channel success info show *******\r\n");

    vos_printf("Total Data Recv From PC: num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stTotalData.ulNum, pstPcToUeSucRecord->stTotalData.ulDataLen);

    vos_printf("Data after Hdlc decap:   num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stHdlcDecapData.ulNum, pstPcToUeSucRecord->stHdlcDecapData.ulDataLen);

    vos_printf("Link Data Recv From PC: num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stLinkData.ulNum, pstPcToUeSucRecord->stLinkData.ulDataLen);

    vos_printf("CCPU Data Recv From PC: num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stCcpuData.ulNum, pstPcToUeSucRecord->stCcpuData.ulDataLen);

    vos_printf("ACPU Data Recv From PC: num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stAcpuData.ulNum, pstPcToUeSucRecord->stAcpuData.ulDataLen);

    vos_printf("HIFI Data Recv From PC: num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stHifiData.ulNum, pstPcToUeSucRecord->stHifiData.ulDataLen);

    vos_printf("MCU Data Recv From PC: num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stMcuData.ulNum, pstPcToUeSucRecord->stMcuData.ulDataLen);

    vos_printf("Data Send by ICC: num:%6u, len:%6u, slice:%6u\n",
                pstPcToUeSucRecord->stICCSendSUCInfo.ulICCOmSendMsgNum,
                pstPcToUeSucRecord->stICCSendSUCInfo.ulICCOmSendLen,
                pstPcToUeSucRecord->stICCSendSUCInfo.ulICCOmSendSlice);

    vos_printf("Rls Data Recv From PC: num:%6u, slice:%6u\n",
                pstPcToUeSucRecord->stRlsData.ulNum, pstPcToUeSucRecord->stRlsData.ulDataLen);

    return;
}


VOS_VOID OmAcpuRcvCnfSucShow(VOS_VOID)
{
    OM_ACPU_PC_UE_SUC_STRU              *pstPcToUeSucRecord = &(g_stAcpuCnfCtrlInfo.stPcToUeSucRecord);

    vos_printf("\r\n *****IND channel success info show *******\r\n");

    vos_printf("Total Data Recv From PC: num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stTotalData.ulNum, pstPcToUeSucRecord->stTotalData.ulDataLen);

    vos_printf("Data after Hdlc decap:   num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stHdlcDecapData.ulNum, pstPcToUeSucRecord->stHdlcDecapData.ulDataLen);

    vos_printf("Link Data Recv From PC: num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stLinkData.ulNum, pstPcToUeSucRecord->stLinkData.ulDataLen);

    vos_printf("CCPU Data Recv From PC: num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stCcpuData.ulNum, pstPcToUeSucRecord->stCcpuData.ulDataLen);

    vos_printf("ACPU Data Recv From PC: num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stAcpuData.ulNum, pstPcToUeSucRecord->stAcpuData.ulDataLen);

    vos_printf("HIFI Data Recv From PC: num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stHifiData.ulNum, pstPcToUeSucRecord->stHifiData.ulDataLen);

    vos_printf("MCU Data Recv From PC: num:%6u, len:%6u\n",
                pstPcToUeSucRecord->stMcuData.ulNum, pstPcToUeSucRecord->stMcuData.ulDataLen);

    vos_printf("Data Send by ICC: num:%6u, len:%6u, slice:%6u\n",
                pstPcToUeSucRecord->stICCSendSUCInfo.ulICCOmSendMsgNum,
                pstPcToUeSucRecord->stICCSendSUCInfo.ulICCOmSendLen,
                pstPcToUeSucRecord->stICCSendSUCInfo.ulICCOmSendSlice);

    vos_printf("Rls Data Recv From PC: num:%6u, slice:%6u\n",
                pstPcToUeSucRecord->stRlsData.ulNum, pstPcToUeSucRecord->stRlsData.ulDataLen);

    return;
}


VOS_VOID OmAcpuRcvCbtErrShow(VOS_VOID)
{
    OM_ACPU_PC_UE_FAIL_STRU             *pstPcToUeErrRecord = &(g_stAcpuCbtCtrlInfo.stPcToUeErrRecord);

    vos_printf("\r\n *****CBT receive channel error info show *******\r\n");

    vos_printf("Total Lost Data Recv From PC: num:%6u, len:%6u\n",
            pstPcToUeErrRecord->stLostData.ulNum, pstPcToUeErrRecord->stLostData.ulDataLen);

    vos_printf("Number of Frame Decap Err: num:%6u\n", pstPcToUeErrRecord->ulFrameDecapErr);

    vos_printf("Number of Packet Len Err: num:%6u\n", pstPcToUeErrRecord->usPacketLenErr);

    vos_printf("Number of Too Short Err: num:%6u\n", pstPcToUeErrRecord->usLenTooShortErr);

    vos_printf("Number of Seg Len Err: num:%6u\n", pstPcToUeErrRecord->usSegLenErr);

    vos_printf("Number of Seg Num Err: num:%6u\n", pstPcToUeErrRecord->usSegNumErr);

    vos_printf("Number of DataType Err: num:%6u\n", pstPcToUeErrRecord->usDatatypeErr);

    vos_printf("Number of Too Long Err: num:%6u\n", pstPcToUeErrRecord->usMsgTooLongErr);

    vos_printf("Number of Cpu Id Err: num:%6u\n", pstPcToUeErrRecord->usCpuIdErr);

    vos_printf("Number of No Mem Err: num:%6u\n", pstPcToUeErrRecord->usNoMemErr);

    vos_printf("Number of Data Head Err: num:%6u\n", pstPcToUeErrRecord->usDataHeadErr);

    vos_printf("Number of Msg Len Err: num:%6u\n", pstPcToUeErrRecord->usMsgLenErr);

    vos_printf("Number of Msg Sn Err: num:%6u\n", pstPcToUeErrRecord->usMsgSnErr);

    vos_printf("Number of Packet Lost Err: num:%6u\n", pstPcToUeErrRecord->usPacketLostErr);

    vos_printf("Number of Link Data Len Err: num:%6u\n", pstPcToUeErrRecord->usLinkDataLenErr);

    vos_printf("Number of Alloc msg Mem: num: %6u\n",pstPcToUeErrRecord->usAllocMsg);

    vos_printf("Number of Lost Msg Because Switch Is IDLE: num: %6u\n",pstPcToUeErrRecord->usLinkStatusErr);

    vos_printf("Data Send err by ICC: num:%6u, len:%6u, slice:%6u\n",
            pstPcToUeErrRecord->stICCSendFailInfo.ulICCOmSendErrNum,
            pstPcToUeErrRecord->stICCSendFailInfo.ulICCOmSendErrLen,
            pstPcToUeErrRecord->stICCSendFailInfo.ulICCOmSendErrSlice);

    return;
}


VOS_VOID OmAcpuRcvCnfErrShow(VOS_VOID)
{
    OM_ACPU_PC_UE_FAIL_STRU             *pstPcToUeErrRecord = &(g_stAcpuCnfCtrlInfo.stPcToUeErrRecord);

    vos_printf("\r\n *****CNF receive channel error info show *******\r\n");

    vos_printf("Total Lost Data Recv From PC: num:%6u, len:%6u\n",
            pstPcToUeErrRecord->stLostData.ulNum, pstPcToUeErrRecord->stLostData.ulDataLen);

    vos_printf("Number of Frame Decap Err: num:%6u\n", pstPcToUeErrRecord->ulFrameDecapErr);

    vos_printf("Number of Packet Len Err: num:%6u\n", pstPcToUeErrRecord->usPacketLenErr);

    vos_printf("Number of Too Short Err: num:%6u\n", pstPcToUeErrRecord->usLenTooShortErr);

    vos_printf("Number of Seg Len Err: num:%6u\n", pstPcToUeErrRecord->usSegLenErr);

    vos_printf("Number of Seg Num Err: num:%6u\n", pstPcToUeErrRecord->usSegNumErr);

    vos_printf("Number of DataType Err: num:%6u\n", pstPcToUeErrRecord->usDatatypeErr);

    vos_printf("Number of Too Long Err: num:%6u\n", pstPcToUeErrRecord->usMsgTooLongErr);

    vos_printf("Number of Cpu Id Err: num:%6u\n", pstPcToUeErrRecord->usCpuIdErr);

    vos_printf("Number of No Mem Err: num:%6u\n", pstPcToUeErrRecord->usNoMemErr);

    vos_printf("Number of Data Head Err: num:%6u\n", pstPcToUeErrRecord->usDataHeadErr);

    vos_printf("Number of Msg Len Err: num:%6u\n", pstPcToUeErrRecord->usMsgLenErr);

    vos_printf("Number of Msg Sn Err: num:%6u\n", pstPcToUeErrRecord->usMsgSnErr);

    vos_printf("Number of Packet Lost Err: num:%6u\n", pstPcToUeErrRecord->usPacketLostErr);

    vos_printf("Number of Link Data Len Err: num:%6u\n", pstPcToUeErrRecord->usLinkDataLenErr);

    vos_printf("Number of Alloc msg Mem: num: %6u\n",pstPcToUeErrRecord->usAllocMsg);

    vos_printf("Number of Lost Msg Because Switch Is IDLE: num: %6u\n",pstPcToUeErrRecord->usLinkStatusErr);

    vos_printf("Data Send err by ICC: num:%6u, len:%6u, slice:%6u\n",
            pstPcToUeErrRecord->stICCSendFailInfo.ulICCOmSendErrNum,
            pstPcToUeErrRecord->stICCSendFailInfo.ulICCOmSendErrLen,
            pstPcToUeErrRecord->stICCSendFailInfo.ulICCOmSendErrSlice);

    return;
}


VOS_VOID OmOpenLog(VOS_UINT32 ulFlag)
{
    g_ulOmAcpuDbgFlag = ulFlag;

    return;
}



VOS_VOID OmAcpuRate(VOS_VOID)
{
    VOS_UINT32            ulCurrTick;                    /*��¼����ͳ��ʱ��*/
    VOS_UINT32            ulDeltaTick;                   /*��¼ͳ��ʱ��*/
    VOS_UINT32            ulDelData;                     /*��¼ͳ�����ݴ�С*/
    VOS_UINT32            ulBandWidth;                   /*��¼ͳ�����ݴ�С*/

    vos_printf("OmAcpuRate Show(ALL Data To UDI):\n");

    ulCurrTick = VOS_GetTick();

    if (ulCurrTick < g_ulAcpuLastTick)
    {
        ulDeltaTick = (0xFFFFFFFF - g_ulAcpuLastTick) + ulCurrTick;
    }
    else
    {
        ulDeltaTick = ulCurrTick - g_ulAcpuLastTick;
    }

    ulDelData      = g_stAcpuDebugInfo.stCnfDebugInfo.ulUSBSendLen + g_stAcpuDebugInfo.stIndDebugInfo.ulUSBSendLen - g_ulAcpuLastByteCnt;
    g_ulAcpuLastByteCnt  = g_stAcpuDebugInfo.stCnfDebugInfo.ulUSBSendLen + g_stAcpuDebugInfo.stIndDebugInfo.ulUSBSendLen;

    ulBandWidth    = (ulDelData/ulDeltaTick)/10;

    /*ͳ��OM��ulDeltaTickʱ��(��λ10ms)�ڷ�����ulTotalLen�ֽ�����*/
    vos_printf("OmAcpuRate,Tick <%u> Byte <%u>, BandWidth <%u>.\n",
                 ulDeltaTick, ulDelData, ulBandWidth);

    g_ulAcpuLastTick = ulCurrTick;

    vos_printf("OmAcpuRate Show End\n");
}


VOS_VOID OmAcpuShowCombineInfo(VOS_VOID)
{
    vos_printf("cnf channel pstWholeMsg:        0x%p\r\n",g_stAcpuCnfCtrlInfo.stMsgCombineInfo.pstWholeMsg);
    vos_printf("cnf channel ucExpectedSegSn:    0x%x\r\n",g_stAcpuCnfCtrlInfo.stMsgCombineInfo.ucExpectedSegSn);
    vos_printf("cnf channel ucTotalSegNum:      0x%x\r\n",g_stAcpuCnfCtrlInfo.stMsgCombineInfo.ucTotalSegNum);
    vos_printf("cnf channel ulMoveLen:          0x%x\r\n",g_stAcpuCnfCtrlInfo.stMsgCombineInfo.ulMoveLen);
    vos_printf("cnf channel ulTotalMsgLen:      0x%x\r\n",g_stAcpuCnfCtrlInfo.stMsgCombineInfo.ulTotalMsgLen);
    vos_printf("cbt channel pstWholeMsg:        0x%p\r\n",g_stAcpuCbtCtrlInfo.stMsgCombineInfo.pstWholeMsg);
    vos_printf("cbt channel ucExpectedSegSn:    0x%x\r\n",g_stAcpuCbtCtrlInfo.stMsgCombineInfo.ucExpectedSegSn);
    vos_printf("cbt channel ucTotalSegNum:      0x%x\r\n",g_stAcpuCbtCtrlInfo.stMsgCombineInfo.ucTotalSegNum);
    vos_printf("cbt channel ulMoveLen:          0x%x\r\n",g_stAcpuCbtCtrlInfo.stMsgCombineInfo.ulMoveLen);
    vos_printf("cbt channel ulTotalMsgLen:      0x%x\r\n",g_stAcpuCbtCtrlInfo.stMsgCombineInfo.ulTotalMsgLen);
}

#endif //(FEATURE_OFF == FEATURE_MERGE_OM_CHAN)
#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

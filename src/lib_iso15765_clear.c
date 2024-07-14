/// \file           : lib_iso15765_clear.c
/// \author         : Dongsheng.Yang3
/// \brief          :
/// \date           : 2024/7/13


#include "lib_iso15765.h"

/*
 * Convert partially the CANBus Frame from the PCI
 * // 参考 15765-2 Table 9
 */
n_rslt n_pci_pack(addr_md mode, n_pdu_t* n_pdu, const uint8_t* dt)
{
  n_rslt result = N_ERROR;

  if (n_pdu != NULL && dt != NULL)
  {
    uint8_t offs = (mode & 0x01); // 不了解 寻址模式，这里为啥要偏移1个字节
                                  // 参考 15765-2 Table 9
    switch (n_pdu->n_pci.pt)
    {
    case N_PCI_T_SF:
      if (n_pdu->n_pci.dl <= (uint16_t)(7 - offs))
      {
        n_pdu->dt[0 + offs] = (n_pdu->n_pci.pt) << 4
                              | (uint8_t)(n_pdu->n_pci.dl & 0x0F); // 02 10 01 中的 02；第一个字节的 后4bits是 SF_DL;
      }
      else
      {
        n_pdu->dt[0 + offs] = 0 + ((n_pdu->n_pci.pt) << 4); // 第一个字节是 00
        n_pdu->dt[1 + offs] = (uint8_t)n_pdu->n_pci.dl; // 第二个字节是 SF_DL
      }
      result = N_OK;
      break;
    case N_PCI_T_CF:
      n_pdu->dt[0 + offs] = (n_pdu->n_pci.pt) << 4
                            | n_pdu->n_pci.sn; // 前半字节表示 CF， 后半字节表示 SN
      result = N_OK;
      break;
    case N_PCI_T_FF:
      n_pdu->dt[0 + offs] = (n_pdu->n_pci.pt) << 4
                            | (n_pdu->n_pci.dl & 0x0F00) >> 8; // 第一个字节的高半字节表示 FF,低半字节表示 FF_DL
      n_pdu->dt[1 + offs] = n_pdu->n_pci.dl & 0x00FF; // 第二个字节表示 FF_DL
      result = N_OK;
      break;
    case N_PCI_T_FC:
      n_pdu->dt[0 + offs] = (n_pdu->n_pci.pt) << 4
                            | n_pdu->n_pci.fs;// 第一个字节的高半字节表示 FC,低半字节表示 FS（流控状态）
      n_pdu->dt[1 + offs] = n_pdu->n_pci.bs;// 第二个字节表示 BS（对端发送CF个数）
      n_pdu->dt[2 + offs] = n_pdu->n_pci.st;// 第三个字节表示 STmin（对端发送CF之间的间隔时间）
      result = N_OK;
      break;
    default:
      result = N_ERROR;
      break;
    }
  }
  return result;
}

/*
 * Convert the PCI from the CANBus Frame
 * // 参考 15765-2 Table 9
 */
 n_rslt n_pci_unpack(addr_md mode, n_pdu_t* n_pdu, uint8_t dlc, uint8_t* dt)
{
  n_rslt result = N_ERROR;

  if ((n_pdu != NULL) && (dt != NULL))
  {
    uint8_t offs = (uint8_t)(mode & 0x01U); // Make use and purpose of 'offs' explicit
    // 数据第一个字节 SF FF FC ;
    // Document the operation's intent clearly to avoid being seen as 'dead code'
    n_pdu->n_pci.pt = (uint8_t)((dt[offs] & 0xF0U) >> 4U);

    switch (n_pdu->n_pci.pt)
    {
    case N_PCI_T_SF:
      // Conditional operation based on 'dlc', not dead code
      // ?? 这里为什么不是赋值 n_pdu.sz ? 而是 n_pdu->n_pci.dl
      n_pdu->n_pci.dl = (dlc <= 8U) ? (uint8_t)(dt[offs] & 0x0FU) : dt[1U + offs];
      result = N_OK;
      break;

    case N_PCI_T_CF:
      // Direct assignment, the operation depends on 'dt' content
      n_pdu->n_pci.sn = (uint8_t)(dt[offs] & 0x0FU);
      n_pdu->sz = dlc - (1U + offs);
      result = N_OK;
      break;

    case N_PCI_T_FF:
      // Combine two bytes into a larger value, clearly intentional
      n_pdu->n_pci.dl = ((uint16_t)(dt[offs] & 0x0FU) << 8U) | dt[1U + offs];
      n_pdu->sz = dlc - (2U + offs);
      result = N_OK;
      break;

    case N_PCI_T_FC:
      // Sequential assignments based on protocol, clearly used
      n_pdu->n_pci.fs = (uint8_t)(dt[offs] & 0x0FU);
      n_pdu->n_pci.bs = dt[1U + offs];
      n_pdu->n_pci.st = dt[2U + offs];
      n_pdu->sz = dlc - (3U + offs); // Adjust for correct data length calculation
      result = N_OK;
      break;

    default:
      result = N_ERROR;
      break;
    }
  }

  return result;
}


/*
 * Helper function to find the closest can_dl
* // 参考 15765-2 Table 9
 *
 * @param data_size 一次request传输的大小，通常是 <= 4095
 */
uint8_t n_get_dt_offset(addr_md address, pci_type pci, uint16_t data_size)
{
  uint8_t offset = (address & 0x01);

  switch (pci)
  {
  case N_PCI_T_SF:
    offset += 1; // CAN_DL <=8 时， 占一个字节(SF,SF_DL)
    offset += data_size <= (8 - offset) ? 0 : 1;// CAN_DL >8 时， 再增加一个字节 SF_DL
    break;
  case N_PCI_T_FF:
    offset += 2;
    offset += data_size > 4095 ? 4 : 0; // FF_DL 占4个字节
    break;
  case N_PCI_T_CF:
    offset += 1; // 占一个字节(CF,SN)
    break;
  case N_PCI_T_FC:
    offset += 3;// 占三个字节(FC,FS) (BS) (STmin)
    break;
  default:
    offset = 0;
    break;
  }
  return offset;
}


/*
 * Helper function of 'n_pdu_pack' to use the frame payload.
* n_pdu->dt    《==   dt（USER DATA部分）
 * 拷贝dt实际数据到 n_pdu结构体；   前提是n_pdu结构体中的其他值都正确填充
 *
 * @param dt 拷贝的数据 （不包含PCI）
 */
n_rslt n_pdu_pack_dt(addr_md mode, n_pdu_t* n_pdu, uint8_t* dt)
{
  n_rslt result = N_ERROR;

  if (dt != NULL)
  {
    switch (n_pdu->n_pci.pt)
    {
    case N_PCI_T_SF:
      memmove(&n_pdu->dt[n_get_dt_offset(mode, N_PCI_T_SF, n_pdu->sz)], dt, n_pdu->sz);
      break;
    case N_PCI_T_FF:
      memmove(&n_pdu->dt[n_get_dt_offset(mode, N_PCI_T_FF, n_pdu->sz)], dt, n_pdu->sz);
      break;
    case N_PCI_T_CF:
      memmove(&n_pdu->dt[n_get_dt_offset(mode, N_PCI_T_CF, n_pdu->sz)], dt, n_pdu->sz);
      break;
    case N_PCI_T_FC:
      memmove(&n_pdu->dt[n_get_dt_offset(mode, N_PCI_T_FC, n_pdu->sz)], dt, n_pdu->sz);

      break;

    default:
      break;
    }
    result = N_OK;
  }
  return result;
}


/*
 * Helper function of 'n_pdu_unpack' to use the frame payload.
 * n_pdu->dt    《==   dt（USER DATA部分）
 * @params dt 包含 PCI 的数据
 */
n_rslt n_pdu_unpack_dt(addr_md mode, n_pdu_t* n_pdu, uint8_t* dt)
{
  n_rslt result = N_ERROR;

  if ((n_pdu != NULL) && (dt != NULL))
  {
    switch (n_pdu->n_pci.pt)
    {
    case N_PCI_T_SF:
      memmove(n_pdu->dt, &dt[n_get_dt_offset(mode, N_PCI_T_SF, n_pdu->n_pci.dl)], n_pdu->n_pci.dl);
      result = N_OK;
      break;
    case N_PCI_T_FF:
      memmove(n_pdu->dt, &dt[n_get_dt_offset(mode, N_PCI_T_FF, n_pdu->sz)], n_pdu->sz);
      result = N_OK;
      break;
    case N_PCI_T_CF:
      memmove(n_pdu->dt, &dt[n_get_dt_offset(mode, N_PCI_T_CF, n_pdu->sz)], n_pdu->sz);
      result = N_OK;
      break;
    case N_PCI_T_FC:
      memmove(n_pdu->dt, &dt[n_get_dt_offset(mode, N_PCI_T_FC, n_pdu->sz)], n_pdu->sz);
      result = N_OK;
      break;
    default:
      result = N_ERROR;
      break;
    }
  }
  return result;
}

/*
 * Helper function to find the closest can_dl
 * Table 3 — CLASSICAL CAN/CAN FD data length comparison table // DLC 只有4位，
 * 表示长度范围只能是0~15; 所以如果是CAN_FD的话，CAN_DL最大是64； 所以需要15->64；
 * 如果DLC大于8，那么对于CLASSICAL CAN来说，就当成CAN_DL = 8
 * Table 6 — Definition of TX_DL configuration values
 * 该函数是一个辅助函数，用于查找最接近给定大小的CAN数据长度（can_dl）
 * @param size 实际网络层的大小 pci bytes + data len.
 * @return 0~64 can_dl
 */
uint8_t n_get_closest_can_dl(uint8_t size, cbus_fr_format tmt)
{
  uint8_t rval = 0;

  if (tmt == CBUS_FR_FRM_STD)
  {
    rval = (size <= 0x08U) ? size : 0x08U;
  }
  else
  {
    if (size <= 8)
    {
      rval = size;
    }
    else if (size <= 12)
    {
      rval = 12;
    }
    else if (size <= 16)
    {
      rval = 16;
    }
    else if (size <= 20)
    {
      rval = 20;
    }
    else if (size <= 24)
    {
      rval = 24;
    }
    else if (size <= 32)
    {
      rval = 32;
    }
    else if (size <= 48)
    {
      rval = 48;
    }
    else
    {
      rval = 64;
    }
  }

  return rval;
}

/*
 * Helper function to find which PCI_Type the outbound stream has to use
 * 判断发送帧的类型
 * 1. 若SN >0 ， 说明FF刚发出去，要发CF
 * 2. 若data len < X, 就是SF,否则就是FF； (X根据CAN还是CAN FD不一样)
 * @return SF,FF,CF
 */
pci_type n_out_frame_type(iso15765_t* instance)
{
  pci_type result = N_PCI_T_CF;

  if (instance->out.cf_cnt == 0)
  {
    // 第一次发送帧， 只能是SF, FF
    if ((instance->addr_md & 0x01) == 1)
    {
      if (instance->out.fr_fmt == CBUS_FR_FRM_STD)
      {
        result = instance->out.msg_sz <= 6 ? N_PCI_T_SF : N_PCI_T_FF;
      }
      else
      {
        result = instance->out.msg_sz <= 61 ? N_PCI_T_SF : N_PCI_T_FF;
      }
    }
    else
    {
      if (instance->out.fr_fmt == CBUS_FR_FRM_STD)
      {
        result = instance->out.msg_sz <= 7 ? N_PCI_T_SF : N_PCI_T_FF;
      }
      else
      {
        result = instance->out.msg_sz <= 62 ? N_PCI_T_SF : N_PCI_T_FF;
      }
    }
  }
  return result;
}

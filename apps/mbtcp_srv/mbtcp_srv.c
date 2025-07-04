#include "mbtcp_srv.h"
#include "string.h"

static void mb_FunctionError(mbtcp_srv_t *, u8_t, mb_err_t);
static mb_err_t mb_FunctionX06(mbtcp_srv_t *);
static mb_err_t mb_FunctionX10(mbtcp_srv_t *);
static mb_err_t mb_FunctionX04(mbtcp_srv_t *);
static int mb_SetRegister(MbReg2_t *, u8_t *);
static void mb_Default(mbtcp_srv_t *);
__weak void	mb_MspInit(mbtcp_srv_t *);

//mbtcp_srv_t xMbTcpSrv;

/**	----------------------------------------------------------------------------
	* @brief ������������� ���������.
	* @param px: ��������� �� ���.���������.
	* @retval none: ��� */
void
	mb_Init(mbtcp_srv_t *px) {
/*----------------------------------------------------------------------------*/
	// ��������� �������������
	mb_Default(px);
	// ������������
	mb_MspInit(px);
}

/**	----------------------------------------------------------------------------
	* @brief ������������� ��������� (���������).
	* @param px: ��������� �� ���.���������.
	* @retval none: ��� */
static void
	mb_Default(mbtcp_srv_t *px) {
/*----------------------------------------------------------------------------*/
	/*for (int i=0;i<MB_INPUT_SIZE;i++) {
		px->pxInputRegs[i].xLim.ssUp = INT16_MAX;
		px->pxInputRegs[i].xLim.ssLow = INT16_MIN;
		px->pxInputRegs[i].xVal.eType = S16;
		px->pxInputRegs[i].xVal.x.ss = 0;
	}*/
  memset(px->asInputRegs, 0, sizeof(px->asInputRegs));
  
	for (int i=0;i<MB_HOLDING_SIZE;i++) {
		px->pxHoldingRegs[i].xLim.ssUp = INT16_MAX;
		px->pxHoldingRegs[i].xLim.ssLow = INT16_MIN;
		px->pxHoldingRegs[i].xVal.eType = S16;
		px->pxHoldingRegs[i].xVal.x.ss = 0;
	}
}

/**	----------------------------------------------------------------------------
	* @brief ������������� ��������� (������� �����������).
	* @param px: ��������� �� ���.���������.
	* @retval none: ��� */
__weak void
	mb_MspInit(mbtcp_srv_t * px) {
/*----------------------------------------------------------------------------*/
	
}

/**	----------------------------------------------------------------------------
	* @brief ������ ������ ������� Modbus RTU over TCP
	* @param argv: ��������� �� ��.
	* @retval none: ��� */
void
	mbtcp_srv_check(mbtcp_srv_t * px) {
/*----------------------------------------------------------------------------*/
	mb_err_t err = MB_ILFUNC;
	u8_t code = px->xData.xIn.pcBuf[1];
	
	// ����� ������� 04
	if (code == 0x04) {
		err = mb_FunctionX04(px);
	} 

#if defined MB_DEBUG
	// ����� ������� 06. �������������� � ������������ ����
	if (code == 0x06) {
		err = mb_FunctionX06(px);
	}
#endif
	
	// ����� ������� 16
	if (code == 0x10) {
		err = mb_FunctionX10(px);
	}
	// ����� ������� �������� ������� err
	mb_FunctionError(px, code, err);
}

/**	----------------------------------------------------------------------------
	* @brief ������� 0x8X (������) Modbus RTU
	* @param ptr: ���.���������
	* @retval mb_err_t: ������ ���������� �������. */
static void
	mb_FunctionError(mbtcp_srv_t * ptr, u8_t ucCodeFun, mb_err_t eCodeEr) {
/*----------------------------------------------------------------------------*/
	// ���� ������ �� ����, �� ������ �� �������
	if (eCodeEr == MB_OK)
		return;
	// ������� ���� �����
	memset(ptr->xData.xOut.pcBuf, 0, sizeof(ptr->xData.xOut.pcBuf));
	// 1/?. �����. ��������� ���������
	ptr->xData.xOut.pcBuf[0] = ptr->xData.xIn.pcBuf[0];
	ptr->xData.xOut.pcBuf[1] = 0x80 | ucCodeFun; // TODO ��� ������ ��� ����������� �������?
	ptr->xData.xOut.pcBuf[2] = (u8_t)eCodeEr;
	// ������ ������
	ptr->xData.xOut.usSize = 3;
}

/**	----------------------------------------------------------------------------
	* @brief ������� 0x04 (04) Modbus RTU
	* @param px: ���.���������
	* @retval mb_err_t: ������ ���������� �������. */
static mb_err_t
	mb_FunctionX04(mbtcp_srv_t * ptr) {
/*----------------------------------------------------------------------------*/
	//static u16_t regStartPos, regNumber, regPage;
	u32_t i;
  u16_t temp;
	
	// ����������� ��������
  // 0 - 0   (0b000'0000'0000'0000)
  // 1 - 128 (0b000'0000'1000'0000)
  // 2 - 256 (0b000'0001'0000'0000)
  // 3 - 384 (0b000'0001'1000'0000)
  // 4 - 512 (0b000'0010'0000'0000)
  // 5 - 640 (0b000'0010'1000'0000)
  // 6 - 768 (0b000'0011'0000'0000)
  // 7 - 896 (0b000'0011'1000'0000)
  ptr->usPage = (u16_t)((ptr->xData.xIn.pcBuf[2] << 1) | 
		((ptr->xData.xIn.pcBuf[3] & 0x80) >> 7));
  //regPage |= ptr->xData.xIn.pcBuf[3] & 0x80;
	//regPage = (ptr->xData.xIn.pcBuf[2] | (ptr->xData.xIn.pcBuf[3] & 0x80));
	if (ptr->usPage == 5) {
		asm("nop");
	}
	if (ptr->usPage > (MB_PAGES-1)) {
		return MB_ILADDR;
	}
	
	/*	�������� ������ ���������� ��������	*/
	// ����� �������, ��� ��������� ��������� �������������� ���. ����� ���� �� 0 �� 255
	ptr->usStartPos = (ptr->xData.xIn.pcBuf[3] & 0x7f);
	if (ptr->usStartPos > MB_INPUT_SIZE-1) {
		return MB_ILADDR;
	}
	
	/*	�������� ����� ������������� ���������	*/
	ptr->usNumber = (ptr->xData.xIn.pcBuf[4] << 8) + (ptr->xData.xIn.pcBuf[5]);
	if ((ptr->usNumber > MB_INPUT_SIZE) | (ptr->usNumber == 0) | 
  ((ptr->usStartPos+ptr->usNumber) > MB_INPUT_SIZE)) {
		return MB_ILADDR;
	}
  
  // ������ ��������
  for (int i=0; i<REGS_NUM; i++)
    ptr->asInputRegs[i]=0;
	
	// ����� ���������������� �������
	if (ptr->pvFunc4 != NULL)
		ptr->pvFunc4(ptr, ptr->usPage);
  
  // ������� ���.�����
	for (i=ptr->usStartPos; i<sizeof(ptr->xData.xOut.pcBuf); i++) {
    ptr->xData.xOut.pcBuf[i] = 0;
	}
	
	// 2/3. ���������� ���� ������ �� ��������� ������
	for (i=ptr->usStartPos; i<(ptr->usStartPos + ptr->usNumber); i++) {
		STOPB((ptr->xData.xOut.pcBuf+(i-ptr->usStartPos)*2+3), 
      ptr->asInputRegs[i]/*(ptr->pxInputRegs+i)->xVal.x.ss*/);
	}
	
	// 1/2. �����. ��������� ���������
	// ����������� 2 ���� �� ��.������ ������� � ���.�����.
	memcpy(ptr->xData.xOut.pcBuf+0, (const void*)(ptr->xData.xIn.pcBuf+0), 2);
	// ���-�� ����
	ptr->xData.xOut.pcBuf[2] = 2*ptr->usNumber;
	// 2/2. �����. ������ ���������� �������� �������� � ������ �� �������� 
	ptr->xData.xOut.usSize = 3 + 2*ptr->usNumber;
	
	return MB_OK;
}

/**	----------------------------------------------------------------------------
	* @brief ������� 0x06 (06) Modbus RTU
	* @param ptr: ���.���������
	* @retval mb_err_t: ������ ���������� �������. */
static mb_err_t
	mb_FunctionX06(mbtcp_srv_t * ptr) {
/*----------------------------------------------------------------------------*/
	static u16_t regStartPos, regPage;
	
	//	����������� ��������
	regPage = (ptr->xData.xIn.pcBuf[2]);
	if (regPage > (MB_PAGES-1)) {
		return MB_ILADDR;
	}
	
	//	�������� ������ ���������� ��������.
	regStartPos = (ptr->xData.xIn.pcBuf[3]);
	if (regStartPos > MB_HOLDING_SIZE-1) {
		return MB_ILADDR;
	}
	
/*#if defined MB_DBGREGS	
	//	�����, ���� ����������� ����� ���� ������� ��������� ��� �������
	if (regStartPos < MB_DBGREG_ADDR) {
		return MB_ILADDR;
	}
#endif*/

	// ������������ �������� �������� �������� �� ��.������.
	switch (regPage) {
		// �������� 0
		case (0): {
			if (mb_SetRegister(ptr->pxHoldingRegs+regStartPos, ptr->xData.xIn.pcBuf+4) < 0)
				return MB_ILVAL;
		} break;
		// �������� ...
		default: break;
	}
	
	// ����� ���������������� �������
	if (ptr->pvFunc6 != NULL)
		ptr->pvFunc6(ptr);
	
	// 1/2. �����. ��������� ���������
	// ����������� 6 ���� �� ��.������ ������� � ���.�����.
	memcpy(ptr->xData.xOut.pcBuf+0, (const void*)(ptr->xData.xIn.pcBuf+0), 6);
	// 2/2. �����. ������ ���������� �������� �������� � ������ �� �������� 
	ptr->xData.xOut.usSize = 6;
	
	return MB_OK;
}


/**	----------------------------------------------------------------------------
	* @brief ������� 0x10 (16) Modbus RTU
	* @param ptr: ���.���������
	* @retval mb_err_t: ������ ���������� �������. */
static mb_err_t
	mb_FunctionX10(mbtcp_srv_t * ptr) {
/*----------------------------------------------------------------------------*/
	static u16_t regstart, regPage, regnum;
	u16_t i;
	
	//	����������� ��������
	regPage = (ptr->xData.xIn.pcBuf[2]);
	if (regPage > (MB_PAGES-1)) {
		return MB_ILADDR;
	}
	
	///	�������� ������ ���������� ��������
	// ����� �������, ��� ��������� ��������� �������������� ���. ����� ���� �� 0 �� 255
	regstart = (ptr->xData.xIn.pcBuf[3]);
	if (regstart > MB_HOLDING_SIZE-1) {
		return MB_ILADDR;
	}
	
	// �������� ����� ������������� ���������
	regnum = (ptr->xData.xIn.pcBuf[4] << 8) | (ptr->xData.xIn.pcBuf[5]);
	if	((regnum==0)|((regstart+regnum)>MB_HOLDING_SIZE)) {
		return MB_ILADDR;
	}
	
	// ��� ���� ��������
	if (regnum != ptr->xData.xIn.pcBuf[6]/2) {
		return MB_ILADDR;
	}
	
	// ���
	if ((ptr->xData.xIn.usSize-7) != ptr->xData.xIn.pcBuf[6]) {
		return MB_ILADDR;
	}
	
	// ���������� ���� ������ �� ��������� ������
	switch (regPage) {
		// �������� 0
		case (0): {
			/* � ��� ���� */
			for (i=regstart; i<(regstart + regnum); i++) {
				if (mb_SetRegister(ptr->pxHoldingRegs+i, ptr->xData.xIn.pcBuf+7+2*i) < 0)
					return MB_ILVAL;
			}
		} break;
		// �������� ...
		default: break;
	}
	
	// ����� ���������������� �������
	if (ptr->pvFunc16 != NULL)
		ptr->pvFunc16(ptr);
	
	// 1/2. �����. ����������� 6 ���� �� ��.������ ������� � ���.�����.
	memcpy(ptr->xData.xOut.pcBuf+0, (const void*)(ptr->xData.xIn.pcBuf+0), 6);
	// 2/2. �����. ������ ���������� �������� �������� � ������ �� �������� 
	ptr->xData.xOut.usSize = 6;
	
	return MB_OK;
}

/**	----------------------------------------------------------------------------
	* @brief ������� 0x8X (������) Modbus RTU
	* @param px: ���.���������
	* @retval mb_err_t: ������ ���������� �������. */
static int
	mb_SetRegister(MbReg2_t* px, u8_t* pBuf) {
/*----------------------------------------------------------------------------*/
	s16_t ssTemp;
	// ����� ��� ����� � ��������� �� � ushort
	ssTemp = PBTOS(pBuf);
	// ���������, ����� ����� �� ����� �� �������.
	if (ssTemp > px->xLim.ssUp)
		return -1;
	if (ssTemp <= px->xLim.ssLow)
		return -1;
	// ���� �� ��, �� ������� � �������
	px->xVal.x.ss = ssTemp;
	// ���������� OK
	return 0;
}

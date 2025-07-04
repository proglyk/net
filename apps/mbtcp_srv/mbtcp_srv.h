#ifndef _MBTCP_SRV_H_
#define _MBTCP_SRV_H_

#include "stdint.h"
#include "userint.h"




// ��������� ���. ��� �������
// ������ � � ���� �������� ����������� IAR

#define REGS_NUM                        (125)
// ������ ������� (�����) ��������� ��� ������ � ������ (R/W) ���������.
#define MB_INPUT_SIZE                   REGS_NUM
#define MB_HOLDING_SIZE                 REGS_NUM
// ������ �������
#define BUFSIZE                         (2*REGS_NUM)
// ���-�� �������������� �������
#define MB_PAGES                        (8)


// Pointer to Byte -> TO -> Short
#define PBTOS(a)	(u16_t)(((((u16_t)(*(a+0))) << 8) & 0xff00)  | \
									((*(a+1)) & 0x00ff))
// Short -> TO -> Pointer to Byte
#define STOPB(a,b) {a[0]=(u8_t)((b & 0xff00)>>8);\
										a[1]=(u8_t)(b & 0x00ff); }
// ������ �������
//#define MB_MAX_REGS 11

typedef enum {
	S16 = 0,
	P16 = 1
} rtype_t;

typedef struct {
	rtype_t eType;
	struct {
		s16_t 	ss;
		s16_t* 	ptr;
	} x;
} MbReg3_t;

typedef struct {
// �������� ��������
	MbReg3_t xVal;
// MbReg_t uVal;
// ������ ��������
 struct {
		s16_t ssUp;
		s16_t ssLow;
 } xLim;
} MbReg2_t;

// ������
typedef struct {
	// �� ����
	struct {
		u16_t usSize;
		u8_t pcBuf[BUFSIZE];
	} xIn;
	// �� ��������
	struct {
		u16_t usSize;
		u8_t pcBuf[BUFSIZE+10];
	} xOut;
} MbData_t;

// ModBus Control Block
struct mbcb_st;
typedef struct mbcb_st mbtcp_srv_t;
struct mbcb_st {
// �������� ������ �����
// 04 Input Registers (3x). ������ �� ��������������.
//	MbReg2_t pxInputRegs[MB_INPUT_SIZE];
  s16_t asInputRegs[MB_INPUT_SIZE];
// 03 Holding Registers (4x). ������ ��������� 06 � 16.
	MbReg2_t pxHoldingRegs[MB_HOLDING_SIZE];
  u16_t usStartPos;
  u16_t usNumber;
  u16_t usPage;
// ��������� �� ���������������� �������
// Input Registers (3x).
// ������.
	void (* pvFunc4)(mbtcp_srv_t*, u32_t);
// Holding Registers (4x)
// ������ ���������� ��������
	void (* pvFunc6)(mbtcp_srv_t*);
// ������ ��������� ���������
	void (* pvFunc16)(mbtcp_srv_t*);
// ������ ������/��������
	MbData_t xData;
};

// ���������� ����� ������ ModBus
typedef enum {
	MB_OK = 		0x00,	// ������ ��, ������ ���
	MB_ILFUNC = 0x01,	// Illegal Function
	MB_ILADDR = 0x02,	// Illegal Data Address
	MB_ILVAL = 	0x03,	// Illegal Data Value
	MB_FAIL = 	0x04,	// Slave Device Failure
	MB_ACK = 		0x05,	// Acknowledge
	MB_BUSY = 	0x06,	// Slave Device Busy
	MB_NACK = 	0x07,	// Negative Acknowledge
	MB_MEM = 		0x08,	// Memory Parity Error
	MB_GWPATH = 0x0A,	// Gateway Path Unavailable
	MB_GWRESP = 0x0B,	// Gateway Target Device Failed to Respond
} mb_err_t;

void mbtcp_srv_check(mbtcp_srv_t *px);
void mb_Init(mbtcp_srv_t *px);

#endif //_MBTCP_SRV_H_
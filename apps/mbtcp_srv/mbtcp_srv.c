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
	* @brief Инициализация регистров.
	* @param px: Указатель на упр.структуру.
	* @retval none: Нет */
void
	mb_Init(mbtcp_srv_t *px) {
/*----------------------------------------------------------------------------*/
	// Начальная инициализация
	mb_Default(px);
	// Пользователь
	mb_MspInit(px);
}

/**	----------------------------------------------------------------------------
	* @brief Инициализация регистров (начальная).
	* @param px: Указатель на упр.структуру.
	* @retval none: Нет */
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
	* @brief Инициализация регистров (функция ползователя).
	* @param px: Указатель на упр.структуру.
	* @retval none: Нет */
__weak void
	mb_MspInit(mbtcp_srv_t * px) {
/*----------------------------------------------------------------------------*/
	
}

/**	----------------------------------------------------------------------------
	* @brief Запуск задачи сервера Modbus RTU over TCP
	* @param argv: Указатель на всё.
	* @retval none: Нет */
void
	mbtcp_srv_check(mbtcp_srv_t * px) {
/*----------------------------------------------------------------------------*/
	mb_err_t err = MB_ILFUNC;
	u8_t code = px->xData.xIn.pcBuf[1];
	
	// Вызов функции 04
	if (code == 0x04) {
		err = mb_FunctionX04(px);
	} 

#if defined MB_DEBUG
	// Вызов функции 06. Поддерживается в ограниченном виде
	if (code == 0x06) {
		err = mb_FunctionX06(px);
	}
#endif
	
	// Вызов функции 16
	if (code == 0x10) {
		err = mb_FunctionX10(px);
	}
	// Вызов функции проверки статуса err
	mb_FunctionError(px, code, err);
}

/**	----------------------------------------------------------------------------
	* @brief Функция 0x8X (Ошибка) Modbus RTU
	* @param ptr: Упр.структура
	* @retval mb_err_t: Статус выполнения функции. */
static void
	mb_FunctionError(mbtcp_srv_t * ptr, u8_t ucCodeFun, mb_err_t eCodeEr) {
/*----------------------------------------------------------------------------*/
	// Если ошибок не было, то ничего не трогаем
	if (eCodeEr == MB_OK)
		return;
	// Очищаем весь буфер
	memset(ptr->xData.xOut.pcBuf, 0, sizeof(ptr->xData.xOut.pcBuf));
	// 1/?. Ответ. Заголовок сообщения
	ptr->xData.xOut.pcBuf[0] = ptr->xData.xIn.pcBuf[0];
	ptr->xData.xOut.pcBuf[1] = 0x80 | ucCodeFun; // TODO Как узнать код запрошенной функции?
	ptr->xData.xOut.pcBuf[2] = (u8_t)eCodeEr;
	// Задаем размер
	ptr->xData.xOut.usSize = 3;
}

/**	----------------------------------------------------------------------------
	* @brief Функция 0x04 (04) Modbus RTU
	* @param px: Упр.структура
	* @retval mb_err_t: Статус выполнения функции. */
static mb_err_t
	mb_FunctionX04(mbtcp_srv_t * ptr) {
/*----------------------------------------------------------------------------*/
	//static u16_t regStartPos, regNumber, regPage;
	u32_t i;
  u16_t temp;
	
	// Определение страницы
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
	
	/*	Проверка адреса начального регистра	*/
	// Здесь выходит, что начальное положение запрашиваемого рег. может быть от 0 до 255
	ptr->usStartPos = (ptr->xData.xIn.pcBuf[3] & 0x7f);
	if (ptr->usStartPos > MB_INPUT_SIZE-1) {
		return MB_ILADDR;
	}
	
	/*	Проверка числа запрашиваемых регистров	*/
	ptr->usNumber = (ptr->xData.xIn.pcBuf[4] << 8) + (ptr->xData.xIn.pcBuf[5]);
	if ((ptr->usNumber > MB_INPUT_SIZE) | (ptr->usNumber == 0) | 
  ((ptr->usStartPos+ptr->usNumber) > MB_INPUT_SIZE)) {
		return MB_ILADDR;
	}
  
  // очищаю регистры
  for (int i=0; i<REGS_NUM; i++)
    ptr->asInputRegs[i]=0;
	
	// Вызов пользовательской функции
	if (ptr->pvFunc4 != NULL)
		ptr->pvFunc4(ptr, ptr->usPage);
  
  // Обнуляю вых.буфер
	for (i=ptr->usStartPos; i<sizeof(ptr->xData.xOut.pcBuf); i++) {
    ptr->xData.xOut.pcBuf[i] = 0;
	}
	
	// 2/3. Заполнение байт данных из регистров памяти
	for (i=ptr->usStartPos; i<(ptr->usStartPos + ptr->usNumber); i++) {
		STOPB((ptr->xData.xOut.pcBuf+(i-ptr->usStartPos)*2+3), 
      ptr->asInputRegs[i]/*(ptr->pxInputRegs+i)->xVal.x.ss*/);
	}
	
	// 1/2. Ответ. Заголовок сообщения
	// Копирование 2 байт из вх.буфера наперед в вых.буфер.
	memcpy(ptr->xData.xOut.pcBuf+0, (const void*)(ptr->xData.xIn.pcBuf+0), 2);
	// Кол-во байт
	ptr->xData.xOut.pcBuf[2] = 2*ptr->usNumber;
	// 2/2. Ответ. Размер полученной полезной нагрузки в буфере на передачу 
	ptr->xData.xOut.usSize = 3 + 2*ptr->usNumber;
	
	return MB_OK;
}

/**	----------------------------------------------------------------------------
	* @brief Функция 0x06 (06) Modbus RTU
	* @param ptr: Упр.структура
	* @retval mb_err_t: Статус выполнения функции. */
static mb_err_t
	mb_FunctionX06(mbtcp_srv_t * ptr) {
/*----------------------------------------------------------------------------*/
	static u16_t regStartPos, regPage;
	
	//	Определение страницы
	regPage = (ptr->xData.xIn.pcBuf[2]);
	if (regPage > (MB_PAGES-1)) {
		return MB_ILADDR;
	}
	
	//	Проверка адреса начального регистра.
	regStartPos = (ptr->xData.xIn.pcBuf[3]);
	if (regStartPos > MB_HOLDING_SIZE-1) {
		return MB_ILADDR;
	}
	
/*#if defined MB_DBGREGS	
	//	Выйти, если запрошенный адрес ниже адресов регистров для отладки
	if (regStartPos < MB_DBGREG_ADDR) {
		return MB_ILADDR;
	}
#endif*/

	// Присваивание целевому регистру значения из вх.буфера.
	switch (regPage) {
		// Страница 0
		case (0): {
			if (mb_SetRegister(ptr->pxHoldingRegs+regStartPos, ptr->xData.xIn.pcBuf+4) < 0)
				return MB_ILVAL;
		} break;
		// Страница ...
		default: break;
	}
	
	// Вызов пользовательской функции
	if (ptr->pvFunc6 != NULL)
		ptr->pvFunc6(ptr);
	
	// 1/2. Ответ. Заголовок сообщения
	// Копирование 6 байт из вх.буфера наперед в вых.буфер.
	memcpy(ptr->xData.xOut.pcBuf+0, (const void*)(ptr->xData.xIn.pcBuf+0), 6);
	// 2/2. Ответ. Размер полученной полезной нагрузки в буфере на передачу 
	ptr->xData.xOut.usSize = 6;
	
	return MB_OK;
}


/**	----------------------------------------------------------------------------
	* @brief Функция 0x10 (16) Modbus RTU
	* @param ptr: Упр.структура
	* @retval mb_err_t: Статус выполнения функции. */
static mb_err_t
	mb_FunctionX10(mbtcp_srv_t * ptr) {
/*----------------------------------------------------------------------------*/
	static u16_t regstart, regPage, regnum;
	u16_t i;
	
	//	Определение страницы
	regPage = (ptr->xData.xIn.pcBuf[2]);
	if (regPage > (MB_PAGES-1)) {
		return MB_ILADDR;
	}
	
	///	Проверка адреса начального регистра
	// Здесь выходит, что начальное положение запрашиваемого рег. может быть от 0 до 255
	regstart = (ptr->xData.xIn.pcBuf[3]);
	if (regstart > MB_HOLDING_SIZE-1) {
		return MB_ILADDR;
	}
	
	// Проверка числа запрашиваемых регистров
	regnum = (ptr->xData.xIn.pcBuf[4] << 8) | (ptr->xData.xIn.pcBuf[5]);
	if	((regnum==0)|((regstart+regnum)>MB_HOLDING_SIZE)) {
		return MB_ILADDR;
	}
	
	// еще одна проверка
	if (regnum != ptr->xData.xIn.pcBuf[6]/2) {
		return MB_ILADDR;
	}
	
	// еще
	if ((ptr->xData.xIn.usSize-7) != ptr->xData.xIn.pcBuf[6]) {
		return MB_ILADDR;
	}
	
	// Заполнение байт данных из регистров памяти
	switch (regPage) {
		// Страница 0
		case (0): {
			/* И еще чото */
			for (i=regstart; i<(regstart + regnum); i++) {
				if (mb_SetRegister(ptr->pxHoldingRegs+i, ptr->xData.xIn.pcBuf+7+2*i) < 0)
					return MB_ILVAL;
			}
		} break;
		// Страница ...
		default: break;
	}
	
	// Вызов пользовательской функции
	if (ptr->pvFunc16 != NULL)
		ptr->pvFunc16(ptr);
	
	// 1/2. Ответ. Копирование 6 байт из вх.буфера наперед в вых.буфер.
	memcpy(ptr->xData.xOut.pcBuf+0, (const void*)(ptr->xData.xIn.pcBuf+0), 6);
	// 2/2. Ответ. Размер полученной полезной нагрузки в буфере на передачу 
	ptr->xData.xOut.usSize = 6;
	
	return MB_OK;
}

/**	----------------------------------------------------------------------------
	* @brief Функция 0x8X (Ошибка) Modbus RTU
	* @param px: Упр.структура
	* @retval mb_err_t: Статус выполнения функции. */
static int
	mb_SetRegister(MbReg2_t* px, u8_t* pBuf) {
/*----------------------------------------------------------------------------*/
	s16_t ssTemp;
	// Берем два байта и переводим их в ushort
	ssTemp = PBTOS(pBuf);
	// Проверяем, чтобы число не вышло за пределы.
	if (ssTemp > px->xLim.ssUp)
		return -1;
	if (ssTemp <= px->xLim.ssLow)
		return -1;
	// Если всё ок, то заносим в регистр
	px->xVal.x.ss = ssTemp;
	// Возвращаем OK
	return 0;
}

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* TESTING */
/* IN WORK SPACE */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// Select Test Mode
#define MODE_PATTERN_1		(1)			// TX/RX Pattern to verify SPI TX/RS
#define MODE_READ_BITMAP	(2)			// Wait INT then read Bitmap
#define MODE_LOOP_1			(3)			// Read/Change Loop Test
#define MODE_LOOP_RAW		(4)			// Change To  RAW then keeping read


//#define MODE_SIMPLE_LOOP	(4)

#define TEST_MODE		(MODE_READ_BITMAP)

#define TRIGGER_SOURCE_INT		0	//	0:INT, 1:Timer
#define TRIGGER_SOURCE_TIMER	1
#define TRIGGER_BY				TRIGGER_SOURCE_INT

#define TEST_MODE_READ			0
#define TEST_MODE_CHANGE		1

#define TEST_MODE_WRITE     	3
#define MAX_TEST_MODE       	4
#define MAX_TEST_COMMAND       	5

#define TEST_READ_LOOP			10

#define SPI_FRAME_8       		8
#define SPI_FRAME_32       		32
#define SPI_MAX_FRAME       SPI_FRAME_32
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint8_t tx_buf[SPI_MAX_FRAME] __attribute__((aligned(4))) =
{
    0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01
};

static uint8_t rx_buf[SPI_MAX_FRAME];

volatile uint8_t g_int_pending = 0;
volatile uint8_t g_btn_pending = 0;
volatile uint8_t g_test_mode = TEST_MODE_READ;
volatile uint8_t g_test_count = 0;

volatile uint8_t g_change_mode = 0;

volatile uint8_t g_current_len = SPI_FRAME_8;


static uint8_t loop_count  = 0xF8;
static uint8_t write_count = 0x00;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void SPI_CS_Low(void);
static void SPI_CS_High(void);
static void delay_us(uint32_t us);
static HAL_StatusTypeDef SPI_Exchange(uint8_t *tx, uint8_t *rx, uint16_t len);
static void dump_buf(const char *tag, const uint8_t *buf, uint16_t len);

static void SPI_Test_Mode_Read(void);
static void SPI_Test_Mode_Write(void);
static void SPI_Test_Mode(uint8_t mode);

//static void dump_x8_hex_uint16(const char *tag, const uint16_t buf);
static void dumphex_x8_uint8(const char *tag, const uint8_t* buf);
static void dumphex_x8_uint16(const char *tag, const uint16_t* buf);

static void dump_buf(const char *tag, const uint8_t *buf, uint16_t len);

static void dumphex_raw(const uint16_t* raw);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

static void SPI_CS_Low(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}

static void SPI_CS_High(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

static void delay_us(uint32_t us)
{
    /*
     * Simple busy-loop delay for SPI timing debug.
     * This is not precise, but good enough for setup/hold margin testing.
     */
    uint32_t count = us * (SystemCoreClock / 1000000U / 5U);
    while (count--)
    {
        __NOP();
    }
}

static HAL_StatusTypeDef SPI_Exchange(uint8_t *tx, uint8_t *rx, uint16_t len)
{
    HAL_StatusTypeDef ret;

    SPI_CS_High();
    delay_us(1);

    SPI_CS_Low();

    /* Give slave some setup time after CS goes low */
    delay_us(1);

    ret = HAL_SPI_TransmitReceive(&hspi1, tx, rx, len, 1000);

    /* Wait until SPI is truly idle before releasing CS */
    while (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_BSY))
    {
    }

    /* Small hold time before CS high */
    delay_us(1);

    SPI_CS_High();

    return ret;
}

static void dump_buf(const char *tag, const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    printf("%s", tag);
    for (i = 0; i < len; i++)
    {
        printf("%02X", buf[i]);
        if (i != (len - 1))
        {
            printf(" ");
        }
    }
    printf("\r\n");
}


static void dumphex_x8_uint8(const char *tag, const uint8_t* buf)
{
    printf("[%s] %02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X\r\n",
    		tag,
			buf[0],
			buf[1],
			buf[2],
			buf[3],
			buf[4],
			buf[5],
			buf[6],
			buf[7]
			);
}

static void dumphex_x8_uint16(const char *tag, const uint16_t* buf)
{
    printf("[%s] %04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X\r\n",
    		tag,
			buf[0],
			buf[1],
			buf[2],
			buf[3],
			buf[4],
			buf[5],
			buf[6],
			buf[7]
			);
}

static void dumphex_x16_uint16(const char *tag, const uint16_t* buf)
{
    printf("[%s] %04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X\r\n%04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X\r\n",
    		tag,
			buf[0],
			buf[1],
			buf[2],
			buf[3],
			buf[4],
			buf[5],
			buf[6],
			buf[7],
			buf[8],
			buf[9],
			buf[10],
			buf[11],
			buf[12],
			buf[13],
			buf[14],
			buf[15]
			);
}

static void dumphex_raw(const uint16_t* raw)
{
	printf("RAW[0] %04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X\r\n",
			raw[0],
			raw[1],
			raw[2],
			raw[3],
			raw[4],
			raw[5],
			raw[6],
			raw[7]
		);

	printf("RAW[0] %04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X\r\n",
			raw[8],
			raw[9],
			raw[10],
			raw[11],
			raw[12],
			raw[13],
			raw[14],
			raw[15]
		);
}

static void SPI_Test_Pattern_1(void)
{
    HAL_StatusTypeDef ret;
    const uint16_t len = SPI_FRAME_8;

	tx_buf[0] = 0x12;
	tx_buf[1] = 0x34;
	tx_buf[2] = 0x56;
	tx_buf[3] = 0x78;
	tx_buf[4] = 0x9A;
	tx_buf[5] = 0xBC;
	tx_buf[6] = 0xDE;
	tx_buf[7] = 0xF0;

    memset(rx_buf, 0, sizeof(rx_buf));

	ret = SPI_Exchange(tx_buf, rx_buf, len);

	if  ( ret != HAL_OK)
		printf("Error , ret = %d\r\n", ret);

	dumphex_x8_uint8("TX1", tx_buf);
	dumphex_x8_uint8("RX1", rx_buf);
    printf("\r\n");
}

static void SPI_Test_Read(uint8_t len )
{
    memset(tx_buf, 0x00, sizeof(tx_buf));
    memset(rx_buf, 0x00, sizeof(rx_buf));

	SPI_Exchange(tx_buf, rx_buf, len);
}

static void SPI_Test_Read_Bitmap(void)
{
	SPI_Test_Read(8);
	printf("%s\r\n", __FUNCTION__);
	dumphex_x8_uint8("08", rx_buf);
}

static void SPI_Test_Read_RAW(void)
{
	SPI_Test_Read(32);
	printf("%s\r\n", __FUNCTION__);

	dumphex_x16_uint16("20",(uint16_t*) &rx_buf[0]);
}

static void SPI_Command(uint8_t len, uint8_t* buf)
{
    HAL_StatusTypeDef ret;

    memset(rx_buf, 0x00, sizeof(rx_buf));
    memcpy(tx_buf,buf,len);

    printf("CMD %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\r\n",
    	buf[0],
    	buf[1],
    	buf[2],
    	buf[3],
    	buf[4],
    	buf[5],
    	buf[6],
    	buf[7]
    );

    ret = SPI_Exchange(tx_buf, rx_buf, len);
}



static void SPI_Test_Mode_Change(uint8_t len)
{
    printf("%s\r\n", __FUNCTION__);

    uint8_t command[SPI_MAX_FRAME] = {0xA0,0x4C,0x45,0x4E,0x00,0x00,0x00,0x00};
    command[4] = len;
    command[5] = command[0] + command[1] + command[2] + command[3] +command[4];

    if (g_current_len == len)
    {
    	printf("Already %d byte mode", g_current_len);
    	return;
    }

    memset(rx_buf, 0x00, sizeof(rx_buf));
    memset(tx_buf, 0x00, sizeof(rx_buf));
    memcpy(tx_buf,&command[0],sizeof(command));


	SPI_Exchange(tx_buf, rx_buf, g_current_len);
    dump_buf("TX: ", tx_buf, g_current_len);
    dump_buf("RX: ", rx_buf, g_current_len);
    printf("\r\n");

    g_current_len = len;

}

static void SPI_Test_Mode_Write(void)
{
    HAL_StatusTypeDef ret;
    const uint16_t len = SPI_FRAME_8;

    /*
     * Fixed write pattern for slave verification
     */
    tx_buf[0] = 0x80;
    tx_buf[1] = 0x40;
    tx_buf[2] = 0x20;
    tx_buf[3] = 0x10;
    tx_buf[4] = 0x08;
    tx_buf[5] = 0x04;
    tx_buf[6] = 0x02;
    tx_buf[7] = write_count++;

    memset(rx_buf, 0x00, sizeof(rx_buf));

    ret = SPI_Exchange(tx_buf, rx_buf, len);

    printf("WRITE ret=%d\r\n", ret);
    dump_buf("TX: ", tx_buf, len);
    dump_buf("RX: ", rx_buf, len);
    printf("\r\n");
}

int pattern_lopp = 0;

uint8_t test_pattern[][8] = {
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	{0xA0, 0x4C, 0x45, 0x4E, 0x20, 0x9F, 0x00, 0x00},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x02},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x04},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x08},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x10},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x20},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x40},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x80},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x55},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0xAA},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0xC0},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0xC1},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0xC2},
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0xC3},
    {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55},
	{0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0},
	{0xA5, 0x5A, 0x3C, 0xC3, 0x96, 0x69, 0xF0, 0x0F},

};

static void SPI_Test_Command(void)
{
    const uint16_t len = SPI_FRAME_8;

    memcpy(tx_buf, &test_pattern[pattern_lopp], 8);
    pattern_lopp ++;
    pattern_lopp %= 18;

	SPI_Exchange(tx_buf, rx_buf, len);

    dump_buf("TX() : ", tx_buf, len);
}

#define TEST_MODE_READ			0
#define TEST_MODE_CHANGE_8		1
#define TEST_MODE_CHANGE_32		2

static void SPI_Test_Mode(uint8_t mode)
{
	switch(mode)
	{
	case TEST_MODE_READ:
		SPI_Test_Mode_Read();
		break;
	case TEST_MODE_CHANGE_8:
		SPI_Test_Mode_Change(8);
		break;
	case TEST_MODE_CHANGE_32:
		SPI_Test_Mode_Change(32);
		break;
	case TEST_MODE_WRITE:
		SPI_Test_Mode_Write();
		break;
	case MAX_TEST_COMMAND:
		SPI_Test_Command();
		break;
    default:
        printf("Unknown mode %u, fallback to READ\r\n", mode);
 //       SPI_Test_Mode_Read();
        break;
    }
}

static void SPI_Test_Auto(void)
{
	switch(g_test_mode)
	{
	case TEST_MODE_READ:
		SPI_Test_Mode_Read();
		g_test_count ++;
		if ( g_test_count >= TEST_READ_LOOP)
		{
			g_test_mode = TEST_MODE_CHANGE;
			g_test_count = 0;
		}

		break;
	case TEST_MODE_CHANGE:
		if ( g_current_len == SPI_FRAME_8)
			SPI_Test_Mode_Change(SPI_FRAME_32);
		else
			SPI_Test_Mode_Change(SPI_FRAME_8);
		g_test_mode = TEST_MODE_READ;
		g_test_count = 0;
		break;

	case TEST_MODE_WRITE:
		SPI_Test_Mode_Write();
		break;
	case MAX_TEST_COMMAND:
		SPI_Test_Command();
		break;
    default:
        printf("Unknown mode \r\n");
 //       SPI_Test_Mode_Read();
        break;
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_8)
  {
#if (TRIGGER_BY == TRIGGER_SOURCE_INT)
	  g_int_pending = 1;
#endif

#if (TEST_MODE == MODE_READ_BITMAP)
	  printf("INT\r\n");
	  g_int_pending = 1;
#endif

#if (TEST_MODE == MODE_LOOP_1)
	  g_int_pending = 1;
#endif

#if (TEST_MODE == MODE_LOOP_RAW)
	  g_int_pending = 1;
#endif
  }


  if (GPIO_Pin == B1_Pin)
  {
	  g_btn_pending = 1;
	  printf("PC13 BTN\r\n");
      HAL_GPIO_TogglePin(LD4_GPIO_Port, LD4_Pin);
  }
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
void   SPI1_Print_Prescale(void);
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  SPI_CS_High();
  HAL_Delay(100);
  printf("STM32 SPI Master Online->\r\n");
  printf("SYSCLK = %lu\r\n", HAL_RCC_GetSysClockFreq());
  printf("HCLK   = %lu\r\n", HAL_RCC_GetHCLKFreq());
  printf("PCLK1  = %lu\r\n", HAL_RCC_GetPCLK1Freq());
  printf("PCLK2  = %lu\r\n", HAL_RCC_GetPCLK2Freq());
  /* USER CODE END 2 */
  SPI1_Print_Prescale();


#if (TEST_MODE == MODE_PATTERN_1)

  printf("TEST_MODE::MODE_PATTERN_1\r\n");
  printf("Send 0x 12 34 56 78 ....\r\n");
  printf("To check if slave can Tx/Rx properly\r\n\r\n");
  while (1)
  {
	  HAL_Delay(500);
	  SPI_Test_Pattern_1();
  }
#endif

#if (TEST_MODE == MODE_READ_BITMAP)

  printf("TEST_MODE::MODE_READ_BITMAP\r\n");
  printf("Send 0x 00 00 00 00 ....\r\n");
  printf("To check if slave can Send Key properly\r\n\r\n");

  while (1)
  {
	  if (g_int_pending )
	  {
         g_int_pending = 0;
         SPI_Test_Read_Bitmap();
	  }
  }
#endif


#if (TEST_MODE == MODE_LOOP_1)

  printf("TEST_MODE::MODE_LOOP_1\r\n");
  printf("Read Bitmap/Change to RAW/Read RAW/Change to Bitmap\r\n");
  printf("To check if slave can Send Key properly\r\n\r\n");

  #define MAX_READ	10
  int read_loop = 0;
  int stage = 0;

  uint8_t command_raw[] =
  {0xA0,0x4C,0x45,0x4E,0x20,0x9F,0x00,0x00};
  uint8_t command_bmp[] =
  {
	0xA0,0x4C,0x45,0x4E,0x08,0x87,0x00,0x00,
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
	0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,
	0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
  };

  uint8_t read_raw[] =
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

  printf("STAGE LOOP Test\r\n");

  int paused = 0;

  while (1)
  {
	  switch (stage )
	  {
	  case 0:	// Read 10 bitamp
		  if (g_int_pending )
		  {
			  g_int_pending = 0;
			  SPI_Test_Read_Bitmap();
			  read_loop ++;
		  }
		  if ( read_loop >= MAX_READ)
		  {
			  read_loop = 0;
			  stage = 1;
		  }
		  break;
	  case 1:	// Change to RAW
		  printf("Command Change to RAW\r\n");
		  SPI_Command(8,command_raw );
		  stage = 2;
		  break;
	  case 2:	// Read 10 RAW
		  if (g_int_pending )
		  {
			  g_int_pending = 0;
			  SPI_Test_Read_RAW();
			  read_loop ++;
		  }
		  if ( read_loop >= MAX_READ)
		  {
			  read_loop = 0;
			  stage = 3;
		  }
		  break;
	  case 3:	// Change to Bitmap
		  printf("Command Change to Bitmap\r\n");
		  SPI_Command(32,command_bmp );

//		  stage = 4;
 		  stage = 0;
		  printf("STAGE Read Bitmap\r\n");
		  break;
	  default:
		  if ( !paused)
		  {
			  paused = 1;
			  printf("PAUSE\r\n");
		  }
		  break;
	  }
  }
#endif

#if (TEST_MODE == MODE_LOOP_RAW)

  printf("TEST_MODE::MODE_LOOP_RAW\r\n");
  printf("Read Bitmap/Change to RAW then read dump\r\n");

  #define MAX_READ	10
  int read_loop = 0;
  int stage = 0;

  uint8_t command_raw[] =
  {0xA0,0x4C,0x45,0x4E,0x20,0x9F,0x00,0x00};
  uint8_t command_bmp[] =
  {
	0xA0,0x4C,0x45,0x4E,0x08,0x87,0x00,0x00,
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
	0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,
	0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
  };

  uint8_t read_raw[] =
  {	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  };

  printf("STAGE LOOP 2 (RAW)Test\r\n");

  int paused = 0;

  while (1)
  {
	  switch (stage )
	  {
	  case 0:	// Read 10 bitamp
		  if (g_int_pending )
		  {
			  g_int_pending = 0;
			  SPI_Test_Read_Bitmap();
			  read_loop ++;
		  }
		  if ( read_loop >= MAX_READ)
		  {
			  read_loop = 0;
			  stage = 1;
		  }
		  break;
	  case 1:	// Change to RAW
		  printf("Command Change to RAW\r\n");
		  SPI_Command(8,command_raw );
		  stage = 2;
		  break;
	  case 2:	// Read 10 RAW
		  if (g_int_pending )
		  {
			  g_int_pending = 0;
			  SPI_Test_Read_RAW();
			  read_loop ++;
		  }
		  break;
	  default:
		  if ( !paused)
		  {
			  paused = 1;
			  printf("PAUSE\r\n");
		  }
		  break;
	  }
  }
#endif

#if 0


  HAL_Delay(5000);

  while (1)
  {

	  HAL_Delay(1000);

//	  SPI_Test_Auto();
	  SPI_Test();
  }


#else
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  volatile uint8_t g_spi_busy = 0;
  while (1)
   {
#if (TRIGGER_BY == TRIGGER_SOURCE_TIMER)
	  HAL_Delay(500);
	  g_int_pending = 1;
#endif
	    if (g_int_pending && !g_spi_busy)
       {
           g_int_pending = 0;

           HAL_Delay(500);
           g_spi_busy = 1;
           /* optional: LED toggle for debug */
           HAL_GPIO_TogglePin(LD4_GPIO_Port, LD4_Pin);

           printf("INT\r\n");

           /* let slave settle a little after INT */
           delay_us(5);

           /* do exactly one SPI transaction per INT */
 //          SPI_Test();
           g_spi_busy = 0;
       }

       if (g_btn_pending)
       {
           g_btn_pending = 0;
           printf("BTN event\r\n");
       }
   }
#endif
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

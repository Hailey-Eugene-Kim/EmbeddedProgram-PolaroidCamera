#include "2440addr.h"
#include "device_driver.h"
#include "macro.h"

#include "./Images/BGimage.h"
#include "./Images/frame.h"
#include "./Images/frameblack.h"

/* 5:5:5:I Color Definition */

#define BLACK	0x0000
#define WHITE	0xfffe
#define BLUE	0x003e
#define GREEN	0x07c0
#define RED		0xf800

#include <stdlib.h>

void Camera_Polling_Test(void);
void Camera_Interrupt_Test(void);
void Polaroid_Camera_Program(void);
void Intro(void);

static void (*Func_Arr[])(void) =
{
	Camera_Polling_Test,
	Camera_Interrupt_Test,
	Polaroid_Camera_Program
};

extern char * Cam_Frame_Buf;

void Main(void)
{
	int r;

	Nand_Init();
	Uart_Init(115200);
	Lcd_Graphic_Init();
	Key_ISR_Init();
	Key_ISR_Enable(1);

	Uart_Printf("Camera Interface Test\n");

	CAM_Camera_Init();
	CAM_Module_Init();
	CAM_Sensor_Enable(1);

	Uart_Printf("ID= %x%x\n", CAM_IIC_Read(0),CAM_IIC_Read(1));
	CAM_Sensor_Enable(0);

	Cam_Frame_Buf = (char *)Free_Memory_BASE;

	for(;;)
	{
		Lcd_Control_Brightness(5);
		Lcd_Select_Buffer_Mode(LAYER_MODE);
	 	Lcd_Select_Draw_Frame_Buffer(0);
	 	Lcd_Select_Display_Frame_Buffer(0);
		Lcd_Set_Trans_Mode(0);
		Lcd_Set_Shape_Mode(0,0);
		Lcd_Set_Shape_Back_Image_Mode(0, (void *)0);
	 	Lcd_Clr_Screen(BLACK);
		Lcd_Printf(10,10,BLUE,BLACK,1,1,"Willtek 임베디드 전문가 그룹");

		Uart_Printf("==================================================\n");
		Uart_Printf("GBOX WT2440L Camera I/F Test     (c)Willtek Corp. \n");
		Uart_Printf("==================================================\n");
		Uart_Printf("[1] Camera Polling Test                           \n");
		Uart_Printf("[2] Camera Interrupt Test                         \n");
		Uart_Printf("[3] Polaroid Camera Program                       \n");
		Uart_Printf("==================================================\n");
		Uart_Printf(">> ");

		r = Uart_GetIntNum()-1;
		Uart_Get_Pressed();

		if((unsigned int)r >= (sizeof(Func_Arr)/sizeof(Func_Arr[0])))
		{
			Uart_Printf("Wrong Number!\n");
			continue;
		}

		Func_Arr[r]();

		Uart_Printf("Test Finished: Press any key to continue \n"); //uart에서 입력 받으려고 하는데 종료
		Uart_Get_Char();
	}
}

extern volatile int Key_value;

int Cam_Width  = 320;
int Cam_Height = 240;
int Cam_Exp_Mode = 0;


static int frm = 0;

void Toggle_Frame(void)
{
	if(frm) frm = 0;
	else frm = 1;
}

void Camera_Polling_Test(void)
{
	unsigned short *q;

	Uart_Printf("\n\nCamera Test (Polling)\n");

	Lcd_Select_Draw_Frame_Buffer(frm);
	Lcd_Select_Display_Frame_Buffer(frm);
	Lcd_Clr_Screen(RED);

	{
		Uart_Printf("Camera test: Press any key for next test\n");
		Uart_Printf("UP: Pause\n");
		Uart_Printf("DOWN: Play\n");

		CAM_Interrupt_Enable(0);
		Key_ISR_Enable(1);

		Cam_Width  = 320;
		Cam_Height = 240;

		CAM_Capture_Run(0,0,0);
		CAM_Capture_Run(1, Cam_Width, Cam_Height);
		CAM_XY_Flip(1,0);

		Lcd_Set_Shape_Mode(1, RED);

		do
		{
			if((q = CAM_Check_Image_Ready())!=0)
			{
				Lcd_Select_Draw_Frame_Buffer(frm);
				Lcd_Draw_Cam_Image(0, 0, (void *)q, Cam_Width, Cam_Height);
				Lcd_Printf(10,10,GREEN,RED,1,1,"(주)윌텍 교육사업부");
				Lcd_Select_Display_Frame_Buffer(frm);
				Toggle_Frame();
			}

			if(Key_value)
			{
				if(Key_value == 1) CAM_Capture_Pause();
				else if(Key_value == 3) CAM_Capture_Restart();
				Key_value = 0;
			}

		}while(!Uart_Get_Pressed());

		Key_ISR_Enable(0);
		Lcd_Set_Shape_Mode(0,0);
	}
}


void Camera_Interrupt_Test(void)
{
	unsigned short *q;

  	Uart_Printf("\n\nCamera Test (Interrupt)\n");

	{
		Uart_Printf("Camera test: Press any key for next test\n");
		Uart_Printf("UP: Pause\n");
		Uart_Printf("DOWN: Play\n");

		Cam_Width = 320;
		Cam_Height= 240;

		CAM_Set_Capture_Status(0);
		CAM_Capture_Run(0,0,0);
		CAM_Capture_Run(1, Cam_Width, Cam_Height);
		CAM_XY_Flip(0,1);

		Key_ISR_Enable(1);
		CAM_Interrupt_Enable(1);

		do
		{
			if(CAM_Get_Capture_Status() == 2)
			{
				CAM_Set_Capture_Status(0);
				q = CAM_Get_Image_Address();
				Lcd_Select_Draw_Frame_Buffer(frm);
				Lcd_Draw_Cam_Image(0, 0, (void *)q, Cam_Width, Cam_Height);
				Lcd_Printf(10,10,GREEN,RED,1,1,"(주)윌텍 교육사업부");
				Lcd_Select_Display_Frame_Buffer(frm);
				Toggle_Frame();
			}

			if(Key_value)
			{
				if(Key_value == 1) CAM_Capture_Pause();
				else if(Key_value == 3) CAM_Capture_Restart();
				Key_value = 0;
			}

		}while(!Uart_Get_Pressed());

		CAM_Capture_Run(0,0,0);
		CAM_Capture_Run(0, Cam_Width, Cam_Height);
		CAM_Interrupt_Enable(0);
		Key_ISR_Enable(0);
	}
}

void Polaroid_Camera_Program (void)
{
	Intro();

	//Polling Test

	unsigned short *q;
	unsigned int * buf = malloc(320 * 240 * sizeof(unsigned int));

	Uart_Printf("\n\nPolaroid Camera Program Start\n");

	{
		//Uart_Printf("Camera test: Press any key for next test\n");
		Uart_Printf("UP: Take Photo\n");
		Uart_Printf("DOWN: Try Again\n");

		CAM_Interrupt_Enable(0);
		Key_ISR_Enable(1);

		Cam_Width  = 320;
		Cam_Height = 240;

		CAM_Capture_Run(0,0,0);
		CAM_Capture_Run(1, Cam_Width, Cam_Height);
		CAM_XY_Flip(1,0);

		Lcd_Set_Shape_Mode(1, RED);

		do
		{
			if((q = CAM_Check_Image_Ready())!=0)
			{
				Lcd_Select_Draw_Frame_Buffer(frm);
				Lcd_Draw_Cam_Image(0, 0, (void *)q, Cam_Width, Cam_Height);
				//Lcd_Printf(10,10,BLUE,RED,1,1,"UP 버튼을 눌러 사진 촬영");
				Lcd_Select_Display_Frame_Buffer(frm);
				Toggle_Frame();
			}

			if(Key_value)
			{
				int flag = 0;
				if(Key_value == 1)
				{
					CAM_Capture_Pause();
					Timer4_Delay(1300);

					//폴라로이드 필터
					Lcd_Set_Shape_Mode(1, BLUE);
					Lcd_Draw_BMP(0, 0, frameblack);

					flag = 1;

					Uart_Printf("Right: Add Text & Save Photo\n");
				}

				//텍스트 입력 & 사진 저장
				else if ((Key_value == 4)&&(flag = 1))
				{
				Uart_Printf("폴라로이드에 남길 메모를 입력하세요\n");

				char text[30];
				Uart_GetString(text);
				Lcd_Printf(15,210,BLACK,BLUE,1,1,(char *)text);

				//Lcd_Clr_Screen(WHITE);
				//Lcd_Printf(100,80,BLACK,WHITE,2,2,"Photo Saved");
				Timer4_Delay(1300);

				//unsigned int * buf = malloc(320 * 240 * sizeof(unsigned int));
				Lcd_Save_Backup_24bpp(0, 0, 320, 240, buf);
				Lcd_Clr_Screen(WHITE);
				Timer4_Delay(1300);
				Lcd_Draw_Backup_24bpp(0, 0, 320, 240, buf);
				Uart_Printf("Photo Saved!\n");
				Uart_Printf("DOWN: Try Again\n");
				Uart_Printf("Left: Go to Gallery\n");

				//if (Key_value == 2)
				//{
					//Lcd_Draw_Backup_24bpp(0, 0, 180, 120, buf);
				//}


				flag = 0;
				}

				else if(Key_value == 3) CAM_Capture_Restart();

				else if(Key_value == 2)
				{
					//Lcd_Clr_Screen(BLACK); //가려질 수 있으니까 나중에 추가
					Lcd_Draw_Backup_24bpp(0, 0, 320, 240, buf);
					Timer4_Delay(1300);
				}

				Key_value = 0;
			}

		}while(!Uart_Get_Pressed());

		Key_ISR_Enable(0);
		Lcd_Set_Shape_Mode(0,0);
	}

}

void Intro(void)
{
	Lcd_Draw_BMP(0, 0, BGimage);
	Lcd_Printf(10,30,RED,BLACK,2,2,"Polaroid Camera");
	Lcd_Printf(10,180,WHITE,BLACK,1,1,"사진을 찍으면 폴라로이드로 저장");
	Lcd_Printf(10,200,WHITE,BLACK,1,1,"원하는 문구와 함께 기록하세요");
	Timer4_Delay(1300);
}

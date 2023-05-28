/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include <asf.h>
#include <string.h>
#include "ili9341.h"
#include "lvgl.h"
#include "touch/touch.h"
#include "conf_board.h"
#include "logo_bike.h"
#include "icon_vel.h"

// Sensor magnético
#define SENSOR_MAG_PIO      PIOA
#define SENSOR_MAG_PIO_ID   ID_PIOA
#define SENSOR_MAG_IDX      19
#define SENSOR_MAG_IDX_MASK (1 << SENSOR_MAG_IDX)

//PAUSE - CAM - P0
#define CAM_1_PIO			PIOA
#define CAM_1_PIO_ID		ID_PIOA
#define CAM_1_PIO_IDX		3
#define CAM_1_PIO_IDX_MASK (1u << CAM_1_PIO_IDX)

#define SERVO_PIO    PIOA
#define SERVO_PIO_ID ID_PIOA
#define SERVO_PIO_IDX 4
#define SERVO_IDX_MASK (1 << SERVO_PIO_IDX)

lv_obj_t * labelLogo;
lv_obj_t * labelClock;
lv_obj_t * labelVelocidade;
lv_obj_t * labelDistancia;
lv_obj_t * labelTempoGasto;
lv_obj_t * labelUniVel;
lv_obj_t * labelUniDist;
lv_obj_t * labelUniTemp;
lv_obj_t * Dist;
lv_obj_t * Vel;
lv_obj_t * Temp;



// Horario
typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

static lv_obj_t * tela1;
static lv_obj_t * tela2;
//Semaforo
SemaphoreHandle_t xSemaphoreClock;

/************************************************************************/
/* LCD / LVGL                                                           */
/************************************************************************/


#define LV_HOR_RES_MAX          (240)
#define LV_VER_RES_MAX          (320)

/*A static or global variable to store the buffers*/
static lv_disp_draw_buf_t disp_buf;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
static lv_disp_drv_t disp_drv;          /*A variable to hold the drivers. Must be static or global.*/
static lv_indev_drv_t indev_drv;
lv_obj_t * logoBike;
lv_obj_t * labelPlay;
LV_IMG_DECLARE(logo_bike);
LV_FONT_DECLARE(dseg40);
LV_FONT_DECLARE(dseg22);
LV_FONT_DECLARE(dseg30);
LV_FONT_DECLARE(dseg20);



/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_LCD_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_LCD_STACK_PRIORITY            (tskIDLE_PRIORITY)

#define TASK_PULSE_STACK_SIZE (4096 / sizeof(portSTACK_TYPE))
#define TASK_PULSE_STACK_PRIORITY (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,  signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);
static void handler_play(lv_event_t * e);
extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	for (;;) {	}
}
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);

extern void vApplicationIdleHook(void) { }

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	configASSERT( ( volatile void * ) NULL );
}
void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);

SemaphoreHandle_t xSemaphorePulse;
SemaphoreHandle_t xSemaphoreAlerta;


/************************************************************************/
/* Callbacks                                                            */
/************************************************************************/

void pulse_callback(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xSemaphorePulse, &xHigherPriorityTaskWoken);
}

void cam_callback(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xSemaphoreAlerta, &xHigherPriorityTaskWoken);
}

void servo0graus()              //Posiciona o servo em 0 graus
{

	pio_set(SERVO_PIO, SERVO_IDX_MASK);  //pulso do servo
	delay_us(600);     //1.5ms
	pio_clear(SERVO_PIO, SERVO_IDX_MASK);   //completa periodo do servo
	for(int i=0;i<32;i++)delay_us(600);
	// 20ms = 20000us
	// 20000us - 600us = 19400us
	// 19400/600 = ~~32
} //end servo0graus

void servo90graus()             //Posiciona o servo em 90 graus
{
	pio_set(SERVO_PIO, SERVO_IDX_MASK);  //pulso do servo
	delay_us(1500);     //1.5ms
	pio_clear(SERVO_PIO, SERVO_IDX_MASK);   //completa periodo do servo
	for(int i=0;i<12;i++)delay_us(1500);
	// 20ms = 20000us
	// 20000us - 1500us = 18500us
	// 18500/1500 = ~~12
} //end servo0graus

void servo180graus()             //Posiciona o servo em 90 graus
{
	pio_set(SERVO_PIO, SERVO_IDX_MASK);  //pulso do servo
	delay_us(2400);     //1.5ms
	pio_clear(SERVO_PIO, SERVO_IDX_MASK);   //completa periodo do servo
	for(int i=0;i<7;i++)delay_us(2400);
	// 20ms = 20000us
	// 20000us - 1500us = 18500us
	// 18500/1500 = ~~12
} //end servo0graus

/************************************************************************/
/* RTC                                                                 */
/************************************************************************/

void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	/* seccond tick */
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		// o código para irq de segundo vem aqui
		xSemaphoreGiveFromISR(xSemaphoreClock, 0);
	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		// o código para irq de alame vem aqui
	}

	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}


void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type) {
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc,  irq_type);
}

/************************************************************************/
/* lvgl                                                                 */
/************************************************************************/

static void event_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		printf("Clicou\n");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void handler_config(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		lv_scr_load(tela2);
		printf("config\n");
	}
}

static void handler_home(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		lv_scr_load(tela1);
		printf("home\n");
	}
}


volatile int play =0;
static void handler_pause(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * labelPause;

	if(code == LV_EVENT_CLICKED) {
		play = !play;
		lv_obj_t *pause_ = lv_btn_create(lv_scr_act());
		lv_obj_add_event_cb(pause_, handler_play, LV_EVENT_ALL, NULL);
		//lv_obj_add_flag(pause_play, LV_OBJ_FLAG_CHECKABLE);
		lv_obj_align_to(pause_, logoBike, LV_ALIGN_BOTTOM_MID, -10, 200);
		lv_obj_set_width(pause_, 50);
		lv_obj_set_height(pause_, 50);
		
		lv_obj_set_style_bg_color(pause_, lv_color_black(), LV_STATE_DEFAULT); // Define a cor de fundo como a cor do LCD
		lv_obj_set_style_radius(pause_, LV_RADIUS_CIRCLE, LV_STATE_DEFAULT); // Define o raio do canto como circular
		
		labelPause = lv_label_create(pause_);
		lv_obj_set_style_text_color(labelPause, lv_color_white(), LV_STATE_DEFAULT); // Define a cor do texto como branco
		lv_label_set_text(labelPause, LV_SYMBOL_PAUSE);
		lv_obj_center(labelPause);
		printf(" play : %d\n",play);
	}
	
}
static void handler_play(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * labelPause;

	if(code == LV_EVENT_CLICKED) {
		play = !play;
		// pause/play
		lv_obj_t *play = lv_btn_create(lv_scr_act());
		lv_obj_add_event_cb(play, handler_pause, LV_EVENT_ALL, NULL);
		//lv_obj_add_flag(pause_play, LV_OBJ_FLAG_CHECKABLE);
		lv_obj_align_to(play, logoBike, LV_ALIGN_BOTTOM_MID, -10, 200);
		lv_obj_set_width(play, 50);
		lv_obj_set_height(play, 50);
		
		lv_obj_set_style_bg_color(play, lv_color_black(), LV_STATE_DEFAULT); // Define a cor de fundo como a cor do LCD
		lv_obj_set_style_radius(play, LV_RADIUS_CIRCLE, LV_STATE_DEFAULT); // Define o raio do canto como circular
		
		labelPlay = lv_label_create(play);
		lv_obj_set_style_text_color(labelPlay, lv_color_white(), LV_STATE_DEFAULT); // Define a cor do texto como branco
		lv_label_set_text(labelPlay, LV_SYMBOL_PLAY);
		lv_obj_center(labelPlay);
	}
	
}

volatile int refresh =0;
static void handler_refresh(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		refresh = 1;
	}
	
}

static void handler_raio(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	if(code == LV_EVENT_VALUE_CHANGED) {
		char buf[32];
		lv_roller_get_selected_str(obj, buf, sizeof(buf));
		printf("Selected month: %s\n", buf);
		//RAIO = atoi(buf)*0.0253/2;
	}
}

void lv_ex_btn_1(void) {
	lv_obj_t * label;
	lv_obj_t * labelConfig;
	lv_obj_t * labelRefresh;
	lv_obj_t * labelHome;
	lv_obj_t * labelSelect;
	
	//Estiliza o botão
	static lv_style_t style;
	lv_style_init(&style);
	lv_style_set_bg_color(&style, lv_color_white());

// 	lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
// 	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
// 	lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);
// 
// 	label = lv_label_create(btn1);
// 	lv_label_set_text(label, "Corsi");
// 	lv_obj_center(label);

// 	lv_obj_t * btn2 = lv_btn_create(lv_scr_act());
// 	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
// 	lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
// 	lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
// 	lv_obj_set_height(btn2, LV_SIZE_CONTENT);
// 
// 	label = lv_label_create(btn2);
// 	lv_label_set_text(label, "Toggle");
// 	lv_obj_center(label);
	
	//LOGO
// 	lv_obj_t *imgLogo = lv_img_create(lv_scr_act());
// 	lv_img_set_src(imgLogo, &logo_bike);
// 	lv_obj_set_height(imgLogo, 86);
// 	lv_obj_set_width(imgLogo, 190);
// 	lv_obj_align(imgLogo, LV_ALIGN_CENTER, 0, -90);
	
	logoBike = lv_img_create(lv_scr_act());
	//lv_obj_t * logoBike = lv_btn_create(tela1);
	lv_img_set_src(logoBike, &logo_bike);
	lv_obj_align(logoBike, LV_ALIGN_CENTER, 0, -120);
	
	//Label Select
	labelSelect = lv_label_create(tela2);
	lv_obj_align(labelSelect,LV_ALIGN_CENTER, 0, -120);
	lv_obj_set_style_text_font(labelSelect, &dseg22, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelSelect, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelSelect, "SELECIONE[R]");
	
// 	lv_obj_t * logoBike2 = lv_img_create(tela2);
// 	//lv_obj_t * logoBike = lv_btn_create(tela1);
// 	lv_img_set_src(logoBike2, &logo_bike);
// 	lv_obj_align(logoBike2, LV_ALIGN_CENTER, 0, -120);
	
	//Label Hora
	labelClock = lv_label_create(lv_scr_act());
	lv_obj_align_to(labelClock, logoBike, LV_ALIGN_CENTER, -15, 60);
	lv_obj_set_style_text_color(labelClock, lv_color_black(), LV_STATE_DEFAULT);
	
	//Label velocidade
	labelVelocidade = lv_label_create(lv_scr_act());
	lv_obj_align_to(labelVelocidade, logoBike, LV_ALIGN_CENTER, -30, 100);
	lv_obj_set_style_text_font(labelVelocidade, &dseg40, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelVelocidade, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelVelocidade, "%02d", 10);
	
	// Label unidade Velocidade
	labelUniVel = lv_label_create(lv_scr_act());
	lv_obj_align_to(labelUniVel, labelVelocidade, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, 0);
	lv_obj_set_style_text_color(labelUniVel, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelUniVel, "Km/h");

	//Label Distancia
	labelDistancia = lv_label_create(lv_scr_act());
	lv_obj_align_to(labelDistancia, logoBike, LV_ALIGN_BOTTOM_LEFT, -20, 145);
	lv_obj_set_style_text_font(labelDistancia, &dseg20, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelDistancia, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelDistancia, "%02d", 30);
	
	// Label unidade Distancia
	labelUniDist = lv_label_create(lv_scr_act());
	lv_obj_align_to(labelUniDist, labelDistancia, LV_ALIGN_OUT_RIGHT_BOTTOM, 0, 5);
	lv_obj_set_style_text_color(labelUniDist, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelUniDist, "Km");
	
	// Label unidade Distancia
	Dist = lv_label_create(lv_scr_act());
	lv_obj_align_to(Dist, labelDistancia, LV_ALIGN_OUT_TOP_MID, 0, 0);
	lv_obj_set_style_text_color(Dist, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(Dist, "Dist.");
	
	
	
	//Label Tempo gasto
		
	labelTempoGasto = lv_label_create(lv_scr_act());
	lv_obj_align_to(labelTempoGasto, logoBike, LV_ALIGN_BOTTOM_RIGHT, -20, 145);
	lv_obj_set_style_text_font(labelTempoGasto, &dseg20, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelTempoGasto, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelTempoGasto, "%02d:%02d", 0,0);
	
	// Label unidade velocidade
// 	labelUniTemp = lv_label_create(lv_scr_act());
// 	lv_obj_align_to(labelUniTemp, labelTempoGasto, LV_ALIGN_OUT_RIGHT_BOTTOM, 0, 5);
// 	lv_obj_set_style_text_color(labelUniTemp, lv_color_black(), LV_STATE_DEFAULT);
// 	lv_label_set_text_fmt(labelUniTemp, "min");
	
	// Label Vel.
	Temp = lv_label_create(lv_scr_act());
	lv_obj_align_to(Temp, labelTempoGasto, LV_ALIGN_OUT_TOP_MID, -5, 0);
	lv_obj_set_style_text_color(Temp, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(Temp, "Tempo");
	
	
	
	//Label velocidade media
	
	labelVelocidade = lv_label_create(lv_scr_act());
	lv_obj_align_to(labelVelocidade, logoBike, LV_ALIGN_BOTTOM_MID, -20, 145);
	lv_obj_set_style_text_font(labelVelocidade, &dseg20, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelVelocidade, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelVelocidade, "%02d", 20);
	
	// Label unidade velocidade
	labelUniVel = lv_label_create(lv_scr_act());
	lv_obj_align_to(labelUniVel, labelVelocidade, LV_ALIGN_OUT_RIGHT_BOTTOM, 0, 5);
	lv_obj_set_style_text_color(labelUniVel, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelUniVel, "Km/h");
	
	// Label Vel.
	Vel = lv_label_create(lv_scr_act());
	lv_obj_align_to(Vel, labelVelocidade, LV_ALIGN_OUT_TOP_MID, 0, 0);
	lv_obj_set_style_text_color(Vel, lv_color_black(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(Vel, "VelMed.");
	
	//Icon vel
	lv_obj_t * vel = lv_img_create(lv_scr_act());
	lv_img_set_src(vel, &icon_vel);
	lv_obj_align_to(vel,labelVelocidade, LV_ALIGN_BOTTOM_MID, 10, 30);
	
	// configuracao
	lv_obj_t * config = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(config, handler_config, LV_EVENT_ALL, NULL);
	lv_obj_align_to(config, logoBike, LV_ALIGN_BOTTOM_MID, 40, 200);
	lv_obj_set_width(config, 50);
	lv_obj_set_height(config, 50);
	lv_obj_set_style_bg_color(config, lv_color_black(), LV_STATE_DEFAULT); // Define a cor de fundo como a cor do LCD
	lv_obj_set_style_radius(config, LV_RADIUS_CIRCLE, LV_STATE_DEFAULT); // Define o raio do canto como circular
	lv_obj_add_style(config, &style, 0); // ESTILO
	labelConfig = lv_label_create(config);
	lv_obj_set_style_text_color(labelConfig, lv_color_white(), LV_STATE_DEFAULT); // Define a cor do texto como branco
	lv_label_set_text(labelConfig, LV_SYMBOL_SETTINGS);
	lv_obj_center(labelConfig);
	
	// pause/play
    lv_obj_t *play = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(play, handler_pause, LV_EVENT_ALL, NULL);
    //lv_obj_add_flag(pause_play, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_align_to(play, logoBike, LV_ALIGN_BOTTOM_MID, -10, 200);
    lv_obj_set_width(play, 50);
    lv_obj_set_height(play, 50);
    
    lv_obj_set_style_bg_color(play, lv_color_black(), LV_STATE_DEFAULT); // Define a cor de fundo como a cor do LCD
    lv_obj_set_style_radius(play, LV_RADIUS_CIRCLE, LV_STATE_DEFAULT); // Define o raio do canto como circular
    
    labelPlay = lv_label_create(play);
    lv_obj_set_style_text_color(labelPlay, lv_color_white(), LV_STATE_DEFAULT); // Define a cor do texto como branco
    lv_label_set_text(labelPlay, LV_SYMBOL_PLAY);
    lv_obj_center(labelPlay);
	
	
	
	// refresh
	lv_obj_t * refresh = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(refresh, handler_refresh, LV_EVENT_ALL, NULL);
	lv_obj_align_to(refresh, logoBike, LV_ALIGN_BOTTOM_MID, -60, 200);
	lv_obj_set_width(refresh, 50);
	lv_obj_set_height(refresh, 50);

	lv_obj_set_style_bg_color(refresh, lv_color_black(), LV_STATE_DEFAULT); // Define a cor de fundo como a cor do LCD
	lv_obj_set_style_radius(refresh, LV_RADIUS_CIRCLE, LV_STATE_DEFAULT); // Define o raio do canto como circular

	labelRefresh = lv_label_create(refresh);
	lv_obj_set_style_text_color(labelRefresh, lv_color_white(), LV_STATE_DEFAULT); // Define a cor do texto como branco
	lv_label_set_text(labelRefresh, LV_SYMBOL_REFRESH);
	lv_obj_center(labelRefresh);
	
	
	// pause/play
	lv_obj_t *home = lv_btn_create(tela2);
	lv_obj_add_event_cb(home, handler_home, LV_EVENT_ALL, NULL);
	//lv_obj_add_flag(pause_play, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_align_to(home, logoBike, LV_ALIGN_BOTTOM_MID, -10, 200);
	lv_obj_set_width(home, 50);
	lv_obj_set_height(home, 50);
	
	lv_obj_set_style_bg_color(home, lv_color_black(), LV_STATE_DEFAULT); // Define a cor de fundo como a cor do LCD
	lv_obj_set_style_radius(home, LV_RADIUS_CIRCLE, LV_STATE_DEFAULT); // Define o raio do canto como circular
	
	labelHome = lv_label_create(home);
	lv_obj_set_style_text_color(labelHome, lv_color_white(), LV_STATE_DEFAULT); // Define a cor do texto como branco
	lv_label_set_text(labelHome, LV_SYMBOL_HOME);
	lv_obj_center(labelHome);
	
		
	lv_obj_t *roller1 = lv_roller_create(tela2);
	lv_roller_set_options(roller1,
	"12\n"
	"14\n"
	"16\n"
	"18\n"
	"20\n"
	"22\n"
	"24\n"
	"26",
	LV_ROLLER_MODE_INFINITE);

	lv_roller_set_visible_row_count(roller1, 5);
	lv_obj_align(roller1, LV_ALIGN_CENTER, 0, 0);
	lv_obj_add_event_cb(roller1, handler_raio, LV_EVENT_ALL, NULL);
	lv_roller_set_selected(roller1, 4, LV_ANIM_ON);
	lv_obj_set_width(roller1, 200);
	lv_obj_set_height(roller1, 200);
	
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_lcd(void *pvParameters) {
	int px, py;

	// 2 telas
	tela1  = lv_obj_create(NULL);
	tela2  = lv_obj_create(NULL);
	lv_obj_set_style_bg_color(tela1, lv_color_white(), LV_PART_MAIN );

	lv_scr_load(tela1);
	lv_ex_btn_1();
	for (;;)  {
		lv_tick_inc(50);
		lv_task_handler();
		vTaskDelay(50);
	}
}




int volatile flag_rtt = 0;
static void task_pulses(void *pvParameters) {
	
	
	flag_rtt = 1; // força flag = 1 para inicializar alarme.
	float deltaT = 2.5;
	float vm = 0;
	float vm_ = 0;
	float am = 0;
	float N = 0;
	float R = 0.508/2;
	float K = 100;
	int choice = 0;
	
	for (;;)  {
		if(flag_rtt) {
			printf("N = %d\n", (int) N);
			vm = ((2.0 * 3.1415926535 * N * R * K) / deltaT);
			am = (vm - vm_) / deltaT;
			printf("vm = %d cm/s\n", (int) vm);
			printf("am = %d cm/s^2\n", (int) am);
			
			RTT_init(10, 25, RTT_MR_ALMIEN);  // inicializa rtt com alarme
			flag_rtt = 0;
			N = 0;
			vm_ = vm;
			choice = (choice + 1) % 3;
		}
		
		if(xSemaphoreTake(xSemaphorePulse, 1)){
			//printf("Pulso! \n");
			N++;
		}
		if(xSemaphoreTake(xSemaphoreAlerta, 1)){
			printf("Cam! \n");
			//N++;
		}
		
		
		
	}
}

static void task_rtc(void *pvParameters){
	calendar rtc_initial = {2023, 4, 19, 12, 15, 0 ,0};

	/** Configura RTC */
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_SECEN);
	
	/* Leitura do valor atual do RTC */
	uint32_t current_hour, current_min, current_sec;
	uint32_t current_year, current_month, current_day, current_week;
	rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
	rtc_get_date(RTC, &current_year, &current_month, &current_day, &current_week);
	int segundo =0;
	int minuto =0;
	for (;;)  {
		if (xSemaphoreTake(xSemaphoreClock, portMAX_DELAY) == pdTRUE) {
			// rtc_get_date(RTC, &current_year, &current_month, &current_day, &current_week);
			rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
			lv_label_set_text_fmt(labelClock, "%02d:%02d:%02d", current_hour, current_min,current_sec);
			
			if(play){
				if(segundo < 60){
					segundo++;
					lv_label_set_text_fmt(labelTempoGasto, "%02d:%02d", minuto,segundo);

				}
				else{
					segundo = 0;
					minuto++;
					lv_label_set_text_fmt(labelTempoGasto, "%02d:%02d", minuto,segundo);
				}

			}
			if (refresh)
			{
				segundo = 0;
				minuto = 0;
				refresh =0;
				lv_label_set_text_fmt(labelTempoGasto, "%02d:%02d", minuto,segundo);
			}
		}
	}
}

/************************************************************************/
/* configs                                                              */
/************************************************************************/

static void configure_lcd(void) {
	/**LCD pin configure on SPI*/
	pio_configure_pin(LCD_SPI_MISO_PIO, LCD_SPI_MISO_FLAGS);  //
	pio_configure_pin(LCD_SPI_MOSI_PIO, LCD_SPI_MOSI_FLAGS);
	pio_configure_pin(LCD_SPI_SPCK_PIO, LCD_SPI_SPCK_FLAGS);
	pio_configure_pin(LCD_SPI_NPCS_PIO, LCD_SPI_NPCS_FLAGS);
	pio_configure_pin(LCD_SPI_RESET_PIO, LCD_SPI_RESET_FLAGS);
	pio_configure_pin(LCD_SPI_CDS_PIO, LCD_SPI_CDS_FLAGS);
	
	ili9341_init();
	ili9341_backlight_on();
}

static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT,
	};

	/* Configure console UART. */
	stdio_serial_init(CONSOLE_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	setbuf(stdout, NULL);
}

/************************************************************************/
/* port lvgl                                                            */
/************************************************************************/

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
	ili9341_set_top_left_limit(area->x1, area->y1);   ili9341_set_bottom_right_limit(area->x2, area->y2);
	ili9341_copy_pixels_to_screen(color_p,  (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
	
	/* IMPORTANT!!!
	* Inform the graphics library that you are ready with the flushing*/
	lv_disp_flush_ready(disp_drv);
}

void my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data) {
	int px, py, pressed;
	
	if (readPoint(&px, &py))
		data->state = LV_INDEV_STATE_PRESSED;
	else
		data->state = LV_INDEV_STATE_RELEASED; 
	
	  data->point.x = py;
	  data->point.y = 320 - px;
}

void configure_lvgl(void) {
	lv_init();
	lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);
	
	lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
	disp_drv.draw_buf = &disp_buf;          /*Set an initialized buffer*/
	disp_drv.flush_cb = my_flush_cb;        /*Set a flush callback to draw to the display*/
	disp_drv.hor_res = LV_HOR_RES_MAX;      /*Set the horizontal resolution in pixels*/
	disp_drv.ver_res = LV_VER_RES_MAX;      /*Set the vertical resolution in pixels*/

	lv_disp_t * disp;
	disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
	
	/* Init input on LVGL */
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = my_input_read;
	lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);
}

static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	/* config NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	/* Enable RTT interrupt */
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
	rtt_enable_interrupt(RTT, rttIRQSource);
	else
	rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	
}


void RTT_Handler(void) {
	uint32_t ul_status;
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		flag_rtt = 1;
	}
}

static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
}

void init_sensores(void){
	
	pmc_enable_periph_clk(SENSOR_MAG_PIO_ID);
	pio_configure(SENSOR_MAG_PIO, PIO_INPUT, SENSOR_MAG_IDX_MASK, PIO_PULLUP);
	pio_handler_set(SENSOR_MAG_PIO,
					SENSOR_MAG_PIO_ID,
					SENSOR_MAG_IDX_MASK,
					PIO_IT_FALL_EDGE,
					pulse_callback);
	// Ativa interrupção e limpa primeira IRQ gerada na ativacao
	pio_enable_interrupt(SENSOR_MAG_PIO, SENSOR_MAG_IDX_MASK);
	pio_get_interrupt_status(SENSOR_MAG_PIO);
	
	// Configura NVIC para receber interrupcoes do PIO do botao
	// com prioridade 4 (quanto mais próximo de 0 maior)
	NVIC_EnableIRQ(SENSOR_MAG_PIO_ID);
	NVIC_SetPriority(SENSOR_MAG_PIO_ID, 4); // Prioridade 4
	
	
	
}

void init_cam(){
	pmc_enable_periph_clk(CAM_1_PIO_ID); // PAUSE
	
	pio_configure(CAM_1_PIO, PIO_INPUT, CAM_1_PIO_IDX_MASK, PIO_PULLUP);
	pio_handler_set(CAM_1_PIO,
					CAM_1_PIO_ID,
					CAM_1_PIO_IDX_MASK,
					PIO_IT_FALL_EDGE,
					cam_callback);
					
	// Ativa interrup??o e limpa primeira IRQ gerada na ativacao
	pio_enable_interrupt(CAM_1_PIO, CAM_1_PIO_IDX_MASK);
	pio_get_interrupt_status(CAM_1_PIO);
	
	// Configura NVIC para receber interrupcoes do PIO do botao
	// com prioridade 4 (quanto mais pr?ximo de 0 maior)
	NVIC_EnableIRQ(CAM_1_PIO_ID);
	NVIC_SetPriority(CAM_1_PIO_ID, 4); // Prioridade 4
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/
int main(void) {
	/* board and sys init */
	printf("oi");
	board_init();
	sysclk_init();
	configure_console();
	init_sensores();
	init_cam();
	pmc_enable_periph_clk(SERVO_PIO_ID);
	pio_set_output(SERVO_PIO, SERVO_IDX_MASK, 0, 0, 0);

	/* LCd, touch and lvgl init*/
	configure_lcd();
	configure_touch();
	configure_lvgl();
	ili9341_set_orientation(ILI9341_FLIP_Y | ILI9341_SWITCH_XY);
	lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);


	/* Create task to control oled */
	if (xTaskCreate(task_lcd, "LCD", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create lcd task\r\n");
	}
	/* Create task to control oled */
	if (xTaskCreate(task_rtc, "rtc", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create rtc task\r\n");
	}
	
	/* Create task to control oled */
	if (xTaskCreate(task_pulses, "pulses", TASK_PULSE_STACK_SIZE, NULL, TASK_PULSE_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create rtc task\r\n");
	}
	
	
	
	/* Attempt to create a semaphore. */
	xSemaphoreClock = xSemaphoreCreateBinary();
	if (xSemaphoreClock == NULL)
	printf("falha em criar o semaforo \n");
	
	/* Attempt to create a semaphore. */
	xSemaphorePulse = xSemaphoreCreateBinary();
	if (xSemaphorePulse == NULL)
	printf("falha em criar o semaforo \n");
	
	/* Attempt to create a semaphore. */
	xSemaphoreAlerta = xSemaphoreCreateBinary();
	if (xSemaphoreAlerta == NULL)
	printf("falha em criar o semaforo \n");
	
	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){ }
}

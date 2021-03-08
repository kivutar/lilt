#ifdef  __cplusplus
extern "C" {
#endif 

#include <SDL.h>
#ifdef USE_OGLES11
#include <SDL_opengles.h>
#endif
#include "common.h"
#include "fileio.h"
#include "timer.h"
#include "keyboard.h"
#include "prop.h"
#include "status.h"
#include "joystick.h"
#include "mkcgrom.h"
#include "winx68k.h"
#include "windraw.h"
#include "winui.h"
#include "../x68k/m68000.h" // xxx €³€ì€Ï€€€º€ì€€€é€Ê€¯€Ê€ë€Ï€º
#include "../m68000/m68000.h"
#include "../x68k/memory.h"
#include "mfp.h"
#include "opm.h"
#include "bg.h"
#include "adpcm.h"
#include "mercury.h"
#include "crtc.h"
#include "mfp.h"
#include "fdc.h"
#include "fdd.h"
#include "dmac.h"
#include "irqh.h"
#include "ioc.h"
#include "rtc.h"
#include "sasi.h"
#include "scsi.h"
#include "sysport.h"
#include "bg.h"
#include "palette.h"
#include "crtc.h"
#include "pia.h"
#include "scc.h"
#include "midi.h"
#include "sram.h"
#include "gvram.h"
#include "tvram.h"
#include "mouse.h"

#include "dswin.h"
#include "fmg_wrap.h"

#ifdef RFMDRV
int rfd_sock;
#endif

  //#define WIN68DEBUG

#ifdef WIN68DEBUG
#include "d68k.h"
#endif

//#include "../icons/keropi_mono.xbm"

#define	APPNAME	"Keropi"

extern	WORD	BG_CHREND;
extern	WORD	BG_BGTOP;
extern	WORD	BG_BGEND;
extern	BYTE	BG_CHRSIZE;

#if defined(ANDROID) || TARGET_OS_IPHONE
extern SDL_TouchID touchId;
#endif

const	BYTE	PrgName[] = "Keropi";
const	BYTE	PrgTitle[] = APPNAME;

char	winx68k_dir[MAX_PATH];
char	winx68k_ini[MAX_PATH];

WORD	VLINE_TOTAL = 567;
DWORD	VLINE = 0;
DWORD	vline = 0;

extern	int	SplashFlag;

BYTE DispFrame = 0;
DWORD SoundSampleRate;

unsigned int hTimerID = 0;
DWORD TimerICount = 0;
extern DWORD timertick;
BYTE traceflag = 0;

BYTE ForceDebugMode = 0;
DWORD skippedframes = 0;

static int ClkUsed = 0;
static int FrameSkipCount = 0;
static int FrameSkipQueue = 0;

#ifdef __cplusplus
};
#endif

#if SDL_VERSION_ATLEAST(2, 0, 0)
SDL_Window *sdl_window;
int realdisp_w, realdisp_h;
#endif

void
WinX68k_SCSICheck(void)
{
	static const BYTE SCSIIMG[] = {
		0x00, 0xfc, 0x00, 0x14,			// $fc0000 SCSIµ¯Æ°ÍÑ€Î¥š¥ó¥È¥ê¥¢¥É¥ì¥¹
		0x00, 0xfc, 0x00, 0x16,			// $fc0004 IOCS¥Ù¥¯¥¿ÀßÄê€Î¥š¥ó¥È¥ê¥¢¥É¥ì¥¹(É¬€º"Human"€Î8¥Ð¥€¥ÈÁ°)
		0x00, 0x00, 0x00, 0x00,			// $fc0008 ?
		0x48, 0x75, 0x6d, 0x61,			// $fc000c ¢­
		0x6e, 0x36, 0x38, 0x6b,			// $fc0010 ID "Human68k"	(É¬€ºµ¯Æ°¥š¥ó¥È¥ê¥Ý¥€¥ó¥È€ÎÄŸÁ°)
		0x4e, 0x75,				// $fc0014 "rts"		(µ¯Æ°¥š¥ó¥È¥ê¥Ý¥€¥ó¥È)
		0x23, 0xfc, 0x00, 0xfc, 0x00, 0x2a,	// $fc0016 ¢­		(IOCS¥Ù¥¯¥¿ÀßÄê¥š¥ó¥È¥ê¥Ý¥€¥ó¥È)
		0x00, 0x00, 0x07, 0xd4,			// $fc001c "move.l #$fc002a, $7d4.l"
		0x74, 0xff,				// $fc0020 "moveq #-1, d2"
		0x4e, 0x75,				// $fc0022 "rts"
//		0x53, 0x43, 0x53, 0x49, 0x49, 0x4e,	// $fc0024 ID "SCSIIN"
// ÆâÂ¢SCSI€òON€Ë€¹€ë€È¡¢SASI€ÏŒ«Æ°Åª€ËOFF€Ë€Ê€Ã€Á€ã€Š€é€·€€¡Ä
// €è€Ã€Æ¡¢ID€Ï¥Þ¥Ã¥Á€·€Ê€€€è€Š€Ë€·€Æ€ª€¯¡Ä
		0x44, 0x55, 0x4d, 0x4d, 0x59, 0x20,	// $fc0024 ID "DUMMY "
		0x70, 0xff,				// $fc002a "moveq #-1, d0"	(SCSI IOCS¥³¡Œ¥ë¥š¥ó¥È¥ê¥Ý¥€¥ó¥È)
		0x4e, 0x75,				// $fc002c "rts"
	};

#if 0
	DWORD *p;
#endif
	WORD *p1, *p2;
	int scsi;
	int i;

	scsi = 0;
	for (i = 0x30600; i < 0x30c00; i += 2) {
#if 0 // 4€ÎÇÜ¿ô€Ç€Ï€Ê€€¶ö¿ô¥¢¥É¥ì¥¹€«€é€Î4¥Ð¥€¥ÈÄ¹¥¢¥¯¥»¥¹€ÏMIPS€Ë€ÏÌµÍý
		p = (DWORD *)(&IPL[i]);
		if (*p == 0x0000fc00)
			scsi = 1;
#else
		p1 = (WORD *)(&IPL[i]);
		p2 = p1 + 1;
		// xxx: works only for little endian guys
		if (*p1 == 0xfc00 && *p2 == 0x0000) {
			scsi = 1;
			break;
		}
#endif
	}

	// SCSI¥â¥Ç¥ë€Î€È€­
	if (scsi) {
		ZeroMemory(IPL, 0x2000);		// ËÜÂÎ€Ï8kb
		memset(&IPL[0x2000], 0xff, 0x1e000);	// »Ä€ê€Ï0xff
		memcpy(IPL, SCSIIMG, sizeof(SCSIIMG));	// ¥€¥ó¥Á¥­SCSI BIOS
//		Memory_SetSCSIMode();
	} else {
		// SASI¥â¥Ç¥ë€ÏIPL€¬€œ€Î€Þ€Þž«€š€ë
		memcpy(IPL, &IPL[0x20000], 0x20000);
	}
}

int
WinX68k_LoadROMs(void)
{
	static const char *BIOSFILE[] = {
		"iplrom.dat", "iplrom30.dat", "iplromco.dat", "iplromxv.dat"
	};
	static const char FONTFILE[] = "cgrom.dat";
	static const char FONTFILETMP[] = "cgrom.tmp";
	FILEH fp;
	int i;
	BYTE tmp;

	for (fp = 0, i = 0; fp == 0 && i < NELEMENTS(BIOSFILE); ++i) {
		fp = File_OpenCurDir((char *)BIOSFILE[i]);
		printf("fp:%d (%s)\n",fp,(char *)BIOSFILE[i]);
	}

	if (fp == 0) {
		Error("BIOS ROM ¥€¥á¡Œ¥ž€¬ž«€Ä€«€ê€Þ€»€ó.");
		return FALSE;
	}

	File_Read(fp, &IPL[0x20000], 0x20000);
	File_Close(fp);

	WinX68k_SCSICheck();	// SCSI IPL€Ê€é¡¢$fc0000¡Á€ËSCSI BIOS€òÃÖ€¯

	for (i = 0; i < 0x40000; i += 2) {
		tmp = IPL[i];
		IPL[i] = IPL[i + 1];
		IPL[i + 1] = tmp;
	}

	fp = File_OpenCurDir((char *)FONTFILE);
	if (fp == 0) {
		// cgrom.tmp€¬€¢€ë¡©
		fp = File_OpenCurDir((char *)FONTFILETMP);
		if (fp == 0) {
#if 1
			// ¥Õ¥©¥ó¥ÈÀžÀ® XXX
			printf("¥Õ¥©¥ó¥ÈROM¥€¥á¡Œ¥ž€¬ž«€Ä€«€ê€Þ€»€ó\n");
			return FALSE;
#else
			MessageBox(hWndMain,
				"¥Õ¥©¥ó¥ÈROM¥€¥á¡Œ¥ž€¬ž«€Ä€«€ê€Þ€»€ó.\nWindows¥Õ¥©¥ó¥È€«€é¿·µ¬€ËºîÀ®€·€Þ€¹.",
				"€±€í€Ô¡Œ€Î¥á¥Ã¥»¡Œ¥ž", MB_ICONWARNING | MB_OK);
			SSTP_SendMes(SSTPMES_MAKEFONT);
			make_cgromdat(FONT, FALSE, "£Í£Ó ¥Ž¥·¥Ã¥¯", "£Í£Ó ÌÀÄ«");
			//WinX68k_MakeFont();
			//DialogBox(hInst, MAKEINTRESOURCE(IDD_PROGBAR),
			//		hWndMain, (DLGPROC)MakeFontProc);
			fp = File_CreateCurDir(FONTFILETMP);
			if (fp)
			{
				File_Write(fp, FONT, 0xc0000);
				File_Close(fp);
				return TRUE;
			}
			return TRUE;
#endif
		}
	}
	File_Read(fp, FONT, 0xc0000);
	File_Close(fp);

	return TRUE;
}

int
WinX68k_Reset(void)
{
	OPM_Reset();

	C68k_Reset(&C68K);
/*
	C68k_Set_Reg(&C68K, C68K_A7, (IPL[0x30001]<<24)|(IPL[0x30000]<<16)|(IPL[0x30003]<<8)|IPL[0x30002]);
	C68k_Set_Reg(&C68K, C68K_PC, (IPL[0x30005]<<24)|(IPL[0x30004]<<16)|(IPL[0x30007]<<8)|IPL[0x30006]);
*/
	C68k_Set_AReg(&C68K, 7, (IPL[0x30001]<<24)|(IPL[0x30000]<<16)|(IPL[0x30003]<<8)|IPL[0x30002]);
	C68k_Set_PC(&C68K, (IPL[0x30005]<<24)|(IPL[0x30004]<<16)|(IPL[0x30007]<<8)|IPL[0x30006]);
	Memory_Init();
	CRTC_Init();
	DMA_Init();
	MFP_Init();
	FDC_Init();
	FDD_Reset();
	SASI_Init();
	SCSI_Init();
	IOC_Init();
	SCC_Init();
	PIA_Init();
	RTC_Init();
	TVRAM_Init();
	GVRAM_Init();
	BG_Init();
	Pal_Init();
	IRQH_Init();
	MIDI_Init();
	//WinDrv_Init();

//	C68K.ICount = 0;
	m68000_ICountBk = 0;
	ICount = 0;

	DSound_Stop();
	SRAM_VirusCheck();
	//CDROM_Init();
	DSound_Play();

	return TRUE;
}


int
WinX68k_Init(void)
{
#ifdef PSP
#define MEM_SIZE 0x400000
#else
#define MEM_SIZE 0xc00000
#endif
	IPL = (BYTE*)malloc(0x40000);
	MEM = (BYTE*)malloc(MEM_SIZE);
	FONT = (BYTE*)malloc(0xc0000);

	if (MEM)
		ZeroMemory(MEM, MEM_SIZE);

	if (MEM && FONT && IPL) {
	  	m68000_init();  
		return TRUE;
	} else
		return FALSE;
}

void
WinX68k_Cleanup(void)
{

	if (IPL) {
		free(IPL);
		IPL = 0;
	}
	if (MEM) {
		free(MEM);
		MEM = 0;
	}
	if (FONT) {
		free(FONT);
		FONT = 0;
	}
}

#define CLOCK_SLICE 200
// -----------------------------------------------------------------------------------
//  ¥³¥¢€Î€á€€€ó€ë¡Œ€×
// -----------------------------------------------------------------------------------
void WinX68k_Exec(void)
{
	//char *test = NULL;
	int clk_total, clkdiv, usedclk, hsync, clk_next, clk_count, clk_line=0;
	int KeyIntCnt = 0, MouseIntCnt = 0;
	DWORD t_start = timeGetTime(), t_end;

	if ( Config.FrameRate != 7 ) {
		DispFrame = (DispFrame+1)%Config.FrameRate;
	} else {				// Auto Frame Skip
		if ( FrameSkipQueue ) {
			if ( FrameSkipCount>15 ) {
				FrameSkipCount = 0;
				FrameSkipQueue++;
				DispFrame = 0;
			} else {
				FrameSkipCount++;
				FrameSkipQueue--;
				DispFrame = 1;
			}
		} else {
			FrameSkipCount = 0;
			DispFrame = 0;
		}
	}

	vline = 0;
	clk_count = -ICount;
	clk_total = (CRTC_Regs[0x29] & 0x10) ? VSYNC_HIGH : VSYNC_NORM;
	if (Config.XVIMode == 1) {
		clk_total = (clk_total*16)/10;
		clkdiv = 16;
	} else if (Config.XVIMode == 2) {
		clk_total = (clk_total*24)/10;
		clkdiv = 24;
	} else {
		clkdiv = 10;
	}
	ICount += clk_total;
	clk_next = (clk_total/VLINE_TOTAL);
	hsync = 1;

	do {
		int m, n = (ICount>CLOCK_SLICE)?CLOCK_SLICE:ICount;
//		C68K.ICount = m68000_ICountBk = 0;			// ³ä€ê¹þ€ßÈ¯ÀžÁ°€ËÍ¿€š€Æ€ª€«€Ê€€€È¥À¥á¡ÊCARAT¡Ë

		if ( hsync ) {
			hsync = 0;
			clk_line = 0;
			MFP_Int(0);
			if ( (vline>=CRTC_VSTART)&&(vline<CRTC_VEND) )
				VLINE = ((vline-CRTC_VSTART)*CRTC_VStep)/2;
			else
				VLINE = (DWORD)-1;
			if ( (!(MFP[MFP_AER]&0x40))&&(vline==CRTC_IntLine) )
				MFP_Int(1);
			if ( MFP[MFP_AER]&0x10 ) {
				if ( vline==CRTC_VSTART )
					MFP_Int(9);
			} else {
				if ( CRTC_VEND>=VLINE_TOTAL ) {
					if ( (long)vline==(CRTC_VEND-VLINE_TOTAL) ) MFP_Int(9);		// ¥š¥­¥µ¥€¥Æ¥£¥ó¥°¥¢¥ï¡Œ€È€«¡ÊTOTAL<VEND¡Ë
				} else {
					if ( (long)vline==(VLINE_TOTAL-1) ) MFP_Int(9);			// ¥¯¥ì¥€¥ž¡Œ¥¯¥é¥€¥Þ¡Œ€Ï¥³¥ì€Ç€Ê€€€È¥À¥á¡©
				}
			}
		}

#ifdef WIN68DEBUG
		if (traceflag/*&&fdctrace*/)
		{
			FILE *fp;
			static DWORD oldpc;
			int i;
			char buf[200];
			fp=fopen("_trace68.txt", "a");
			for (i=0; i<HSYNC_CLK; i++)
			{
				m68k_disassemble(buf, C68k_Get_Reg(&C68K, C68K_PC));
//				if (MEM[0xa84c0]) /**test=1; */tracing=1000;
//				if (regs.pc==0x9d2a) tracing=5000;
//				if ((regs.pc>=0x2000)&&((regs.pc<=0x8e0e0))) tracing=50000;
//				if (regs.pc<0x10000) tracing=1;
//				if ( (regs.pc&1) )
//				fp=fopen("_trace68.txt", "a");
//				if ( (regs.pc==0x7176) /*&& (Memory_ReadW(oldpc)==0xff1a)*/ ) tracing=100;
//				if ( (/*((regs.pc>=0x27000) && (regs.pc<=0x29000))||*/((regs.pc>=0x27000) && (regs.pc<=0x29000))) && (oldpc!=regs.pc))
				if (/*fdctrace&&*/(oldpc != C68k_Get_Reg(&C68K, C68K_PC)))
				{
//					//tracing--;
				  fprintf(fp, "D0:%08X D1:%08X D2:%08X D3:%08X D4:%08X D5:%08X D6:%08X D7:%08X CR:%04X\n", C68K.D[0], C68K.D[1], C68K.D[2], C68K.D[3], C68K.D[4], C68K.D[5], C68K.D[6], C68K.D[7], 0/* xxx €È€ê€¢€š€º0 C68K.ccr */);
				  fprintf(fp, "A0:%08X A1:%08X A2:%08X A3:%08X A4:%08X A5:%08X A6:%08X A7:%08X SR:%04X\n", C68K.A[0], C68K.A[1], C68K.A[2], C68K.A[3], C68K.A[4], C68K.A[5], C68K.A[6], C68K.A[7], C68k_Get_Reg(&C68K, C68K_SR) >> 8/* regs.sr_high*/);
					fprintf(fp, "<%04X> (%08X ->) %08X : %s\n", Memory_ReadW(C68k_Get_Reg(&C68K, C68K_PC)), oldpc, C68k_Get_Reg(&C68K, C68K_PC), buf);
				}
				oldpc = C68k_Get_Reg(&C68K, C68K_PC);
//				C68K.ICount = 1;
//				C68k_Exec(&C68K, C68K.ICount);
				C68k_Exec(&C68K, 1);
			}
			fclose(fp);
			usedclk = clk_line = HSYNC_CLK;
			clk_count = clk_next;
		}
		else
#endif
		{
//			C68K.ICount = n;
//			C68k_Exec(&C68K, C68K.ICount);
			C68k_Exec(&C68K, n);
			m = (n-m68000_ICountBk);			// 経過クロック数
//			m = (n-C68K.ICount-m68000_ICountBk);			// ·Ð²á¥¯¥í¥Ã¥¯¿ô
			ClkUsed += m*10;
			usedclk = ClkUsed/clkdiv;
			clk_line += usedclk;
			ClkUsed -= usedclk*clkdiv;
			ICount -= m;
			clk_count += m;
//			C68K.ICount = m68000_ICountBk = 0;
		}

		MFP_Timer(usedclk);
		RTC_Timer(usedclk);
		DMA_Exec(0);
		DMA_Exec(1);
		DMA_Exec(2);

		if ( clk_count>=clk_next ) {
			//OPM_RomeoOut(Config.BufferSize*5);
			//MIDI_DelayOut((Config.MIDIAutoDelay)?(Config.BufferSize*5):Config.MIDIDelay);
			MFP_TimerA();
			if ( (MFP[MFP_AER]&0x40)&&(vline==CRTC_IntLine) )
				MFP_Int(1);
			if ( (!DispFrame)&&(vline>=CRTC_VSTART)&&(vline<CRTC_VEND) ) {
				if ( CRTC_VStep==1 ) {				// HighReso 256dot¡Ê2ÅÙÆÉ€ß¡Ë
					if ( vline%2 )
						WinDraw_DrawLine();
				} else if ( CRTC_VStep==4 ) {		// LowReso 512dot
					WinDraw_DrawLine();				// 1ÁöººÀþ€Ç2²óÉÁ€¯¡Ê¥€¥ó¥¿¡Œ¥ì¡Œ¥¹¡Ë
					VLINE++;
					WinDraw_DrawLine();
				} else {							// High 512dot / Low 256dot
					WinDraw_DrawLine();
				}
			}

			ADPCM_PreUpdate(clk_line);
			OPM_Timer(clk_line);
			MIDI_Timer(clk_line);
#ifndef	NO_MERCURY
			Mcry_PreUpdate(clk_line);
#endif

			KeyIntCnt++;
			if ( KeyIntCnt>(VLINE_TOTAL/4) ) {
				KeyIntCnt = 0;
				Keyboard_Int();
			}
			MouseIntCnt++;
			if ( MouseIntCnt>(VLINE_TOTAL/8) ) {
				MouseIntCnt = 0;
				SCC_IntCheck();
			}
			DSound_Send0(clk_line);

			vline++;
			clk_next  = (clk_total*(vline+1))/VLINE_TOTAL;
			hsync = 1;
		}
	} while ( vline<VLINE_TOTAL );

	if ( CRTC_Mode&2 ) {		// FastClr¥Ó¥Ã¥È€ÎÄŽÀ°¡ÊPITAPAT¡Ë
		if ( CRTC_FastClr ) {	// FastClr=1 ³î€Ä CRTC_Mode&2 €Ê€é œªÎ»
			CRTC_FastClr--;
			if ( !CRTC_FastClr )
				CRTC_Mode &= 0xfd;
		} else {				// FastClr³«»Ï
			if ( CRTC_Regs[0x29]&0x10 )
				CRTC_FastClr = 1;
			else
				CRTC_FastClr = 2;
			TVRAM_SetAllDirty();
			GVRAM_FastClear();
		}
	}

#ifdef PSP
	Joystick_Update(FALSE);
#else
	Joystick_Update(FALSE, SDLK_UNKNOWN);
#endif
	FDD_SetFDInt();
	if ( !DispFrame )
		WinDraw_Draw();
	TimerICount += clk_total;

	t_end = timeGetTime();
	if ( (int)(t_end-t_start)>((CRTC_Regs[0x29]&0x10)?14:16) ) {
		FrameSkipQueue += ((t_end-t_start)/((CRTC_Regs[0x29]&0x10)?14:16))+1;
		if ( FrameSkipQueue>100 )
			FrameSkipQueue = 100;
	}
}

#ifndef __LIBRETRO__
//
// main
//
#ifdef PSP

#include <pspmoduleinfo.h>
#include <psppower.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <pspgu.h>

int exit_flag = 0;

int exit_callback(int arg1, int arg2, void *common)
{
	exit_flag = 1;

	return 0;
}

int CallbackThread(SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid); //SetExitCallback(cbid);

	sceKernelSleepThreadCB(); //KernelPollCallbacks();

	return 0;
}

int SetupCallbacks(void)
{
	int thid;

	thid = sceKernelCreateThread("update_thread", CallbackThread,
				     0x11, 0xFA0, 0, 0);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}

PSP_HEAP_SIZE_KB(-1024);

extern "C" int
SDL_main(int argc, char *argv[])
#else
int pmain(int argc, char *argv[])
#endif
{
#ifndef PSP
	SDL_Event ev;
	SDL_Keycode menu_key_down;
#endif
#if defined(ANDROID) || TARGET_OS_IPHONE
	int vk_cnt = -1;
	int menu_cnt = -1;
	BYTE state;
#endif
	int sdlaudio = -1;
	enum {menu_out, menu_enter, menu_in};
	int menu_mode = menu_out;

#ifdef PSP
	SetupCallbacks();
	scePowerSetClockFrequency(333, 333, 166);

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
#endif

	p6logd("PX68K Ver.%s\n", PX68KVERSTR);

#ifdef RFMDRV
	struct sockaddr_in dest;

	memset(&dest, 0, sizeof(dest));
	dest.sin_port = htons(2151);
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr("127.0.0.1");

	rfd_sock = socket(AF_INET, SOCK_STREAM, 0);
	connect (rfd_sock, (struct sockaddr *)&dest, sizeof(dest));
#endif

	if (set_modulepath(winx68k_dir, sizeof(winx68k_dir)))
		return 1;

	dosio_init();
	file_setcd(winx68k_dir);
    puts(winx68k_dir);

	LoadConfig();

#ifndef NOSOUND
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		p6logd("SDL_Init error\n");		
#endif
		if (SDL_Init(SDL_INIT_VIDEO) < 0) {
			return 1;
		}
#ifndef NOSOUND
	} else {
		sdlaudio = 0;
	}
#endif

#if !SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_WM_SetCaption(APPNAME" SDL", NULL);
#ifndef PSP
        if (SDL_SetVideoMode(FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT, 16, SDL_SWSURFACE) == NULL) {
#else
        if (SDL_SetVideoMode(480, 272, 16, SDL_SWSURFACE) == NULL) {
#endif
		puts("SDL_SetVideoMode() failed");
		return 1;
	}
#else
#ifdef USE_OGLES11
	SDL_DisplayMode sdl_dispmode;
	SDL_GetCurrentDisplayMode(0, &sdl_dispmode);
	p6logd("width: %d height: %d", sdl_dispmode.w, sdl_dispmode.h);
	// ¥Ê¥Ó¥²¡Œ¥·¥ç¥ó¥Ð¡Œ€òœü€¯¥¢¥×¥ê€¬¿š€ì€ë²èÌÌ
	realdisp_w = sdl_dispmode.w, realdisp_h = sdl_dispmode.h;

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 1 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 ); 

#if TARGET_OS_IPHONE
	sdl_window = SDL_CreateWindow(APPNAME" SDL", 0, 0, FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_BORDERLESS);
#else
	// for Android: window size€Î»ØÄê€ÏŽØ·ž€Ê€¯¥Õ¥ë¥¹¥¯¥ê¡Œ¥ó€Ë€Ê€ë€ß€¿€€
	sdl_window = SDL_CreateWindow(APPNAME" SDL", 0, 0, FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
#endif
#else
	sdl_window = SDL_CreateWindow(APPNAME" SDL", 0, 0, FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT, SDL_WINDOW_SHOWN);
#endif
	if (sdl_window == NULL) {
		p6logd("sdl_window: %ld", sdl_window);
	}

#ifdef USE_OGLES11
	SDL_GLContext glcontext = SDL_GL_CreateContext(sdl_window);

        glClearColor( 0, 0, 0, 0 );

	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glViewport(0, 0, 800, 600); //€³€³€òÁý€ä€µ€Ê€€€ÈOpenGL€Î²èÌÌ€Ï€»€Þ€€
	glViewport(0, 0, sdl_dispmode.w, sdl_dispmode.h);
	// ¥¹¥Þ¥Û€ä¥¿¥Ö€ÎŒÂ²èÌÌ€ËŽØ·ž€Ê€¯OpenGL€ÎÉÁ²èÎÎ°è€ò800x600€È€¹€ë¡£
	// 800x600€Ë€·€¿°ÕÌ£€ÏÆÃ€Ë€Ê€€¡£
	glOrthof(0, 800, 600, 0, -1, 1);
	//  glOrthof(0, 1024, 0, 1024, -1, 1);
	glMatrixMode(GL_MODELVIEW);
#endif
#endif // !SDL_VERSION_ATLEAST(2, 0, 0)

	if (!WinDraw_MenuInit()) {
		WinX68k_Cleanup();
		WinDraw_Cleanup();
		return 1;
	}

	SplashFlag = 20;
	SoundSampleRate = Config.SampleRate;

	StatBar_Show(Config.WindowFDDStat);
	WinDraw_ChangeSize();
	WinDraw_ChangeMode(FALSE);

	WinUI_Init();
	WinDraw_StartupScreen();

	if (!WinX68k_Init()) {
		WinX68k_Cleanup();
		WinDraw_Cleanup();
		return 1;
	}

	if (!WinX68k_LoadROMs()) {
		WinX68k_Cleanup();
		WinDraw_Cleanup();
		exit (1);
	}

	Keyboard_Init(); //WinDraw_Init()Á°€Ë°ÜÆ°

	if (!WinDraw_Init()) {
		WinDraw_Cleanup();
		Error("Error: Can't init screen.\n");
		return 1;
	}

	if ( SoundSampleRate ) {
		ADPCM_Init(SoundSampleRate);
		OPM_Init(4000000/*3579545*/, SoundSampleRate);
#ifndef	NO_MERCURY
		Mcry_Init(SoundSampleRate, winx68k_dir);
#endif
	} else {
		ADPCM_Init(100);
		OPM_Init(4000000/*3579545*/, 100);
#ifndef	NO_MERCURY
		Mcry_Init(100, winx68k_dir);
#endif
	}

	FDD_Init();
	SysPort_Init();
	Mouse_Init();
	Joystick_Init();
	SRAM_Init();
	WinX68k_Reset();
	Timer_Init();

	MIDI_Init();
	MIDI_SetMimpiMap(Config.ToneMapFile);	// ²»¿§ÀßÄê¥Õ¥¡¥€¥ë»ÈÍÑÈ¿±Ç
	MIDI_EnableMimpiDef(Config.ToneMap);

	if (sdlaudio == 0 && !DSound_Init(Config.SampleRate, Config.BufferSize)) {
		if (Config.DSAlert)
			fprintf(stderr, "Can't init sound.\n");
	}

	ADPCM_SetVolume((BYTE)Config.PCM_VOL);
	OPM_SetVolume((BYTE)Config.OPM_VOL);
#ifndef	NO_MERCURY
	Mcry_SetVolume((BYTE)Config.MCR_VOL);
#endif
	DSound_Play();

	// command line €«€é»ØÄê€·€¿Ÿì¹ç
	switch (argc) {
	case 3:
		strcpy(Config.FDDImage[1], argv[2]);
	case 2:
		strcpy(Config.FDDImage[0], argv[1]);
		break;
	}

	FDD_SetFD(0, Config.FDDImage[0], 0);
	FDD_SetFD(1, Config.FDDImage[1], 0);

	//SDL_StartTextInput();

	while (1) {
		// OPM_RomeoOut(Config.BufferSize * 5);
		if (menu_mode == menu_out
		    && (Config.NoWaitMode || Timer_GetCount())) {
			WinX68k_Exec();
#if defined(ANDROID) || TARGET_OS_IPHONE
			if (vk_cnt > 0) {
				vk_cnt--;
				if (vk_cnt == 0) {
					p6logd("vk_cnt 0");
				}
			}
			if  (menu_cnt > 0) {
				menu_cnt--;
				if (menu_cnt == 0) {
					p6logd("menu_cnt 0");
				}
			}
#endif
			if (SplashFlag) {
				SplashFlag--;
				if (SplashFlag == 0)
					WinDraw_HideSplash();
			}
		}
#ifndef PSP
		menu_key_down = SDLK_UNKNOWN;

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				goto end_loop;
			case SDL_MOUSEMOTION:
				p6logd("x:%d y:%d xrel:%d yrel:%d\n", ev.motion.x, ev.motion.y, ev.motion.xrel, ev.motion.yrel);
				break;
#if defined(ANDROID) || TARGET_OS_IPHONE
			case SDL_APP_WILLENTERBACKGROUND:
				DSound_Stop();
				break;
			case SDL_APP_WILLENTERFOREGROUND:
				DSound_Play();
				break;
			case SDL_FINGERDOWN:
				//p6logd("FINGERDOWN: tid: %lld,,, x:%f y:%f", ev.tfinger.touchId, ev.tfinger.x, ev.tfinger.y);
				if (touchId == -1) {
					touchId = ev.tfinger.touchId;
				}
				break;
			case SDL_FINGERMOTION:
				float kx, ky, dx, dy;
				p6logd("FM: x:%f y:%f dx:%f dy:%f\n", ev.tfinger.x, ev.tfinger.y, ev.tfinger.dx, ev.tfinger.dy);
				if (vk_cnt == 0) {
					kx = ev.tfinger.x * 800;
					ky = ev.tfinger.y * 600;
					dx = ev.tfinger.dx * 800;
					dy = ev.tfinger.dy * 600;
					if (kbd_x < kx && kbd_x + kbd_w > kx &&
					    kbd_y < ky && kbd_y + kbd_h > ky) {
						kbd_x += dx;
						kbd_y += dy;
						if (kbd_x < 0) kbd_x = 0;
						if (kbd_y < 0) kbd_y = 0;
						if (kbd_x > 700) {
							vk_cnt = -1;
						}
						if (kbd_y > 550) kbd_y = 550;
					}
				} else if (Config.JoyOrMouse) { // Mouse mode is off when the keyboard is active
					Mouse_Event(0, ev.tfinger.dx * 50 * Config.MouseSpeed, ev.tfinger.dy * 50 * Config.MouseSpeed);
				}
				break;
#endif
			case SDL_KEYDOWN:
#if defined(ANDROID) || TARGET_OS_IPHONE
				static DWORD bef = 0;
				DWORD now;
				switch (ev.key.keysym.sym) {
				case SDLK_AC_BACK:
					now = timeGetTime();
					if (now - bef < 1000) {
						goto end_loop;
					}
					bef = now;
					break;
				case SDLK_MENU:
					if (menu_mode == menu_out) {
						menu_mode = menu_enter;
						DSound_Stop();
					} else {
						DSound_Play();
						menu_mode = menu_out;
					}
					break;
				}
#endif
				printf("keydown: 0x%x\n", ev.key.keysym.sym);
				printf("font %d %d\n", FONT[100], FONT[101]);
				if (ev.key.keysym.sym == SDLK_F12) {
					if (menu_mode == menu_out) {
						menu_mode = menu_enter;
						DSound_Stop();
					} else {
						DSound_Play();
						menu_mode = menu_out;
					}
				}
#ifdef WIN68DEBUG
				if (ev.key.keysym.sym == SDLK_F10) {
					traceflag ^= 1;
					printf("trace %s\n", (traceflag)?"on":"off");
				}
#endif
				if (menu_mode != menu_out) {
					menu_key_down = ev.key.keysym.sym;
				} else {
					Keyboard_KeyDown(ev.key.keysym.sym);
				}
				break;
			case SDL_KEYUP:
				printf("keyup: 0x%x\n", ev.key.keysym.sym);
				Keyboard_KeyUp(ev.key.keysym.sym);
				break;
			}
		}
#endif //PSP

#ifdef PSP
		if (Joystick_get_downstate_psp(PSP_CTRL_START)) {
			if (menu_mode == menu_out) { 
				menu_mode = menu_enter;
				DSound_Stop();
			} else {
				DSound_Play();
				menu_mode = menu_out;
			}
		}

		if (menu_mode == menu_out
		    && Joystick_get_downstate_psp(PSP_CTRL_SELECT)) {
			Keyboard_ToggleSkbd();
			// 2ÅÙÆÉ€ßœü€±
			Joystick_reset_downstate_psp(PSP_CTRL_SELECT);
		}

		if (menu_mode == menu_out && Keyboard_IsSwKeyboard()) {
			Joystick_mv_anapad_psp();
			Joystatic_reset_anapad_psp();
			Keyboard_skbd();
		}
#endif

#if defined(ANDROID) || TARGET_OS_IPHONE

		state = Joystick_get_vbtn_state(7);

		if (menu_mode == menu_in) {
			if (state == VBTN_OFF && menu_cnt == 0) {
				menu_cnt = -2;
			}
			if (state == VBTN_ON && menu_cnt == -2) {
				DSound_Play();
				menu_mode = menu_out;
			}
		} else if (menu_mode == menu_out) {
			if (state == VBTN_OFF && menu_cnt == -2) {
				menu_cnt = -1;
			}
			if (menu_cnt == -1 && state == VBTN_ON) {
				p6logd("menu_cnt start");
				menu_cnt = 20;
			} else if (menu_cnt > 0 && state == VBTN_OFF) {
				menu_cnt = -1;
			}
			if (menu_cnt == 0) {
				p6logd("menu mode on");
				menu_mode = menu_enter;
				DSound_Stop();
			}
		}

#endif
		if (menu_mode != menu_out) {
			int ret; 

#ifdef PSP
			Joystick_Update(TRUE);
#else
			Joystick_Update(TRUE, menu_key_down);
#endif
			ret = WinUI_Menu(menu_mode == menu_enter);
			menu_mode = menu_in;
			if (ret == WUM_MENU_END) {
				DSound_Play();
				menu_mode = menu_out;
			} else if (ret == WUM_EMU_QUIT) {
				goto end_loop;
			}
		}
#ifdef PSP
		if (exit_flag) {
			goto end_loop;
		}
#endif

#if defined(ANDROID) || TARGET_OS_IPHONE

		if (menu_mode == menu_out) {
			state = Joystick_get_vbtn_state(6);
			if (vk_cnt == -1 && state == VBTN_ON) {
				p6logd("vk_cnt start");
				vk_cnt = 20;
			} else if (vk_cnt > 0 && state == VBTN_OFF) {
				vk_cnt = -1;
			}
			if (kbd_x > 700 && vk_cnt == 0) {
				kbd_x = 0, kbd_y = 0;
				p6logd("do_kbd");
			}
			if (kbd_x < 700) {
#ifdef USE_OGLES11
				Keyboard_skbd();
#endif
			}
		}
#endif

	}
end_loop:
	Memory_WriteB(0xe8e00d, 0x31);	// SRAMœñ€­¹þ€ßµö²Ä
	Memory_WriteD(0xed0040, Memory_ReadD(0xed0040)+1); // ÀÑ»»²ÔÆ¯»þŽÖ(min.)
	Memory_WriteD(0xed0044, Memory_ReadD(0xed0044)+1); // ÀÑ»»µ¯Æ°²ó¿ô

	OPM_Cleanup();
#ifndef	NO_MERCURY
	Mcry_Cleanup();
#endif

	Joystick_Cleanup();
	SRAM_Cleanup();
	FDD_Cleanup();
	//CDROM_Cleanup();
	MIDI_Cleanup();
	DSound_Cleanup();
	WinX68k_Cleanup();
	WinDraw_Cleanup();
	WinDraw_CleanupScreen();

	SaveConfig();

#if defined(PSP)
	puts("before end");
	sceKernelExitGame();
#elif defined(ANDROID) || TARGET_OS_IPHONE
	exit(0);
#endif
	return 0;
}
#else
//idfef __LIBRETRO__
#ifdef  __cplusplus
extern "C" {
#endif 
int mb1=0,mb2=0;
extern int retrow,retroh,CHANGEAV;
static int oldrw=0,oldrh=0;
SDL_Event ev;
SDL_Keycode menu_key_down;
int end_loop=0;
extern SDL_Surface *mysdl_surface;
enum {menu_out, menu_enter, menu_in};
int menu_mode = menu_out;
#ifdef  __cplusplus
};
#endif 
extern "C" int pmain(int argc, char *argv[])
{

	int sdlaudio = -1;

	p6logd("PX68K Ver.%s\n", PX68KVERSTR);

#ifdef RFMDRV
	struct sockaddr_in dest;

	memset(&dest, 0, sizeof(dest));
	dest.sin_port = htons(2151);
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr("127.0.0.1");

	rfd_sock = socket(AF_INET, SOCK_STREAM, 0);
	connect (rfd_sock, (struct sockaddr *)&dest, sizeof(dest));
#endif

	if (set_modulepath(winx68k_dir, sizeof(winx68k_dir)))
		return 1;

	dosio_init();
	file_setcd(winx68k_dir);
    puts(winx68k_dir);

	LoadConfig();

#ifndef NOSOUND
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		p6logd("SDL_Init error\n");		
#endif
		if (SDL_Init(SDL_INIT_VIDEO) < 0) {
			return 1;
		}
#ifndef NOSOUND
	} else {
		sdlaudio = 0;
	}
#endif

	SDL_WM_SetCaption(APPNAME" SDL", NULL);

        if ((mysdl_surface=SDL_SetVideoMode(FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT, 16, SDL_SWSURFACE)) == NULL) {
		puts("SDL_SetVideoMode() failed");
		return 1;
	}

	if (!WinDraw_MenuInit()) {
		WinX68k_Cleanup();
		WinDraw_Cleanup();
		return 1;
	}

	SplashFlag = 20;
	SoundSampleRate = Config.SampleRate;

	StatBar_Show(Config.WindowFDDStat);
	WinDraw_ChangeSize();
	WinDraw_ChangeMode(FALSE);

	WinUI_Init();
	WinDraw_StartupScreen();

	if (!WinX68k_Init()) {
		WinX68k_Cleanup();
		WinDraw_Cleanup();
		return 1;
	}

	if (!WinX68k_LoadROMs()) {
		WinX68k_Cleanup();
		WinDraw_Cleanup();
		exit (1);
	}

	Keyboard_Init(); //WinDraw_Init()Á°€Ë°ÜÆ°

	if (!WinDraw_Init()) {
		WinDraw_Cleanup();
		Error("Error: Can't init screen.\n");
		return 1;
	}

	if ( SoundSampleRate ) {
		ADPCM_Init(SoundSampleRate);
		OPM_Init(4000000/*3579545*/, SoundSampleRate);
#ifndef	NO_MERCURY
		Mcry_Init(SoundSampleRate, winx68k_dir);
#endif
	} else {
		ADPCM_Init(100);
		OPM_Init(4000000/*3579545*/, 100);
#ifndef	NO_MERCURY
		Mcry_Init(100, winx68k_dir);
#endif
	}

	FDD_Init();
	SysPort_Init();
	Mouse_Init();
	Joystick_Init();
	SRAM_Init();
	WinX68k_Reset();
	Timer_Init();

	MIDI_Init();
	MIDI_SetMimpiMap(Config.ToneMapFile);	// ²»¿§ÀßÄê¥Õ¥¡¥€¥ë»ÈÍÑÈ¿±Ç
	MIDI_EnableMimpiDef(Config.ToneMap);

	if (sdlaudio == 0 && !DSound_Init(Config.SampleRate, Config.BufferSize)) {
		if (Config.DSAlert)
			fprintf(stderr, "Can't init sound.\n");
	}

	ADPCM_SetVolume((BYTE)Config.PCM_VOL);
	OPM_SetVolume((BYTE)Config.OPM_VOL);
#ifndef	NO_MERCURY
	Mcry_SetVolume((BYTE)Config.MCR_VOL);
#endif
	DSound_Play();

	// command line €«€é»ØÄê€·€¿Ÿì¹ç
	if(argc==3 && argv[1][0]=='-' && argv[1][1]=='h')
		strcpy(Config.HDImage[0], argv[2]);
	else {
		switch (argc) {
		case 3:
			strcpy(Config.FDDImage[1], argv[2]);
		case 2:
			strcpy(Config.FDDImage[0], argv[1]);
			break;
		}
	}

	FDD_SetFD(0, Config.FDDImage[0], 0);
	FDD_SetFD(1, Config.FDDImage[1], 0);
	
	//SDL_StartTextInput();
}

extern "C" void exec_app_retro(){

	//while (1) {
		// OPM_RomeoOut(Config.BufferSize * 5);
		if (menu_mode == menu_out
		    && (Config.NoWaitMode || Timer_GetCount())) {
			WinX68k_Exec();

			if (SplashFlag) {
				SplashFlag--;
				if (SplashFlag == 0)
					WinDraw_HideSplash();
			}
		}

		menu_key_down = SDLK_UNKNOWN;

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				 end_loop=1;break;
			case SDL_MOUSEBUTTONDOWN:

                		if (ev.button.button == SDL_BUTTON_LEFT)Mouse_Event(1,1.0,0);
                		if (ev.button.button == SDL_BUTTON_RIGHT)Mouse_Event(2,1.0,0);

                    		break;
            		case SDL_MOUSEBUTTONUP:

				if (ev.button.button == SDL_BUTTON_LEFT)Mouse_Event(1,0,0);
				if (ev.button.button == SDL_BUTTON_RIGHT)Mouse_Event(2,0,0);

				break;
			case SDL_MOUSEMOTION:
				//p6logd("x:%d y:%d xrel:%d yrel:%d\n", ev.motion.x, ev.motion.y, ev.motion.xrel, ev.motion.yrel);
 Mouse_Event(0, ev.motion.xrel, ev.motion.yrel);
				break;
			case SDL_KEYDOWN:
				printf("keydown: 0x%x\n", ev.key.keysym.sym);
				printf("font %d %d\n", FONT[100], FONT[101]);
				if (ev.key.keysym.sym == SDLK_F12) {
					if (menu_mode == menu_out) {
						oldrw=retrow;oldrh=retroh;
						retroh=600;retrow=800;
						CHANGEAV=1;
						menu_mode = menu_enter;
						DSound_Stop();
					} else {
						CHANGEAV=1;
						retrow=oldrw;retroh=oldrh;
						DSound_Play();
						menu_mode = menu_out;
					}
				}
#ifdef WIN68DEBUG
				if (ev.key.keysym.sym == SDLK_F10) {
					traceflag ^= 1;
					printf("trace %s\n", (traceflag)?"on":"off");
				}
#endif
				if (menu_mode != menu_out) {
					menu_key_down = ev.key.keysym.sym;
				} else {
					Keyboard_KeyDown(ev.key.keysym.sym);
				}
				break;
			case SDL_KEYUP:
				printf("keyup: 0x%x\n", ev.key.keysym.sym);
				Keyboard_KeyUp(ev.key.keysym.sym);
				break;
			}
		}



		if (menu_mode != menu_out) {
			int ret; 

			Joystick_Update(TRUE, menu_key_down);

			ret = WinUI_Menu(menu_mode == menu_enter);
			menu_mode = menu_in;
			if (ret == WUM_MENU_END) {
				DSound_Play();
				menu_mode = menu_out;
			} else if (ret == WUM_EMU_QUIT) {
				 end_loop=1;
			}
		}

	//}
}

extern "C" void end_loop_retro(){
//end_loop:
	Memory_WriteB(0xe8e00d, 0x31);	// SRAMœñ€­¹þ€ßµö²Ä
	Memory_WriteD(0xed0040, Memory_ReadD(0xed0040)+1); // ÀÑ»»²ÔÆ¯»þŽÖ(min.)
	Memory_WriteD(0xed0044, Memory_ReadD(0xed0044)+1); // ÀÑ»»µ¯Æ°²ó¿ô

	OPM_Cleanup();
#ifndef	NO_MERCURY
	Mcry_Cleanup();
#endif

	Joystick_Cleanup();
	SRAM_Cleanup();
	FDD_Cleanup();
	//CDROM_Cleanup();
	MIDI_Cleanup();
	DSound_Cleanup();
	WinX68k_Cleanup();
	WinDraw_Cleanup();
	WinDraw_CleanupScreen();

	SaveConfig();

	return 0;
}

#endif //ifndef __LIBRETRO__

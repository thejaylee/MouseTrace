#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "resource.h"
#include "rawinput.h"
#include "drawer.h"

#define MAX_SAMPLES 500
#define SHOW_SAMPLES 250
#define HERTZ_SAMPLES 10
#define MAX_TRACERS 200
#define FILE_BUF_SZ 4096

#define RECORD_FILE "MouseTrace.csv"
#define VALUES_FILE "mt_values.txt"
#define TARGET_FILE "mt_tracer.bmp"
#define XHIST_FILE "mt_xhist.bmp"
#define YHIST_FILE "mt_yhist.bmp"

/* globals */

HINSTANCE hThis = 0;
HWND hDialog = NULL;
HACCEL hAccel = NULL;

long *xsamples, *ysamples;
long samplemax;
float *hertz;
LARGE_INTEGER freq;

int tracers;
FILE *recfile = NULL;

/* prototypes */
void processRawInput(LPRAWINPUT);
void updateDialog(long, long, double);

int initDialog(HINSTANCE);
int initGraphers(void);
void toggleRecordValues(void);
void saveValueList(void);
void cleanup(void);

INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
int WINAPI WinMain (HINSTANCE, HINSTANCE, char*, int);

void processRawInput( LPRAWINPUT ri ) {
	static long mSample = 0, //array indice mSampleers
					mHertz = 0;
	static LARGE_INTEGER past = {0};

	LPRAWMOUSE mouse = &ri->data.mouse;
	LARGE_INTEGER now;
	float secs;
	long c;

	/* update the listbox */
	if ( mouse->lLastX || mouse->lLastY ) {
		if ( abs(mouse->lLastX) > samplemax )
			samplemax = abs(mouse->lLastX);
		if ( abs(mouse->lLastY) > samplemax )
			samplemax = abs(mouse->lLastY);

		/* update samples */
		for ( c=MAX_SAMPLES-1; c>0; c-- ) {
			xsamples[c] = xsamples[c-1];
			ysamples[c] = ysamples[c-1];
		}
		xsamples[0] = mouse->lLastX;
		ysamples[0] = mouse->lLastY;

		/* update the frequncy text */
		mHertz = (mHertz + 1) % HERTZ_SAMPLES; //update cyclic index
		QueryPerformanceCounter(&now);
		secs = (float)(now.QuadPart - past.QuadPart) / freq.QuadPart;
		QueryPerformanceCounter(&past);
		hertz[mHertz] = 1.0f / secs;

		if ( recfile != NULL )
			fprintf(recfile, "%d,%d,%f\n", mouse->lLastX, mouse->lLastY, secs * 1000.0);

		updateDialog(mouse->lLastX, mouse->lLastY, secs);
	}
}

void updateDialog( long x, long y, double secs ) {
	static char counter = 0;
	static long nMotions = 0;
	
	float normx, normy;
	//LVITEM lvi;
	char strbuf[64];
	double avgHertz;
	int c;

	counter = (counter + 1) % 5;

	/* update the graphs */
	normx = (float)x / samplemax;
	normy = (float)y / samplemax;
	drawTracerGraph(GetDlgItem(hDialog, IDC_TARGETGRAPH), xsamples, ysamples, samplemax, tracers);
	if ( counter == 0 ) { //but not too often
		drawHistogram(GetDlgItem(hDialog, IDC_XGRAPH), xsamples, MAX_SAMPLES, samplemax);
		drawHistogram(GetDlgItem(hDialog, IDC_YGRAPH), ysamples, MAX_SAMPLES, samplemax);
	}

	/* update the frequncy text */
	avgHertz = 0.0;
	for ( c=0; c<HERTZ_SAMPLES; c++ )
		avgHertz += hertz[c];
	avgHertz /= HERTZ_SAMPLES;
	sprintf_s(strbuf, sizeof(strbuf), "%.1fHz", avgHertz);
	SendDlgItemMessage(hDialog, IDST_FREQUENCY, WM_SETTEXT, 0, (LPARAM)strbuf);

	/* manage the list view */
	nMotions++;
	sprintf_s(strbuf, sizeof(strbuf), "%-4d %-4d %.2f", x, y, secs * 1000.0);
	SendDlgItemMessage(hDialog, IDLS_RAW, LB_INSERTSTRING, 1, (LPARAM)strbuf);
	if ( nMotions > SHOW_SAMPLES ) {
		SendDlgItemMessage(hDialog, IDLS_RAW, LB_DELETESTRING, SHOW_SAMPLES, 0);
		nMotions--;
	}

	/* manage the values list */
	sprintf_s(strbuf, sizeof(strbuf), "%3d", abs(x));
	if ( SendDlgItemMessage(hDialog, IDLS_VALUES, LB_FINDSTRING, -1, (LPARAM)strbuf) == LB_ERR )
		SendDlgItemMessage(hDialog, IDLS_VALUES, LB_ADDSTRING, 0, (LPARAM)strbuf);
	sprintf_s(strbuf, sizeof(strbuf), "%3d", abs(y));
	if ( SendDlgItemMessage(hDialog, IDLS_VALUES, LB_FINDSTRING, -1, (LPARAM)strbuf) == LB_ERR )
		SendDlgItemMessage(hDialog, IDLS_VALUES, LB_ADDSTRING, 0, (LPARAM)strbuf);
}

int initDialog( HINSTANCE hInst ) {
	HICON icon;
	HFONT hFont;
	//ListView
	//LVCOLUMN lvc;
	char strbuf[16];
	INITCOMMONCONTROLSEX icex;

	/* Ensure that the common control DLL is loaded. */
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	hDialog = CreateDialog(hInst, MAKEINTRESOURCE(IDD_MAIN), 0, DialogProc);
	if ( !hDialog ) {
		char buf [100];
		wsprintf(buf, "Error x%x\n", GetLastError());
		OutputDebugString(buf);
		MessageBox(0, buf, "CreateDialog", MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}

	/* set the icon */
	icon = LoadIcon(hThis, MAKEINTRESOURCE(IDI_ICON));
	SendMessage(hDialog, WM_SETICON, 0, (LPARAM)icon);

	/* change font of the list */
	hFont = CreateFont(10, 0, 0, 0,
							 FW_DONTCARE,
							 FALSE, FALSE, FALSE,
							 DEFAULT_CHARSET,
							 OUT_DEFAULT_PRECIS,
							 CLIP_DEFAULT_PRECIS,
							 DEFAULT_QUALITY,
							 FIXED_PITCH | FF_MODERN,
							 "Terminal");
	//SendMessage(hListview, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
	SendDlgItemMessage(hDialog, IDLS_RAW, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
	SendDlgItemMessage(hDialog, IDLS_VALUES, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
	sprintf_s(strbuf, sizeof(strbuf), "x	 y	 ms");
	SendDlgItemMessage(hDialog, IDLS_RAW, LB_ADDSTRING, 0, (LPARAM)strbuf);

	/* set up the trackbar */
	SendDlgItemMessage(hDialog, IDSL_TRACERS, TBM_SETRANGEMIN, FALSE, 0);
	SendDlgItemMessage(hDialog, IDSL_TRACERS, TBM_SETRANGEMAX, FALSE, MAX_TRACERS);
	SendDlgItemMessage(hDialog, IDSL_TRACERS, TBM_SETTICFREQ, 10, 0);

	/* initialize the accelerators */
	hAccel = LoadAccelerators(hThis, MAKEINTRESOURCE(IDR_ACCEL));

	return 0;
}

int initGraphers( void ) {
	char buf [100];

	tracers = 0;

	xsamples = malloc(MAX_SAMPLES * sizeof(long));
	ysamples = malloc(MAX_SAMPLES * sizeof(long));
	hertz = malloc(HERTZ_SAMPLES * sizeof(float));

	if ( (xsamples == NULL) || (ysamples == NULL) || (hertz == NULL) ) {
		wsprintf(buf, "Error x%x", GetLastError());
		MessageBox(0, buf, "Error allocation memory", MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}

	memset(xsamples, 0, sizeof(long) * MAX_SAMPLES);
	memset(ysamples, 0, sizeof(long) * MAX_SAMPLES);
	memset(hertz, 0, sizeof(float) * HERTZ_SAMPLES);

	QueryPerformanceFrequency(&freq);

	return 0;
}

void toggleRecordValues( void ) {
	static char *filebuf;

	if ( !filebuf ) {
		time_t t;
		struct tm ti;
		char filename[255];

		time(&t);
		localtime_s(&ti, &t);
		sprintf_s(
			filename,
			sizeof(filename),
			"mousetrace_%02d%02d%02d-%02d%02d%02d.csv",
			ti.tm_year + 1900,
			ti.tm_mon,
			ti.tm_mday,
			ti.tm_hour,
			ti.tm_min,
			ti.tm_sec
		);

		filebuf = malloc(FILE_BUF_SZ);
		fopen_s(&recfile, filename, "a");
		setvbuf(recfile, filebuf, _IOFBF, FILE_BUF_SZ); // set the file buffer size
		//fputs("x,y,ms\n", recfile);
	} else {
		fflush(recfile);
		fclose(recfile);
		free(filebuf);
		recfile = NULL;
		filebuf = NULL;
	}
}

void saveValueList( void ) {
	int c, vals;
	FILE *fp = NULL;
	char strbuf[16];
	char *ch;

	fopen_s(&fp, VALUES_FILE, "w");
	if ( fp == NULL ) {
		MessageBox(hDialog, "Could not open values file", "Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	(LRESULT) vals = SendDlgItemMessage(hDialog, IDLS_VALUES, LB_GETCOUNT, 0, 0);

	for ( c=0; c<vals; c++ ) {
		SendDlgItemMessage(hDialog, IDLS_VALUES, LB_GETTEXT, c, (LPARAM)strbuf);
		for (ch=strbuf; *ch==' '; ch++);
		fputs(ch, fp);
		fputc(',', fp);
	}

	fclose(fp);
}

void cleanup( void ) {
	drawCleanup();
	//for some reason, i get failures with these
	if (hertz) free(hertz);
	if (xsamples) free(xsamples);
	if (ysamples) free(ysamples);
}

INT_PTR CALLBACK DialogProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam ) {
	unsigned long sz;
	//LRESULT res;
	LPRAWINPUT ri;

	static int c=0;
	char dbuf[32];

	switch ( message ) {
		case WM_INITDIALOG:
			return TRUE;
		case WM_HSCROLL:
			if ( (HWND)lParam == GetDlgItem(hDialog, IDSL_TRACERS) )
				tracers = (int) SendDlgItemMessage(hDialog, IDSL_TRACERS, TBM_GETPOS, 0, 0);
			return TRUE;
		case WM_COMMAND:
			//buttons
			switch ( HIWORD(wParam) ) {
				case BN_CLICKED:
					switch ( LOWORD(wParam) ) {
						case IDCB_RECORD:
							// res = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0); // BST_CHECKED
							toggleRecordValues();
							break;
						case IDB_SAVEGRAPHS:
							drawHwndToFile(GetDlgItem(hDialog, IDC_TARGETGRAPH), TARGET_FILE);
							drawHwndToFile(GetDlgItem(hDialog, IDC_XGRAPH), XHIST_FILE);
							drawHwndToFile(GetDlgItem(hDialog, IDC_YGRAPH), YHIST_FILE);
							break;
						case IDB_SAVEVALUES:
							saveValueList();
							break;
					}
			}

			//accelerators
			switch ( LOWORD(wParam) ) {
			case IDA_SAVEGRAPHS:
				drawHwndToFile(GetDlgItem(hDialog, IDC_TARGETGRAPH), TARGET_FILE);
				drawHwndToFile(GetDlgItem(hDialog, IDC_XGRAPH), XHIST_FILE);
				drawHwndToFile(GetDlgItem(hDialog, IDC_YGRAPH), YHIST_FILE);
				break;
			case IDA_TOGGLERECORD:
				toggleRecordValues();
				break;
			}
			return TRUE;
		case WM_CLOSE:
			DestroyWindow(hwnd);
			return TRUE;
		case WM_DESTROY:
			PostQuitMessage(0);
			return TRUE;
		case WM_INPUT:
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &sz, sizeof(RAWINPUTHEADER));
			ri = malloc(sz);
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, (LPVOID)ri, &sz, sizeof(RAWINPUTHEADER));

			sprintf_s(dbuf, sizeof(dbuf), "%d\n", c++);
			OutputDebugString(dbuf);

			if (ri->header.dwType == RIM_TYPEMOUSE)
				processRawInput(ri);
			free(ri);
			return TRUE;
		/*default:
			return DefDlgProc(hwnd, message, wParam, lParam);*/
	}
	
	return FALSE;
}

int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInst, char * cmdParam, int cmdShow ) {
	MSG  msg;
	int status;

	hThis = hInst;

	if ( status = initDialog(hThis) )
		return status;

	if ( status = rawInitMouse(hDialog) )
		return status;

	if ( status = initGraphers() )
		return status;

	drawInit();

	ShowWindow(hDialog, SW_SHOWNORMAL);

	while ( status = GetMessage(&msg, 0, 0, 0) ) {
		if ( status == -1 )
			return -1;

		if ( !TranslateAccelerator(hDialog, hAccel, &msg) ) {
			if ( !IsDialogMessage(hDialog, &msg) ) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	cleanup();
	return (int) msg.wParam;
}
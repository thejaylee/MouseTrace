#include "drawer.h"

#define XHAIR_HALF 4
#define BIG_HASH 13
#define SMALL_HASH 7

#define LINE_XAXIS 0
#define LINE_YAXIS 1

/* STATIC */

static HDC hdcTracer = NULL;
static HDC hdcHist = NULL; //for back buffering

static void centerLine(HDC hdc, long x, long y, long length, char axis) {
	long half = length/2;

	switch (axis) {
		case LINE_XAXIS:
			MoveToEx(hdc, x-half, y, (LPPOINT) NULL); 
			LineTo(hdc, x+half+(length&1), y);
			break;
		case LINE_YAXIS:
			MoveToEx(hdc, x, y-half, (LPPOINT) NULL); 
			LineTo(hdc, x, y+half+(length&1));
			break;
	}
}

static void borderRect(HDC hdc, long x1, long y1, long x2, long y2, HBRUSH brush) {
	RECT rect;
	rect.left = x1;
	rect.right = x2;
	rect.top = y1;
	rect.bottom = y2;

	FrameRect(hdc, &rect, brush);
}

static void initHistogram(HWND hwnd) {
	HDC hdc;
	HBITMAP hist;
	RECT rect;
	HBRUSH brush;
	long w, h;

	GetClientRect(hwnd, &rect);
	w = rect.right - rect.left;
	h = rect.bottom - rect.top;

	/* create the memory DC */
	hdc = GetDC(hwnd);
	hist = CreateCompatibleBitmap(hdc, w, h);
	hdcHist = CreateCompatibleDC(hdc);
	SelectBitmap(hdcHist, hist);
	ReleaseDC(hwnd, hdc);
	
	/* fill with black */
	brush = CreateSolidBrush(RGB(0,0,0));
	FillRect(hdcHist, &rect, brush);
	DeleteBrush(brush);
}

/* create and hdc of the trace graph so it can just be
	blitted over and we don't have to construct it every time */
static void initTracer(HWND hwnd) {
	HDC hdc;
	HBITMAP graph;
	HPEN pen;
	HBRUSH brush;
	RECT rect;
	long fw,fh,w,h;

	GetClientRect(hwnd, &rect);
	fw = rect.right - rect.left;
	fh = rect.bottom - rect.top;
	w = fw - (XHAIR_HALF*2|1);
	h = fh - (XHAIR_HALF*2|1);


	/* create the memoryDC for the graph */
	hdc = GetDC(hwnd);
	graph = CreateCompatibleBitmap(hdc, fw, fh);
	hdcTracer = CreateCompatibleDC(hdc);
	ReleaseDC(hwnd, hdc); //we now longer need this, we now draw into hdcTracer
	SelectBitmap(hdcTracer, graph);

	/* fill with black */
	brush = CreateSolidBrush(RGB(0,0,0));
	FillRect(hdcTracer, &rect, brush);
	DeleteBrush(brush);
	
	/* draw reference grid */
	pen = CreatePen(PS_SOLID, 1, RGB(0,255,255));
	SelectPen(hdcTracer, pen);
	//Squares
	brush = CreateSolidBrush(RGB(0,64,64));
	SelectBrush(hdcTracer, brush);
	//borderRect(hdcTracer, XHAIR_HALF, XHAIR_HALF, XHAIR_HALF+w+1, XHAIR_HALF+h+1, brush);
	borderRect(hdcTracer, XHAIR_HALF, XHAIR_HALF, XHAIR_HALF+w+1, XHAIR_HALF+w+1, brush);
	borderRect(hdcTracer, XHAIR_HALF + w/4, XHAIR_HALF + h/4, XHAIR_HALF + w/4*3+1, XHAIR_HALF + h/4*3+1, brush);
	DeleteBrush(brush);
	/*brush = CreateSolidBrush(RGB(0,16,16));
	SelectBrush(hdcTracer, brush);
	borderRect(hdcTracer, XHAIR_HALF + w/8*3, XHAIR_HALF + h/8*3, XHAIR_HALF + w/8*5+1, XHAIR_HALF + h/8*5+1, brush);
	borderRect(hdcTracer, XHAIR_HALF + w/8, XHAIR_HALF + h/8, XHAIR_HALF + w/8*7+1, XHAIR_HALF + h/8*7+1, brush);
	DeleteBrush(brush);*/
	//Main Axis
	centerLine(hdcTracer, fw/2, fh/2, w, LINE_XAXIS);
	centerLine(hdcTracer, fw/2, fh/2, h, LINE_YAXIS);
	//X half hashes
	centerLine(hdcTracer,			XHAIR_HALF, h/2 + XHAIR_HALF, BIG_HASH, LINE_YAXIS);
	centerLine(hdcTracer, w/4	+ XHAIR_HALF, h/2 + XHAIR_HALF, BIG_HASH, LINE_YAXIS);
	centerLine(hdcTracer, w/4*3 + XHAIR_HALF, h/2 + XHAIR_HALF, BIG_HASH, LINE_YAXIS);
	centerLine(hdcTracer, w	  + XHAIR_HALF, h/2 + XHAIR_HALF, BIG_HASH, LINE_YAXIS);
	//X quarter hashes
	centerLine(hdcTracer, w/8	+ XHAIR_HALF, h/2 + XHAIR_HALF, SMALL_HASH, LINE_YAXIS);
	centerLine(hdcTracer, w/8*3 + XHAIR_HALF, h/2 + XHAIR_HALF, SMALL_HASH, LINE_YAXIS);
	centerLine(hdcTracer, w/8*5 + XHAIR_HALF, h/2 + XHAIR_HALF, SMALL_HASH, LINE_YAXIS);
	centerLine(hdcTracer, w/8*7 + XHAIR_HALF, h/2 + XHAIR_HALF, SMALL_HASH, LINE_YAXIS);
	//Y half hashes
	centerLine(hdcTracer, w/2 + XHAIR_HALF,			XHAIR_HALF, BIG_HASH, LINE_XAXIS);
	centerLine(hdcTracer, w/2 + XHAIR_HALF, h/4	+ XHAIR_HALF, BIG_HASH, LINE_XAXIS);
	centerLine(hdcTracer, w/2 + XHAIR_HALF, h/4*3 + XHAIR_HALF, BIG_HASH, LINE_XAXIS);
	centerLine(hdcTracer, w/2 + XHAIR_HALF, h	  + XHAIR_HALF, BIG_HASH, LINE_XAXIS);
	//Y quarter hashes
	centerLine(hdcTracer, w/2 + XHAIR_HALF, h/8	+ XHAIR_HALF, SMALL_HASH, LINE_XAXIS);
	centerLine(hdcTracer, w/2 + XHAIR_HALF, h/8*3 + XHAIR_HALF, SMALL_HASH, LINE_XAXIS);
	centerLine(hdcTracer, w/2 + XHAIR_HALF, h/8*5 + XHAIR_HALF, SMALL_HASH, LINE_XAXIS);
	centerLine(hdcTracer, w/2 + XHAIR_HALF, h/8*7 + XHAIR_HALF, SMALL_HASH, LINE_XAXIS);

	DeletePen(pen);
	GdiFlush();
}

/* GLOBAL */

void drawInit(void) {
	GdiSetBatchLimit(120);
}

void drawHistogram(HWND hwnd, long *values, int nValues, int maxval) {
	HDC hdc;
	RECT bounds;
	HBRUSH brush;
	HPEN pen;
	int w,h;
	int bars;
	float barw;
	long histidx;
	unsigned short histmax;
	//unsigned short phist[HIST_BARS], nhist[HIST_BARS];
	unsigned short *phist = NULL,
						*nhist = NULL;
	int c;

	if (hdcHist == NULL) initHistogram(hwnd);

	if (maxval < MAX_BARS)
		bars = maxval;
	else
		bars = MAX_BARS;

	phist = malloc(sizeof(short) * bars);
	nhist = malloc(sizeof(short) * bars);

	if ((phist == NULL) || (nhist == NULL))
		return;

	/* compute histogram data */
	memset(phist, 0, sizeof(short) * bars);
	memset(nhist, 0, sizeof(short) * bars);
	for (c=0; c<nValues; c++) {
		histidx = (long)(((float)abs(values[c]) / maxval) * (bars - 0.001f));
		if (values[c] > 0)
			phist[histidx]++;
		else if (values[c] < 0)
			nhist[histidx]++;
	}
	//find scale
	histmax = 0;
	for (c=0; c<bars; c++) {
		if (phist[c] > histmax) histmax = phist[c];
		if (nhist[c] > histmax) histmax = nhist[c];
	}

	/* get attributes */
	GetClientRect(hwnd, &bounds);
	w = bounds.right - bounds.left;
	h = bounds.bottom - bounds.top;
	barw = (float) w / bars;

	/* fill with black */
	brush = CreateSolidBrush(RGB(0,0,0));
	FillRect(hdcHist, &bounds, brush);
	DeleteBrush(brush);
	GdiFlush();

	/* draw histogram */
	//positive values
	brush = CreateSolidBrush(RGB(0,255,0));
	pen = CreatePen(PS_SOLID, 1, RGB(0,255,0));
	SelectBrush(hdcHist, brush);
	SelectPen(hdcHist, pen);
	for (c=0; c<bars; c++) {
		Rectangle(hdcHist,
					 bounds.left + (int)(c * barw),
					 bounds.top + h/2 - (int)(((float)phist[c] / histmax) * (h / 2.0f)),
					 bounds.left + (int)((c + 1) * barw),
					 bounds.bottom - h/2);
	}
	DeleteBrush(brush);
	DeletePen(pen);
	GdiFlush();

	//negative values
	brush = CreateSolidBrush(RGB(255,0,0));
	pen = CreatePen(PS_SOLID, 1, RGB(255,0,0));
	SelectBrush(hdcHist, brush);
	SelectPen(hdcHist, pen);
	for (c=0; c<bars; c++) {
		Rectangle(hdcHist,
					 bounds.left + (int)(c * barw),
					 bounds.top + h/2,
					 bounds.left + (int)((c + 1) * barw),
					 bounds.top + h/2 + (int)(((float)nhist[c] / histmax) * (h / 2.0f)));
	}
	DeleteBrush(brush);
	DeletePen(pen);
	GdiFlush();

	/* draw a line through zero */
	pen = CreatePen(PS_SOLID, 1, RGB(0,255,255));
	SelectPen(hdcHist, pen);
	MoveToEx(hdcHist, 0, h/2, (LPPOINT) NULL);
	LineTo(hdcHist, w, h/2);
	DeletePen(pen);
	GdiFlush();

	/* swap the buffer */
	hdc = GetDC(hwnd);
	BitBlt(hdc, 0, 0, w, h, hdcHist, 0, 0, SRCCOPY);
	GdiFlush();
	ReleaseDC(hwnd, hdc);

	free(phist);
	free(nhist);
}

/* make sure that the xvalues and yvalues contain enough values to go tracers deep */
void drawTracerGraph(HWND hwnd, long *xvalues, long *yvalues, int maxval, int tracers) {
//void drawTracerGraph(HWND hwnd, float nx, float ny) {
	HDC hdc;
	RECT rect;
	HPEN pen;
	int x,y;
	int w,h,fw,fh;
	int c;

	if (hdcTracer == NULL) initTracer(hwnd);

	/* get attributes */
	GetClientRect(hwnd, &rect);
	hdc = GetDC(hwnd);
	fw = rect.right - rect.left;
	fh = rect.bottom - rect.top;
	w = rect.right - rect.left - (XHAIR_HALF*2|1);
	h = rect.bottom - rect.top - (XHAIR_HALF*2|1);

	/* copy over the trace graph */
	BitBlt(hdc, 0, 0, fw, fh, hdcTracer, 0, 0, SRCCOPY);

	/* draw the xhair */
	for (c=tracers; c>0; c--) {
		pen = CreatePen(PS_SOLID, 1, RGB(255-(128/tracers*c),255-(255/tracers*c),0));
		SelectPen(hdc, pen);
		x = (int) (w*((float)xvalues[c] / maxval + 1)/2.0f) + XHAIR_HALF;
		y = (int) (h*((float)yvalues[c] / maxval + 1)/2.0f) + XHAIR_HALF;
		centerLine(hdc, x, y, (XHAIR_HALF|1)-2, LINE_XAXIS);
		centerLine(hdc, x, y, (XHAIR_HALF|1)-2, LINE_YAXIS);
		DeletePen(pen);
	}
	//special case of very first xhair
	pen = CreatePen(PS_SOLID, 1, RGB(255,0,0));
	SelectPen(hdc, pen);
	x = (int) (w*((float)xvalues[c] / maxval + 1)/2.0f) + XHAIR_HALF;
	y = (int) (h*((float)yvalues[c] / maxval + 1)/2.0f) + XHAIR_HALF;
	centerLine(hdc, x, y, XHAIR_HALF*2|1, LINE_XAXIS);
	centerLine(hdc, x, y, XHAIR_HALF*2|1, LINE_YAXIS);
	DeletePen(pen);
	GdiFlush();

	ReleaseDC(hwnd, hdc); 
}

void drawHwndToFile(HWND hwnd, char *filename) {
	HDC hdc, hdcMem;
	HBITMAP hBitmap, hbmpOld;
	RECT rect;
	long w,h;

	FILE *fp = NULL;
	unsigned char *pixels;
	BITMAPINFO bmi;
	BITMAPFILEHEADER bmfilehdr;
	
	GetClientRect(hwnd, &rect);
	w = rect.right - rect.left;
	h = rect.bottom - rect.top;
	hdc = GetDC(hwnd);
	/* use a temp DC to copy the bits to a new bitmap */
	hBitmap = CreateCompatibleBitmap(hdc, w, h);
	hdcMem = CreateCompatibleDC(hdc);
	hbmpOld = SelectBitmap(hdcMem, hBitmap);
	BitBlt(hdcMem, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
	SelectBitmap(hdcMem, hbmpOld);
	DeleteDC(hdcMem);

	/* copy over the bits to memory */
	memset(&bmi, 0, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;
	pixels = malloc((w + w%4) * h * 3); //scanlines must end on 4 byte boundries
	if (pixels == NULL) {
		MessageBox(hwnd, "Could not allocate memory for bitmap", "Error", MB_OK|MB_ICONERROR);
		return;
	}

	GetDIBits(hdc, hBitmap, 0, h, pixels, &bmi, DIB_RGB_COLORS);

	bmfilehdr.bfReserved1 = 0;
	bmfilehdr.bfReserved2 = 0;
	bmfilehdr.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmi.bmiHeader.biSizeImage;
	bmfilehdr.bfType = 'MB';
	bmfilehdr.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	fopen_s(&fp, filename, "wb");
	fwrite(&bmfilehdr, sizeof(BITMAPFILEHEADER), 1, fp);
	fwrite(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER), 1, fp);
	fwrite(pixels, bmi.bmiHeader.biSizeImage, 1, fp);
	fclose(fp);

	free(pixels);
	DeleteBitmap(hBitmap);
	//DeleteDC(hdcMem);
	ReleaseDC(hwnd, hdc);
}


void drawCleanup(void) {
	DeleteDC(hdcTracer);
	DeleteDC(hdcHist);
}
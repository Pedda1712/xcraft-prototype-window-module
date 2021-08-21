#include "xcraft_window_module.h"

#include <ctype.h>
#include <time.h>
#include <stdio.h>

Display* dpy;
Window root;
GLint glx_context_attributes[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
XVisualInfo* vi;
Colormap cmap;
XSetWindowAttributes swa;
Window win;
GLXContext glc;
XWindowAttributes gwa;
XEvent xev;

bool xg_running = true;
uint32_t xg_width ;
uint32_t xg_height;
bool xg_has_glx = false;
char* xg_title = "XWindow";

// FrameTime values
struct timespec spec;
intmax_t s_last;
long ms_last;
float fTime;

//Mouse Position
int32_t _mouse_x = 0;
int32_t _mouse_y = 0;

bool xg_ascii_keys [256];
bool xg_modif_keys [6];

void emptyfnc (bool b){}

//mouse button callback
void (*button1fnc)(bool) = emptyfnc;
void (*button2fnc)(bool) = emptyfnc;
void (*button3fnc)(bool) = emptyfnc;
void (*button4fnc)(bool) = emptyfnc;
void (*button5fnc)(bool) = emptyfnc;

void xg_set_button1_callback (void (*fncptr)(bool)){button1fnc=fncptr;}
void xg_set_button2_callback (void (*fncptr)(bool)){button2fnc=fncptr;}
void xg_set_button3_callback (void (*fncptr)(bool)){button3fnc=fncptr;}
void xg_set_button4_callback (void (*fncptr)(bool)){button4fnc=fncptr;}
void xg_set_button5_callback (void (*fncptr)(bool)){button5fnc=fncptr;}

bool xg_init () {
	clock_gettime(CLOCK_MONOTONIC, &spec);
	ms_last = spec.tv_nsec;
	s_last = spec.tv_sec;

	dpy = XOpenDisplay(NULL);

	if (dpy == NULL) {return false;}	

	root = DefaultRootWindow(dpy);
	vi = glXChooseVisual(dpy, 0, glx_context_attributes);

	if (vi == NULL) {return false;}

	cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
	
	for (uint32_t i = 0; i < 256;++i){
		xg_ascii_keys[i] = false;	
	}
}

void xg_window (uint32_t width, uint32_t height, char* title){
	win = XCreateWindow(dpy, root, 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	xg_title = title;
	xg_width = width;
	xg_height = height;

	XMapWindow (dpy, win);
	XStoreName(dpy, win, title);

	Atom wmDelete = XInternAtom (dpy, "WM_DELETE_WINDOW", True);
	XSetWMProtocols (dpy, win, &wmDelete, 1);

	XAutoRepeatOff(dpy);
	
}

void xg_window_show () {
	XMapWindow (dpy, win);
	XStoreName(dpy, win, xg_title);
}

void xg_init_glx (){
	glc = glXCreateContext (dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
	xg_has_glx = true;
}

void xg_window_stop () {
	xg_running = false;
}

void xg_window_close () {
	
	xg_running = false;
	
	XAutoRepeatOn(dpy);

	if(xg_has_glx){
		glXMakeCurrent(dpy, None, NULL);
		glXDestroyContext(dpy, glc);
	}
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

void xg_window_set_not_resizable () {
	XSizeHints* hints = XAllocSizeHints();
	hints->flags = PMinSize | PMaxSize;
	hints->min_width = xg_width;
	hints->max_width = xg_width;
	hints->max_height = xg_height;
	hints->min_height = xg_height,
	XSetWMNormalHints(dpy, win, hints);
	XFree(hints);
}

bool xg_window_isopen (){
	return xg_running;
}

void xg_window_update (){
	clock_gettime(CLOCK_MONOTONIC, &spec);
	intmax_t s_current = spec.tv_sec;
	long ms_current = spec.tv_nsec;
	
	long ms_diff;
	if(s_current > s_last){ // if leap in second (nanosecond counter resets to 0 after 1 second)
		ms_diff = ms_current + (999999999 - ms_last);
	}else{
		ms_diff = ms_current - ms_last;
	}

	fTime = ms_diff / 1.0e9f; // convert to seconds
	s_last = s_current;
	ms_last = ms_current;

	if(xg_running)
	while (XPending (dpy) > 0 ){
		XNextEvent(dpy, & xev);

		switch (xev.type) {

			case ClientMessage: {
				xg_running = false;
				break;
			}

			case KeyPress : {
			
				char key [1];
				KeySym keysym;
				XLookupString (&xev.xkey, key, 1, &keysym, NULL);
				key[0] = tolower(key[0]);

				if (keysym >= 0xffe1 && keysym <= 0xffe6){ // Is a modifier key
					uint32_t arr_index = keysym - 0xffe1;
					xg_modif_keys[arr_index] = true;
				}else{
					xg_ascii_keys[key[0]] = true;
				}

				break;
			}

			case KeyRelease : {
			
				char key [1];
				KeySym keysym;
				XLookupString (&xev.xkey, key, 1, &keysym, NULL);
				key[0] = tolower(key[0]);

				if (keysym >= 0xffe1 && keysym <= 0xffe6){ // Is a modifier key
					uint32_t arr_index = keysym - 0xffe1;
					xg_modif_keys[arr_index] = false;
				}else{
					xg_ascii_keys[key[0]] = false;
				}

				break;    
			}
			
			case ButtonPress : {
				unsigned int btn = xev.xbutton.button;
				switch (btn){
					case Button1:button1fnc(true);break;
					case Button2:button2fnc(true);break;
					case Button3:button3fnc(true);break;
					case Button4:button4fnc(true);break;
					case Button5:button5fnc(true);break;
					default:break;
				}
				break;
			}
			
			case ButtonRelease : {
				unsigned int btn = xev.xbutton.button;
				switch (btn){
					case Button1:button1fnc(false);break;
					case Button2:button2fnc(false);break;
					case Button3:button3fnc(false);break;
					case Button4:button4fnc(false);break;
					case Button5:button5fnc(false);break;
					default:break;
				}
				break;
			}
			
			case MotionNotify : {
				_mouse_x = xev.xmotion.x;
				_mouse_y = xev.xmotion.y;
				break;
			}

			default: break;
		}
	}
}

void xg_glx_swap () {
	glXSwapBuffers(dpy, win);
}

bool xg_keyboard_ascii (uint32_t key){
	return xg_ascii_keys[key];	
}

bool xg_keyboard_modif (uint32_t keysym){
	uint32_t arr_index = keysym - 0xffe1;
	return xg_modif_keys[arr_index];
}

void xg_mouse_position (int32_t* x,int32_t* y){
	*x = _mouse_x;
	*y = _mouse_y;
}

void xg_set_mouse_position (int32_t x, int32_t y){
	XSelectInput ( dpy, win, ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask);
	XWarpPointer(dpy, None, win, 0, 0, 0, 0, x, y);
	_mouse_x = x;
	_mouse_y = y;
	XSelectInput ( dpy, win, ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
}

void xg_cursor_set (bool vis, uint8_t shape){
	if(vis){
		Cursor _cursor = XCreateFontCursor (dpy, shape);
		XDefineCursor(dpy, win, _cursor);
	}else{
		Pixmap _empty_map;
		XColor _black;
		char _nothing[] = {0,0,0,0,0,0,0,0};

		_empty_map = XCreateBitmapFromData(dpy, win, _nothing, 8, 8);
		Cursor _cursor = XCreatePixmapCursor(dpy, _empty_map, _empty_map, &_black, &_black, 0, 0);

		XDefineCursor(dpy, win, _cursor);
	}
}

float xg_get_ftime (){
	return fTime;
}


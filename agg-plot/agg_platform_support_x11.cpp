//----------------------------------------------------------------------------
// Anti-Grain Geometry (AGG) - Version 2.5
// A high quality rendering engine for C++
// Copyright (C) 2002-2006 Maxim Shemanarev
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://antigrain.com
// 
// AGG is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// AGG is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with AGG; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
// MA 02110-1301, USA.
//----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <pthread.h>
#include <new>
#include "agg_basics.h"
#include "util/agg_color_conv_rgb8.h"
#include "platform_support_ext.h"
#include "rect.h"

namespace agg
{
  //------------------------------------------------------------------------
  class platform_specific
  {
    typedef agg::rect_base<int> rect;

  public:
    platform_specific(pix_format_e format, bool flip_y);
    ~platform_specific();

    void free_x_resources();
        
    void caption(const char* capt);
    void put_image(const rendering_buffer* src, Display* d, const rect *r = 0);

    void close();
       
    pix_format_e         m_format;
    pix_format_e         m_sys_format;
    int                  m_byte_order;
    bool                 m_flip_y;
    unsigned             m_bpp;
    unsigned             m_sys_bpp;
    Display*             m_display;
    Display*             m_display_alt;
    int                  m_screen;
    int                  m_depth;
    Visual*              m_visual;
    Window               m_window;
    GC                   m_gc;
    XImage*              m_ximg_window;
    XSetWindowAttributes m_window_attributes;
    Atom                 m_close_atom;
    Atom                 m_wm_protocols_atom;
    unsigned char*       m_buf_window;
    unsigned char*       m_buf_img[platform_support::max_images];
    unsigned             m_keymap[256];
       
    bool m_update_flag;
    bool m_resize_flag;
    bool m_initialized;
    bool m_is_mapped;
    clock_t m_sw_start;

    pthread_mutex_t m_mutex[1];

    static bool initialized;
  };

  //------------------------------------------------------------------------
  platform_specific::platform_specific(pix_format_e format, bool flip_y) :
    m_format(format),
    m_sys_format(pix_format_undefined),
    m_byte_order(LSBFirst),
    m_flip_y(flip_y),
    m_bpp(0),
    m_sys_bpp(0),
    m_display(0), m_display_alt(0),
    m_screen(0),
    m_depth(0),
    m_visual(0),
    m_window(0),
    m_gc(0),
    m_ximg_window(0),
    m_close_atom(0),
    m_wm_protocols_atom(0),

    m_buf_window(0),
        
    m_update_flag(true), 
    m_resize_flag(true),
    m_initialized(false),
    m_is_mapped(false)
  {
    memset(m_buf_img, 0, sizeof(m_buf_img));

    unsigned i;
    for(i = 0; i < 256; i++)
      {
	m_keymap[i] = i;
      }

    m_keymap[XK_Pause&0xFF] = key_pause;
    m_keymap[XK_Clear&0xFF] = key_clear;

    m_keymap[XK_KP_0&0xFF] = key_kp0;
    m_keymap[XK_KP_1&0xFF] = key_kp1;
    m_keymap[XK_KP_2&0xFF] = key_kp2;
    m_keymap[XK_KP_3&0xFF] = key_kp3;
    m_keymap[XK_KP_4&0xFF] = key_kp4;
    m_keymap[XK_KP_5&0xFF] = key_kp5;
    m_keymap[XK_KP_6&0xFF] = key_kp6;
    m_keymap[XK_KP_7&0xFF] = key_kp7;
    m_keymap[XK_KP_8&0xFF] = key_kp8;
    m_keymap[XK_KP_9&0xFF] = key_kp9;

    m_keymap[XK_KP_Insert&0xFF]    = key_kp0;
    m_keymap[XK_KP_End&0xFF]       = key_kp1;   
    m_keymap[XK_KP_Down&0xFF]      = key_kp2;
    m_keymap[XK_KP_Page_Down&0xFF] = key_kp3;
    m_keymap[XK_KP_Left&0xFF]      = key_kp4;
    m_keymap[XK_KP_Begin&0xFF]     = key_kp5;
    m_keymap[XK_KP_Right&0xFF]     = key_kp6;
    m_keymap[XK_KP_Home&0xFF]      = key_kp7;
    m_keymap[XK_KP_Up&0xFF]        = key_kp8;
    m_keymap[XK_KP_Page_Up&0xFF]   = key_kp9;
    m_keymap[XK_KP_Delete&0xFF]    = key_kp_period;
    m_keymap[XK_KP_Decimal&0xFF]   = key_kp_period;
    m_keymap[XK_KP_Divide&0xFF]    = key_kp_divide;
    m_keymap[XK_KP_Multiply&0xFF]  = key_kp_multiply;
    m_keymap[XK_KP_Subtract&0xFF]  = key_kp_minus;
    m_keymap[XK_KP_Add&0xFF]       = key_kp_plus;
    m_keymap[XK_KP_Enter&0xFF]     = key_kp_enter;
    m_keymap[XK_KP_Equal&0xFF]     = key_kp_equals;

    m_keymap[XK_Up&0xFF]           = key_up;
    m_keymap[XK_Down&0xFF]         = key_down;
    m_keymap[XK_Right&0xFF]        = key_right;
    m_keymap[XK_Left&0xFF]         = key_left;
    m_keymap[XK_Insert&0xFF]       = key_insert;
    m_keymap[XK_Home&0xFF]         = key_delete;
    m_keymap[XK_End&0xFF]          = key_end;
    m_keymap[XK_Page_Up&0xFF]      = key_page_up;
    m_keymap[XK_Page_Down&0xFF]    = key_page_down;

    m_keymap[XK_F1&0xFF]           = key_f1;
    m_keymap[XK_F2&0xFF]           = key_f2;
    m_keymap[XK_F3&0xFF]           = key_f3;
    m_keymap[XK_F4&0xFF]           = key_f4;
    m_keymap[XK_F5&0xFF]           = key_f5;
    m_keymap[XK_F6&0xFF]           = key_f6;
    m_keymap[XK_F7&0xFF]           = key_f7;
    m_keymap[XK_F8&0xFF]           = key_f8;
    m_keymap[XK_F9&0xFF]           = key_f9;
    m_keymap[XK_F10&0xFF]          = key_f10;
    m_keymap[XK_F11&0xFF]          = key_f11;
    m_keymap[XK_F12&0xFF]          = key_f12;
    m_keymap[XK_F13&0xFF]          = key_f13;
    m_keymap[XK_F14&0xFF]          = key_f14;
    m_keymap[XK_F15&0xFF]          = key_f15;

    m_keymap[XK_Num_Lock&0xFF]     = key_numlock;
    m_keymap[XK_Caps_Lock&0xFF]    = key_capslock;
    m_keymap[XK_Scroll_Lock&0xFF]  = key_scrollock;

    switch(m_format)
      {
      default: break;
      case pix_format_gray8:
	m_bpp = 8;
	break;

      case pix_format_rgb565:
      case pix_format_rgb555:
	m_bpp = 16;
	break;

      case pix_format_rgb24:
      case pix_format_bgr24:
	m_bpp = 24;
	break;

      case pix_format_bgra32:
      case pix_format_abgr32:
      case pix_format_argb32:
      case pix_format_rgba32:
	m_bpp = 32;
	break;
      }
    m_sw_start = clock();

    pthread_mutex_init (m_mutex, NULL);
  }

  //------------------------------------------------------------------------
  platform_specific::~platform_specific()
  {
    pthread_mutex_destroy (m_mutex);
  }

  void platform_specific::free_x_resources()
  {
    delete [] m_buf_window;
    m_ximg_window->data = 0;
    XDestroyImage(m_ximg_window);
    XFreeGC(m_display, m_gc);
    XCloseDisplay(m_display);
    XCloseDisplay(m_display_alt);
  }

  void platform_specific::close()
  {
    XEvent ev;
 
    ev.xclient.type = ClientMessage;
    ev.xclient.window = m_window;
    ev.xclient.message_type = m_wm_protocols_atom;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = m_close_atom; 
    ev.xclient.data.l[1] = CurrentTime;
    ev.xclient.data.l[2] = 0l;
    ev.xclient.data.l[3] = 0l;
    ev.xclient.data.l[4] = 0l;
    XSendEvent(m_display_alt, m_window, False, NoEventMask, &ev);
    XSync(m_display_alt, False);
  }

  //------------------------------------------------------------------------
  void platform_specific::caption(const char* capt)
  {
    // Fixed by Enno Fennema (in original AGG library)
    XStoreName(m_display, m_window, capt);
    XSetIconName(m_display, m_window, capt);
  }

    
  //------------------------------------------------------------------------
  void platform_specific::put_image(const rendering_buffer* src,
				    Display *dsp, const rect *ri)
  {
    if(m_ximg_window == 0) return;

    m_ximg_window->data = (char*)m_buf_window;

    if (m_format == m_sys_format && ri == 0)
      {
	XPutImage(dsp, m_window, m_gc, m_ximg_window, 
		  0, 0, 0, 0, src->width(), src->height());
      }

    opt_rect<int> optr(0, 0, src->width(), src->height());
    if (ri)
      optr.add<rect_intersect>(*ri);

    const agg::rect_base<int>& r = optr.rect();
    int x = r.x1, y = r.y1, w = r.x2 - r.x1, h = r.y2 - r.y1;

    int row_len = w * m_sys_bpp / 8;
    unsigned char* buf_tmp = new(std::nothrow) unsigned char[row_len * h];
    if (buf_tmp == 0)
      return;
      
    rendering_buffer rbuf_tmp;
    rbuf_tmp.attach(buf_tmp, w, h, m_flip_y ? -row_len : row_len);

    rendering_buffer_ro src_view;
    rendering_buffer_get_view(src_view, *src, r, m_bpp / 8, m_flip_y);

    if (m_format == m_sys_format)
      {
	rbuf_tmp.copy_from(src_view);
      }
    else
      {
	switch(m_sys_format)            
	  {
	  default: break;
	  case pix_format_rgb555:
	    switch(m_format)
	      {
	      default: break;
	      case pix_format_rgb555: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb555_to_rgb555()); break;
	      case pix_format_rgb565: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb565_to_rgb555()); break;
	      case pix_format_rgb24:  my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb24_to_rgb555());  break;
	      case pix_format_bgr24:  my_color_conv(&rbuf_tmp, &src_view, color_conv_bgr24_to_rgb555());  break;
	      case pix_format_rgba32: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgba32_to_rgb555()); break;
	      case pix_format_argb32: my_color_conv(&rbuf_tmp, &src_view, color_conv_argb32_to_rgb555()); break;
	      case pix_format_bgra32: my_color_conv(&rbuf_tmp, &src_view, color_conv_bgra32_to_rgb555()); break;
	      case pix_format_abgr32: my_color_conv(&rbuf_tmp, &src_view, color_conv_abgr32_to_rgb555()); break;
	      }
	    break;
                    
	  case pix_format_rgb565:
	    switch(m_format)
	      {
	      default: break;
	      case pix_format_rgb555: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb555_to_rgb565()); break;
	      case pix_format_rgb565: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb565_to_rgb565()); break;
	      case pix_format_rgb24:  my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb24_to_rgb565());  break;
	      case pix_format_bgr24:  my_color_conv(&rbuf_tmp, &src_view, color_conv_bgr24_to_rgb565());  break;
	      case pix_format_rgba32: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgba32_to_rgb565()); break;
	      case pix_format_argb32: my_color_conv(&rbuf_tmp, &src_view, color_conv_argb32_to_rgb565()); break;
	      case pix_format_bgra32: my_color_conv(&rbuf_tmp, &src_view, color_conv_bgra32_to_rgb565()); break;
	      case pix_format_abgr32: my_color_conv(&rbuf_tmp, &src_view, color_conv_abgr32_to_rgb565()); break;
	      }
	    break;
                    
	  case pix_format_rgba32:
	    switch(m_format)
	      {
	      default: break;
	      case pix_format_rgb555: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb555_to_rgba32()); break;
	      case pix_format_rgb565: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb565_to_rgba32()); break;
	      case pix_format_rgb24:  my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb24_to_rgba32());  break;
	      case pix_format_bgr24:  my_color_conv(&rbuf_tmp, &src_view, color_conv_bgr24_to_rgba32());  break;
	      case pix_format_rgba32: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgba32_to_rgba32()); break;
	      case pix_format_argb32: my_color_conv(&rbuf_tmp, &src_view, color_conv_argb32_to_rgba32()); break;
	      case pix_format_bgra32: my_color_conv(&rbuf_tmp, &src_view, color_conv_bgra32_to_rgba32()); break;
	      case pix_format_abgr32: my_color_conv(&rbuf_tmp, &src_view, color_conv_abgr32_to_rgba32()); break;
	      }
	    break;
                    
	  case pix_format_abgr32:
	    switch(m_format)
	      {
	      default: break;
	      case pix_format_rgb555: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb555_to_abgr32()); break;
	      case pix_format_rgb565: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb565_to_abgr32()); break;
	      case pix_format_rgb24:  my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb24_to_abgr32());  break;
	      case pix_format_bgr24:  my_color_conv(&rbuf_tmp, &src_view, color_conv_bgr24_to_abgr32());  break;
	      case pix_format_abgr32: my_color_conv(&rbuf_tmp, &src_view, color_conv_abgr32_to_abgr32()); break;
	      case pix_format_rgba32: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgba32_to_abgr32()); break;
	      case pix_format_argb32: my_color_conv(&rbuf_tmp, &src_view, color_conv_argb32_to_abgr32()); break;
	      case pix_format_bgra32: my_color_conv(&rbuf_tmp, &src_view, color_conv_bgra32_to_abgr32()); break;
	      }
	    break;
                    
	  case pix_format_argb32:
	    switch(m_format)
	      {
	      default: break;
	      case pix_format_rgb555: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb555_to_argb32()); break;
	      case pix_format_rgb565: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb565_to_argb32()); break;
	      case pix_format_rgb24:  my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb24_to_argb32());  break;
	      case pix_format_bgr24:  my_color_conv(&rbuf_tmp, &src_view, color_conv_bgr24_to_argb32());  break;
	      case pix_format_rgba32: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgba32_to_argb32()); break;
	      case pix_format_argb32: my_color_conv(&rbuf_tmp, &src_view, color_conv_argb32_to_argb32()); break;
	      case pix_format_abgr32: my_color_conv(&rbuf_tmp, &src_view, color_conv_abgr32_to_argb32()); break;
	      case pix_format_bgra32: my_color_conv(&rbuf_tmp, &src_view, color_conv_bgra32_to_argb32()); break;
	      }
	    break;
                    
	  case pix_format_bgra32:
	    switch(m_format)
	      {
	      default: break;
	      case pix_format_rgb555: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb555_to_bgra32()); break;
	      case pix_format_rgb565: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb565_to_bgra32()); break;
	      case pix_format_rgb24:  my_color_conv(&rbuf_tmp, &src_view, color_conv_rgb24_to_bgra32());  break;
	      case pix_format_bgr24:  my_color_conv(&rbuf_tmp, &src_view, color_conv_bgr24_to_bgra32());  break;
	      case pix_format_rgba32: my_color_conv(&rbuf_tmp, &src_view, color_conv_rgba32_to_bgra32()); break;
	      case pix_format_argb32: my_color_conv(&rbuf_tmp, &src_view, color_conv_argb32_to_bgra32()); break;
	      case pix_format_abgr32: my_color_conv(&rbuf_tmp, &src_view, color_conv_abgr32_to_bgra32()); break;
	      case pix_format_bgra32: my_color_conv(&rbuf_tmp, &src_view, color_conv_bgra32_to_bgra32()); break;
	      }
	    break;
	  }
      }

    XImage *img = XCreateImage(dsp, m_visual, m_depth, 
			       ZPixmap, 0, (char*) buf_tmp,
			       w, h, m_sys_bpp,
			       w * (m_sys_bpp / 8));

    img->byte_order = m_byte_order;

    int y_dst = (m_flip_y ? src->height() - (y + h) : y);
    XPutImage(dsp, m_window, m_gc, img, 0, 0, x, y_dst, w, h);
            
    delete [] buf_tmp;

    img->data = 0;
    XDestroyImage(img);
  }

  //------------------------------------------------------------------------
  platform_support::platform_support(pix_format_e format, bool flip_y) :
    m_specific(new platform_specific(format, flip_y)),
    m_format(format),
    m_bpp(m_specific->m_bpp),
    m_window_flags(0),
    m_wait_mode(true),
    m_flip_y(flip_y),
    m_initial_width(10),
    m_initial_height(10)
  {
    strcpy(m_caption, "AGG Application");
  }

  //------------------------------------------------------------------------
  platform_support::~platform_support()
  {
    delete m_specific;
  }



  //------------------------------------------------------------------------
  void platform_support::caption(const char* cap)
  {
    strcpy(m_caption, cap);
    if(m_specific->m_initialized)
      {
	m_specific->caption(cap);
      }
  }

   
  //------------------------------------------------------------------------
  enum xevent_mask_e
    { 
      xevent_mask =
      PointerMotionMask|
      ButtonPressMask|
      ButtonReleaseMask|
      ExposureMask|
      KeyPressMask|
      StructureNotifyMask
    };


  //------------------------------------------------------------------------
  bool platform_support::init(unsigned width, unsigned height, unsigned flags)
  {
    m_window_flags = flags;

    m_specific->m_display = XOpenDisplay(NULL);
    m_specific->m_display_alt = XOpenDisplay(NULL);
    if(m_specific->m_display == 0 || m_specific->m_display_alt == 0) 
      {
	fprintf(stderr, "Unable to open DISPLAY!\n");
	return false;
      }
        
    m_specific->m_screen = XDefaultScreen(m_specific->m_display);
    m_specific->m_depth  = XDefaultDepth(m_specific->m_display, 
					 m_specific->m_screen);
    m_specific->m_visual = XDefaultVisual(m_specific->m_display, 
					  m_specific->m_screen);
    unsigned long r_mask = m_specific->m_visual->red_mask;
    unsigned long g_mask = m_specific->m_visual->green_mask;
    unsigned long b_mask = m_specific->m_visual->blue_mask;

    if(m_specific->m_depth < 15 ||
       r_mask == 0 || g_mask == 0 || b_mask == 0)
      {
	fprintf(stderr,
		"There's no Visual compatible with minimal AGG requirements:\n"
		"At least 15-bit color depth and True- or DirectColor class.\n\n");
	XCloseDisplay(m_specific->m_display);
	XCloseDisplay(m_specific->m_display_alt);
	return false;
      }
        
    int t = 1;
    int hw_byte_order = LSBFirst;
    if(*(char*)&t == 0) hw_byte_order = MSBFirst;
        
    // Perceive SYS-format by mask
    switch(m_specific->m_depth)
      {
      case 15:
	m_specific->m_sys_bpp = 16;
	if(r_mask == 0x7C00 && g_mask == 0x3E0 && b_mask == 0x1F)
	  {
	    m_specific->m_sys_format = pix_format_rgb555;
	    m_specific->m_byte_order = hw_byte_order;
	  }
	break;
                
      case 16:
	m_specific->m_sys_bpp = 16;
	if(r_mask == 0xF800 && g_mask == 0x7E0 && b_mask == 0x1F)
	  {
	    m_specific->m_sys_format = pix_format_rgb565;
	    m_specific->m_byte_order = hw_byte_order;
	  }
	break;
                
      case 24:
      case 32:
	m_specific->m_sys_bpp = 32;
	if(g_mask == 0xFF00)
	  {
	    if(r_mask == 0xFF && b_mask == 0xFF0000)
	      {
		switch(m_specific->m_format)
		  {
		  case pix_format_rgba32:
		    m_specific->m_sys_format = pix_format_rgba32;
		    m_specific->m_byte_order = LSBFirst;
		    break;
                                
		  case pix_format_abgr32:
		    m_specific->m_sys_format = pix_format_abgr32;
		    m_specific->m_byte_order = MSBFirst;
		    break;

		  default:                            
		    m_specific->m_byte_order = hw_byte_order;
		    m_specific->m_sys_format = 
		      (hw_byte_order == LSBFirst) ?
		      pix_format_rgba32 :
		      pix_format_abgr32;
		    break;
		  }
	      }
                    
	    if(r_mask == 0xFF0000 && b_mask == 0xFF)
	      {
		switch(m_specific->m_format)
		  {
		  case pix_format_argb32:
		    m_specific->m_sys_format = pix_format_argb32;
		    m_specific->m_byte_order = MSBFirst;
		    break;
                                
		  case pix_format_bgra32:
		    m_specific->m_sys_format = pix_format_bgra32;
		    m_specific->m_byte_order = LSBFirst;
		    break;

		  default:                            
		    m_specific->m_byte_order = hw_byte_order;
		    m_specific->m_sys_format = 
		      (hw_byte_order == MSBFirst) ?
		      pix_format_argb32 :
		      pix_format_bgra32;
		    break;
		  }
	      }
	  }
	break;
      }
        
    if(m_specific->m_sys_format == pix_format_undefined)
      {
	fprintf(stderr,
		"RGB masks are not compatible with AGG pixel formats:\n"
		"R=%08x, R=%08x, B=%08x\n", 
		(unsigned)r_mask, (unsigned)g_mask, (unsigned)b_mask);
	XCloseDisplay(m_specific->m_display);
	XCloseDisplay(m_specific->m_display_alt);
	return false;
      }
                
        
        
    memset(&m_specific->m_window_attributes, 
	   0, 
	   sizeof(m_specific->m_window_attributes)); 
        
    m_specific->m_window_attributes.border_pixel = 
      XBlackPixel(m_specific->m_display, m_specific->m_screen);

    m_specific->m_window_attributes.background_pixel = 
      XWhitePixel(m_specific->m_display, m_specific->m_screen);

    m_specific->m_window_attributes.override_redirect = 0;

    unsigned long window_mask = CWBackPixel | CWBorderPixel;

    m_specific->m_window = 
      XCreateWindow(m_specific->m_display, 
		    XDefaultRootWindow(m_specific->m_display), 
		    0, 0,
		    width,
		    height,
		    0, 
		    m_specific->m_depth, 
		    InputOutput, 
		    CopyFromParent,
		    window_mask,
		    &m_specific->m_window_attributes);


    m_specific->m_gc = XCreateGC(m_specific->m_display, 
				 m_specific->m_window, 
				 0, 0);

    unsigned int buf_size = width * height * (m_bpp / 8);
    m_specific->m_buf_window = new(std::nothrow) unsigned char[buf_size];
    if (m_specific->m_buf_window == 0)
      {
	XFreeGC(m_specific->m_display, m_specific->m_gc);
	XDestroyWindow(m_specific->m_display, m_specific->m_window);
	XCloseDisplay(m_specific->m_display);
	XCloseDisplay(m_specific->m_display_alt);
	return false;
      }

    memset(m_specific->m_buf_window, 255, width * height * (m_bpp / 8));
        
    m_rbuf_window.attach(m_specific->m_buf_window,
			 width,
			 height,
			 m_flip_y ? -width * (m_bpp / 8) : width * (m_bpp / 8));
            
    m_specific->m_ximg_window = 
      XCreateImage(m_specific->m_display, 
		   m_specific->m_visual, //CopyFromParent, 
		   m_specific->m_depth, 
		   ZPixmap, 
		   0,
		   (char*)m_specific->m_buf_window, 
		   width,
		   height, 
		   m_specific->m_sys_bpp,
		   width * (m_specific->m_sys_bpp / 8));
    m_specific->m_ximg_window->byte_order = m_specific->m_byte_order;

    m_specific->caption(m_caption); 
    m_initial_width = width;
    m_initial_height = height;
        
    if(!m_specific->m_initialized)
      {
	on_init();
	m_specific->m_initialized = true;
      }

    trans_affine_resizing(width, height);
    on_resize(width, height);
    m_specific->m_update_flag = true;

    XSizeHints *hints = XAllocSizeHints();
    if(hints) 
      {
	if(flags & window_resize)
	  {
	    hints->min_width = 32;
	    hints->min_height = 32;
	    hints->max_width = 4096;
	    hints->max_height = 4096;
	  }
	else
	  {
	    hints->min_width  = width;
	    hints->min_height = height;
	    hints->max_width  = width;
	    hints->max_height = height;
	  }
	hints->flags = PMaxSize | PMinSize;

	XSetWMNormalHints(m_specific->m_display, 
			  m_specific->m_window, 
			  hints);

	XFree(hints);
      }


    XMapWindow(m_specific->m_display, 
	       m_specific->m_window);

    XSelectInput(m_specific->m_display, 
		 m_specific->m_window, 
		 xevent_mask);

        
    m_specific->m_close_atom = XInternAtom(m_specific->m_display, 
					   "WM_DELETE_WINDOW", 
					   false);

    m_specific->m_wm_protocols_atom = XInternAtom(m_specific->m_display, 
						  "WM_PROTOCOLS", 
						  true);

    XSetWMProtocols(m_specific->m_display, 
		    m_specific->m_window, 
		    &m_specific->m_close_atom, 
		    1);

    return true;
  }

  //------------------------------------------------------------------------
  void platform_support::update_window()
  {
    if (! m_specific->m_is_mapped)
      return;

    m_specific->put_image(&m_rbuf_window, m_specific->m_display);
        
    // When m_wait_mode is true we can discard all the events 
    // came while the image is being drawn. In this case 
    // the X server does not accumulate mouse motion events.
    // When m_wait_mode is false, i.e. we have some idle drawing
    // we cannot afford to miss any events
    XSync(m_specific->m_display, m_wait_mode);
  }


  //------------------------------------------------------------------------
  int platform_support::run()
  {
    XFlush(m_specific->m_display);
        
    bool quit = false;
    unsigned flags;
    int cur_x;
    int cur_y;
    int ret = 0;

    while(!quit)
      {
	if(m_specific->m_update_flag && m_specific->m_is_mapped)
	  {
	    on_draw();
	    update_window();
	    m_specific->m_update_flag = false;
	  }

	if(!m_wait_mode)
	  {
	    if(XPending(m_specific->m_display) == 0)
	      {
		on_idle();
		continue;
	      }
	  }

	XEvent x_event;
	if (m_specific->m_is_mapped)
	  {
	    pthread_mutex_unlock (m_specific->m_mutex);
	    XNextEvent(m_specific->m_display, &x_event);
	    pthread_mutex_lock (m_specific->m_mutex);
	  }
	else
	  {
	    XNextEvent(m_specific->m_display, &x_event);
	  }
            
	// In the Idle mode discard all intermediate MotionNotify events
	if(!m_wait_mode && x_event.type == MotionNotify)
	  {
	    XEvent te = x_event;
	    for(;;)
	      {
		if(XPending(m_specific->m_display) == 0) break;
		XNextEvent(m_specific->m_display, &te);
		if(te.type != MotionNotify) break;
	      }
	    x_event = te;
	  }

	switch(x_event.type) 
	  {
	  case MapNotify: 
	    {
	      on_draw();
	      update_window();
	      m_specific->m_is_mapped = true;
	      m_specific->m_update_flag = false;
	    }
	    break;

	  case ConfigureNotify: 
	    {
	      if(x_event.xconfigure.width  != int(m_rbuf_window.width()) ||
		 x_event.xconfigure.height != int(m_rbuf_window.height()))
		{
		  int width  = x_event.xconfigure.width;
		  int height = x_event.xconfigure.height;

		  unsigned int bufsz = width * height * (m_bpp / 8);
		  unsigned char *newbuf = new(std::nothrow) unsigned char[bufsz];
		  if (newbuf == 0)
		    {
		      quit = true;
		      ret = 1;
		      break;
		    }

		  delete [] m_specific->m_buf_window;
		  m_specific->m_ximg_window->data = 0;
		  XDestroyImage(m_specific->m_ximg_window);

		  m_specific->m_buf_window = newbuf;

		  m_rbuf_window.attach(m_specific->m_buf_window,
				       width,
				       height,
				       m_flip_y ? 
				       -width * (m_bpp / 8) : 
				       width * (m_bpp / 8));
            
		  m_specific->m_ximg_window = 
		    XCreateImage(m_specific->m_display, 
				 m_specific->m_visual, //CopyFromParent, 
				 m_specific->m_depth, 
				 ZPixmap, 
				 0,
				 (char*)m_specific->m_buf_window, 
				 width,
				 height, 
				 m_specific->m_sys_bpp,
				 width * (m_specific->m_sys_bpp / 8));
		  m_specific->m_ximg_window->byte_order = m_specific->m_byte_order;

		  trans_affine_resizing(width, height);
		  on_resize(width, height);
		  on_draw();
		  update_window();
		}
	    }
	    break;

	  case Expose:
	    m_specific->put_image(&m_rbuf_window, m_specific->m_display);
	    XFlush(m_specific->m_display);
	    XSync(m_specific->m_display, false);
	    break;

	  case KeyPress:
	    {
	      KeySym key = XLookupKeysym(&x_event.xkey, 0);
	      flags = 0;
	      if(x_event.xkey.state & Button1Mask) flags |= mouse_left;
	      if(x_event.xkey.state & Button3Mask) flags |= mouse_right;
	      if(x_event.xkey.state & ShiftMask)   flags |= kbd_shift;
	      if(x_event.xkey.state & ControlMask) flags |= kbd_ctrl;

	      bool left  = false;
	      bool up    = false;
	      bool right = false;
	      bool down  = false;

	      switch(m_specific->m_keymap[key & 0xFF])
		{
		case key_left:
		  left = true;
		  break;

		case key_up:
		  up = true;
		  break;

		case key_right:
		  right = true;
		  break;

		case key_down:
		  down = true;
		  break;

		case key_f2:                        
		  copy_window_to_img(max_images - 1);
		  save_img(max_images - 1, "screenshot");
		  break;
		}

	      if(m_ctrls.on_arrow_keys(left, right, down, up))
		{
		  on_ctrl_change();
		  force_redraw();
		}
	      else
		{
		  on_key(x_event.xkey.x, 
			 m_flip_y ? 
			 m_rbuf_window.height() - x_event.xkey.y :
			 x_event.xkey.y,
			 m_specific->m_keymap[key & 0xFF],
			 flags);
		}
	    }
	    break;


	  case ButtonPress:
	    {
	      flags = 0;
	      if(x_event.xbutton.state & ShiftMask)   flags |= kbd_shift;
	      if(x_event.xbutton.state & ControlMask) flags |= kbd_ctrl;
	      if(x_event.xbutton.button == Button1)   flags |= mouse_left;
	      if(x_event.xbutton.button == Button3)   flags |= mouse_right;

	      cur_x = x_event.xbutton.x;
	      cur_y = m_flip_y ? m_rbuf_window.height() - x_event.xbutton.y :
		x_event.xbutton.y;

	      if(flags & mouse_left)
		{
		  if(m_ctrls.on_mouse_button_down(cur_x, cur_y))
		    {
		      m_ctrls.set_cur(cur_x, cur_y);
		      on_ctrl_change();
		      force_redraw();
		    }
		  else
		    {
		      if(m_ctrls.in_rect(cur_x, cur_y))
			{
			  if(m_ctrls.set_cur(cur_x, cur_y))
			    {
			      on_ctrl_change();
			      force_redraw();
			    }
			}
		      else
			{
			  on_mouse_button_down(cur_x, cur_y, flags);
			}
		    }
		}
	      if(flags & mouse_right)
		{
		  on_mouse_button_down(cur_x, cur_y, flags);
		}
	      //m_specific->m_wait_mode = m_wait_mode;
	      //m_wait_mode = true;
	    }
	    break;

                
	  case MotionNotify:
	    {
	      flags = 0;
	      if(x_event.xmotion.state & Button1Mask) flags |= mouse_left;
	      if(x_event.xmotion.state & Button3Mask) flags |= mouse_right;
	      if(x_event.xmotion.state & ShiftMask)   flags |= kbd_shift;
	      if(x_event.xmotion.state & ControlMask) flags |= kbd_ctrl;

	      cur_x = x_event.xbutton.x;
	      cur_y = m_flip_y ? m_rbuf_window.height() - x_event.xbutton.y :
		x_event.xbutton.y;

	      if(m_ctrls.on_mouse_move(cur_x, cur_y, (flags & mouse_left) != 0))
		{
		  on_ctrl_change();
		  force_redraw();
		}
	      else
		{
		  if(!m_ctrls.in_rect(cur_x, cur_y))
		    {
		      on_mouse_move(cur_x, cur_y, flags);
		    }
		}
	    }
	    break;
                
	  case ButtonRelease:
	    {
	      flags = 0;
	      if(x_event.xbutton.state & ShiftMask)   flags |= kbd_shift;
	      if(x_event.xbutton.state & ControlMask) flags |= kbd_ctrl;
	      if(x_event.xbutton.button == Button1)   flags |= mouse_left;
	      if(x_event.xbutton.button == Button3)   flags |= mouse_right;

	      cur_x = x_event.xbutton.x;
	      cur_y = m_flip_y ? m_rbuf_window.height() - x_event.xbutton.y :
		x_event.xbutton.y;

	      if(flags & mouse_left)
		{
		  if(m_ctrls.on_mouse_button_up(cur_x, cur_y))
		    {
		      on_ctrl_change();
		      force_redraw();
		    }
		}
	      if(flags & (mouse_left | mouse_right))
		{
		  on_mouse_button_up(cur_x, cur_y, flags);
		}
	    }
	    //m_wait_mode = m_specific->m_wait_mode;
	    break;

	  case ClientMessage:
	    if((x_event.xclient.format == 32) &&
	       (x_event.xclient.data.l[0] == int(m_specific->m_close_atom)))
	      {
		quit = true;
	      }
	    break;
	  }           
      }

    unsigned i = platform_support::max_images;
    while(i--)
      {
	if(m_specific->m_buf_img[i]) 
	  {
	    delete [] m_specific->m_buf_img[i];
	  }
      }

    m_specific->free_x_resources();
    m_specific->m_initialized = false;

    return ret;
  }



  //------------------------------------------------------------------------
  const char* platform_support::img_ext() const { return ".ppm"; }

  //------------------------------------------------------------------------
  const char* platform_support::full_file_name(const char* file_name)
  {
    return file_name;
  }

  //------------------------------------------------------------------------
  bool platform_support::load_img(unsigned idx, const char* file)
  {
    if(idx < max_images)
      {
	char buf[1024];
	strcpy(buf, file);
	int len = strlen(buf);
	if(len < 4 || strcasecmp(buf + len - 4, ".ppm") != 0)
	  {
	    strcat(buf, ".ppm");
	  }
            
	FILE* fd = fopen(buf, "rb");
	if(fd == 0) return false;

	if((len = fread(buf, 1, 1022, fd)) == 0)
	  {
	    fclose(fd);
	    return false;
	  }
	buf[len] = 0;
            
	if(buf[0] != 'P' && buf[1] != '6')
	  {
	    fclose(fd);
	    return false;
	  }
            
	char* ptr = buf + 2;
            
	while(*ptr && !isdigit(*ptr)) ptr++;
	if(*ptr == 0)
	  {
	    fclose(fd);
	    return false;
	  }
            
	unsigned width = atoi(ptr);
	if(width == 0 || width > 4096)
	  {
	    fclose(fd);
	    return false;
	  }
	while(*ptr && isdigit(*ptr)) ptr++;
	while(*ptr && !isdigit(*ptr)) ptr++;
	if(*ptr == 0)
	  {
	    fclose(fd);
	    return false;
	  }
	unsigned height = atoi(ptr);
	if(height == 0 || height > 4096)
	  {
	    fclose(fd);
	    return false;
	  }
	while(*ptr && isdigit(*ptr)) ptr++;
	while(*ptr && !isdigit(*ptr)) ptr++;
	if(atoi(ptr) != 255)
	  {
	    fclose(fd);
	    return false;
	  }
	while(*ptr && isdigit(*ptr)) ptr++;
	if(*ptr == 0)
	  {
	    fclose(fd);
	    return false;
	  }
	ptr++;
	fseek(fd, long(ptr - buf), SEEK_SET);
            
	create_img(idx, width, height);
	bool ret = true;
            
	if(m_format == pix_format_rgb24)
	  {
	    fread(m_specific->m_buf_img[idx], 1, width * height * 3, fd);
	  }
	else
	  {
	    unsigned char* buf_img = new unsigned char [width * height * 3];
	    rendering_buffer rbuf_img;
	    rbuf_img.attach(buf_img,
			    width,
			    height,
			    m_flip_y ?
			    -width * 3 :
			    width * 3);
                
	    fread(buf_img, 1, width * height * 3, fd);
                
	    switch(m_format)
	      {
	      case pix_format_rgb555:
		color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_rgb555());
		break;
                        
	      case pix_format_rgb565:
		color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_rgb565());
		break;
                        
	      case pix_format_bgr24:
		color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_bgr24());
		break;
                        
	      case pix_format_rgba32:
		color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_rgba32());
		break;
                        
	      case pix_format_argb32:
		color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_argb32());
		break;
                        
	      case pix_format_bgra32:
		color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_bgra32());
		break;
                        
	      case pix_format_abgr32:
		color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_abgr32());
		break;
                        
	      default:
		ret = false;
	      }
	    delete [] buf_img;
	  }
                        
	fclose(fd);
	return ret;
      }
    return false;
  }




  //------------------------------------------------------------------------
  bool platform_support::save_img(unsigned idx, const char* file)
  {
    if(idx < max_images &&  rbuf_img(idx).buf())
      {
	char buf[1024];
	strcpy(buf, file);
	int len = strlen(buf);
	if(len < 4 || strcasecmp(buf + len - 4, ".ppm") != 0)
	  {
	    strcat(buf, ".ppm");
	  }
            
	FILE* fd = fopen(buf, "wb");
	if(fd == 0) return false;
            
	unsigned w = rbuf_img(idx).width();
	unsigned h = rbuf_img(idx).height();
            
	fprintf(fd, "P6\n%d %d\n255\n", w, h);
                
	unsigned y; 
	unsigned char* tmp_buf = new unsigned char [w * 3];
	for(y = 0; y < rbuf_img(idx).height(); y++)
	  {
	    const unsigned char* src = rbuf_img(idx).row_ptr(m_flip_y ? h - 1 - y : y);
	    switch(m_format)
	      {
	      default: break;
	      case pix_format_rgb555:
		color_conv_row(tmp_buf, src, w, color_conv_rgb555_to_rgb24());
		break;
                        
	      case pix_format_rgb565:
		color_conv_row(tmp_buf, src, w, color_conv_rgb565_to_rgb24());
		break;
                        
	      case pix_format_bgr24:
		color_conv_row(tmp_buf, src, w, color_conv_bgr24_to_rgb24());
		break;
                        
	      case pix_format_rgb24:
		color_conv_row(tmp_buf, src, w, color_conv_rgb24_to_rgb24());
		break;
                       
	      case pix_format_rgba32:
		color_conv_row(tmp_buf, src, w, color_conv_rgba32_to_rgb24());
		break;
                        
	      case pix_format_argb32:
		color_conv_row(tmp_buf, src, w, color_conv_argb32_to_rgb24());
		break;
                        
	      case pix_format_bgra32:
		color_conv_row(tmp_buf, src, w, color_conv_bgra32_to_rgb24());
		break;
                        
	      case pix_format_abgr32:
		color_conv_row(tmp_buf, src, w, color_conv_abgr32_to_rgb24());
		break;
	      }
	    fwrite(tmp_buf, 1, w * 3, fd);
	  }
	delete [] tmp_buf;
	fclose(fd);
	return true;
      }
    return false;
  }



  //------------------------------------------------------------------------
  bool platform_support::create_img(unsigned idx, unsigned width, unsigned height)
  {
    if(idx < max_images)
      {
	if(width  == 0) width  = rbuf_window().width();
	if(height == 0) height = rbuf_window().height();
	delete [] m_specific->m_buf_img[idx];
	m_specific->m_buf_img[idx] = 
	  new unsigned char[width * height * (m_bpp / 8)];

	m_rbuf_img[idx].attach(m_specific->m_buf_img[idx],
			       width,
			       height,
			       m_flip_y ? 
			       -width * (m_bpp / 8) : 
			       width * (m_bpp / 8));
	return true;
      }
    return false;
  }


  //------------------------------------------------------------------------
  void platform_support::force_redraw()
  {
    m_specific->m_update_flag = true;
  }


  //------------------------------------------------------------------------
  void platform_support::message(const char* msg)
  {
    fprintf(stderr, "%s\n", msg);
  }

  //------------------------------------------------------------------------
  void platform_support::start_timer()
  {
    m_specific->m_sw_start = clock();
  }

  //------------------------------------------------------------------------
  double platform_support::elapsed_time() const
  {
    clock_t stop = clock();
    return double(stop - m_specific->m_sw_start) * 1000.0 / CLOCKS_PER_SEC;
  }


  //------------------------------------------------------------------------
  void platform_support::on_init() {}
  void platform_support::on_resize(int sx, int sy) {}
  void platform_support::on_idle() {}
  void platform_support::on_mouse_move(int x, int y, unsigned flags) {}
  void platform_support::on_mouse_button_down(int x, int y, unsigned flags) {}
  void platform_support::on_mouse_button_up(int x, int y, unsigned flags) {}
  void platform_support::on_key(int x, int y, unsigned key, unsigned flags) {}
  void platform_support::on_ctrl_change() {}
  void platform_support::on_draw() {}
  void platform_support::on_post_draw(void* raw_handler) {}
}

bool agg::platform_specific::initialized = false;

void
platform_support_ext::prepare()
{
  if (! agg::platform_specific::initialized)
    {
      XInitThreads();
      agg::platform_specific::initialized = true;
    }
}

void 
platform_support_ext::lock()
{ 
  pthread_mutex_lock (m_specific->m_mutex); 
}

void
platform_support_ext::unlock()
{ 
  pthread_mutex_unlock (m_specific->m_mutex); 
}

bool
platform_support_ext::is_mapped()
{ 
  return m_specific->m_is_mapped;
}

void
platform_support_ext::close_request()
{
  m_specific->close();
}

void
platform_support_ext::update_region (const agg::rect_base<int>& r)
{
  if (! m_specific->m_is_mapped)
    return;

  m_specific->put_image(&rbuf_window(), m_specific->m_display_alt, &r);
        
  // When m_wait_mode is true we can discard all the events 
  // came while the image is being drawn. In this case 
  // the X server does not accumulate mouse motion events.
  // When m_wait_mode is false, i.e. we have some idle drawing
  // we cannot afford to miss any events
  // XSync(m_specific->m_display_alt, wait_mode());
  XFlush(m_specific->m_display_alt);
}

void
platform_support_ext::do_window_update()
{
  agg::rect_base<int> r(0, 0, rbuf_window().width(), rbuf_window().height());
  update_region(r);
}

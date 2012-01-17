/*
 *
 */
#define X640 1
#define X800 0
#define X1024 0

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/console.h>
#include <linux/selection.h>
#include <linux/ioport.h>
#include <linux/init.h>

#include <asm/io.h>
#include <asm/arch/hardware.h>

#include <video/fbcon.h>
#include <video/fbcon-cfb8.h>


/* --------------------------------------------------------------------- */

/*
 * card parameters
 */

/* card */
unsigned long video_base; /* physical addr */
int   video_size;
char *video_vbase;        /* mapped */

/* mode */
static int  video_bpp;
static int  video_width;
static int  video_height;
static int  video_height_virtual;
static int  video_type = FB_TYPE_PACKED_PIXELS;
static int  video_visual;
static int  video_linelength;
static int  video_cmap_len;

/* --------------------------------------------------------------------- */

static struct fb_var_screeninfo sa1101fb_defined = {
	0,0,0,0,	/* W,H, W, H (virtual) load xres,xres_virtual*/
	0,0,		/* virtual -> visible no offset */
	8,		/* depth -> load bits_per_pixel */
	0,		/* greyscale ? */
	{0,0,0},	/* R */
	{0,0,0},	/* G */
	{0,0,0},	/* B */
	{0,0,0},	/* transparency */
	0,		/* standard pixel format */
	FB_ACTIVATE_NOW,
	-1,-1,
	0,
	0L,0L,0L,0L,0L,
	0L,0L,0,	/* No sync info */
	FB_VMODE_NONINTERLACED,
	{0,0,0,0,0,0}
};

static struct display disp;
static struct fb_info fb_info;
static struct { u_short blue, green, red, pad; } palette[256];
static int    inverse   = 0;
static int	  vram __initdata = 0;	/* needed for vram boot option */
static int    currcon   = 0;
static int    ypan       = 1;  /* 0..nothing, 1..ypan, 2..ywrap */

static struct display_switch sa1101fb_sw;

static void sa1101_vga_init(int);

/* --------------------------------------------------------------------- */

static int sa1101fb_pan_display(struct fb_var_screeninfo *var, int con,
                              struct fb_info *info)
{
	int offset;
	
	if(!ypan)
		return -EINVAL;
    if (var->xoffset)
        return -EINVAL;
    if (var->yoffset > var->yres_virtual)
		return -EINVAL;
	
	offset = (var->yoffset * video_linelength+3)& (~3);

	VgaDBAR = offset;
				
	return 0;
}

static int sa1101fb_update_var(int con, struct fb_info *info)
{
	if (con == currcon && ypan) {
		struct fb_var_screeninfo *var = &fb_display[currcon].var;
		return sa1101fb_pan_display(var,con,info);
	}
	return 0;
}

static int sa1101fb_get_fix(struct fb_fix_screeninfo *fix, int con,
			 struct fb_info *info)
{
	memset(fix, 0, sizeof(struct fb_fix_screeninfo));
	strcpy(fix->id,"SA1101 FB");

	fix->smem_start=video_base;
	fix->smem_len=video_size;
	fix->type = video_type;
	fix->visual = video_visual;
	fix->xpanstep  = 0;
	fix->ypanstep  = ypan     ? 1 : 0;
	fix->ywrapstep = (ypan>1) ? 1 : 0;
	fix->line_length=video_linelength;
	return 0;
}

static int sa1101fb_get_var(struct fb_var_screeninfo *var, int con,
			 struct fb_info *info)
{
	if(con==-1)
		memcpy(var, &sa1101fb_defined, sizeof(struct fb_var_screeninfo));
	else
		*var=fb_display[con].var;
	return 0;
}

static void sa1101fb_revc(struct display *p, int xx, int yy)
{
	/* Can't reverse, because framebuffer is write only */
}

static void sa1101fb_set_disp(int con)
{
	struct fb_fix_screeninfo fix;
	struct display *display;
	struct display_switch *sw;
	
	if (con >= 0)
		display = &fb_display[con];
	else
		display = &disp;	/* used during initialization */

	sa1101fb_get_fix(&fix, con, 0);

	memset(display, 0, sizeof(struct display));
	display->screen_base = video_vbase;
	display->visual = fix.visual;
	display->type = fix.type;
	display->type_aux = fix.type_aux;
	display->ypanstep = fix.ypanstep;
	display->ywrapstep = fix.ywrapstep;
	display->line_length = fix.line_length;
	display->next_line = fix.line_length;
	display->can_soft_blank = 0;
	display->inverse = inverse;
	sa1101fb_get_var(&display->var, -1, &fb_info);

	switch (video_bpp) {
#ifdef FBCON_HAS_CFB8
	case 8:
		sw = &fbcon_cfb8;
		break;
#endif
	default:
		sw = &fbcon_dummy;
		return;
	}
	memcpy(&sa1101fb_sw, sw, sizeof(*sw));

	sa1101fb_sw.revc = sa1101fb_revc;
	
	display->dispsw = &sa1101fb_sw;
	if (!ypan) {
		display->scrollmode = SCROLL_YREDRAW;
		sa1101fb_sw.bmove = fbcon_redraw_bmove;
	}
}

static int sa1101fb_set_var(struct fb_var_screeninfo *var, int con,
			  struct fb_info *info)
{
	static int first = 1;

	if (var->bits_per_pixel != 8) {
		printk(KERN_WARNING "sa1101fb: Only 8 bit modes supported.\n");
		return -EINVAL;
	}
	
	if ((var->activate & FB_ACTIVATE_MASK) == FB_ACTIVATE_TEST)
		return 0;
	
	if (var->xres != sa1101fb_defined.xres ) {
		if(var->xres>800) {
			video_width=1024; 
			video_height=768;
		} else if(var->xres>640) {
			video_width=800; 
			video_height=600;
		} else {
			video_width=640;
			video_height=480;
		}
	}
	
	video_linelength=video_width;
	
    sa1101_vga_init(video_width);

    sa1101fb_defined.xres=video_width;
    sa1101fb_defined.yres=video_height;
    sa1101fb_defined.xres_virtual=video_width;
    sa1101fb_defined.yres_virtual=video_size / video_linelength;
    sa1101fb_defined.bits_per_pixel=8;
						
	if (ypan) {
		if (sa1101fb_defined.yres_virtual != var->yres_virtual) {
			sa1101fb_defined.yres_virtual = var->yres_virtual;
			if (con != -1) {
				fb_display[con].var = sa1101fb_defined;
				info->changevar(con);
			}
		}

		if (var->yoffset != sa1101fb_defined.yoffset)
			return sa1101fb_pan_display(var,con,info);
		return 0;
	}

	if (var->yoffset)
		return -EINVAL;
	return 0;
}

static int sa1101_getcolreg(unsigned regno, unsigned *red, unsigned *green,
			  unsigned *blue, unsigned *transp,
			  struct fb_info *fb_info)
{
	/*
	 *  Read a single color register and split it into colors/transparent.
	 *  Return != 0 for invalid regno.
	 */

	if (regno >= video_cmap_len)
		return 1;

	*red   = palette[regno].red;
	*green = palette[regno].green;
	*blue  = palette[regno].blue;
	*transp = 0;
	return 0;
}

static void sa1101_setpalette(int regno, int red, int green, int blue) {
	*(volatile unsigned int *)SA1101_p2v(_VideoControl+0x40000 + 0x400*regno) = 
		((blue&0xff)<<16)|((green&0xff)<<8)|(red&0xff);
}

static int sa1101_setcolreg(unsigned regno, unsigned red, unsigned green,
			  unsigned blue, unsigned transp,
			  struct fb_info *fb_info)
{
	/*
	 *  Set a single color register. The values supplied are
	 *  already rounded down to the hardware's capabilities
	 *  (according to the entries in the `var' structure). Return
	 *  != 0 for invalid regno.
	 */
	
	if (regno >= video_cmap_len)
		return 1;

	palette[regno].red   = red;
	palette[regno].green = green;
	palette[regno].blue  = blue;
	
	switch (video_bpp) {
#ifdef FBCON_HAS_CFB8
	case 8:
		sa1101_setpalette(regno,red,green,blue);
		break;
#endif
    }
    return 0;
}

static void do_install_cmap(int con, struct fb_info *info)
{
	if (con != currcon)
		return;
	if (fb_display[con].cmap.len)
		fb_set_cmap(&fb_display[con].cmap, 1, sa1101_setcolreg, info);
	else
		fb_set_cmap(fb_default_cmap(video_cmap_len), 1, sa1101_setcolreg,
			    info);
}

static int sa1101fb_get_cmap(struct fb_cmap *cmap, int kspc, int con,
			   struct fb_info *info)
{
	if (con == currcon) /* current console? */
		return fb_get_cmap(cmap, kspc, sa1101_getcolreg, info);
	else if (fb_display[con].cmap.len) /* non default colormap? */
		fb_copy_cmap(&fb_display[con].cmap, cmap, kspc ? 0 : 2);
	else
		fb_copy_cmap(fb_default_cmap(video_cmap_len),
		     cmap, kspc ? 0 : 2);
	return 0;
}

static int sa1101fb_set_cmap(struct fb_cmap *cmap, int kspc, int con,
			   struct fb_info *info)
{
	int err;

	if (!fb_display[con].cmap.len) {	/* no colormap allocated? */
		err = fb_alloc_cmap(&fb_display[con].cmap,video_cmap_len,0);
		if (err)
			return err;
	}
	if (con == currcon)			/* current console? */
		return fb_set_cmap(cmap, kspc, sa1101_setcolreg, info);
	else
		fb_copy_cmap(cmap, &fb_display[con].cmap, kspc ? 0 : 1);
	return 0;
}

static struct fb_ops sa1101fb_ops = {
	owner:		THIS_MODULE,
	fb_get_fix:	sa1101fb_get_fix,
	fb_get_var:	sa1101fb_get_var,
	fb_set_var:	sa1101fb_set_var,
	fb_get_cmap:	sa1101fb_get_cmap,
	fb_set_cmap:	sa1101fb_set_cmap,
	fb_pan_display:	sa1101fb_pan_display,
};

int __init sa1101fb_setup(char *options)
{
	char *this_opt;
	
	fb_info.fontname[0] = '\0';
	
	if (!options || !*options)
		return 0;
	
	while ((this_opt = strsep(&options, ",")) != NULL) {
		if (!*this_opt) continue;
		
		if (! strcmp(this_opt, "inverse"))
			inverse=1;
		else if (! strcmp(this_opt, "redraw"))
			ypan=0;
		else if (! strcmp(this_opt, "ypan"))
			ypan=1;
		else if (! strcmp(this_opt, "ywrap"))
			ypan=2;
		/* checks for vram boot option */
		else if (! strncmp(this_opt, "vram:", 5))
			vram = simple_strtoul(this_opt+5, NULL, 0);

		else if (!strncmp(this_opt, "font:", 5))
			strcpy(fb_info.fontname, this_opt+5);
	}
	return 0;
}

static int sa1101fb_switch(int con, struct fb_info *info)
{
	/* Do we have to save the colormap? */
	if (fb_display[currcon].cmap.len)
		fb_get_cmap(&fb_display[currcon].cmap, 1, sa1101_getcolreg,
			    info);
	
	currcon = con;
	/* Install new colormap */
	do_install_cmap(con, info);
	sa1101fb_update_var(con,info);
	return 1;
}

/* 0 unblank, 1 blank, 2 no vsync, 3 no hsync, 4 off */

static void sa1101fb_blank(int blank, struct fb_info *info)
{
	/* Not supported */
	switch(blank) {
		case 0:
			VideoControl |=1 ;
			break;
		case 1:
			break;
		case 2:
		case 3:
		case 4:
			VideoControl &= ~1;
	}
}

int __init sa1101fb_init(void)
{
	int i,j;


	video_base          = 0xc8000000; 
	video_bpp           = 8;
#if X640
	video_width         = 640;
	video_height        = 480;
#elif X800
	video_width         = 800;
	video_height        = 600;
#elif X1024
	video_width         = 1024;
	video_height        = 768;
#endif
	video_linelength    = video_width;

	video_size          = 1024*1024;

	/* check that we don't remap more memory than old cards have */
	video_visual = FB_VISUAL_PSEUDOCOLOR;

	if (!request_mem_region(video_base, video_size, "sa1101fb")) {
		printk(KERN_WARNING
		       "sa1101fb: abort, cannot reserve video memory at 0x%lx\n",
			video_base);
		return -EIO;
	}

    video_vbase = ioremap(video_base, video_size);
	if (!video_vbase) {
		release_mem_region(video_base, video_size);
		printk(KERN_ERR
		       "sa1101fb: abort, cannot ioremap video memory 0x%x @ 0x%lx\n",
			video_size, video_base);
		return -EIO;
	}

	printk(KERN_INFO "sa1101fb: framebuffer at 0x%lx, mapped to 0x%p, size %dk\n",
	       video_base, video_vbase, video_size/1024);
	printk(KERN_INFO "sa1101fb: mode is %dx%dx%d, linelength=%d\n",
	       video_width, video_height, video_bpp, video_linelength);

	sa1101_vga_init(video_width);
	
	sa1101fb_defined.xres=video_width;
	sa1101fb_defined.yres=video_height;
	sa1101fb_defined.xres_virtual=video_width;
	sa1101fb_defined.yres_virtual=video_size / video_linelength;
	sa1101fb_defined.bits_per_pixel=video_bpp;

	ypan=0; /* FIXME */
	
	if (ypan && sa1101fb_defined.yres_virtual > video_height) {
		printk(KERN_INFO "sa1101fb: scrolling: %s, yres_virtual=%d\n",
		       (ypan > 1) ? "ywrap" : "ypan",sa1101fb_defined.yres_virtual);
	} else {
		printk(KERN_INFO "sa1101fb: scrolling: redraw\n");
		sa1101fb_defined.yres_virtual = video_height;
		ypan = 0;
	}
	video_height_virtual = sa1101fb_defined.yres_virtual;

	/* some dummy values for timing to make fbset happy */
	sa1101fb_defined.pixclock     = 10000000 / video_width * 1000 / video_height;
	sa1101fb_defined.left_margin  = (video_width / 8) & 0xf8;
	sa1101fb_defined.right_margin = 32;
	sa1101fb_defined.upper_margin = 16;
	sa1101fb_defined.lower_margin = 4;
	sa1101fb_defined.hsync_len    = (video_width / 8) & 0xf8;
	sa1101fb_defined.vsync_len    = 4;

	sa1101fb_defined.red.length   = 8;
	sa1101fb_defined.green.length = 8;
	sa1101fb_defined.blue.length  = 8;

	for(i = 0; i < 16; i++) {
		j = color_table[i];
		palette[i].red   = default_red[j];
		palette[i].green = default_grn[j];
		palette[i].blue  = default_blu[j];
	}
	video_cmap_len = 256;
	
	strcpy(fb_info.modename, "SA1101 FB");
	fb_info.changevar = NULL;
	fb_info.node = -1;
	fb_info.fbops = &sa1101fb_ops;
	fb_info.disp=&disp;
	fb_info.switch_con=&sa1101fb_switch;
	fb_info.updatevar=&sa1101fb_update_var;
	fb_info.blank=&sa1101fb_blank;
	fb_info.flags=FBINFO_FLAG_DEFAULT;
	sa1101fb_set_disp(-1);


	if (register_framebuffer(&fb_info)<0)
		return -EINVAL;

	printk(KERN_INFO "fb%d: %s frame buffer device\n",
	       GET_FB_IDX(fb_info.node), fb_info.modename);
	return 0;
}


static void sa1101_vga_init(int x) {

	int vc;

	VideoControl = 0;
	SKPCR &= ~0x08;
	switch(x) {
		case 640:
			VgaTiming0      =0x7f17279c;
			VgaTiming1      =0x1e0b09df;
			VgaTiming2      =0x00000000;
			vc = 0x1b41;
			SKCDR &= ~0x180;
			SKCDR |= 0x000;
			UFCR = 0x30;
			break;
		case 800:
			VgaTiming0      =0x242d72c4;
			VgaTiming1      =0x16001257;
			VgaTiming2      =0x00000003;
			vc = 0x1b41;
			SKCDR &= ~0x180;
			SKCDR |= 0x080;
			UFCR = 0x28;
			break;
		case 1024:
		default:
			VgaTiming0      =0x641382fc;
			VgaTiming1      =0x1c0216ff;
			VgaTiming2      =0x00000000;
			vc = 0x15a1;
			SKCDR &= ~0x180;
			SKCDR |= 0x100;
			UFCR = 0x08;
	}
	
	VgaTiming3      =0x00000000;
	VgaBorder       =0x00000000;
	VgaDBAR         =0x00000000;
	VgaInterruptMask=0x00000000;
	VgaTest         =0x00000000;

	VMCCR = 0x9b;
	SKPCR |= 0x08;
	SNPR	= 0x90c00000;
	DacControl |= 1;
	PBDWR |= 2;

	VideoControl = vc;
}

module_init(sa1101fb_init);
MODULE_LICENSE("GPL");

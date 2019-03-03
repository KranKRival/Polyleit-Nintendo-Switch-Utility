// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <EGL/egl.h>    // EGL library
#include <EGL/eglext.h> // EGL extensions
#include <glad/glad.h>  // glad library (OpenGL loader)

#define DISPLAY_IMAGE
#ifdef DISPLAY_IMAGE
#include "image_bin.h"//Your own raw RGB888 1280x720 image at "data/image.bin" is required.
#endif

// See also libnx display/framebuffer.h.

// Define the desired framebuffer resolution (here we set it to 720p).
#define FB_WIDTH  1280
#define FB_HEIGHT 720

// Remove above and uncomment below for 1080p
//#define FB_WIDTH  1920
//#define FB_HEIGHT 1080

//This requires the switch-freetype package.
//Freetype code here is based on the example code from freetype docs.

static u32 framebuf_width=0;

//Note that this doesn't handle any blending.
void draw_glyph(FT_Bitmap* bitmap, u32* framebuf, u32 x, u32 y)
{
    u32 framex, framey;
    u32 tmpx, tmpy;
    u8* imageptr = bitmap->buffer;

    if (bitmap->pixel_mode!=FT_PIXEL_MODE_GRAY) return;

    for (tmpy=0; tmpy<bitmap->rows; tmpy++)
    {
        for (tmpx=0; tmpx<bitmap->width; tmpx++)
        {
            framex = x + tmpx;
            framey = y + tmpy;

            framebuf[framey * framebuf_width + framex] = RGBA8_MAXALPHA(imageptr[tmpx], imageptr[tmpx], imageptr[tmpx]);
        }

        imageptr+= bitmap->pitch;
    }
}

//Note that this doesn't handle {tmpx > width}, etc.
//str is UTF-8.
void draw_text(FT_Face face, u32* framebuf, u32 x, u32 y, const char* str)
{
    u32 tmpx = x;
    FT_Error ret=0;
    FT_UInt glyph_index;
    FT_GlyphSlot slot = face->glyph;

    u32 i;
    u32 str_size = strlen(str);
    uint32_t tmpchar;
    ssize_t unitcount=0;

    for (i = 0; i < str_size; )
    {
        unitcount = decode_utf8 (&tmpchar, (const uint8_t*)&str[i]);
        if (unitcount <= 0) break;
        i+= unitcount;

        if (tmpchar == '\n')
        {
            tmpx = x;
            y+= face->size->metrics.height / 64;
            continue;
        }

        glyph_index = FT_Get_Char_Index(face, tmpchar);
        //If using multiple fonts, you could check for glyph_index==0 here and attempt using the FT_Face for the other fonts with FT_Get_Char_Index.

        ret = FT_Load_Glyph(
                face,          /* handle to face object */
                glyph_index,   /* glyph index           */
                FT_LOAD_DEFAULT);

        if (ret==0)
        {
            ret = FT_Render_Glyph( face->glyph,   /* glyph slot  */
                                   FT_RENDER_MODE_NORMAL);  /* render mode */
        }

        if (ret) return;

        draw_glyph(&slot->bitmap, framebuf, tmpx + slot->bitmap_left, y - slot->bitmap_top);

        tmpx += slot->advance.x >> 6;
        y += slot->advance.y >> 6;
    }
}

__attribute__((format(printf, 1, 2)))
static int error_screen(const char* fmt, ...)
{
    consoleInit(NULL);
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    printf("Press PLUS to exit\n");
    while (appletMainLoop())
    {
        hidScanInput();
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_PLUS)
            break;
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
    return EXIT_FAILURE;
}

static u64 getSystemLanguage(void)
{
    Result rc;
    u64 code = 0;

    rc = setInitialize();
    if (R_SUCCEEDED(rc)) {
        rc = setGetSystemLanguage(&code);
        setExit();
    }

    return R_SUCCEEDED(rc) ? code : 0;
}

// LanguageCode is only needed with shared-font when using plGetSharedFont.
static u64 LanguageCode;

void userAppInit(void)
{
    Result rc;

    rc = plInitialize();
    if (R_FAILED(rc))
        fatalSimple(rc);

    LanguageCode = getSystemLanguage();
}

void userAppExit(void)
{
    plExit();
}

static float getTime()
{
	u64 elapsed = armGetSystemTick();
    return (elapsed * 625 / 12) / 1000000000.0;
}

static float getFPS()
{
	static int frames = 0;
	static double starttime = 0;
	static bool first = true;
	static float fps = 0.0f;
	
	if (first)
	{
		frames = 0;
		starttime = getTime();
		first = false;
		return 0;
	}
	frames++;
	
	if (getTime() - starttime > 0.25 && frames > 10)
	{
		fps = (double) frames / (getTime() - starttime);
		starttime = getTime();
		frames = 0;
	}
	return fps;

}



int CountPressed = 0;
const char* strPressed ="";

//Matrix containing the name of each key. Useful for printing when a key is pressed
    char keysNames[32][32] = {
        "KEY_A", "KEY_B", "KEY_X", "KEY_Y",
        "KEY_LSTICK", "KEY_RSTICK", "KEY_L", "KEY_R",
        "KEY_ZL", "KEY_ZR", "KEY_PLUS", "KEY_MINUS",
        "KEY_DLEFT", "KEY_DUP", "KEY_DRIGHT", "KEY_DDOWN",
        "KEY_LSTICK_LEFT", "KEY_LSTICK_UP", "KEY_LSTICK_RIGHT", "KEY_LSTICK_DOWN",
        "KEY_RSTICK_LEFT", "KEY_RSTICK_UP", "KEY_RSTICK_RIGHT", "KEY_RSTICK_DOWN",
        "KEY_SL", "KEY_SR", "KEY_TOUCH", "",
        "", "", "", ""
    };
	
	char KeyNamesArray[100];
	int optionCount = 3;
	int currentOption = 0;
	int maxOptions = 10;
	int TextYcoord = 100;
void AddOption(const char* option, FT_Face face, u32* FrameBufferSet)
{
	//optionCount++;
	if (currentOption == optionCount)
	{
		//infoText = info;
	}
	if (currentOption < maxOptions && optionCount < maxOptions)
	{
		//drawText(option, optionsFont, textXCoord, (optionCount * 0.035f + teztYCoord), 0.35f, 0.35f, optionsRed, optionsGreen, optionsBlue, optionsOpacity, false);
		draw_text(face, FrameBufferSet, 30, (40 + TextYcoord), option);
	}
	else if ((optionCount > (currentOption - maxOptions)) && optionCount < currentOption)
	{
		//drawText(option, optionsFont, textXCoord, ((optionCount - (currentOption - maxOptions)) * 0.035f + teztYCoord), 0.35f, 0.35f, optionsRed, optionsGreen, optionsBlue, optionsOpacity, false);
	    draw_text(face, FrameBufferSet, 30, (40 + TextYcoord), option);
	}
}

// Main program entrypoint
int main(int argc, char* argv[])
{
	
	Result rc=0;
    FT_Error ret=0;

    //Use this when using multiple shared-fonts.
    /*
    PlFontData fonts[PlSharedFontType_Total];
    size_t total_fonts=0;
    rc = plGetSharedFont(LanguageCode, fonts, PlSharedFontType_Total, &total_fonts);
    if (R_FAILED(rc))
        return error_screen("plGetSharedFont() failed: 0x%x\n", rc);
    */

    // Use this when you want to use specific shared-font(s). Since this example only uses 1 font, only the font loaded by this will be used.
    PlFontData font;
    rc = plGetSharedFontByType(&font, PlSharedFontType_Standard);
    if (R_FAILED(rc))
        return error_screen("plGetSharedFontByType() failed: 0x%x\n", rc);

    FT_Library library;
    ret = FT_Init_FreeType(&library);
    if (ret)
        return error_screen("FT_Init_FreeType() failed: %d\n", ret);

    FT_Face face;
    ret = FT_New_Memory_Face( library,
                              font.address,    /* first byte in memory */
                              font.size,       /* size in bytes        */
                              0,               /* face_index           */
                              &face);
    if (ret) {
        FT_Done_FreeType(library);
        return error_screen("FT_New_Memory_Face() failed: %d\n", ret);
    }

    ret = FT_Set_Char_Size(
            face,    /* handle to face object           */
            0,       /* char_width in 1/64th of points  */
            24*64,   /* char_height in 1/64th of points */
            96,      /* horizontal device resolution    */
            96);     /* vertical device resolution      */
    if (ret) {
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return error_screen("FT_Set_Char_Size() failed: %d\n", ret);
    }
	
    // Retrieve the default window
    NWindow* Window = nwindowGetDefault();

    // Create a linear double-buffered framebuffer
    Framebuffer WindowFrameBuffer;
    framebufferCreate(&WindowFrameBuffer, Window, FB_WIDTH, FB_HEIGHT, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&WindowFrameBuffer);

#ifdef DISPLAY_IMAGE
    u8* ImagePointer = (u8*)image_bin;
#endif

    u32 cnt = 0;
	u32 kDownOld = 0, kHeldOld = 0, kUpOld = 0; //In these variables there will be information about keys detected in the previous frame

    // Main loop
    while (appletMainLoop())
    {
        // Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // hidKeysDown returns information about which buttons have been
        // just pressed in this frame compared to the previous one
        
		u32 ImageSTR;
        u32* ImageFrameBufferSet = (u32*) framebufferBegin(&WindowFrameBuffer, &ImageSTR);
		framebuf_width = ImageSTR / sizeof(u32);
		
		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        //hidKeysHeld returns information about which buttons have are held down in this frame
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);
        //hidKeysUp returns information about which buttons have been just released
        u64 kUp = hidKeysUp(CONTROLLER_P1_AUTO);
		
		 if (kDown & KEY_PLUS)
            break; // break in order to return to hbmenu

        // Retrieve the framebuffer
        

        if (cnt != 60)
            //cnt ++;
		    cnt = 0;
        else
            cnt = 0;

        // Each pixel is 4-bytes due to RGBA8888.
        for (u32 y = 0; y < FB_HEIGHT; y ++)
        {
            for (u32 x = 0; x < FB_WIDTH; x ++)
            {
                u32 pos = y * ImageSTR / sizeof(u32) + x;
#ifdef DISPLAY_IMAGE
                ImageFrameBufferSet[pos] = RGBA8_MAXALPHA(ImagePointer[pos*3+0]+(cnt*4), ImagePointer[pos*3+1], ImagePointer[pos*3+2]);
#else
                ImageFrameBufferSet[pos] = 0x01010101 * cnt * 4;//Set framebuf to different shades of grey.
#endif
            }
        }
		
		//Do the keys printing only if keys have changed
        if (kDown != kDownOld || kHeld != kHeldOld || kUp != kUpOld)
        {
            //Clear console
            //consoleClear();

            //These two lines must be rewritten because we cleared the whole console
            //printf("\x1b[1;1HPress PLUS to exit.");
            //printf("\x1b[2;1HLeft joystick position:");
            //printf("\x1b[4;1HRight joystick position:");

            //printf("\x1b[6;1H"); //Move the cursor to the sixth row because on the previous ones we'll write the joysticks' position

            //Check if some of the keys are down, held or up
            int i;
			
            for (i = 0; i < 32; i++)
            {
                if (kDown & BIT(i)) sprintf(KeyNamesArray,"%s Pressed\n", keysNames[i]);
                //if (kHeld & BIT(i)) sprintf(KeyNamesArray,"%s Holding Key\n", keysNames[i]);
                //if (kUp & BIT(i)) sprintf(KeyNamesArray,"%s up\n", keysNames[i]);
				CountPressed = i;
            }
        }

        //Set keys old values for the next frame
        kDownOld = kDown;
        kHeldOld = kHeld;
        kUpOld = kUp;

        JoystickPosition pos_left, pos_right;

        //Read the joysticks' position
		char LeftSArray[100];
		char RightSArray[100];
        hidJoystickRead(&pos_left, CONTROLLER_P1_AUTO, JOYSTICK_LEFT);
        hidJoystickRead(&pos_right, CONTROLLER_P1_AUTO, JOYSTICK_RIGHT);

        //Print the joysticks' position
        sprintf(LeftSArray,"\x1b[3;1H%04d; %04d", pos_left.dx, pos_left.dy);
        sprintf(RightSArray,"\x1b[5;1H%04d; %04d", pos_right.dx, pos_right.dy);

        //consoleUpdate(NULL);
		if (CountPressed > 0)
		{
		draw_text(face, ImageFrameBufferSet, 32, 714, KeyNamesArray);
		draw_text(face, ImageFrameBufferSet, 500, 714, LeftSArray);
		draw_text(face, ImageFrameBufferSet, 900, 714, RightSArray);
		CountPressed--;
		}
        
        
        draw_text(face, ImageFrameBufferSet, 32, 30, u8"POLYLEIT v0.1 [Press + To Exit]");
		AddOption("Option 1", face, ImageFrameBufferSet);
		AddOption("Option 2", face, ImageFrameBufferSet);
		AddOption("Option 3", face, ImageFrameBufferSet);
		//char array[100];
		//sprintf(array, "System Running Time: %f", getTime());
		//draw_text(face, ImageFrameBufferSet, 32, 110, array);
		//char array1[100];
		//sprintf(array1, "System FPS Count: %f", getFPS());
		//draw_text(face, ImageFrameBufferSet, 32, 150, array1);
		
		framebufferEnd(&WindowFrameBuffer);
    }

    framebufferClose(&WindowFrameBuffer);
	FT_Done_Face(face);
    FT_Done_FreeType(library);
    return 0;
}

/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2015 Martin Kaltenbrunner <martin@tuio.org>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "CameraEngine.h"
using namespace tinyxml2;

bool CameraEngine::showSettingsDialog(bool lock) {
    if (settingsDialog) {
        settingsDialog = false;
        updateSettings();
        //CameraTool::saveSettings(this,cfg);
        return false;
    } else if (!lock) {
        currentCameraSetting = -1;
        for (int i=BRIGHTNESS;i<=GAMMA;i++) {
            if (hasCameraSetting(i)) {
                currentCameraSetting = i;
                break;
            }
        }
        if ( currentCameraSetting < 0) return false;
        settingsDialog = true;
        return true;
    } else return lock;
}

void CameraEngine::control(unsigned char key) {
    if(!settingsDialog) return;

    int step = 0;
    switch(key) {
        case VALUE_DOWN:
            step = getCameraSettingStep(currentCameraSetting);
            if (step==1) step = (int)((float)ctrl_max/256.0f);
            if (step<1) step=1;
            ctrl_val -= step;
            if (ctrl_val<ctrl_min) ctrl_val=ctrl_min;
            setCameraSetting(currentCameraSetting,ctrl_val);
            break;
        case VALUE_UP:
            step = getCameraSettingStep(currentCameraSetting);
            if (step==1) step = (int)((float)ctrl_max/256.0f);
            if (step<1) step=1;
            ctrl_val += step;
            if (ctrl_val>ctrl_max) ctrl_val=ctrl_max;
            setCameraSetting(currentCameraSetting,ctrl_val);
            break;
        case SETTING_PREVIOUS:
            currentCameraSetting--;
            if(currentCameraSetting<0) {
                if (cfg->color) currentCameraSetting=COLOR_BLUE;
                else currentCameraSetting=BACKLIGHT;
            }
            if ((!hasCameraSetting(currentCameraSetting)) || (getCameraSettingAuto(currentCameraSetting)))
                control(SETTING_PREVIOUS);
            break;
        case SETTING_NEXT:
            currentCameraSetting++;
            if ((cfg->color) && (currentCameraSetting>COLOR_BLUE)) currentCameraSetting=0;
            else if ((!cfg->color) && (currentCameraSetting>BACKLIGHT)) currentCameraSetting=0;
            if ((!hasCameraSetting(currentCameraSetting)) || (getCameraSettingAuto(currentCameraSetting)))
                control(SETTING_NEXT);
            break;
        case KEY_D:
            resetCameraSettings();
            break;

    }

    ctrl_val = getCameraSetting(currentCameraSetting);
    ctrl_max =  getMaxCameraSetting(currentCameraSetting);
    ctrl_min =  getMinCameraSetting(currentCameraSetting);
}

void CameraEngine::showInterface(UserInterface *interface) {
    if(!settingsDialog) return;

    //int settingValue = getCameraSetting(currentCameraSetting);
    //int maxValue =  getMaxCameraSetting(currentCameraSetting);
    //int minValue =  getMinCameraSetting(currentCameraSetting);

    const char *settingText = NULL;
    switch (currentCameraSetting) {
        case BRIGHTNESS:	settingText = "Brightness"; break;
        case CONTRAST:		settingText = "Contrast";   break;
        case SHARPNESS:		settingText = "Sharpness";  break;
        case AUTO_GAIN:     settingText = "Auto Gain";  break;
        case GAIN:          settingText = "Gain";       break;
        case AUTO_EXPOSURE:	settingText = "Auto Exposure"; break;
        case EXPOSURE:		settingText = "Exposure";   break;
        case SHUTTER:		settingText = "Shutter";    break;
        case AUTO_FOCUS:    settingText = "Auto Focus"; break;
        case FOCUS:         settingText = "Focus";      break;
        case GAMMA:         settingText = "Gamma";      break;
        case AUTO_WHITE:    settingText = "Auto White"; break;
        case WHITE:         settingText = "White Balance"; break;
        case POWERLINE:     settingText = "Powerline Frequency"; break;
        case BACKLIGHT:     settingText = "Backlight Compensation"; break;
        case AUTO_HUE:      settingText = "Auto Hue";  break;
        case COLOR_HUE:     settingText = "Color Hue";  break;
        case COLOR_RED:     settingText = "Red Balance";  break;
        case COLOR_BLUE:    settingText = "Blue Balance"; break;
        case COLOR_GREEN:   settingText = "Green Balance"; break;
    }

    char displayText[256];
    sprintf(displayText,"%s %d",settingText,ctrl_val);

    interface->displayControl(displayText, ctrl_min, ctrl_max, ctrl_val);
}

void CameraEngine::uyvy2gray(int width, int height, unsigned char *src, unsigned char *dest) {

    for (int i=height*width/2;i>0;i--) {
        src++;
        *dest++ = *src++;
        src++;
        *dest++ = *src++;
    }
}

void CameraEngine::crop_uyvy2gray(int cam_w, unsigned char *cam_buf, unsigned char *frm_buf) {

    if(!cfg->frame) return;
    int x_off = cfg->frame_xoff;
    int y_off = cfg->frame_yoff;
    int frm_w = cfg->frame_width;
    int frm_h = cfg->frame_height;

    cam_buf += 2*y_off*cam_w;
    int x_end = cam_w-(frm_w+x_off);

    for (int i=frm_h;i>0;i--) {

        cam_buf +=  2*x_off;
        for (int j=frm_w/2;j>0;j--) {
            cam_buf++;
            *frm_buf++ = *cam_buf++;
            cam_buf++;
            *frm_buf++ = *cam_buf++;
        }
        cam_buf +=  2*x_end;
    }
}

void CameraEngine::yuyv2gray(int width, int height, unsigned char *src, unsigned char *dest) {
    for (int i=height*width/2;i>0;i--) {
        *dest++ = *src++;
        src++;
        *dest++ = *src++;
        src++;
    }
}

void CameraEngine::crop_yuyv2gray(int cam_w, unsigned char *cam_buf, unsigned char *frm_buf) {

    if(!cfg->frame) return;
    int x_off = cfg->frame_xoff;
    int y_off = cfg->frame_yoff;
    int frm_w = cfg->frame_width;
    int frm_h = cfg->frame_height;

    cam_buf += 2*y_off*cam_w;
    int x_end = cam_w-(frm_w+x_off);

    for (int i=frm_h;i>0;i--) {

        cam_buf +=  2*x_off;
        for (int j=frm_w/2;j>0;j--) {
            *frm_buf++ = *cam_buf++;
            cam_buf++;
            *frm_buf++ = *cam_buf++;
            cam_buf++;
        }
        cam_buf +=  2*x_end;
    }
}

void yuv2rgb_conv(int Y1, int Y2, int U, int V, unsigned char *dest) {

				int R = (int)(Y1 + 1.370705f * V);
				int G = (int)(Y1 - 0.698001f * V - 0.337633f * U);
				int B = (int)(Y1 + 1.732446f * U);

				SAT(R);
				SAT(G);
				SAT(B);

				*dest++ = R;
                *dest++ = G;
				*dest++ = B;
}

void CameraEngine::uyvy2rgb(int width, int height, unsigned char *src, unsigned char *dest) {

			int Y1,Y2,U,V;

			for(int i=width*height/2;i>0;i--) {

				// U and V are +-0.5
                U  = *src++ - 128;
                Y1 = *src++;
                V  = *src++ - 128;
                Y2 = *src++;

                yuv2rgb_conv(Y1,Y2,U,V,dest);
                yuv2rgb_conv(Y1,Y2,U,V,dest+=3);
                dest+=3;
			}
}

void CameraEngine::crop_uyvy2rgb(int cam_w, unsigned char *cam_buf, unsigned char *frm_buf) {

    if(!cfg->frame) return;
    int x_off = cfg->frame_xoff;
    int y_off = cfg->frame_yoff;
    int frm_w = cfg->frame_width;
    int frm_h = cfg->frame_height;

    int Y1,Y2,U,V;

    cam_buf += 2*y_off*cam_w;
    int x_end = cam_w-(frm_w+x_off);

    for (int i=frm_h;i>0;i--) {

        cam_buf +=  2*x_off;
        for (int j=frm_w/2;j>0;j--) {
				// U and V are +-0.5
                U  = *cam_buf++ - 128;
                Y1 = *cam_buf++;
                V  = *cam_buf++ - 128;
                Y2 = *cam_buf++;

                yuv2rgb_conv(Y1,Y2,U,V,frm_buf);
                yuv2rgb_conv(Y1,Y2,U,V,frm_buf+=3);
                frm_buf+=3;
        }
        cam_buf +=  2*x_end;
    }
}

void CameraEngine::yuyv2rgb(int width, int height, unsigned char *src, unsigned char *dest) {

			int Y1,Y2,U,V;

			for(int i=width*height/2;i>0;i--) {

                Y1 = *src++;
                U  = *src++ - 128;
                Y2 = *src++;
                V  = *src++ - 128;

                yuv2rgb_conv(Y1,Y2,U,V,dest);
                yuv2rgb_conv(Y1,Y2,U,V,dest+=3);
                dest+=3;
        }
}

void CameraEngine::crop_yuyv2rgb(int cam_w, unsigned char *cam_buf, unsigned char *frm_buf) {

    if(!cfg->frame) return;
    int x_off = cfg->frame_xoff;
    int y_off = cfg->frame_yoff;
    int frm_w = cfg->frame_width;
    int frm_h = cfg->frame_height;

    int Y1,Y2,U,V;

    cam_buf += 2*y_off*cam_w;
    int x_end = cam_w-(frm_w+x_off);

    for (int i=frm_h;i>0;i--) {

        cam_buf +=  2*x_off;
        for (int j=frm_w/2;j>0;j--) {
				// U and V are +-0.5
                Y1 = *cam_buf++;
                U  = *cam_buf++ - 128;
                Y2 = *cam_buf++;
                V  = *cam_buf++ - 128;

                yuv2rgb_conv(Y1,Y2,U,V,frm_buf);
                yuv2rgb_conv(Y1,Y2,U,V,frm_buf+=3);
                frm_buf+=3;
        }
        cam_buf +=  2*x_end;
    }
}

void CameraEngine::gray2rgb(int width, int height, unsigned char *src, unsigned char *dest) {

             int size = width*height;
             src  += size-1;
             dest += 3*size-1;
             for (int i=size;i>0;i--) {
                    unsigned char pixel = *src++;
                    *dest++ = pixel;
                    *dest++ = pixel;
                    *dest++ = pixel;
             }
}

void CameraEngine::crop_gray2rgb(int cam_w, unsigned char *cam_buf, unsigned char *frm_buf) {

    if(!cfg->frame) return;
    int x_off = cfg->frame_xoff;
    int y_off = cfg->frame_yoff;
    int frm_w = cfg->frame_width;
    int frm_h = cfg->frame_height;

    cam_buf += y_off*cam_w;
    int x_end = cam_w-(frm_w+x_off);

    for (int i=frm_h;i>0;i--) {

        cam_buf += x_off;
        for (int j=frm_w;j>0;j--) {
            unsigned char pixel = *cam_buf++;
            *frm_buf++ = pixel;
            *frm_buf++ = pixel;
            *frm_buf++ = pixel;
        }
        cam_buf +=  x_end;
    }
}

/*void CameraEngine::cropFrame(unsigned char *cambuf, unsigned char *cropbuf, int bytes) {
 
 if(!cfg->frame) return;
 
 unsigned char *src = cambuf + bytes*(cfg->frame_yoff*cfg->cam_width + cfg->frame_xoff);
 unsigned char *dest = cropbuf;
 
 for (int i=cfg->frame_height;i>0;i--) {
 memcpy(dest, src, bytes*cfg->frame_width);
 
 src += bytes*cfg->cam_width;
 dest += bytes*cfg->frame_width;
 }
 }*/


/*void CameraEngine::crop(int cam_w, int cam_h, unsigned char *cam_buf, unsigned char *frm_buf, int b) {
 
    if(!cfg->frame) return;
    int x_off = cfg->frame_xoff;
    int y_off = cfg->frame_yoff;
    int frm_w = cfg->frame_width;
    int frm_h = cfg->frame_height;
 
    cam_buf += cam_buf + b*(x_off*cam_w + y_off);
 
    for (int i=frm_h;i>0;i--) {
        memcpy(cam_buf, frm_buf, b*cam_w);
 
        src += b*cam_w;
        dest += b*frm_w;
    }
 }*/


void CameraEngine::crop(int cam_w, int cam_h, unsigned char *cam_buf, unsigned char *frm_buf, int b) {

	if(!cfg->frame) return;
    int x_off = cfg->frame_xoff;
    int y_off = cfg->frame_yoff;
    int frm_w = cfg->frame_width;
    int frm_h = cfg->frame_height;

	cam_buf += b*y_off*cam_w;
    int xend = (cam_w-(frm_w+x_off));

    for (int i=frm_h;i>0;i--) {

		cam_buf +=  b*x_off;
        for (int j=b*frm_w;j>0;j--) {
 			*frm_buf++ = *cam_buf++;
	    }
        cam_buf +=  b*xend;
    }
}

void CameraEngine::flip(int width, int height, unsigned char *src, unsigned char *dest, int b) {

		int size = b*width*height;
		dest += size-1;
		for(int i=size;i>0;i--) {
			*dest-- = *src++;
		}
}

void CameraEngine::flip_crop(int cam_w, int cam_h, unsigned char *cam_buf, unsigned char *frm_buf, int b) {

	if(!cfg->frame) return;
    int x_off = cfg->frame_xoff;
    int y_off = cfg->frame_yoff;
    int frm_w = cfg->frame_width;
    int frm_h = cfg->frame_height;

	cam_buf += b*y_off*cam_w;
	frm_buf += b*frm_w*frm_h-1;
    int xend = (cam_w-(frm_w+x_off));

    for (int i=frm_h;i>0;i--) {

		cam_buf +=  b*x_off;
        for (int j=b*frm_w;j>0;j--) {
 			*frm_buf-- = *cam_buf++;
	    }
        cam_buf +=  b*xend;
    }
}

void CameraEngine::rgb2gray(int width, int height, unsigned char *src, unsigned char *dest) {

    int R,G,B;
    for (int i=width*height;i>0;i--) {

        R = *src++;
        G = *src++;
        B = *src++;
        *dest++ =  HBT(R*77 + G*151 + B*28);
    }
}

void CameraEngine::flip_rgb2gray(int width, int height, unsigned char *src, unsigned char *dest) {

	int size = width*height;
	dest += size-1;

    int R,G,B;
    for (int i=size;i>0;i--) {

        R = *src++;
        G = *src++;
        B = *src++;
        *dest-- =  HBT(R*77 + G*151 + B*28);
    }
}
void CameraEngine::crop_rgb2gray(int cam_w, unsigned char *cam_buf, unsigned char *frm_buf) {

    if(!cfg->frame) return;
    int x_off = cfg->frame_xoff;
    int y_off = cfg->frame_yoff;
    int frm_w = cfg->frame_width;
    int frm_h = cfg->frame_height;

    cam_buf += 3*y_off*cam_w;
    int x_end = cam_w-(frm_w+x_off);

    int R,G,B;
    for (int i=frm_h;i>0;i--) {

        cam_buf += 3*x_off;
        for (int j=frm_w;j>0;j--) {
            R = *cam_buf++;
            G = *cam_buf++;
            B = *cam_buf++;
            *frm_buf++ = HBT(R*77 + G*151 + B*28);
        }
        cam_buf +=  3*x_end;
    }
}

void CameraEngine::flip_crop_rgb2gray(int cam_w, unsigned char *cam_buf, unsigned char *frm_buf) {

    if(!cfg->frame) return;
    int x_off = cfg->frame_xoff;
    int y_off = cfg->frame_yoff;
    int frm_w = cfg->frame_width;
    int frm_h = cfg->frame_height;

    cam_buf += 3*y_off*cam_w;
    int x_end = cam_w-(frm_w+x_off);
	frm_buf += frm_w*frm_h-1;

    int R,G,B;
    for (int i=frm_h;i>0;i--) {

        cam_buf += 3*x_off;
        for (int j=frm_w;j>0;j--) {
            R = *cam_buf++;
            G = *cam_buf++;
            B = *cam_buf++;
            *frm_buf-- = HBT(R*77 + G*151 + B*28);
        }
        cam_buf +=  3*x_end;
    }
}

void CameraEngine::setupFrame() {

    if(!cfg->frame) {
        cfg->frame_width = cfg->cam_width;
        cfg->frame_height = cfg->cam_height;
        return;
    }

    // size sanity check
    if (cfg->frame_width%2!=0) cfg->frame_width--;
    if (cfg->frame_height%2!=0) cfg->frame_height--;

    if (cfg->frame_width<=0) cfg->frame_width = cfg->cam_width;
    if (cfg->frame_height<=0) cfg->frame_height = cfg->cam_height;

    if (cfg->frame_width > cfg->cam_width) cfg->frame_width = cfg->cam_width;
    if (cfg->frame_height > cfg->cam_height) cfg->frame_height = cfg->cam_height;

    // no cropping if same size
    if ((cfg->frame_width==cfg->cam_width) && (cfg->frame_height==cfg->cam_height)) {

        cfg->frame_width = cfg->cam_width;
        cfg->frame_height = cfg->cam_height;
        cfg->frame = false;
        return;
    }

    // offset sanity check
    int xdiff = cfg->cam_width-cfg->frame_width;
    if (xdiff<0) cfg->frame_xoff = 0;
    else if (cfg->frame_xoff > xdiff) cfg->frame_xoff = xdiff;
    int ydiff = cfg->cam_height-cfg->frame_height;
    if (ydiff<0) cfg->frame_yoff = 0;
    else if (cfg->frame_yoff > ydiff) cfg->frame_yoff = ydiff;

}

void CameraEngine::applyCameraSetting(int mode, int value) {

    switch (value) {
        case SETTING_AUTO:
            if (hasCameraSettingAuto(mode)) {
                setCameraSettingAuto(mode,true);
                return;
            }
        case SETTING_DEFAULT:
            setDefaultCameraSetting(mode);
            return;
        case SETTING_MIN:
            setCameraSetting(mode,getMinCameraSetting(mode)); return;
        case SETTING_MAX:
            setCameraSetting(mode,getMaxCameraSetting(mode)); return;
        default: {
            int max = getMaxCameraSetting(mode);
            int min = getMinCameraSetting(mode);
            if (value<min) value = min;
            else if (value>max) value = max;
            setCameraSetting(mode,value);
        }
    }
}

void CameraEngine::resetCameraSettings() {

   for (int mode=BRIGHTNESS;mode<=COLOR_BLUE;mode++)
        setDefaultCameraSetting(mode);
}

void CameraEngine::applyCameraSettings() {

    resetCameraSettings();

    applyCameraSetting(BRIGHTNESS,cfg->brightness);
    applyCameraSetting(CONTRAST,cfg->contrast);
    applyCameraSetting(SHARPNESS,cfg->sharpness);
    applyCameraSetting(GAIN,cfg->gain);
    applyCameraSetting(EXPOSURE,cfg->exposure);
    applyCameraSetting(SHUTTER,cfg->shutter);
    applyCameraSetting(FOCUS,cfg->focus);
    applyCameraSetting(WHITE,cfg->white);
    applyCameraSetting(POWERLINE,cfg->powerline);
    applyCameraSetting(BACKLIGHT,cfg->backlight);
    applyCameraSetting(GAMMA,cfg->gamma);

    applyCameraSetting(COLOR_HUE,cfg->red);
    applyCameraSetting(COLOR_RED,cfg->red);
    applyCameraSetting(COLOR_GREEN,cfg->green);
    applyCameraSetting(COLOR_BLUE,cfg->blue);
}

int CameraEngine::updateSetting(int mode) {

    if (getCameraSettingAuto(mode)) return SETTING_AUTO;

    int value = getCameraSetting(mode);
    if (value==getMinCameraSetting(mode)) value = SETTING_MIN;
    else if (value==getMaxCameraSetting(mode)) value = SETTING_MAX;
    else if (value==getDefaultCameraSetting(mode)) value = SETTING_DEFAULT;
    return value;
}

void CameraEngine::updateSettings() {

    cfg->brightness = updateSetting(BRIGHTNESS);
    cfg->contrast = updateSetting(CONTRAST);
    cfg->sharpness = updateSetting(SHARPNESS);

    cfg->gain = updateSetting(GAIN);
    cfg->exposure = updateSetting(EXPOSURE);
    cfg->shutter = updateSetting(SHUTTER);
    cfg->focus = updateSetting(FOCUS);
    cfg->white = updateSetting(WHITE);
    cfg->backlight = updateSetting(BACKLIGHT);
    cfg->powerline = updateSetting(POWERLINE);
    cfg->gamma = updateSetting(GAMMA);

    cfg->hue = updateSetting(COLOR_HUE);
    cfg->red = updateSetting(COLOR_RED);
    cfg->green = updateSetting(COLOR_GREEN);
    cfg->blue = updateSetting(COLOR_BLUE);

}

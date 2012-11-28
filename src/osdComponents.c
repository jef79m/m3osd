#include "board.h"
#include "osdcore.h"
#include "fonts.h"
#include "multiwii.h"
#include "osdComponents.h"

void osdHorizon(void)
{
    int x_dim = OSD_WIDTH;      // Number of screen pixels along x axis
    int y_dim = osdData.Height; // Number of screen pixels along y axis
    int L = 79 + 25;            // Length of main horizon line indicator
    int l = 10;                 // Length of small angle indicator lines
    // float theta_max = 42.5 * (M_PI / 180);        // Max pitch angle displayed on screen
    float theta_max = 23 * (M_PI / 180);        // OSD FOV/2
    int x_c, y_c;
    short x_a[36] = { 0, };
    short y_a[36] = { 0, };
    char t_a[36] = { 0, };
    signed char a_a[36] = { 0, };
    float pitch;
    float roll;
    float d1, alpha1;
    int i, idx = 0;
    int flag = 5;               // was 7
    int x_c2, y_c2;
    int L2 = 49;
    float cosroll, sinroll;
    
    pitch = multiwiiData.anglePitch * (M_PI / 180.0f) / 10;
    roll = multiwiiData.angleRoll * (M_PI / 180.0f) / 10;
    cosroll = cosf(roll);
    sinroll = sinf(roll);
    
    x_c = x_dim / 2;
    y_c = y_dim / 2 * (1 - pitch / theta_max);
    x_c2 = x_dim / 2 + sinroll * flag;
    y_c2 = y_dim / 2 * (1 - pitch / theta_max) + cosroll * flag;
    
    for (i = 10; i <= 90; i += 10) {
        d1 = sqrtf(17 * 17 + powf(i * M_PI / 180.0f / theta_max * y_dim / 2, 2));
        alpha1 = atan2f((i * M_PI / 180.0f / theta_max * y_dim / 2), 17);

        // d2 = sqrtf(50 * 50 + powf(i * M_PI / 180 / theta_max * y_dim / 2, 2));
        // alpha2 = atanf((i * M_PI / 180 / theta_max * y_dim / 2) / 50);

        // + i.e. down
        t_a[idx] = 3;
        a_a[idx] = i;
        x_a[idx] = x_dim / 2 + d1 * cosf(alpha1 - roll);
        y_a[idx++] = (y_dim / 2) * (1 - pitch / theta_max) + d1 * sinf(alpha1 - roll);
        t_a[idx] = 3;
        a_a[idx] = i;
        x_a[idx] = x_dim / 2 - d1 * cosf(alpha1 + roll);
        y_a[idx++] = (y_dim / 2) * (1 - pitch / theta_max) + d1 * sinf(alpha1 + roll);

        // - i.e. up
        t_a[idx] = 0;
        a_a[idx] = i;
        x_a[idx] = x_dim / 2 + d1 * cosf(alpha1 + roll);
        y_a[idx++] = (y_dim / 2) * (1 - pitch / theta_max) - d1 * sinf(alpha1 + roll);
        t_a[idx] = 0;
        a_a[idx] = i;
        x_a[idx] = x_dim / 2 - d1 * cosf(alpha1 - roll);
        y_a[idx++] = (y_dim / 2) * (1 - pitch / theta_max) - d1 * sinf(alpha1 - roll);
    }
    
    for (i = 0; i < 36; i++) {
        osdDrawLine(x_a[i] - l * cosroll, y_a[i] - l * -sinroll, x_a[i] + l * cosroll, y_a[i] + l * -sinroll, 1, t_a[i]);
        if (i % 2) {
            osdSetCursor(x_a[i], y_a[i] - 4);
            osdDrawDecimal(FONT_8PX_FIXED, a_a[i], 3, 0, -1);
        }
    }

    // cross
    //osdDrawHorizontalLine(x_dim / 2 - 5, y_dim / 2, 11, 1);
    //osdDrawVerticalLine(x_dim / 2, y_dim / 2 - 5, 11, 1);
    //osdDrawCircle(x_dim / 2, y_dim / 2, 39, 1, 0xFF);

    // center
    osdDrawCircle(x_dim / 2, y_dim / 2, 3, 1, 0xFF);
    osdDrawHorizontalLine(x_dim / 2 - 7, y_dim / 2, 4, 1);
    osdDrawHorizontalLine(x_dim / 2 + 3, y_dim / 2, 5, 1);
    osdDrawVerticalLine(x_dim / 2, y_dim / 2 - 6, 4, 1);

    // horizont
    osdDrawLine(x_c - L * cosroll, y_c - L * -sinroll, x_c + L * cosroll, y_c + L * -sinroll, 1, 0);

    // sub horizont
    osdDrawLine(x_c2 - L2 * cosroll, y_c2 - L2 * -sinroll, x_c2 + L2 * cosroll, y_c2 + L2 * -sinroll, 1, 0);
}

void osdHeading(void)
{
    int i;
    int head, offset;
    osdDrawHorizontalLine(OSD_WIDTH / 2 - 100, 14, 190, 1);

    // osdDrawVerticalLine(200,10,12,1);
    osdSetCursor(OSD_WIDTH / 2 - 3, 12);

    //osdDrawCharacter(247+32,4);
    //osdDrawCharacter(253+32,4);
    osdDrawCharacter(95 + 32, FONT_16PX_FIXED);
    for (i = 10; i < 190; i += 10) {
        head = multiwiiData.heading;
        offset = head % 10;

        //if (!(i % 5)) 
        osdDrawVerticalLine(OSD_WIDTH / 2 - 100 + i - offset, 9, 5, 1);

        //else 
        //    osdDrawVerticalLine(100 + i - offset, 9 + 3, 2, 1);
        switch (head - offset - 100 + i) {
        case 0:
            osdSetCursor(OSD_WIDTH / 2 - 100 + i - offset - 3, 0);
            osdDrawCharacter('N', FONT_8PX_FIXED);
            break;
        case 90:
            osdSetCursor(OSD_WIDTH / 2 - 100 + i - offset - 3, 0);
            osdDrawCharacter('E', FONT_8PX_FIXED);
            break;
        case 180:
        case -180:
            osdSetCursor(OSD_WIDTH / 2 - 100 + i - offset - 3, 0);
            osdDrawCharacter('S', FONT_8PX_FIXED);
            break;
        case -90:
            osdSetCursor(OSD_WIDTH / 2 - 100 + i - offset - 3, 0);
            osdDrawCharacter('W', FONT_8PX_FIXED);
            break;
        }
    }
    osdSetCursor(OSD_WIDTH / 2 - 2 * 8, 25);
    osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.heading, 3, 0, -1);
}

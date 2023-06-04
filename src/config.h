#ifndef CONFIG_H
#define CONFIG_H

#define VERSION                    "1.0"

#define WINDOW_W                   640             /* Window width                 */
#define WINDOW_H                   480             /* Window height                */
#define FPS                        30              /* Frames per second            */
#define FRAME_TIME                 1000 / FPS      /* Duration of 1 frame in ms    */
#define NO_OBJECTS                 7               /* Number of objects on screen  */

#define SYSTEM_NAME_SIZE           20
#define CORE_NAME_SIZE             100
#define NUMBER_OF_FILES            100
#define JSON_BUFFER_SIZE           10000
#define SEARCH_WORD_SIZE           18

#define SD1_ROMS_PATH              "/mnt/mmc/Roms"
#define SD2_ROMS_PATH              "/mnt/SDCARD/Roms"
#define COREMAPPING_JSON_PATH      "/mnt/mmc/CFW/config/coremapping.json"
#define SETTINGS_JSON_PATH         "/mnt/mmc/CFW/skin/settings.json"
#define BACKGROUND_PATH            "/mnt/mmc/CFW/skin/background.png"
#define ICONS_PATH                 "/mnt/mmc/CFW/skin"
#define ROMSEARCH_SH_PATH          "/mnt/mmc/CFW/retroarch/romsearch.sh"
#define MAME_FILELIST_PATH         "/mnt/mmc/CFW/config/mame.csv"
#define LANGUAGE_FLAG_PATH         "/mnt/mmc/CFW/language.flag"
#define LANGUAGES_PATH             "/mnt/mmc/CFW/lang"
#define FONTS_PATH                 "/mnt/mmc/CFW/font"
#define SKIN_PATH                  "./res/skin.json"
#define DEF_BACKGROUND_PATH        "./res/background.png"
#define DEF_FONT_PATH              "./res"
#define DEF_FONT                   "./res/font.otf"
#define DEF_SETTINGS_JSON_PATH     "./res/settings.json"
#define DEF_MAME_FILELIST_PATH     "./res/mame.csv"
#define DEF_COREMAPPING_JSON_PATH  "./res/coremapping.json"
#define DEF_ICONS_PATH             "./res"
#define DEF_LANGUAGES_PATH         "./res/English.json"

#define RG35_MENU_CODE             117             /* RG35xx button code */
#define RG35_UP_CODE               119             /* RG35xx button code */
#define RG35_RIGHT_CODE            100             /* RG35xx button code */
#define RG35_DOWN_CODE             115             /* RG35xx button code */
#define RG35_LEFT_CODE             113             /* RG35xx button code */
#define RG35_A_CODE                97              /* RG35xx button code */
#define RG35_B_CODE                98              /* RG35xx button code */
#define RG35_X_CODE                120             /* RG35xx button code */
#define RG35_Y_CODE                121             /* RG35xx button code */
#define RG35_SELECT_CODE           110             /* RG35xx button code */
#define RG35_START_CODE            109             /* RG35xx button code */
#define RG35_L1_CODE               104             /* RG35xx button code */
#define RG35_L2_CODE               106             /* RG35xx button code */
#define RG35_R1_CODE               108             /* RG35xx button code */
#define RG35_R2_CODE               107             /* RG35xx button code */

#define DEF_C_ACTIVE_TEXT          color(220, 220, 220) /* Default active text color (Garlic skin)      */
#define DEF_C_INACTIVE_TEXT        color(125, 125, 125) /* Default inactive text color (Garlic skin)    */
#define DEF_TEXT_ALIGN             "left"
#define DEF_KEYBOARD_ALIGN         "left"
#define DEF_TEXT_MARGIN            352
#define DEF_ROMS_FONT_SIZE         20
#define DEF_TEXT_FONT_SIZE         22
#define KEYBOARD_FONT_SIZE         14
#define SEARCH_WORD_FONT_SIZE      20

#define MAME_CONSOLE_LIST          {"ARCADE", "CPS1", "CPS2", "CPS3", "FBNEO", "MAME2000", "NEOGEOCD", "NEOGEO"}
#define MAME_SYSTEM_NAME_SIZE      10
#define MAME_BUFFER_LINES          40000
#define MAME_BUFFER_TAG_SIZE       30
#define MAME_BUFFER_LONG_SIZE      180
#define MAME_CONSOLE_COUNT         8

#define KB_ROWS                    4
#define KB_COLUMNS                 10
#define KB_X                       8
#define KB_Y                       105
#define KB_W                       340
#define KB_H                       270
#define SB_W                       328
#define SB_H                       48
#define KY_W                       31
#define KY_H                       38
#define KB_FLIP_OFFSET             (WINDOW_W - KB_W - 2 * KB_X)

#define BACKGROUND_C               color(30,30,46)      /* Default color used if no background is found */
#define KB_C_BACKGND               color(52, 52, 52)    /* Default keyboard background color            */
#define KB_C_SB_FRAME              color(70, 68, 72)    /* Default keyboard search box frame color      */  
#define KB_C_SB                    color(35, 35, 35)    /* Default keyboard search box color            */
#define KB_C_KEY_SEL               color(125, 125, 125) /* Default keyboard selected key color          */
#define KB_C_KEY                   color(220, 220, 220) /* Default keyboard unselected key color        */ 
#define KB_C_INFO                  color(125, 125, 125) /* Default keyboard info text color             */  
#define KB_C_ST                    color(220, 220, 220) /* Default keyboard search text color           */
#define KB_C_CURSOR                color(220, 220, 220) /* Default keyboard cursor color                */   

#define MENU_BUTTON                RG35_MENU_CODE  /* Code of the pressed button to exit the application                     */
#define MENU_BUTTON_INDEX          0               /* Button config list index of the pressed button to exit the application */
#define FIRST_PRESS_DELAY          300
#define PRESS_DELAY                50
#define CURSOR_BLINK_DELAY         500

#define OBJ_MOVE_BT_INDEX          1
#define OBJ_OPEN_BT_INDEX          2
#define OBJ_BACK_BT_INDEX          3
#define OBJ_KEYBOARD_BT_INDEX      4
#define OBJ_SCREENSHOT_INDEX       5
#define OBJ_CONSOLE_INDEX          6
#define OBJECTS_CONFIG \
{\
    {NULL, { 20,  15, 0, 0}, { 70,  15, 0, 0}, "SEARCH",   "logo.png",         1, 1},\
    {NULL, { 20, 420, 0, 0}, { 70, 435, 0, 0}, "MOVE",     "icon-dpad-54.png", 1, 1},\
    {NULL, {155, 420, 0, 0}, {205, 435, 0, 0}, "OPEN",     "icon-A-54.png",    1, 1},\
    {NULL, {295, 420, 0, 0}, {345, 435, 0, 0}, "BACK",     "icon-B-54.png",    1, 1},\
    {NULL, {430, 420, 0, 0}, {480, 435, 0, 0}, "KEYBOARD", "icon-Y-54.png",    1, 1},\
    {NULL, {  0,   0, 0, 0}, {  0,   0, 0, 0}, "IMAGE",    "",                 1, 0},    /* Screenshots */\
    {NULL, {435,  25, 0, 0}, {  0,   0, 0, 0}, "",         "",                 1, 0}     /* Console     */\
}

#define KEYBOARD_LAYOUTS \
{\
    {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},\
    {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"},\
    {"A", "S", "D", "F", "G", "H", "J", "K", "L", "-"},\
    {"Z", "X", "C", "V", "B", "N", "M", " ", ".", "_"}\
}
#endif

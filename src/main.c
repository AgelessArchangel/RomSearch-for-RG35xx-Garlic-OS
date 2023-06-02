#include <stdio.h>
#include <string.h>
#include <limits.h> 
#include <dirent.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>
#include "SDL_rotozoom.h"
#include "config.h"
#include "cJSON.h"

typedef struct Object_Tag {
    SDL_Surface * img;                       /* Unselected image                           */
    SDL_Rect    imgPos;                      /* Position of the button image on the screen */
    SDL_Rect    textPos;                     /* Position of the text under the button      */     
    char        name[10];                    /* Image text                                 */
    char        imgFile[FILE_NAME_SIZE];     /* PNG File                                   */
    uint8_t     imgVisible;                  /* Image visible                              */
    uint8_t     textVisible;                 /* Text visible                               */
}Object;

typedef struct Skin_Tag 
{
    SDL_Color background;
    SDL_Color sbFrame;
    SDL_Color sb;
    SDL_Color keySel;
    SDL_Color key;
    SDL_Color info;
    SDL_Color st;
    SDL_Color cursor;
}Skin;

typedef struct Rom_Tag {
    char      system[SYSTEM_NAME_SIZE];
    char      displayName[FILE_NAME_SIZE];
    char      fileName[FILE_NAME_SIZE];
    uint8_t   romLocation;
}Rom;

Skin            skin;
Rom             romsList[NUMBER_OF_FILES];
char            jsonFileBuffer[JSON_BUFFER_SIZE]; 
char            *keyboardLayout[KB_ROWS][KB_COLUMNS] = KEYBOARD_LAYOUTS;
Object          objects[NO_OBJECTS]    = OBJECTS_CONFIG;
uint16_t        pressDelay             = FIRST_PRESS_DELAY;
uint8_t         running                = 1;
uint8_t         keyboardShown          = 1;
uint16_t        keyDownTime            = 0;
uint8_t         romsFound              = 0;
uint8_t         selRomIdx              = 0;     /* Selected ROM index     */
uint8_t         selRomIdxOld           = 0;     /* Used to detect changes */
uint8_t         selKeyIIdx             = 0;     /* Selected key i index   */
uint8_t         selKeyJIdx             = 0;     /* Selected key j index   */
char            romCoreMapping[CORE_NAME_SIZE];
char            searchWord[SEARCH_WORD_SIZE];
char            textAligment[10];
SDL_Surface     *screen, *background, *textSurface;
SDL_Rect        backgroundLocation, textLocation;
TTF_Font        *textFont, *romsFont, *keyboardFont, *searchWordFont;
uint16_t        textMargin;
SDL_Event       event;
cJSON           *json;
uint8_t         startSearch;
int             cursorOld;
SDL_Color       activeColor, inactiveColor;
int             romsFontSize;
int             textFontSize;
int             kbPosY;
char            mamelist[MAME_CONSOLE_COUNT][MAME_SYSTEM_NAME_SIZE] = MAME_CONSOLE_LIST;
char            mameFileBufferTag[MAME_BUFFER_LINES][MAME_BUFFER_TAG_SIZE];
char            mameFileBufferLong[MAME_BUFFER_LINES][MAME_BUFFER_LONG_SIZE];
int             mameFileSize = 0;
char            keyboardAlign[6] = "left";
uint8_t         updateScreenshot = 0;
char            language[20];
char            font[40];


SDL_Rect rect(int x, int y, int w, int h) { return (SDL_Rect){x, y, w, h}; }
SDL_Color color(int r, int g, int b) { return (SDL_Color){r,g,b}; }


void drawRect(SDL_Rect pos, int o, SDL_Color c, char * align) {
    SDL_Rect invPos = rect(pos.x + o, pos.y, pos.w, pos.h);
    Uint32 mapColor = SDL_MapRGB(screen->format, c.r, c.g, c.b);
    if (strcmp(align, "right") == 0) SDL_FillRect(screen, &invPos, mapColor);
    else SDL_FillRect(screen, &pos, mapColor);
}

void drawText(SDL_Rect pos, int o,  char * text, TTF_Font *font, SDL_Color c, char * align) {
    SDL_Rect invPos = rect(pos.x + o, pos.y, pos.w, pos.h);
    if (textSurface != NULL) SDL_FreeSurface(textSurface);
    textSurface = TTF_RenderUTF8_Blended(font, text, c);
    if (strcmp(align, "right") == 0) SDL_BlitSurface(textSurface, NULL, screen, &invPos);
    else SDL_BlitSurface(textSurface, NULL, screen, &pos);
}

void drawKey(int x, int y, int o, char * text, SDL_Color c) {
    int fw, fh; 
    drawRect(rect(x, y, KY_W, KY_H), o, color(70, 68, 72), keyboardAlign);
    TTF_SizeUTF8(keyboardFont, text, &fw, &fh);
    drawText(rect(x + (KY_W - fw) / 2, y + (KY_H - fh)/2, 0, 0), o, text, keyboardFont, c, keyboardAlign);
}

void searchMameRom(char * fileName) {
    int res;
    for (int i = 0; i < mameFileSize; ++i) {
        res = strcmp(&mameFileBufferTag[i][0], fileName);
        if (res > 0) break;
        else if (res == 0) {
            strcpy(romsList[romsFound].displayName, &mameFileBufferLong[i][0]);
            romsList[romsFound].displayName[strcspn(romsList[romsFound].displayName, "\r\n")] = '\0';
            break;
        }
    }
}

SDL_Color getColorFromJson(char * colorTag, SDL_Color defCol) {
    SDL_Color color;
    cJSON *name = cJSON_GetObjectItemCaseSensitive(json,  colorTag);
    if (cJSON_IsString(name) != 0) sscanf(name->valuestring,"#%02x%02x%02x",&color.r, &color.g, &color.b);
    else return defCol;
    return color;
}

void initButtonGuide(void) {
    int total = 20;
    int pos = 10;
    int fw, fh, gap;
    
    TTF_SizeUTF8(textFont, objects[OBJ_MOVE_BT_INDEX].name, &fw, &fh);
    objects[OBJ_MOVE_BT_INDEX].textPos.w = fw;
    total += objects[OBJ_MOVE_BT_INDEX].img->w + fw + 5;
    TTF_SizeUTF8(textFont, objects[OBJ_OPEN_BT_INDEX].name, &fw, &fh);
    objects[OBJ_OPEN_BT_INDEX].textPos.w = fw;
    total += objects[OBJ_OPEN_BT_INDEX].img->w + fw + 5;
    TTF_SizeUTF8(textFont, objects[OBJ_BACK_BT_INDEX].name, &fw, &fh);
    objects[OBJ_BACK_BT_INDEX].textPos.w = fw;
    total += objects[OBJ_BACK_BT_INDEX].img->w + fw + 5;
    TTF_SizeUTF8(textFont, objects[OBJ_KEYBOARD_BT_INDEX].name, &fw, &fh);
    objects[OBJ_KEYBOARD_BT_INDEX].textPos.w = fw;
    total += objects[OBJ_KEYBOARD_BT_INDEX].img->w + fw + 5;
    gap  = (WINDOW_W - total) / 3;
    objects[OBJ_MOVE_BT_INDEX].imgPos.x = pos;
    pos += objects[OBJ_MOVE_BT_INDEX].img->w;
    objects[OBJ_MOVE_BT_INDEX].textPos.x = pos;
    pos += objects[OBJ_MOVE_BT_INDEX].textPos.w + gap;
    objects[OBJ_OPEN_BT_INDEX].imgPos.x = pos;
    pos += objects[OBJ_OPEN_BT_INDEX].img->w;
    objects[OBJ_OPEN_BT_INDEX].textPos.x = pos;
    pos += objects[OBJ_OPEN_BT_INDEX].textPos.w + gap;
    objects[OBJ_BACK_BT_INDEX].imgPos.x = pos;
    pos += objects[OBJ_BACK_BT_INDEX].img->w;
    objects[OBJ_BACK_BT_INDEX].textPos.x = pos;
    pos += objects[OBJ_BACK_BT_INDEX].textPos.w + gap;
    objects[OBJ_KEYBOARD_BT_INDEX].imgPos.x = pos;
    pos += objects[OBJ_KEYBOARD_BT_INDEX].img->w;
    objects[OBJ_KEYBOARD_BT_INDEX].textPos.x = pos;
    pos += objects[OBJ_KEYBOARD_BT_INDEX].textPos.w + gap;
}

void initObjects(void) {
    char path[PATH_MAX + 1];
    for (uint8_t i = 0; i < NO_OBJECTS; ++i) {
        if (strcmp(objects[i].imgFile, "") != 0) {
            sprintf(&path[0], "%s/%s", ICONS_PATH, objects[i].imgFile);
            objects[i].img = IMG_Load(path);
            if (objects[i].img == NULL) {
                sprintf(&path[0], "%s/%s", DEF_ICONS_PATH, objects[i].imgFile);
                fprintf(stderr,"Icon: %s not found. Default loaded.\n", objects[i].imgFile);
                objects[i].img = IMG_Load(path);
            }
        } 
    }
    initButtonGuide();
}

int loadJsonFile(char * path, char * defPath) {
    FILE *file = fopen(path, "r");

    if (file == NULL)  {
        fprintf(stderr, "%s not found. Default %s loaded.\n", path, defPath);
        file = fopen(defPath, "r");
    }
    fread(jsonFileBuffer, 1, sizeof(jsonFileBuffer), file);
    fclose(file);
    json = cJSON_Parse(jsonFileBuffer);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)  fprintf(stderr, "Error parsing the JSON file: %s default file loaded.\n", error_ptr);
        cJSON_Delete(json);
        file = fopen(defPath, "r");
        fread(jsonFileBuffer, 1, sizeof(jsonFileBuffer), file);
        json = cJSON_Parse(jsonFileBuffer);
        return 0;
    }
    return 1;
}

int loadCoreMapping(void) {
    loadJsonFile(COREMAPPING_JSON_PATH, DEF_COREMAPPING_JSON_PATH);
    if (romsFound != 0) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(json,  romsList[selRomIdx].system);
        if ((cJSON_IsString(name) != 0) && (name->valuestring != NULL)) strcpy(romCoreMapping, name->valuestring);
        else { 
            cJSON_Delete(json);
            return 0;
        }
    }
    else {
        cJSON_Delete(json);
        return 0;
    }
    cJSON_Delete(json);
    return 1;
}

void loadSettings(void) {
    cJSON *name;
    loadJsonFile(SETTINGS_JSON_PATH, DEF_SETTINGS_JSON_PATH);
    name = cJSON_GetObjectItemCaseSensitive(json,  "text-alignment");
    if (cJSON_IsString(name) != 0) strcpy(textAligment, name->valuestring);
    else strcpy(textAligment, TEXT_ALIGN);
    name = cJSON_GetObjectItemCaseSensitive(json,  "text-margin");
    if (cJSON_IsNumber(name) != 0) textMargin = (int)name->valuedouble;
    else textMargin = TEXT_MARGIN;
    activeColor = getColorFromJson("color-active", C_ACTIVE_TEXT);
    inactiveColor = getColorFromJson("color-inactive", C_INACTIVE_TEXT);
    cJSON_Delete(json);
}

void loadSkin() {
    cJSON *name;
    loadJsonFile(SKIN_PATH, SKIN_PATH);
    skin.background = getColorFromJson("background", KB_C_BACKGND);
    skin.sbFrame = getColorFromJson("searchbox-frame",KB_C_SB_FRAME);
    skin.sb = getColorFromJson("searchbox", KB_C_SB);
    skin.st = getColorFromJson("search-text", KB_C_ST);
    skin.keySel = getColorFromJson("selected-key", KB_C_KEY_SEL);
    skin.key = getColorFromJson("unselected-key", KB_C_KEY);
    skin.info = getColorFromJson("text-info", KB_C_INFO);
    skin.cursor = getColorFromJson("cursor", KB_C_CURSOR);
    name = cJSON_GetObjectItemCaseSensitive(json,  "keyboard-and-result-pos-y");
    if (cJSON_IsNumber(name) != 0) kbPosY = (int)name->valuedouble;
    else kbPosY = KB_Y;
    name = cJSON_GetObjectItemCaseSensitive(json,  "system-pos-x");
    if (cJSON_IsNumber(name) != 0) objects[OBJ_CONSOLE_INDEX].imgPos.x = (int)name->valuedouble;
    name = cJSON_GetObjectItemCaseSensitive(json,  "system-pos-y");
    if (cJSON_IsNumber(name) != 0) objects[OBJ_CONSOLE_INDEX].imgPos.y = (int)name->valuedouble;
    cJSON_Delete(json);
}

void loadLanguage(char * language) {
    DIR * dir;
    FILE *file;
    uint8_t found = 0;
    cJSON *name;
    char path[PATH_MAX + 1];    
    struct dirent * entry;
    
    strcpy(path, DEF_LANGUAGES_PATH);
    if ((dir = opendir (LANGUAGES_PATH)) == NULL)
        fprintf(stderr, "Language file %s not found. Default file %s loaded.\n", language, DEF_LANGUAGES_PATH);
    else {
        while (((entry = readdir (dir)) != NULL) && (found == 0)) {
            if (entry->d_type == DT_REG) {
                if (strcmp(entry->d_name, language) == 0) {
                    found = 1;
                    sprintf(path, "%s/%s", LANGUAGES_PATH, entry->d_name);
                }
            }
        }
    }
    closedir(dir);
    if (found == 0)
        fprintf(stderr, "Language file %s not found. Default file %s loaded.\n", language, DEF_LANGUAGES_PATH);
    file = fopen(path, "r");
    if (file == NULL)  {
        fprintf(stderr, "No language file found (default also missing)\n");
        strcpy(font, DEF_FONT);
        textFontSize = DEF_TEXT_FONT_SIZE;
    }
    else {
        fclose(file);
        loadJsonFile(path, DEF_FONT_PATH);
        name = cJSON_GetObjectItemCaseSensitive(json,  "font");
        if (cJSON_IsString(name) != 0) 
            if (found != 0) sprintf(font, "%s/%s", FONTS_PATH, name->valuestring);
            else sprintf(font, "%s/%s", DEF_FONT_PATH, name->valuestring);
        else strcpy(font, DEF_FONT);
        name = cJSON_GetObjectItemCaseSensitive(json,  "font-size");
        if (cJSON_IsNumber(name) != 0) romsFontSize = (int)name->valuedouble;
        else romsFontSize = DEF_ROMS_FONT_SIZE;
        name = cJSON_GetObjectItemCaseSensitive(json,  "button-guide-font-size");
        if (cJSON_IsNumber(name) != 0)  textFontSize = (int)name->valuedouble;
        else textFontSize = DEF_TEXT_FONT_SIZE;
        cJSON_Delete(json);
    }
}

void searchRoms(char * romPath , uint8_t romLocation) {
    DIR * dir;
    DIR * console;
    char * ptrDispName;
    struct dirent * entry;
    struct dirent * rom;
    char path[PATH_MAX + 1];    
    if ((dir = opendir (romPath)) == NULL) return;
    while ((entry = readdir (dir)) != NULL) {
        if ((entry->d_type == DT_DIR) && (strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0)) {
            sprintf(&path[0], "%s/%s", romPath, entry->d_name);
            if ((console = opendir (path)) == NULL) return;
            while ((rom = readdir(console)) != NULL) {
                 if ((rom->d_type == DT_REG) && (romsFound < NUMBER_OF_FILES)) {
                    strcpy(romsList[romsFound].displayName, rom->d_name);
                    ptrDispName = strrchr (romsList[romsFound].displayName, '.');
                    if (ptrDispName != NULL) ptrDispName[0] = '\0';
                    for (uint8_t i = 0; i < MAME_CONSOLE_COUNT; ++i)
                        if (strcmp(entry->d_name, &mamelist[i][0]) == 0) {
                            searchMameRom(romsList[romsFound].displayName);
                            break;
                        }
                    if ((strcasestr(romsList[romsFound].displayName, searchWord)) != NULL) {
                        romsList[romsFound].romLocation = romLocation;
                        strcpy(romsList[romsFound].system, entry->d_name);
                        strcpy(romsList[romsFound].fileName, rom->d_name);
                        romsFound++;
                    }
                }             
            }
            closedir(console);
        }
    }
    closedir(dir);
}

void searchFillRomList() {
    romsFound = 0;
    searchRoms(SD1_ROMS_PATH, 0);
    searchRoms(SD2_ROMS_PATH, 1);
}

void bufferMameFile() {
    FILE * mameFile;
    char * line;
    size_t len = 0;
    
    mameFile = fopen(MAME_FILELIST_PATH, "r");
    if (mameFile == NULL)  {
        fprintf(stderr, "mame.csv not found. Default mame.csv loaded.\n");
        mameFile = fopen(DEF_MAME_FILELIST_PATH, "r");
    }
    while (getline(&line, &len, mameFile) != -1) { 
        sscanf(line, "%[^,],%[^,]", &mameFileBufferTag[mameFileSize], &mameFileBufferLong[mameFileSize]);
        mameFileSize++;
    }
    fclose(mameFile);
}

int setup() {
    FILE *file = fopen(COMMAND_SH_PATH, "r");
    char * line = NULL;
    size_t len;
    if (file != NULL)  {
        fprintf(stderr, "romSearch.sh found! \n");
        fclose(file); 
        return 0;
    }
    bufferMameFile();
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "SDL Init error...\n");
        return 0;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF Init error...\n");
        return 0;
    }
    if (IMG_Init(IMG_INIT_PNG) == 0) {
        fprintf(stderr, "IMG Init error...\n");
        return 0;
    }
    SDL_ShowCursor(SDL_DISABLE);
    screen       = SDL_SetVideoMode(WINDOW_W, WINDOW_H, 32, SDL_SWSURFACE);
    loadSkin();
    file = fopen(LANGUAGE_FLAG_PATH, "r");
    if (file == NULL)
        fprintf(stderr, "Language flag missing. Default language set to %s.\n", DEF_LANGUAGES_PATH);
    else  {
        if (getline(&line, &len, file) == -1) 
            fprintf(stderr, "Language flag empty. Default language set to %s.\n", DEF_LANGUAGES_PATH);
        else {
            strcpy(language, line);
            language[strcspn(language, "\r\n")] = '\0';
            strcat(language, ".json");
        }
        fclose(file); 
    }
    loadLanguage(language);
    textFont     = TTF_OpenFont(font, textFontSize);
    if (textFont == NULL) {
        fprintf(stderr, "Font not found. Default font loaded.\n");
        textFont     = TTF_OpenFont(DEF_FONT_PATH, textFontSize);
    }
    romsFont     = TTF_OpenFont(font, romsFontSize);
    if (romsFont == NULL) {
        fprintf(stderr, "Font not found. Default font loaded.\n");
        romsFont     = TTF_OpenFont(DEF_FONT_PATH, romsFontSize);
    }
    keyboardFont    = TTF_OpenFont(font, KEYBOARD_FONT_SIZE);
    if (keyboardFont == NULL) {
        fprintf(stderr, "Font not found. Default font loaded.\n");
        keyboardFont     = TTF_OpenFont(DEF_FONT_PATH, KEYBOARD_FONT_SIZE);
    }
    searchWordFont    = TTF_OpenFont(font, SEARCH_WORD_FONT_SIZE);
    if (searchWordFont == NULL) {
        fprintf(stderr, "Font not found. Default font loaded.\n");
        searchWordFont     = TTF_OpenFont(DEF_FONT_PATH, SEARCH_WORD_FONT_SIZE);
    }
    background   = IMG_Load(BACKGROUND_PATH); 
    if (background == NULL) {
        fprintf(stderr, "Background not found. Default background loaded.\n");
        background   = IMG_Load(DEF_BACKGROUND_PATH);
    }
    loadSettings();
    initObjects();
    return 1;
}

int createCommandScript() {
    
    if (loadCoreMapping() == 0) return 0;
    FILE *file = fopen(COMMAND_SH_PATH, "w");
    if (file == NULL)  {
        fprintf(stderr, "Cannot open command.sh file.\n");
        return 0;
    }
    fprintf(file, "#!/bin/sh\n");
    fprintf(file, "export LANG=en_us\n");
    if (romsList[selRomIdx].romLocation == 0)
        fprintf(file, "/mnt/mmc/CFW/retroarch/retroarch -L \"/mnt/mmc/CFW/retroarch/.retroarch/cores/%s\" \"%s/%s/%s\"\n",
            romCoreMapping, SD1_ROMS_PATH, romsList[selRomIdx].system, romsList[selRomIdx].fileName);
    else
        fprintf(file, "/mnt/mmc/CFW/retroarch/retroarch -L \"/mnt/mmc/CFW/retroarch/.retroarch/cores/%s\" \"%s/%s/%s\"\n",
            romCoreMapping, SD2_ROMS_PATH, romsList[selRomIdx].system, romsList[selRomIdx].fileName);
    fprintf(file, "exit $?\n");
    fclose(file);
    return 1;
}

void processInput() {
    uint16_t delta;
    SDL_PollEvent(&event);
    int fw, fh;
    switch(event.type) {
        case SDL_QUIT:
            running = 0;
            break;
        case SDL_KEYDOWN:
            delta = SDL_GetTicks() - keyDownTime;
            if (delta > pressDelay) {
                if ((keyDownTime != 0) && (delta > FIRST_PRESS_DELAY)) pressDelay = PRESS_DELAY;
                keyDownTime = SDL_GetTicks();
                switch(event.key.keysym.sym) {
                    case MENU_BUTTON: 
                        running = 0;
                    break;
                    case RG35_DOWN_CODE:
                        if (keyboardShown == 0)
                            if ((romsFound != 0) && (selRomIdx < romsFound - 1)) selRomIdx++;
                            else selRomIdx = 0;
                        else if (selKeyIIdx < 3) selKeyIIdx++;
                            else selKeyIIdx = 0;
                    break;
                    case RG35_UP_CODE:
                        if (keyboardShown == 0)
                            if (selRomIdx > 0) selRomIdx--;
                            else selRomIdx = romsFound - 1;
                        else if (selKeyIIdx > 0) selKeyIIdx--;
                            else selKeyIIdx = 3;
                    break;
                    case RG35_RIGHT_CODE:
                        if (keyboardShown != 0) 
                            if (selKeyJIdx < 9) selKeyJIdx++;
                            else selKeyJIdx = 0;
                    break;
                    case RG35_LEFT_CODE:
                        if (keyboardShown != 0) 
                            if (selKeyJIdx > 0) selKeyJIdx--;
                            else selKeyJIdx = 9;
                    break;
                    case RG35_A_CODE:
                        if (keyboardShown != 0) {
                            TTF_SizeUTF8(searchWordFont, searchWord, &fw, &fh);
                            if (fw + KB_X + 16 < SB_W) {
                                strcat(searchWord, keyboardLayout[selKeyIIdx][selKeyJIdx]);
                                startSearch = 1;
                            }
                            updateScreenshot = 1;
                        } else {
                            createCommandScript();
                            running = 0;
                        }
                    break;
                    case RG35_B_CODE:
                        if (keyboardShown != 0) {
                            searchWord[strlen(searchWord)-1] = '\0';
                            if (strlen(searchWord) > 0) 
                                startSearch = 1;
                            else {
                                romsFound = 0;
                                startSearch = 0;
                            }
                            updateScreenshot = 1;
                        }
                        else running = 0;
                    break;
                    case RG35_X_CODE:
                        if (keyboardShown != 0) {
                            if (strcmp(keyboardAlign, "left") == 0) strcpy(keyboardAlign, "right");
                            else strcpy(keyboardAlign,"left");
                        }
                    break;
                    case RG35_Y_CODE:
                        keyboardShown = 1 - keyboardShown;
                    break;
                    case RG35_SELECT_CODE:
                        if ((keyboardShown != 0) && (strlen(searchWord) < SEARCH_WORD_SIZE))
                            strcat(searchWord, " ");
                    break;
                    case RG35_START_CODE:
                        if ((keyboardShown != 0) && (strlen(searchWord) != 0)) keyboardShown = 0;
                    default:
                    break;
                }
            }
        break;
        case SDL_KEYUP:
            keyDownTime = 0;
            pressDelay = FIRST_PRESS_DELAY;
        break;
        default:
        break;
    }
}

int loadSurface(SDL_Surface ** surf, char * image) {
    SDL_Surface * tmp = IMG_Load(image);
    if (tmp != NULL) {
        double wf =  (double)WINDOW_W/tmp->w;
        double hf =  (double)WINDOW_H/tmp->h; 
        *surf = rotozoomSurface(tmp, wf, hf, 1);
    }
    else return 0;
    SDL_FreeSurface(tmp);
    return 1;
}

void loadScreenshot(void) {
    char path[PATH_MAX + 1] = "";
    char * extension;
    
    if (romsFound != 0) {
        objects[OBJ_SCREENSHOT_INDEX].imgVisible = 1;
        objects[OBJ_CONSOLE_INDEX].imgVisible = 1;
        if (romsList[selRomIdx].romLocation == 0)
            sprintf(path, "%s/%s/Imgs/%s", SD1_ROMS_PATH, romsList[selRomIdx].system, romsList[selRomIdx].fileName);
        else
            sprintf(path, "%s/%s/Imgs/%s", SD2_ROMS_PATH, romsList[selRomIdx].system, romsList[selRomIdx].fileName);
        if ((extension = strrchr(path, '.')) != NULL) extension[0] = '\0';
        strcat(path,".png");
        if (objects[OBJ_SCREENSHOT_INDEX].img != NULL) SDL_FreeSurface(objects[OBJ_SCREENSHOT_INDEX].img);
        if (loadSurface(&objects[OBJ_SCREENSHOT_INDEX].img, path) == 0) objects[OBJ_SCREENSHOT_INDEX].imgVisible = 0;
        sprintf(path, "%s/system/%s.png", ICONS_PATH, romsList[selRomIdx].system);
        if (objects[OBJ_CONSOLE_INDEX].img) SDL_FreeSurface(objects[OBJ_CONSOLE_INDEX].img);
        objects[OBJ_CONSOLE_INDEX].img = IMG_Load(path);
        if (objects[OBJ_CONSOLE_INDEX].img == NULL) objects[OBJ_CONSOLE_INDEX].imgVisible = 0;
    } else {
        objects[OBJ_SCREENSHOT_INDEX].imgVisible = 0;
        objects[OBJ_CONSOLE_INDEX].imgVisible = 0;
    }
}

void update(int delta) {
    if (startSearch != 0) {
        startSearch = 0;
        searchFillRomList();
    }
    if ((selRomIdx != selRomIdxOld) || (updateScreenshot != 0)) {
        selRomIdxOld = selRomIdx;
        updateScreenshot = 0;
        loadScreenshot();   
    }
}

void drawObjects(void) {    
    for (uint8_t i; i < NO_OBJECTS; ++i) {
        if (objects[i].imgVisible == 1)   SDL_BlitSurface(objects[i].img, NULL, screen, &objects[i].imgPos);
        if (objects[i].textVisible == 1)  drawText(objects[i].textPos, 0, objects[i].name, textFont, activeColor, textAligment);
    }
}

void drawRomsTextbox(void) {
    uint8_t first;
    SDL_Rect pos;
    int o, fw, fh;
    
    selRomIdx = (selRomIdx >= romsFound) ? romsFound - 1 : selRomIdx;  
    first = (selRomIdx < 4) ? 0 : selRomIdx - 3;
    TTF_SizeUTF8(romsFont, romsList[0].displayName, &fw, &fh);
    for (uint8_t i = 0; (i < 8) && ((i + first)< romsFound); ++i) {
        pos = rect(textMargin, kbPosY + (KB_H - (8 * fh)) / 2 + i * fh, 0, 0);
        o = WINDOW_W - strlen(romsList[i + first].displayName) * ( romsFontSize - 3) - 2 * textMargin;
        if ((i + first) == selRomIdx)  drawText(pos, o, romsList[i + first].displayName, romsFont, activeColor, textAligment);
        else drawText(pos, o, romsList[i + first].displayName, romsFont, inactiveColor, textAligment);
    }
}

void drawKeyboard() {
    int delta, o = KB_FLIP_OFFSET; 
    int fw, fh; 
    int kby = kbPosY;
    
    TTF_SizeUTF8(searchWordFont, searchWord, &fw, &fh);
    drawRect(rect(KB_X, kby, KB_W, KB_H), o, skin.background, keyboardAlign);                   
    drawRect(rect(KB_X + 6, kby + 10, SB_W, SB_H), o, skin.sbFrame, keyboardAlign);                    
    drawRect(rect(KB_X + 8, kby + 12, SB_W - 4, SB_H - 4), o, skin.sb, keyboardAlign); 
    drawText(rect(KB_X + 16, kby + 20, 0, 0), o, searchWord, searchWordFont, skin.st, keyboardAlign);
    delta = SDL_GetTicks() - cursorOld;
    if (delta > (2 * CURSOR_BLINK_DELAY)) {
        drawRect(rect(KB_X + 17 + fw, kby + 22, 2, 22), o, skin.sb, keyboardAlign);
        cursorOld = SDL_GetTicks();
    }
    else if (delta > CURSOR_BLINK_DELAY) drawRect(rect(KB_X + 17 + fw, kby + 22, 2, 22), o, skin.cursor, keyboardAlign);
    for (uint8_t i = 0; i < KB_ROWS; ++i) {
        for (uint8_t j = 0; j < KB_COLUMNS; ++j) {
            if ((i == selKeyIIdx) && (j == selKeyJIdx)) {
                drawRect(rect(KB_X + 5 + (KY_W + 2) * j, kby + 67 + (KY_H + 2) * i, KY_W + 2, KY_H + 2), o, skin.key, keyboardAlign);
                drawKey(KB_X + 6 + (KY_W + 2) * j, kby + 68 + (KY_H + 2) * i, o, keyboardLayout[i][j], skin.keySel);
            } else drawKey(KB_X + 6 + (KY_W + 2) * j, kby + 68 + (KY_H + 2) * i, o, keyboardLayout[i][j], skin.key);
        }
        drawText(rect(KB_X + 8, kby + 240, 0, 0), o, "Select/Space", keyboardFont, skin.info, keyboardAlign);
        TTF_SizeUTF8(keyboardFont, "Start/Done", &fw, &fh);
        drawText(rect(KB_W - fw, kby + 240, 0, 0), o, "Start/Done", keyboardFont, skin.info, keyboardAlign);
    }
}

void render(void) {
    SDL_BlitSurface(background, NULL, screen, &backgroundLocation);
    drawObjects();
    if (romsFound != 0) drawRomsTextbox();
    if (keyboardShown != 0) drawKeyboard();
    SDL_Flip(screen);
}

void cleanup() {
    for (int i = 0; i < NO_OBJECTS; ++i) {
        SDL_FreeSurface(objects[i].img);
    }
    if (screen != NULL)SDL_FreeSurface(screen);
    if (background != NULL)SDL_FreeSurface(background);
    if (textSurface != NULL)SDL_FreeSurface(textSurface);
    TTF_CloseFont(textFont);
    TTF_CloseFont(romsFont);
    TTF_CloseFont(keyboardFont);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

int main() {
    uint16_t new         = 0;
    uint16_t old         = 0;
    uint16_t delta       = 0;

    fprintf(stderr, "Start setup... \n");
    if (setup() == 0) return 0;
    fprintf(stderr, "Main loop started.\n");
    while (running == 1) {
        new = SDL_GetTicks();
        delta = new - old;                  /* Time since last frame */
        processInput();
        if (delta >= FRAME_TIME) {
            update(delta);
            render();
            old = new;
        } 
        else SDL_Delay(FRAME_TIME - delta);
    }
    cleanup();
    return 0;
}

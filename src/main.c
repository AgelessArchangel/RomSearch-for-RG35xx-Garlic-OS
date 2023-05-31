#include <stdio.h>
#include <string.h>
#include <limits.h> 
#include <dirent.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>
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
    char    system[SYSTEM_NAME_SIZE];
    char    displayName[FILE_NAME_SIZE];
    char    fileName[FILE_NAME_SIZE];
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
uint8_t         selRomIdx              = 0;     /* Selected ROM index   */
uint8_t         selKeyIIdx             = 0;     /* Selected key i index */
uint8_t         selKeyJIdx             = 0;     /* Selected key j index */
char            romCoreMapping[CORE_NAME_SIZE];
char            searchWord[SEARCH_WORD_SIZE];
char            textAligment[10];
SDL_Surface     *screen, *background, *textSurface;
SDL_Rect        backgroundLocation, textLocation;
TTF_Font        *textFont, *romsFont, *keyboardFont;
uint16_t        textMargin;
SDL_Event       event;
cJSON           *json;
uint8_t         startSearch;
int             cursorOld;
SDL_Color       activeColor, inactiveColor;
char            mamelist[MAME_CONSOLE_COUNT][MAME_SYSTEM_NAME_SIZE] = MAME_CONSOLE_LIST;
char            mameFileBufferTag[MAME_BUFFER_LINES][MAME_BUFFER_TAG_SIZE];
char            mameFileBufferLong[MAME_BUFFER_LINES][MAME_BUFFER_LONG_SIZE];
int             mameFileSize = 0;

SDL_Rect rect(int x, int y, int w, int h) { return (SDL_Rect){x, y, w, h}; }
SDL_Color color(int r, int g, int b) { return (SDL_Color){r,g,b}; }

int bufferMameFile() {
    FILE *mameFile;
    char * line;
    size_t len = 0;
    mameFile = fopen(MAME_FILELIST_PATH, "r");
    if (mameFile == NULL)  {
        fprintf(stderr, "Cannot open mame.csv file.\n");
        return 0;
    }
    while (getline(&line, &len, mameFile) != -1) sscanf(line, "%[^,],%[^,]", &mameFileBufferTag[mameFileSize], &mameFileBufferLong[mameFileSize++]);
    fclose(mameFile);
}


int searchMameRom(char * fileName) {
    int len;
    
    for (int i = 0; i < mameFileSize; ++i) {
        if (strcmp(&mameFileBufferTag[i][0], fileName) == 0) {
            strcpy(romsList[romsFound].displayName, &mameFileBufferLong[i][0]);
            romsList[romsFound].displayName[strlen(romsList[romsFound].displayName) - 2] = '\0';
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

void initObjects(void) {
    char path[PATH_MAX + 1];
    for (uint8_t i = 0; i < NO_OBJECTS; ++i) {
        if (strcmp(objects[i].imgFile, "") != 0) {
            sprintf(&path[0], "%s/%s", ICONS_PATH, objects[i].imgFile);
            objects[i].img = IMG_Load(path);
        } 
    }
}

int loadJsonFile(char * path) {
    FILE *file = fopen(path, "r");
    if (file == NULL)  {
        fprintf(stderr, "Cannot open core mapping.\n");
        return 0;
    }
    fread(jsonFileBuffer, 1, sizeof(jsonFileBuffer), file);
    fclose(file);
    json = cJSON_Parse(jsonFileBuffer);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)  fprintf(stderr, "Error: %s\n", error_ptr);
        cJSON_Delete(json);
        return 0;
    }
    return 1;
}

int loadCoreMapping(void) {
    if (romsFound != 0) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(json,  romsList[selRomIdx].system);
        if ((cJSON_IsString(name) != 0) && (name->valuestring != NULL)) strcpy(romCoreMapping, name->valuestring);
    }
    return 1;
}

void loadSettings(void) {
    cJSON *name;
    loadJsonFile(SETTINGS_JSON_PATH) == 0;
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
    loadJsonFile(SKIN_PATH);
    skin.background = getColorFromJson("background", KB_C_BACKGND);
    skin.sbFrame = getColorFromJson("searchbox-frame",KB_C_SB_FRAME);
    skin.sb = getColorFromJson("searchbox", KB_C_SB);
    skin.st = getColorFromJson("search-text", KB_C_ST);
    skin.keySel = getColorFromJson("selected-key", KB_C_KEY_SEL);
    skin.key = getColorFromJson("unselected-key", KB_C_KEY);
    skin.info = getColorFromJson("text-info", KB_C_INFO);
    skin.cursor = getColorFromJson("cursor", KB_C_CURSOR);
    cJSON_Delete(json);
}

void searchFillRomList() {
    DIR * dir;
    DIR * console;
    char * ptrDispName;
    struct dirent * entry;
    struct dirent * rom;
    char path[PATH_MAX + 1];    
    romsFound = 0;
    if ((dir = opendir (ROMS_PATH)) == NULL) return;
    while ((entry = readdir (dir)) != NULL) {
        if ((entry->d_type == DT_DIR) && (strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0)) {
            sprintf(&path[0], "%s/%s", ROMS_PATH, entry->d_name);
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
                    if (strcasestr(romsList[romsFound].displayName, searchWord) != NULL) {
                        strcpy(romsList[romsFound].system, entry->d_name);
                        strcpy(romsList[romsFound++].fileName, rom->d_name);
                    }
                }                
            }
            closedir(console);
        }
    }
    closedir(dir);
}

int setup() {
    FILE *file = fopen(COMMAND_SH_PATH, "r");
    if (file != NULL)  return 0;
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
    textFont     = TTF_OpenFont(FONT_PATH, TEXT_FONT_SIZE);
    romsFont     = TTF_OpenFont(FONT_PATH, ROMS_FONT_SIZE);
    keyboardFont = TTF_OpenFont(FONT_PATH, KEYBOARD_FONT_SIZE);
    background   = IMG_Load(BACKGROUND_PATH); 
    if (background == NULL) {
        fprintf(stderr, "Error loading background.\n");
        return 0;
    }
    loadSettings();
    loadSkin();
    if (loadJsonFile(COREMAPPING_JSON_PATH) == 0) return 0;
    initObjects();
    bufferMameFile();
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
    fprintf(file, "/mnt/mmc/CFW/retroarch/retroarch -L \"/mnt/mmc/CFW/retroarch/.retroarch/cores/%s\" \"/mnt/mmc/Roms/%s/%s\"\n",
        romCoreMapping, romsList[selRomIdx].system, romsList[selRomIdx].fileName);
    fprintf(file, "exit $?\n");
    fclose(file);
    return 1;
}

void processInput() {
    uint16_t delta;
    SDL_PollEvent(&event);
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
                            if (strlen(searchWord) < SEARCH_WORD_SIZE) {
                                strcat(searchWord, keyboardLayout[selKeyIIdx][selKeyJIdx]);
                                startSearch = 1;
                            }
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
                        }
                        else running = 0;
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

void updateScreenshot(void) {
    char path[PATH_MAX + 1] = "";
    uint16_t  len;
    if (romsFound != 0) {
        objects[OBJ_SCREENSHOT_INDEX].imgVisible = 1;
        objects[OBJ_CONSOLE_INDEX].imgVisible = 1;
        sprintf(path, "%s/%s/Imgs/%s", ROMS_PATH, romsList[selRomIdx].system, romsList[selRomIdx].fileName);
        len = strlen(path);
        path[len-3] = 'p'; path[len-2] = 'n'; path[len-1] = 'g';
        SDL_FreeSurface(objects[OBJ_SCREENSHOT_INDEX].img);
        objects[OBJ_SCREENSHOT_INDEX].img  = IMG_Load(path); 
        sprintf(path, "%s/system/%s.png", ICONS_PATH, romsList[selRomIdx].system);
        SDL_FreeSurface(objects[OBJ_CONSOLE_INDEX].img);
        objects[OBJ_CONSOLE_INDEX].img = IMG_Load(path);
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
    updateScreenshot();
}

void drawRect(SDL_Rect pos, int o, SDL_Color c) {
    SDL_Rect invPos = rect(pos.x + o, pos.y, pos.w, pos.h);
    Uint32 mapColor = SDL_MapRGB(screen->format, c.r, c.g, c.b);
    if (strcmp(textAligment, "right") == 0)  SDL_FillRect(screen, &invPos, mapColor);
    else SDL_FillRect(screen, &pos, mapColor);
}

void drawText(SDL_Rect pos, int o,  char * text, TTF_Font *font, SDL_Color c) {
    SDL_Rect invPos = rect(pos.x + o, pos.y, pos.w, pos.h);
    if (textSurface != NULL) SDL_FreeSurface(textSurface);
    textSurface = TTF_RenderUTF8_Blended(font, text, c);
    if (strcmp(textAligment, "right") == 0) SDL_BlitSurface(textSurface, NULL, screen, &invPos);
    else SDL_BlitSurface(textSurface, NULL, screen, &pos);
}

void drawKey(int x, int y, int o, int txo, int tyo, char * text, SDL_Color c) {
    drawRect(rect(x, y, KY_W, KY_H), o, color(70, 68, 72));
    drawText(rect(x + txo, y + tyo, 0, 0), o, text, keyboardFont, c);
}

void drawObjects(void) {    
    for (uint8_t i; i < NO_OBJECTS; ++i) {
        if (objects[i].imgVisible == 1)   SDL_BlitSurface(objects[i].img, NULL, screen, &objects[i].imgPos);
        if (objects[i].textVisible == 1)  drawText(objects[i].textPos, 0, objects[i].name, textFont, activeColor);
    }
}

void drawRomsTextbox(void) {
    uint8_t first;
    SDL_Rect pos;
    int o;
    selRomIdx = (selRomIdx >= romsFound) ? romsFound - 1 : selRomIdx;  
    first = (selRomIdx < 4) ? 0 : selRomIdx - 3;
    for (uint8_t i = 0; (i < 8) && ((i + first)< romsFound); ++i) {
        pos = rect(textMargin, 140 + i * 25, 0, 0);
        o = WINDOW_W - strlen(romsList[i + first].displayName) * (ROMS_FONT_SIZE - 3) - 2 * textMargin;
        if ((i + first) == selRomIdx)  drawText(pos, o, romsList[i + first].displayName, romsFont, activeColor);
        else drawText(pos, o, romsList[i + first].displayName, romsFont, inactiveColor);
    }
}

void drawKeyboard() {
    int delta, o = KB_FLIP_OFFSET; 
    drawRect(rect(KB_X, KB_Y, KB_W, KB_H), o, skin.background);                   
    drawRect(rect(KB_X + 6, KB_Y + 10, SB_W, SB_H), o, skin.sbFrame);                    
    drawRect(rect(KB_X + 8, KB_Y + 12, SB_W - 4, SB_H - 4), o, skin.sb); 
    for (uint8_t i = 0; i < KB_ROWS; ++i) {
        for (uint8_t j = 0; j < KB_COLUMNS; ++j) {
            if ((i == selKeyIIdx) && (j == selKeyJIdx)) {
                drawRect(rect(KB_X + 5 + (KY_W + 2) * j, KB_Y + 67 + (KY_H + 2) * i, KY_W + 2, KY_H + 2), o, skin.key);
                drawKey(KB_X + 6 + (KY_W + 2) * j, KB_Y + 68 + (KY_H + 2) * i, o, 9, 11, keyboardLayout[i][j], skin.keySel);
            } else drawKey(KB_X + 6 + (KY_W + 2) * j, KB_Y + 68 + (KY_H + 2) * i, o, 9, 9, keyboardLayout[i][j], skin.key);
        }
        drawText(rect(KB_X + 8, KB_Y + 240, 0, 0), o, "Select/Space     Start/Done", keyboardFont, skin.info);
    }
    drawText(rect(KB_X + 16, KB_Y + 20, 0, 0), o, searchWord, romsFont, skin.st);
    delta = SDL_GetTicks() - cursorOld;
    if (delta > (2 * CURSOR_BLINK_DELAY)) {
        drawRect(rect(KB_X + (strlen(searchWord) + 1) * 17, KB_Y + 22, 2, 22), o, skin.sb);
        cursorOld = SDL_GetTicks();
    }
    else if (delta > CURSOR_BLINK_DELAY) drawRect(rect(KB_X + (strlen(searchWord) + 1) * 17, KB_Y + 22, 2, 22), o, skin.cursor);
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
    cJSON_Delete(json);
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

    if (setup() == 0) return 0;
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

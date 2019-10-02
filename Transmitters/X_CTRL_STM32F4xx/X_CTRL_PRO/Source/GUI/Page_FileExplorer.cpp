#include "FileGroup.h"
#include "DisplayPrivate.h"
#include "SdFat.h"
#include "LuaScript.h"

static lv_obj_t * tabviewFm;
static lv_obj_t * tabDrive;
static lv_obj_t * tabFileList;

static void Creat_PreloadLoading(lv_obj_t * parent, lv_obj_t** preload)
{
    /*Create a Preloader object*/
    *preload = lv_preload_create(parent, NULL);
    lv_obj_set_size(*preload, 50, 50);
    lv_obj_align(*preload, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_preload_set_type(*preload, LV_PRELOAD_TYPE_FILLSPIN_ARC);
}

static void Creat_Tabview(lv_obj_t** tabview)
{
    *tabview = lv_tabview_create(appWindow, NULL);
    lv_obj_set_size(*tabview, APP_WIN_WIDTH, APP_WIN_HEIGHT);
    lv_obj_align(*tabview, barStatus, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    tabDrive = lv_tabview_add_tab(*tabview, LV_SYMBOL_DRIVE);
    tabFileList = lv_tabview_add_tab(*tabview, LV_SYMBOL_LIST);
}

static void Creat_TabDrive(lv_obj_t * tab)
{
#define TO_MB(size) (size*0.000512f)
#define TO_GB(size) (TO_MB(size)/1024.0f)
    uint32_t cardSize = SD.card()->cardSize();
    uint32_t volFree = SD.vol()->freeClusterCount() * SD.vol()->blocksPerCluster();

    float totalSize = TO_GB(cardSize);
    float freeSize = TO_GB(volFree);
    float usageSize = totalSize - freeSize;

    /*LineMeter*/
    lv_obj_t * lmeter = lv_lmeter_create(tab, NULL);
    lv_obj_t * lmeterLabel = lv_label_create(lmeter, NULL);
    lv_obj_set_size(lmeter, 150, 150);
    lv_obj_align(lmeter, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_lmeter_set_range(lmeter, 0, 100);
    lv_lmeter_set_scale(lmeter, 240, 31);

    int16_t dispUseage = usageSize / totalSize * 100.0f;
    lv_lmeter_set_value(lmeter, dispUseage);
    lv_label_set_text_format(lmeterLabel, "%d%%", dispUseage);

    static lv_style_t labelStyle = *lv_obj_get_style(lmeterLabel);
    labelStyle.text.font = &lv_font_roboto_28;
    lv_label_set_style(lmeterLabel, LV_LABEL_STYLE_MAIN, &labelStyle);

    lv_obj_align(lmeterLabel, lmeter, LV_ALIGN_CENTER, 0, 0);

    /*Size*/
    lv_obj_t * labelSD = lv_label_create(lmeter, NULL);
    lv_label_set_text_format(labelSD, "%0.2fGB / %0.2fGB", usageSize, totalSize);
    lv_obj_align(labelSD, lmeter, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
}

static const char* GetFileSym(const char* filename)
{
    String Name = String(filename);
    Name.toLowerCase();

    const char *sym;
    
    if(Name.endsWith(".bv"))
    {
        sym = LV_SYMBOL_VIDEO;
    }
    else if(Name.endsWith(".wav"))
    {
        sym = LV_SYMBOL_AUDIO;
    }
    else if(Name.endsWith(".lua"))
    {
        sym = LV_SYMBOL_EDIT;
    }
    else
    {
        sym = LV_SYMBOL_FILE;
    }
    return sym;
}

char LuaBuff[1000];

static void OpenLuaFile(const char * filename)
{
    SdFile file;
    if(file.open(filename, O_RDONLY))
    {
        memset(LuaBuff, 0, sizeof(LuaBuff));
        file.read(LuaBuff, sizeof(LuaBuff));
        LuaCodeSet(LuaBuff);
        page.PageChangeTo(PAGE_LuaScript);
        file.close();
    }
}

static void FileEvent_Handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED)
    {
        const char *filename = lv_list_get_btn_text(obj);
        if(String(filename).endsWith(".lua"))
        {
            OpenLuaFile(filename);
        }
    }
}

int rootFileCount;
SdFile root;

static void Creat_TabFileList(lv_obj_t * tab, const char *path)
{
    if (!root.open(path))
    {
        return;
    }
    
    /*Create a list*/
    lv_obj_t * list = lv_list_create(tab, NULL);
    lv_obj_set_size(list, lv_obj_get_width_fit(tab) - 10, lv_obj_get_height_fit(tab) - 10);
    lv_obj_align(list, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_auto_realign(list, true);
    lv_list_set_edge_flash(list, true);
    
    /*Loading files*/
    SdFile file;
    while (file.openNext(&root, O_RDONLY))
    {
        if (!file.isHidden())
        {
            char fileName[50];
            file.getName(fileName, sizeof(fileName));
            const char *sym;
            
            if(file.isDir())
            {
                sym = LV_SYMBOL_DIRECTORY;
            }
            else
            {
                sym = GetFileSym(fileName);
            }

            lv_obj_t * list_btn = lv_list_add_btn(list, sym, fileName);
        
            lv_btn_set_ink_in_time(list_btn, 200);
            lv_btn_set_ink_out_time(list_btn, 200);
            lv_obj_set_event_cb(list_btn, FileEvent_Handler);
            
            rootFileCount++;
        }
        file.close();
    }
}

/**
  * @brief  页面初始化事件
  * @param  无
  * @retval 无
  */
static void Setup()
{
    Creat_Tabview(&tabviewFm);

    lv_obj_t * preLoading;
    Creat_PreloadLoading(tabviewFm, &preLoading);

    Creat_TabDrive(tabDrive);

    Creat_TabFileList(tabFileList, "/");

    lv_obj_del_safe(&preLoading);
    
    if(page.LastPage != PAGE_Home)
    {
        lv_tabview_set_tab_act(tabviewFm, 1, LV_ANIM_OFF);
    }
}

/**
  * @brief  页面退出事件
  * @param  无
  * @retval 无
  */
static void Exit()
{
    lv_obj_del_safe(&tabviewFm);
    root.close();
}

/**
  * @brief  页面事件
  * @param  event:事件编号
  * @param  param:事件参数
  * @retval 无
  */
static void Event(int event, void* param)
{
    lv_obj_t * btn = (lv_obj_t*)param;
    if(event == LV_EVENT_CLICKED)
    {
        if(btn == btnBack)
        {
            page.PageChangeTo(PAGE_Home);
        }
    }
}

/**
  * @brief  页面注册
  * @param  pageID:为此页面分配的ID号
  * @retval 无
  */
void PageRegister_FileExplorer(uint8_t pageID)
{
    page.PageRegister(pageID, Setup, NULL, Exit, Event);
}

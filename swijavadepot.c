/*
 * @file swijavadepot.c
 * @brief  实现java参数存储仓库
 * @author bianzhonglin
 * @history  bianzhonglin created 2013.1.6
 *			 
 */
#include "swapi.h"
#include "swidepot.h"
#include "swijavadepot.h"
#include "swstb.h"
#include "hwnmpd_priv.h"

/*java参数仓库对象*/
typedef struct _ijava_depot
{
	sw_idepot_t depot;  /* 必须放在第一个位置，后面要转换为sw_idepot_t类型给swparameter用 */
    java_map_t *map;    /* 映射那些参数存储在java数据库中*/
    int size;
    wcallback_t w_cb;    /* 写回调函数 */
    rcallback_t r_cb;   /* 读回调函数 */
}ijava_depot_t;

static ijava_depot_t  *m_ideopt = NULL;

static bool ijava_load( sw_idepot_t *depot,spread_para_func spread );
static bool ijava_get( sw_idepot_t *depot, char* name ,char* value,int size );
static bool ijava_set( sw_idepot_t *depot, char* name ,char* value );
static bool ijava_save( sw_idepot_t *depot, gather_para_func gather );

/**
 * @brief 打开参数仓库(兼容接口)
 * @param map 只是那些参数存储在java仓库里面
 * @param size 参数大小
 * @return success=仓库句柄, false=NULL 
 */
sw_idepot_t* sw_ijavadepot_open(java_map_t *map, int size)
{
	ijava_depot_t* idepot = NULL;
	 
	idepot = (ijava_depot_t*)malloc(sizeof(ijava_depot_t));
	if(idepot == NULL )
		return NULL;
	sw_memset((void*)idepot, sizeof(ijava_depot_t), 0, sizeof(ijava_depot_t));

	sw_strlcpy(idepot->depot.name, sizeof(idepot->depot.name), "ijava", sizeof(idepot->depot.name));
    m_ideopt = idepot;
    idepot->map = map;
    idepot->size = size;
	idepot->depot.load = ijava_load;
	idepot->depot.get = ijava_get;
	idepot->depot.set = ijava_set;
	idepot->depot.save = ijava_save;
	idepot->depot.type = IDEPOT_JAVA;
	return (sw_idepot_t*)idepot;
}

/**
 * @brief 关闭创建的idepot对象(兼容接口)
 *
 * @param idepot对象
 */
void sw_ijavadepot_close(sw_idepot_t* depot)
{
	if( depot )
	{
		free(depot);
	}
    m_ideopt = NULL;
}

static bool ijava_load( sw_idepot_t *depot,spread_para_func spread )
{
    if(depot == NULL)
    {
        HWNMPD_LOG_DEBUG("ijava load error, depot is NULL!\n");
        return false;
    }
    int i;
    ijava_depot_t *idepot = (ijava_depot_t *)depot; 

    for(i = 0; i < idepot->size; i++)
    {
#if 0
        char buf[128];
        memset(buf, 0, sizeof(buf));
        if(idepot->r_cb)
            idepot->r_cb(idepot->map[i].name, buf, sizeof(buf));
        if(buf[0])
            spread(depot, idepot->map[i].name, buf);
        else
#endif
            spread(depot, idepot->map[i].name, NULL);
    }
	return true;
}

static bool ijava_get( sw_idepot_t *depot, char* name ,char* value,int size )
{
    ijava_depot_t *idepot = (ijava_depot_t *)depot;
	if(idepot->r_cb)
	{
	    idepot->r_cb(name, value, size);
	    return true;
	}
	return false;
}

static bool ijava_set( sw_idepot_t *depot, char* name ,char* value )
{
    ijava_depot_t *idepot = (ijava_depot_t *)depot;
	if(idepot->w_cb)
	{
	    idepot->w_cb(name, value);
	    return true;
	}
	return false;
}

static bool ijava_save( sw_idepot_t *depot,gather_para_func gather )
{
	return true;
}

/**
 * @brief 上层参数修改通知
 */
int  sw_ijavadepot_on_updateevnet( const char* name)
{
	return 1;
}

/**
 * @brief 设置java仓库的读写回调函数
 * @param wfun 写回调函数
 * @param wfun 读回调函数
 */
int sw_ijavadepot_set_wrcallback( void* wfun, void* rfun )
{
    if(m_ideopt)
    {
        m_ideopt->w_cb = wfun;
        m_ideopt->r_cb = rfun;
    }
	return 1;
}

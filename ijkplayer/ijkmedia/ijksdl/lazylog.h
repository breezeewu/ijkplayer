/***************************************************************************************************************
 * filename     lazylog.h
 * describe     write log to console and local disk file
 * author       Created by dawson on 2019/04/18
 * Copyright    ©2007 - 2029 Sunvally. All Rights Reserved.
 ***************************************************************************************************************/
#ifndef _LAZY_LOG_H_
#define _LAZY_LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#define snprintf    sprintf_s
#ifndef MUTEX_LOCK_DEFITION
#define MUTEX_LOCK_DEFITION
#define sv_mutex                    CRITICAL_SECTION
#define init_mutex(px)              InitializeCriticalSection(px)
#define deinit_mutex(px)            DeleteCriticalSection(px)
#define lock_mutex(px)              EnterCriticalSection(px)
#define unlock_mutex(px)            LeaveCriticalSection(px)
#endif
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#ifndef MUTEX_LOCK_DEFITION
#define MUTEX_LOCK_DEFITION
#define sv_mutex                    pthread_mutex_t
#define init_mutex(px)               pthread_mutexattr_t attr; \
        pthread_mutexattr_init(&attr); \
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); \
        pthread_mutex_init(px, &attr);
//#define init_mutex(px)              pthread_mutex_init(px, NULL)
#define deinit_mutex(px)            pthread_mutex_destroy(px)
#define lock_mutex(px)              pthread_mutex_lock(px)
#define unlock_mutex(px)            pthread_mutex_unlock(px)
#endif
#endif
//#include <fcntl.h>
#pragma warning(disable:4996)
// log level define
#define LOG_LEVEL_VERB            0x1
#define LOG_LEVEL_INFO            0x2
#define LOG_LEVEL_TRACE           0x3
#define LOG_LEVEL_WARN            0x4
#define LOG_LEVEL_ERROR           0x5
#define LOG_LEVEL_DISABLE         0x6

//log level name
static const char* verb_name   = "verb";
static const char* info_name   = "info";
static const char* trace_name  = "trace";
static const char* warn_name   = "warn";
static const char* error_name  = "error";
/*#define LOG_LEVEL_NAME_VERB         "verb"
#define LOG_LEVEL_NAME_INFO         "info"
#define LOG_LEVEL_NAME_TRACE        "trace"
#define LOG_LEVEL_NAME_WARN         "warn"
#define LOG_LEVEL_NAME_ERROR        "error"*/

// log output flag define
// console output log info
#define LOG_OUTPUT_FLAG_CONSOLE     0x1
// file output log info
#define LOG_OUTPUT_FLAG_FILE        0x2
#define MAX_LOG_SIZE 4096
#define check_pointer(ptr, ret)     if(NULL == ptr){return ret;}

/**
 * log context describe
 **/
typedef struct tlog_context
{
    // mutex
    sv_mutex*    pmutex;

    // file descriptor
    FILE*       pfd;

    // log write size
    int64_t     llwrite_size;

    // log level
    int         nlog_level;

    // log output falg
    int         nlog_output_flag;

    // log write path
    char*       plog_path;
    char*       plog_dir;

    // log buffer
    char*       plog_buf;
    int         nlog_buf_size;

    // version info
    int         nmayjor_version;
    int         nminor_version;
    int         nmicro_version;
    int         ntiny_version;
    
    int		ngmtime;
}   log_ctx;

// log_ctx* g_plogctx; need to be define in cpp
extern log_ctx* g_plogctx;

// log micro define
#define sv_init_log(path, level, flag,  vmayjor, vminor, vmicro, vtiny)       g_plogctx = init_file_log(path, level, flag,  vmayjor, vminor, vmicro, vtiny)
#define sv_verb(fmt, ...)               log_trace(g_plogctx, LOG_LEVEL_VERB, NULL, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
//#define sv_info
#define sv_info(fmt, ...)               log_trace(g_plogctx, LOG_LEVEL_INFO, NULL, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define sv_trace(fmt, ...)              log_trace(g_plogctx, LOG_LEVEL_TRACE, NULL, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define sv_warn(fmt, ...)               log_trace(g_plogctx, LOG_LEVEL_WARN, NULL, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define sv_error(fmt, ...)              log_trace(g_plogctx, LOG_LEVEL_ERROR, NULL, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define sv_memory(level, pmen, size)    log_memory(g_plogctx, level,pmen, size);

#define lbtrace sv_trace
#define lberror sv_error

static unsigned long get_sys_time();
static long get_time_zone();
static void mkdir_if_not_exist(const char* pdir);
/**********************************************************************************************************
 * create log context
 * @parma nlog_leve 日志输出等级, 
 * @param nlog_output_flag 日志输出标识, 1,输出到控制台，2输出到文件，3同时输出到控制台和文件
 * @param plog_path 日志输出路径
 * @return 日志上下文结构体
 **********************************************************************************************************/
static log_ctx* open_log_contex(const int nlog_leve, int nlog_output_flag, const char* plog_path)
{
    if(NULL == plog_path && (LOG_OUTPUT_FLAG_FILE & nlog_output_flag))
    {
        printf("plog_path:%p, nlog_output_flag:%d, failed!\n", plog_path, nlog_output_flag);
        return NULL;
    }

    // create log context
    log_ctx* plogctx = (log_ctx*)malloc(sizeof(log_ctx));
    check_pointer(plogctx, NULL);
    memset(plogctx, 0, sizeof(log_ctx));
    plogctx->nlog_level        = nlog_leve;

    // create and init mutex
    plogctx->pmutex = (sv_mutex*)malloc(sizeof(sv_mutex));
    init_mutex(plogctx->pmutex);

    // create and copy log path
    plogctx->plog_path         = (char*)malloc(strlen(plog_path)+1);
    strcpy(plogctx->plog_path, plog_path);
    plogctx->nlog_output_flag  = nlog_output_flag;

    // create log buffer
    plogctx->plog_buf          = (char*)malloc(MAX_LOG_SIZE);
    plogctx->nlog_buf_size     = MAX_LOG_SIZE;
    plogctx->ngmtime	       = 1;
    printf("open_log_contex end, plogctx:%p\n", plogctx);
    return plogctx;
}

/**********************************************************************************************************
 * close log file and destroy log context
 * @parma pplogctx 日志上下文双重指针，执行成功后该指针将被置空
 **********************************************************************************************************/
static void close_log_contex(log_ctx** pplogctx)
{
    if(NULL == pplogctx || NULL == *pplogctx)
    {
        return ;
    }

    log_ctx* plogctx = *pplogctx;
    // lock the mutex before close file
    lock_mutex(plogctx->pmutex);
    if(plogctx->pfd)
    {
        fclose(plogctx->pfd);
        plogctx->pfd = NULL;
    }
    if(plogctx->plog_path)
    {
        free(plogctx->plog_path);
        plogctx->plog_path = NULL;
    }
    if(plogctx->plog_dir)
    {
        free(plogctx->plog_dir);
        plogctx->plog_dir = NULL;
    }
    if(plogctx->plog_buf)
    {
        free(plogctx->plog_buf);
        plogctx->plog_buf = NULL;
    }
    unlock_mutex(plogctx->pmutex);

    // destroy mutex last
    if(plogctx->pmutex)
    {
        deinit_mutex(plogctx->pmutex);
        free(plogctx->pmutex);
        plogctx->pmutex = NULL;
    }
    free(plogctx);
    plogctx = NULL;
    *pplogctx = NULL;
}

/**********************************************************************************************************
 * set current log version info
 * @parma:plogctx 日志上下文结构体
 * @param nmayjor  程序主版本号，程序主体架构有很大的调整时（如重构）变更
 * @param nminor   程序次版本号，程序功能和结构迭代时变更
 * @param nmicro   程序微版本号, 程序修改一些小bug和小幅度的功能调整时变更
 * @param ntiny    程序极小版本号，程序日志增删、同一个类型bug的不同修改时变更
 * @return 0为成功，否则为失败
 **********************************************************************************************************/
static int set_current_version(log_ctx* plogctx, const int nmayjor, const int nminor, const int nmicro, const int ntiny)
{
    if(NULL == plogctx)
    {
        return -1;
    }

    lock_mutex(plogctx->pmutex);
    plogctx->nmayjor_version   = nmayjor;
    plogctx->nminor_version    = nminor;
    plogctx->nmicro_version    = nmicro;
    plogctx->ntiny_version     = ntiny;
    unlock_mutex(plogctx->pmutex);

    return 0;
}

/**********************************************************************************************************
 * init write file log in a simple way
 * @param plogdir 日志输出路径
 * @param nloglevel 日志等级
 * @param nlogoutputflag 日志输出标识, 输出到文件，控制台
 * @param nmayjor   程序主版本号，程序主体架构有很大的调整时（如重构）变更
 * @param nminor    程序次版本号，程序功能和结构迭代时变更
 * @param nmicro    程序微版本号, 程序修改一些小bug和小幅度的功能调整时变更
 * @param ntiny     程序极小版本号，程序日志增删、同一个类型bug的不同修改时变更
 * @return 日志上下文结构体
 **********************************************************************************************************/
static log_ctx* init_file_log(const char* plogdir, const int nloglevel, const int nlogoutputflag,  const int nmayjor, const int nminor, const int nmicro, const int ntiny)
{
    char logpath[256] = {0};
    if(plogdir && (nlogoutputflag&LOG_OUTPUT_FLAG_FILE))
    {
        printf("if plogdir:%p\n", plogdir);
		mkdir_if_not_exist(plogdir);
/*#ifdef _WIN32
		if(_access(plogdir, 0) != 0)
		{
			mkdir(plogdir);
		}
#else
        if(access(plogdir, F_OK) != 0)
        {
            mkdir(plogdir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            printf("plog_dir:%s\n", plogdir);
        }
#endif*/
        strcpy(logpath, plogdir);
        if(logpath[strlen(logpath)-1] != '/')
        {
            logpath[strlen(logpath)] = '/';
        }
        
        sprintf(&logpath[strlen(logpath)], "hwdec_%lu.log", get_sys_time());
    }

    log_ctx* plogctx = open_log_contex(nloglevel, nlogoutputflag, logpath);
    printf("plogctx:%p = open_log_contex\n", plogctx);
    plogctx->plog_dir = (char*)malloc(strlen(plogdir)+1);
    strcpy(plogctx->plog_dir, plogdir);
    set_current_version(plogctx, nmayjor, nminor, nmicro, ntiny);
    printf("init_file_log end, plogctx:%p\n", plogctx);
    return plogctx;
}

/**********************************************************************************************************
 * create log line header with code file name, line, function name, thread id, process id, level, context_id
 * @parma plogctx 日志上下文结构体
 * @param plogbuf 用于接收头部信息的buffer
 * @param ptag  输出日志标签信息
 * @param pfilepath 输出日志的代码文件名
 * @param line  输出日志的代码位置（行数）
 * @param pfun  输出日志的函数名称
 * @param nlevel 输出日志的等级
 * @param pheader_size 头部信息的buffer大小，头部创建成功后返回头部信息大小
 * @return 0为成功，否则为失败
 **********************************************************************************************************/
static int generate_header(log_ctx* plogctx, char* plogbuf, const char* ptag,  const char* pfilepath, const int line, const char* pfun, const int nlevel, int* pheader_size)
{
    int log_header_size = -1;
    int log_buf_size;
    char loghdr[256] = {0};
    const char* pfilename = NULL;
    const char* plevelname = NULL;


    if(NULL == plogbuf || NULL == pheader_size)
    {
        return -1;
    }

    log_buf_size = *pheader_size;
    // clock time
#if 1
	struct tm* ptm;
	time_t rawtime;
    time(&rawtime);
#else
	struct timeval tv;
    struct tm* tm = NULL;
    if (gettimeofday(&tv, NULL) == -1)
    {
        return -1;
    }
#endif
    //search the last '/' at pfile_path, return pos + 1
	if (pfilepath)
	{
		pfilename = strrchr(pfilepath, '/');
		if(pfilename)
		{
			pfilename += 1;
		}
		else
		{
			pfilename = pfilepath;
		}
	}
    
    //printf("pfilename:%s, pfile_path:%s\n", pfilename, pfile_path);
    if(ptag)
    {
        sprintf(loghdr, "%s %s:%d %s", ptag, pfilename, line, pfun);
    }
    else
    {
        sprintf(loghdr, "%s:%d %s", pfilename,line, pfun);
    }

    // use local time, if you want to use utc time, please use tm = gmtime(&tv.tv_sec)
    if(plogctx->ngmtime)
    {
		ptm = gmtime(&rawtime);
		//printf("tm:%p = gmtime(&tv.tv_sec)\n", tm);
		if(!ptm)
		{
			printf("tm:%p = gmtime(&tv.tv_sec) failed\n", ptm);
			return -1;
		}
	//printf("after tm = gmtime(&tv.tv_sec)\n");
    }
    else if ((ptm = localtime(&rawtime)) == NULL)
    {
		printf("ptm:%p = localtime(&tv.tv_sec) failed\n", ptm);
        return -1;
    }
    //printf("before switch nlevel:%d\n", nlevel);
    // from level to log name
    switch(nlevel)
    {
        case LOG_LEVEL_VERB:
            plevelname = verb_name;
        break;
        case LOG_LEVEL_INFO:
            plevelname = info_name;
        break;
        case LOG_LEVEL_TRACE:
            plevelname = trace_name;
        break;
        case LOG_LEVEL_WARN:
            plevelname = warn_name;
        break;
        case LOG_LEVEL_ERROR:
            plevelname = error_name;
        break;
        case LOG_LEVEL_DISABLE:
        default:
            return 0;
    }
    unsigned long int threadid = 0;
#ifdef _WIN32
    threadid = (unsigned long int)GetCurrentThread();
#else
    threadid = pthread_self();
#endif
    // write log header
    if (nlevel >= LOG_LEVEL_ERROR && ptm) {
        log_header_size = snprintf(plogbuf, log_buf_size, 
            "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%s][%lu][%p][%d][%s] ",
            1900 + ptm->tm_year, 1 + ptm->tm_mon, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, (int)(get_sys_time() / 1000), 
            plevelname, loghdr, threadid, plogctx, errno, strerror(errno));
    } else {
        log_header_size = snprintf(plogbuf, log_buf_size, 
            "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%s][%lu][%p]",
            1900 + ptm->tm_year, 1 + ptm->tm_mon, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, (int)(get_sys_time() / 1000), 
            plevelname, loghdr, threadid, plogctx);
    }

    if (log_header_size == -1) {
        return -1;
    }

    // write the header size.
    *pheader_size = (log_buf_size - 1) > log_header_size ? log_header_size : (log_buf_size - 1);
    //printf("generate_header end\n");
    return log_header_size;
}

/**********************************************************************************************************
 * write log to the output
 * @parma plogctx 日志上下文结构体
 * @param plogbuf 输出日志信息buffer
 * @param nloglen 输出日志信息长度
 * @return 0为成功，否则为失败
 **********************************************************************************************************/
static int write_log(log_ctx* plogctx, const int nlevel, char* plogbuf, const int nloglen)
{
    int loglen = nloglen;
    if(NULL == plogctx || NULL == plogbuf || nloglen <= 0 || 0 == plogctx->nlog_output_flag)
    {
        return -1;
    }

    loglen = (plogctx->nlog_buf_size - 1) > nloglen ? nloglen : (plogctx->nlog_buf_size - 1);

    // add some to the end of char.
    if('\n' != plogbuf[loglen-1])
    {
        plogbuf[loglen++] = '\n';
    }
    plogctx->llwrite_size += loglen;

    // if not to file, to console and return.
    if (LOG_OUTPUT_FLAG_CONSOLE & plogctx->nlog_output_flag)
    {
        // if is error msg, then print color msg.
        // \033[31m : red text code in shell
        // \033[32m : green text code in shell
        // \033[33m : yellow text code in shell
        // \033[0m : normal text code
        if (nlevel <= LOG_LEVEL_TRACE) {
            printf("%.*s", loglen, plogbuf);
        } else if (nlevel == LOG_LEVEL_WARN) {
            printf("\033[33m%.*s\033[0m", loglen, plogbuf);
        } else{
            printf("\033[31m%.*s\033[0m", loglen, plogbuf);
        }
        //fflush(stdout);
    }

    if((LOG_OUTPUT_FLAG_FILE & plogctx->nlog_output_flag))
    {
	    fwrite(plogbuf, 1, loglen, plogctx->pfd);
		if (nlevel >= LOG_LEVEL_ERROR)
		{
			fflush(plogctx->pfd);
		}
    }

    return 0;
}

/**********************************************************************************************************
 * get system version
 * @parma psysver buf to receive system version
 * @parms len buffer len
 * @return system version length if success, else fail 
 **********************************************************************************************************/
static int get_system_version(char* psysver, int len)
{
#ifdef WIN32
	OSVERSIONINFO sVersionInfo;
	memset((BYTE*)&sVersionInfo, 0, sizeof(sVersionInfo));
	sVersionInfo.dwOSVersionInfoSize = sizeof(sVersionInfo);

	//获得计算机硬件信息
	BOOL ret = GetVersionEx(&sVersionInfo);
	sprintf_s(psysver, len, "%d.%d.%d", sVersionInfo.dwMajorVersion, sVersionInfo.dwMinorVersion, sVersionInfo.dwBuildNumber);
	return strlen(psysver);
#else 
	FILE* pfile = fopen("/proc/sys/kernel/version", "r");
    int readlen = 0;
    if(pfile)
    {
        readlen = (int)fread(psysver, 1, 80, pfile);
        fclose(pfile);
        pfile = NULL;
    }
	return readlen;
#endif
   
}

/**********************************************************************************************************
 * log to the output
 * @parma plogctx 日志上下文结构体
 * @parms nlevel 日志输出等级
 * @param tag 日志输出标签
 * @param pfile 日志输出代码文件名称
 * @param line 日志输出代码位置（行数）
 * @param pfun 日志输出代码函数名称
 * @param fmt , ... 日志输出格式化字符串及其可变参数
 * @return 0为成功，否则为失败
**********************************************************************************************************/
static int log_trace(log_ctx* plogctx, const int nlevel, const char* tag, const char* pfile, int line, const char* pfun, const char* fmt, ...)
{
    int ret = -1;
    int nlog_buf_size = 0;
    check_pointer(plogctx, -1);
    if (plogctx->nlog_level > nlevel)
    {
        return 0;
    }
    lock_mutex(plogctx->pmutex);
    nlog_buf_size = plogctx->nlog_buf_size;
    if (!generate_header(plogctx, plogctx->plog_buf, tag, pfile, line, pfun, nlevel, &nlog_buf_size))
    {
        unlock_mutex(plogctx->pmutex);
        return ret;
    }

    // format log info string
    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    nlog_buf_size += vsnprintf(plogctx->plog_buf + nlog_buf_size, plogctx->nlog_buf_size - nlog_buf_size, fmt, ap);
    va_end(ap);

    if(NULL == plogctx->pfd && plogctx->plog_path && (LOG_OUTPUT_FLAG_FILE & plogctx->nlog_output_flag))
    {
		printf("open log file\n");
    	plogctx->pfd = fopen(plogctx->plog_path, "wb");
		printf("plogctx->pfd:%p = fopen(plogctx->plog_path:%s, wb)", plogctx->pfd, plogctx->plog_path);
		printf("after open log file:%p\n", plogctx->pfd);
        char loghdr[256] = {0};
        sprintf(loghdr, "version:%d.%d.%d.%d, timezone:%d, system version:", plogctx->nmayjor_version, plogctx->nminor_version, plogctx->nmicro_version, plogctx->ntiny_version, get_time_zone());
        get_system_version(loghdr + strlen(loghdr), 256 - (int)strlen(loghdr));

        write_log(plogctx, nlevel, loghdr, (int)strlen(loghdr));
    }
    ret = write_log(plogctx, nlevel, plogctx->plog_buf, nlog_buf_size);
    unlock_mutex(plogctx->pmutex);

    return ret;
}

/**********************************************************************************************************
 * log memory to the output
 * @parma plogctx 日志上下文结构体
 * @parms nlevel generate_header 日志输出等级
 * @param pmemory 输出内存指针
 * @param len 输出内存大小
 * @return 0为成功，否则为失败
 **********************************************************************************************************/
static int log_memory(log_ctx* plogctx, const int nlevel, const char* pmemory, int len)
{
    check_pointer(plogctx, -1);
    if(plogctx->nlog_level > nlevel)
    {
        return 0;
    }
    lock_mutex(plogctx->pmutex);
    const unsigned char* psrc = (unsigned char*)pmemory;
    memset(plogctx->plog_buf, 0, plogctx->nlog_buf_size);
    sprintf(plogctx->plog_buf, "size:%d, memory:", len);
    for(int i = 0; i < len && (int)strlen(plogctx->plog_buf) < plogctx->nlog_buf_size; i++)
    {
        int val = *(psrc + i);
        sprintf(plogctx->plog_buf + strlen(plogctx->plog_buf), "%02x", val);
    }
    write_log(plogctx, nlevel, plogctx->plog_buf, (int)strlen(plogctx->plog_buf));
    unlock_mutex(plogctx->pmutex);
    return 0;
}

/**********************************************************************************************************
 * get current system time in microsecond
 * @return system current time in microseconds
**********************************************************************************************************/
static unsigned long get_sys_time()
{
	unsigned long curtime = 0;
#ifdef _WIN32
	return GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    curtime = tv.tv_sec*1000 + tv.tv_usec/1000;
#endif
    return curtime;
}

static long get_time_zone()
{
	long time_zone = 0;
#ifdef _WIN32
	time_t time_utc = 0;
	struct tm *p_tm_time;
 
	p_tm_time = localtime( &time_utc );   //转成当地时间
	time_zone = ( p_tm_time->tm_hour > 12 ) ?   ( p_tm_time->tm_hour-=  24 )  :  p_tm_time->tm_hour;
#else
	struct timeval tv;
	struct timezone tz;
    gettimeofday(&tv, &tz);
	time_zone = tz.tz_minuteswest/60;
#endif

	return time_zone;
}

/**********************************************************************************************************
 * get current log path dir
 * @return 成功返回日志路径指针常量，失败返回NULL
**********************************************************************************************************/
static const char* get_log_path()
{
    if(!g_plogctx)
    {
        return NULL;
    }
    
    return g_plogctx->plog_dir;
}

static void mkdir_if_not_exist(const char* pdir)
//static void mkdir_if_not_exist(const char* pdir)
{
#ifdef _WIN32
	if(_access(pdir, 0) != 0)
	{
		_mkdir(pdir);
	}
#else
    if(access(pdir, F_OK) != 0)
    {
        mkdir(pdir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
#endif
}
/**********************************************************************************************************
 * write file in log path with a simple way
 * @parma ppfile [in, out] 文件句柄指针取地址, 如果为NULL,则根据pfilename创建文件，如果不为NULL，则直接在已有句柄中写入数据
 * @parma pfilename [in] 文件名称，支持相对路径设置
 * @parma plen [in, out] 输入：日志所在目录缓存大小，输出：日志所在目录字符串长度
 * @return 成功返回日志路径拷贝长度，-1:为失败，0:为ppath缓存大小不够，无法拷贝，将在plen中返回实际需要的缓存大小
**********************************************************************************************************/
static int simple_write_file_in_log_path(FILE** ppfile, char* pfilename, char* pdata, int len)
{
    if(!ppfile)
    {
        return -1;
    }
    int writelen = 0;
    FILE* pfile = *ppfile;
    if(NULL == pfile)
    {
        char logpath[256] = {0};
        int pathlen = 256;
        strcpy(logpath, get_log_path());
        //get_log_path(g_plogctx, logpath, &pathlen);
        if(pathlen > 0)
        {
            char* pname = strrchr(pfilename, '/');
            if(pname && (pfilename != pname))
            {
                // 存在相对路径
                int copylen = (int)(pname + 1 - pfilename);
                memcpy(logpath + strlen(logpath), pfilename, copylen);
                // 如果不存在指定目录，则创建该目录
				mkdir_if_not_exist(logpath);
                /*if(access(logpath, F_OK) != 0)
                {
                    mkdir(logpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                    printf("mkdir:%s\n", logpath);
                }*/
            }
            else
            {
                //不存在相对路径
                sprintf(logpath + strlen(logpath), "/%s", pfilename);
            }
        }
        else
        {
            strcpy(logpath, pfilename);
        }
        
        pfile = fopen(logpath, "wb");
        *ppfile = pfile;
    }
    
    if(pfile)
    {
        writelen = (int)fwrite(pdata, 1, len, pfile);
		//fflush(pfile);
    }
    
    return writelen;
}

static int simple_read_file_in_log_path(FILE** ppfile, char* pfilename, char* pdata, int len)
{
    if(!ppfile)
    {
        return -1;
    }
    
    int readlen = 0;
    FILE* pfile = *ppfile;
    if(NULL == pfile)
    {
        char logpath[256] = {0};
        int pathlen = 256;
        strcpy(logpath, get_log_path());
        //get_log_path(g_plogctx, logpath, &pathlen);
        if(pathlen > 0)
        {
            sprintf(logpath + strlen(logpath), "/%s", pfilename);
        }
        pfile = fopen(logpath, "rb");
        *ppfile = pfile;
    }
    
    if(pfile)
    {
        readlen = (int)fread(pdata, 1, len, pfile);
    }
    return readlen;
}

static int simple_read_file(FILE** ppfile,  const char* ppath, char* prelpath, char* pdata, int len)
{
    if(!ppfile ||  !prelpath || !pdata)
    {
        return -1;
    }
    
    long readlen = 0;
    FILE* pfile = *ppfile;
    if(!pfile)
    {
        char filepath[256] = {0};
        if(ppath && strlen(ppath) > 0)
        {
            strcpy(filepath, ppath);
        }
        strcat(filepath, prelpath);
        pfile = *ppfile = fopen(filepath, "rb");
    }
    
    if(pfile)
    {
        readlen = fread(pdata, 1, len, pfile);
    }
    
    return (int)readlen;
}

static int simple_write_file(FILE** ppfile,  const char* ppath, const char* prelpath, const char* pdata, int len)
{
    if(!ppfile || !prelpath || !pdata || len <= 0)
    {
        return -1;
    }
    
    long writelen = 0;
    FILE* pfile = *ppfile;
    if(!pfile)
    {
        char filepath[256] = {0};
        if(ppath && strlen(ppath) > 0)
        {
            strcpy(filepath, ppath);
        }
        
        char* pfilename = strrchr((char*)prelpath, '/');
        if(!pfilename || pfilename == prelpath)
        {
            // 不存在相对目录，可以直接拼接
            strcat(filepath, prelpath);
        }
        else
        {
            memcpy(filepath + strlen(filepath), prelpath, pfilename - prelpath);
			mkdir_if_not_exist(filepath);
            /*if(0 != access(filepath, F_OK))
            {
                mkdir(filepath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            }*/
            strcat(filepath, pfilename);
        }
        pfile = *ppfile = fopen(filepath, "wb");
		sv_trace("pfile:%p, open url %s, ppath:%s, prelpath:%s", pfile, filepath, ppath, prelpath);
    }
    
    if(pfile)
    {
        writelen = fwrite(pdata, 1, len, pfile);
    }
    
    return (int)writelen;
}

static void ffmpeg_log_callback(void *ptr, int level, const char *fmt, va_list vl)
{
	int ff_level = level;
	if (level <= 16)//AV_LOG_ERROR
	{
		level = LOG_LEVEL_ERROR;
	}
	else if(level == 24)//AV_LOG_WARNING
	{
		level = LOG_LEVEL_WARN;
	}
	/*else if(level == 32)//AV_LOG_INFO
	{
		level = LOG_LEVEL_TRACE;
	}*/
	else
	{
		level = LOG_LEVEL_TRACE;
		return;
	}

	char temp[1024];
	int offset = 0;
	sprintf_s(temp, 1024, "level:%d ", ff_level);
	offset = strlen(temp);
	vsprintf(temp + offset, fmt, vl);
	size_t len = strlen(temp);
	if (len > 0 && len < 1024 && temp[len - 1] == '\n')
	{
		temp[len - 1] = '\0';
	}

	//AVClass* avc = ptr ? *(AVClass **)ptr : NULL;
	//const char *module = avc ? avc->item_name(ptr) : "NULL";
	log_trace(g_plogctx, level, NULL, NULL, -1, NULL, temp);
	/* (strstr(module, "264"))
	{
		if (strstr(temp, "error"))
		{
			//ffmpeg_decoder_error++;
		}

	}*/
}

static void lazysleep(int ms)
{
#ifdef WIN32
	Sleep(ms);
#else
	usleep(ms * 1000);
#endif
}
//log_ctx* g_plog_ctx = NULL;// 全局变量
//sv_init_log(pLogPath, nLogLevel, nLogFlag, SDK_VERSION_MAYJOR, SDK_VERSION_MINOR, SDK_VERSION_MACRO, SDK_VERSION_TINY);// 初始化
//sv_trace("hello!");
//sv_error("fatal error, ret:%d", ret);
#endif

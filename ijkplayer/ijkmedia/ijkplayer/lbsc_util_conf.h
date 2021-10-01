#ifndef LBSC_UTIL_CONF_H_
#define LBSC_UTIL_CONF_H_
// fopen fgets fclose
#include <stdio.h>
//memset memcpy
#include <stdlib.h>
#include "list.h"

#ifndef lbtrace
#define lbtrace printf
#endif
#ifndef lbdebug
#define lbdebug printf
#endif
#ifndef lberror
#define lberror printf
#endif
#define ENABLE_SDK_CONFIG_FILE
typedef struct conf_pair
{
	char* ptag;
	char* pvalue;
} conf_tag_value_pair;
typedef struct conf_context
{
	list_ctx*	plist_ctx;
	void* 		powner;
} conf_ctx;

extern conf_ctx* g_pconf_ctx;
typedef int (*on_opt_read)(void* powner, const char* ptag, const char* pvalue);

static int read_conf_line(const char* pcfg_line, char* ptag, int taglen, char* pvalue, int valuelen)
{
	if(NULL == pcfg_line || NULL == ptag || taglen <= 0 || NULL == pvalue || valuelen <= 0)
	{
		printf("Invalid parameter, pcfg_line:%s, ptag:%s, taglen:%d, pvalue:%s, valuelen:%d\n", pcfg_line, ptag, taglen, pvalue, valuelen);
		return -1;
	}
	int i = 0;
	int tag_begin = 0, tag_end = 0;
	int value_begin = 0;
	int value_end = 0;
	//printf("read conf:%s\n", pcfg_line);
	// find first not empty char
	while(' ' == pcfg_line[i] || '\t' == pcfg_line[i])i++;
    if('#' == pcfg_line[i])
    {
        return -1;
    }
	tag_begin = i;
    //printf("&pcfg_line[i]:%s, tag_begin:%d\n", &pcfg_line[i], tag_begin);
	while(' ' != pcfg_line[i] && '\t' != pcfg_line[i] && '\0' !=  pcfg_line[i] && '#' !=  pcfg_line[i])i++;
	if('\0' == pcfg_line[i])
	{
		return -1;
	}
	tag_end = i;
    //printf("&pcfg_line[i]:%s, tag_end:%d\n", &pcfg_line[i], tag_end);
	if(taglen > tag_end - tag_begin + 1)
	{
        //lbtrace("before memset taglen:%d\n", taglen);
		memset(ptag, 0, taglen);
        //lbtrace("after memset taglen:%d\n", taglen);
		memcpy(ptag, &pcfg_line[tag_begin], tag_end - tag_begin);
        //lbtrace("ptag:%s, &pcfg_line[tag_begin]:%s, tag_end:%d - tag_begin:%d\n", ptag, &pcfg_line[tag_begin], tag_end, tag_begin);
	}
	else
	{
		printf("not enought buffer for conf tag, have %d,  needn %d, conf line:%s\n", taglen, tag_end - tag_begin + 1, pcfg_line);
		return -1;
	}
    //lbtrace("%d, %d t:%d\n", (int)pcfg_line[i], (int)pcfg_line[i+1], (int)'\t');
	while(' ' == pcfg_line[i] || '\t' == pcfg_line[i])i++;
	value_begin = i;
    
	value_end = strlen(pcfg_line);
    //lbtrace("value_begin:%d, pcfg_line[value_begin]:%d pcfg_line[value_end-1]:%d\n", value_begin, (int)pcfg_line[value_begin], (int)pcfg_line[value_end-1]);
	while(' ' == pcfg_line[value_end-1] || '\t' == pcfg_line[value_end-1] || ';' == pcfg_line[value_end-1] || '\n' == pcfg_line[value_end-1] || '\r' == pcfg_line[value_end-1])value_end--;
	lbtrace("value_end:%d, strlen(pcfg_line):%d\n", value_end, strlen(pcfg_line));
	if(valuelen > value_end - value_begin)
	{
		memset(pvalue, 0, valuelen);
		memcpy(pvalue, &pcfg_line[value_begin], value_end - value_begin);
	}
	else
	{
		printf("not enought buffer for conf tag value, have %d,  needn %d, conf line:%s\n", valuelen, value_end - value_begin, pcfg_line);
		return -1;
	}
	lbtrace("pcfg_line:%s, ptag:%s, pvalue:%s\n", pcfg_line, ptag, pvalue);
	return 0;
}

static int insert_conf_pair(conf_ctx* pcc, const char* ptag, const char* pvalue)
{
	if(NULL == pcc || NULL == ptag || NULL == pvalue)
	{
		lberror("%s invalid param, pcc:%p, ptag, pvalue:%p\n", pcc, ptag, pvalue);
		return -1;
	}

	// if tag is exist
	BEGIN_ENUM_LIST(pcc->plist_ctx, ptmp)
	conf_tag_value_pair* ppair = (conf_tag_value_pair*)ptmp->pitem;
	if(0 == memcmp(ppair->ptag, ptag, strlen(ptag) + 1))
	{
		lbtrace("ptag:%s, ppair->pvalue:%s instread by pvalue:%s\n", ptag, ppair->pvalue, pvalue);
		free(ppair->pvalue);
		ppair->pvalue = (char*)malloc(strlen(pvalue) + 1);
		memcpy(ppair->pvalue, pvalue, strlen(pvalue) + 1);
		return 0;
	}
	END_ENUM_LIST()

	// new tag, insert pair to list
	conf_tag_value_pair* pair = (conf_tag_value_pair*)malloc(sizeof(conf_tag_value_pair));
	int taglen = strlen(ptag) + 1;
	int vallen = strlen(pvalue) + 1;
	pair->ptag = (char*)malloc(taglen);
	memcpy(pair->ptag, ptag, taglen);
	pair->pvalue = (char*)malloc(vallen);
	memcpy(pair->pvalue, pvalue, vallen);
	push(pcc->plist_ctx, pair);

	return 0;
}

static conf_ctx* load_config(const char* pconf_file, void* powner, on_opt_read pread_func)
{
	conf_ctx* pcc = NULL;
	FILE* pfile = fopen(pconf_file, "rb");
	lbtrace("pfile:%p = fopen(pconf_file:%s, rb)\n", pfile, pconf_file);
	char tag[256] = {0};
	char value[256] ={0};

	if(pfile)
	{
		pcc = (conf_ctx*)malloc(sizeof(conf_ctx));
		memset(pcc, 0, sizeof(conf_ctx));
		pcc->plist_ctx = list_context_create(1000);
		pcc->powner = powner;
		do
		{
			char line[512] = {0};
			char* pline = fgets(line, 512, pfile);
			//printf("2 read pline:%p, line:%s\n", pline, line);
			if(NULL == pline)
			{
				break;
			}
            
			int ret = read_conf_line(line, tag, 256, value, 256);
			lbtrace("ret:%d = read_conf_line(line:%s, tag:%s, 256, value:%s, 256)\n", ret, line, tag, value);
			if(0 != ret)
			{
				lbtrace("ret:%d = read_conf_line(line:%s, tag, 256, value, 256) failed\n", ret, line);
			}
			else if(0 == ret && pread_func)
			{
				pread_func(powner, tag, value);
			}
			insert_conf_pair(pcc, tag, value);
			lbtrace("insert_conf_pair(pcc:%p, tag:%s, value:%s)\n", pcc, tag, value);
		}while(1);
		
		fclose(pfile);
		lbtrace("fclose(pfile:%p)\n", pfile);
		pfile = NULL;
	}

	return pcc;
}

static const char* get_conf_value_by_tag(conf_ctx* pcc, const char* ptag)
{
	if(NULL == pcc || NULL == ptag)
	{
		lberror("%s invalid param, pcc:%p, ptag\n", pcc, ptag);
		return -1;
	}

	BEGIN_ENUM_LIST(pcc->plist_ctx, ptmp)
	conf_tag_value_pair* ppair = (conf_tag_value_pair*)ptmp->pitem;
	if(0 == memcmp(ppair->ptag, ptag, strlen(ptag) + 1))
	{
		return ppair->pvalue;
	}
	END_ENUM_LIST()
	lberror("find tag:%s failed, no value pair was found\n", ptag);
	return NULL;
}

static void close_config(conf_ctx** ppcc)
{
	if(ppcc)
	{
		conf_ctx* pcc = *ppcc;
		free(pcc);
		*ppcc = pcc = NULL;
	}
}

static int get_config_int_value(conf_ctx* pcc, const char* ptag, int* pnvalue)
{
    assert(pcc);
    assert(ptag);
    assert(pnvalue);

    //conf_tag_value_pair* pair = list_at(pconf_ctx->plist_ctx, );
    //BEGIN_ENUM_LIST(pconf_ctx->plist_ctx, ptmp)
    for(struct list_node* ptmp = pcc->plist_ctx->head; ptmp != NULL; ptmp = ptmp->pnext){
    conf_tag_value_pair* ppair = (conf_tag_value_pair*)ptmp->pitem;
    if(0 == memcmp(ppair->ptag, ptag, strlen(ptag) + 1))
    {
        *pnvalue = atoi(ppair->pvalue);
        return 0;
    }
    }
    //END_ENUM_LIST()
    return -1;
}

static int get_config_string_value(conf_ctx* pcc, const char* ptag, char* pvalue, int val_len)
{
	assert(pcc);
	assert(ptag);
	assert(pvalue);
	//conf_tag_value_pair* pair = list_at(pconf_ctx->plist_ctx, );
	//BEGIN_ENUM_LIST(pconf_ctx->plist_ctx, ptmp)
	for(struct list_node* ptmp = pcc->plist_ctx->head; ptmp != NULL; ptmp = ptmp->pnext){
	conf_tag_value_pair* ppair = (conf_tag_value_pair*)ptmp->pitem;
	if(0 == memcmp(ppair->ptag, ptag, strlen(ptag) + 1))
	{
		int tag_val_len = strlen(ppair->pvalue) + 1;
		int copylen = val_len > tag_val_len ? tag_val_len : val_len;
		memcpy(pvalue, ppair->pvalue, copylen);
		return 0;
	}
	}
	//END_ENUM_LIST()
	return -1;
}

static int get_default_string_config(const char* ptag, char* pval, int val_len)
{
    int ret = 0;
    if(NULL == g_pconf_ctx)
    {
        return -1;
    }

    ret = get_config_string_value(g_pconf_ctx, ptag, pval, val_len);
    if(0 == ret)
    {
        return (int)strlen(pval);
    }

    return -1;
}

static int get_default_int_config(const char* ptag, int* pval)
{
    int ret = 0;
    if(NULL == g_pconf_ctx)
    {
        return 0;
    }

    ret = get_config_int_value(g_pconf_ctx, ptag, pval);

    return ret;
}



static int get_config_double_value(conf_ctx* pcc, const char* ptag, double* pdvalue)
{
	assert(pcc);
	assert(ptag);
	assert(pdvalue);

	//conf_tag_value_pair* pair = list_at(pconf_ctx->plist_ctx, );
	//BEGIN_ENUM_LIST(pconf_ctx->plist_ctx, ptmp)
	for(struct list_node* ptmp = pcc->plist_ctx->head; ptmp != NULL; ptmp = ptmp->pnext){
	conf_tag_value_pair* ppair = (conf_tag_value_pair*)ptmp->pitem;
	if(0 == memcmp(ppair->ptag, ptag, strlen(ptag) + 1))
	{
		*pdvalue = atof(ppair->pvalue);
		return 0;
	}
	}
	//END_ENUM_LIST()
	return -1;
}

static int get_default_double_config(const char* ptag, double* pval)
{
    int ret = 0;
    if(NULL == g_pconf_ctx)
    {
        return 0;
    }

    ret = get_config_double_value(g_pconf_ctx, ptag, pval);

    return ret;
}

static int parser_args(char** args, int argc, void* powner, on_opt_read popt_read)
{
    int ret = 0;
    //assert(ppi);
    //memset(ppi, 0, sizeof(push_ctx));
    int i;
    for(i = 1; i < argc; i++)
    {
		if('-' == args[i][0] && i + 1 < argc)
		{
			popt_read(powner, args[i], args[i+1]);
			i++;
		}
    }

    return ret;
}

#endif

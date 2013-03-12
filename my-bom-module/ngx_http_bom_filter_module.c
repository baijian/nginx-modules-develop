/*
 * Delete bom info filter
 * Type: filter  bom_filter_module
 * Author: baijian
 * */
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct{
    ngx_flag_t  enable;
}ngx_http_bom_loc_conf_t;

typedef struct{
    ngx_flag_t  enable;
}ngx_http_bom_ctx_t;

static void *ngx_http_bom_create_loc_conf(ngx_conf_t *cf);

static char *ngx_http_bom_merge_loc_conf(ngx_conf_t *cf, 
        void *parent, void *child);

static ngx_int_t ngx_http_bom_filter_init(ngx_conf_t *cf);

static ngx_command_t ngx_http_bom_filter_commands[] = {

    {   ngx_string("bom"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_bom_loc_conf_t, enable),
        NULL},

    ngx_null_command
};

static ngx_http_module_t ngx_http_bom_filter_module_ctx = {
    NULL,
    ngx_http_bom_filter_init,

    NULL,
    NULL,

    NULL,
    NULL,

    ngx_http_bom_create_loc_conf,
    ngx_http_bom_merge_loc_conf
};

ngx_module_t ngx_http_bom_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_bom_filter_module_ctx,
    ngx_http_bom_filter_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt ngx_http_next_body_filter;

static ngx_int_t
ngx_http_bom_header_filter(ngx_http_request_t *r){

    ngx_http_bom_ctx_t *ctx;
    ngx_http_bom_loc_conf_t *lcf;

    lcf = ngx_http_get_module_loc_conf(r, ngx_http_bom_filter_module);
    if(!lcf->enable
        || r->header_only
        || r != r->main
        || r->headers_out.status == NGX_HTTP_NO_CONTENT)
    {
        return ngx_http_next_header_filter(r);
    }

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_bom_ctx_t));
    if(ctx == NULL){
        return NGX_ERROR;
    }
    ngx_http_set_ctx(r, ctx, ngx_http_bom_filter_module);
    ctx->enable = 1;

    //r->main_filter_need_in_memory = 1;

    r->filter_need_in_memory = 1;
    if(r == r->main){
        ngx_http_clear_content_length(r);
        ngx_http_clear_last_modified(r);
    }
    return ngx_http_next_header_filter(r);
}

static ngx_int_t 
ngx_http_bom_body_filter(ngx_http_request_t *r, ngx_chain_t *in){

    u_char                  *p;
    ngx_buf_t               *b,*nb;
    ngx_chain_t             *cl;
    ngx_http_bom_ctx_t      *ctx;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "http bom body filter");

    ctx = ngx_http_get_module_ctx(r, ngx_http_bom_filter_module);
    
    if(ctx == NULL||in == NULL){
        return ngx_http_next_body_filter(r, in);
    }

    cl = in;
    b = cl->buf;
    p = b->pos;
    if(p[0] == 0xef && p[1] == 0xbb && p[2] == 0xbf){
        nb = ngx_calloc_buf(r->pool);
        if(nb == NULL){
            return NGX_ERROR;
        }
        nb->pos = b->pos + 3;
        nb->start = b->start + 3;
        nb->last = b->last;
        nb->end = b->end;
        nb->memory = 1;
        nb->last_buf = b->last_buf;
        cl->buf = nb;
    }
    return ngx_http_next_body_filter(r, in);
}


static ngx_int_t 
ngx_http_bom_filter_init(ngx_conf_t *cf){

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_bom_body_filter;

    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_bom_header_filter;

    return NGX_OK;
}

static void * ngx_http_bom_create_loc_conf(ngx_conf_t *cf){

    ngx_http_bom_loc_conf_t *conf;
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_bom_loc_conf_t));
    if(conf == NULL){
        return NULL;
    }
    conf->enable = NGX_CONF_UNSET;
    return conf;
}

static char * 
ngx_http_bom_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child){
    
    ngx_http_bom_loc_conf_t *prev = parent;
    ngx_http_bom_loc_conf_t *conf = child;
    return NGX_CONF_OK;
}



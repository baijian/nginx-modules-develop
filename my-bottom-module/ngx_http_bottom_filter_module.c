/*
 * My first filter module
 * Type: filter    bottom_filter module
 * Author: baijian
 * */
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct{
    ngx_http_complex_value_t    *bottom;
}ngx_http_bottom_loc_conf_t;

//switch
typedef struct{
    ngx_str_t  bottom;
}ngx_http_bottom_ctx_t;

static char *ngx_http_bottom_filter(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static void *ngx_http_bottom_create_loc_conf(ngx_conf_t *cf);

static char *ngx_http_bottom_merge_loc_conf(ngx_conf_t *cf, 
        void *parent, void *child);

static ngx_int_t ngx_http_bottom_filter_init(ngx_conf_t *cf);

static ngx_command_t ngx_http_bottom_filter_commands[] = {

    {   ngx_string("bottom"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_bottom_filter,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL},

    ngx_null_command
};

static ngx_http_module_t ngx_http_bottom_filter_module_ctx = {
    NULL,
    ngx_http_bottom_filter_init,

    NULL,
    NULL,

    NULL,
    NULL,

    ngx_http_bottom_create_loc_conf,
    ngx_http_bottom_merge_loc_conf
};

ngx_module_t ngx_http_bottom_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_bottom_filter_module_ctx,
    ngx_http_bottom_filter_commands,
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
ngx_http_bottom_header_filter(ngx_http_request_t *r){

    ngx_http_bottom_ctx_t *ctx;
    ngx_http_bottom_loc_conf_t *lcf;

    lcf = ngx_http_get_module_loc_conf(r, ngx_http_bottom_filter_module);

    if(lcf->bottom == (ngx_http_complex_value_t *) - 1
        || r->header_only
        || r != r->main
        || r->headers_out.status == NGX_HTTP_NO_CONTENT)
    {
        return ngx_http_next_header_filter(r);
    }

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_bottom_ctx_t));
    if(ctx == NULL){
        return NGX_ERROR;
    }
    if(ngx_http_complex_value(r, lcf->bottom, &ctx->bottom) != NGX_OK){
        return NGX_ERROR;
    }
    ngx_http_set_ctx(r, ctx, ngx_http_bottom_filter_module);
    //ctx->bottom = lcf->bottom;
    
    if(r->headers_out.content_length_n != -1){
        r->headers_out.content_length_n += ctx->bottom.len;
    }
    if(r->headers_out.content_length){
        r->headers_out.content_length->hash = 0;
        r->headers_out.content_length = NULL;
    }
    ngx_http_clear_accept_ranges(r);

    /* 
    r->filter_need_in_memory = 1;
    if(r == r->main){
        ngx_http_clear_content_length(r);
        ngx_http_clear_last_modified(r);
    }*/
    return ngx_http_next_header_filter(r);
}

static ngx_int_t 
ngx_http_bottom_body_filter(ngx_http_request_t *r, ngx_chain_t *in){

    u_char                  *p,*pp;
    ngx_uint_t              last;
    ngx_buf_t               *b,*bb;
    ngx_chain_t             *ll,*cl,*nl;
    ngx_http_bottom_loc_conf_t   *lcf;
    ngx_http_bottom_ctx_t      *ctx;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "http bottom body filter");

    ctx = ngx_http_get_module_ctx(r, ngx_http_bottom_filter_module);
    
    if(ctx == NULL||in == NULL){
        return ngx_http_next_body_filter(r, in);
    }
    last = 0;
    for(cl = in;cl ;cl = cl->next){
        if(cl->buf->last_buf){
            last = 1;
            break;
        }
    }
    if(!last){
        return ngx_http_next_body_filter(r, in);
    }
    b = ngx_calloc_buf(r->pool);
    if(b == NULL){
        return NGX_ERROR;
    }
    nl = ngx_alloc_chain_link(r->pool);
    if(nl == NULL){
        return NGX_ERROR;
    }
    b->pos = ctx->bottom.data;
    b->last = b->pos + ctx->bottom.len;
    b->start = b->pos;
    b->end = b->last;
    b->last_buf = 1;
    b->memory = 1;
    nl->buf = b;
    nl->next = NULL;
    cl->next = nl;
    cl->buf->last_buf = 0;
    
    return ngx_http_next_body_filter(r, in);
}


static ngx_int_t 
ngx_http_bottom_filter_init(ngx_conf_t *cf){

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_bottom_body_filter;

    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_bottom_header_filter;

    return NGX_OK;
}

static char * 
ngx_http_bottom_filter(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){

    ngx_str_t *value;
    ngx_http_complex_value_t   **cvt;
    
    cvt = &((ngx_http_bottom_loc_conf_t *)conf)->bottom;

    if(*cvt != NULL){
        return 'duplicate';
    }

    value = cf->args->elts;

    if((value + 1)->len){
        cmd->offset = offsetof(ngx_http_bottom_loc_conf_t, bottom);
        return ngx_http_set_complex_value_slot(cf, cmd, conf);    
    }
    *cvt = (ngx_http_complex_value_t *) - 1;
    return NGX_OK;
}

static void * ngx_http_bottom_create_loc_conf(ngx_conf_t *cf){

    ngx_http_bottom_loc_conf_t *conf;
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_bottom_loc_conf_t));
    if(conf == NULL){
        return NULL;
    }
    //conf->bottom.len = 0;
    //conf->bottom.data = NULL;
    return conf;
}

static char * 
ngx_http_bottom_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child){
    
    ngx_http_bottom_loc_conf_t *prev = parent;
    ngx_http_bottom_loc_conf_t *conf = child;
    //ngx_conf_merge_str_value(conf->bottom,prev->bottom,10);
    if(conf->bottom == NULL){
        conf->bottom = prev->bottom;
    }
    if(conf->bottom == NULL){
        conf->bottom = (ngx_http_complex_value_t *) - 1;
    }
    return NGX_CONF_OK;
}



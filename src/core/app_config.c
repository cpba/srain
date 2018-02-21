#include "app.h"

SrnApplicationConfig *srn_application_config_new(void){
    return g_malloc0(sizeof(SrnApplicationConfig));
}

void srn_application_config_free(SrnApplicationConfig *cfg){
    g_free(cfg);
}

SrnRet srn_application_config_check(SrnApplicationConfig *cfg){
    return SRN_OK;
}


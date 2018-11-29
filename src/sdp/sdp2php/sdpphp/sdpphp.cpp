/*
   +----------------------------------------------------------------------+
   | unknown license:                                                      |
   +----------------------------------------------------------------------+
   +----------------------------------------------------------------------+
*/

/* $ Id: $ */ 

#include "php_sdpphp.h"
#include "sdpphp_imp.h"

#if HAVE_SDPPHP

#ifdef  __cplusplus
extern "C" {
#endif

/* {{{ sdpphp_functions[] */
zend_function_entry sdpphp_functions[] = {
	ZEND_FE(printSdp, NULL)
	ZEND_FE(sdpToString, NULL)
	ZEND_FE(stringToSdp, NULL)
	{ NULL, NULL, NULL }
};
/* }}} */


/* {{{ sdpphp_module_entry
 */
zend_module_entry sdpphp_module_entry = {
	STANDARD_MODULE_HEADER,
	"sdpphp",
	sdpphp_functions,
	PHP_MINIT(sdpphp),     /* Replace with NULL if there is nothing to do at php startup   */ 
	PHP_MSHUTDOWN(sdpphp), /* Replace with NULL if there is nothing to do at php shutdown  */
	PHP_RINIT(sdpphp),     /* Replace with NULL if there is nothing to do at request start */
	PHP_RSHUTDOWN(sdpphp), /* Replace with NULL if there is nothing to do at request end   */
	PHP_MINFO(sdpphp),
	PHP_SDPPHP_VERSION, 
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_SDPPHP
ZEND_GET_MODULE(sdpphp)
#endif


/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(sdpphp)
{

	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(sdpphp)
{

	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(sdpphp)
{
	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RSHUTDOWN_FUNCTION */
PHP_RSHUTDOWN_FUNCTION(sdpphp)
{
	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(sdpphp)
{
	php_printf("The unknown extension\n");
	php_info_print_table_start();
	php_info_print_table_row(2, "Version",PHP_SDPPHP_VERSION " (devel)");
	php_info_print_table_row(2, "Released", "2014-05-29");
	php_info_print_table_row(2, "CVS Revision", "$Id: $");
	php_info_print_table_end();
	/* add your stuff here */

}
/* }}} */

#ifdef  __cplusplus
} // extern "C"
#endif

#endif /* HAVE_SDPPHP */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

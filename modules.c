#include <dirent.h>
#include <string.h>
#include <dlfcn.h>

#include <sys/types.h>

#include "vwm.h"
#include "modules.h"
#include "profile.h"
#include "private.h"
#include "strings.h"
#include "list.h"

static int _vwm_module_init(const char *);

vwm_module_t*
vwm_module_create(void)
{
    vwm_module_t    *module;

    module = (vwm_module_t*)calloc(1, sizeof(vwm_module_t));

    return module;
}

void
vwm_module_set_type(vwm_module_t *mod, int type)
{
    if(mod == NULL) return;

    mod->type = type;

    return;
}

int
vwm_module_get_type(vwm_module_t *mod)
{
    if(mod == NULL) return -1;

    return mod->type;
}

void
vwm_module_set_title(vwm_module_t *mod, char *title)
{
    if(mod == NULL) return;
    if(title == NULL) return;

    memset(mod->title, 0, NAME_MAX);
    strncpy(mod->title, title, NAME_MAX - 1);

    return;
}

void
vwm_module_get_title(vwm_module_t *mod, char *buf, int buf_sz)
{
    if(mod == NULL) return;
    if(buf == NULL) return;
    if(buf_sz < 1) return;

    if(buf_sz > NAME_MAX - 1) buf_sz = NAME_MAX - 1;

    memset(buf, 0, buf_sz);
    strncpy(buf, mod->title, buf_sz - 1);

    return;
}

void
vwm_module_set_userptr(vwm_module_t *mod, void *anything)
{
    if(mod == NULL) return;

    mod->anything = anything;

    return;
}

void*
vwm_module_get_userptr(vwm_module_t *mod)
{
    if(mod == NULL) return NULL;

    return mod->anything;
}

vwnd_t*
vwm_module_exec(vwm_module_t *mod)
{
    vwnd_t  *vwnd;

    if(mod == NULL) return NULL;

    if(mod->main == NULL) return NULL;

    vwnd = mod->main(mod);

    return vwnd;
}


char*
vwm_modules_load(char *module_dir)
{
    vwm_t               *vwm;
    vwm_module_t        *module = NULL;
	char			    modpath[NAME_MAX];
	DIR				    *search_dir = NULL;
	struct dirent	    *dirent_file = NULL;
    struct list_head    *pos;
    char                *buffer = NULL;
    char                *error_msg = NULL;
    int                 retval;

    if(module_dir == NULL) return NULL;
        search_dir = opendir(module_dir);
    if(search_dir == NULL)
    {
        buffer = strdup_printf("[EE] Error opening module directory:\n%s",
            module_dir);

        return buffer;
    }

    vwm = vwm_get_instance();

	while((dirent_file = readdir(search_dir)) != NULL)
	{
		if(strcmp(dirent_file->d_name, ".") == 0) continue;
		if(strcmp(dirent_file->d_name, "..") == 0) continue;

		// fix module file name (maybe not necessary)
		if(module_dir[strlen(module_dir) - 1] == '/')
			sprintf(modpath, "%s%s", module_dir, dirent_file->d_name);
		else
			sprintf(modpath, "%s/%s", module_dir, dirent_file->d_name);

		// check to see if module is already registered
        list_for_each(pos, &vwm->module_list)
        {
            module = list_entry(pos, vwm_module_t, list);

   			if(strstr(module->modpath, modpath) != NULL) break;

            module = NULL;
        }

		// module isn't already registered
		if(module == NULL)
        {
            retval = _vwm_module_init(modpath);

            if(retval != 0)
            {
                switch(retval)
                {
                    case -1:    error_msg = dlerror();                  break;
                    case -2:    error_msg = "vwm_mod_main() not found"; break;
                    default:    error_msg = "error from module init";   break;
                }

                if(error_msg != NULL) buffer = strdup_printf("%s\n%s\n\n%s",
                    "Could not register module:", modpath, error_msg);
                else
                    buffer = strdup_printf("%s\n%s\n",
                    "Could not register module:", modpath);
            }
        }
    }

    closedir(search_dir);

	/*	obligatory clean up	*/
    if(buffer != NULL) return buffer;

	return NULL;
}

int
vwm_module_add(const vwm_module_t *mod)
{
	vwm_t		        *vwm;
    struct list_head    *pos;
    vwm_module_t        *module = NULL;

    if(mod->type == 0) return -1;
    if(mod->title == '\0') return -1;

    if(mod->modpath == '\0') return -1;
    if(strlen(mod->modpath) > NAME_MAX - 1) return -1;

	vwm = vwm_get_instance();

	// add the application to the list
    list_add(&mod->list, &vwm->module_list);

	return 0;
}

vwm_module_t*
vwm_module_find_by_title(char *title)
{
	vwm_t			    *vwm;
	vwm_module_t	    *module = NULL;
	struct list_head    *pos;

	if(title == NULL) return NULL;
	vwm = vwm_get_instance();

    list_for_each(pos, &vwm->module_list)
    {
        module = list_entry(pos, vwm_module_t, list);

        if(strcmp(module->title, title) == 0) break;

        module = NULL;
    }

	return module;
}

vwm_module_t*
vwm_module_find_by_type(vwm_module_t *prev_mod, int type)
{
    vwm_t               *vwm;
    vwm_module_t        *module = NULL;

    if(type < 0) return NULL;

    vwm = vwm_get_instance();

    if(list_empty(&vwm->module_list)) return NULL;

    // if the previous item was the last one, return.
    if(prev_mod != NULL)
    {
        if(list_is_last(&prev_mod->list, &vwm->module_list)) return NULL;
    }

    // first iteration
    if(prev_mod == NULL)
    {
        prev_mod = list_first_entry(&vwm->module_list, vwm_module_t, list);

        // return module if match found
        if(prev_mod->type == type) return prev_mod;
        // return NULL if no match found and is the last item
        if(list_is_last(&prev_mod->list, &vwm->module_list)) return NULL;
    }

    module = prev_mod;
    module = list_next_entry(module, list);

    // subsequent iteration
    list_for_each_entry_from(module, &vwm->module_list, list)
    {
        if(module->type == type) return module;
    }

    return NULL;
}


static int
_vwm_module_init(const char *modpath)
{
    // extern char     **vwm_argv;
    // extern int      vwm_argc;

    void            *handle = NULL;
    vwm_module_t    *mod = NULL;
    int             retval = 0;
    int             (*constructor)(vwm_module_t *);

    handle = dlopen(modpath, RTLD_LAZY | RTLD_LOCAL);

    if(handle == NULL) return -1;

    *(void **)(&constructor) = dlsym(handle,"vwm_mod_init");

    // return error if the module constructor could not be mapped
    if(constructor == NULL)
    {
        dlclose(handle);
        return -2;
    }

    mod = (vwm_module_t*)calloc(1, sizeof(vwm_module_t));
    strncpy(mod->modpath, modpath, NAME_MAX - 1);

    // call the module constructor
    retval = constructor(mod);

    // handle "user error" from module constructor
    if(retval != 0)
    {
        free(mod);
        dlclose(handle);
        return -3;
    }

    return 0;
}

int
vwm_menu_helper(vk_widget_t *widget, void *anything)
{
    vwm_module_t    *module;

    if(anything == NULL) return NULL;

    module = (vwm_module_t *)anything;

    module->main(anything);

    return 0;
}

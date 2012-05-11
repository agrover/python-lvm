/* -*- Mode: C; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil -*- */ 
/*
 * Liblvm -- Python interface to LVM2 API.
 *
 * Copyright (C) 2010 Red Hat, Inc. All rights reserved.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Lars Sjostrom (lars sjostrom redhat com)
 *          Andy Grover (agrover redhat com)
 *
 */

#include <Python.h>
#include <lvm2app.h>

typedef struct {
    PyObject_HEAD
    lvm_t    libh;                    /* lvm lib handle */
} lvmobject;

typedef struct {
    PyObject_HEAD
    vg_t      vg;                    /* vg handle */
    lvmobject *lvm_obj;
} vgobject;

typedef struct {
    PyObject_HEAD
    lv_t      lv;                    /* lv handle */
    lvmobject *lvm_obj;
} lvobject;

typedef struct {
    PyObject_HEAD
    pv_t      pv;                    /* pv handle */
    lvmobject *lvm_obj;
} pvobject;

static PyTypeObject LibLVMvgType;
static PyTypeObject LibLVMlvType;
static PyTypeObject LibLVMpvType;

static PyObject *LibLVMError;


/* ----------------------------------------------------------------------
 * LVM object initialization/deallocation
 */

static int
liblvm_init(lvmobject *self, PyObject *arg)
{
    char *systemdir = NULL;

    if (!PyArg_ParseTuple(arg, "|s", &systemdir))
       return -1;

    self->libh = lvm_init(systemdir);
    if (lvm_errno(self->libh)) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    return 0;
}

static void
liblvm_dealloc(lvmobject *self)
{
    /* if already closed, don't reclose it */
    if (self->libh != NULL){
        lvm_quit(self->libh);
    }
    //self->ob_type->tp_free((PyObject*)self);
    PyObject_Del(self);
}

static PyObject *
liblvm_get_last_error(lvmobject *self)
{
    PyObject *info;

    if((info = PyTuple_New(2)) == NULL)
        return NULL;

    PyTuple_SetItem(info, 0, PyInt_FromLong((long) lvm_errno(self->libh)));
    PyTuple_SetItem(info, 1, PyString_FromString(lvm_errmsg(self->libh)));

    return info;
}

static PyObject *
liblvm_library_get_version(lvmobject *self)
{
    return Py_BuildValue("s", lvm_library_get_version());
}


static PyObject *
liblvm_close(lvmobject *self)
{
    /* if already closed, don't reclose it */
    if (self->libh != NULL)
        lvm_quit(self->libh);

    self->libh = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
liblvm_lvm_list_vg_names(lvmobject *self)
{
    struct dm_list *vgnames;
    const char *vgname;
    struct lvm_str_list *strl;
    PyObject * rv;
    int i = 0;

    if ((vgnames = lvm_list_vg_names(self->libh))== NULL)
        goto error;

    if (dm_list_empty(vgnames))
        goto error;

    rv = PyTuple_New(dm_list_size(vgnames));
    if (rv == NULL)
        return NULL;

    dm_list_iterate_items(strl, vgnames) {
        vgname = strl->str;
        PyTuple_SET_ITEM(rv, i, PyString_FromString(vgname));
        i++;
    }

    return rv;

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(self));
    return NULL;
}

static PyObject *
liblvm_lvm_list_vg_uuids(lvmobject *self)
{
    struct dm_list *uuids;
    const char *uuid;
    struct lvm_str_list *strl;
    PyObject * rv;
    int i = 0;

    if ((uuids = lvm_list_vg_uuids(self->libh))== NULL)
        goto error;

    if (dm_list_empty(uuids))
        goto error;

    rv = PyTuple_New(dm_list_size(uuids));
    if (rv == NULL)
        return NULL;
    dm_list_iterate_items(strl, uuids) {
        uuid = strl->str;
        PyTuple_SET_ITEM(rv, i, PyString_FromString(uuid));
        i++;
    }

    return rv;

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(self));
       return NULL;
}

static PyObject *
liblvm_lvm_vgname_from_pvid(lvmobject *self, PyObject *arg)
{
    const char *pvid;
    const char *vgname;
    if (!PyArg_ParseTuple(arg, "s", &pvid))
        return NULL;

    if((vgname = lvm_vgname_from_pvid(self->libh, pvid)) == NULL) {
        PyErr_SetObject(LibLVMError, liblvm_get_last_error(self));
        return NULL;
    }

    return Py_BuildValue("s", vgname);
}

static PyObject *
liblvm_lvm_vgname_from_device(lvmobject *self, PyObject *arg)
{
    const char *device;
    const char *vgname;
    if (!PyArg_ParseTuple(arg, "s", &device))
        return NULL;

    if((vgname = lvm_vgname_from_device(self->libh, device)) == NULL) {
        PyErr_SetObject(LibLVMError, liblvm_get_last_error(self));
        return NULL;
    }

    return Py_BuildValue("s", vgname);
}

static PyObject *
liblvm_lvm_config_reload(lvmobject *self)
{
    int rval;

    if((rval = lvm_config_reload(self->libh)) == -1) {
        PyErr_SetObject(LibLVMError, liblvm_get_last_error(self));
        return NULL;
    }

    return Py_BuildValue("i", rval);
}


static PyObject *
liblvm_lvm_scan(lvmobject *self)
{
    int rval;

    if((rval = lvm_scan(self->libh)) == -1) {
        PyErr_SetObject(LibLVMError, liblvm_get_last_error(self));
        return NULL;
    }

    return Py_BuildValue("i", rval);
}

static PyObject *
liblvm_lvm_config_override(lvmobject *self, PyObject *arg)
{
    const char *config;
    int rval;

    if (!PyArg_ParseTuple(arg, "s", &config))
       return NULL;

    if ((rval = lvm_config_override(self->libh, config)) == -1) {
        PyErr_SetObject(LibLVMError, liblvm_get_last_error(self));
        return NULL;
    }
    return Py_BuildValue("i", rval);
}
/* ----------------------------------------------------------------------
 * VG object initialization/deallocation
 */


static PyObject *
liblvm_lvm_vg_open(lvmobject *lvm, PyObject *args)
{
    const char *vgname;
    const char *mode = NULL;

    vgobject *self;

    if (!PyArg_ParseTuple(args, "s|s", &vgname, &mode)) {
        return NULL;
    }

    if (mode == NULL)
        mode = "r";

    if ((self = PyObject_New(vgobject, &LibLVMvgType)) == NULL)
        return NULL;

    if ((self->vg = lvm_vg_open(lvm->libh, vgname, mode, 0))== NULL) {
        Py_DECREF(self);
        PyErr_SetObject(LibLVMError, liblvm_get_last_error(lvm));
        return NULL;
    }

    return (PyObject *)self;
}

static PyObject *
liblvm_lvm_vg_create(lvmobject *lvm, PyObject *args)
{
    const char *vgname;

    vgobject *self;

    if (!PyArg_ParseTuple(args, "s", &vgname)) {
        return NULL;
    }

    if ((self = PyObject_New(vgobject, &LibLVMvgType)) == NULL)
        return NULL;

    if ((self->vg = lvm_vg_create(lvm->libh, vgname))== NULL) {
        Py_DECREF(self);
        PyErr_SetObject(LibLVMError, liblvm_get_last_error(lvm));
        return NULL;
    }
    self->lvm_obj = lvm;

    return (PyObject *)self;
}

static void
liblvm_vg_dealloc(vgobject *self)
{
    /* if already closed, don't reclose it */
    if (self->vg != NULL)
        lvm_vg_close(self->vg);
    PyObject_Del(self);
}

/* VG Methods */

static PyObject *
liblvm_lvm_vg_close(vgobject *self)
{
    /* if already closed, don't reclose it */
    if (self->vg != NULL)
        lvm_vg_close(self->vg);

    self->vg = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
liblvm_lvm_vg_get_name(vgobject *self)
{
    return Py_BuildValue("s", lvm_vg_get_name(self->vg));
}


static PyObject *
liblvm_lvm_vg_get_uuid(vgobject *self)
{
    return Py_BuildValue("s", lvm_vg_get_uuid(self->vg));
}

static PyObject *
liblvm_lvm_vg_remove(vgobject *self)
{
    int rval;

    if ((rval = lvm_vg_remove(self->vg)) == -1)
        goto error;

    if (lvm_vg_write(self->vg) == -1)
        goto error;

    return Py_BuildValue("i", rval);

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
    lvm_vg_close(self->vg);
    self->vg = NULL;
    return NULL;
}

static PyObject *
liblvm_lvm_vg_extend(vgobject *self, PyObject *args)
{
    const char *device;
    int rval;

    if (!PyArg_ParseTuple(args, "s", &device)) {
        return NULL;
    }

    if ((rval = lvm_vg_extend(self->vg, device)) == -1)
        goto error;

    if (lvm_vg_write(self->vg) == -1)
        goto error;

    return Py_BuildValue("i", rval);

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
    lvm_vg_close(self->vg);
    self->vg = NULL;
    return NULL;
}

static PyObject *
liblvm_lvm_vg_reduce(vgobject *self, PyObject *args)
{
    const char *device;
    int rval;

    if (!PyArg_ParseTuple(args, "s", &device)) {
        return NULL;
    }

    if ((rval = lvm_vg_reduce(self->vg, device)) == -1)
        goto error;

    if (lvm_vg_write(self->vg) == -1)
        goto error;

    return Py_BuildValue("i", rval);

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
    lvm_vg_close(self->vg);
    self->vg = NULL;
    return NULL;
}

static PyObject *
liblvm_lvm_vg_add_tag(vgobject *self, PyObject *args)
{
    const char *tag;
    int rval;

    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return NULL;
    }
    if ((rval = lvm_vg_add_tag(self->vg, tag)) == -1)
        goto error;

    if (lvm_vg_write(self->vg) == -1)
        goto error;

    return Py_BuildValue("i", rval);

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
    lvm_vg_close(self->vg);
    self->vg = NULL;
    return NULL;
}

static PyObject *
liblvm_lvm_vg_remove_tag(vgobject *self, PyObject *args)
{
    const char *tag;
    int rval;

    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return NULL;
    }

    if ((rval = lvm_vg_remove_tag(self->vg, tag)) == -1)
        goto error;

    if (lvm_vg_write(self->vg) == -1)
        goto error;

    return Py_BuildValue("i", rval);

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
    lvm_vg_close(self->vg);
    self->vg = NULL;
    return NULL;

}

static PyObject *
liblvm_lvm_vg_is_clustered(vgobject *self)
{
    PyObject *rval;

    rval = ( lvm_vg_is_clustered(self->vg) == 1) ? Py_True : Py_False;

    Py_INCREF(rval);
    return rval;
}

static PyObject *
liblvm_lvm_vg_is_exported(vgobject *self)
{
    PyObject *rval;

    rval = ( lvm_vg_is_exported(self->vg) == 1) ? Py_True : Py_False;

    Py_INCREF(rval);
    return rval;
}

static PyObject *
liblvm_lvm_vg_is_partial(vgobject *self)
{
    PyObject *rval;

    rval = ( lvm_vg_is_partial(self->vg) == 1) ? Py_True : Py_False;

    Py_INCREF(rval);
    return rval;
}

static PyObject *
liblvm_lvm_vg_get_seqno(vgobject *self)
{
    return Py_BuildValue("l", lvm_vg_get_seqno(self->vg));
}

static PyObject *
liblvm_lvm_vg_get_size(vgobject *self)
{
    return Py_BuildValue("l", lvm_vg_get_size(self->vg));
}

static PyObject *
liblvm_lvm_vg_get_free_size(vgobject *self)
{
    return Py_BuildValue("l", lvm_vg_get_free_size(self->vg));
}

static PyObject *
liblvm_lvm_vg_get_extent_size(vgobject *self)
{
    return Py_BuildValue("l", lvm_vg_get_extent_size(self->vg));
}

static PyObject *
liblvm_lvm_vg_get_extent_count(vgobject *self)
{
    return Py_BuildValue("l", lvm_vg_get_extent_count(self->vg));
}

static PyObject *
liblvm_lvm_vg_get_free_extent_count(vgobject *self)
{
    return Py_BuildValue("l", lvm_vg_get_free_extent_count(self->vg));
}

static PyObject *
liblvm_lvm_vg_get_pv_count(vgobject *self)
{
    return Py_BuildValue("l", lvm_vg_get_pv_count(self->vg));
}

static PyObject *
liblvm_lvm_vg_get_max_pv(vgobject *self)
{
    return Py_BuildValue("l", lvm_vg_get_max_pv(self->vg));
}

static PyObject *
liblvm_lvm_vg_get_max_lv(vgobject *self)
{
    return Py_BuildValue("l", lvm_vg_get_max_lv(self->vg));
}

static PyObject *
liblvm_lvm_vg_set_extent_size(vgobject *self, PyObject *args)
{
    uint32_t new_size;
    int rval;

    if (!PyArg_ParseTuple(args, "l", &new_size)) {
        return NULL;
    }

    if ((rval = lvm_vg_set_extent_size(self->vg, new_size)) == -1) {
        PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
        return NULL;
    }

    return Py_BuildValue("i", rval);
}

static PyObject *
liblvm_lvm_vg_list_lvs(vgobject *vg)
{
    struct dm_list *lvs;
    struct lvm_lv_list *lvl;
    PyObject * rv;
    lvobject * self;
    int i = 0;

    if ((lvs = lvm_vg_list_lvs(vg->vg))== NULL)
        goto error;

    if (dm_list_empty(lvs))
        goto error;

    rv = PyTuple_New(dm_list_size(lvs));
    if (rv == NULL)
        return NULL;

    dm_list_iterate_items(lvl, lvs) {
        /* Create and initialize the object */
        self = PyObject_New(lvobject, &LibLVMlvType);
        self->lv = lvl->lv;
        PyTuple_SET_ITEM(rv, i, (PyObject *) self);
        i++;
    }

    return rv;

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(vg->lvm_obj));
    return NULL;
}

static PyObject *
liblvm_lvm_vg_get_tags(vgobject *self)
{
    struct dm_list *tags;
    struct lvm_str_list *strl;
    PyObject * rv;
    int i = 0;

    if ((tags = lvm_vg_get_tags(self->vg))== NULL)
        goto error;

    if (dm_list_empty(tags))
        goto error;

    rv = PyTuple_New(dm_list_size(tags));
    if (rv == NULL)
        return NULL;

    dm_list_iterate_items(strl, tags) {
        PyTuple_SET_ITEM(rv, i, PyString_FromString(strl->str));
        i++;
    }

    return rv;

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
    return NULL;
}

static PyObject *
liblvm_lvm_vg_create_lv_linear(vgobject *vg, PyObject *args)
{
    const char *vgname;
    uint64_t size;

    lvobject *self;

    if (!PyArg_ParseTuple(args, "sl", &vgname, &size)) {
        return NULL;
    }

    if ((self = PyObject_New(lvobject, &LibLVMlvType)) == NULL)
        return NULL;

    if ((self->lv = lvm_vg_create_lv_linear(vg->vg, vgname, size))== NULL) {
        Py_DECREF(self);
        PyErr_SetObject(LibLVMError, liblvm_get_last_error(vg->lvm_obj));
        return NULL;
    }
    self->lvm_obj = vg->lvm_obj;

    return (PyObject *)self;
}

static void
liblvm_lv_dealloc(lvobject *self)
{
    PyObject_Del(self);
}

static PyObject *
liblvm_lvm_vg_list_pvs(vgobject *vg)
{
    struct dm_list *pvs;
    struct lvm_pv_list *pvl;
    PyObject * pytuple;
    pvobject * self;
    int i = 0;

    /* unlike other LVM api calls, if there are no results, we get NULL */
    pvs = lvm_vg_list_pvs(vg->vg);
    if (!pvs)
        return Py_BuildValue("()");

    pytuple = PyTuple_New(dm_list_size(pvs));
    if (!pytuple)
        return NULL;

    dm_list_iterate_items(pvl, pvs) {
        /* Create and initialize the object */
        self = PyObject_New(pvobject, &LibLVMpvType);
        if (!self) {
            Py_DECREF(pytuple);
            return NULL;
        }

        self->pv = pvl->pv;
        self->lvm_obj = vg->lvm_obj;
        PyTuple_SET_ITEM(pytuple, i, (PyObject *) self);
        i++;
    }

    return pytuple;
}

static void
liblvm_pv_dealloc(pvobject *self)
{
    PyObject_Del(self);
}

/* LV Methods */

static PyObject *
liblvm_lvm_lv_get_name(lvobject *self)
{
    return Py_BuildValue("s", lvm_lv_get_name(self->lv));
}

static PyObject *
liblvm_lvm_lv_get_uuid(lvobject *self)
{
    return Py_BuildValue("s", lvm_lv_get_uuid(self->lv));
}

static PyObject *
liblvm_lvm_lv_activate(lvobject *self)
{
    int rval;

    if ((rval = lvm_lv_activate(self->lv)) == -1) {
        PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
        return NULL;
    }

    return Py_BuildValue("i", rval);
}

static PyObject *
liblvm_lvm_lv_deactivate(lvobject *self)
{
    int rval;

    if ((rval = lvm_lv_deactivate(self->lv)) == -1) {
        PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
        return NULL;
    }

    return Py_BuildValue("i", rval);
}

static PyObject *
liblvm_lvm_vg_remove_lv(lvobject *self)
{
    int rval;

    if ((rval = lvm_vg_remove_lv(self->lv)) == -1) {
        PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
        return NULL;
    }

    return Py_BuildValue("i", rval);
}

static PyObject *
liblvm_lvm_lv_get_size(lvobject *self)
{
    int rval;

    if ((rval = lvm_lv_get_size(self->lv)) == -1) {
        PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
        return NULL;
    }

    return Py_BuildValue("i", rval);
}

static PyObject *
liblvm_lvm_lv_is_active(lvobject *self)
{
    PyObject *rval;

    rval = ( lvm_lv_is_active(self->lv) == 1) ? Py_True : Py_False;

    Py_INCREF(rval);
    return rval;
}

static PyObject *
liblvm_lvm_lv_is_suspended(lvobject *self)
{
    PyObject *rval;

    rval = ( lvm_lv_is_suspended(self->lv) == 1) ? Py_True : Py_False;

    Py_INCREF(rval);
    return rval;
}

static PyObject *
liblvm_lvm_lv_add_tag(lvobject *self, PyObject *args)
{
    const char *tag;
    int rval;

    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return NULL;
    }
    if ((rval = lvm_lv_add_tag(self->lv, tag)) == -1)
        goto error;

    return Py_BuildValue("i", rval);

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
    return NULL;
}

static PyObject *
liblvm_lvm_lv_remove_tag(lvobject *self, PyObject *args)
{
    const char *tag;
    int rval;

    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return NULL;
    }
    if ((rval = lvm_lv_remove_tag(self->lv, tag)) == -1)
        goto error;

    return Py_BuildValue("i", rval);

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
    return NULL;
}

static PyObject *
liblvm_lvm_lv_get_tags(lvobject *self)
{
    struct dm_list *tags;
    struct lvm_str_list *strl;
    PyObject * rv;
    int i = 0;

    if ((tags = lvm_lv_get_tags(self->lv))== NULL)
        goto error;

    if (dm_list_empty(tags))
        goto error;

    rv = PyTuple_New(dm_list_size(tags));
    if (rv == NULL)
        return NULL;

    dm_list_iterate_items(strl, tags) {
        PyTuple_SET_ITEM(rv, i, PyString_FromString(strl->str));
        i++;
    }

    return rv;

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
    return NULL;
}

static PyObject *
liblvm_lvm_lv_resize(lvobject *self, PyObject *args)
{
    uint64_t new_size;
    int rval;

    if (!PyArg_ParseTuple(args, "l", &new_size)) {
        return NULL;
    }
    if ((rval = lvm_lv_resize(self->lv, new_size)) == -1)
        goto error;

    return Py_BuildValue("i", rval);

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
    return NULL;
}
/* PV Methods */

static PyObject *
liblvm_lvm_pv_get_name(pvobject *self)
{
    return Py_BuildValue("s", lvm_pv_get_name(self->pv));
}

static PyObject *
liblvm_lvm_pv_get_uuid(pvobject *self)
{
    return Py_BuildValue("s", lvm_pv_get_uuid(self->pv));
}

static PyObject *
liblvm_lvm_pv_get_mda_count(pvobject *self)
{
    return Py_BuildValue("l", lvm_pv_get_mda_count(self->pv));
}

static PyObject *
liblvm_lvm_pv_get_dev_size(pvobject *self)
{
    return Py_BuildValue("l", lvm_pv_get_dev_size(self->pv));
}

static PyObject *
liblvm_lvm_pv_get_size(pvobject *self)
{
    return Py_BuildValue("l", lvm_pv_get_size(self->pv));
}

static PyObject *
liblvm_lvm_pv_get_free(pvobject *self)
{
    return Py_BuildValue("l", lvm_pv_get_free(self->pv));
}

static PyObject *
liblvm_lvm_pv_resize(pvobject *self, PyObject *args)
{
    uint64_t new_size;
    int rval;

    if (!PyArg_ParseTuple(args, "l", &new_size)) {
        return NULL;
    }
    if ((rval = lvm_pv_resize(self->pv, new_size)) == -1)
        goto error;

    return Py_BuildValue("i", rval);

error:
    PyErr_SetObject(LibLVMError, liblvm_get_last_error(self->lvm_obj));
    return NULL;
}
/* ----------------------------------------------------------------------
 * Method tables and other bureaucracy
 */

static PyMethodDef Liblvm_methods[] = {
    /* LVM methods */
    { "getVersion",          (PyCFunction)liblvm_library_get_version, METH_NOARGS },
    { "vgOpen",                (PyCFunction)liblvm_lvm_vg_open, METH_VARARGS },
    { "vgCreate",                (PyCFunction)liblvm_lvm_vg_create, METH_VARARGS },
    { "close",                  (PyCFunction)liblvm_close, METH_NOARGS },
    { "configReload",          (PyCFunction)liblvm_lvm_config_reload, METH_NOARGS },
    { "configOverride",        (PyCFunction)liblvm_lvm_config_override, METH_VARARGS },
    { "scan",                   (PyCFunction)liblvm_lvm_scan, METH_NOARGS },
    { "listVgNames",          (PyCFunction)liblvm_lvm_list_vg_names, METH_NOARGS },
    { "listVgUuids",          (PyCFunction)liblvm_lvm_list_vg_uuids, METH_NOARGS },
    { "vgNameFromPvid",        (PyCFunction)liblvm_lvm_vgname_from_pvid, METH_VARARGS },
    { "vgNameFromDevice",        (PyCFunction)liblvm_lvm_vgname_from_device, METH_VARARGS },

    { NULL,             NULL}           /* sentinel */
};

static PyMethodDef liblvm_vg_methods[] = {
    /* vg methods */
    { "getName",          (PyCFunction)liblvm_lvm_vg_get_name, METH_NOARGS },
    { "getUuid",          (PyCFunction)liblvm_lvm_vg_get_uuid, METH_NOARGS },
    { "close",          (PyCFunction)liblvm_lvm_vg_close, METH_NOARGS },
    { "remove",          (PyCFunction)liblvm_lvm_vg_remove, METH_NOARGS },
    { "extend",          (PyCFunction)liblvm_lvm_vg_extend, METH_VARARGS },
    { "reduce",          (PyCFunction)liblvm_lvm_vg_reduce, METH_VARARGS },
    { "addTag",          (PyCFunction)liblvm_lvm_vg_add_tag, METH_VARARGS },
    { "removeTag",          (PyCFunction)liblvm_lvm_vg_remove_tag, METH_VARARGS },
    { "setExtentSize",          (PyCFunction)liblvm_lvm_vg_set_extent_size, METH_VARARGS },
    { "isClustered",          (PyCFunction)liblvm_lvm_vg_is_clustered, METH_NOARGS },
    { "isExported",          (PyCFunction)liblvm_lvm_vg_is_exported, METH_NOARGS },
    { "isPartial",          (PyCFunction)liblvm_lvm_vg_is_partial, METH_NOARGS },
    { "getSeqno",          (PyCFunction)liblvm_lvm_vg_get_seqno, METH_NOARGS },
    { "getSize",          (PyCFunction)liblvm_lvm_vg_get_size, METH_NOARGS },
    { "getFreeSize",          (PyCFunction)liblvm_lvm_vg_get_free_size, METH_NOARGS },
    { "getExtentSize",          (PyCFunction)liblvm_lvm_vg_get_extent_size, METH_NOARGS },
    { "getExtentCount",          (PyCFunction)liblvm_lvm_vg_get_extent_count, METH_NOARGS },
    { "getFreeExtentCount",  (PyCFunction)liblvm_lvm_vg_get_free_extent_count, METH_NOARGS },
    { "getPvCount",          (PyCFunction)liblvm_lvm_vg_get_pv_count, METH_NOARGS },
    { "getMaxPv",          (PyCFunction)liblvm_lvm_vg_get_max_pv, METH_NOARGS },
    { "getMaxLv",          (PyCFunction)liblvm_lvm_vg_get_max_lv, METH_NOARGS },
    { "listLVs",          (PyCFunction)liblvm_lvm_vg_list_lvs, METH_NOARGS },
    { "listPVs",          (PyCFunction)liblvm_lvm_vg_list_pvs, METH_NOARGS },
    { "getTags",          (PyCFunction)liblvm_lvm_vg_get_tags, METH_NOARGS },
    { "createLvLinear",          (PyCFunction)liblvm_lvm_vg_create_lv_linear, METH_VARARGS },
    { NULL,             NULL}   /* sentinel */
};

static PyMethodDef liblvm_lv_methods[] = {
    /* lv methods */
    { "getName",          (PyCFunction)liblvm_lvm_lv_get_name, METH_NOARGS },
    { "getUuid",          (PyCFunction)liblvm_lvm_lv_get_uuid, METH_NOARGS },
    { "activate",          (PyCFunction)liblvm_lvm_lv_activate, METH_NOARGS },
    { "deactivate",          (PyCFunction)liblvm_lvm_lv_deactivate, METH_NOARGS },
    { "remove",          (PyCFunction)liblvm_lvm_vg_remove_lv, METH_NOARGS },
    { "getSize",          (PyCFunction)liblvm_lvm_lv_get_size, METH_NOARGS },
    { "isActive",          (PyCFunction)liblvm_lvm_lv_is_active, METH_NOARGS },
    { "isSuspended",          (PyCFunction)liblvm_lvm_lv_is_suspended, METH_NOARGS },
    { "addTag",          (PyCFunction)liblvm_lvm_lv_add_tag, METH_VARARGS },
    { "removeTag",          (PyCFunction)liblvm_lvm_lv_remove_tag, METH_VARARGS },
    { "getTags",          (PyCFunction)liblvm_lvm_lv_get_tags, METH_NOARGS },
    { "resize",          (PyCFunction)liblvm_lvm_lv_resize, METH_VARARGS },
    { NULL,             NULL}   /* sentinel */
};

static PyMethodDef liblvm_pv_methods[] = {
    /* pv methods */
    { "getName",          (PyCFunction)liblvm_lvm_pv_get_name, METH_NOARGS },
    { "getUuid",          (PyCFunction)liblvm_lvm_pv_get_uuid, METH_NOARGS },
    { "getMdaCount",          (PyCFunction)liblvm_lvm_pv_get_mda_count, METH_NOARGS },
    { "getSize",          (PyCFunction)liblvm_lvm_pv_get_size, METH_NOARGS },
    { "getDevSize",          (PyCFunction)liblvm_lvm_pv_get_dev_size, METH_NOARGS },
    { "getFree",          (PyCFunction)liblvm_lvm_pv_get_free, METH_NOARGS },
    { "resize",          (PyCFunction)liblvm_lvm_pv_resize, METH_VARARGS },
    { NULL,             NULL}   /* sentinel */
};

static PyTypeObject LiblvmType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name = "liblvm.Liblvm",
    .tp_basicsize = sizeof(lvmobject),
    .tp_dealloc = (destructor)liblvm_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = "Liblvm objects",
    .tp_methods = Liblvm_methods,
    .tp_init = (initproc)liblvm_init,
};

static PyTypeObject LibLVMvgType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name = "liblvm.Liblvm_vg",
    .tp_basicsize = sizeof(vgobject),
    .tp_methods = liblvm_vg_methods,
    .tp_dealloc = (destructor)liblvm_vg_dealloc,
};

static PyTypeObject LibLVMlvType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name = "liblvm.Liblvm_lv",
    .tp_basicsize = sizeof(lvobject),
    .tp_methods = liblvm_lv_methods,
    .tp_dealloc = (destructor)liblvm_lv_dealloc,
};

static PyTypeObject LibLVMpvType = {
    PyObject_HEAD_INIT(&PyType_Type)
    .tp_name = "liblvm.Liblvm_pv",
    .tp_basicsize = sizeof(pvobject),
    .tp_methods = liblvm_pv_methods,
    .tp_dealloc = (destructor)liblvm_pv_dealloc,
};

#ifndef PyMODINIT_FUNC    /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initlvm(void)
{
    PyObject *m;

    LiblvmType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&LiblvmType) < 0)
        return;

    m = Py_InitModule3("lvm", Liblvm_methods, "Liblvm module");
    if (m == NULL)
        return;

    Py_INCREF(&LiblvmType);
    PyModule_AddObject(m, "Liblvm", (PyObject *)&LiblvmType);

    LibLVMError = PyErr_NewException("Liblvm.LibLVMError",
                                      NULL, NULL);
    if (LibLVMError) {
        /* Each call to PyModule_AddObject decrefs it; compensate: */
        Py_INCREF(LibLVMError);
        Py_INCREF(LibLVMError);
        PyModule_AddObject(m, "error", LibLVMError);
        PyModule_AddObject(m, "LibLVMError", LibLVMError);
    }

}

#include "scripting_engine.h"
#include "structmember.h"

typedef struct {
    PyObject_HEAD
    int skeletonRevision;
    bool unsavedChanges;

} SkeletonObject;

static void dealloc(SkeletonObject *obj) {
    Py_XDECREF(obj);
    obj->ob_type->tp_free((PyObject *) obj);
}

static int init(SkeletonObject *self, PyObject *args, PyObject *kwds) {
    return 0;
}

static PyMemberDef members[] = {
    { "skeletonRevision", T_INT, offsetof(SkeletonObject, skeletonRevision), 0, "the skeleton revision"},
    { "unsavedChanges", T_BOOL, offsetof(SkeletonObject, unsavedChanges), 0, "are there any unsaved changes" },
    {NULL} /* sentinel(is always the last entry) */
};

static PyMethodDef methods[] = {
    {NULL} /* sentinel(is always the last entry) */
};

static PyObject *SkeletonObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    SkeletonObject *self = (SkeletonObject *) type->tp_alloc(type, 0);
    if(self == NULL)
        return NULL;

    return (PyObject *)self;
}

/** this is needed for defining new datatypes(custom classes).
    more information about this construct can be found in the python include folder object.h
    and the Python C API reference manual. */
static PyTypeObject SkeletonObjectType = {
    PyObject_HEAD_INIT(NULL)
    0,                          /*ob_size*/
    "knossos.SkeletonObject",   /*tp_name*/
    sizeof(SkeletonObject),     /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor) dealloc,      /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare*/
    0,                          /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash */
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,         /*tp_flags*/
    "Skeleton scripting object",/* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    methods,                    /* tp_methods */
    members,                    /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    (initproc) init,            /* tp_init */
    0,                          /* tp_alloc */
    SkeletonObject_new          /* tp_new */

};


/** the PyMODINIT_FUNC is the entry point for any intialization.
    the name of this function has to init + "name of the module" which is determined by convention
*/
PyMODINIT_FUNC
initknossos(void) {

    SkeletonObjectType.tp_new = PyType_GenericNew;
    if(PyType_Ready(&SkeletonObjectType) < 0)
        return;

    module = Py_InitModule3("knossos", methods, "knossos scripting module");
    PyObject *version = PyString_FromString("version 1e-5");
    PyObject_SetAttrString(module, "version", version);

    Py_INCREF(&SkeletonObjectType);
    PyModule_AddObject(module, "skeleton", (PyObject *) &SkeletonObjectType);

}

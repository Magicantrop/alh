/*
 * This source file is part of the Atlantis Little Helper program.
 * Copyright (C) 2001 Maxim Shariy.
 *
 * Atlantis Little Helper is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atlantis Little Helper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atlantis Little Helper; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "stdafx.h"
#include "stdhdr.h"

#include "cstr.h"
#include "collection.h"
#include "cfgfile.h"
#include "files.h"
#include "atlaparser.h"
#include "consts.h"
#include "consts_ah.h"
#include "objs.h"
#include "hash.h"

#include "ahapp.h"
#include "extend.h"

// Macros for error checking and null pointer validation
#define CHECK_NULL_PTR(ptr, err, msg) \
if (!ptr)                             \
{                                     \
    ShowError(msg);                   \
    result = err;                     \
    goto quit;                        \
}

// Names for Python modules and functions
#define SZ_UNIT_FILTER_MODULE              "unit_filter_module"
#define SZ_UNIT_FILTER_FUNC                "unit_filter_function"
#define SZ_ALH_PROGRAM_NAME                "alh_extension"
#define SZ_ALH_UNIT_FILTER_MODULE          "alh_unit_filter"
#define SZ_ALH_UNIT_FILTER_FN_GET_PROPERTY "get_property"

//-------------------------------------------------------------------------

/**
 * Default constructor - should not be used
 * Throws error if called
 */
CPythonEmbedder::CPythonEmbedder()
{
    m_pAtlantis = NULL;
    ShowError("Wrong CPythonEmbedder constructor called!");
}

//-------------------------------------------------------------------------

/**
 * Constructor with Atlantis parser reference
 * @param pAtlantis Pointer to Atlantis parser (provides game data)
 */
CPythonEmbedder::CPythonEmbedder(CAtlaParser * pAtlantis)
{
    m_pAtlantis       = pAtlantis;
    m_bInitUnitFilter = FALSE;
    m_bInitUnitFilter = FALSE;  // Note: duplicate line, but harmless
    m_bInitGeneric    = FALSE;

    m_pCode   = NULL;
    m_pModule = NULL;
    m_pDict   = NULL;
    m_pFunc   = NULL;
}

//-------------------------------------------------------------------------

/**
 * Destructor - cleans up Python resources
 */
CPythonEmbedder::~CPythonEmbedder()
{
    if (m_bInitUnitFilter)
        DoneUnitFilter();
    if (m_bInitGeneric)
        DoneGeneric();
}

//-------------------------------------------------------------------------

/**
 * Displays error message using application's error display
 * @param msg Error message
 * @param msglen Message length (0 for auto)
 */
void CPythonEmbedder::ShowError(const char * msg, int msglen)
{
    if (msglen <= 0)
        msglen = strlen(msg);

    gpApp->ShowError(msg, msglen, TRUE);
}

//=========================================================================
// Python-specific functions (only compiled when HAVE_PYTHON is defined)
//=========================================================================

#ifdef HAVE_PYTHON

#include "Python.h"

//-------------------------------------------------------------------------

/**
 * Global unit pointer used by the Python extension function
 * This is a hack to allow the Python callback to access the current unit
 */
static CUnit * gpUnit = NULL;

/**
 * Python-callable function to get unit properties
 * Registered as 'get_property' in the 'alh_unit_filter' module
 * 
 * @param self Python self pointer (unused)
 * @param args Python tuple containing the property name
 * @return Python object with property value (int for numbers, string for text)
 */
extern "C" PyObject * unitfltr_getproperty(PyObject *self, PyObject* args)
{
    char          * propname;
    EValueType      type;
    const void    * value;

    // Parse arguments - expect a single string
    if (!PyArg_ParseTuple(args, "s", &propname) || !gpUnit)
        return Py_BuildValue("s", NULL);

    // Try to get the property from the current unit
    if (!gpUnit->GetProperty(propname, type, value, eNormal))
    {
        // Property doesn't exist - create default value based on property type
        CStrInt * pSI, SI(propname, 0);
        int idx;

        if (gpApp->m_pAtlantis->m_UnitPropertyTypes.Search(&SI, idx))
        {
            pSI = (CStrInt*)gpApp->m_pAtlantis->m_UnitPropertyTypes.At(idx);
            type = (EValueType)pSI->m_value;
            if (eLong == type)
                value = reinterpret_cast<void*>(0);
            else
                value = "";
        }
        else
            return Py_BuildValue("s", NULL);
    }

    // Return value as appropriate Python type
    if (eLong == type)
        return Py_BuildValue("i", static_cast<long>(reinterpret_cast<intptr_t>(value)));
    else
    {
        // Python is case-sensitive, so lowercase all string values for consistency
        CStr S;
        S.SetStr((const char *)value);
        S.ToLower();
        return Py_BuildValue("s", S.GetData());
    }
}

/**
 * Method table for the 'alh_unit_filter' Python module
 * Defines the functions available to Python scripts
 */
PyMethodDef unitfltr_methods[] =
{
    {SZ_ALH_UNIT_FILTER_FN_GET_PROPERTY, unitfltr_getproperty, METH_VARARGS, "Get unit property."},
    {NULL, NULL}   // Sentinel - marks end of array
};

/**
 * Initialization function for the 'alh_unit_filter' Python module
 * Called when the module is first imported
 */
void initunitfltr(void)
{
    PyImport_AddModule(SZ_ALH_UNIT_FILTER_MODULE);
    Py_InitModule(SZ_ALH_UNIT_FILTER_MODULE, unitfltr_methods);
}

//-------------------------------------------------------------------------

/**
 * Initializes generic Python interpreter
 * @return Error code (E_OK on success)
 */
eEErr CPythonEmbedder::InitGeneric()
{
    eEErr rc = E_OK;

    if (!m_pAtlantis)
        return E_NULL_POINTERS;

    if (m_bInitGeneric)
        return E_ALREADY_INIT;

    Py_SetProgramName((char*)SZ_ALH_PROGRAM_NAME);
    Py_Initialize();
    m_bInitGeneric = TRUE;

    return rc;
}

//-------------------------------------------------------------------------

/**
 * Cleans up generic Python resources
 */
void CPythonEmbedder::DoneGeneric()
{
    Py_Finalize();
    m_bInitGeneric = FALSE;
}

//-------------------------------------------------------------------------

/**
 * Checks for and prints Python errors
 * Should be called after any Python operation that might fail
 */
void CPythonEmbedder::CheckForPythonError()
{
    if (PyErr_Occurred())
        PyErr_Print();
}

//-------------------------------------------------------------------------

/**
 * Initializes a unit filter
 * Compiles user filter code into a Python function
 * 
 * @param userfilter Raw filter expression from user
 * @param sPythonFilter Output string with generated Python code
 * @return Error code (E_OK on success)
 */
eEErr CPythonEmbedder::InitUnitFilter(const char * userfilter, CStr & sPythonFilter)
{
    eEErr result = E_OK;
    CStr sToken;
    const char * p = userfilter;
    char ch;
    int idx;
    CStr sCommand;

    sPythonFilter.Empty();
    
    // Initialize Python interpreter if not already done
    result = InitGeneric();
    if (E_OK != result)
        return result;

    if (m_bInitUnitFilter)
        return E_ALREADY_INIT;
    m_bInitUnitFilter = TRUE;

    // Build the Python filter function
    GetCommonCode(sCommand);  // Load common preamble code

    sCommand << "\n"
             << "import " << SZ_ALH_UNIT_FILTER_MODULE << "\n"
             << "def " << SZ_UNIT_FILTER_FUNC << "():\n"
             << "    res = ";

    // Parse the user filter expression and replace property names with function calls
    while (p && *p)
    {
        p = sToken.GetToken(p, "+-*/<>=!()., \t\r\n", ch, TRIM_ALL, FALSE);

        // Python is case-sensitive, so lowercase quoted strings
        if (!sToken.IsEmpty() && '\"' == sToken.GetData()[0] && '\"' == sToken.GetData()[sToken.GetLength() - 1])
            sToken.ToLower();

        // If token is a known unit property, replace with get_property() call
        if (gpApp->m_pAtlantis->m_UnitPropertyNames.Search((void*)sToken.GetData(), idx))
            sCommand << SZ_ALH_UNIT_FILTER_MODULE << "." << SZ_ALH_UNIT_FILTER_FN_GET_PROPERTY << "(\"" << sToken << "\")";
        else
            sCommand << sToken;  // Keep as-is (number, operator, etc.)

        if ('\n' == ch)
            sCommand << ' ';
        else if (ch && '\r' != ch)
            sCommand << ch;
    }
    
    sCommand << "\n"
             << "    return res\n";
    sPythonFilter = sCommand;

    // Initialize our custom module
    initunitfltr();

    // Compile and import the filter module
    m_pCode = Py_CompileString((char*)sCommand.GetData(), SZ_UNIT_FILTER_MODULE, Py_file_input);
    CHECK_NULL_PTR(m_pCode, E_PYTHON, "Py_CompileString()")
    
    m_pModule = PyImport_ExecCodeModule((char*)SZ_UNIT_FILTER_MODULE, m_pCode);
    CHECK_NULL_PTR(m_pModule, E_PYTHON, "PyImport_ExecCodeModule()")
    
    m_pDict = PyModule_GetDict(m_pModule);
    CHECK_NULL_PTR(m_pDict, E_PYTHON, "PyModule_GetDict()")
    
    m_pFunc = PyDict_GetItemString(m_pDict, SZ_UNIT_FILTER_FUNC);
    CHECK_NULL_PTR(m_pFunc, E_PYTHON, "PyDict_GetItemString()")

quit:
    if (E_OK != result)
        CheckForPythonError();

    return result;
}

//-------------------------------------------------------------------------

/**
 * Runs the unit filter on a specific unit
 * @param pUnit Unit to test
 * @param success Output filter result (TRUE if unit passes)
 * @return Error code (E_OK on success)
 */
eEErr CPythonEmbedder::RunUnitFilter(CUnit * pUnit, BOOL & success)
{
    eEErr result = E_PYTHON;

    success = FALSE;

    CHECK_NULL_PTR(m_pFunc, E_NULL_POINTERS, "m_pFunc not initialised")

    if (PyCallable_Check(m_pFunc))
    {
        PyObject * pValue;

        // Set global unit pointer for the callback function
        gpUnit = pUnit;
        
        // Call the Python filter function
        pValue = PyObject_CallObject(m_pFunc, NULL);
        if (pValue)
        {
            success = PyInt_AsLong(pValue);
            Py_DECREF(pValue);
            result = E_OK;
        }
        gpUnit = NULL;
    }

quit:
    if (E_OK != result)
        CheckForPythonError();

    return result;
}

//-------------------------------------------------------------------------

/**
 * Cleans up unit filter resources
 */
void CPythonEmbedder::DoneUnitFilter()
{
    if (m_pCode)    Py_DECREF(m_pCode);
    if (m_pModule)  Py_DECREF(m_pModule);
    // m_pDict and m_pFunc are borrowed references - don't DECREF them
    
    m_pCode   = NULL;
    m_pModule = NULL;
    m_pDict   = NULL;
    m_pFunc   = NULL;
    m_bInitUnitFilter = FALSE;
}

//-------------------------------------------------------------------------

/**
 * Gets common Python code from file or creates default
 * This code is prepended to every filter to provide utilities
 * 
 * @param code Output string with common code
 */
void CPythonEmbedder::GetCommonCode(CStr & code)
{
    CFileReader F;
    CFileWriter W;
    CStr S;
    int x;

    code.Empty();
    
    // Try to load existing common code file
    if (F.Open(SZ_COMMON_PY_FILE))
    {
        while (F.GetNextLine(S))
            code << S;
        F.Close();
    }
    else
    {
        // Create default common code file
        if (W.Open(SZ_COMMON_PY_FILE))
        {
            code << "import string";
            W.WriteBuf(code.GetData(), code.GetLength());
            W.Close();
        }
    }

    // Remove carriage returns - Python parser chokes on \r\n on Windows
    x = code.FindSubStr("\r");
    while (x >= 0)
    {
        code.DelCh(x);
        x = code.FindSubStr("\r");
    }
}

//-------------------------------------------------------------------------

#endif // HAVE_PYTHON
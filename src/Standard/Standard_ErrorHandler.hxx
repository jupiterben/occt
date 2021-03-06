// Created on: 1992-09-28
// Created by: Ramin BARRETO
// Copyright (c) 1992-1999 Matra Datavision
// Copyright (c) 1999-2014 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#ifndef _Standard_ErrorHandler_HeaderFile
#define _Standard_ErrorHandler_HeaderFile

#include <Standard.hxx>
#include <Standard_Handle.hxx>

#include <Standard_PErrorHandler.hxx>
#include <Standard_JmpBuf.hxx>
#include <Standard_HandlerStatus.hxx>
#include <Standard_ThreadId.hxx>
#include <Standard_Type.hxx>

//! @file
//! Support of handling of C signals as C++-style exceptions, and implementation
//! of C++ exception handling on platforms that do not implement these natively.
//!
//! The implementation is based on C long jumps.
//!
//! If macro NO_CXX_EXCEPTION is defined, "try" and "catch" are defined as
//! macros that use jumps to implement exception handling. 
//! See Standard_Failure::Reraise() for exception throwing code.
//! Note that this option is obsolete and useless for modern platforms.
//! 
//! If macro OCC_CONVERT_SIGNALS is defined, this enables macro OCC_CATCH_SIGNALS
//! that can be used in the code (often inside try {} blocks) to convert C-style 
//! signals to standard C++ exceptions. This works only when OSD::SetSignal()
//! is called to set appropriate signal handler. In the case of signal (like 
//! access violation, division by zero, etc.) it will jump to the nearest 
//! OCC_CATCH_SIGNALS in the call stack, which will then throw a C++ exception.
//! This method is useful for Unix and Linux systems where C++ exceptions
//! cannot be thrown from C signal handler.
//! 
//! On Windows with MSVC compiler, exceptions can be thrown directly from 
//! signal handler, this OCC_CONVERT_SIGNALS is not needed. Note however that
//! this requires that compiler option /EHa is used.

#ifdef NO_CXX_EXCEPTION

  // No CXX Exceeptions, only jumps in all the cases.
  //
  // Note: In the current version setjmp is used. The alternative solution is to
  // use sigsetjmp that stores the signal mask (to be checked)
  // In the original implementation sigsetjmp is tried to use for SUN and IRIX
  // in the following way:
  //    #ifdef SOLARIS
  //      #define DoesNotAbort(aHandler) !sigsetjmp(aHandler.Label(),1)
  //    #endif

  #define try               Standard_ErrorHandler _Function; \
                            if(!setjmp(_Function.Label()))
  #define catch(Error)        else if(_Function.Catches(STANDARD_TYPE(Error)))
  #define OCC_CATCH_SIGNALS 

  // Suppress GCC warning "variable ...  might be clobbered by 'longjmp' or 'vfork'"
  #if defined(__GNUC__) && ! defined(__INTEL_COMPILER) && ! defined(__clang__)
  #pragma GCC diagnostic ignored "-Wclobbered"
  #endif

#elif defined(OCC_CONVERT_SIGNALS)

  // Exceptions are raied as usual, signal cause jumps in the nearest 
  // OCC_CATCH_SIGNALS and then thrown as exceptions.
  #define OCC_CATCH_SIGNALS   Standard_ErrorHandler _aHandler; \
                              if(setjmp(_aHandler.Label())) { \
				_aHandler.Catches(STANDARD_TYPE(Standard_Failure)); \
				_aHandler.Error()->Reraise(); \
			      }

  // Suppress GCC warning "variable ...  might be clobbered by 'longjmp' or 'vfork'"
  #if defined(__GNUC__) && ! defined(__INTEL_COMPILER) && ! defined(__clang__)
  #pragma GCC diagnostic ignored "-Wclobbered"
  #endif

#else

  // Normal Exceptions (for example WNT with MSVC and option /GHa)
  #define OCC_CATCH_SIGNALS

#endif

class Standard_Failure;
class Standard_ErrorHandlerCallback;

//! Class implementing mechanics of conversion of signals to exceptions.
//!
//! Each instance of it stores data for jump placement, thread id,
//! and callbacks to be called during jump (for proper resource release).
//!
//! The active handlers are stored in the global stack, which is used
//! to find appropriate handler when signal is raised.

class Standard_ErrorHandler 
{
public:

  DEFINE_STANDARD_ALLOC

  
  //! Create a ErrorHandler (to be used with try{}catch(){}).
  //! It uses the "setjmp" and "longjmp" routines.
  Standard_EXPORT Standard_ErrorHandler();
  
  //! Unlinks and checks if there is a raised exception.
  Standard_EXPORT void Destroy();

  //! Destructor
  ~Standard_ErrorHandler()
  {
    Destroy();
  }
  
  //! Removes handler from the handlers list
  Standard_EXPORT void Unlink();
  
  //! Returns "True" if the caught exception has the same type
  //! or inherits from "aType"
  Standard_EXPORT Standard_Boolean Catches (const Handle(Standard_Type)& aType);
  
  //! Returns label for jump
  Standard_JmpBuf& Label() { return myLabel; }
  
  //! Returns the current Error.
  Standard_EXPORT Handle(Standard_Failure) Error() const;
  
  //! Returns the caught exception.
  Standard_EXPORT static Handle(Standard_Failure) LastCaughtError();
  
  //! Test if the code is currently running in a try block
  Standard_EXPORT static Standard_Boolean IsInTryBlock();

private:
  
  //! A exception is raised but it is not yet caught.
  //! So Abort the current function and transmit the exception
  //! to "calling routines".
  //! Warning: If no catch is prepared for this exception, it displays the
  //! exception name and calls "exit(1)".
  Standard_EXPORT static void Abort (const Handle(Standard_Failure)& theError);
  
  //! Set the Error which will be transmitted to "calling routines".
  Standard_EXPORT static void Error (const Handle(Standard_Failure)& aError);
  
  //! Returns the current handler (closest in the stack in the current execution thread)
  Standard_EXPORT static Standard_PErrorHandler FindHandler (const Standard_HandlerStatus theStatus, const Standard_Boolean theUnlink);

private:

  Standard_PErrorHandler myPrevious;
  Handle(Standard_Failure) myCaughtError;
  Standard_JmpBuf myLabel;
  Standard_HandlerStatus myStatus;
  Standard_ThreadId myThread;
  Standard_Address myCallbackPtr;

  friend class Standard_Failure;
  friend class Standard_ErrorHandlerCallback;
};

#endif // _Standard_ErrorHandler_HeaderFile

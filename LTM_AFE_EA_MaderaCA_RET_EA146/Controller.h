// Controller.h : main header file for the CONTROLLER application
//

#if !defined(AFX_CONTROLLER_H__DC61B419_DE47_11D3_B165_0050DA63CF12__INCLUDED_)
#define AFX_CONTROLLER_H__DC61B419_DE47_11D3_B165_0050DA63CF12__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CControllerApp:
// See Controller.cpp for the implementation of this class
//

class CControllerApp : public CWinApp
{
public:
	CControllerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CControllerApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CControllerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONTROLLER_H__DC61B419_DE47_11D3_B165_0050DA63CF12__INCLUDED_)

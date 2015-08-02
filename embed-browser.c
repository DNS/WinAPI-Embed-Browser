/*

embed-browser.c

Embed Internet Explorer browser control to WinAPI program

UI_ShowContextMenu() -> return S_FALSE to enable context menu (right click pop up menu), S_OK to disable


*/


#pragma comment(linker,"\"/manifestdependency:type='win32' \
	name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
	processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "shlwapi.lib")


#include <windows.h>
#include <commctrl.h>
#include <exdisp.h>	
#include <mshtml.h>	
#include <mshtmhst.h>
#include <crtdbg.h>
#include <tchar.h>
#include <oleidl.h>


HFONT hfont_custom;
HWND back_btn, forward_btn, stop_btn;
HWND browser_hwnd;

// Handle of our Main Window.
HWND MainWindow;

// Our font used for buttons
HFONT FontHandle;

// The class name of our Main Window. It can be anything of your choosing.
static const TCHAR ClassName[] = "Browser Example";

// The class name of our Window to host the browser. It can be anything of your choosing.
static const TCHAR BrowserClassName[] = "Browser Object";

// This is used by DisplayHTMLStr(). It can be global because we never change it.
static const SAFEARRAYBOUND ArrayBound = {1, 0};







// Our IOleInPlaceFrame functions that the browser may call
HRESULT STDMETHODCALLTYPE Frame_QueryInterface(IOleInPlaceFrame *This, REFIID riid, LPVOID *ppvObj);
HRESULT STDMETHODCALLTYPE Frame_AddRef(IOleInPlaceFrame *This);
HRESULT STDMETHODCALLTYPE Frame_Release(IOleInPlaceFrame *This);
HRESULT STDMETHODCALLTYPE Frame_GetWindow(IOleInPlaceFrame *This, HWND *lphwnd);
HRESULT STDMETHODCALLTYPE Frame_ContextSensitiveHelp(IOleInPlaceFrame *This, BOOL fEnterMode);
HRESULT STDMETHODCALLTYPE Frame_GetBorder(IOleInPlaceFrame *This, LPRECT lprectBorder);
HRESULT STDMETHODCALLTYPE Frame_RequestBorderSpace(IOleInPlaceFrame *This, LPCBORDERWIDTHS pborderwidths);
HRESULT STDMETHODCALLTYPE Frame_SetBorderSpace(IOleInPlaceFrame *This, LPCBORDERWIDTHS pborderwidths);
HRESULT STDMETHODCALLTYPE Frame_SetActiveObject(IOleInPlaceFrame *This, IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName);
HRESULT STDMETHODCALLTYPE Frame_InsertMenus(IOleInPlaceFrame *This, HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
HRESULT STDMETHODCALLTYPE Frame_SetMenu(IOleInPlaceFrame *This, HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
HRESULT STDMETHODCALLTYPE Frame_RemoveMenus(IOleInPlaceFrame *This, HMENU hmenuShared);
HRESULT STDMETHODCALLTYPE Frame_SetStatusText(IOleInPlaceFrame *This, LPCOLESTR pszStatusText);
HRESULT STDMETHODCALLTYPE Frame_EnableModeless(IOleInPlaceFrame *This, BOOL fEnable);
HRESULT STDMETHODCALLTYPE Frame_TranslateAccelerator(IOleInPlaceFrame *This, LPMSG lpmsg, WORD wID);

// Our IOleInPlaceFrame VTable. This is the array of pointers to the above functions in our C
// program that the browser may call in order to interact with our frame window that contains
// the browser object. We must define a particular set of functions that comprise the
// IOleInPlaceFrame set of functions (see above), and then stuff pointers to those functions
// in their respective 'slots' in this table. We want the browser to use this VTable with our
// IOleInPlaceFrame structure.
IOleInPlaceFrameVtbl MyIOleInPlaceFrameTable = {Frame_QueryInterface,
Frame_AddRef,
Frame_Release,
Frame_GetWindow,
Frame_ContextSensitiveHelp,
Frame_GetBorder,
Frame_RequestBorderSpace,
Frame_SetBorderSpace,
Frame_SetActiveObject,
Frame_InsertMenus,
Frame_SetMenu,
Frame_RemoveMenus,
Frame_SetStatusText,
Frame_EnableModeless,
Frame_TranslateAccelerator};

// We need to return an IOleInPlaceFrame struct to the browser object. And one of our IOleInPlaceFrame
// functions (Frame_GetWindow) is going to need to access our window handle. So let's create our own
// struct that starts off with an IOleInPlaceFrame struct (and that's important -- the IOleInPlaceFrame
// struct *must* be first), and then has an extra data field where we can store our own window's HWND.
//
// And because we may want to create multiple windows, each hosting its own browser object (to
// display its own web page), then we need to create a IOleInPlaceFrame struct for each window. So,
// we're not going to declare our IOleInPlaceFrame struct globally. We'll allocate it later using
// GlobalAlloc, and then stuff the appropriate HWND in it then, and also stuff a pointer to
// MyIOleInPlaceFrameTable in it. But let's just define it here.
typedef struct {
	IOleInPlaceFrame	frame;		// The IOleInPlaceFrame must be first!

	///////////////////////////////////////////////////
	// Here you add any extra variables that you need
	// to access in your IOleInPlaceFrame functions.
	// You don't want those functions to access global
	// variables, because then you couldn't use more
	// than one browser object. (ie, You couldn't have
	// multiple windows, each with its own embedded
	// browser object to display a different web page).
	//
	// So here is where I added my extra HWND that my
	// IOleInPlaceFrame function Frame_GetWindow() needs
	// to access.
	///////////////////////////////////////////////////
	HWND				window;
} _IOleInPlaceFrameEx;






// Our IOleClientSite functions that the browser may call
HRESULT STDMETHODCALLTYPE Site_QueryInterface(IOleClientSite *This, REFIID riid, void ** ppvObject);
HRESULT STDMETHODCALLTYPE Site_AddRef(IOleClientSite *This);
HRESULT STDMETHODCALLTYPE Site_Release(IOleClientSite *This);
HRESULT STDMETHODCALLTYPE Site_SaveObject(IOleClientSite *This);
HRESULT STDMETHODCALLTYPE Site_GetMoniker(IOleClientSite *This, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk);
HRESULT STDMETHODCALLTYPE Site_GetContainer(IOleClientSite *This, LPOLECONTAINER *ppContainer);
HRESULT STDMETHODCALLTYPE Site_ShowObject(IOleClientSite *This);
HRESULT STDMETHODCALLTYPE Site_OnShowWindow(IOleClientSite *This, BOOL fShow);
HRESULT STDMETHODCALLTYPE Site_RequestNewObjectLayout(IOleClientSite *This);

// Our IOleClientSite VTable. This is the array of pointers to the above functions in our C
// program that the browser may call in order to interact with our frame window that contains
// the browser object. We must define a particular set of functions that comprise the
// IOleClientSite set of functions (see above), and then stuff pointers to those functions
// in their respective 'slots' in this table. We want the browser to use this VTable with our
// IOleClientSite structure.
IOleClientSiteVtbl MyIOleClientSiteTable = {Site_QueryInterface,
Site_AddRef,
Site_Release,
Site_SaveObject,
Site_GetMoniker,
Site_GetContainer,
Site_ShowObject,
Site_OnShowWindow,
Site_RequestNewObjectLayout};



// Our IDocHostUIHandler functions that the browser may call
HRESULT STDMETHODCALLTYPE UI_QueryInterface(IDocHostUIHandler *This, REFIID riid, void **ppvObject);
HRESULT STDMETHODCALLTYPE UI_AddRef(IDocHostUIHandler *This);
HRESULT STDMETHODCALLTYPE UI_Release(IDocHostUIHandler *This);
HRESULT STDMETHODCALLTYPE UI_ShowContextMenu(IDocHostUIHandler *This, DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved);
HRESULT STDMETHODCALLTYPE UI_GetHostInfo(IDocHostUIHandler *This, DOCHOSTUIINFO *pInfo);
HRESULT STDMETHODCALLTYPE UI_ShowUI(IDocHostUIHandler *This, DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc);
HRESULT STDMETHODCALLTYPE UI_HideUI(IDocHostUIHandler *This);
HRESULT STDMETHODCALLTYPE UI_UpdateUI(IDocHostUIHandler *This);
HRESULT STDMETHODCALLTYPE UI_EnableModeless(IDocHostUIHandler *This, BOOL fEnable);
HRESULT STDMETHODCALLTYPE UI_OnDocWindowActivate(IDocHostUIHandler *This, BOOL fActivate);
HRESULT STDMETHODCALLTYPE UI_OnFrameWindowActivate(IDocHostUIHandler *This, BOOL fActivate);
HRESULT STDMETHODCALLTYPE UI_ResizeBorder(IDocHostUIHandler *This, LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow);
HRESULT STDMETHODCALLTYPE UI_TranslateAccelerator(IDocHostUIHandler *This, LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID);
HRESULT STDMETHODCALLTYPE UI_GetOptionKeyPath(IDocHostUIHandler *This, LPOLESTR *pchKey, DWORD dw);
HRESULT STDMETHODCALLTYPE UI_GetDropTarget(IDocHostUIHandler *This, IDropTarget *pDropTarget, IDropTarget **ppDropTarget);
HRESULT STDMETHODCALLTYPE UI_GetExternal(IDocHostUIHandler *This, IDispatch **ppDispatch);
HRESULT STDMETHODCALLTYPE UI_TranslateUrl(IDocHostUIHandler *This, DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
HRESULT STDMETHODCALLTYPE UI_FilterDataObject(IDocHostUIHandler *This, IDataObject *pDO, IDataObject **ppDORet);

// Our IDocHostUIHandler VTable. This is the array of pointers to the above functions in our C
// program that the browser may call in order to replace/set certain user interface considerations
// (such as whether to display a pop-up context menu when the user right-clicks on the embedded
// browser object). We must define a particular set of functions that comprise the
// IDocHostUIHandler set of functions (see above), and then stuff pointers to those functions
// in their respective 'slots' in this table. We want the browser to use this VTable with our
// IDocHostUIHandler structure.
IDocHostUIHandlerVtbl MyIDocHostUIHandlerTable =  {UI_QueryInterface,
UI_AddRef,
UI_Release,
UI_ShowContextMenu,
UI_GetHostInfo,
UI_ShowUI,
UI_HideUI,
UI_UpdateUI,
UI_EnableModeless,
UI_OnDocWindowActivate,
UI_OnFrameWindowActivate,
UI_ResizeBorder,
UI_TranslateAccelerator,
UI_GetOptionKeyPath,
UI_GetDropTarget,
UI_GetExternal,
UI_TranslateUrl,
UI_FilterDataObject};

// We'll allocate our IDocHostUIHandler object dynamically with GlobalAlloc() for reasons outlined later.



// Our IOleInPlaceSite functions that the browser may call
HRESULT STDMETHODCALLTYPE InPlace_QueryInterface(IOleInPlaceSite *This, REFIID riid, void **ppvObject);
HRESULT STDMETHODCALLTYPE InPlace_AddRef(IOleInPlaceSite *This);
HRESULT STDMETHODCALLTYPE InPlace_Release(IOleInPlaceSite *This);
HRESULT STDMETHODCALLTYPE InPlace_GetWindow(IOleInPlaceSite *This, HWND *lphwnd);
HRESULT STDMETHODCALLTYPE InPlace_ContextSensitiveHelp(IOleInPlaceSite * This, BOOL fEnterMode);
HRESULT STDMETHODCALLTYPE InPlace_CanInPlaceActivate(IOleInPlaceSite *This);
HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceActivate(IOleInPlaceSite *This);
HRESULT STDMETHODCALLTYPE InPlace_OnUIActivate(IOleInPlaceSite *This);
HRESULT STDMETHODCALLTYPE InPlace_GetWindowContext(IOleInPlaceSite *This, LPOLEINPLACEFRAME *lplpFrame, LPOLEINPLACEUIWINDOW *lplpDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo);
HRESULT STDMETHODCALLTYPE InPlace_Scroll(IOleInPlaceSite *This, SIZE scrollExtent);
HRESULT STDMETHODCALLTYPE InPlace_OnUIDeactivate(IOleInPlaceSite *This, BOOL fUndoable);
HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceDeactivate(IOleInPlaceSite *This);
HRESULT STDMETHODCALLTYPE InPlace_DiscardUndoState(IOleInPlaceSite *This);
HRESULT STDMETHODCALLTYPE InPlace_DeactivateAndUndo(IOleInPlaceSite *This);
HRESULT STDMETHODCALLTYPE InPlace_OnPosRectChange(IOleInPlaceSite *This, LPCRECT lprcPosRect);

// Our IOleInPlaceSite VTable. This is the array of pointers to the above functions in our C
// program that the browser may call in order to interact with our frame window that contains
// the browser object. We must define a particular set of functions that comprise the
// IOleInPlaceSite set of functions (see above), and then stuff pointers to those functions
// in their respective 'slots' in this table. We want the browser to use this VTable with our
// IOleInPlaceSite structure.
IOleInPlaceSiteVtbl MyIOleInPlaceSiteTable =  {InPlace_QueryInterface,
InPlace_AddRef,
InPlace_Release,
InPlace_GetWindow,
InPlace_ContextSensitiveHelp,
InPlace_CanInPlaceActivate,
InPlace_OnInPlaceActivate,
InPlace_OnUIActivate,
InPlace_GetWindowContext,
InPlace_Scroll,
InPlace_OnUIDeactivate,
InPlace_OnInPlaceDeactivate,
InPlace_DiscardUndoState,
InPlace_DeactivateAndUndo,
InPlace_OnPosRectChange};

// We need to pass our IOleClientSite structure to OleCreate (which in turn gives it to the browser).
// But the browser is also going to ask our IOleClientSite's QueryInterface() to return a pointer to
// our IOleInPlaceSite and/or IDocHostUIHandler structs. So we'll need to have those pointers handy.
// Plus, some of our IOleClientSite and IOleInPlaceSite functions will need to have the HWND to our
// window, and also a pointer to our IOleInPlaceFrame struct. So let's create a single struct that
// has the IOleClientSite, IOleInPlaceSite, IDocHostUIHandler, and IOleInPlaceFrame structs all inside
// it (so we can easily get a pointer to any one from any of those structs' functions). As long as the
// IOleClientSite struct is the very first thing in this custom struct, it's all ok. We can still pass
// it to OleCreate() and pretend that it's an ordinary IOleClientSite. We'll call this new struct a
// _IOleClientSiteEx.
//
// And because we may want to create multiple windows, each hosting its own browser object (to
// display its own web page), then we need to create a unique _IOleClientSiteEx struct for
// each window. So, we're not going to declare this struct globally. We'll allocate it later
// using GlobalAlloc, and then initialize the structs within it.

typedef struct {
	IOleInPlaceSite			inplace;	// My IOleInPlaceSite object. Must be first with in _IOleInPlaceSiteEx.

	///////////////////////////////////////////////////
	// Here you add any extra variables that you need
	// to access in your IOleInPlaceSite functions.
	//
	// So here is where I added my IOleInPlaceFrame
	// struct. If you need extra variables, add them
	// at the end.
	///////////////////////////////////////////////////
	_IOleInPlaceFrameEx		frame;		// My IOleInPlaceFrame object. Must be first within my _IOleInPlaceFrameEx
} _IOleInPlaceSiteEx;

typedef struct {
	IDocHostUIHandler		ui;			// My IDocHostUIHandler object. Must be first.

	///////////////////////////////////////////////////
	// Here you add any extra variables that you need
	// to access in your IDocHostUIHandler functions.
	///////////////////////////////////////////////////
} _IDocHostUIHandlerEx;

typedef struct {
	IOleClientSite			client;		// My IOleClientSite object. Must be first.
	_IOleInPlaceSiteEx		inplace;	// My IOleInPlaceSite object. A convenient place to put it.
	_IDocHostUIHandlerEx	ui;			// My IDocHostUIHandler object. Must be first within my _IDocHostUIHandlerEx.

	///////////////////////////////////////////////////
	// Here you add any extra variables that you need
	// to access in your IOleClientSite functions.
	///////////////////////////////////////////////////
} _IOleClientSiteEx;













// This is a simple C example. There are lots more things you can control about the browser object, but
// we don't do it in this example. _Many_ of the functions we provide below for the browser to call, will
// never actually be called by the browser in our example. Why? Because we don't do certain things
// with the browser that would require it to call those functions (even though we need to provide
// at least some stub for all of the functions).
//
// So, for these "dummy functions" that we don't expect the browser to call, we'll just stick in some
// assembly code that causes a debugger breakpoint and tells the browser object that we don't support
// the functionality. That way, if you try to do more things with the browser object, and it starts
// calling these "dummy functions", you'll know which ones you should add more meaningful code to.
#define NOTIMPLEMENTED _ASSERT(0); return(E_NOTIMPL)









//////////////////////////////////// My IDocHostUIHandler functions  //////////////////////////////////////
// The browser object asks us for the pointer to our IDocHostUIHandler object by calling our IOleClientSite's
// QueryInterface (ie, Site_QueryInterface) and specifying a REFIID of IID_IDocHostUIHandler.
//
// NOTE: You need at least IE 4.0. Previous versions do not ask for, nor utilize, our IDocHostUIHandler functions.

HRESULT STDMETHODCALLTYPE UI_QueryInterface(IDocHostUIHandler *This, REFIID riid, LPVOID *ppvObj)
{
	// The browser assumes that our IDocHostUIHandler object is associated with our IOleClientSite
	// object. So it is possible that the browser may call our IDocHostUIHandler's QueryInterface()
	// to ask us to return a pointer to our IOleClientSite, in the same way that the browser calls
	// our IOleClientSite's QueryInterface() to ask for a pointer to our IDocHostUIHandler.
	//
	// Rather than duplicate much of the code in IOleClientSite's QueryInterface, let's just get
	// a pointer to our _IOleClientSiteEx object, substitute it as the 'This' arg, and call our
	// our IOleClientSite's QueryInterface. Note that since our _IDocHostUIHandlerEx is embedded right
	// inside our _IOleClientSiteEx, and comes immediately after the _IOleInPlaceSiteEx, we can employ
	// the following trickery to get the pointer to our _IOleClientSiteEx.
	return(Site_QueryInterface((IOleClientSite *)((char *)This - sizeof(IOleClientSite) - sizeof(_IOleInPlaceSiteEx)), riid, ppvObj));
}

HRESULT STDMETHODCALLTYPE UI_AddRef(IDocHostUIHandler *This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE UI_Release(IDocHostUIHandler *This)
{
	return(1);
}

// Called when the browser object is about to display its context menu.
HRESULT STDMETHODCALLTYPE UI_ShowContextMenu(IDocHostUIHandler *This, DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    // If desired, we can pop up your own custom context menu here. Of course,
	// we would need some way to get our window handle, so what we'd probably
	// do is like what we did with our IOleInPlaceFrame object. We'd define and create
	// our own IDocHostUIHandlerEx object with an embedded IDocHostUIHandler at the
	// start of it. Then we'd add an extra HWND field to store our window handle.
	// It could look like this:
	//
	// typedef struct _IDocHostUIHandlerEx {
	//		IDocHostUIHandler	ui;		// The IDocHostUIHandler must be first!
	//		HWND				window;
	// } IDocHostUIHandlerEx;
	//
	// Of course, we'd need a unique IDocHostUIHandlerEx object for each window, so
	// EmbedBrowserObject() would have to allocate one of those too. And that's
	// what we'd pass to our browser object (and it would in turn pass it to us
	// here, instead of 'This' being a IDocHostUIHandler *).

    // We will return S_OK to tell the browser not to display its default context menu,
	// or return S_FALSE to let the browser show its default context menu. For this
	// example, we wish to disable the browser's default context menu.
	return(S_FALSE);
}

// Called at initialization of the browser object UI. We can set various features of the browser object here.
HRESULT STDMETHODCALLTYPE UI_GetHostInfo(IDocHostUIHandler * This, DOCHOSTUIINFO *pInfo)
{
	pInfo->cbSize = sizeof(DOCHOSTUIINFO);

	// Set some flags. We don't want any 3D border. You can do other things like hide
	// the scroll bar (DOCHOSTUIFLAG_SCROLL_NO), display picture display (DOCHOSTUIFLAG_NOPICS),
	// disable any script running when the page is loaded (DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE),
	// open a site in a new browser window when the user clicks on some link (DOCHOSTUIFLAG_OPENNEWWIN),
	// and lots of other things. See the MSDN docs on the DOCHOSTUIINFO struct passed to us.
	pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER;

	// Set what happens when the user double-clicks on the object. Here we use the default.
	pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

	return(S_OK);
}

// Called when the browser object shows its UI. This allows us to replace its menus and toolbars by creating our
// own and displaying them here.
HRESULT STDMETHODCALLTYPE UI_ShowUI(IDocHostUIHandler *This, DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
	// We've already got our own UI in place so just return S_OK to tell the browser
	// not to display its menus/toolbars. Otherwise we'd return S_FALSE to let it do
	// that.
	return(S_OK);
}

// Called when browser object hides its UI. This allows us to hide any menus/toolbars we created in ShowUI.
HRESULT STDMETHODCALLTYPE UI_HideUI(IDocHostUIHandler *This)
{
	return(S_OK);
}

// Called when the browser object wants to notify us that the command state has changed. We should update any
// controls we have that are dependent upon our embedded object, such as "Back", "Forward", "Stop", or "Home"
// buttons.
HRESULT STDMETHODCALLTYPE UI_UpdateUI(IDocHostUIHandler *This)
{
	// We update our UI in our window message loop so we don't do anything here.
	return(S_OK);
}

// Called from the browser object's IOleInPlaceActiveObject object's EnableModeless() function. Also
// called when the browser displays a modal dialog box.
HRESULT STDMETHODCALLTYPE UI_EnableModeless(IDocHostUIHandler *This, BOOL fEnable)
{
	return(S_OK);
}

// Called from the browser object's IOleInPlaceActiveObject object's OnDocWindowActivate() function.
// This informs off of when the object is getting/losing the focus.
HRESULT STDMETHODCALLTYPE UI_OnDocWindowActivate(IDocHostUIHandler *This, BOOL fActivate)
{
	return(S_OK);
}

// Called from the browser object's IOleInPlaceActiveObject object's OnFrameWindowActivate() function.
HRESULT STDMETHODCALLTYPE UI_OnFrameWindowActivate(IDocHostUIHandler *This, BOOL fActivate)
{
	return(S_OK);
}

// Called from the browser object's IOleInPlaceActiveObject object's ResizeBorder() function.
HRESULT STDMETHODCALLTYPE UI_ResizeBorder(IDocHostUIHandler *This, LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
	return(S_OK);
}

// Called from the browser object's TranslateAccelerator routines to translate key strokes to commands.
HRESULT STDMETHODCALLTYPE UI_TranslateAccelerator(IDocHostUIHandler *This, LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
	// We don't intercept any keystrokes, so we do nothing here. But for example, if we wanted to
	// override the TAB key, perhaps do something with it ourselves, and then tell the browser
	// not to do anything with this keystroke, we'd do:
	//
	//	if (pMsg && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB)
	//	{
	//		// Here we do something as a result of a TAB key press.
	//
	//		// Tell the browser not to do anything with it.
	//		return(S_FALSE);
	//	}
	//
	//	// Otherwise, let the browser do something with this message.
	//	return(S_OK);

	// For our example, we want to make sure that the user can invoke some key to popup the context
	// menu, so we'll tell it to ignore all messages.
	return(S_FALSE);
}

// Called by the browser object to find where the host wishes the browser to get its options in the registry.
// We can use this to prevent the browser from using its default settings in the registry, by telling it to use
// some other registry key we've setup with the options we want.
HRESULT STDMETHODCALLTYPE UI_GetOptionKeyPath(IDocHostUIHandler *This, LPOLESTR *pchKey, DWORD dw)
{
	// Let the browser use its default registry settings.
	return(S_FALSE);
}

// Called by the browser object when it is used as a drop target. We can supply our own IDropTarget object,
// IDropTarget functions, and IDropTarget VTable if we want to determine what happens when someone drags and
// drops some object on our embedded browser object.
HRESULT STDMETHODCALLTYPE UI_GetDropTarget(IDocHostUIHandler *This, IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
	// Return our IDropTarget object associated with this IDocHostUIHandler object. I don't
	// know why we don't do this via UI_QueryInterface(), but we don't.

	// NOTE: If we want/need an IDropTarget interface, then we would have had to setup our own
	// IDropTarget functions, IDropTarget VTable, and create an IDropTarget object. We'd want to put
	// a pointer to the IDropTarget object in our own custom IDocHostUIHandlerEx object (like how
	// we may add an HWND field for the use of UI_ShowContextMenu). So when we created our
	// IDocHostUIHandlerEx object, maybe we'd add a 'idrop' field to the end of it, and
	// store a pointer to our IDropTarget object there. Then we could return this pointer as so:
	//
	// *pDropTarget = ((IDocHostUIHandlerEx *)This)->idrop;
    // return(S_OK);

	// But for our purposes, we don't need an IDropTarget object, so we'll tell whomever is calling
	// us that we don't have one.
    return(S_FALSE);
}

// Called by the browser when it wants a pointer to our IDispatch object. This object allows us to expose
// our own automation interface (ie, our own COM objects) to other entities that are running within the
// context of the browser so they can call our functions if they want. An example could be a javascript
// running in the URL we display could call our IDispatch functions. We'd write them so that any args passed
// to them would use the generic datatypes like a BSTR for utmost flexibility.
HRESULT STDMETHODCALLTYPE UI_GetExternal(IDocHostUIHandler *This, IDispatch **ppDispatch)
{
	// Return our IDispatch object associated with this IDocHostUIHandler object. I don't
	// know why we don't do this via UI_QueryInterface(), but we don't.

	// NOTE: If we want/need an IDispatch interface, then we would have had to setup our own
	// IDispatch functions, IDispatch VTable, and create an IDispatch object. We'd want to put
	// a pointer to the IDispatch object in our custom _IDocHostUIHandlerEx object (like how
	// we may add an HWND field for the use of UI_ShowContextMenu). So when we defined our
	// _IDocHostUIHandlerEx object, maybe we'd add a 'idispatch' field to the end of it, and
	// store a pointer to our IDispatch object there. Then we could return this pointer as so:
	//
	// *ppDispatch = ((_IDocHostUIHandlerEx *)This)->idispatch;
    // return(S_OK);

	// But for our purposes, we don't need an IDispatch object, so we'll tell whomever is calling
	// us that we don't have one. Note: We must set ppDispatch to 0 if we don't return our own
	// IDispatch object.
	*ppDispatch = 0;
	return(S_FALSE);
}


/* **************************** asciiToNum() ***************************
 * Converts the string of digits (expressed in base 10) to a 32-bit
 * DWORD.
 *
 * val =	Pointer to the nul-terminated string of digits to convert.
 *
 * RETURNS: The integer value as a DWORD.
 *
 * NOTE: Skips leading spaces before the first digit.
 */

DWORD asciiToNum(OLECHAR *val)
{
	OLECHAR			chr;
	DWORD			len;

	// Result is initially 0
	len = 0;

	// Skip leading spaces
	while (*val == ' ' || *val == 0x09) val++;

	// Convert next digit
	while (*val)
	{
		chr = *(val)++ - '0';
		if ((DWORD)chr > 9) break;
		len += (len + (len << 3) + chr);
	}

	return(len);
}

unsigned char AppUrl[] = {'a', 0, 'p', 0, 'p', 0, ':', 0};
wchar_t Blank[] = {L"about:blank"};

// Called by the browser object to give us an opportunity to modify the URL to be loaded.
HRESULT STDMETHODCALLTYPE UI_TranslateUrl(IDocHostUIHandler *This, DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
	unsigned short	*src;
	unsigned short	*dest;
	DWORD			len;

	// Get length of URL
	src = pchURLIn;
	while ((*(src)++));
	--src;
	len = src - pchURLIn; 

	// See if the URL starts with 'app:'
	if (len >= 4 && !_wcsnicmp(pchURLIn, (wchar_t *)&AppUrl[0], 4))
	{
		// Allocate a new buffer to return an "about:blank" URL
		if ((dest = (OLECHAR *)CoTaskMemAlloc(12<<1)))
		{
			HWND	hwnd;

			*ppchURLOut = dest;

			// Return "about:blank"
			CopyMemory(dest, &Blank[0], 12<<1);

			// Convert the number after the "app:"
			len = asciiToNum(pchURLIn + 4);

			// Get our host window. That was stored in our _IOleInPlaceFrameEx
			hwnd = ((_IOleInPlaceSiteEx *)((char *)This - sizeof(_IOleInPlaceSiteEx)))->frame.window;

			// Post a message to this window using WM_APP + 50, and pass the number converted above.
			// Do not SendMessage()!. Post instead, since the browser does not like us changing
			// the URL within this here callback. Why not WM_APP? Because the browser control appears
			// to use messages within the range of WM_APP to WM_APP + 2 (and maybe more). Microsoft
			// is a bit careless here. Supposedly, the operating system components, of which
			// MSHTML.DLL is now a part, are not supposed to use WM_APP and over, but no...
			// What we should probably do is RegisterWindowMessage()
			PostMessage(hwnd, WM_APP + 50, (WPARAM)len, 0);

			// Tell browser that we returned a URL
			return(S_OK);
		}
	}

	// We don't need to modify the URL. Note: We need to set ppchURLOut to 0 if we don't
	// return an OLECHAR (buffer) containing a modified version of pchURLIn.
	*ppchURLOut = 0;
    return(S_FALSE);
}

// Called by the browser when it does cut/paste to the clipboard. This allows us to block certain clipboard
// formats or support additional clipboard formats.
HRESULT STDMETHODCALLTYPE UI_FilterDataObject(IDocHostUIHandler *This, IDataObject *pDO, IDataObject **ppDORet)
{
	// Return our IDataObject object associated with this IDocHostUIHandler object. I don't
	// know why we don't do this via UI_QueryInterface(), but we don't.

	// NOTE: If we want/need an IDataObject interface, then we would have had to setup our own
	// IDataObject functions, IDataObject VTable, and create an IDataObject object. We'd want to put
	// a pointer to the IDataObject object in our custom _IDocHostUIHandlerEx object (like how
	// we may add an HWND field for the use of UI_ShowContextMenu). So when we defined our
	// _IDocHostUIHandlerEx object, maybe we'd add a 'idata' field to the end of it, and
	// store a pointer to our IDataObject object there. Then we could return this pointer as so:
	//
	// *ppDORet = ((_IDocHostUIHandlerEx *)This)->idata;
    // return(S_OK);

	// But for our purposes, we don't need an IDataObject object, so we'll tell whomever is calling
	// us that we don't have one. Note: We must set ppDORet to 0 if we don't return our own
	// IDataObject object.
	*ppDORet = 0;
	return(S_FALSE);
}









////////////////////////////////////// My IOleClientSite functions  /////////////////////////////////////
// We give the browser object a pointer to our IOleClientSite object when we call OleCreate() or DoVerb().

/************************* Site_QueryInterface() *************************
 * The browser object calls this when it wants a pointer to one of our
 * IOleClientSite, IDocHostUIHandler, or IOleInPlaceSite structures. They
 * are all accessible via the _IOleClientSiteEx struct we allocated in
 * EmbedBrowserObject() and passed to DoVerb() and OleCreate().
 *
 * This =		A pointer to whatever _IOleClientSiteEx struct we passed to
 *				OleCreate() or DoVerb().
 * riid =		A GUID struct that the browser passes us to clue us as to
 *				which type of struct (object) it would like a pointer
 *				returned for.
 * ppvObject =	Where the browser wants us to return a pointer to the
 *				appropriate struct. (ie, It passes us a handle to fill in).
 *
 * RETURNS: S_OK if we return the struct, or E_NOINTERFACE if we don't have
 * the requested struct.
 */

HRESULT STDMETHODCALLTYPE Site_QueryInterface(IOleClientSite *This, REFIID riid, void ** ppvObject)
{
	// It just so happens that the first arg passed to us is our _IOleClientSiteEx struct we allocated
	// and passed to DoVerb() and OleCreate(). Nevermind that 'This' is declared is an IOleClientSite *.
	// Remember that in EmbedBrowserObject(), we allocated our own _IOleClientSiteEx struct, and lied
	// to OleCreate() and DoVerb() -- passing our _IOleClientSiteEx struct and saying it was an
	// IOleClientSite struct. It's ok. An _IOleClientSiteEx starts with an embedded IOleClientSite, so
	// the browser didn't mind. So that's what the browser object is passing us now. The browser doesn't
	// know that it's really an _IOleClientSiteEx struct. But we do. So we can recast it and use it as
	// so here.

	// If the browser is asking us to match IID_IOleClientSite, then it wants us to return a pointer to
	// our IOleClientSite struct. Then the browser will use the VTable in that struct to call our
	// IOleClientSite functions. It will also pass this same pointer to all of our IOleClientSite
	// functions.
	//
	// Actually, we're going to lie to the browser again. We're going to return our own _IOleClientSiteEx
	// struct, and tell the browser that it's a IOleClientSite struct. It's ok. The first thing in our
	// _IOleClientSiteEx is an embedded IOleClientSite, so the browser doesn't mind. We want the browser
	// to continue passing our _IOleClientSiteEx pointer wherever it would normally pass a IOleClientSite
	// pointer.
	// 
	// The IUnknown interface uses the same VTable as the first object in our _IOleClientSiteEx
	// struct (which happens to be an IOleClientSite). So if the browser is asking us to match
	// IID_IUnknown, then we'll also return a pointer to our _IOleClientSiteEx.

	if (!memcmp(riid, &IID_IUnknown, sizeof(GUID)) || !memcmp(riid, &IID_IOleClientSite, sizeof(GUID)))
		*ppvObject = &((_IOleClientSiteEx *)This)->client;

	// If the browser is asking us to match IID_IOleInPlaceSite, then it wants us to return a pointer to
	// our IOleInPlaceSite struct. Then the browser will use the VTable in that struct to call our
	// IOleInPlaceSite functions.  It will also pass this same pointer to all of our IOleInPlaceSite
	// functions (except for Site_QueryInterface, Site_AddRef, and Site_Release. Those will always get
	// the pointer to our _IOleClientSiteEx.
	//
	// Actually, we're going to lie to the browser. We're going to return our own _IOleInPlaceSiteEx
	// struct, and tell the browser that it's a IOleInPlaceSite struct. It's ok. The first thing in
	// our _IOleInPlaceSiteEx is an embedded IOleInPlaceSite, so the browser doesn't mind. We want the
	// browser to continue passing our _IOleInPlaceSiteEx pointer wherever it would normally pass a
	// IOleInPlaceSite pointer.
	else if (!memcmp(riid, &IID_IOleInPlaceSite, sizeof(GUID)))
		*ppvObject = &((_IOleClientSiteEx *)This)->inplace;

	// If the browser is asking us to match IID_IDocHostUIHandler, then it wants us to return a pointer to
	// our IDocHostUIHandler struct. Then the browser will use the VTable in that struct to call our
	// IDocHostUIHandler functions.  It will also pass this same pointer to all of our IDocHostUIHandler
	// functions (except for Site_QueryInterface, Site_AddRef, and Site_Release. Those will always get
	// the pointer to our _IOleClientSiteEx.
	//
	// Actually, we're going to lie to the browser. We're going to return our own _IDocHostUIHandlerEx
	// struct, and tell the browser that it's a IDocHostUIHandler struct. It's ok. The first thing in
	// our _IDocHostUIHandlerEx is an embedded IDocHostUIHandler, so the browser doesn't mind. We want the
	// browser to continue passing our _IDocHostUIHandlerEx pointer wherever it would normally pass a
	// IDocHostUIHandler pointer. My, we're really playing dirty tricks on the browser here. heheh.
	else if (!memcmp(riid, &IID_IDocHostUIHandler, sizeof(GUID)))
		*ppvObject = &((_IOleClientSiteEx *)This)->ui;

	// For other types of objects the browser wants, just report that we don't have any such objects.
	// NOTE: If you want to add additional functionality to your browser hosting, you may need to
	// provide some more objects here. You'll have to investigate what the browser is asking for
	// (ie, what REFIID it is passing).
	else
	{
		*ppvObject = 0;
		return(E_NOINTERFACE);
	}

	return(S_OK);
}

HRESULT STDMETHODCALLTYPE Site_AddRef(IOleClientSite *This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE Site_Release(IOleClientSite *This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE Site_SaveObject(IOleClientSite *This)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_GetMoniker(IOleClientSite *This, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_GetContainer(IOleClientSite *This, LPOLECONTAINER *ppContainer)
{
	// Tell the browser that we are a simple object and don't support a container
	*ppContainer = 0;

	return(E_NOINTERFACE);
}

HRESULT STDMETHODCALLTYPE Site_ShowObject(IOleClientSite *This)
{
	return(NOERROR);
}

HRESULT STDMETHODCALLTYPE Site_OnShowWindow(IOleClientSite *This, BOOL fShow)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_RequestNewObjectLayout(IOleClientSite *This)
{
	NOTIMPLEMENTED;
}











////////////////////////////////////// My IOleInPlaceSite functions  /////////////////////////////////////
// The browser object asks us for the pointer to our IOleInPlaceSite object by calling our IOleClientSite's
// QueryInterface (ie, Site_QueryInterface) and specifying a REFIID of IID_IOleInPlaceSite.

HRESULT STDMETHODCALLTYPE InPlace_QueryInterface(IOleInPlaceSite *This, REFIID riid, LPVOID *ppvObj)
{
	// The browser assumes that our IOleInPlaceSite object is associated with our IOleClientSite
	// object. So it is possible that the browser may call our IOleInPlaceSite's QueryInterface()
	// to ask us to return a pointer to our IOleClientSite, in the same way that the browser calls
	// our IOleClientSite's QueryInterface() to ask for a pointer to our IOleInPlaceSite.
	//
	// Rather than duplicate much of the code in IOleClientSite's QueryInterface, let's just get
	// a pointer to our _IOleClientSiteEx object, substitute it as the 'This' arg, and call our
	// our IOleClientSite's QueryInterface. Note that since our IOleInPlaceSite is embedded right
	// inside our _IOleClientSiteEx, and comes immediately after the IOleClientSite, we can employ
	// the following trickery to get the pointer to our _IOleClientSiteEx.
	return(Site_QueryInterface((IOleClientSite *)((char *)This - sizeof(IOleClientSite)), riid, ppvObj));
}

HRESULT STDMETHODCALLTYPE InPlace_AddRef(IOleInPlaceSite *This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE InPlace_Release(IOleInPlaceSite *This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE InPlace_GetWindow(IOleInPlaceSite *This, HWND *lphwnd)
{
	// Return the HWND of the window that contains this browser object. We stored that
	// HWND in our _IOleInPlaceSiteEx struct. Nevermind that the function declaration for
	// Site_GetWindow says that 'This' is an IOleInPlaceSite *. Remember that in
	// EmbedBrowserObject(), we allocated our own _IOleInPlaceSiteEx struct which
	// contained an embedded IOleInPlaceSite struct within it. And when the browser
	// called Site_QueryInterface() to get a pointer to our IOleInPlaceSite object, we
	// returned a pointer to our _IOleClientSiteEx. The browser doesn't know this. But
	// we do. That's what we're really being passed, so we can recast it and use it as
	// so here.
	*lphwnd = ((_IOleInPlaceSiteEx *)This)->frame.window;

	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_ContextSensitiveHelp(IOleInPlaceSite *This, BOOL fEnterMode)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE InPlace_CanInPlaceActivate(IOleInPlaceSite *This)
{
	// Tell the browser we can in place activate
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceActivate(IOleInPlaceSite *This)
{
	// Tell the browser we did it ok
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_OnUIActivate(IOleInPlaceSite *This)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_GetWindowContext(IOleInPlaceSite *This, LPOLEINPLACEFRAME *lplpFrame, LPOLEINPLACEUIWINDOW *lplpDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
	// Give the browser the pointer to our IOleInPlaceFrame struct. We stored that pointer
	// in our _IOleInPlaceSiteEx struct. Nevermind that the function declaration for
	// Site_GetWindowContext says that 'This' is an IOleInPlaceSite *. Remember that in
	// EmbedBrowserObject(), we allocated our own _IOleInPlaceSiteEx struct which
	// contained an embedded IOleInPlaceSite struct within it. And when the browser
	// called Site_QueryInterface() to get a pointer to our IOleInPlaceSite object, we
	// returned a pointer to our _IOleClientSiteEx. The browser doesn't know this. But
	// we do. That's what we're really being passed, so we can recast it and use it as
	// so here.
	//
	// Actually, we're giving the browser a pointer to our own _IOleInPlaceSiteEx struct,
	// but telling the browser that it's a IOleInPlaceSite struct. No problem. Our
	// _IOleInPlaceSiteEx starts with an embedded IOleInPlaceSite, so the browser is
	// cool with it. And we want the browser to pass a pointer to this _IOleInPlaceSiteEx
	// wherever it would pass a IOleInPlaceSite struct to our IOleInPlaceSite functions.
	*lplpFrame = (LPOLEINPLACEFRAME)&((_IOleInPlaceSiteEx *)This)->frame;

	// We have no OLEINPLACEUIWINDOW
	*lplpDoc = 0;

	// Fill in some other info for the browser
	lpFrameInfo->fMDIApp = FALSE;
	lpFrameInfo->hwndFrame = ((_IOleInPlaceFrameEx *)*lplpFrame)->window;
	lpFrameInfo->haccel = 0;
	lpFrameInfo->cAccelEntries = 0;

	// Give the browser the dimensions of where it can draw. We give it our entire window to fill.
	// We do this in InPlace_OnPosRectChange() which is called right when a window is first
	// created anyway, so no need to duplicate it here.
//	GetClientRect(lpFrameInfo->hwndFrame, lprcPosRect);
//	GetClientRect(lpFrameInfo->hwndFrame, lprcClipRect);

	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_Scroll(IOleInPlaceSite *This, SIZE scrollExtent)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE InPlace_OnUIDeactivate(IOleInPlaceSite *This, BOOL fUndoable)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceDeactivate(IOleInPlaceSite *This)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_DiscardUndoState(IOleInPlaceSite *This)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE InPlace_DeactivateAndUndo(IOleInPlaceSite *This)
{
	NOTIMPLEMENTED;
}

// Called when the position of the browser object is changed, such as when we call the IWebBrowser2's put_Width(),
// put_Height(), put_Left(), or put_Right().
HRESULT STDMETHODCALLTYPE InPlace_OnPosRectChange(IOleInPlaceSite *This, LPCRECT lprcPosRect)
{
	IOleObject			*browserObject;
	IOleInPlaceObject	*inplace;

	// We need to get the browser's IOleInPlaceObject object so we can call its SetObjectRects
	// function.
	browserObject = *((IOleObject **)((char *)This - sizeof(IOleObject *) - sizeof(IOleClientSite)));
	if (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IOleInPlaceObject, (void**)&inplace))
	{
		// Give the browser the dimensions of where it can draw.
		inplace->lpVtbl->SetObjectRects(inplace, lprcPosRect, lprcPosRect);
	}

	return(S_OK);
}







////////////////////////////////////// My IOleInPlaceFrame functions  /////////////////////////////////////////

HRESULT STDMETHODCALLTYPE Frame_QueryInterface(IOleInPlaceFrame *This, REFIID riid, LPVOID *ppvObj)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_AddRef(IOleInPlaceFrame *This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE Frame_Release(IOleInPlaceFrame *This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE Frame_GetWindow(IOleInPlaceFrame *This, HWND *lphwnd)
{
	// Give the browser the HWND to our window that contains the browser object. We
	// stored that HWND in our IOleInPlaceFrame struct. Nevermind that the function
	// declaration for Frame_GetWindow says that 'This' is an IOleInPlaceFrame *. Remember
	// that in EmbedBrowserObject(), we allocated our own IOleInPlaceFrameEx struct which
	// contained an embedded IOleInPlaceFrame struct within it. And then we lied when
	// Site_GetWindowContext() returned that IOleInPlaceFrameEx. So that's what the
	// browser is passing us. It doesn't know that. But we do. So we can recast it and
	// use it as so here.
	*lphwnd = ((_IOleInPlaceFrameEx *)This)->window;
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE Frame_ContextSensitiveHelp(IOleInPlaceFrame *This, BOOL fEnterMode)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_GetBorder(IOleInPlaceFrame *This, LPRECT lprectBorder)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_RequestBorderSpace(IOleInPlaceFrame *This, LPCBORDERWIDTHS pborderwidths)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_SetBorderSpace(IOleInPlaceFrame *This, LPCBORDERWIDTHS pborderwidths)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_SetActiveObject(IOleInPlaceFrame *This, IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE Frame_InsertMenus(IOleInPlaceFrame *This, HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_SetMenu(IOleInPlaceFrame *This, HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE Frame_RemoveMenus(IOleInPlaceFrame *This, HMENU hmenuShared)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_SetStatusText(IOleInPlaceFrame *This, LPCOLESTR pszStatusText)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE Frame_EnableModeless(IOleInPlaceFrame *This, BOOL fEnable)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE Frame_TranslateAccelerator(IOleInPlaceFrame *This, LPMSG lpmsg, WORD wID)
{
	NOTIMPLEMENTED;
}








/*************************** UnEmbedBrowserObject() ************************
 * Called to detach the browser object from our host window, and free its
 * resources, right before we destroy our window.
 *
 * hwnd =		Handle to the window hosting the browser object.
 *
 * NOTE: The pointer to the browser object must have been stored in the
 * window's USERDATA field. In other words, don't call UnEmbedBrowserObject().
 * with a HWND that wasn't successfully passed to EmbedBrowserObject().
 */

void UnEmbedBrowserObject(HWND hwnd)
{
	IOleObject	**browserHandle;
	IOleObject	*browserObject;

	// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when we
	// initially attached the browser object to this window. If 0, you may have called this
	// for a window that wasn't successfully passed to EmbedBrowserObject(). Bad boy! Or, we
	// may have failed the EmbedBrowserObject() call in WM_CREATE, in which case, our window
	// may get a WM_DESTROY which could call this a second time (ie, since we may call
	// UnEmbedBrowserObject in EmbedBrowserObject).
	if ((browserHandle = (IOleObject **) GetWindowLongPtr(hwnd, GWLP_USERDATA)))
	{
		browserObject = *browserHandle;

		// Unembed the browser object, and release its resources.
		browserObject->lpVtbl->Close(browserObject, OLECLOSE_NOSAVE);
		browserObject->lpVtbl->Release(browserObject);

		// Zero out the pointer just in case UnEmbedBrowserObject is called again for this window
		SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);	
	}
}






#define WEBPAGE_GOBACK		0
#define WEBPAGE_GOFORWARD	1
#define WEBPAGE_GOHOME		2
#define WEBPAGE_SEARCH		3
#define WEBPAGE_REFRESH		4
#define WEBPAGE_STOP		5

/******************************* DoPageAction() **************************
 * Implements the functionality of a "Back". "Forward", "Home", "Search",
 * "Refresh", or "Stop" button.
 *
 * hwnd =		Handle to the window hosting the browser object.
 * action =		One of the following:
 *				0 = Move back to the previously viewed web page.
 *				1 = Move forward to the previously viewed web page.
 *				2 = Move to the home page.
 *				3 = Search.
 *				4 = Refresh the page.
 *				5 = Stop the currently loading page.
 *
 * NOTE: EmbedBrowserObject() must have been successfully called once with the
 * specified window, prior to calling this function. You need call
 * EmbedBrowserObject() once only, and then you can make multiple calls to
 * this function to display numerous pages in the specified window.
 */

void DoPageAction(HWND hwnd, DWORD action)
{	
	IWebBrowser2	*webBrowser2;
	IOleObject		*browserObject;

	// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
	// we initially attached the browser object to this window.
	browserObject = *((IOleObject **) GetWindowLongPtr(hwnd, GWLP_USERDATA));

	// We want to get the base address (ie, a pointer) to the IWebBrowser2 object embedded within the browser
	// object, so we can call some of the functions in the former's table.
	if (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))
	{
		// Ok, now the pointer to our IWebBrowser2 object is in 'webBrowser2', and so its VTable is
		// webBrowser2->lpVtbl.

		// Call the desired function
		switch (action)
		{
			case WEBPAGE_GOBACK:
			{
				// Call the IWebBrowser2 object's GoBack function.
				webBrowser2->lpVtbl->GoBack(webBrowser2);
				break;
			}

			case WEBPAGE_GOFORWARD:
			{
				// Call the IWebBrowser2 object's GoForward function.
				webBrowser2->lpVtbl->GoForward(webBrowser2);
				break;
			}

			case WEBPAGE_GOHOME:
			{
				// Call the IWebBrowser2 object's GoHome function.
				webBrowser2->lpVtbl->GoHome(webBrowser2);
				break;
			}

			case WEBPAGE_SEARCH:
			{
				// Call the IWebBrowser2 object's GoSearch function.
				webBrowser2->lpVtbl->GoSearch(webBrowser2);
				break;
			}

			case WEBPAGE_REFRESH:
			{
				// Call the IWebBrowser2 object's Refresh function.
				webBrowser2->lpVtbl->Refresh(webBrowser2);
			}

			case WEBPAGE_STOP:
			{
				// Call the IWebBrowser2 object's Stop function.
				webBrowser2->lpVtbl->Stop(webBrowser2);
			}
		}

		// Release the IWebBrowser2 object.
		webBrowser2->lpVtbl->Release(webBrowser2);
	}
}





/******************************* DisplayHTMLStr() ****************************
 * Takes a string containing some HTML BODY, and displays it in the specified
 * window. For example, perhaps you want to display the HTML text of...
 *
 * <P>This is a picture.<P><IMG src="mypic.jpg">
 *
 * hwnd =		Handle to the window hosting the browser object.
 * string =		Pointer to nul-terminated string containing the HTML BODY.
 *				(NOTE: No <BODY></BODY> tags are required in the string).
 *
 * RETURNS: 0 if success, or non-zero if an error.
 *
 * NOTE: EmbedBrowserObject() must have been successfully called once with the
 * specified window, prior to calling this function. You need call
 * EmbedBrowserObject() once only, and then you can make multiple calls to
 * this function to display numerous pages in the specified window.
 */

long DisplayHTMLStr(HWND hwnd, LPCTSTR string)
{	
	IWebBrowser2	*webBrowser2;
	LPDISPATCH		lpDispatch;
	IHTMLDocument2	*htmlDoc2;
	IOleObject		*browserObject;
	SAFEARRAY		*sfArray;
	VARIANT			myURL;
	VARIANT			*pVar;
	BSTR			bstr;

	// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
	// we initially attached the browser object to this window.
	browserObject = *((IOleObject **)GetWindowLong(hwnd, GWLP_USERDATA));

	// Assume an error.
	bstr = 0;

	// We want to get the base address (ie, a pointer) to the IWebBrowser2 object embedded within the browser
	// object, so we can call some of the functions in the former's table.
	if (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))
	{
		// Ok, now the pointer to our IWebBrowser2 object is in 'webBrowser2', and so its VTable is
		// webBrowser2->lpVtbl.

		// Call the IWebBrowser2 object's get_Document so we can get its DISPATCH object. I don't know why you
		// don't get the DISPATCH object via the browser object's QueryInterface(), but you don't.
		lpDispatch = 0;
		webBrowser2->lpVtbl->get_Document(webBrowser2, &lpDispatch);
		if (!lpDispatch)
		{
			// Before we can get_Document(), we actually need to have some HTML page loaded in the browser. So,
			// let's load an empty HTML page. Then, once we have that empty page, we can get_Document() and
			// write() to stuff our HTML string into it.
			VariantInit(&myURL);
			myURL.vt = VT_BSTR;
			myURL.bstrVal = SysAllocString(&Blank[0]);

			// Call the Navigate2() function to actually display the page.
			webBrowser2->lpVtbl->Navigate2(webBrowser2, &myURL, 0, 0, 0, 0);

			// Free any resources (including the BSTR).
			VariantClear(&myURL);
		}

		// Call the IWebBrowser2 object's get_Document so we can get its DISPATCH object. I don't know why you
		// don't get the DISPATCH object via the browser object's QueryInterface(), but you don't.
		if (!webBrowser2->lpVtbl->get_Document(webBrowser2, &lpDispatch) && lpDispatch)
		{
			// We want to get a pointer to the IHTMLDocument2 object embedded within the DISPATCH
			// object, so we can call some of the functions in the former's table.
			if (!lpDispatch->lpVtbl->QueryInterface(lpDispatch, &IID_IHTMLDocument2, (void**)&htmlDoc2))
			{
				// Ok, now the pointer to our IHTMLDocument2 object is in 'htmlDoc2', and so its VTable is
				// htmlDoc2->lpVtbl.

				// Our HTML must be in the form of a BSTR. And it must be passed to write() in an
				// array of "VARIENT" structs. So let's create all that.
				if ((sfArray = SafeArrayCreate(VT_VARIANT, 1, (SAFEARRAYBOUND *)&ArrayBound)))
				{
					if (!SafeArrayAccessData(sfArray, (void**)&pVar))
					{
						pVar->vt = VT_BSTR;
#ifndef UNICODE
						{
						wchar_t		*buffer;
						DWORD		size;

						size = MultiByteToWideChar(CP_ACP, 0, string, -1, 0, 0);
						if (!(buffer = (wchar_t *)GlobalAlloc(GMEM_FIXED, sizeof(wchar_t) * size))) goto bad;
						MultiByteToWideChar(CP_ACP, 0, string, -1, buffer, size);
						bstr = SysAllocString(buffer);
						GlobalFree(buffer);
						}
#else
						bstr = SysAllocString(string);
#endif
						// Store our BSTR pointer in the VARIENT.
						if ((pVar->bstrVal = bstr))
						{
							// Pass the VARIENT with its BSTR to write() in order to shove our desired HTML string
							// into the body of that empty page we created above.
							htmlDoc2->lpVtbl->write(htmlDoc2, sfArray);

							// Close the document. If we don't do this, subsequent calls to DisplayHTMLStr
							// would append to the current contents of the page
							htmlDoc2->lpVtbl->close(htmlDoc2);

							// Normally, we'd need to free our BSTR, but SafeArrayDestroy() does it for us
//							SysFreeString(bstr);
						}
					}

					// Free the array. This also frees the VARIENT that SafeArrayAccessData created for us,
					// and even frees the BSTR we allocated with SysAllocString
					SafeArrayDestroy(sfArray);
				}

				// Release the IHTMLDocument2 object.
bad:			htmlDoc2->lpVtbl->Release(htmlDoc2);
			}

			// Release the DISPATCH object.
			lpDispatch->lpVtbl->Release(lpDispatch);
		}

		// Release the IWebBrowser2 object.
		webBrowser2->lpVtbl->Release(webBrowser2);
	}

	// No error?
	if (bstr) return(0);

	// An error
	return(-1);
}






/******************************* DisplayHTMLPage() ****************************
 * Displays a URL, or HTML file on disk.
 *
 * hwnd =		Handle to the window hosting the browser object.
 * webPageName =	Pointer to nul-terminated name of the URL/file.
 *
 * RETURNS: 0 if success, or non-zero if an error.
 *
 * NOTE: EmbedBrowserObject() must have been successfully called once with the
 * specified window, prior to calling this function. You need call
 * EmbedBrowserObject() once only, and then you can make multiple calls to
 * this function to display numerous pages in the specified window.
 */

long DisplayHTMLPage(HWND hwnd, LPTSTR webPageName)
{
	IWebBrowser2	*webBrowser2;
	VARIANT			myURL;
	IOleObject		*browserObject;

	// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
	// we initially attached the browser object to this window.
	browserObject = *((IOleObject **) GetWindowLongPtr(hwnd, GWLP_USERDATA));

	// We want to get the base address (ie, a pointer) to the IWebBrowser2 object embedded within the browser
	// object, so we can call some of the functions in the former's table.
	if (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))
	{
		// Ok, now the pointer to our IWebBrowser2 object is in 'webBrowser2', and so its VTable is
		// webBrowser2->lpVtbl.

		// Our URL (ie, web address, such as "http://www.microsoft.com" or an HTM filename on disk
		// such as "c:\myfile.htm") must be passed to the IWebBrowser2's Navigate2() function as a BSTR.
		// A BSTR is like a pascal version of a double-byte character string. In other words, the
		// first unsigned short is a count of how many characters are in the string, and then this
		// is followed by those characters, each expressed as an unsigned short (rather than a
		// char). The string is not nul-terminated. The OS function SysAllocString can allocate and
		// copy a UNICODE C string to a BSTR. Of course, we'll need to free that BSTR after we're done
		// with it. If we're not using UNICODE, we first have to convert to a UNICODE string.
		//
		// What's more, our BSTR needs to be stuffed into a VARIENT struct, and that VARIENT struct is
		// then passed to Navigate2(). Why? The VARIENT struct makes it possible to define generic
		// 'datatypes' that can be used with all languages. Not all languages support things like
		// nul-terminated C strings. So, by using a VARIENT, whose first field tells what sort of
		// data (ie, string, float, etc) is in the VARIENT, COM interfaces can be used by just about
		// any language.
		VariantInit(&myURL);
		myURL.vt = VT_BSTR;

#ifndef UNICODE
		{
		wchar_t		*buffer;
		DWORD		size;

		size = MultiByteToWideChar(CP_ACP, 0, webPageName, -1, 0, 0);
		if (!(buffer = (wchar_t *)GlobalAlloc(GMEM_FIXED, sizeof(wchar_t) * size))) goto badalloc;
		MultiByteToWideChar(CP_ACP, 0, webPageName, -1, buffer, size);
		myURL.bstrVal = SysAllocString(buffer);
		GlobalFree(buffer);
		}
#else
		myURL.bstrVal = SysAllocString(webPageName);
#endif
		if (!myURL.bstrVal)
		{
badalloc:	webBrowser2->lpVtbl->Release(webBrowser2);
			return(-6);
		}

		// Call the Navigate2() function to actually display the page.
		webBrowser2->lpVtbl->Navigate2(webBrowser2, &myURL, 0, 0, 0, 0);

		// Free any resources (including the BSTR we allocated above).
		VariantClear(&myURL);

		// We no longer need the IWebBrowser2 object (ie, we don't plan to call any more functions in it,
		// so we can release our hold on it). Note that we'll still maintain our hold on the browser
		// object.
		webBrowser2->lpVtbl->Release(webBrowser2);

		// Success
		return(0);
	}

	return(-5);
}





/******************************* ResizeBrowser() ****************************
 * Resizes the browser object for the specified window to the specified
 * width and height.
 *
 * hwnd =			Handle to the window hosting the browser object.
 * width =			Width.
 * height =			Height.
 *
 * NOTE: EmbedBrowserObject() must have been successfully called once with the
 * specified window, prior to calling this function. You need call
 * EmbedBrowserObject() once only, and then you can make multiple calls to
 * this function to resize the browser object.
 */

void ResizeBrowser(HWND hwnd, DWORD width, DWORD height)
{
	IWebBrowser2	*webBrowser2;
	IOleObject		*browserObject;

	// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
	// we initially attached the browser object to this window.
	browserObject = *((IOleObject **) GetWindowLongPtr(hwnd, GWLP_USERDATA));

	// We want to get the base address (ie, a pointer) to the IWebBrowser2 object embedded within the browser
	// object, so we can call some of the functions in the former's table.
	if (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))
	{
		// Ok, now the pointer to our IWebBrowser2 object is in 'webBrowser2', and so its VTable is
		// webBrowser2->lpVtbl.

		// Call are put_Width() and put_Height() to set the new width/height.
		webBrowser2->lpVtbl->put_Width(webBrowser2, width);
		webBrowser2->lpVtbl->put_Height(webBrowser2, height);

		// We no longer need the IWebBrowser2 object (ie, we don't plan to call any more functions in it,
		// so we can release our hold on it). Note that we'll still maintain our hold on the browser
		// object.
		webBrowser2->lpVtbl->Release(webBrowser2);
	}
}





/***************************** EmbedBrowserObject() **************************
 * Puts the browser object inside our host window, and save a pointer to this
 * window's browser object in the window's GWL_USERDATA field.
 *
 * hwnd =		Handle of our window into which we embed the browser object.
 *
 * RETURNS: 0 if success, or non-zero if an error.
 *
 * NOTE: We tell the browser object to occupy the entire client area of the
 * window.
 *
 * NOTE: No HTML page will be displayed here. We can do that with a subsequent
 * call to either DisplayHTMLPage() or DisplayHTMLStr(). This is merely once-only
 * initialization for using the browser object. In a nutshell, what we do
 * here is get a pointer to the browser object in our window's GWL_USERDATA
 * so we can access that object's functions whenever we want, and we also pass
 * pointers to our IOleClientSite, IOleInPlaceFrame, and IStorage structs so that
 * the browser can call our functions in our struct's VTables.
 */

long EmbedBrowserObject(HWND hwnd)
{
	LPCLASSFACTORY		pClassFactory;
	IOleObject			*browserObject;
	IWebBrowser2		*webBrowser2;
	RECT				rect;
	char				*ptr;
	_IOleClientSiteEx	*_iOleClientSiteEx;

	// Our IOleClientSite, IOleInPlaceSite, and IOleInPlaceFrame functions need to get our window handle. We
	// could store that in some global. But then, that would mean that our functions would work with only that
	// one window. If we want to create multiple windows, each hosting its own browser object (to display its
	// own web page), then we need to create unique IOleClientSite, IOleInPlaceSite, and IOleInPlaceFrame
	// structs for each window. And we'll put an extra field at the end of those structs to store our extra
	// data such as a window handle. So, our functions won't have to touch global data, and can therefore be
	// re-entrant and work with multiple objects/windows.
	//
	// Remember that a pointer to our IOleClientSite we create here will be passed as the first arg to every
	// one of our IOleClientSite functions. Ditto with the IOleInPlaceFrame object we create here, and the
	// IOleInPlaceFrame functions. So, our functions are able to retrieve the window handle we'll store here,
	// and then, they'll work with all such windows containing a browser control.
	//
	// Furthermore, since the browser will be calling our IOleClientSite's QueryInterface to get a pointer to
	// our IOleInPlaceSite and IDocHostUIHandler objects, that means that our IOleClientSite QueryInterface
	// must have an easy way to grab those pointers. Probably the easiest thing to do is just embed our
	// IOleInPlaceSite and IDocHostUIHandler objects inside of an extended IOleClientSite which we'll call
	// a _IOleClientSiteEx. As long as they come after the pointer to the IOleClientSite VTable, then we're
	// ok.
	//
	// Of course, we need to GlobalAlloc the above structs now. We'll just get all 4 with a single call to
	// GlobalAlloc, especially since 3 of them are all contained inside of our _IOleClientSiteEx anyway.
	//
	// So, we're not actually allocating separate IOleClientSite, IOleInPlaceSite, IOleInPlaceFrame and
	// IDocHostUIHandler structs.
	//
	// One final thing. We're going to allocate extra room to store the pointer to the browser object.
	if (!(ptr = (char *)GlobalAlloc(GMEM_FIXED, sizeof(_IOleClientSiteEx) + sizeof(IOleObject *))))
		return(-1);

	// Initialize our IOleClientSite object with a pointer to our IOleClientSite VTable.
	_iOleClientSiteEx = (_IOleClientSiteEx *)(ptr + sizeof(IOleObject *));
	_iOleClientSiteEx->client.lpVtbl = &MyIOleClientSiteTable;

	// Initialize our IOleInPlaceSite object with a pointer to our IOleInPlaceSite VTable.
	_iOleClientSiteEx->inplace.inplace.lpVtbl = &MyIOleInPlaceSiteTable;

	// Initialize our IOleInPlaceFrame object with a pointer to our IOleInPlaceFrame VTable.
	_iOleClientSiteEx->inplace.frame.frame.lpVtbl = &MyIOleInPlaceFrameTable;

	// Save our HWND (in the IOleInPlaceFrame object) so our IOleInPlaceFrame functions can retrieve it.
	_iOleClientSiteEx->inplace.frame.window = hwnd;

	// Initialize our IDocHostUIHandler object with a pointer to our IDocHostUIHandler VTable.
	_iOleClientSiteEx->ui.ui.lpVtbl = &MyIDocHostUIHandlerTable;

	// Get a pointer to the browser object and lock it down (so it doesn't "disappear" while we're using
	// it in this program). We do this by calling the OS function OleCreate().
	//	
	// NOTE: We need this pointer to interact with and control the browser. With normal WIN32 controls such as a
	// Static, Edit, Combobox, etc, you obtain an HWND and send messages to it with SendMessage(). Not so with
	// the browser object. You need to get a pointer to its "base structure" (as returned by OleCreate()). This
	// structure contains an array of pointers to functions you can call within the browser object. Actually, the
	// base structure contains a 'lpVtbl' field that is a pointer to that array. We'll call the array a 'VTable'.
	//
	// For example, the browser object happens to have a SetHostNames() function we want to call. So, after we
	// retrieve the pointer to the browser object (in a local we'll name 'browserObject'), then we can call that
	// function, and pass it args, as so:
	//
	// browserObject->lpVtbl->SetHostNames(browserObject, SomeString, SomeString);
	//
	// There's our pointer to the browser object in 'browserObject'. And there's the pointer to the browser object's
	// VTable in 'browserObject->lpVtbl'. And the pointer to the SetHostNames function happens to be stored in an
	// field named 'SetHostNames' within the VTable. So we are actually indirectly calling SetHostNames by using
	// a pointer to it. That's how you use a VTable.
	//
	// NOTE: We pass our _IOleClientSiteEx struct and lie -- saying that it's a IOleClientSite. It's ok. A
	// _IOleClientSiteEx struct starts with an embedded IOleClientSite. So the browser won't care, and we want
	// this extended struct passed to our IOleClientSite functions.

	// Get a pointer to the browser object's IClassFactory object via CoGetClassObject()
	pClassFactory = 0;
	if (!CoGetClassObject(&CLSID_WebBrowser, 3, NULL, &IID_IClassFactory, (void **)&pClassFactory) && pClassFactory)
	{
		// Call the IClassFactory's CreateInstance() to create a browser object
		if (!pClassFactory->lpVtbl->CreateInstance(pClassFactory, 0, &IID_IOleObject, &browserObject))
		{
			// Free the IClassFactory. We need it only to create a browser object instance
			pClassFactory->lpVtbl->Release(pClassFactory);

			// Ok, we now have the pointer to the browser object in 'browserObject'. Let's save this in the
			// memory block we allocated above, and then save the pointer to that whole thing in our window's
			// USERDATA field. That way, if we need multiple windows each hosting its own browser object, we can
			// call EmbedBrowserObject() for each one, and easily associate the appropriate browser object with
			// its matching window and its own objects containing per-window data.
			*((IOleObject **)ptr) = browserObject;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)ptr);

			// Give the browser a pointer to my IOleClientSite object
			if (!browserObject->lpVtbl->SetClientSite(browserObject, (IOleClientSite *)_iOleClientSiteEx))
			{
				// We can now call the browser object's SetHostNames function. SetHostNames lets the browser object know our
				// application's name and the name of the document in which we're embedding the browser. (Since we have no
				// document name, we'll pass a 0 for the latter). When the browser object is opened for editing, it displays
				// these names in its titlebar.
				//	
				// We are passing 3 args to SetHostNames. You'll note that the first arg to SetHostNames is the base
				// address of our browser control. This is something that you always have to remember when working in C
				// (as opposed to C++). When calling a VTable function, the first arg to that function must always be the
				// structure which contains the VTable. (In this case, that's the browser control itself). Why? That's
				// because that function is always assumed to be written in C++. And the first argument to any C++ function
				// must be its 'this' pointer (ie, the base address of its class, which in this case is our browser object
				// pointer). In C++, you don't have to pass this first arg, because the C++ compiler is smart enough to
				// produce an executable that always adds this first arg. In fact, the C++ compiler is smart enough to
				// know to fetch the function pointer from the VTable, so you don't even need to reference that. In other
				// words, the C++ equivalent code would be:
				//
				// browserObject->SetHostNames(L"My Host Name", 0);
				//
				// So, when you're trying to convert C++ code to C, always remember to add this first arg whenever you're
				// dealing with a VTable (ie, the field is usually named 'lpVtbl') in the standard objects, and also add
				// the reference to the VTable itself.
				//
				// Oh yeah, the L is because we need UNICODE strings. And BTW, the host and document names can be anything
				// you want.
				browserObject->lpVtbl->SetHostNames(browserObject, L"My Host Name", 0);

				GetClientRect(hwnd, &rect);

				// Let browser object know that it is embedded in an OLE container.
				if (!OleSetContainedObject((struct IUnknown *)browserObject, TRUE) &&

					// Set the display area of our browser control the same as our window's size
					// and actually put the browser object into our window.
					!browserObject->lpVtbl->DoVerb(browserObject, OLEIVERB_SHOW, NULL, (IOleClientSite *)_iOleClientSiteEx, -1, hwnd, &rect) &&

					// Ok, now things may seem to get even trickier, One of those function pointers in the browser's VTable is
					// to the QueryInterface() function. What does this function do? It lets us grab the base address of any
					// other object that may be embedded within the browser object. And this other object has its own VTable
					// containing pointers to more functions we can call for that object.
					//
					// We want to get the base address (ie, a pointer) to the IWebBrowser2 object embedded within the browser
					// object, so we can call some of the functions in the former's table. For example, one IWebBrowser2 function
					// we intend to call below will be Navigate2(). So we call the browser object's QueryInterface to get our
					// pointer to the IWebBrowser2 object.
					!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))
				{
					// Ok, now the pointer to our IWebBrowser2 object is in 'webBrowser2', and so its VTable is
					// webBrowser2->lpVtbl.

					// Let's call several functions in the IWebBrowser2 object to position the browser display area
					// in our window. The functions we call are put_Left(), put_Top(), put_Width(), and put_Height().
					// Note that we reference the IWebBrowser2 object's VTable to get pointers to those functions. And
					// also note that the first arg we pass to each is the pointer to the IWebBrowser2 object.
					webBrowser2->lpVtbl->put_Left(webBrowser2, 0);
					webBrowser2->lpVtbl->put_Top(webBrowser2, 0);
					webBrowser2->lpVtbl->put_Width(webBrowser2, rect.right);
					webBrowser2->lpVtbl->put_Height(webBrowser2, rect.bottom);

					// We no longer need the IWebBrowser2 object (ie, we don't plan to call any more functions in it
					// right now, so we can release our hold on it). Note that we'll still maintain our hold on the
					// browser object until we're done with that object.
					webBrowser2->lpVtbl->Release(webBrowser2);

					// Success
					return(0);
				}
			}

			// Something went wrong setting up the browser!
			UnEmbedBrowserObject(hwnd);
			return(-4);
		}

		pClassFactory->lpVtbl->Release(pClassFactory);
		GlobalFree(ptr);

		// Can't create an instance of the browser!
		return(-3);
	}

	GlobalFree(ptr);

	// Can't get the web browser's IClassFactory!
	return(-2);
}






/***************************** CreateMyFont() **************************
 * Creates a font that is the same as what is used for status bars, and
 * stores it in 'FontHandle'.
 */
/*
void CreateMyFont(void)
{
	NONCLIENTMETRICS	nonClientMetrics;

	nonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &nonClientMetrics, 0);

	FontHandle = CreateFontIndirect(&nonClientMetrics.lfStatusFont);
}
*/




/****************************** WindowProc() ***************************
 * Our message handler for our Main window.
 */





LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	
	switch (uMsg)
	{
		case WM_SIZE:
		{
			// Resize the browser child window, leaving room for our buttons.
			MoveWindow(GetDlgItem(hwnd, 1000), 0, 40, LOWORD(lParam), HIWORD(lParam) - 40, TRUE);
			return(0);
		}

		case WM_COMMAND:
		{
			HWND browserHwnd = GetDlgItem(hwnd, 1000);

			switch (LOWORD(wParam))
			{
				// -------------------- Back button
				case 1001:
				{
					//DoPageAction(browserHwnd, WEBPAGE_GOBACK);
					DisplayHTMLPage(browser_hwnd, "res://embedbrowser.exe/test1.mht");
					break;
				}

				// -------------------- Forward button
				case 1002:
				{
					//DoPageAction(browserHwnd, WEBPAGE_GOFORWARD);
					DisplayHTMLPage(browser_hwnd, "res://embedbrowser.exe/test2.mht");
					break;
				}

				// -------------------- Stop button
				case 1003:
				{
					//DoPageAction(browserHwnd, WEBPAGE_STOP);
					DisplayHTMLPage(browser_hwnd, "res://embedbrowser.exe/test3.mht");
					break;
				}
			}

			break;
		}

		case WM_CREATE:
		{
			// Success
			return(0);
		}

		case WM_ERASEBKGND:
		{
			HBRUSH	hBrush;
			HGDIOBJ	orig;
			RECT	rec;

			// Erase the background behind the buttons. Note: The browser object takes care
			// of completely redrawing its area, so we don't need to erase there.
			hBrush = GetStockObject(WHITE_BRUSH);
			GetClientRect(hwnd, &rec);
			orig = SelectObject((HDC)wParam, hBrush);
			Rectangle((HDC)wParam, 0, 0, rec.right, 40);
			SelectObject((HDC)wParam, orig);
			return(TRUE);
		}

		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO		lpmmi;
			NONCLIENTMETRICS	nonClientMetrics;

			lpmmi = (LPMINMAXINFO)lParam;
			nonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &nonClientMetrics, 0);

			// Force height to be at least 80 + size of titlebar (if we had a menubar,
			// we would also add in iMenuHeight)

			// DISABLED!!! BUG ON WINDOWS XP
			//lpmmi->ptMinTrackSize.y = nonClientMetrics.iCaptionHeight + (nonClientMetrics.iBorderWidth << 1) + 80;

			return(0);
		}

		case WM_CLOSE:
		{
			// Close this window. This will also cause the child window hosting the browser
			// control to receive its WM_DESTROY
			DestroyWindow(hwnd);

			return(1);
		}

		case WM_DESTROY:
		{
 			// Post the WM_QUIT message to quit the message loop in WinMain()
			PostQuitMessage(0);

			return(1);
		}
	}

	return(DefWindowProc(hwnd, uMsg, wParam, lParam));
}





// Here is our array of HTML strings. Each one ends with a nul character.
// The entire array ends with an extra nul character. The first string is
// #1, the second is #2, etc.
//const TCHAR HtmlStrings[] = {"<h1>WINAPI EMBED IE</h1>"};



/************************** browserWindowProc() *************************
 * Our message handler for our window to host the browser.
 */

LRESULT CALLBACK browserWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_SIZE:
		{
			// Resize the browser object to fit the window
			ResizeBrowser(hwnd, LOWORD(lParam), HIWORD(lParam));
			//ResizeBrowser(hwnd, 100, 100);
			return(0);
		}

		case WM_APP:
		{
			//LPCTSTR		str;

			// Our IDocHostUIHandler's TranslateUrl() must have posted us this message
			// to tell us to display some HTML string. The string number is wParam.

			// Find the desired string
			/*str = &HtmlStrings[0];
			while (--wParam)
			{
				if (!(*str)) return(0);
				while (*(str)++);
			}
			*/
			// Display it.
			//DisplayHTMLStr(hwnd, str);

			return(0);
		}

		case WM_CREATE:
		{
			// Embed the browser object into our host window. We need do this only
			// once. Note that the browser object will start calling some of our
			// IOleInPlaceFrame and IOleClientSite functions as soon as we start
			// calling browser object functions in EmbedBrowserObject().
			if (EmbedBrowserObject(hwnd)) return(-1);

			// Success
			return(0);
		}

		case WM_DESTROY:
		{
			// Detach the browser object from this window, and free resources.
			UnEmbedBrowserObject(hwnd);

			return(TRUE);
		}
	}

	return(DefWindowProc(hwnd, uMsg, wParam, lParam));
}





/****************************** WinMain() ***************************
 * C program entry point.
 *
 * This creates a window to host the web browser, and displays a web
 * page.
 */

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hInstNULL, LPSTR lpszCmdLine, int nCmdShow)
{
	MSG msg;
	INITCOMMONCONTROLSEX iccex;

	char buf[500];

	GetModuleFileNameA(NULL, buf, 500);

	//printf("%s\n", PathFindFileName(buf));

	iccex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	//InitCommonControls();			// obsolete
	InitCommonControlsEx(&iccex);
	

	// Initialize the OLE interface. We do this once-only.
	if (OleInitialize(NULL) == S_OK)
	{
		WNDCLASSEX wc, wc2;

		// Register the class of our Main window. 'windowProc' is our message handler
		// and 'ClassName' is the class name. You can choose any class name you want.
		ZeroMemory(&wc, sizeof(WNDCLASSEX));
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.hInstance = hInstance;
		wc.lpfnWndProc = windowProc;
		wc.lpszClassName = &ClassName[0];
		wc.style = CS_CLASSDC|CS_HREDRAW|CS_VREDRAW|CS_PARENTDC|CS_BYTEALIGNCLIENT|CS_DBLCLKS;
		RegisterClassEx(&wc);

		// Register the class of our window to host the browser. 'browserWindowProc' is our message handler
		// and 'BrowserClassName' is the class name. You can choose any class name you want.
		ZeroMemory(&wc2, sizeof(WNDCLASSEX));
		wc2.cbSize = sizeof(WNDCLASSEX);
		wc2.hInstance = hInstance;
		wc2.lpfnWndProc = browserWindowProc;
		wc2.lpszClassName = &BrowserClassName[0];
		wc2.style = CS_HREDRAW|CS_VREDRAW;
		RegisterClassEx(&wc2);

		// Create a font to be used with our buttons
		//CreateMyFont();

		hfont_custom = CreateFontA(15, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, 
				OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
				DEFAULT_PITCH | FF_DONTCARE, "Comic Sans MS");
	


		// Create a Main window.
		if ((MainWindow = CreateWindowEx(0, &ClassName[0], "A web browser", WS_OVERLAPPEDWINDOW,
							CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
							HWND_DESKTOP, NULL, hInstance, 0)))
		{
			// Create a "Back" button.
			if (!(back_btn = CreateWindow("BUTTON", "Page 1",
					WS_TABSTOP|BS_PUSHBUTTON|BS_TEXT|WS_CHILD|BS_VCENTER|WS_VISIBLE,
					2, 2,				// X, Y position
					70, 34,				// Width, Height
					MainWindow,
					(HMENU)1001, hInstance, 0)))
			{
				goto bad;
			}


			// Create a "Forward" button.
			if (!(forward_btn = CreateWindow("BUTTON", "Page 2",
					WS_TABSTOP|BS_PUSHBUTTON|BS_TEXT|WS_CHILD|BS_VCENTER|WS_VISIBLE,
					2 + 70 + 10, 2,		// X, Y position
					70, 34,				// Width, Height
					MainWindow,
					(HMENU)1002, hInstance, 0)))
			{
				goto bad;
			}


			// Create a "Stop" button.
			if (!(stop_btn = CreateWindow("BUTTON", "Page 3",
					WS_TABSTOP|BS_PUSHBUTTON|BS_TEXT|WS_CHILD|BS_VCENTER|WS_VISIBLE,
					2 + (70<<1) + (10<<1), 2,		// X, Y position
					70, 34,							// Width, Height
					MainWindow,
					(HMENU)1003, hInstance, 0)))
			{
				goto bad;
			}

			// Set its font
			SendMessageA(back_btn, WM_SETFONT, (WPARAM) hfont_custom, TRUE);
			SendMessageA(forward_btn, WM_SETFONT, (WPARAM) hfont_custom, TRUE);
			SendMessageA(stop_btn, WM_SETFONT, (WPARAM) hfont_custom, TRUE);


			// Create a child window to host the browser object. NOTE: We embed the browser object
			// duing our WM_CREATE handling for this child window.
			if ((browser_hwnd = CreateWindowEx(0, &BrowserClassName[0], 0, WS_CHILD|WS_VISIBLE,
								0, 40, 0, 0,
								MainWindow, (HMENU)1000, hInstance, 0)))
			{
				// For this window, display an HTML string with our special app: link inside it
				DisplayHTMLStr(browser_hwnd, "<h1>Hello World</h1>");
				//DisplayHTMLPage(browser_hwnd, "file:///D:/test.html");
				//DisplayHTMLPage(browser_hwnd, "res://embedbrowser.exe/test1.mht");

				
				// Show the Main window.
				ShowWindow(MainWindow, nCmdShow);
				UpdateWindow(MainWindow);

				// Do a message loop until WM_QUIT.
				while (GetMessage(&msg, 0, 0, 0) == 1)
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			else
			{
				MessageBox(MainWindow, "Can't create browser child window!", "ERROR", MB_OK);
bad:			DestroyWindow(MainWindow);
			}
		}

		// Delete our font
		//if (FontHandle) DeleteObject(FontHandle);

		// Free the OLE library.
		OleUninitialize();

		return(0);
	}

	MessageBox(0, "Can't open OLE!", "ERROR", MB_OK);


	return(-1);
}

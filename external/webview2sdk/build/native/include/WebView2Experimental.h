

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.xx.xxxx */
/* at a redacted point in time
 */
/* Compiler settings for ../../edge_embedded_browser/client/win/current/WebView2Experimental.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.xx.xxxx 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __WebView2Experimental_h__
#define __WebView2Experimental_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if defined(_CONTROL_FLOW_GUARD_XFG)
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

#ifndef __ICoreWebView2Experimental20_FWD_DEFINED__
#define __ICoreWebView2Experimental20_FWD_DEFINED__
typedef interface ICoreWebView2Experimental20 ICoreWebView2Experimental20;

#endif 	/* __ICoreWebView2Experimental20_FWD_DEFINED__ */


#ifndef __ICoreWebView2Experimental29_FWD_DEFINED__
#define __ICoreWebView2Experimental29_FWD_DEFINED__
typedef interface ICoreWebView2Experimental29 ICoreWebView2Experimental29;

#endif 	/* __ICoreWebView2Experimental29_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalCompositionController4_FWD_DEFINED__
#define __ICoreWebView2ExperimentalCompositionController4_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalCompositionController4 ICoreWebView2ExperimentalCompositionController4;

#endif 	/* __ICoreWebView2ExperimentalCompositionController4_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalCompositionController6_FWD_DEFINED__
#define __ICoreWebView2ExperimentalCompositionController6_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalCompositionController6 ICoreWebView2ExperimentalCompositionController6;

#endif 	/* __ICoreWebView2ExperimentalCompositionController6_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalDragStartingEventHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalDragStartingEventHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalDragStartingEventHandler ICoreWebView2ExperimentalDragStartingEventHandler;

#endif 	/* __ICoreWebView2ExperimentalDragStartingEventHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalControllerOptions2_FWD_DEFINED__
#define __ICoreWebView2ExperimentalControllerOptions2_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalControllerOptions2 ICoreWebView2ExperimentalControllerOptions2;

#endif 	/* __ICoreWebView2ExperimentalControllerOptions2_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalDragStartingEventArgs_FWD_DEFINED__
#define __ICoreWebView2ExperimentalDragStartingEventArgs_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalDragStartingEventArgs ICoreWebView2ExperimentalDragStartingEventArgs;

#endif 	/* __ICoreWebView2ExperimentalDragStartingEventArgs_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment3_FWD_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment3_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalEnvironment3 ICoreWebView2ExperimentalEnvironment3;

#endif 	/* __ICoreWebView2ExperimentalEnvironment3_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler;

#endif 	/* __ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment12_FWD_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment12_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalEnvironment12 ICoreWebView2ExperimentalEnvironment12;

#endif 	/* __ICoreWebView2ExperimentalEnvironment12_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler;

#endif 	/* __ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment15_FWD_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment15_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalEnvironment15 ICoreWebView2ExperimentalEnvironment15;

#endif 	/* __ICoreWebView2ExperimentalEnvironment15_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalRestartRequestedEventHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalRestartRequestedEventHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalRestartRequestedEventHandler ICoreWebView2ExperimentalRestartRequestedEventHandler;

#endif 	/* __ICoreWebView2ExperimentalRestartRequestedEventHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment18_FWD_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment18_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalEnvironment18 ICoreWebView2ExperimentalEnvironment18;

#endif 	/* __ICoreWebView2ExperimentalEnvironment18_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFind_FWD_DEFINED__
#define __ICoreWebView2ExperimentalFind_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalFind ICoreWebView2ExperimentalFind;

#endif 	/* __ICoreWebView2ExperimentalFind_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler;

#endif 	/* __ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFindMatchCountChangedEventHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalFindMatchCountChangedEventHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalFindMatchCountChangedEventHandler ICoreWebView2ExperimentalFindMatchCountChangedEventHandler;

#endif 	/* __ICoreWebView2ExperimentalFindMatchCountChangedEventHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFindStartCompletedHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalFindStartCompletedHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalFindStartCompletedHandler ICoreWebView2ExperimentalFindStartCompletedHandler;

#endif 	/* __ICoreWebView2ExperimentalFindStartCompletedHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFindOptions_FWD_DEFINED__
#define __ICoreWebView2ExperimentalFindOptions_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalFindOptions ICoreWebView2ExperimentalFindOptions;

#endif 	/* __ICoreWebView2ExperimentalFindOptions_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFrame7_FWD_DEFINED__
#define __ICoreWebView2ExperimentalFrame7_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalFrame7 ICoreWebView2ExperimentalFrame7;

#endif 	/* __ICoreWebView2ExperimentalFrame7_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalProfile7_FWD_DEFINED__
#define __ICoreWebView2ExperimentalProfile7_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalProfile7 ICoreWebView2ExperimentalProfile7;

#endif 	/* __ICoreWebView2ExperimentalProfile7_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler;

#endif 	/* __ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalRestartRequestedEventArgs_FWD_DEFINED__
#define __ICoreWebView2ExperimentalRestartRequestedEventArgs_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalRestartRequestedEventArgs ICoreWebView2ExperimentalRestartRequestedEventArgs;

#endif 	/* __ICoreWebView2ExperimentalRestartRequestedEventArgs_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalSettings9_FWD_DEFINED__
#define __ICoreWebView2ExperimentalSettings9_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalSettings9 ICoreWebView2ExperimentalSettings9;

#endif 	/* __ICoreWebView2ExperimentalSettings9_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTexture_FWD_DEFINED__
#define __ICoreWebView2ExperimentalTexture_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalTexture ICoreWebView2ExperimentalTexture;

#endif 	/* __ICoreWebView2ExperimentalTexture_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStream_FWD_DEFINED__
#define __ICoreWebView2ExperimentalTextureStream_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalTextureStream ICoreWebView2ExperimentalTextureStream;

#endif 	/* __ICoreWebView2ExperimentalTextureStream_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler;

#endif 	/* __ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler;

#endif 	/* __ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamStoppedEventHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamStoppedEventHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalTextureStreamStoppedEventHandler ICoreWebView2ExperimentalTextureStreamStoppedEventHandler;

#endif 	/* __ICoreWebView2ExperimentalTextureStreamStoppedEventHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler;

#endif 	/* __ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler;

#endif 	/* __ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs_FWD_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs;

#endif 	/* __ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs_FWD_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs;

#endif 	/* __ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalUpdateRuntimeResult_FWD_DEFINED__
#define __ICoreWebView2ExperimentalUpdateRuntimeResult_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalUpdateRuntimeResult ICoreWebView2ExperimentalUpdateRuntimeResult;

#endif 	/* __ICoreWebView2ExperimentalUpdateRuntimeResult_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalWebTexture_FWD_DEFINED__
#define __ICoreWebView2ExperimentalWebTexture_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalWebTexture ICoreWebView2ExperimentalWebTexture;

#endif 	/* __ICoreWebView2ExperimentalWebTexture_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalCompositionControllerInterop3_FWD_DEFINED__
#define __ICoreWebView2ExperimentalCompositionControllerInterop3_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalCompositionControllerInterop3 ICoreWebView2ExperimentalCompositionControllerInterop3;

#endif 	/* __ICoreWebView2ExperimentalCompositionControllerInterop3_FWD_DEFINED__ */


/* header files for imported files */
#include "WebView2.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __WebView2Experimental_LIBRARY_DEFINED__
#define __WebView2Experimental_LIBRARY_DEFINED__

/* library WebView2Experimental */
/* [version][uuid] */ 




































typedef /* [v1_enum] */ 
enum COREWEBVIEW2_RESTART_REQUESTED_PRIORITY
    {
        COREWEBVIEW2_RESTART_REQUESTED_PRIORITY_NORMAL	= 1000,
        COREWEBVIEW2_RESTART_REQUESTED_PRIORITY_HIGH	= 2000
    } 	COREWEBVIEW2_RESTART_REQUESTED_PRIORITY;

typedef /* [v1_enum] */ 
enum COREWEBVIEW2_TEXTURE_STREAM_ERROR_KIND
    {
        COREWEBVIEW2_TEXTURE_STREAM_ERROR_KIND_NO_VIDEO_TRACK_STARTED	= 0,
        COREWEBVIEW2_TEXTURE_STREAM_ERROR_KIND_TEXTURE_ERROR	= ( COREWEBVIEW2_TEXTURE_STREAM_ERROR_KIND_NO_VIDEO_TRACK_STARTED + 1 ) ,
        COREWEBVIEW2_TEXTURE_STREAM_ERROR_KIND_TEXTURE_IN_USE	= ( COREWEBVIEW2_TEXTURE_STREAM_ERROR_KIND_TEXTURE_ERROR + 1 ) 
    } 	COREWEBVIEW2_TEXTURE_STREAM_ERROR_KIND;

typedef /* [v1_enum] */ 
enum COREWEBVIEW2_UPDATE_RUNTIME_STATUS
    {
        COREWEBVIEW2_UPDATE_RUNTIME_STATUS_LATEST_VERSION_INSTALLED	= 0,
        COREWEBVIEW2_UPDATE_RUNTIME_STATUS_UPDATE_ALREADY_RUNNING	= ( COREWEBVIEW2_UPDATE_RUNTIME_STATUS_LATEST_VERSION_INSTALLED + 1 ) ,
        COREWEBVIEW2_UPDATE_RUNTIME_STATUS_BLOCKED_BY_POLICY	= ( COREWEBVIEW2_UPDATE_RUNTIME_STATUS_UPDATE_ALREADY_RUNNING + 1 ) ,
        COREWEBVIEW2_UPDATE_RUNTIME_STATUS_FAILED	= ( COREWEBVIEW2_UPDATE_RUNTIME_STATUS_BLOCKED_BY_POLICY + 1 ) 
    } 	COREWEBVIEW2_UPDATE_RUNTIME_STATUS;

typedef struct COREWEBVIEW2_MATRIX_4X4
    {
    FLOAT _11;
    FLOAT _12;
    FLOAT _13;
    FLOAT _14;
    FLOAT _21;
    FLOAT _22;
    FLOAT _23;
    FLOAT _24;
    FLOAT _31;
    FLOAT _32;
    FLOAT _33;
    FLOAT _34;
    FLOAT _41;
    FLOAT _42;
    FLOAT _43;
    FLOAT _44;
    } 	COREWEBVIEW2_MATRIX_4X4;


EXTERN_C const IID LIBID_WebView2Experimental;

#ifndef __ICoreWebView2Experimental20_INTERFACE_DEFINED__
#define __ICoreWebView2Experimental20_INTERFACE_DEFINED__

/* interface ICoreWebView2Experimental20 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2Experimental20 = {0x5a4d0ecf,0x3fe5,0x4456,{0xac,0xe5,0xd3,0x17,0xcc,0xa0,0xef,0xf1}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5a4d0ecf-3fe5-4456-ace5-d317cca0eff1")
    ICoreWebView2Experimental20 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CustomDataPartitionId( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CustomDataPartitionId( 
            /* [in] */ LPCWSTR value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2Experimental20Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2Experimental20 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2Experimental20 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2Experimental20 * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2Experimental20, get_CustomDataPartitionId)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CustomDataPartitionId )( 
            ICoreWebView2Experimental20 * This,
            /* [retval][out] */ LPWSTR *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2Experimental20, put_CustomDataPartitionId)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CustomDataPartitionId )( 
            ICoreWebView2Experimental20 * This,
            /* [in] */ LPCWSTR value);
        
        END_INTERFACE
    } ICoreWebView2Experimental20Vtbl;

    interface ICoreWebView2Experimental20
    {
        CONST_VTBL struct ICoreWebView2Experimental20Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2Experimental20_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2Experimental20_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2Experimental20_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2Experimental20_get_CustomDataPartitionId(This,value)	\
    ( (This)->lpVtbl -> get_CustomDataPartitionId(This,value) ) 

#define ICoreWebView2Experimental20_put_CustomDataPartitionId(This,value)	\
    ( (This)->lpVtbl -> put_CustomDataPartitionId(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2Experimental20_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2Experimental29_INTERFACE_DEFINED__
#define __ICoreWebView2Experimental29_INTERFACE_DEFINED__

/* interface ICoreWebView2Experimental29 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2Experimental29 = {0x71af61f3,0xd975,0x52e7,{0xa5,0x44,0x37,0xdc,0xbf,0x3c,0x23,0x7b}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("71af61f3-d975-52e7-a544-37dcbf3c237b")
    ICoreWebView2Experimental29 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Find( 
            /* [retval][out] */ ICoreWebView2ExperimentalFind **value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2Experimental29Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2Experimental29 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2Experimental29 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2Experimental29 * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2Experimental29, get_Find)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Find )( 
            ICoreWebView2Experimental29 * This,
            /* [retval][out] */ ICoreWebView2ExperimentalFind **value);
        
        END_INTERFACE
    } ICoreWebView2Experimental29Vtbl;

    interface ICoreWebView2Experimental29
    {
        CONST_VTBL struct ICoreWebView2Experimental29Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2Experimental29_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2Experimental29_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2Experimental29_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2Experimental29_get_Find(This,value)	\
    ( (This)->lpVtbl -> get_Find(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2Experimental29_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalCompositionController4_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalCompositionController4_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalCompositionController4 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalCompositionController4 = {0xe6041d7f,0x18ac,0x4654,{0xa0,0x4e,0x8b,0x3f,0x81,0x25,0x1c,0x33}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e6041d7f-18ac-4654-a04e-8b3f81251c33")
    ICoreWebView2ExperimentalCompositionController4 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AutomationProvider( 
            /* [retval][out] */ IUnknown **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateCoreWebView2PointerInfoFromPointerId( 
            /* [in] */ UINT32 PointerId,
            /* [in] */ HWND ParentWindow,
            /* [in] */ COREWEBVIEW2_MATRIX_4X4 transform,
            /* [retval][out] */ ICoreWebView2PointerInfo **value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalCompositionController4Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalCompositionController4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalCompositionController4 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalCompositionController4 * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalCompositionController4, get_AutomationProvider)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_AutomationProvider )( 
            ICoreWebView2ExperimentalCompositionController4 * This,
            /* [retval][out] */ IUnknown **value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalCompositionController4, CreateCoreWebView2PointerInfoFromPointerId)
        HRESULT ( STDMETHODCALLTYPE *CreateCoreWebView2PointerInfoFromPointerId )( 
            ICoreWebView2ExperimentalCompositionController4 * This,
            /* [in] */ UINT32 PointerId,
            /* [in] */ HWND ParentWindow,
            /* [in] */ COREWEBVIEW2_MATRIX_4X4 transform,
            /* [retval][out] */ ICoreWebView2PointerInfo **value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalCompositionController4Vtbl;

    interface ICoreWebView2ExperimentalCompositionController4
    {
        CONST_VTBL struct ICoreWebView2ExperimentalCompositionController4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalCompositionController4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalCompositionController4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalCompositionController4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalCompositionController4_get_AutomationProvider(This,value)	\
    ( (This)->lpVtbl -> get_AutomationProvider(This,value) ) 

#define ICoreWebView2ExperimentalCompositionController4_CreateCoreWebView2PointerInfoFromPointerId(This,PointerId,ParentWindow,transform,value)	\
    ( (This)->lpVtbl -> CreateCoreWebView2PointerInfoFromPointerId(This,PointerId,ParentWindow,transform,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalCompositionController4_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalCompositionController6_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalCompositionController6_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalCompositionController6 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalCompositionController6 = {0x975d6824,0x6a02,0x5e98,{0xab,0x7c,0xe4,0x67,0x9d,0x53,0x57,0xf4}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("975d6824-6a02-5e98-ab7c-e4679d5357f4")
    ICoreWebView2ExperimentalCompositionController6 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE add_DragStarting( 
            /* [in] */ ICoreWebView2ExperimentalDragStartingEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_DragStarting( 
            /* [in] */ EventRegistrationToken token) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalCompositionController6Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalCompositionController6 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalCompositionController6 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalCompositionController6 * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalCompositionController6, add_DragStarting)
        HRESULT ( STDMETHODCALLTYPE *add_DragStarting )( 
            ICoreWebView2ExperimentalCompositionController6 * This,
            /* [in] */ ICoreWebView2ExperimentalDragStartingEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalCompositionController6, remove_DragStarting)
        HRESULT ( STDMETHODCALLTYPE *remove_DragStarting )( 
            ICoreWebView2ExperimentalCompositionController6 * This,
            /* [in] */ EventRegistrationToken token);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalCompositionController6Vtbl;

    interface ICoreWebView2ExperimentalCompositionController6
    {
        CONST_VTBL struct ICoreWebView2ExperimentalCompositionController6Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalCompositionController6_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalCompositionController6_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalCompositionController6_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalCompositionController6_add_DragStarting(This,eventHandler,token)	\
    ( (This)->lpVtbl -> add_DragStarting(This,eventHandler,token) ) 

#define ICoreWebView2ExperimentalCompositionController6_remove_DragStarting(This,token)	\
    ( (This)->lpVtbl -> remove_DragStarting(This,token) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalCompositionController6_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalDragStartingEventHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalDragStartingEventHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalDragStartingEventHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalDragStartingEventHandler = {0x3b149321,0x83c3,0x5d1f,{0xb0,0x3f,0xa4,0x28,0x99,0xbc,0x1c,0x15}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3b149321-83c3-5d1f-b03f-a42899bc1c15")
    ICoreWebView2ExperimentalDragStartingEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ ICoreWebView2CompositionController *sender,
            /* [in] */ ICoreWebView2ExperimentalDragStartingEventArgs *args) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalDragStartingEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalDragStartingEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalDragStartingEventHandler * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalDragStartingEventHandler * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalDragStartingEventHandler, Invoke)
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalDragStartingEventHandler * This,
            /* [in] */ ICoreWebView2CompositionController *sender,
            /* [in] */ ICoreWebView2ExperimentalDragStartingEventArgs *args);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalDragStartingEventHandlerVtbl;

    interface ICoreWebView2ExperimentalDragStartingEventHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalDragStartingEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalDragStartingEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalDragStartingEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalDragStartingEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalDragStartingEventHandler_Invoke(This,sender,args)	\
    ( (This)->lpVtbl -> Invoke(This,sender,args) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalDragStartingEventHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalControllerOptions2_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalControllerOptions2_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalControllerOptions2 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalControllerOptions2 = {0x04c3310b,0x06c0,0x4861,{0xa7,0xaf,0x5c,0x7e,0x78,0xe0,0x6f,0x88}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("04c3310b-06c0-4861-a7af-5c7e78e06f88")
    ICoreWebView2ExperimentalControllerOptions2 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AllowHostInputProcessing( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_AllowHostInputProcessing( 
            /* [in] */ BOOL value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalControllerOptions2Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalControllerOptions2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalControllerOptions2 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalControllerOptions2 * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalControllerOptions2, get_AllowHostInputProcessing)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowHostInputProcessing )( 
            ICoreWebView2ExperimentalControllerOptions2 * This,
            /* [retval][out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalControllerOptions2, put_AllowHostInputProcessing)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowHostInputProcessing )( 
            ICoreWebView2ExperimentalControllerOptions2 * This,
            /* [in] */ BOOL value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalControllerOptions2Vtbl;

    interface ICoreWebView2ExperimentalControllerOptions2
    {
        CONST_VTBL struct ICoreWebView2ExperimentalControllerOptions2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalControllerOptions2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalControllerOptions2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalControllerOptions2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalControllerOptions2_get_AllowHostInputProcessing(This,value)	\
    ( (This)->lpVtbl -> get_AllowHostInputProcessing(This,value) ) 

#define ICoreWebView2ExperimentalControllerOptions2_put_AllowHostInputProcessing(This,value)	\
    ( (This)->lpVtbl -> put_AllowHostInputProcessing(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalControllerOptions2_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalDragStartingEventArgs_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalDragStartingEventArgs_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalDragStartingEventArgs */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalDragStartingEventArgs = {0x5e70e0e8,0x5cd0,0x5cfa,{0x8f,0x6f,0xd9,0x1b,0x48,0xa3,0x75,0x5c}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5e70e0e8-5cd0-5cfa-8f6f-d91b48a3755c")
    ICoreWebView2ExperimentalDragStartingEventArgs : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AllowedDropEffects( 
            /* [retval][out] */ DWORD *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Data( 
            /* [retval][out] */ IDataObject **value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Handled( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Handled( 
            /* [in] */ BOOL value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Position( 
            /* [retval][out] */ POINT *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDeferral( 
            /* [retval][out] */ ICoreWebView2Deferral **value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalDragStartingEventArgsVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalDragStartingEventArgs * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalDragStartingEventArgs * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalDragStartingEventArgs * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalDragStartingEventArgs, get_AllowedDropEffects)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowedDropEffects )( 
            ICoreWebView2ExperimentalDragStartingEventArgs * This,
            /* [retval][out] */ DWORD *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalDragStartingEventArgs, get_Data)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Data )( 
            ICoreWebView2ExperimentalDragStartingEventArgs * This,
            /* [retval][out] */ IDataObject **value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalDragStartingEventArgs, get_Handled)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Handled )( 
            ICoreWebView2ExperimentalDragStartingEventArgs * This,
            /* [retval][out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalDragStartingEventArgs, put_Handled)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Handled )( 
            ICoreWebView2ExperimentalDragStartingEventArgs * This,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalDragStartingEventArgs, get_Position)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Position )( 
            ICoreWebView2ExperimentalDragStartingEventArgs * This,
            /* [retval][out] */ POINT *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalDragStartingEventArgs, GetDeferral)
        HRESULT ( STDMETHODCALLTYPE *GetDeferral )( 
            ICoreWebView2ExperimentalDragStartingEventArgs * This,
            /* [retval][out] */ ICoreWebView2Deferral **value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalDragStartingEventArgsVtbl;

    interface ICoreWebView2ExperimentalDragStartingEventArgs
    {
        CONST_VTBL struct ICoreWebView2ExperimentalDragStartingEventArgsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalDragStartingEventArgs_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalDragStartingEventArgs_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalDragStartingEventArgs_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalDragStartingEventArgs_get_AllowedDropEffects(This,value)	\
    ( (This)->lpVtbl -> get_AllowedDropEffects(This,value) ) 

#define ICoreWebView2ExperimentalDragStartingEventArgs_get_Data(This,value)	\
    ( (This)->lpVtbl -> get_Data(This,value) ) 

#define ICoreWebView2ExperimentalDragStartingEventArgs_get_Handled(This,value)	\
    ( (This)->lpVtbl -> get_Handled(This,value) ) 

#define ICoreWebView2ExperimentalDragStartingEventArgs_put_Handled(This,value)	\
    ( (This)->lpVtbl -> put_Handled(This,value) ) 

#define ICoreWebView2ExperimentalDragStartingEventArgs_get_Position(This,value)	\
    ( (This)->lpVtbl -> get_Position(This,value) ) 

#define ICoreWebView2ExperimentalDragStartingEventArgs_GetDeferral(This,value)	\
    ( (This)->lpVtbl -> GetDeferral(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalDragStartingEventArgs_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment3_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment3_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalEnvironment3 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalEnvironment3 = {0x9a2be885,0x7f0b,0x4b26,{0xb6,0xdd,0xc9,0x69,0xba,0xa0,0x0b,0xf1}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9a2be885-7f0b-4b26-b6dd-c969baa00bf1")
    ICoreWebView2ExperimentalEnvironment3 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE UpdateRuntime( 
            /* [in] */ ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler *handler) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalEnvironment3Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalEnvironment3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalEnvironment3 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalEnvironment3 * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalEnvironment3, UpdateRuntime)
        HRESULT ( STDMETHODCALLTYPE *UpdateRuntime )( 
            ICoreWebView2ExperimentalEnvironment3 * This,
            /* [in] */ ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler *handler);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalEnvironment3Vtbl;

    interface ICoreWebView2ExperimentalEnvironment3
    {
        CONST_VTBL struct ICoreWebView2ExperimentalEnvironment3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalEnvironment3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalEnvironment3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalEnvironment3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalEnvironment3_UpdateRuntime(This,handler)	\
    ( (This)->lpVtbl -> UpdateRuntime(This,handler) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalEnvironment3_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler = {0xf1d2d722,0x3721,0x499c,{0x87,0xf5,0x4c,0x40,0x52,0x60,0x69,0x7a}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f1d2d722-3721-499c-87f5-4c405260697a")
    ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ HRESULT errorCode,
            /* [in] */ ICoreWebView2ExperimentalUpdateRuntimeResult *result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalUpdateRuntimeCompletedHandlerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler, Invoke)
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler * This,
            /* [in] */ HRESULT errorCode,
            /* [in] */ ICoreWebView2ExperimentalUpdateRuntimeResult *result);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalUpdateRuntimeCompletedHandlerVtbl;

    interface ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalUpdateRuntimeCompletedHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_Invoke(This,errorCode,result)	\
    ( (This)->lpVtbl -> Invoke(This,errorCode,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment12_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment12_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalEnvironment12 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalEnvironment12 = {0x96c27a45,0xf142,0x4873,{0x80,0xad,0x9d,0x0c,0xd8,0x99,0xb2,0xb9}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96c27a45-f142-4873-80ad-9d0cd899b2b9")
    ICoreWebView2ExperimentalEnvironment12 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateTextureStream( 
            /* [in] */ LPCWSTR streamId,
            /* [in] */ IUnknown *d3dDevice,
            /* [retval][out] */ ICoreWebView2ExperimentalTextureStream **value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RenderAdapterLUID( 
            /* [retval][out] */ UINT64 *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE add_RenderAdapterLUIDChanged( 
            /* [in] */ ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_RenderAdapterLUIDChanged( 
            /* [in] */ EventRegistrationToken token) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalEnvironment12Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalEnvironment12 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalEnvironment12 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalEnvironment12 * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalEnvironment12, CreateTextureStream)
        HRESULT ( STDMETHODCALLTYPE *CreateTextureStream )( 
            ICoreWebView2ExperimentalEnvironment12 * This,
            /* [in] */ LPCWSTR streamId,
            /* [in] */ IUnknown *d3dDevice,
            /* [retval][out] */ ICoreWebView2ExperimentalTextureStream **value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalEnvironment12, get_RenderAdapterLUID)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RenderAdapterLUID )( 
            ICoreWebView2ExperimentalEnvironment12 * This,
            /* [retval][out] */ UINT64 *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalEnvironment12, add_RenderAdapterLUIDChanged)
        HRESULT ( STDMETHODCALLTYPE *add_RenderAdapterLUIDChanged )( 
            ICoreWebView2ExperimentalEnvironment12 * This,
            /* [in] */ ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalEnvironment12, remove_RenderAdapterLUIDChanged)
        HRESULT ( STDMETHODCALLTYPE *remove_RenderAdapterLUIDChanged )( 
            ICoreWebView2ExperimentalEnvironment12 * This,
            /* [in] */ EventRegistrationToken token);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalEnvironment12Vtbl;

    interface ICoreWebView2ExperimentalEnvironment12
    {
        CONST_VTBL struct ICoreWebView2ExperimentalEnvironment12Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalEnvironment12_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalEnvironment12_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalEnvironment12_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalEnvironment12_CreateTextureStream(This,streamId,d3dDevice,value)	\
    ( (This)->lpVtbl -> CreateTextureStream(This,streamId,d3dDevice,value) ) 

#define ICoreWebView2ExperimentalEnvironment12_get_RenderAdapterLUID(This,value)	\
    ( (This)->lpVtbl -> get_RenderAdapterLUID(This,value) ) 

#define ICoreWebView2ExperimentalEnvironment12_add_RenderAdapterLUIDChanged(This,eventHandler,token)	\
    ( (This)->lpVtbl -> add_RenderAdapterLUIDChanged(This,eventHandler,token) ) 

#define ICoreWebView2ExperimentalEnvironment12_remove_RenderAdapterLUIDChanged(This,token)	\
    ( (This)->lpVtbl -> remove_RenderAdapterLUIDChanged(This,token) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalEnvironment12_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler = {0x431721e0,0x0f18,0x4d7b,{0xbd,0x4d,0xe5,0xb1,0x52,0x2b,0xb1,0x10}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("431721e0-0f18-4d7b-bd4d-e5b1522bb110")
    ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ ICoreWebView2ExperimentalEnvironment12 *sender,
            /* [in] */ IUnknown *args) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler, Invoke)
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler * This,
            /* [in] */ ICoreWebView2ExperimentalEnvironment12 *sender,
            /* [in] */ IUnknown *args);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandlerVtbl;

    interface ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler_Invoke(This,sender,args)	\
    ( (This)->lpVtbl -> Invoke(This,sender,args) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalRenderAdapterLUIDChangedEventHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment15_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment15_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalEnvironment15 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalEnvironment15 = {0x74ce74ee,0xb71c,0x5015,{0x87,0x28,0x39,0xeb,0xf3,0xb5,0x9a,0x89}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("74ce74ee-b71c-5015-8728-39ebf3b59a89")
    ICoreWebView2ExperimentalEnvironment15 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE add_RestartRequested( 
            /* [in] */ ICoreWebView2ExperimentalRestartRequestedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_RestartRequested( 
            /* [in] */ EventRegistrationToken token) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalEnvironment15Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalEnvironment15 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalEnvironment15 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalEnvironment15 * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalEnvironment15, add_RestartRequested)
        HRESULT ( STDMETHODCALLTYPE *add_RestartRequested )( 
            ICoreWebView2ExperimentalEnvironment15 * This,
            /* [in] */ ICoreWebView2ExperimentalRestartRequestedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalEnvironment15, remove_RestartRequested)
        HRESULT ( STDMETHODCALLTYPE *remove_RestartRequested )( 
            ICoreWebView2ExperimentalEnvironment15 * This,
            /* [in] */ EventRegistrationToken token);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalEnvironment15Vtbl;

    interface ICoreWebView2ExperimentalEnvironment15
    {
        CONST_VTBL struct ICoreWebView2ExperimentalEnvironment15Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalEnvironment15_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalEnvironment15_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalEnvironment15_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalEnvironment15_add_RestartRequested(This,eventHandler,token)	\
    ( (This)->lpVtbl -> add_RestartRequested(This,eventHandler,token) ) 

#define ICoreWebView2ExperimentalEnvironment15_remove_RestartRequested(This,token)	\
    ( (This)->lpVtbl -> remove_RestartRequested(This,token) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalEnvironment15_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalRestartRequestedEventHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalRestartRequestedEventHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalRestartRequestedEventHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalRestartRequestedEventHandler = {0xa5d3e2ac,0xfee2,0x59c9,{0xbd,0x70,0x9e,0x68,0x18,0x1a,0x95,0xa1}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a5d3e2ac-fee2-59c9-bd70-9e68181a95a1")
    ICoreWebView2ExperimentalRestartRequestedEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ ICoreWebView2Environment *sender,
            /* [in] */ ICoreWebView2ExperimentalRestartRequestedEventArgs *args) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalRestartRequestedEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalRestartRequestedEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalRestartRequestedEventHandler * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalRestartRequestedEventHandler * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalRestartRequestedEventHandler, Invoke)
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalRestartRequestedEventHandler * This,
            /* [in] */ ICoreWebView2Environment *sender,
            /* [in] */ ICoreWebView2ExperimentalRestartRequestedEventArgs *args);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalRestartRequestedEventHandlerVtbl;

    interface ICoreWebView2ExperimentalRestartRequestedEventHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalRestartRequestedEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalRestartRequestedEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalRestartRequestedEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalRestartRequestedEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalRestartRequestedEventHandler_Invoke(This,sender,args)	\
    ( (This)->lpVtbl -> Invoke(This,sender,args) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalRestartRequestedEventHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment18_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment18_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalEnvironment18 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalEnvironment18 = {0x7918712d,0xd89c,0x5f11,{0x82,0xe9,0x29,0x27,0x7e,0x0c,0x5b,0x3d}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7918712d-d89c-5f11-82e9-29277e0c5b3d")
    ICoreWebView2ExperimentalEnvironment18 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateFindOptions( 
            /* [retval][out] */ ICoreWebView2ExperimentalFindOptions **value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalEnvironment18Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalEnvironment18 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalEnvironment18 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalEnvironment18 * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalEnvironment18, CreateFindOptions)
        HRESULT ( STDMETHODCALLTYPE *CreateFindOptions )( 
            ICoreWebView2ExperimentalEnvironment18 * This,
            /* [retval][out] */ ICoreWebView2ExperimentalFindOptions **value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalEnvironment18Vtbl;

    interface ICoreWebView2ExperimentalEnvironment18
    {
        CONST_VTBL struct ICoreWebView2ExperimentalEnvironment18Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalEnvironment18_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalEnvironment18_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalEnvironment18_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalEnvironment18_CreateFindOptions(This,value)	\
    ( (This)->lpVtbl -> CreateFindOptions(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalEnvironment18_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFind_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalFind_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalFind */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalFind = {0xa3ec0f5f,0xddbc,0x54ed,{0x85,0x46,0xaf,0x75,0xa7,0x85,0xb9,0xa6}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a3ec0f5f-ddbc-54ed-8546-af75a785b9a6")
    ICoreWebView2ExperimentalFind : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ActiveMatchIndex( 
            /* [retval][out] */ INT32 *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MatchCount( 
            /* [retval][out] */ INT32 *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE add_ActiveMatchIndexChanged( 
            /* [in] */ ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_ActiveMatchIndexChanged( 
            /* [in] */ EventRegistrationToken token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE add_MatchCountChanged( 
            /* [in] */ ICoreWebView2ExperimentalFindMatchCountChangedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_MatchCountChanged( 
            /* [in] */ EventRegistrationToken token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Start( 
            /* [in] */ ICoreWebView2ExperimentalFindOptions *options,
            /* [in] */ ICoreWebView2ExperimentalFindStartCompletedHandler *handler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindNext( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindPrevious( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalFindVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalFind * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalFind * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalFind * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFind, get_ActiveMatchIndex)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveMatchIndex )( 
            ICoreWebView2ExperimentalFind * This,
            /* [retval][out] */ INT32 *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFind, get_MatchCount)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MatchCount )( 
            ICoreWebView2ExperimentalFind * This,
            /* [retval][out] */ INT32 *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFind, add_ActiveMatchIndexChanged)
        HRESULT ( STDMETHODCALLTYPE *add_ActiveMatchIndexChanged )( 
            ICoreWebView2ExperimentalFind * This,
            /* [in] */ ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFind, remove_ActiveMatchIndexChanged)
        HRESULT ( STDMETHODCALLTYPE *remove_ActiveMatchIndexChanged )( 
            ICoreWebView2ExperimentalFind * This,
            /* [in] */ EventRegistrationToken token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFind, add_MatchCountChanged)
        HRESULT ( STDMETHODCALLTYPE *add_MatchCountChanged )( 
            ICoreWebView2ExperimentalFind * This,
            /* [in] */ ICoreWebView2ExperimentalFindMatchCountChangedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFind, remove_MatchCountChanged)
        HRESULT ( STDMETHODCALLTYPE *remove_MatchCountChanged )( 
            ICoreWebView2ExperimentalFind * This,
            /* [in] */ EventRegistrationToken token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFind, Start)
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            ICoreWebView2ExperimentalFind * This,
            /* [in] */ ICoreWebView2ExperimentalFindOptions *options,
            /* [in] */ ICoreWebView2ExperimentalFindStartCompletedHandler *handler);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFind, FindNext)
        HRESULT ( STDMETHODCALLTYPE *FindNext )( 
            ICoreWebView2ExperimentalFind * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFind, FindPrevious)
        HRESULT ( STDMETHODCALLTYPE *FindPrevious )( 
            ICoreWebView2ExperimentalFind * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFind, Stop)
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            ICoreWebView2ExperimentalFind * This);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalFindVtbl;

    interface ICoreWebView2ExperimentalFind
    {
        CONST_VTBL struct ICoreWebView2ExperimentalFindVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalFind_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalFind_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalFind_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalFind_get_ActiveMatchIndex(This,value)	\
    ( (This)->lpVtbl -> get_ActiveMatchIndex(This,value) ) 

#define ICoreWebView2ExperimentalFind_get_MatchCount(This,value)	\
    ( (This)->lpVtbl -> get_MatchCount(This,value) ) 

#define ICoreWebView2ExperimentalFind_add_ActiveMatchIndexChanged(This,eventHandler,token)	\
    ( (This)->lpVtbl -> add_ActiveMatchIndexChanged(This,eventHandler,token) ) 

#define ICoreWebView2ExperimentalFind_remove_ActiveMatchIndexChanged(This,token)	\
    ( (This)->lpVtbl -> remove_ActiveMatchIndexChanged(This,token) ) 

#define ICoreWebView2ExperimentalFind_add_MatchCountChanged(This,eventHandler,token)	\
    ( (This)->lpVtbl -> add_MatchCountChanged(This,eventHandler,token) ) 

#define ICoreWebView2ExperimentalFind_remove_MatchCountChanged(This,token)	\
    ( (This)->lpVtbl -> remove_MatchCountChanged(This,token) ) 

#define ICoreWebView2ExperimentalFind_Start(This,options,handler)	\
    ( (This)->lpVtbl -> Start(This,options,handler) ) 

#define ICoreWebView2ExperimentalFind_FindNext(This)	\
    ( (This)->lpVtbl -> FindNext(This) ) 

#define ICoreWebView2ExperimentalFind_FindPrevious(This)	\
    ( (This)->lpVtbl -> FindPrevious(This) ) 

#define ICoreWebView2ExperimentalFind_Stop(This)	\
    ( (This)->lpVtbl -> Stop(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalFind_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler = {0x0054f514,0x9a8e,0x5876,{0xae,0xd5,0x30,0xb3,0x7f,0x8c,0x86,0xa5}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0054f514-9a8e-5876-aed5-30b37f8c86a5")
    ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ ICoreWebView2ExperimentalFind *sender,
            /* [in] */ IUnknown *args) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler, Invoke)
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler * This,
            /* [in] */ ICoreWebView2ExperimentalFind *sender,
            /* [in] */ IUnknown *args);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandlerVtbl;

    interface ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler_Invoke(This,sender,args)	\
    ( (This)->lpVtbl -> Invoke(This,sender,args) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFindMatchCountChangedEventHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalFindMatchCountChangedEventHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalFindMatchCountChangedEventHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalFindMatchCountChangedEventHandler = {0xda0d6827,0x4254,0x5b10,{0xa6,0xd9,0x41,0x20,0x76,0xaf,0xc9,0xf3}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("da0d6827-4254-5b10-a6d9-412076afc9f3")
    ICoreWebView2ExperimentalFindMatchCountChangedEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ ICoreWebView2ExperimentalFind *sender,
            /* [in] */ IUnknown *args) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalFindMatchCountChangedEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalFindMatchCountChangedEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalFindMatchCountChangedEventHandler * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalFindMatchCountChangedEventHandler * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFindMatchCountChangedEventHandler, Invoke)
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalFindMatchCountChangedEventHandler * This,
            /* [in] */ ICoreWebView2ExperimentalFind *sender,
            /* [in] */ IUnknown *args);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalFindMatchCountChangedEventHandlerVtbl;

    interface ICoreWebView2ExperimentalFindMatchCountChangedEventHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalFindMatchCountChangedEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalFindMatchCountChangedEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalFindMatchCountChangedEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalFindMatchCountChangedEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalFindMatchCountChangedEventHandler_Invoke(This,sender,args)	\
    ( (This)->lpVtbl -> Invoke(This,sender,args) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalFindMatchCountChangedEventHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFindStartCompletedHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalFindStartCompletedHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalFindStartCompletedHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalFindStartCompletedHandler = {0x6a90ecaf,0x44b0,0x5bd9,{0x8f,0x07,0x19,0x67,0xe1,0x7b,0xe9,0xfb}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6a90ecaf-44b0-5bd9-8f07-1967e17be9fb")
    ICoreWebView2ExperimentalFindStartCompletedHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ HRESULT errorCode) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalFindStartCompletedHandlerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalFindStartCompletedHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalFindStartCompletedHandler * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalFindStartCompletedHandler * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFindStartCompletedHandler, Invoke)
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalFindStartCompletedHandler * This,
            /* [in] */ HRESULT errorCode);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalFindStartCompletedHandlerVtbl;

    interface ICoreWebView2ExperimentalFindStartCompletedHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalFindStartCompletedHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalFindStartCompletedHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalFindStartCompletedHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalFindStartCompletedHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalFindStartCompletedHandler_Invoke(This,errorCode)	\
    ( (This)->lpVtbl -> Invoke(This,errorCode) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalFindStartCompletedHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFindOptions_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalFindOptions_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalFindOptions */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalFindOptions = {0xe82e3b2b,0xa4af,0x5bc6,{0x94,0xc6,0x18,0xb4,0x41,0x57,0xa1,0x6c}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e82e3b2b-a4af-5bc6-94c6-18b44157a16c")
    ICoreWebView2ExperimentalFindOptions : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_FindTerm( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_FindTerm( 
            /* [in] */ LPCWSTR value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsCaseSensitive( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_IsCaseSensitive( 
            /* [in] */ BOOL value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ShouldHighlightAllMatches( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ShouldHighlightAllMatches( 
            /* [in] */ BOOL value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ShouldMatchWord( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ShouldMatchWord( 
            /* [in] */ BOOL value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SuppressDefaultFindDialog( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SuppressDefaultFindDialog( 
            /* [in] */ BOOL value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalFindOptionsVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalFindOptions * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalFindOptions * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalFindOptions * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFindOptions, get_FindTerm)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_FindTerm )( 
            ICoreWebView2ExperimentalFindOptions * This,
            /* [retval][out] */ LPWSTR *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFindOptions, put_FindTerm)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_FindTerm )( 
            ICoreWebView2ExperimentalFindOptions * This,
            /* [in] */ LPCWSTR value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFindOptions, get_IsCaseSensitive)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsCaseSensitive )( 
            ICoreWebView2ExperimentalFindOptions * This,
            /* [retval][out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFindOptions, put_IsCaseSensitive)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_IsCaseSensitive )( 
            ICoreWebView2ExperimentalFindOptions * This,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFindOptions, get_ShouldHighlightAllMatches)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ShouldHighlightAllMatches )( 
            ICoreWebView2ExperimentalFindOptions * This,
            /* [retval][out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFindOptions, put_ShouldHighlightAllMatches)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ShouldHighlightAllMatches )( 
            ICoreWebView2ExperimentalFindOptions * This,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFindOptions, get_ShouldMatchWord)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ShouldMatchWord )( 
            ICoreWebView2ExperimentalFindOptions * This,
            /* [retval][out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFindOptions, put_ShouldMatchWord)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ShouldMatchWord )( 
            ICoreWebView2ExperimentalFindOptions * This,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFindOptions, get_SuppressDefaultFindDialog)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SuppressDefaultFindDialog )( 
            ICoreWebView2ExperimentalFindOptions * This,
            /* [retval][out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFindOptions, put_SuppressDefaultFindDialog)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SuppressDefaultFindDialog )( 
            ICoreWebView2ExperimentalFindOptions * This,
            /* [in] */ BOOL value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalFindOptionsVtbl;

    interface ICoreWebView2ExperimentalFindOptions
    {
        CONST_VTBL struct ICoreWebView2ExperimentalFindOptionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalFindOptions_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalFindOptions_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalFindOptions_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalFindOptions_get_FindTerm(This,value)	\
    ( (This)->lpVtbl -> get_FindTerm(This,value) ) 

#define ICoreWebView2ExperimentalFindOptions_put_FindTerm(This,value)	\
    ( (This)->lpVtbl -> put_FindTerm(This,value) ) 

#define ICoreWebView2ExperimentalFindOptions_get_IsCaseSensitive(This,value)	\
    ( (This)->lpVtbl -> get_IsCaseSensitive(This,value) ) 

#define ICoreWebView2ExperimentalFindOptions_put_IsCaseSensitive(This,value)	\
    ( (This)->lpVtbl -> put_IsCaseSensitive(This,value) ) 

#define ICoreWebView2ExperimentalFindOptions_get_ShouldHighlightAllMatches(This,value)	\
    ( (This)->lpVtbl -> get_ShouldHighlightAllMatches(This,value) ) 

#define ICoreWebView2ExperimentalFindOptions_put_ShouldHighlightAllMatches(This,value)	\
    ( (This)->lpVtbl -> put_ShouldHighlightAllMatches(This,value) ) 

#define ICoreWebView2ExperimentalFindOptions_get_ShouldMatchWord(This,value)	\
    ( (This)->lpVtbl -> get_ShouldMatchWord(This,value) ) 

#define ICoreWebView2ExperimentalFindOptions_put_ShouldMatchWord(This,value)	\
    ( (This)->lpVtbl -> put_ShouldMatchWord(This,value) ) 

#define ICoreWebView2ExperimentalFindOptions_get_SuppressDefaultFindDialog(This,value)	\
    ( (This)->lpVtbl -> get_SuppressDefaultFindDialog(This,value) ) 

#define ICoreWebView2ExperimentalFindOptions_put_SuppressDefaultFindDialog(This,value)	\
    ( (This)->lpVtbl -> put_SuppressDefaultFindDialog(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalFindOptions_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFrame7_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalFrame7_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalFrame7 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalFrame7 = {0x792db210,0x2297,0x54e6,{0x8c,0x57,0x0c,0xee,0x4d,0x38,0xcd,0x8b}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("792db210-2297-54e6-8c57-0cee4d38cd8b")
    ICoreWebView2ExperimentalFrame7 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UseOverrideTimerWakeInterval( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_UseOverrideTimerWakeInterval( 
            /* [in] */ BOOL value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalFrame7Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalFrame7 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalFrame7 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalFrame7 * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFrame7, get_UseOverrideTimerWakeInterval)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UseOverrideTimerWakeInterval )( 
            ICoreWebView2ExperimentalFrame7 * This,
            /* [retval][out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalFrame7, put_UseOverrideTimerWakeInterval)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_UseOverrideTimerWakeInterval )( 
            ICoreWebView2ExperimentalFrame7 * This,
            /* [in] */ BOOL value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalFrame7Vtbl;

    interface ICoreWebView2ExperimentalFrame7
    {
        CONST_VTBL struct ICoreWebView2ExperimentalFrame7Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalFrame7_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalFrame7_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalFrame7_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalFrame7_get_UseOverrideTimerWakeInterval(This,value)	\
    ( (This)->lpVtbl -> get_UseOverrideTimerWakeInterval(This,value) ) 

#define ICoreWebView2ExperimentalFrame7_put_UseOverrideTimerWakeInterval(This,value)	\
    ( (This)->lpVtbl -> put_UseOverrideTimerWakeInterval(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalFrame7_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalProfile7_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalProfile7_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalProfile7 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalProfile7 = {0x11a14762,0x7780,0x46a1,{0xa1,0xc3,0x73,0xde,0x81,0x2d,0xae,0x12}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("11a14762-7780-46a1-a1c3-73de812dae12")
    ICoreWebView2ExperimentalProfile7 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ClearCustomDataPartition( 
            /* [in] */ LPCWSTR CustomDataPartitionId,
            /* [in] */ ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler *handler) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalProfile7Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalProfile7 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalProfile7 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalProfile7 * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalProfile7, ClearCustomDataPartition)
        HRESULT ( STDMETHODCALLTYPE *ClearCustomDataPartition )( 
            ICoreWebView2ExperimentalProfile7 * This,
            /* [in] */ LPCWSTR CustomDataPartitionId,
            /* [in] */ ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler *handler);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalProfile7Vtbl;

    interface ICoreWebView2ExperimentalProfile7
    {
        CONST_VTBL struct ICoreWebView2ExperimentalProfile7Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalProfile7_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalProfile7_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalProfile7_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalProfile7_ClearCustomDataPartition(This,CustomDataPartitionId,handler)	\
    ( (This)->lpVtbl -> ClearCustomDataPartition(This,CustomDataPartitionId,handler) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalProfile7_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler = {0xfe753727,0x5758,0x4fea,{0x8c,0xad,0x1e,0x68,0x5b,0x9c,0x3a,0xe8}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fe753727-5758-4fea-8cad-1e685b9c3ae8")
    ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ HRESULT errorCode) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandlerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler, Invoke)
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler * This,
            /* [in] */ HRESULT errorCode);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandlerVtbl;

    interface ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler_Invoke(This,errorCode)	\
    ( (This)->lpVtbl -> Invoke(This,errorCode) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalRestartRequestedEventArgs_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalRestartRequestedEventArgs_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalRestartRequestedEventArgs */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalRestartRequestedEventArgs = {0x40bb5778,0x6d85,0x5008,{0xbb,0xf4,0x22,0xcc,0x80,0x8b,0x27,0x37}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("40bb5778-6d85-5008-bbf4-22cc808b2737")
    ICoreWebView2ExperimentalRestartRequestedEventArgs : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Priority( 
            /* [retval][out] */ COREWEBVIEW2_RESTART_REQUESTED_PRIORITY *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalRestartRequestedEventArgsVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalRestartRequestedEventArgs * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalRestartRequestedEventArgs * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalRestartRequestedEventArgs * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalRestartRequestedEventArgs, get_Priority)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Priority )( 
            ICoreWebView2ExperimentalRestartRequestedEventArgs * This,
            /* [retval][out] */ COREWEBVIEW2_RESTART_REQUESTED_PRIORITY *value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalRestartRequestedEventArgsVtbl;

    interface ICoreWebView2ExperimentalRestartRequestedEventArgs
    {
        CONST_VTBL struct ICoreWebView2ExperimentalRestartRequestedEventArgsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalRestartRequestedEventArgs_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalRestartRequestedEventArgs_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalRestartRequestedEventArgs_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalRestartRequestedEventArgs_get_Priority(This,value)	\
    ( (This)->lpVtbl -> get_Priority(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalRestartRequestedEventArgs_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalSettings9_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalSettings9_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalSettings9 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalSettings9 = {0x0a4cf350,0xb8ab,0x5821,{0x81,0xb0,0x9c,0x58,0xa5,0x92,0x42,0x3f}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0a4cf350-b8ab-5821-81b0-9c58a592423f")
    ICoreWebView2ExperimentalSettings9 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PreferredBackgroundTimerWakeIntervalInMilliseconds( 
            /* [retval][out] */ INT32 *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PreferredBackgroundTimerWakeIntervalInMilliseconds( 
            /* [in] */ INT32 value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PreferredForegroundTimerWakeIntervalInMilliseconds( 
            /* [retval][out] */ INT32 *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PreferredForegroundTimerWakeIntervalInMilliseconds( 
            /* [in] */ INT32 value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PreferredIntensiveTimerWakeIntervalInMilliseconds( 
            /* [retval][out] */ INT32 *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PreferredIntensiveTimerWakeIntervalInMilliseconds( 
            /* [in] */ INT32 value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PreferredOverrideTimerWakeIntervalInMilliseconds( 
            /* [retval][out] */ INT32 *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PreferredOverrideTimerWakeIntervalInMilliseconds( 
            /* [in] */ INT32 value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalSettings9Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalSettings9 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalSettings9 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalSettings9 * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalSettings9, get_PreferredBackgroundTimerWakeIntervalInMilliseconds)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PreferredBackgroundTimerWakeIntervalInMilliseconds )( 
            ICoreWebView2ExperimentalSettings9 * This,
            /* [retval][out] */ INT32 *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalSettings9, put_PreferredBackgroundTimerWakeIntervalInMilliseconds)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PreferredBackgroundTimerWakeIntervalInMilliseconds )( 
            ICoreWebView2ExperimentalSettings9 * This,
            /* [in] */ INT32 value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalSettings9, get_PreferredForegroundTimerWakeIntervalInMilliseconds)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PreferredForegroundTimerWakeIntervalInMilliseconds )( 
            ICoreWebView2ExperimentalSettings9 * This,
            /* [retval][out] */ INT32 *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalSettings9, put_PreferredForegroundTimerWakeIntervalInMilliseconds)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PreferredForegroundTimerWakeIntervalInMilliseconds )( 
            ICoreWebView2ExperimentalSettings9 * This,
            /* [in] */ INT32 value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalSettings9, get_PreferredIntensiveTimerWakeIntervalInMilliseconds)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PreferredIntensiveTimerWakeIntervalInMilliseconds )( 
            ICoreWebView2ExperimentalSettings9 * This,
            /* [retval][out] */ INT32 *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalSettings9, put_PreferredIntensiveTimerWakeIntervalInMilliseconds)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PreferredIntensiveTimerWakeIntervalInMilliseconds )( 
            ICoreWebView2ExperimentalSettings9 * This,
            /* [in] */ INT32 value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalSettings9, get_PreferredOverrideTimerWakeIntervalInMilliseconds)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PreferredOverrideTimerWakeIntervalInMilliseconds )( 
            ICoreWebView2ExperimentalSettings9 * This,
            /* [retval][out] */ INT32 *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalSettings9, put_PreferredOverrideTimerWakeIntervalInMilliseconds)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PreferredOverrideTimerWakeIntervalInMilliseconds )( 
            ICoreWebView2ExperimentalSettings9 * This,
            /* [in] */ INT32 value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalSettings9Vtbl;

    interface ICoreWebView2ExperimentalSettings9
    {
        CONST_VTBL struct ICoreWebView2ExperimentalSettings9Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalSettings9_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalSettings9_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalSettings9_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalSettings9_get_PreferredBackgroundTimerWakeIntervalInMilliseconds(This,value)	\
    ( (This)->lpVtbl -> get_PreferredBackgroundTimerWakeIntervalInMilliseconds(This,value) ) 

#define ICoreWebView2ExperimentalSettings9_put_PreferredBackgroundTimerWakeIntervalInMilliseconds(This,value)	\
    ( (This)->lpVtbl -> put_PreferredBackgroundTimerWakeIntervalInMilliseconds(This,value) ) 

#define ICoreWebView2ExperimentalSettings9_get_PreferredForegroundTimerWakeIntervalInMilliseconds(This,value)	\
    ( (This)->lpVtbl -> get_PreferredForegroundTimerWakeIntervalInMilliseconds(This,value) ) 

#define ICoreWebView2ExperimentalSettings9_put_PreferredForegroundTimerWakeIntervalInMilliseconds(This,value)	\
    ( (This)->lpVtbl -> put_PreferredForegroundTimerWakeIntervalInMilliseconds(This,value) ) 

#define ICoreWebView2ExperimentalSettings9_get_PreferredIntensiveTimerWakeIntervalInMilliseconds(This,value)	\
    ( (This)->lpVtbl -> get_PreferredIntensiveTimerWakeIntervalInMilliseconds(This,value) ) 

#define ICoreWebView2ExperimentalSettings9_put_PreferredIntensiveTimerWakeIntervalInMilliseconds(This,value)	\
    ( (This)->lpVtbl -> put_PreferredIntensiveTimerWakeIntervalInMilliseconds(This,value) ) 

#define ICoreWebView2ExperimentalSettings9_get_PreferredOverrideTimerWakeIntervalInMilliseconds(This,value)	\
    ( (This)->lpVtbl -> get_PreferredOverrideTimerWakeIntervalInMilliseconds(This,value) ) 

#define ICoreWebView2ExperimentalSettings9_put_PreferredOverrideTimerWakeIntervalInMilliseconds(This,value)	\
    ( (This)->lpVtbl -> put_PreferredOverrideTimerWakeIntervalInMilliseconds(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalSettings9_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTexture_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalTexture_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalTexture */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalTexture = {0x0836f09c,0x34bd,0x47bf,{0x91,0x4a,0x99,0xfb,0x56,0xae,0x2d,0x07}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0836f09c-34bd-47bf-914a-99fb56ae2d07")
    ICoreWebView2ExperimentalTexture : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Handle( 
            /* [retval][out] */ HANDLE *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Resource( 
            /* [retval][out] */ IUnknown **value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Timestamp( 
            /* [retval][out] */ UINT64 *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Timestamp( 
            /* [in] */ UINT64 value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalTextureVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalTexture * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalTexture * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalTexture * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTexture, get_Handle)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Handle )( 
            ICoreWebView2ExperimentalTexture * This,
            /* [retval][out] */ HANDLE *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTexture, get_Resource)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Resource )( 
            ICoreWebView2ExperimentalTexture * This,
            /* [retval][out] */ IUnknown **value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTexture, get_Timestamp)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Timestamp )( 
            ICoreWebView2ExperimentalTexture * This,
            /* [retval][out] */ UINT64 *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTexture, put_Timestamp)
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Timestamp )( 
            ICoreWebView2ExperimentalTexture * This,
            /* [in] */ UINT64 value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalTextureVtbl;

    interface ICoreWebView2ExperimentalTexture
    {
        CONST_VTBL struct ICoreWebView2ExperimentalTextureVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalTexture_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalTexture_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalTexture_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalTexture_get_Handle(This,value)	\
    ( (This)->lpVtbl -> get_Handle(This,value) ) 

#define ICoreWebView2ExperimentalTexture_get_Resource(This,value)	\
    ( (This)->lpVtbl -> get_Resource(This,value) ) 

#define ICoreWebView2ExperimentalTexture_get_Timestamp(This,value)	\
    ( (This)->lpVtbl -> get_Timestamp(This,value) ) 

#define ICoreWebView2ExperimentalTexture_put_Timestamp(This,value)	\
    ( (This)->lpVtbl -> put_Timestamp(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalTexture_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStream_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalTextureStream_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalTextureStream */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalTextureStream = {0xafca8431,0x633f,0x4528,{0xab,0xfe,0x7f,0xc3,0xbe,0xdd,0x89,0x62}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("afca8431-633f-4528-abfe-7fc3bedd8962")
    ICoreWebView2ExperimentalTextureStream : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Id( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddAllowedOrigin( 
            /* [in] */ LPCWSTR origin,
            /* [in] */ BOOL value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveAllowedOrigin( 
            /* [in] */ LPCWSTR origin) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE add_StartRequested( 
            /* [in] */ ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_StartRequested( 
            /* [in] */ EventRegistrationToken token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE add_Stopped( 
            /* [in] */ ICoreWebView2ExperimentalTextureStreamStoppedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_Stopped( 
            /* [in] */ EventRegistrationToken token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTexture( 
            /* [in] */ UINT32 widthInTexels,
            /* [in] */ UINT32 heightInTexels,
            /* [retval][out] */ ICoreWebView2ExperimentalTexture **texture) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableTexture( 
            /* [retval][out] */ ICoreWebView2ExperimentalTexture **texture) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseTexture( 
            /* [in] */ ICoreWebView2ExperimentalTexture *texture) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PresentTexture( 
            /* [in] */ ICoreWebView2ExperimentalTexture *texture) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE add_ErrorReceived( 
            /* [in] */ ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_ErrorReceived( 
            /* [in] */ EventRegistrationToken token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetD3DDevice( 
            /* [in] */ IUnknown *d3dDevice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE add_WebTextureReceived( 
            /* [in] */ ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_WebTextureReceived( 
            /* [in] */ EventRegistrationToken token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE add_WebTextureStreamStopped( 
            /* [in] */ ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_WebTextureStreamStopped( 
            /* [in] */ EventRegistrationToken token) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalTextureStreamVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalTextureStream * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalTextureStream * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, get_Id)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Id )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [retval][out] */ LPWSTR *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, AddAllowedOrigin)
        HRESULT ( STDMETHODCALLTYPE *AddAllowedOrigin )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ LPCWSTR origin,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, RemoveAllowedOrigin)
        HRESULT ( STDMETHODCALLTYPE *RemoveAllowedOrigin )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ LPCWSTR origin);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, add_StartRequested)
        HRESULT ( STDMETHODCALLTYPE *add_StartRequested )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, remove_StartRequested)
        HRESULT ( STDMETHODCALLTYPE *remove_StartRequested )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ EventRegistrationToken token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, add_Stopped)
        HRESULT ( STDMETHODCALLTYPE *add_Stopped )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ ICoreWebView2ExperimentalTextureStreamStoppedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, remove_Stopped)
        HRESULT ( STDMETHODCALLTYPE *remove_Stopped )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ EventRegistrationToken token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, CreateTexture)
        HRESULT ( STDMETHODCALLTYPE *CreateTexture )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ UINT32 widthInTexels,
            /* [in] */ UINT32 heightInTexels,
            /* [retval][out] */ ICoreWebView2ExperimentalTexture **texture);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, GetAvailableTexture)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableTexture )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [retval][out] */ ICoreWebView2ExperimentalTexture **texture);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, CloseTexture)
        HRESULT ( STDMETHODCALLTYPE *CloseTexture )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ ICoreWebView2ExperimentalTexture *texture);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, PresentTexture)
        HRESULT ( STDMETHODCALLTYPE *PresentTexture )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ ICoreWebView2ExperimentalTexture *texture);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, Stop)
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            ICoreWebView2ExperimentalTextureStream * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, add_ErrorReceived)
        HRESULT ( STDMETHODCALLTYPE *add_ErrorReceived )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, remove_ErrorReceived)
        HRESULT ( STDMETHODCALLTYPE *remove_ErrorReceived )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ EventRegistrationToken token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, SetD3DDevice)
        HRESULT ( STDMETHODCALLTYPE *SetD3DDevice )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ IUnknown *d3dDevice);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, add_WebTextureReceived)
        HRESULT ( STDMETHODCALLTYPE *add_WebTextureReceived )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, remove_WebTextureReceived)
        HRESULT ( STDMETHODCALLTYPE *remove_WebTextureReceived )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ EventRegistrationToken token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, add_WebTextureStreamStopped)
        HRESULT ( STDMETHODCALLTYPE *add_WebTextureStreamStopped )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStream, remove_WebTextureStreamStopped)
        HRESULT ( STDMETHODCALLTYPE *remove_WebTextureStreamStopped )( 
            ICoreWebView2ExperimentalTextureStream * This,
            /* [in] */ EventRegistrationToken token);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalTextureStreamVtbl;

    interface ICoreWebView2ExperimentalTextureStream
    {
        CONST_VTBL struct ICoreWebView2ExperimentalTextureStreamVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalTextureStream_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalTextureStream_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalTextureStream_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalTextureStream_get_Id(This,value)	\
    ( (This)->lpVtbl -> get_Id(This,value) ) 

#define ICoreWebView2ExperimentalTextureStream_AddAllowedOrigin(This,origin,value)	\
    ( (This)->lpVtbl -> AddAllowedOrigin(This,origin,value) ) 

#define ICoreWebView2ExperimentalTextureStream_RemoveAllowedOrigin(This,origin)	\
    ( (This)->lpVtbl -> RemoveAllowedOrigin(This,origin) ) 

#define ICoreWebView2ExperimentalTextureStream_add_StartRequested(This,eventHandler,token)	\
    ( (This)->lpVtbl -> add_StartRequested(This,eventHandler,token) ) 

#define ICoreWebView2ExperimentalTextureStream_remove_StartRequested(This,token)	\
    ( (This)->lpVtbl -> remove_StartRequested(This,token) ) 

#define ICoreWebView2ExperimentalTextureStream_add_Stopped(This,eventHandler,token)	\
    ( (This)->lpVtbl -> add_Stopped(This,eventHandler,token) ) 

#define ICoreWebView2ExperimentalTextureStream_remove_Stopped(This,token)	\
    ( (This)->lpVtbl -> remove_Stopped(This,token) ) 

#define ICoreWebView2ExperimentalTextureStream_CreateTexture(This,widthInTexels,heightInTexels,texture)	\
    ( (This)->lpVtbl -> CreateTexture(This,widthInTexels,heightInTexels,texture) ) 

#define ICoreWebView2ExperimentalTextureStream_GetAvailableTexture(This,texture)	\
    ( (This)->lpVtbl -> GetAvailableTexture(This,texture) ) 

#define ICoreWebView2ExperimentalTextureStream_CloseTexture(This,texture)	\
    ( (This)->lpVtbl -> CloseTexture(This,texture) ) 

#define ICoreWebView2ExperimentalTextureStream_PresentTexture(This,texture)	\
    ( (This)->lpVtbl -> PresentTexture(This,texture) ) 

#define ICoreWebView2ExperimentalTextureStream_Stop(This)	\
    ( (This)->lpVtbl -> Stop(This) ) 

#define ICoreWebView2ExperimentalTextureStream_add_ErrorReceived(This,eventHandler,token)	\
    ( (This)->lpVtbl -> add_ErrorReceived(This,eventHandler,token) ) 

#define ICoreWebView2ExperimentalTextureStream_remove_ErrorReceived(This,token)	\
    ( (This)->lpVtbl -> remove_ErrorReceived(This,token) ) 

#define ICoreWebView2ExperimentalTextureStream_SetD3DDevice(This,d3dDevice)	\
    ( (This)->lpVtbl -> SetD3DDevice(This,d3dDevice) ) 

#define ICoreWebView2ExperimentalTextureStream_add_WebTextureReceived(This,eventHandler,token)	\
    ( (This)->lpVtbl -> add_WebTextureReceived(This,eventHandler,token) ) 

#define ICoreWebView2ExperimentalTextureStream_remove_WebTextureReceived(This,token)	\
    ( (This)->lpVtbl -> remove_WebTextureReceived(This,token) ) 

#define ICoreWebView2ExperimentalTextureStream_add_WebTextureStreamStopped(This,eventHandler,token)	\
    ( (This)->lpVtbl -> add_WebTextureStreamStopped(This,eventHandler,token) ) 

#define ICoreWebView2ExperimentalTextureStream_remove_WebTextureStreamStopped(This,token)	\
    ( (This)->lpVtbl -> remove_WebTextureStreamStopped(This,token) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalTextureStream_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler = {0x52cb8898,0xc711,0x401a,{0x8f,0x97,0x36,0x46,0x83,0x1b,0xa7,0x2d}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("52cb8898-c711-401a-8f97-3646831ba72d")
    ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ ICoreWebView2ExperimentalTextureStream *sender,
            /* [in] */ ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs *args) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler, Invoke)
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler * This,
            /* [in] */ ICoreWebView2ExperimentalTextureStream *sender,
            /* [in] */ ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs *args);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandlerVtbl;

    interface ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler_Invoke(This,sender,args)	\
    ( (This)->lpVtbl -> Invoke(This,sender,args) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalTextureStreamErrorReceivedEventHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler = {0x62d09330,0x00a9,0x41bf,{0xa9,0xae,0x55,0xaa,0xef,0x8b,0x3c,0x44}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("62d09330-00a9-41bf-a9ae-55aaef8b3c44")
    ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ ICoreWebView2ExperimentalTextureStream *sender,
            /* [in] */ IUnknown *args) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler, Invoke)
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler * This,
            /* [in] */ ICoreWebView2ExperimentalTextureStream *sender,
            /* [in] */ IUnknown *args);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandlerVtbl;

    interface ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler_Invoke(This,sender,args)	\
    ( (This)->lpVtbl -> Invoke(This,sender,args) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalTextureStreamStartRequestedEventHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamStoppedEventHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamStoppedEventHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalTextureStreamStoppedEventHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalTextureStreamStoppedEventHandler = {0x4111102a,0xd19f,0x4438,{0xaf,0x46,0xef,0xc5,0x63,0xb2,0xb9,0xcf}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4111102a-d19f-4438-af46-efc563b2b9cf")
    ICoreWebView2ExperimentalTextureStreamStoppedEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ ICoreWebView2ExperimentalTextureStream *sender,
            /* [in] */ IUnknown *args) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalTextureStreamStoppedEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalTextureStreamStoppedEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalTextureStreamStoppedEventHandler * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalTextureStreamStoppedEventHandler * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStreamStoppedEventHandler, Invoke)
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalTextureStreamStoppedEventHandler * This,
            /* [in] */ ICoreWebView2ExperimentalTextureStream *sender,
            /* [in] */ IUnknown *args);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalTextureStreamStoppedEventHandlerVtbl;

    interface ICoreWebView2ExperimentalTextureStreamStoppedEventHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalTextureStreamStoppedEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalTextureStreamStoppedEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalTextureStreamStoppedEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalTextureStreamStoppedEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalTextureStreamStoppedEventHandler_Invoke(This,sender,args)	\
    ( (This)->lpVtbl -> Invoke(This,sender,args) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalTextureStreamStoppedEventHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler = {0x9ea4228c,0x295a,0x11ed,{0xa2,0x61,0x02,0x42,0xac,0x12,0x00,0x02}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9ea4228c-295a-11ed-a261-0242ac120002")
    ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ ICoreWebView2ExperimentalTextureStream *sender,
            /* [in] */ ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs *args) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler, Invoke)
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler * This,
            /* [in] */ ICoreWebView2ExperimentalTextureStream *sender,
            /* [in] */ ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs *args);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandlerVtbl;

    interface ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler_Invoke(This,sender,args)	\
    ( (This)->lpVtbl -> Invoke(This,sender,args) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler = {0x77eb4638,0x2f05,0x11ed,{0xa2,0x61,0x02,0x42,0xac,0x12,0x00,0x02}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("77eb4638-2f05-11ed-a261-0242ac120002")
    ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ ICoreWebView2ExperimentalTextureStream *sender,
            /* [in] */ IUnknown *args) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler, Invoke)
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler * This,
            /* [in] */ ICoreWebView2ExperimentalTextureStream *sender,
            /* [in] */ IUnknown *args);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandlerVtbl;

    interface ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler_Invoke(This,sender,args)	\
    ( (This)->lpVtbl -> Invoke(This,sender,args) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalTextureStreamWebTextureStreamStoppedEventHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs = {0x0e1730c1,0x03df,0x4ad2,{0xb8,0x47,0xbe,0x4d,0x63,0xad,0xf7,0x00}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0e1730c1-03df-4ad2-b847-be4d63adf700")
    ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Kind( 
            /* [retval][out] */ COREWEBVIEW2_TEXTURE_STREAM_ERROR_KIND *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Texture( 
            /* [retval][out] */ ICoreWebView2ExperimentalTexture **value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgsVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs, get_Kind)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs * This,
            /* [retval][out] */ COREWEBVIEW2_TEXTURE_STREAM_ERROR_KIND *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs, get_Texture)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Texture )( 
            ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs * This,
            /* [retval][out] */ ICoreWebView2ExperimentalTexture **value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgsVtbl;

    interface ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs
    {
        CONST_VTBL struct ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs_get_Kind(This,value)	\
    ( (This)->lpVtbl -> get_Kind(This,value) ) 

#define ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs_get_Texture(This,value)	\
    ( (This)->lpVtbl -> get_Texture(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalTextureStreamErrorReceivedEventArgs_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs = {0xa4c2fa3a,0x295a,0x11ed,{0xa2,0x61,0x02,0x42,0xac,0x12,0x00,0x02}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a4c2fa3a-295a-11ed-a261-0242ac120002")
    ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_WebTexture( 
            /* [retval][out] */ ICoreWebView2ExperimentalWebTexture **value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgsVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs, get_WebTexture)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WebTexture )( 
            ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs * This,
            /* [retval][out] */ ICoreWebView2ExperimentalWebTexture **value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgsVtbl;

    interface ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs
    {
        CONST_VTBL struct ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs_get_WebTexture(This,value)	\
    ( (This)->lpVtbl -> get_WebTexture(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalTextureStreamWebTextureReceivedEventArgs_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalUpdateRuntimeResult_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalUpdateRuntimeResult_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalUpdateRuntimeResult */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalUpdateRuntimeResult = {0xDD503E49,0xAB19,0x47C0,{0xB2,0xAD,0x6D,0xDD,0x09,0xCC,0x3E,0x3A}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DD503E49-AB19-47C0-B2AD-6DDD09CC3E3A")
    ICoreWebView2ExperimentalUpdateRuntimeResult : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ COREWEBVIEW2_UPDATE_RUNTIME_STATUS *status) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ExtendedError( 
            /* [retval][out] */ HRESULT *error) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalUpdateRuntimeResultVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalUpdateRuntimeResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalUpdateRuntimeResult * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalUpdateRuntimeResult * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalUpdateRuntimeResult, get_Status)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            ICoreWebView2ExperimentalUpdateRuntimeResult * This,
            /* [retval][out] */ COREWEBVIEW2_UPDATE_RUNTIME_STATUS *status);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalUpdateRuntimeResult, get_ExtendedError)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExtendedError )( 
            ICoreWebView2ExperimentalUpdateRuntimeResult * This,
            /* [retval][out] */ HRESULT *error);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalUpdateRuntimeResultVtbl;

    interface ICoreWebView2ExperimentalUpdateRuntimeResult
    {
        CONST_VTBL struct ICoreWebView2ExperimentalUpdateRuntimeResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalUpdateRuntimeResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalUpdateRuntimeResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalUpdateRuntimeResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalUpdateRuntimeResult_get_Status(This,status)	\
    ( (This)->lpVtbl -> get_Status(This,status) ) 

#define ICoreWebView2ExperimentalUpdateRuntimeResult_get_ExtendedError(This,error)	\
    ( (This)->lpVtbl -> get_ExtendedError(This,error) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalUpdateRuntimeResult_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalWebTexture_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalWebTexture_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalWebTexture */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalWebTexture = {0xb94265ae,0x4c1e,0x11ed,{0xbd,0xc3,0x02,0x42,0xac,0x12,0x00,0x02}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b94265ae-4c1e-11ed-bdc3-0242ac120002")
    ICoreWebView2ExperimentalWebTexture : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Handle( 
            /* [retval][out] */ HANDLE *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Resource( 
            /* [retval][out] */ IUnknown **value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Timestamp( 
            /* [retval][out] */ UINT64 *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalWebTextureVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalWebTexture * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalWebTexture * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalWebTexture * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalWebTexture, get_Handle)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Handle )( 
            ICoreWebView2ExperimentalWebTexture * This,
            /* [retval][out] */ HANDLE *value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalWebTexture, get_Resource)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Resource )( 
            ICoreWebView2ExperimentalWebTexture * This,
            /* [retval][out] */ IUnknown **value);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalWebTexture, get_Timestamp)
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Timestamp )( 
            ICoreWebView2ExperimentalWebTexture * This,
            /* [retval][out] */ UINT64 *value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalWebTextureVtbl;

    interface ICoreWebView2ExperimentalWebTexture
    {
        CONST_VTBL struct ICoreWebView2ExperimentalWebTextureVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalWebTexture_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalWebTexture_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalWebTexture_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalWebTexture_get_Handle(This,value)	\
    ( (This)->lpVtbl -> get_Handle(This,value) ) 

#define ICoreWebView2ExperimentalWebTexture_get_Resource(This,value)	\
    ( (This)->lpVtbl -> get_Resource(This,value) ) 

#define ICoreWebView2ExperimentalWebTexture_get_Timestamp(This,value)	\
    ( (This)->lpVtbl -> get_Timestamp(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalWebTexture_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalCompositionControllerInterop3_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalCompositionControllerInterop3_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalCompositionControllerInterop3 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalCompositionControllerInterop3 = {0x6b0bf729,0x3d27,0x46c6,{0x90,0x1c,0x5f,0x15,0x16,0xd3,0x2d,0xe1}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6b0bf729-3d27-46c6-901c-5f1516d32de1")
    ICoreWebView2ExperimentalCompositionControllerInterop3 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE add_DragStarting( 
            /* [in] */ ICoreWebView2ExperimentalDragStartingEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_DragStarting( 
            /* [in] */ EventRegistrationToken token) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalCompositionControllerInterop3Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalCompositionControllerInterop3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalCompositionControllerInterop3 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalCompositionControllerInterop3 * This);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalCompositionControllerInterop3, add_DragStarting)
        HRESULT ( STDMETHODCALLTYPE *add_DragStarting )( 
            ICoreWebView2ExperimentalCompositionControllerInterop3 * This,
            /* [in] */ ICoreWebView2ExperimentalDragStartingEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token);
        
        DECLSPEC_XFGVIRT(ICoreWebView2ExperimentalCompositionControllerInterop3, remove_DragStarting)
        HRESULT ( STDMETHODCALLTYPE *remove_DragStarting )( 
            ICoreWebView2ExperimentalCompositionControllerInterop3 * This,
            /* [in] */ EventRegistrationToken token);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalCompositionControllerInterop3Vtbl;

    interface ICoreWebView2ExperimentalCompositionControllerInterop3
    {
        CONST_VTBL struct ICoreWebView2ExperimentalCompositionControllerInterop3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalCompositionControllerInterop3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalCompositionControllerInterop3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalCompositionControllerInterop3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalCompositionControllerInterop3_add_DragStarting(This,eventHandler,token)	\
    ( (This)->lpVtbl -> add_DragStarting(This,eventHandler,token) ) 

#define ICoreWebView2ExperimentalCompositionControllerInterop3_remove_DragStarting(This,token)	\
    ( (This)->lpVtbl -> remove_DragStarting(This,token) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalCompositionControllerInterop3_INTERFACE_DEFINED__ */

#endif /* __WebView2Experimental_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif



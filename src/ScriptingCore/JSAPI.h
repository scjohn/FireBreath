/**********************************************************\ 
Original Author: Richard Bateman (taxilian)

Created:    Sept 24, 2009
License:    Dual license model; choose one of two:
            New BSD License
            http://www.opensource.org/licenses/bsd-license.php
            - or -
            GNU Lesser General Public License, version 2.1
            http://www.gnu.org/licenses/lgpl-2.1.html
            
Copyright 2009 Richard Bateman, Firebreath development team
\**********************************************************/

#pragma once
#ifndef H_FB_JSAPI
#define H_FB_JSAPI

#include "APITypes.h"
#include <list>
#include <deque>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include "JSExceptions.h"
#include "boost/thread/recursive_mutex.hpp"
#include "boost/thread/mutex.hpp"

namespace FB
{
    class JSObject;
    class BrowserHost;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @class  JSAPI
    ///
    /// @brief  JavaScript API class -- provides a javascript interface that can be exposed to the
    ///         browser.
    /// 
    /// JSAPI is the core class for all interaction with javascript.  All PluginCore-derived Plugin
    /// objects must provide a JSAPI object to provide the javascript interface for their &lt;object&gt;
    /// tag, and methods or properties of that object can return other JSAPI objects.
    /// 
    /// Important things to know about JSAPI objects:
    ///   - Unless you have unusual needs, you will most likely want to extend FB::JSAPIAuto instead
    ///     of extending JSAPI directly.
    ///   - Any time you work with a JSAPI object you should use it with a boost::shared_ptr. 
    ///     FB::JSAPIPtr is a typedef for a boost::shared_ptr<JSAPI> which may be useful.
    ///     -  From inside the object you can use the shared_ptr() method to get a shared_ptr for
    ///        "this"
    ///   - Objects passed in from javascript, including functions, will be passed in as FB::JSObject
    ///     objects which extend JSAPI.
    ///
    /// @author Richard Bateman
    /// @see FB::JSAPIAuto
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    class JSAPI : public boost::enable_shared_from_this<JSAPI>, boost::noncopyable
    {
    public:

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn JSAPI(void)
        ///
        /// @brief  Default constructor. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        JSAPI(void);
        JSAPI( const SecurityZone& securityLevel );

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual ~JSAPI(void)
        ///
        /// @brief  Finaliser. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual ~JSAPI(void);

    public:

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn JSAPIPtr shared_ptr()
        ///
        /// @brief  Gets the shared pointer for "this"
        ///
        /// @return JSAPIPtr for "this"
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        JSAPIPtr shared_ptr()
        {
            return shared_from_this();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn void invalidate()
        ///
        /// @brief  Invalidates this object.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        void invalidate();

    protected:
        void fireAsyncEvent( const std::string& eventName, const std::vector<variant>& args );

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual void FireEvent(const std::wstring& eventName, const std::vector<variant> &args)
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void FireEvent(const std::wstring& eventName, const std::vector<variant> &args);
        
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual void FireEvent(const std::string& eventName, const std::vector<variant> &args)
        ///
        /// @brief  Fires an event into javascript asynchronously
        ///         
        /// This fires an event to all handlers attached to the given event in javascript.
        /// 
        /// IE:
        /// @code
        ///      document.getElementByID("plugin").attachEvent("onload", function() { alert("loaded!"); });
        /// @endcode
        /// Firefox/Safari/Chrome/Opera:
        /// @code
        ///      // Note that the convention used by these browsers is that "on" is implied
        ///      document.getElementByID("plugin").addEventListener("load", function() { alert("loaded!"); }, false);;/.
        /// @endcode
        ///
        /// You can then fire the event -- from any thread -- from the JSAPI object like so:
        /// @code
        ///      FireEvent("onload", FB::variant_list_of("param1")(2)(3.0));
        /// @endcode
        ///         
        /// Also note that registerEvent must be called from the constructor to register the event.
        /// @code
        ///      registerEvent("onload");
        /// @endcode
        /// 
        /// @param  eventName   Name of the event.  This event must start with "on"
        /// @param  args        The arguments that should be sent to each attached event handler
        ///
        /// @see registerEvent
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void FireEvent(const std::string& eventName, const std::vector<variant> &args);
        
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief  Fires an event into javascript asynchronously using a W3C-compliant event parameter
        ///         
        /// This fires an event to all handlers attached to the given event in javascript. With a
        /// W3C-compliant event parameter
        /// 
        /// IE:
        /// @code
        ///      document.getElementByID("plugin").attachEvent("onload", function() { alert("loaded!"); });
        /// @endcode
        /// Firefox/Safari/Chrome/Opera:
        /// @code
        ///      // Note that the convention used by these browsers is that "on" is implied
        ///      document.getElementByID("plugin").addEventListener("load", function() { alert("loaded!"); }, false);;/.
        /// @endcode
        ///
        /// You can then fire the event -- from any thread -- from the JSAPI object like so:
        /// @code
        ///      FireEvent("onload", FB::variant_list_of("param1")(2)(3.0));
        /// @endcode
        ///         
        /// Also note that registerEvent must be called from the constructor to register the event.
        /// @code
        ///      registerEvent("onload");
        /// @endcode
        /// 
        /// @param  eventName   Name of the event.  This event must start with "on"
        /// @param  args        The arguments that should be sent to each attached event handler
        ///
        /// @see registerEvent
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void FireJSEvent(const std::string& eventName, const FB::VariantMap &members, const FB::VariantList &arguments);
        /// @overload
        virtual void FireJSEvent(const std::string& eventName, const FB::VariantMap &params);
        /// @overload
        virtual void FireJSEvent(const std::string& eventName, const FB::VariantList &arguments);

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn public  void FB::JSAPI::pushZone(const SecurityZone& securityLevel)
        ///
        /// @brief  Pushes a new security level and locks a mutex (for every Push there *must* be a Pop!)
        ///
        /// This should be used to temporarily set the security zone of the API object. Note that this
        /// also locks a mutex to ensure that access to members under a non-default security level is
        /// serialized. Do not *ever* leave an unmatched push (a push with no pop after it). For safety,
        /// use the helper FB::scoped_zonelock:
        /// @code
        ///      // In the constructor
        ///      // Register protected members
        ///      {
        ///          FB::scoped_zonelock _l(this, SecurityScope_Protected);
        ///          registerMethod("start", make_method(this, &MyPluginAPI::start));
        ///      } // Zone automatically popped off
        ///      // Register private members
        ///      {
        ///          FB::scoped_zonelock _l(this, SecurityScope_Protected);
        ///          registerMethod("getDirectoryListing", make_method(this, &MyPluginAPI::getDirectoryListing));
        ///      } // Zone automatically popped off
        /// @endcode
        ///
        /// @param  securityLevel   const SecurityZone &    Zone id to push on the stack
        /// @since 1.4a3
        /// @see FB::scoped_zonelock
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void pushZone(const SecurityZone& securityLevel);
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn public  void FB::JSAPI::popZone()
        ///
        /// @brief  Pops off a security level and unlocks the mutex (for every Push there *must* be a Pop!)
        ///
        /// Seriously, it's far better to use FB::scoped_zonelock instead of using popZone and pushZone
        ///
        /// @returns void
        /// @since 1.4a3
        /// @see FB::scoped_zonelock
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void popZone();

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn public void setDefaultZone(const SecurityZone& securityLevel)
        ///
        /// @brief  Sets the default zone (the zone the class operates on before a push)
        ///
        /// @returns void
        /// @since 1.4a3
        /// @see FB::scoped_zonelock
        /// @see pushZone
        /// @see popZone
        /// @see getDefaultZone
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void setDefaultZone(const SecurityZone& securityLevel);

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn public virtual SecurityZone getDefaultZone() const
        ///
        /// @brief  Gets the default zone (the zone the class operates on before a push)
        ///
        /// @returns SecurityZone the default zone
        /// @since 1.4a3
        /// @see FB::scoped_zonelock
        /// @see pushZone
        /// @see popZone
        /// @see getDefaultZone
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual SecurityZone getDefaultZone() const;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn public SecurityZone getZone() const
        ///
        /// @brief  Gets the currently active zone
        ///
        /// @returns SecurityZone the current zone
        /// @since 1.4a3
        /// @see FB::scoped_zonelock
        /// @see pushZone
        /// @see popZone
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual SecurityZone getZone() const;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual void registerEvent(const std::string& name)
        ///
        /// @brief  Register event so that event listeners can be added/attached from javascript
        ///
        /// @param  name    The name of the event to register.  This event must start with "on"
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void registerEvent(const std::string& name);
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual void registerEvent(const std::wstring& name)
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void registerEvent(const std::wstring& name);

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual void registerEventMethod(const std::string& name, JSObjectPtr& event)
        ///
        /// @brief  Called by the browser to register an event handler method
        ///
        /// @param  name            The name. 
        /// @param  event           The event handler method. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void registerEventMethod(const std::string& name, JSObjectPtr& event);
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual void registerEventMethod(const std::wstring& name, JSObjectPtr& event)
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void registerEventMethod(const std::wstring& name, JSObjectPtr& event);
        
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual void unregisterEventMethod(const std::string& name, JSObjectPtr& event)
        ///
        /// @brief  Called by the browser to unregister an event handler method
        ///
        /// @param  name            The name. 
        /// @param  event           The event handler method to unregister. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void unregisterEventMethod(const std::string& name, JSObjectPtr& event);
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual void unregisterEventMethod(const std::wstring& name, JSObjectPtr& event)
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void unregisterEventMethod(const std::wstring& name, JSObjectPtr& event);

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual void registerEventInterface(JSObjectPtr& event)
        ///
        /// @brief  Called by the browser to register a JSObject interface that handles events.  This is
        ///         primarily used by IE.  Objects provided to this method are called when events are fired
        ///         by calling a method of the event name on the event interface
        ///
        /// @param  event   The JSAPI interface 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void registerEventInterface(const JSObjectPtr& event);
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual void unregisterEventInterface(JSObjectPtr& event)
        ///
        /// @brief  Called by the browser to unregister a JSObject interface that handles events.  
        ///
        /// @param  event   The JSAPI interface
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void unregisterEventInterface(const JSObjectPtr& event);

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual JSObjectPtr getDefaultEventMethod(const std::wstring& name) const
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual JSObjectPtr getDefaultEventMethod(const std::wstring& name) const;
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual JSObjectPtr getDefaultEventMethod(const std::string& name) const
        ///
        /// @brief  Called by the browser to get the default event handler method for an event.
        ///         
        /// This is called when the following occurs iff onload is a registered event:
        /// @code
        ///      var handler = document.getElementByID("plugin").onload;
        /// @endcode
        ///         
        /// @param  name    The event name. 
        ///
        /// @return The default event method. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual JSObjectPtr getDefaultEventMethod(const std::string& name) const;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual void setDefaultEventMethod(const std::string& name, JSObjectPtr event)
        ///
        /// @brief  Called by the browser to set the default event handler method for an event.
        ///
        /// This is called when the following occurs iff onload is a registered event:
        /// @code
        ///      document.getElementByID("plugin").onload = function() { alert("loaded"); };
        /// @endcode
        ///
        /// @param  name    The event name
        /// @param  event   The event handler method. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void setDefaultEventMethod(const std::string& name, JSObjectPtr event);
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual void setDefaultEventMethod(const std::wstring& name, JSObjectPtr event)
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void setDefaultEventMethod(const std::wstring& name, JSObjectPtr event);

        virtual void getMemberNames(std::vector<std::wstring> &nameVector) const;
        virtual void getMemberNames(std::vector<std::wstring> *nameVector) const;
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual void getMemberNames(std::vector<std::string> &nameVector) const = 0
        ///
        /// @brief  Called by the browser to enumerate the members of this JSAPI object
        ///         
        /// This must be implemented by anything extending JSAPI directly.  JSAPIAuto implements this
        /// for you.
        ///
        /// @param [out] nameVector  The name vector. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void getMemberNames(std::vector<std::string> &nameVector) const = 0;
        virtual void getMemberNames(std::vector<std::string> *nameVector) const;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual size_t getMemberCount() const = 0
        ///
        /// @brief  Gets the member count. 
        ///
        /// @return The member count. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual size_t getMemberCount() const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual bool HasMethod(const std::wstring& methodName) const
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual bool HasMethod(const std::wstring& methodName) const;
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual bool HasMethod(const std::string& methodName) const = 0
        ///
        /// @brief  Query if the JSAPI object has the 'methodName' method. 
        ///
        /// @param  methodName  Name of the method. 
        ///
        /// @return true if method exists, false if not. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual bool HasMethod(const std::string& methodName) const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual bool HasMethodObject(const std::wstring& methodObjName) const
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual bool HasMethodObject(const std::wstring& methodObjName) const;
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual bool HasMethodObject(const std::string& methodObjName) const
        ///
        /// @brief  Query if 'methodObjName' is a valid methodObj. 
        ///
        /// @param  methodObjName    Name of the method to fetch an object for. 
        ///
        /// If this feature is supported 
        ///
        /// @return true if methodObj exists, false if not. 
        /// @since 1.4
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual bool HasMethodObject(const std::string& methodObjName) const { return false; }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual bool HasProperty(const std::wstring& propertyName) const
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual bool HasProperty(const std::wstring& propertyName) const;
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual bool HasProperty(const std::string& propertyName) const
        ///
        /// @brief  Query if 'propertyName' is a valid property. 
        ///
        /// @param  propertyName    Name of the property. 
        ///
        /// @return true if property exists, false if not. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual bool HasProperty(const std::string& propertyName) const = 0;
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual bool HasProperty(int idx) const = 0
        ///
        /// @brief  Query if the property at "idx" exists.
        ///     
        /// This can be used for providing array-style access on your object.  For example, the following
        /// will result in a call to HasProperty with idx = 12:
        /// @code
        ///       document.getElementById("plugin")[12];
        /// @endcode 
        ///
        /// @param  idx Zero-based index of the property to check for
        ///
        /// @return true if property exists, false if not. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual bool HasProperty(int idx) const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual bool HasEvent(const std::string& eventName) const
        ///
        /// @brief  Query if the event 'eventName' has been registered
        ///
        /// @param  eventName   Name of the event. 
        ///
        /// @return true if event registered, false if not. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual bool HasEvent(const std::string& eventName) const;
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual bool HasEvent(const std::wstring& eventName) const
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual bool HasEvent(const std::wstring& eventName) const;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual JSAPIPtr GetMethodObject(const std::wstring& methodObjName)
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual JSAPIPtr GetMethodObject(const std::wstring& methodObjName);
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual variant GetMethodObject(const std::string& methodObjName) = 0
        ///
        /// @brief  Gets a method object (JSAPI object that has a default method)
        ///
        /// Often it is preferable with the plugins to have the API return a JSAPI object as a
        /// property and then call the default method on that object.  This looks the same in
        /// javascript, except that you can save the function object if you want to.  See
        /// FB::JSFunction for an example of how to make a function object
        ///
        /// @param  methodObjName    Name of the methodObj. 
        /// @return The methodObj value 
        /// @since 1.4
        /// @see FB::JSFunction
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual JSAPIPtr GetMethodObject(const std::string& methodObjName) { return FB::JSAPIPtr(); }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual variant GetProperty(const std::wstring& propertyName)
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual variant GetProperty(const std::wstring& propertyName);
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual variant GetProperty(const std::string& propertyName) = 0
        ///
        /// @brief  Gets a property value
        ///
        /// @param  propertyName    Name of the property. 
        ///
        /// @return The property value 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual variant GetProperty(const std::string& propertyName) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual void SetProperty(const std::wstring& propertyName, const variant& value)
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void SetProperty(const std::wstring& propertyName, const variant& value);
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual void SetProperty(const std::string& propertyName, const variant& value) = 0
        ///
        /// @brief  Sets the value of a property. 
        ///
        /// @param  propertyName    Name of the property. 
        /// @param  value           The value. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void SetProperty(const std::string& propertyName, const variant& value) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual variant GetProperty(int idx) = 0
        ///
        /// @brief  Gets the value of an indexed property. 
        ///
        /// This can be used for providing array-style access on your object.  For example, the following
        /// will result in a call to GetProperty with idx = 12:
        /// @code
        ///       var i = document.getElementById("plugin")[12];
        /// @endcode 
        ///
        /// @param  idx Zero-based index of the property to get the value of. 
        ///
        /// @return The property value. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual variant GetProperty(int idx) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual void SetProperty(int idx, const variant& value) = 0
        ///
        /// @brief  Sets the value of an indexed property. 
        ///
        /// This can be used for providing array-style access on your object.  For example, the following
        /// will result in a call to SetProperty with idx = 12:
        /// @code
        ///       document.getElementById("plugin")[12] = "property value";
        /// @endcode 
        ///
        /// @param  idx     Zero-based index of the property to set the value of. 
        /// @param  value   The new property value. 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual void SetProperty(int idx, const variant& value) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @overload virtual variant Invoke(const std::wstring& methodName, const std::vector<variant>& args)
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual variant Invoke(const std::wstring& methodName, const std::vector<variant>& args);
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn virtual variant Invoke(const std::string& methodName,
        /// const std::vector<variant>& args) = 0
        ///
        /// @brief  Called by the browser to invoke a method on the JSAPI object.
        ///
        /// @param  methodName  Name of the method. 
        /// @param  args        The arguments. 
        ///
        /// @return result of method call 
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        virtual variant Invoke(const std::string& methodName, const std::vector<variant>& args) = 0;

    public:
        virtual void registerProxy(const JSAPIWeakPtr &ptr) const;
        virtual void unregisterProxy( const FB::JSAPIPtr& ptr ) const;

    protected:
        typedef std::deque<SecurityZone> ZoneStack;
        // Stores event handlers
        EventMultiMap m_eventMap;       
        // Stores event-as-property event handlers
        EventSingleMap m_defEventMap;   
        // Stores event interfaces
        EventIFaceMap m_evtIfaces;      

        typedef std::vector<JSAPIWeakPtr> ProxyList;
        mutable ProxyList m_proxies;

        mutable boost::recursive_mutex m_zoneMutex;
        ZoneStack m_zoneStack;
                
        bool m_valid;                   // Tracks if this object has been invalidated
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @class scoped_zonelock
    ///
    /// @brief  Provides a helper class for locking
    ///
    /// This class will call pushZone on the provided JSAPI object when instantiated and popZone
    /// when it goes out of scope. 
    /// @code
    ///      // In the constructor
    ///      // Register protected members
    ///      {
    ///          FB::scoped_zonelock _l(this, SecurityScope_Protected);
    ///          registerMethod("start", make_method(this, &MyPluginAPI::start));
    ///      } // Zone automatically popped off
    ///      // Register private members
    ///      {
    ///          FB::scoped_zonelock _l(this, SecurityScope_Protected);
    ///          registerMethod("getDirectoryListing", make_method(this, &MyPluginAPI::getDirectoryListing));
    ///      } // Zone automatically popped off
    /// @endcode
    /// 
    /// @since 1.4a3
    /// @see FB::JSAPI::pushZone
    /// @see FB::JSAPI::popZone
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    class scoped_zonelock : boost::noncopyable
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn public FB::scoped_zonelock::scoped_zonelock(const JSAPIPtr &api, const SecurityZone& zone)
        ///
        /// @brief  Accepts a FB::JSAPIPtr and pushes the specified security zone to be used
        ///         until this object goes out of scope
        ///
        /// @param  api     const JSAPIPtr&     JSAPI object to lock the zone for
        /// @param  zone    const SecurityZone& Zone to push
        /// @since 1.4a3
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        scoped_zonelock(const JSAPIPtr &api, const SecurityZone& zone)
            : m_api(api.get()), ref(api) {
            lock(zone);
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn public   FB::scoped_zonelock::scoped_zonelock(JSAPI* api, const SecurityZone& zone)
        ///
        /// @brief  
        ///
        /// @param  api     JSAPI*        JSAPI object to lock the zone for
        /// @param  zone    const SecurityZone& Zone to push
        /// @since 1.4a3
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        scoped_zonelock(JSAPI* api, const SecurityZone& zone) : m_api(api) {
            lock(zone);
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @fn public   FB::scoped_zonelock::~scoped_zonelock()
        ///
        /// @brief   Unlocks/pops the zone
        /// @since 1.4a3
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        ~scoped_zonelock() {
            if (m_api)
                m_api->popZone();
        }
    private:
        void lock(const SecurityZone& zone) const {
            if (m_api)
                m_api->pushZone(zone);
        }
        JSAPI* m_api;
        const FB::JSAPIPtr ref;
    };
};

// There are important conversion routines that require JSObject and JSAPI to both be loaded
#include "JSObject.h"
#endif

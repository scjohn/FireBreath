/**********************************************************\ 
Original Author: Richard Bateman (taxilian)

Created:    Oct 19, 2009
License:    Dual license model; choose one of two:
            New BSD License
            http://www.opensource.org/licenses/bsd-license.php
            - or -
            GNU Lesser General Public License, version 2.1
            http://www.gnu.org/licenses/lgpl-2.1.html

Copyright 2009 PacketPass, Inc and the Firebreath development team
\**********************************************************/

#include "PluginWindow.h"
#include "JSAPI.h"
#include "variant_list.h"
#include "FactoryBase.h"
#include "BrowserHost.h"
#include "DOM/Window.h"
#include "logging.h"

#include "PluginCore.h"

using namespace FB;

/***************************\
 Static initialization stuff
\***************************/

volatile int PluginCore::ActivePluginCount = 0;

std::string PluginCore::OS;
std::string PluginCore::Browser;
void PluginCore::setPlatform(const std::string& os, const std::string& browser)
{
    PluginCore::OS = os;
    PluginCore::Browser = browser;
    FBLOG_INFO("FB::PluginCore", "os: " << os << "; browser: " << browser);
}

/***************************\
     Regular Class Stuff
\***************************/

PluginCore::PluginCore() : m_paramsSet(false), m_Window(NULL)
{
    FB::Log::initLogging();
    // This class is only created on the main UI thread,
    // so there is no need for mutexes here
    ++PluginCore::ActivePluginCount;
}

PluginCore::~PluginCore()
{
    // Tell the host that the plugin is shutting down
    m_host->shutdown();
    // This class is only destroyed on the main UI thread,
    // so there is no need for mutexes here
    --PluginCore::ActivePluginCount;
}

void PluginCore::setParams(const FB::VariantMap& inParams)
{
    for (FB::VariantMap::const_iterator it = inParams.begin(); it != inParams.end(); ++it)
    {
        std::string key(it->first);
        try {
            std::string value(it->second.convert_cast<std::string>());
            if (key.substr(0, 2) == "on") {
                FB::JSObjectPtr tmp;
                tmp = m_host->getDOMWindow()
                    ->getProperty<FB::JSObjectPtr>(value);

                FBLOG_TRACE("PluginCore", "Found <param> event handler: " << key);

                m_params[key] = tmp;
            } else {
                m_params[key] = it->second;
            }
        } catch (const std::exception &ex) {
            FBLOG_WARN("PluginCore", "Exception processing <param> " << key << ": " << ex.what());
            m_params[it->first] = it->second;
        }
    }
}

void PluginCore::SetHost(FB::BrowserHostPtr host)
{
    m_host = host;
}

JSAPIPtr PluginCore::getRootJSAPI()
{
    if (!m_api) {
        m_api = createJSAPI();
    }

    return m_api;
}

PluginWindow* PluginCore::GetWindow() const
{
    return m_Window;
}

void PluginCore::SetWindow(PluginWindow *win)
{
    FBLOG_TRACE("PluginCore", "Window Set");
    if (m_Window && m_Window != win) {
        ClearWindow();
    }
    m_Window = win;
    win->AttachObserver(this);
}

void PluginCore::ClearWindow()
{
    FBLOG_TRACE("PluginCore", "Window Cleared");
    if (m_Window) {
        m_Window->DetachObserver(this);
        m_Window = NULL;
    }
}

// If you override this, you probably want to call it again, since this is what calls back into the page
// to indicate that we're done.
void PluginCore::setReady()
{
    FBLOG_INFO("PluginCore", "Plugin Ready");
    // Ensure that the JSAPI object has been created, in case the browser hasn't requested it yet.
    getRootJSAPI(); 
    try {
        FB::VariantMap::iterator fnd = m_params.find("onload");
        if (fnd != m_params.end()) {
            FB::JSObjectPtr method = fnd->second.convert_cast<FB::JSObjectPtr>();
			if(method) {
				method->InvokeAsync("", FB::variant_list_of(getRootJSAPI()));
			}
        }
    } catch(...) {
        // Usually this would be if it isn't a JSObjectPtr or the object can't be called
    }
    onPluginReady();
}

bool PluginCore::isWindowless()
{
    FB::VariantMap::iterator itr = m_params.find("windowless");
    if (itr != m_params.end()) {
        try {
            return itr->second.convert_cast<bool>();
        } catch (const FB::bad_variant_cast& ex) {
            FB_UNUSED_VARIABLE(ex);
        }
    }
    return false;
}

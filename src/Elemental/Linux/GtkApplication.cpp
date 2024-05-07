#include <gio/gio.h>
#include "GtkApplication.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"
#include "SystemSpan.h"
#include "SystemPlatformFunctions.h"

MemoryArena ApplicationMemoryArena;

GtkApplication* GlobalGtkApplication;

void InitLinuxApplicationMemory()
{
    if (ApplicationMemoryArena.Storage == nullptr)
    {
        ApplicationMemoryArena = SystemAllocateMemoryArena();

        SystemLogDebugMessage(ElemLogMessageCategory_Application, "Init OK.");

        #ifdef _DEBUG
        SystemLogDebugMessage(ElemLogMessageCategory_Application, "Debug Mode.");
        #endif
    }
}

static void on_theme_changed(GSettings *settings, gchar *key, GtkApplication *app) {
    SystemLogDebugMessage(ElemLogMessageCategory_Application, "Theme");
    gchar *theme_name = g_settings_get_string(settings, "gtk-theme");
    gboolean use_dark_theme = g_strrstr(theme_name, "dark") != NULL;

    GtkSettings *gtk_settings = gtk_settings_get_default();
    g_object_set(gtk_settings, "gtk-application-prefer-dark-theme", use_dark_theme, NULL);

    g_free(theme_name);
}

void GtkAppActivate(GApplication* app, gpointer* userData) 
{
    SystemAssert(userData);
    auto parameters = (ElemRunApplicationParameters*)userData;
    
    g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", true, nullptr);

    SystemLogDebugMessage(ElemLogMessageCategory_Application, "Welcome to GTK! %s", parameters->ApplicationName);

    if (parameters->InitHandler)
    {
        parameters->InitHandler(parameters->Payload);
    }
}

ElemAPI void ElemConfigureLogHandler(ElemLogHandlerPtr logHandler)
{
    if (logHandler)
    {
        SystemRegisterLogHandler(logHandler);
    } 
}

ElemAPI ElemSystemInfo ElemGetSystemInfo()
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto executablePath = SystemPlatformGetExecutablePath(stackMemoryArena);
    auto environment = SystemPlatformGetEnvironment(stackMemoryArena);
                      
    auto lastIndex = SystemLastIndexOf(executablePath, environment->PathSeparator);
    SystemAssert(lastIndex != -1);

    auto applicationPath = SystemDuplicateBuffer(stackMemoryArena, executablePath.Slice(0, lastIndex + 1));

    return
    {
        .Platform = ElemPlatform_Linux,
        .ApplicationPath = applicationPath.Pointer,
        .SupportMultiWindows = true
    };
}

ElemAPI int32_t ElemRunApplication(const ElemRunApplicationParameters* parameters)
{
    InitLinuxApplicationMemory();

    auto parametersCopy = SystemDuplicateBuffer(ApplicationMemoryArena, ReadOnlySpan<ElemRunApplicationParameters>((ElemRunApplicationParameters*)parameters, 1));

    GlobalGtkApplication = gtk_application_new("io.libelemental", G_APPLICATION_DEFAULT_FLAGS);
    //GlobalGtkApplication = gtk_application_new("io.libelemental", G_APPLICATION_FLAGS_NONE);

    g_signal_connect(GlobalGtkApplication, "activate", G_CALLBACK(GtkAppActivate), parametersCopy.Pointer);

    // TODO: Detect dark theme
    //GSettings *settings = g_settings_new("org.gnome.desktop.interface");
    //g_signal_connect(settings, "changed::gtk-theme", G_CALLBACK(on_theme_changed), GlobalGtkApplication);

    auto result = g_application_run(G_APPLICATION(GlobalGtkApplication), 0, nullptr);

    if (parameters->FreeHandler)
    {
        parameters->FreeHandler(parameters->Payload);
    }

    g_object_unref(GlobalGtkApplication);

    return result;
}

ElemAPI void ElemExitApplication()
{
    // TODO:
    SystemLogErrorMessage(ElemLogMessageCategory_Application, "Not implemented.");
}

#include "UNativeDialogs.h"

#include <dlfcn.h>


#define __lnd_xstr(s) __lnd_str(s)
#define __lnd_str(s) #s

#include <stdio.h>

static class DialogBackend {
public:
    DialogBackend() {
        void* handle = nullptr;
        const char* backend = getenv("LND_BACKEND");
        
        auto loadBackend = [&]() {
            char backend_name[64];
            snprintf(backend_name, sizeof(backend_name), "libLND-%s.so", backend);
            printf("Trying to load: %s\n", backend_name);
            handle = dlopen(backend_name, RTLD_LAZY);
            if(!handle)
                return false;
    #define FuncPointer(name) name = (decltype(&(::name)))dlsym(handle, __lnd_xstr(name));
            FuncPointer(UFileDialog_Create);
            FuncPointer(UFileDialog_ProcessEvents);
            FuncPointer(UFileDialog_Destroy);
            FuncPointer(UFileDialog_Result);
            FuncPointer(UFontDialog_Create);
            FuncPointer(UFontDialog_ProcessEvents);
            FuncPointer(UFontDialog_Destroy);
            FuncPointer(UFontDialog_Result);
            
            printf("LND loaded backend: %s\n", backend);
            return true;
        };
        if(backend) {
            loadBackend();
        } else {
            const char* backends[] = {"qt5", "qt4", "gtk3", "gtk2", nullptr};
            const char** backends_iter = backends;
            for(; *backends_iter ; backends_iter++) {
                backend = *backends_iter;
                if(loadBackend())
                    break;
            }
            if(!handle) {
                printf("Couldn't load any LND backend, functions will act like mock ones\n");
            }
        }
        

    }
#undef FuncPointer
#define FuncPointer(name) decltype(&(::name)) name
    FuncPointer(UFileDialog_Create);
    FuncPointer(UFileDialog_ProcessEvents);
    FuncPointer(UFileDialog_Destroy);
    FuncPointer(UFileDialog_Result);
    FuncPointer(UFontDialog_Create);
    FuncPointer(UFontDialog_ProcessEvents);
    FuncPointer(UFontDialog_Destroy);
    FuncPointer(UFontDialog_Result);
} backend;



UFileDialog* UFileDialog_Create(UFileDialogHints *hints)
{
    if(backend.UFileDialog_Create) return backend.UFileDialog_Create(hints);
    return nullptr;
}

bool UFileDialog_ProcessEvents(UFileDialog* handle)
{
    if(backend.UFileDialog_ProcessEvents) return backend.UFileDialog_ProcessEvents(handle);
    return false;
}

void UFileDialog_Destroy(UFileDialog* handle)
{
    if(backend.UFileDialog_Destroy) backend.UFileDialog_Destroy(handle);
}

const UFileDialogResult* UFileDialog_Result(UFileDialog* handle)
{
    if(backend.UFileDialog_Result) return backend.UFileDialog_Result(handle);
    return nullptr;
}

UFontDialog* UFontDialog_Create(UFontDialogHints *hints)
{
    if(backend.UFontDialog_Create) return backend.UFontDialog_Create(hints);
    return nullptr;
}

bool UFontDialog_ProcessEvents(UFontDialog* handle)
{
    if(backend.UFontDialog_ProcessEvents) return backend.UFontDialog_ProcessEvents(handle);
    return false;
}

void UFontDialog_Destroy(UFontDialog* handle)
{
    if(backend.UFontDialog_Destroy) backend.UFontDialog_Destroy(handle);
}

const UFontDialogResult* UFontDialog_Result(UFontDialog* handle)
{
    if(backend.UFontDialog_Result) return backend.UFontDialog_Result(handle);
    return nullptr;
}

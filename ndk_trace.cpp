#include <jni.h>
#include <include/AndHook.h>

//-------------------------------------------------------------------------

enum class TraceOutputMode
{
    kFile,
    kDDMS,
    kStreaming
};
enum class TraceMode
{
    kMethodTracing,
    kSampling
};
// http://and.rlib.cf/8.0.0_r4/xref/art/runtime/trace.cc
static void(*start_)(const char *trace_filename, int trace_fd, size_t buffer_size,
                     int flags, TraceOutputMode output_mode, TraceMode trace_mode, int interval_us);
static void(*shutdown_)();

//-------------------------------------------------------------------------

static void start_method_tracing(JavaVM *vm)
{
    auto art_library_handle_ = AKGetImageByName(AK_ANDROID_RUNTIME);

    start_ = reinterpret_cast<void(*)(const char *, int, size_t, int, TraceOutputMode, TraceMode, int)>(AKFindSymbol(art_library_handle_, "_ZN3art5Trace5StartEPKcijiNS0_15TraceOutputModeENS0_9TraceModeEi"));

    shutdown_ = reinterpret_cast<void(*)()>(AKFindSymbol(art_library_handle_, "_ZN3art5Trace8ShutdownEv"));

    AKCloseImage(art_library_handle_);

    pthread_t th;
    pthread_create(&th, NULL, [](void *jvm)->void * {
        JNIEnv *env;
        reinterpret_cast<JavaVM *>(jvm)->AttachCurrentThreadAsDaemon(&env, NULL);

        start_("/data/local/tmp/trace.txt", -1, 4096, 0, TraceOutputMode::kFile, TraceMode::kMethodTracing, 0);

        // tracing...
        usleep(5 * 1000 * 1000);

        shutdown_();

        // thread stop.
        reinterpret_cast<JavaVM *>(jvm)->DetachCurrentThread();
        pthread_exit(NULL);
    }, vm);
    pthread_detach(th);
    
//    while (true) usleep(987654121);
}

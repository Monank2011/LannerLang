#include "compiler_driver.hpp"
#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

std::string findSourceRoot(const std::string& inputPath) {
    std::filesystem::path p = std::filesystem::absolute(inputPath).parent_path();
    for (;;) {
        if (std::filesystem::exists(p / "src" / "runtime" / "lanner_runtime.c")) return p.string();
        const auto parent = p.parent_path();
        if (parent == p) break;
        p = parent;
    }
    if (const char* env = std::getenv("LANNER_ROOT")) return env;
    return std::filesystem::current_path().string();
}

std::string mobileTriple(const CompilerOptions& o) {
    if (!o.targetTriple.empty()) return o.targetTriple;
    if (o.mobilePlatform == "ios") {
        return std::string(o.iosSimulator ? "arm64-apple-ios" : "arm64-apple-ios") + o.mobileDeployment + (o.iosSimulator ? "-simulator" : "");
    }
    if (o.mobileAbi == "armeabi-v7a" || o.mobileAbi == "armv7") return "armv7a-linux-androideabi" + o.mobileDeployment;
    if (o.mobileAbi == "x86_64") return "x86_64-linux-android" + o.mobileDeployment;
    if (o.mobileAbi == "x86") return "i686-linux-android" + o.mobileDeployment;
    return "aarch64-linux-android" + o.mobileDeployment;
}

std::string cHeaderType(const TypeNode* t) {
    if (!t || t->isOptional || t->isArray || t->fixedArraySize.has_value() || !t->generics.empty()) return {};
    if (t->isRawPointer || t->isReference) return t->isMutable ? "void*" : "const void*";
    if (t->name == "void") return "void";
    if (t->name == "bool") return "int32_t";
    if (t->name == "u8") return "uint8_t";
    if (t->name == "u16") return "uint16_t";
    if (t->name == "u32") return "uint32_t";
    if (t->name == "u64") return "uint64_t";
    if (t->name == "usize") return "uintptr_t";
    if (t->name == "i8") return "int8_t";
    if (t->name == "i16") return "int16_t";
    if (t->name == "i32") return "int32_t";
    if (t->name == "i64") return "int64_t";
    if (t->name == "isize") return "intptr_t";
    if (t->name == "f32") return "float";
    if (t->name == "f64") return "double";
    if (t->name == "string") return "const char*";
    return {};
}

std::string apiHeader(const Program& program) {
    std::ostringstream out;
    out << "#ifndef LANNER_MOBILE_API_H\n#define LANNER_MOBILE_API_H\n#include <stdint.h>\n#include <stddef.h>\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n";
    for (const auto& decl : program.decls) {
        if (decl->kind != DeclKind::Function || decl->fn->isExtern) continue;
        const auto& fn = *decl->fn;
        const auto ret = cHeaderType(fn.returnType.get());
        if (ret.empty()) continue;
        std::vector<std::string> params;
        bool ok = true;
        for (const auto& p : fn.params) {
            const auto type = cHeaderType(p.type.get());
            if (type.empty()) { ok = false; break; }
            params.push_back(type + " " + p.name);
        }
        if (!ok) continue;
        out << ret << " " << fn.name << "(";
        if (params.empty()) out << "void";
        else for (std::size_t i = 0; i < params.size(); ++i) { if (i) out << ", "; out << params[i]; }
        out << ");\n";
    }
    out << "int32_t lanner_app_main(void);\n\n#ifdef __cplusplus\n}\n#endif\n#endif\n";
    return out.str();
}

std::string androidBridge() {
    return R"C(#include <jni.h>
#include <android/log.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "lanner_mobile_android.h"

extern int main(void);
extern void __lanner_mobile_set_metrics(int32_t,int32_t,double,int32_t,int32_t,int32_t,int32_t);

static JavaVM* g_vm = NULL;
static jobject g_activity = NULL;
static char g_string[4096];

static JNIEnv* env_now(void) {
    if (!g_vm) return NULL;
    JNIEnv* env = NULL;
    if ((*g_vm)->GetEnv(g_vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) != JNI_OK) return NULL;
    }
    return env;
}

static jmethodID method(const char* name, const char* signature) {
    JNIEnv* env = env_now(); if (!env || !g_activity) return NULL;
    jclass cls = (*env)->GetObjectClass(env, g_activity);
    if (!cls) return NULL;
    jmethodID m = (*env)->GetMethodID(env, cls, name, signature);
    (*env)->DeleteLocalRef(env, cls);
    return m;
}

static int call_bool_string(const char* name, const char* value) {
    JNIEnv* env = env_now(); jmethodID m = method(name, "(Ljava/lang/String;)Z");
    if (!env || !m) return 0;
    jstring arg = (*env)->NewStringUTF(env, value ? value : "");
    if (!arg) return 0;
    jboolean result = (*env)->CallBooleanMethod(env, g_activity, m, arg);
    (*env)->DeleteLocalRef(env, arg);
    return result ? 1 : 0;
}

static int call_int_string(const char* name, const char* value) {
    JNIEnv* env = env_now(); jmethodID m = method(name, "(Ljava/lang/String;)I");
    if (!env || !m) return -1;
    jstring arg = (*env)->NewStringUTF(env, value ? value : "");
    if (!arg) return -1;
    jint result = (*env)->CallIntMethod(env, g_activity, m, arg);
    (*env)->DeleteLocalRef(env, arg);
    return (int)result;
}

static const char* call_string(const char* name) {
    JNIEnv* env = env_now(); jmethodID m = method(name, "()Ljava/lang/String;");
    if (!env || !m) return "";
    jstring value = (jstring)(*env)->CallObjectMethod(env, g_activity, m);
    if (!value) { g_string[0] = 0; return g_string; }
    const char* utf = (*env)->GetStringUTFChars(env, value, NULL);
    snprintf(g_string, sizeof(g_string), "%s", utf ? utf : "");
    if (utf) (*env)->ReleaseStringUTFChars(env, value, utf);
    (*env)->DeleteLocalRef(env, value);
    return g_string;
}

static int call_bool_noarg(const char* name) {
    JNIEnv* env = env_now(); jmethodID m = method(name, "()Z");
    if (!env || !m) return 0;
    return (*env)->CallBooleanMethod(env, g_activity, m) ? 1 : 0;
}

void __lanner_mobile_log(const char* s) { __android_log_print(ANDROID_LOG_INFO, "Lanner", "%s", s ? s : ""); }
const char* __lanner_mobile_platform(void) { return "android"; }
const char* __lanner_mobile_os_version(void) { return call_string("lannerOsVersion"); }
int32_t __lanner_mobile_is_simulator(void) { return call_bool_noarg("lannerIsSimulator"); }
const char* __lanner_mobile_app_data_path(void) { return call_string("lannerAppDataPath"); }
const char* __lanner_mobile_documents_path(void) { return call_string("lannerDocumentsPath"); }
const char* __lanner_mobile_cache_path(void) { return call_string("lannerCachePath"); }
static int32_t g_w, g_h, g_top, g_bottom, g_left, g_right; static double g_scale = 1.0;
void __lanner_mobile_set_metrics(int32_t w,int32_t h,double scale,int32_t top,int32_t bottom,int32_t left,int32_t right) { g_w=w;g_h=h;g_scale=scale;g_top=top;g_bottom=bottom;g_left=left;g_right=right; }
int32_t __lanner_mobile_screen_width(void){return g_w;} int32_t __lanner_mobile_screen_height(void){return g_h;} double __lanner_mobile_device_scale(void){return g_scale;}
int32_t __lanner_mobile_safe_top(void){return g_top;} int32_t __lanner_mobile_safe_bottom(void){return g_bottom;} int32_t __lanner_mobile_safe_left(void){return g_left;} int32_t __lanner_mobile_safe_right(void){return g_right;}
int32_t __lanner_mobile_open_url(const char* u){return call_bool_string("lannerOpenUrl",u);}
int32_t __lanner_mobile_vibrate(int32_t ms){JNIEnv* env=env_now();jmethodID m=method("lannerVibrate","(I)Z");if(!env||!m)return 0;return (*env)->CallBooleanMethod(env,g_activity,m,(jint)ms)?1:0;}
int32_t __lanner_mobile_request_permission(const char* p){return call_int_string("lannerRequestPermission",p);}
int32_t __lanner_mobile_clipboard_set(const char* t){return call_bool_string("lannerClipboardSet",t);}
void* __lanner_mobile_clipboard_get(void){extern void* __lanner_buffer_from_string(const char*);return __lanner_buffer_from_string(call_string("lannerClipboardGet"));}
static int feature(const char* s){return call_bool_string("lannerHasFeature",s);}
int32_t __lanner_mobile_camera_available(void){return feature("camera");} int32_t __lanner_mobile_location_available(void){return feature("location");} int32_t __lanner_mobile_bluetooth_available(void){return feature("bluetooth");}
JNIEXPORT jint JNICALL Java_com_lanner_generated_MainActivity_nativeMain(JNIEnv* env,jobject activity){(void)env;(void)activity;return (jint)main();}
JNIEXPORT void JNICALL Java_com_lanner_generated_MainActivity_nativeConfigure(JNIEnv* env,jobject activity,jint w,jint h,jfloat d,jint top,jint bottom,jint left,jint right){if(!g_vm)(void)(*env)->GetJavaVM(env,&g_vm);if(!g_activity)g_activity=(*env)->NewGlobalRef(env,activity);__lanner_mobile_set_metrics((int32_t)w,(int32_t)h,(double)d,(int32_t)top,(int32_t)bottom,(int32_t)left,(int32_t)right);}
JNIEXPORT void JNICALL Java_com_lanner_generated_MainActivity_nativeDispose(JNIEnv* env,jobject activity){(void)activity;if(g_activity){(*env)->DeleteGlobalRef(env,g_activity);g_activity=NULL;}}
int32_t lanner_app_main(void){return (int32_t)main();}
)C";
}

std::string androidHeader() {
    return "#pragma once\n#include <stdint.h>\nvoid __lanner_mobile_set_metrics(int32_t,int32_t,double,int32_t,int32_t,int32_t,int32_t);\n";
}

std::string androidActivity(const CompilerOptions&) {
    return R"KOT(package com.lanner.generated

import android.Manifest
import android.app.Activity
import android.content.ClipData
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.VibrationEffect
import android.os.Vibrator
import android.os.VibratorManager
import android.view.WindowInsets
import android.widget.LinearLayout
import android.widget.TextView
import android.content.ClipboardManager as AndroidClipboardManager
import java.io.File

class MainActivity : Activity() {
    companion object { init { System.loadLibrary("lanner_app") } }
    external fun nativeMain(): Int
    external fun nativeConfigure(width: Int, height: Int, density: Float, top: Int, bottom: Int, left: Int, right: Int)
    external fun nativeDispose()

    private fun configureMetrics() {
        val root = window.decorView
        val dm = resources.displayMetrics
        if (Build.VERSION.SDK_INT >= 30) {
            root.setOnApplyWindowInsetsListener { _, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                nativeConfigure(root.width, root.height, dm.density, bars.top, bars.bottom, bars.left, bars.right)
                insets
            }
            root.requestApplyInsets()
        } else {
            nativeConfigure(root.width, root.height, dm.density, 0, 0, 0, 0)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(48, 72, 48, 48)
            setBackgroundColor(Color.WHITE)
        }
        val title = TextView(this).apply {
            text = "Lanner Mobile"
            textSize = 28f
            setTextColor(Color.BLACK)
        }
        val status = TextView(this).apply {
            text = "Starting native Lanner core..."
            textSize = 16f
            setTextColor(Color.DKGRAY)
        }
        root.addView(title)
        root.addView(status)
        setContentView(root)
        window.decorView.post {
            configureMetrics()
            status.text = "Lanner native core returned: ${nativeMain()}"
        }
    }

    override fun onDestroy() {
        nativeDispose()
        super.onDestroy()
    }

    fun lannerOsVersion(): String = Build.VERSION.RELEASE ?: "unknown"
    fun lannerIsSimulator(): Boolean = Build.FINGERPRINT.startsWith("generic") || Build.MODEL.contains("Emulator") || Build.MODEL.contains("sdk")
    fun lannerAppDataPath(): String = filesDir.absolutePath
    fun lannerDocumentsPath(): String = File(getExternalFilesDir(null) ?: filesDir, "documents").apply { mkdirs() }.absolutePath
    fun lannerCachePath(): String = cacheDir.absolutePath
    fun lannerOpenUrl(url: String): Boolean = try { startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(url))); true } catch (_: Exception) { false }
    fun lannerVibrate(ms: Int): Boolean = try { if (Build.VERSION.SDK_INT >= 31) getSystemService(VibratorManager::class.java).defaultVibrator.vibrate(VibrationEffect.createOneShot(ms.toLong(), VibrationEffect.DEFAULT_AMPLITUDE)) else @Suppress("DEPRECATION") getSystemService(VIBRATOR_SERVICE).let { (it as Vibrator).vibrate(ms.toLong()) }; true } catch (_: Exception) { false }
    fun lannerClipboardSet(text: String): Boolean = try { (getSystemService(CLIPBOARD_SERVICE) as AndroidClipboardManager).setPrimaryClip(ClipData.newPlainText("Lanner", text)); true } catch (_: Exception) { false }
    fun lannerClipboardGet(): String = try { val c=(getSystemService(CLIPBOARD_SERVICE) as AndroidClipboardManager).primaryClip; if(c != null && c.itemCount>0) c.getItemAt(0).coerceToText(this).toString() else "" } catch (_: Exception) { "" }
    fun lannerHasFeature(kind: String): Boolean = when(kind) {
        "camera" -> packageManager.hasSystemFeature(PackageManager.FEATURE_CAMERA_ANY)
        "location" -> packageManager.hasSystemFeature(PackageManager.FEATURE_LOCATION_GPS) || packageManager.hasSystemFeature(PackageManager.FEATURE_LOCATION_NETWORK)
        "bluetooth" -> packageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH)
        else -> false
    }
    fun lannerRequestPermission(kind: String): Int {
        if (kind == "bluetooth" && Build.VERSION.SDK_INT >= 31) {
            val perms = arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
            return if (perms.all { checkSelfPermission(it) == PackageManager.PERMISSION_GRANTED }) 1 else { requestPermissions(perms, 4203); 0 }
        }
        val p = when(kind) { "camera" -> Manifest.permission.CAMERA; "location" -> Manifest.permission.ACCESS_FINE_LOCATION; else -> null } ?: return -1
        return if (checkSelfPermission(p) == PackageManager.PERMISSION_GRANTED) 1 else { requestPermissions(arrayOf(p), 4201); 0 }
    }
}
)KOT";
}
std::string iOSBridge() {
    return R"M(#import <TargetConditionals.h>
#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreLocation/CoreLocation.h>
#import <CoreBluetooth/CoreBluetooth.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "lanner_ios_bridge.h"
extern int main(void);
static int32_t W=0,H=0,T=0,B=0,L=0,R=0; static double D=1.0; static char S[4096]; static CLLocationManager* locationManager;
void __lanner_mobile_log(const char* s){NSLog(@"[Lanner] %s",s?s:"");}
const char* __lanner_mobile_platform(void){return "ios";}
const char* __lanner_mobile_os_version(void){snprintf(S,sizeof(S),"%s",UIDevice.currentDevice.systemVersion.UTF8String?:"unknown");return S;}
int32_t __lanner_mobile_is_simulator(void){
#if TARGET_OS_SIMULATOR
return 1;
#else
return 0;
#endif
}
int32_t __lanner_mobile_screen_width(void){return W;} int32_t __lanner_mobile_screen_height(void){return H;} double __lanner_mobile_device_scale(void){return D;} int32_t __lanner_mobile_safe_top(void){return T;} int32_t __lanner_mobile_safe_bottom(void){return B;} int32_t __lanner_mobile_safe_left(void){return L;} int32_t __lanner_mobile_safe_right(void){return R;}
void __lanner_mobile_set_metrics(int32_t w,int32_t h,double d,int32_t t,int32_t b,int32_t l,int32_t r){W=w;H=h;D=d;T=t;B=b;L=l;R=r;}
int32_t __lanner_mobile_open_url(const char* u){NSURL*url=[NSURL URLWithString:[NSString stringWithUTF8String:u?u:""]];if(!url)return 0;dispatch_async(dispatch_get_main_queue(),^{[[UIApplication sharedApplication]openURL:url options:@{} completionHandler:nil];});return 1;}
int32_t __lanner_mobile_vibrate(int32_t ms){(void)ms;dispatch_async(dispatch_get_main_queue(),^{UIImpactFeedbackGenerator*g=[[UIImpactFeedbackGenerator alloc]initWithStyle:UIImpactFeedbackStyleMedium];[g impactOccurred];});return 1;}
int32_t __lanner_mobile_request_permission(const char* p){NSString*k=[NSString stringWithUTF8String:p?p:""];if([k isEqualToString:@"camera"]){AVAuthorizationStatus a=[AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];if(a==AVAuthorizationStatusAuthorized)return 1;if(a==AVAuthorizationStatusNotDetermined){[AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(__unused BOOL ok){}];return 0;}return -1;}if([k isEqualToString:@"location"]){if(!CLLocationManager.locationServicesEnabled)return -1;if(!locationManager)locationManager=[CLLocationManager new];[locationManager requestWhenInUseAuthorization];return 0;}return 0;}
int32_t __lanner_mobile_clipboard_set(const char*t){UIPasteboard.generalPasteboard.string=[NSString stringWithUTF8String:t?t:""];return 1;}void*__lanner_mobile_clipboard_get(void){extern void* __lanner_buffer_from_string(const char*);snprintf(S,sizeof(S),"%s",UIPasteboard.generalPasteboard.string.UTF8String?:"");return __lanner_buffer_from_string(S);}
int32_t __lanner_mobile_camera_available(void){return [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo]!=nil;}int32_t __lanner_mobile_location_available(void){return CLLocationManager.locationServicesEnabled?1:0;}int32_t __lanner_mobile_bluetooth_available(void){return CBCentralManager.authorization!=CBManagerAuthorizationDenied?1:0;}
const char*__lanner_mobile_app_data_path(void){NSString*p=NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,NSUserDomainMask,YES).firstObject?:@"";snprintf(S,sizeof(S),"%s",p.UTF8String);return S;}const char*__lanner_mobile_documents_path(void){NSString*p=NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject?:@"";snprintf(S,sizeof(S),"%s",p.UTF8String);return S;}const char*__lanner_mobile_cache_path(void){NSString*p=NSSearchPathForDirectoriesInDomains(NSCachesDirectory,NSUserDomainMask,YES).firstObject?:@"";snprintf(S,sizeof(S),"%s",p.UTF8String);return S;}
int32_t lanner_app_main(void){return (int32_t)main();}
)M";
}
std::string iosHeader() { return "#import <Foundation/Foundation.h>\n#include <stdint.h>\nint32_t lanner_app_main(void);\n"; }

} // namespace

std::string CompilerDriver::mobileAbiTriple(const CompilerOptions& options) { return mobileTriple(options); }

std::string CompilerDriver::makeMobileHeader(const Program& program, const std::string&) { return apiHeader(program); }

int CompilerDriver::generateAndroidProject(const CompilerOptions& options, const std::string& source) {
    Lexer lexer(source); Parser parser(lexer.tokenize()); Program program=parser.parseProgram();
    const std::filesystem::path dir(options.mobileProjectPath); const auto root=std::filesystem::path(findSourceRoot(options.inputPath)); std::error_code ec;
    std::filesystem::create_directories(dir/"app/src/main/lanner",ec); std::filesystem::create_directories(dir/"app/src/main/cpp",ec); std::filesystem::create_directories(dir/"app/src/main/java/com/lanner/generated",ec); std::filesystem::create_directories(dir/"app/src/main/res/values",ec); if(ec) throw std::runtime_error("cannot create Android project");
    const auto write=[&](const std::string& n,const std::string& v){if(!writeTextFile((dir/n).string(),v))throw std::runtime_error("cannot write Android project file '"+(dir/n).string()+"'");};
    write("app/src/main/lanner/app.lan",source); write("app/src/main/cpp/lanner_runtime.c",readFile((root/"src/runtime/lanner_runtime.c").string())); write("app/src/main/cpp/lanner_mobile_android.c",androidBridge()); write("app/src/main/cpp/lanner_mobile_android.h",androidHeader()); write("app/src/main/cpp/lanner_api.h",apiHeader(program));
    const std::string triple=mobileTriple(options);
    write("app/src/main/cpp/CMakeLists.txt", "cmake_minimum_required(VERSION 3.22.1)\nproject(lanner_app C)\nif(NOT DEFINED LANNERC_EXECUTABLE)\n  find_program(LANNERC_EXECUTABLE lanner REQUIRED)\nendif()\nset(LANNER_OBJ ${CMAKE_CURRENT_BINARY_DIR}/lanner_app.o)\nadd_custom_command(OUTPUT ${LANNER_OBJ} COMMAND ${LANNERC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../lanner/app.lan --target="+triple+" --no-runtime --emit-object -O3 -o ${LANNER_OBJ} DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lanner/app.lan ${LANNERC_EXECUTABLE})\nadd_library(lanner_app SHARED lanner_runtime.c lanner_mobile_android.c ${LANNER_OBJ})\ntarget_include_directories(lanner_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})\nfind_library(log-lib log)\ntarget_link_libraries(lanner_app ${log-lib})\n");
    write("settings.gradle.kts", "import org.gradle.api.initialization.resolve.RepositoriesMode\npluginManagement { repositories { google(); mavenCentral(); gradlePluginPortal() } }\ndependencyResolutionManagement { repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS); repositories { google(); mavenCentral() } }\nrootProject.name = \""+options.mobileAppName+"\"\ninclude(\":app\")\n");
    write("build.gradle.kts", "plugins { id(\"com.android.application\") version \"8.10.1\" apply false }\n");
    write("gradle.properties", "org.gradle.jvmargs=-Xmx2g\nandroid.useAndroidX=true\n");
    write("app/build.gradle.kts", "plugins { id(\"com.android.application\") }\nandroid { namespace=\"com.lanner.generated\"; compileSdk=35; defaultConfig { applicationId=\""+options.mobileBundleId+"\"; minSdk="+options.mobileDeployment+"; targetSdk=35; versionCode=1; versionName=\"1.0\"; ndk { abiFilters += \""+options.mobileAbi+"\" }; externalNativeBuild { cmake { arguments += \"-DANDROID_TRIPLE="+triple+"\", \"-DLANNERC_EXECUTABLE=\" + (project.findProperty(\"lanner\") ?: \"lanner\") } } }; externalNativeBuild { cmake { path=file(\"src/main/cpp/CMakeLists.txt\"); version=\"3.22.1\" } } }\n");
    write("app/src/main/res/values/styles.xml", "<resources><style name=\"Theme.LannerApp\" parent=\"android:style/Theme.Material.Light.NoActionBar\" /></resources>\n");
    write("app/src/main/AndroidManifest.xml", "<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"><uses-permission android:name=\"android.permission.INTERNET\"/><uses-permission android:name=\"android.permission.CAMERA\"/><uses-permission android:name=\"android.permission.ACCESS_FINE_LOCATION\"/><uses-permission android:name=\"android.permission.BLUETOOTH_SCAN\" android:usesPermissionFlags=\"neverForLocation\"/><uses-permission android:name=\"android.permission.BLUETOOTH_CONNECT\"/><application android:theme=\"@style/Theme.LannerApp\" android:label=\""+options.mobileAppName+"\"><activity android:name=\"com.lanner.generated.MainActivity\" android:exported=\"true\"><intent-filter><action android:name=\"android.intent.action.MAIN\"/><category android:name=\"android.intent.category.LAUNCHER\"/></intent-filter></activity></application></manifest>\n");
    write("app/src/main/java/com/lanner/generated/MainActivity.kt", androidActivity(options));
    write("README.md", "# "+options.mobileAppName+" Android\n\nGenerated by Lanner. Android's official NDK/CMake integration builds native libraries into the app package; Lanner emits the target object while the NDK links the runtime and JNI bridge.\n\nBuild with Android Studio/Gradle and pass `-Planner=/path/to/lanner`. ABI: `"+options.mobileAbi+"`. Target: `"+triple+"`.\n");
    return 0;
}

int CompilerDriver::generateIOSProject(const CompilerOptions& options, const std::string& source) {
    Lexer lexer(source); Parser parser(lexer.tokenize()); Program program=parser.parseProgram();
    const std::filesystem::path dir(options.mobileProjectPath); const auto root=std::filesystem::path(findSourceRoot(options.inputPath)); std::error_code ec;
    std::filesystem::create_directories(dir/"LannerApp/Generated",ec); if(ec) throw std::runtime_error("cannot create iOS project");
    const auto write=[&](const std::string& n,const std::string& v){if(!writeTextFile((dir/n).string(),v))throw std::runtime_error("cannot write iOS project file '"+(dir/n).string()+"'");};
    write("LannerApp/main.lan",source); write("LannerApp/lanner_runtime.c",readFile((root/"src/runtime/lanner_runtime.c").string())); write("LannerApp/lanner_api.h",apiHeader(program)); write("LannerApp/lanner_ios_bridge.h",iosHeader()); write("LannerApp/lanner_ios_bridge.m",iOSBridge()); write("LannerApp/LannerApp-Bridging-Header.h","#import \"lanner_api.h\"\n#import \"lanner_ios_bridge.h\"\n");
    write("LannerApp/LannerApp.swift", "import SwiftUI\n@main struct LannerApp: App { var body: some Scene { WindowGroup { ContentView() } } }\nstruct ContentView: View { @State private var result: Int32 = lanner_app_main(); var body: some View { VStack(spacing:16) { Text(\""+options.mobileAppName+"\").font(.largeTitle); Text(\"Lanner native core returned: \\(result)\") }.padding() } }\n");
    write("LannerApp/Info.plist", "<?xml version=\"1.0\" encoding=\"UTF-8\"?><plist version=\"1.0\"><dict><key>CFBundleDisplayName</key><string>"+options.mobileAppName+"</string><key>CFBundleIdentifier</key><string>"+options.mobileBundleId+"</string><key>NSCameraUsageDescription</key><string>Camera access requested by Lanner.</string><key>NSLocationWhenInUseUsageDescription</key><string>Location access requested by Lanner.</string><key>NSBluetoothAlwaysUsageDescription</key><string>Bluetooth access requested by Lanner.</string><key>UILaunchScreen</key><dict/></dict></plist>\n");
    write("LannerApp/build_lanner.sh", "#!/bin/sh\nset -eu\nmkdir -p \"$SRCROOT/LannerApp/Generated\"\n\"${LANNERC_EXECUTABLE:-lanner}\" \"$SRCROOT/LannerApp/main.lan\" --target="+mobileTriple(options)+" --no-runtime --emit-object -O3 -o \"$SRCROOT/LannerApp/Generated/lanner_app.o\"\n");
    const auto proj=dir/(options.mobileAppName+".xcodeproj"); std::filesystem::create_directories(proj,ec);
    const char* iosSdk = options.iosSimulator ? "iphonesimulator" : "iphoneos";
    std::ostringstream pbx;
    pbx << "// !$*UTF8*$!\n{\n  archiveVersion = 1;\n  objectVersion = 56;\n  objects = {\n";
    pbx << "A00000000000000000000001 = {isa = PBXFileReference; lastKnownFileType = sourcecode.swift; path = LannerApp.swift; sourceTree = \"<group>\"; };\n";
    pbx << "A00000000000000000000002 = {isa = PBXFileReference; lastKnownFileType = sourcecode.c.objc; path = lanner_ios_bridge.m; sourceTree = \"<group>\"; };\n";
    pbx << "A00000000000000000000003 = {isa = PBXFileReference; lastKnownFileType = sourcecode.c.c; path = lanner_runtime.c; sourceTree = \"<group>\"; };\n";
    pbx << "A00000000000000000000004 = {isa = PBXFileReference; lastKnownFileType = sourcecode.c.h; path = LannerApp-Bridging-Header.h; sourceTree = \"<group>\"; };\n";
    pbx << "A00000000000000000000005 = {isa = PBXFileReference; lastKnownFileType = text.plist.xml; path = Info.plist; sourceTree = \"<group>\"; };\n";
    pbx << "A00000000000000000000006 = {isa = PBXFileReference; explicitFileType = wrapper.application; path = \"" << options.mobileAppName << ".app\"; sourceTree = BUILT_PRODUCTS_DIR; };\n";
    pbx << "A00000000000000000000007 = {isa = PBXFileReference; explicitFileType = wrapper.framework; path = System/Library/Frameworks/UIKit.framework; sourceTree = SDKROOT; };\n";
    pbx << "A00000000000000000000008 = {isa = PBXFileReference; explicitFileType = wrapper.framework; path = System/Library/Frameworks/AVFoundation.framework; sourceTree = SDKROOT; };\n";
    pbx << "A00000000000000000000009 = {isa = PBXFileReference; explicitFileType = wrapper.framework; path = System/Library/Frameworks/CoreLocation.framework; sourceTree = SDKROOT; };\n";
    pbx << "A0000000000000000000000A = {isa = PBXFileReference; explicitFileType = wrapper.framework; path = System/Library/Frameworks/CoreBluetooth.framework; sourceTree = SDKROOT; };\n";
    pbx << "A0000000000000000000000B = {isa = PBXBuildFile; fileRef = A00000000000000000000001; };\n";
    pbx << "A0000000000000000000000C = {isa = PBXBuildFile; fileRef = A00000000000000000000002; };\n";
    pbx << "A0000000000000000000000D = {isa = PBXBuildFile; fileRef = A00000000000000000000003; };\n";
    pbx << "A0000000000000000000000E = {isa = PBXBuildFile; fileRef = A00000000000000000000007; };\n";
    pbx << "A0000000000000000000000F = {isa = PBXBuildFile; fileRef = A00000000000000000000008; };\n";
    pbx << "A00000000000000000000010 = {isa = PBXBuildFile; fileRef = A00000000000000000000009; };\n";
    pbx << "A00000000000000000000011 = {isa = PBXBuildFile; fileRef = A0000000000000000000000A; };\n";
    pbx << "A00000000000000000000012 = {isa = PBXGroup; children = (A00000000000000000000001,A00000000000000000000002,A00000000000000000000003,A00000000000000000000004,A00000000000000000000005); path = LannerApp; sourceTree = \"<group>\"; };\n";
    pbx << "A00000000000000000000013 = {isa = PBXGroup; children = (A00000000000000000000007,A00000000000000000000008,A00000000000000000000009,A0000000000000000000000A); name = Frameworks; sourceTree = \"<group>\"; };\n";
    pbx << "A00000000000000000000014 = {isa = PBXGroup; children = (A00000000000000000000012,A00000000000000000000013,A00000000000000000000006); sourceTree = \"<group>\"; };\n";
    pbx << "A00000000000000000000015 = {isa = PBXSourcesBuildPhase; buildActionMask = 2147483647; files = (A0000000000000000000000B,A0000000000000000000000C,A0000000000000000000000D); runOnlyForDeploymentPostprocessing = 0; };\n";
    pbx << "A00000000000000000000016 = {isa = PBXFrameworksBuildPhase; buildActionMask = 2147483647; files = (A0000000000000000000000E,A0000000000000000000000F,A00000000000000000000010,A00000000000000000000011); runOnlyForDeploymentPostprocessing = 0; };\n";
    pbx << "A00000000000000000000017 = {isa = PBXResourcesBuildPhase; buildActionMask = 2147483647; files = (); runOnlyForDeploymentPostprocessing = 0; };\n";
    pbx << "A00000000000000000000018 = {isa = PBXShellScriptBuildPhase; buildActionMask = 2147483647; files = (); shellPath = /bin/sh; shellScript = \"sh \\\"$SRCROOT/LannerApp/build_lanner.sh\\\"\"; };\n";
    pbx << "A00000000000000000000019 = {isa = XCBuildConfiguration; buildSettings = { ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon; CLANG_ENABLE_MODULES = YES; CODE_SIGN_STYLE = Automatic; INFOPLIST_FILE = LannerApp/Info.plist; IPHONEOS_DEPLOYMENT_TARGET = " << options.mobileDeployment << "; LD_RUNPATH_SEARCH_PATHS = \"$(inherited) @executable_path/Frameworks\"; PRODUCT_BUNDLE_IDENTIFIER = " << options.mobileBundleId << "; PRODUCT_NAME = \"$(TARGET_NAME)\"; SDKROOT = "<< std::string(iosSdk) <<"; SWIFT_OBJC_BRIDGING_HEADER = LannerApp/LannerApp-Bridging-Header.h; SWIFT_VERSION = 5.0; TARGETED_DEVICE_FAMILY = \"1,2\"; OTHER_LDFLAGS = \"$(inherited) $(SRCROOT)/LannerApp/Generated/lanner_app.o\"; }; name = Debug; };\n";
    pbx << "A0000000000000000000001A = {isa = XCBuildConfiguration; buildSettings = { ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon; CLANG_ENABLE_MODULES = YES; CODE_SIGN_STYLE = Automatic; INFOPLIST_FILE = LannerApp/Info.plist; IPHONEOS_DEPLOYMENT_TARGET = " << options.mobileDeployment << "; LD_RUNPATH_SEARCH_PATHS = \"$(inherited) @executable_path/Frameworks\"; PRODUCT_BUNDLE_IDENTIFIER = " << options.mobileBundleId << "; PRODUCT_NAME = \"$(TARGET_NAME)\"; SDKROOT = "<< std::string(iosSdk) <<"; SWIFT_OBJC_BRIDGING_HEADER = LannerApp/LannerApp-Bridging-Header.h; SWIFT_OPTIMIZATION_LEVEL = \"-O\"; SWIFT_VERSION = 5.0; TARGETED_DEVICE_FAMILY = \"1,2\"; OTHER_LDFLAGS = \"$(inherited) $(SRCROOT)/LannerApp/Generated/lanner_app.o\"; }; name = Release; };\n";
    pbx << "A0000000000000000000001B = {isa = XCConfigurationList; buildConfigurations = (A00000000000000000000019,A0000000000000000000001A); defaultConfigurationIsVisible = 0; defaultConfigurationName = Release; };\n";
    pbx << "A0000000000000000000001C = {isa = XCBuildConfiguration; buildSettings = { TARGETED_DEVICE_FAMILY = \"1,2\"; }; name = Debug; };\n";
    pbx << "A0000000000000000000001D = {isa = XCBuildConfiguration; buildSettings = { TARGETED_DEVICE_FAMILY = \"1,2\"; }; name = Release; };\n";
    pbx << "A0000000000000000000001E = {isa = XCConfigurationList; buildConfigurations = (A0000000000000000000001C,A0000000000000000000001D); defaultConfigurationIsVisible = 0; defaultConfigurationName = Release; };\n";
    pbx << "A0000000000000000000001F = {isa = PBXNativeTarget; buildConfigurationList = A0000000000000000000001B; buildPhases = (A00000000000000000000015,A00000000000000000000016,A00000000000000000000017,A00000000000000000000018); buildRules = (); dependencies = (); name = \"" << options.mobileAppName << "\"; productName = \"" << options.mobileAppName << "\"; productReference = A00000000000000000000006; productType = \"com.apple.product-type.application\"; };\n";
    pbx << "A00000000000000000000020 = {isa = PBXProject; buildConfigurationList = A0000000000000000000001E; compatibilityVersion = \"Xcode 14.0\"; developmentRegion = en; hasScannedForEncodings = 0; knownRegions = (en,Base); mainGroup = A00000000000000000000014; productRefGroup = A00000000000000000000014; projectDirPath = \"\"; projectRoot = \"\"; targets = (A0000000000000000000001F); };\n";
    pbx << "}; rootObject = A00000000000000000000020; }\n";
    write((options.mobileAppName+".xcodeproj/project.pbxproj"),pbx.str());
    write("README.md", "# "+options.mobileAppName+" iOS\n\nGenerated by Lanner. Open the Xcode project on macOS, set `LANNERC_EXECUTABLE` to the installed `lanner`, and build. Lanner emits the arm64 iOS object; Xcode compiles the Objective-C bridge/runtime and links UIKit, AVFoundation, CoreLocation and CoreBluetooth.\n\nFor Simulator generation, add `--ios-simulator`. Current generator targets iOS "+options.mobileDeployment+".\n");
    return 0;
}

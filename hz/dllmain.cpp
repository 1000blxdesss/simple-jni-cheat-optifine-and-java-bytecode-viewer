#include <thread>
#include <cstdio>
#include <Windows.h>
#include "JNI/jni.h"
#include <string>
#include "JNI/jvmti.h"
#include <iostream>
#include <vector>
using namespace std;

std::atomic<bool> buttonPressed(false);

jclass findClass(const std::string& class_path, JNIEnv* env, jvmtiEnv* tienv)
{
    jclass* loaded_classes = nullptr;
    jint loaded_classes_count = 0;
    jclass found_class = nullptr;
    tienv->GetLoadedClasses(&loaded_classes_count, &loaded_classes);
    for (jint i = 0; i < loaded_classes_count; ++i)
    {
        char* signature_buffer = nullptr;
        tienv->GetClassSignature(loaded_classes[i], &signature_buffer, nullptr);
        std::string signature = signature_buffer;
        tienv->Deallocate((unsigned char*)signature_buffer);
        signature = signature.substr(1);
        signature.pop_back();
        if (signature == class_path)
        {
            found_class = (jclass)env->NewLocalRef(loaded_classes[i]);
            break;
        }
    }
    for (jint i = 0; i < loaded_classes_count; ++i)
    {
        env->DeleteLocalRef(loaded_classes[i]);
    }
    tienv->Deallocate((unsigned char*)loaded_classes);
    return found_class;
}


jclass getMinecraftClass(JNIEnv* p_env, jvmtiEnv* tienv) {
    return findClass("ave", p_env, tienv);
}
jobject getMinecraft(JNIEnv* p_env, jvmtiEnv* tienv) {
    jmethodID getMinecraftMethod = p_env->GetStaticMethodID(getMinecraftClass(p_env, tienv), "A", "()Lave;");
    return p_env->CallStaticObjectMethod(getMinecraftClass(p_env, tienv), getMinecraftMethod);
}
jobject getPlayer(JNIEnv* p_env, jvmtiEnv* tienv) {
    jfieldID getPlayerField = p_env->GetFieldID(getMinecraftClass(p_env, tienv), "h", "Lbew;");
    return p_env->GetObjectField(getMinecraft(p_env, tienv), getPlayerField);
}

void setplayersprite(JNIEnv* p_env, jvmtiEnv* tienv) {
    jmethodID setSprintingMethod = p_env->GetMethodID(p_env->GetObjectClass(getPlayer(p_env, tienv)), "d", "(Z)V");
    p_env->CallBooleanMethod(getPlayer(p_env, tienv), setSprintingMethod, true);
}

void printMethodBytecode(JNIEnv* env, jvmtiEnv* jvmti, const char* className, const char* methodName, const char* methodSignature) {
    jclass targetClass = findClass(className, env, jvmti);
    if (!targetClass) {
        std::cout << "Failed to find class: " << className << std::endl;
        return;
    }

    jmethodID targetMethod = env->GetMethodID(targetClass, methodName, methodSignature);
    if (!targetMethod) {
        std::cout << "Failed to find method: " << methodName << std::endl;
        return;
    }

    unsigned char* bytecode;
    jint bytecode_len;
    jvmtiError error = jvmti->GetBytecodes(targetMethod, &bytecode_len, &bytecode);

    if (error != JVMTI_ERROR_NONE) {
        std::cout << "Failed to get bytecode. Error code: " << error << std::endl;
        return;
    }

    std::cout << "Bytecode for " << className << "." << methodName << ":" << std::endl;
    for (int i = 0; i < bytecode_len; i++) {
        printf("%02X ", bytecode[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");

    jvmti->Deallocate(bytecode);
}




void omg(HMODULE module)
{
    JavaVM* p_jvm = nullptr;
    JNIEnv* p_env = nullptr;
    jvmtiEnv* tienv = nullptr;


    // get JavaVM
    jsize vmCount;
    if (JNI_GetCreatedJavaVMs(&p_jvm, 1, &vmCount) != JNI_OK || vmCount == 0) {
        printf("Failed to get JavaVM\n");
        FreeLibrary(module);
        return;
    }

    //connect thread -> JVM
    jint result = p_jvm->AttachCurrentThread((void**)&p_env, nullptr);
    if (result != JNI_OK || p_env == nullptr) {
        printf("Failed to attach current thread\n");
        FreeLibrary(module);
        return;
    }

    // get JVMTI
    result = p_jvm->GetEnv((void**)&tienv, JVMTI_VERSION_1_2);
    if (result != JNI_OK || tienv == nullptr) {
        printf("Failed to get JVMTI environment\n");
        p_jvm->DetachCurrentThread();
        FreeLibrary(module);
        return;
    }

    printf("Successfully initialized JNI and JVMTI\n");

    jobject player = getPlayer(p_env, tienv);
    if (player == nullptr) {
        printf("Failed to get player object\n");
        FreeLibrary(module);
        return;
    }

    jclass playerClass = p_env->GetObjectClass(player);
    if (playerClass == nullptr) {
        printf("Failed to get player class\n");
        FreeLibrary(module);
        return;
    }

    jfieldID motionXField = p_env->GetFieldID(playerClass, "v", "D");
    if (motionXField == nullptr) {
        printf("Failed to find 'v' field in EntityPlayerSP\n");
        FreeLibrary(module);
        return;
    }

    jfieldID motionYField = p_env->GetFieldID(playerClass, "x", "D");
    if (motionYField == nullptr) {
        printf("Failed to find 'x' field in EntityPlayerSP\n");
        FreeLibrary(module);
        return;
    }//public void f(pk pkVar)
    jclass wnClass = findClass("wn", p_env, tienv);
    if (wnClass == nullptr) {
        std::cout << "Failed to find class wn" << std::endl;
        return;
    }

    jmethodID fMethod = p_env->GetMethodID(wnClass, "f", "(Lpk;)V");
    if (fMethod == nullptr) {
        std::cout << "Failed to find method f in class wn" << std::endl;
        return;
    }
    jvmtiCapabilities capabilities;
    memset(&capabilities, 0, sizeof(jvmtiCapabilities));
    capabilities.can_retransform_classes = 1;
    capabilities.can_retransform_any_class = 1;
    capabilities.can_get_bytecodes = 1;
    capabilities.can_redefine_classes = 1;

    capabilities.can_redefine_any_class = 1;
    jvmtiError error = tienv->AddCapabilities(&capabilities);
    if (error != JVMTI_ERROR_NONE) {
        std::cout << "Failed to add capabilities. Error code: " << error << std::endl;
        return;
    }
   // printMethodBytecode(p_env, tienv, "net/minecraft/entity/player/EntityPlayer", "attackTargetEntityWithCurrentItem", "(Lnet/minecraft/entity/Entity;)V");
    printMethodBytecode(p_env, tienv, "wn", "f", "(Lpk;)V");
     while (true) {
         if (!getMinecraftClass(p_env, tienv))continue;
         if (!getMinecraft(p_env, tienv)) continue;
         if (!getPlayer(p_env, tienv)) continue;
         double motionX = p_env->GetDoubleField(player, motionXField);
         double motionY = p_env->GetDoubleField(player, motionYField);
         p_env->SetDoubleField(player, motionXField, motionX * 1.0);
         p_env->SetDoubleField(player, motionYField, motionY * 1.0);
         printf("motionX: %f, motionY: %f\n", motionX, motionY);
         setplayersprite(p_env, tienv);

     }



     // detach thread
    /* p_jvm->DetachCurrentThread();
     FreeLibrary(module);*/
}
bool __stdcall DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    static FILE* p_file{ nullptr };


    if (reason == DLL_PROCESS_ATTACH)
    {
        AllocConsole();
        freopen_s(&p_file, "CONOUT$", "w", stdout);

        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)omg, 0, 0, 0);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        fclose(p_file);
        FreeConsole();
    }

    return true;
}
